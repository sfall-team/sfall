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
#include "Combat.h"
#include "LoadGameHook.h"

#include "Skills.h"

namespace sfall
{

struct SkillModifier {
	DWORD id { 0 };
	int maximum { 300 };
	int mod { 0 };
};

static std::vector<SkillModifier> skillMaxMods;
static SkillModifier baseSkillMax;
static BYTE skillCosts[512 * fo::SKILL_count];
static DWORD basedOnPoints;
static double* multipliers;

static int _fastcall CheckSkillMax(DWORD critter, int base) {
	for (DWORD i = 0; i < skillMaxMods.size(); i++) {
		if (critter == skillMaxMods[i].id) {
			return min(base, skillMaxMods[i].maximum);
		}
	}
	return min(base, baseSkillMax.maximum);
}

static void __declspec(naked) skill_level_hack() {
	__asm {
		mov  edx, esi;      // level skill (base)
		call CheckSkillMax; // ecx - critter
		mov  edx, 0x4AA64B;
		jmp  edx;
	}
}

static void __declspec(naked) skill_inc_point_force_hack() {
	__asm {
		push ecx;
		push eax;
		mov  edx, 0x7FFFFFFF; // base
		call CheckSkillMax;   // ecx - critter
		pop  edx;             // skill level (from eax)
		cmp  edx, eax;        // eax = max
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) skill_inc_point_hack() {
	__asm {
		push ecx;
		push eax;
		mov  edx, 0x7FFFFFFF; // base
		mov  ecx, esi;        // critter
		call CheckSkillMax;
		pop  edx;             // skill level (from eax)
		cmp  edx, eax;        // eax = max
		pop  ecx;
		retn;
	}
}

static int _fastcall GetStatBonus( fo::GameObject* critter, const fo::SkillInfo* info, int skill, int points) {
	double result = 0;
	for (int i = 0; i < 7; i++) {
		result += fo::func::stat_level(critter, i) * multipliers[skill * 7 + i];
	}
	result += points*info->skillPointMulti;
	result += info->base;
	return (int)result;
}

//On input, ebx/edx contains the skill id, ecx contains the critter, edi contains a SkillInfo*, ebp contains the number of skill points
//On exit ebx, ecx, edi, ebp are preserved, esi contains skill base + stat bonus + skillpoints*multiplier
static const DWORD StatBonusHookRet = 0x4AA5D6;
static void __declspec(naked) skill_level_hack_bonus() {
	__asm {
		push ecx;
		push ebp;
		push ebx;
		mov  edx, edi;
		call GetStatBonus; // ecx - critter
		mov  esi, eax;
		pop  ecx;
		jmp  StatBonusHookRet;
	}
}

static const DWORD SkillIncCostRet = 0x4AA7C1;
static void __declspec(naked) skill_inc_point_hack_cost() {
	__asm { // eax - current skill level, ebx - current skill, ecx - num free skill points
		mov  edx, basedOnPoints;
		test edx, edx;
		jz   next;
		mov  edx, ebx;
		mov  eax, esi;
		call fo::funcoffs::skill_points_;
next:
		mov  edx, ebx;
		shl  edx, 9;
		add  edx, eax;
		movzx eax, skillCosts[edx]; // eax - cost of the skill
		jmp  SkillIncCostRet;
	}
}

static const DWORD SkillDecCostRet = 0x4AA98D;
static void __declspec(naked) skill_dec_point_hack_cost() {
	__asm { // eax - current skill level, ebx - current skill, ecx - num free skill points
		mov  edx, basedOnPoints;
		test edx, edx;
		jz   next;
		mov  edx, ebx;
		mov  eax, edi;
		call fo::funcoffs::skill_points_;
		lea  ecx, [eax - 1];
next:
		mov  edx, ebx;
		shl  edx, 9;
		add  edx, ecx;
		movzx eax, skillCosts[edx]; // eax - cost of the skill
		jmp  SkillDecCostRet;
	}
}

static void __declspec(naked) skill_dec_point_hook_cost() {
	__asm {
		mov   edx, ebx;
		shl   edx, 9;
		add   edx, eax;
		movzx eax, skillCosts[edx];
		retn;
	}
}

void _stdcall SetSkillMax(DWORD critter, DWORD maximum) {
	if (critter == -1) {
		baseSkillMax.maximum = maximum;
		return;
	}
	for (DWORD i = 0; i < skillMaxMods.size(); i++) {
		if (critter == skillMaxMods[i].id) {
			skillMaxMods[i].maximum = maximum;
			return;
		}
	}
	SkillModifier cm;
	cm.id = critter;
	cm.maximum = maximum;
	cm.mod = 0;
	skillMaxMods.push_back(cm);
}

static void Reset_OnGameLoad() {
	skillMaxMods.clear();
	baseSkillMax.maximum = 300;
	baseSkillMax.mod = 0;
}

void Skills::init() {
	MakeJump(0x4AA63C, skill_level_hack, 1);
	MakeCall(0x4AA847, skill_inc_point_force_hack);
	MakeCall(0x4AA725, skill_inc_point_hack);

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

		MakeJump(0x4AA59D, skill_level_hack_bonus, 1);
		MakeJump(0x4AA738, skill_inc_point_hack_cost);
		MakeJump(0x4AA940, skill_dec_point_hack_cost, 1);
		HookCalls(skill_dec_point_hook_cost, { 0x4AA9E1,  0x4AA9F1 });

		basedOnPoints = GetPrivateProfileIntA("Skills", "BasedOnPoints", 0, file);
		if (basedOnPoints) HookCall(0x4AA9EC, (void*)fo::funcoffs::skill_points_); // skill_dec_point_
	}

	LoadGameHook::OnGameReset() += Reset_OnGameLoad;
}

}
