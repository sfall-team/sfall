/*
 *    sfall
 *    Copyright (C) 2011  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdio.h>
#include <vector>

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "Knockback.h"
#include "LoadGameHook.h"

#include "Skills.h"

namespace sfall
{

struct ChanceModifier {
	DWORD id;
	int maximum;
	int mod;
};

static std::vector<ChanceModifier> SkillMaxMods;
static ChanceModifier BaseSkillMax;
static BYTE skillCosts[512 * fo::SKILL_count];
static DWORD basedOnPoints;

static int _stdcall SkillMaxHook2(int base, DWORD critter) {
	for(DWORD i=0;i<SkillMaxMods.size();i++) {
		if(critter==SkillMaxMods[i].id) {
			return min(base, SkillMaxMods[i].maximum);
		}
	}
	return min(base, BaseSkillMax.maximum);
}

static void __declspec(naked) SkillHookA() {
	__asm {
	push ecx;
	push esi;
	call SkillMaxHook2;
	push 0x4AA64B;
	retn;
	}
}

static void __declspec(naked) SkillHookB() {
	__asm {
	push edx;
	push ecx;
	push ebx;
	push eax;
	push ecx;
	push 0x7fffffff;
	call SkillMaxHook2;
	pop  edx;
	cmp  edx, eax;
	pop  ebx;
	pop  ecx;
	pop  edx;
	jl   win;
	push 0x4AA84E;
	retn;
win:
	push 0x4AA85C;
	retn;
	}
}

static const DWORD SkillHookWin = 0x4AA738;
static const DWORD SkillHookFail = 0x4AA72C;
static void __declspec(naked) SkillHookC() {
	__asm {
	pushad;
	push eax;
	push esi;
	push 0x7fffffff;
	call SkillMaxHook2;
	pop edx;
	cmp edx, eax;
	popad;
	jl win;
	jmp SkillHookFail;
win:
	jmp SkillHookWin;
	}
}

void _stdcall SetSkillMax(DWORD critter, DWORD maximum) {
	if (critter == -1) {
		BaseSkillMax.maximum = maximum;
		return;
	}
	for (DWORD i = 0; i < SkillMaxMods.size(); i++) {
		if (critter == SkillMaxMods[i].id) {
			SkillMaxMods[i].maximum = maximum;
			return;
		}
	}
	ChanceModifier cm;
	cm.id = critter;
	cm.maximum = maximum;
	cm.mod = 0;
	SkillMaxMods.push_back(cm);
}

double* multipliers;
static const DWORD StatBonusHookRet = 0x4AA5D6;

static int __declspec(naked) _stdcall stat_level(void* critter, int stat) {
	__asm {
		push edx;
		mov eax, [esp+8];
		mov edx, [esp+12];
		call fo::funcoffs::stat_level_;
		pop edx;
		ret 8;
	}
}

static int _stdcall GetStatBonusHook2(const fo::SkillInfo* info, int skill, int points, void* critter) {
	double result = 0;
	for (int i = 0; i < 7; i++) {
		result += stat_level(critter, i)*multipliers[skill * 7 + i];
	}
	result += points*info->skillPointMulti;
	result += info->base;
	return (int)result;
}

//On input, ebx contains the skill id, ecx contains the critter, edx contains the skill id, edi contains a SkillInfo*, ebp contains the number of skill points
//On exit ebx, ecx, edi, ebp are preserved, esi contains skill base + stat bonus + skillpoints*multiplier
static void __declspec(naked) GetStatBonusHook() {
	__asm {
		push edx;
		push ecx;
		push ecx;
		push ebp;
		push ebx;
		push edi;
		call GetStatBonusHook2;
		mov esi, eax;
		pop ecx;
		pop edx;
		jmp StatBonusHookRet;
	}
}

static const DWORD SkillIncCostRet = 0x4AA7C1;
static void __declspec(naked) SkillIncCostHook() {
	__asm {
		//eax - current skill level, ebx - current skill, ecx - num free skill points
		mov edx, basedOnPoints;
		test edx, edx;
		jz next;
		mov edx, ebx;
		mov eax, esi;
		call fo::funcoffs::skill_points_;
next:
		mov edx, ebx;
		shl edx, 9;
		add edx, eax;
		movzx eax, skillCosts[edx];
		//eax - cost of the skill
		jmp SkillIncCostRet;
	}
}

static const DWORD SkillDecCostRet = 0x4AA98D;
static void __declspec(naked) SkillDecCostHook() {
	__asm {
		//eax - current skill level, ebx - current skill, ecx - num free skill points
		mov edx, basedOnPoints;
		test edx, edx;
		jz next;
		mov edx, ebx;
		mov eax, edi;
		call fo::funcoffs::skill_points_;
next:
		lea ecx, [eax-1];
		mov edx, ebx;
		shl edx, 9;
		add edx, ecx;
		movzx eax, skillCosts[edx];
		//eax - cost of the skill
		jmp SkillDecCostRet;
	}
}

static void __declspec(naked) SkillLevelCostHook() {
	__asm {
		push edx;
		mov edx, ebx;
		shl edx, 9;
		add edx, eax;
		movzx eax, skillCosts[edx];
		pop edx;
		retn;
	}
}

void Skills_OnGameLoad() {
	SkillMaxMods.clear();
	BaseSkillMax.maximum = 300;
	BaseSkillMax.mod = 0;
}

void Skills::init() {
	MakeCall(0x4AA63C, SkillHookA, true);
	MakeCall(0x4AA847, SkillHookB, true);
	MakeCall(0x4AA725, SkillHookC, true);

	char buf[512], key[16], file[64];
	auto skillsFile = GetConfigString("Misc", "SkillsFile", "");
	if (skillsFile.size() > 0) {
		fo::SkillInfo *skills = fo::var::skill_data;
		sprintf(file, ".\\%s", skillsFile.c_str());
		multipliers = new double[7 * fo::SKILL_count];
		memset(multipliers, 0, 7 * fo::SKILL_count * sizeof(double));

		for (int i = 0; i < fo::SKILL_count; i++) {
			sprintf(key, "Skill%d", i);
			if (GetPrivateProfileStringA("Skills", key, "", buf, 64, file)) {
				char* tok = strtok(buf, "|");
				while (tok) {
					if (strlen(tok) >= 2) {
						double m = atof(&tok[1]);
						switch (tok[0]) {
						case 's': multipliers[i * 7 + 0] = m; break;
						case 'p': multipliers[i * 7 + 1] = m; break;
						case 'e': multipliers[i * 7 + 2] = m; break;
						case 'c': multipliers[i * 7 + 3] = m; break;
						case 'i': multipliers[i * 7 + 4] = m; break;
						case 'a': multipliers[i * 7 + 5] = m; break;
						case 'l': multipliers[i * 7 + 6] = m; break;
						default: continue;
						}
					}
					tok = strtok(0, "|");
				}
			} else {
				multipliers[i * 7 + skills[i].statA] = skills[i].statMulti;
				if (skills[i].statB >= 0) multipliers[i * 7 + skills[i].statB] = skills[i].statMulti;
			}
			sprintf(key, "SkillCost%d", i);
			if (GetPrivateProfileStringA("Skills", key, "", buf, 512, file)) {
				char* tok = strtok(buf, "|");
				DWORD upto = 0;
				BYTE price = 1;
				while (tok && upto < 512) {
					if (strlen(tok)) {
						DWORD next = atoi(tok);
						while (upto < next && upto < 512) skillCosts[i * 512 + upto++] = price;
						price++;
					}
					tok = strtok(0, "|");
				}
				while (upto < 512) skillCosts[i * 512 + upto++] = price;
			} else {
				for (int j = 0;   j <= 100; j++) skillCosts[i * 512 + j] = 1;
				for (int j = 101; j <= 125; j++) skillCosts[i * 512 + j] = 2;
				for (int j = 126; j <= 150; j++) skillCosts[i * 512 + j] = 3;
				for (int j = 151; j <= 175; j++) skillCosts[i * 512 + j] = 4;
				for (int j = 176; j <= 200; j++) skillCosts[i * 512 + j] = 5;
				for (int j = 201; j <= 512; j++) skillCosts[i * 512 + j] = 6;
			}
			sprintf(key, "SkillBase%d", i);
			skills[i].base = GetPrivateProfileIntA("Skills", key, skills[i].base, file);
			sprintf(key, "SkillMulti%d", i);
			skills[i].skillPointMulti = GetPrivateProfileIntA("Skills", key, skills[i].skillPointMulti, file);
			sprintf(key, "SkillImage%d", i);
			skills[i].image = GetPrivateProfileIntA("Skills", key, skills[i].image, file);
		}

		MakeCall(0x4AA59D, GetStatBonusHook, true);
		MakeCall(0x4AA738, &SkillIncCostHook, true);
		MakeCall(0x4AA93D, &SkillDecCostHook, true);
		HookCall(0x4AA9E1, &SkillLevelCostHook);
		HookCall(0x4AA9F1, &SkillLevelCostHook);
		basedOnPoints = GetPrivateProfileIntA("Skills", "BasedOnPoints", 0, file);
		if (basedOnPoints) HookCall(0x4AA9EC, (void*)fo::funcoffs::skill_points_);
	}

	LoadGameHook::onGameReset += Skills_OnGameLoad;
}

}
