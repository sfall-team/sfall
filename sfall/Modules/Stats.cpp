/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Stats.h"

namespace sfall
{

static DWORD statMaximumsPC[fo::STAT_max_stat];
static DWORD statMinimumsPC[fo::STAT_max_stat];
static DWORD statMaximumsNPC[fo::STAT_max_stat];
static DWORD statMinimumsNPC[fo::STAT_max_stat];

static DWORD cCritter;

static DWORD xpTable[99];

static void __declspec(naked) GetCurrentStatHook1() {
	__asm {
		mov cCritter, eax;
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		push 0x4AEF4D;
		retn;
	}
}

static void __declspec(naked) GetCurrentStatHook2() {
	__asm {
		shl esi, 2;
		mov eax, cCritter;
		cmp eax, dword ptr ds:[FO_VAR_obj_dude];
		je pc;
		cmp ecx, statMinimumsNPC[esi];
		jg npc1;
		mov eax, statMinimumsNPC[esi];
		jmp end;
npc1:
		cmp ecx, statMaximumsNPC[esi];
		jl npc2;
		mov eax, statMaximumsNPC[esi];
		jmp end;
npc2:
		mov eax, ecx;
		jmp end;
pc:
		cmp ecx, statMinimumsPC[esi];
		jge pc1;
		mov eax, statMinimumsPC[esi];
		jmp end;
pc1:
		cmp ecx, statMaximumsPC[esi];
		jle pc2;
		mov eax, statMaximumsPC[esi];
		jmp end;
pc2:
		mov eax, ecx;
end:
		push 0x4AF3D7;
		retn;
	}
}

static void __declspec(naked) SetCurrentStatHook() {
	__asm {
		cmp esi, dword ptr ds:[FO_VAR_obj_dude];
		je pc;
		cmp ebx, statMinimumsNPC[ecx*4];
		jl fail;
		cmp ebx, statMaximumsNPC[ecx*4];
		jg fail;
		jmp end;
pc:
		cmp ebx, statMinimumsPC[ecx*4];
		jl fail;
		cmp ebx, statMaximumsPC[ecx*4];
		jg fail;
		jmp end;
fail:
		push 0x4AF57E;
		retn;
end:
		push 0x4AF59C;
		retn;
	}
}

static void __declspec(naked) GetLevelXPHook() {
	__asm {
		dec eax;
		mov eax, [xpTable+eax*4];
		ret;
	}
}
static void __declspec(naked) GetNextLevelXPHook() {
	__asm {
		mov eax, ds:[FO_VAR_Level_];
		jmp GetLevelXPHook;
	}
}

unsigned short standardApAcBonus = 4;
unsigned short extraApAcBonus = 4;
static const DWORD ApAcRetAddr = 0x4AF0A4;
static void __declspec(naked) ApplyApAcBonus() {
	using namespace fo;
	__asm {
		push edi;
		push edx;
		cmp [esp+12], 2;
		jge h2hEvade;
		xor edi, edi;
		jmp standard;
h2hEvade:
		mov edx, PERK_hth_evade_perk;
		mov eax, dword ptr ds:[FO_VAR_obj_dude];
		call fo::funcoffs::perk_level_;
		imul ax, extraApAcBonus;
		imul ax, [ebx+0x40];
		mov edi, eax;
standard:
		mov eax, [ebx+0x40];
		imul ax, standardApAcBonus;
		add eax, edi;
		shr eax, 2;
		pop edx;
		pop edi;
		jmp ApAcRetAddr;
	}
}

static int StatFormulas[33 * 2];
static int StatShifts[33 * 7];
static double StatMulti[33 * 7];
static int __declspec(naked) _stdcall StatLevel(void* critter, int id) {
	__asm {
		mov eax, [esp+4];
		mov edx, [esp+8];
		call fo::funcoffs::stat_level_;
		retn 8;
	}
}

static void __declspec(naked) _stdcall ProtoPtr(DWORD pid, int** proto) {
	__asm {
		mov eax, [esp+4];
		mov edx, [esp+8];
		call fo::funcoffs::proto_ptr_;
		retn 8;
	}
}

static void _stdcall StatRecalcDerived(DWORD* critter) {
	int basestats[7];
	for (int i = 0; i < 7; i++) basestats[i] = StatLevel(critter, i);
	int* proto;
	ProtoPtr(critter[25], &proto);

	for (int i = 7; i <= 32; i++) {
		if (i >= 17 && i <= 30) continue;

		double sum = 0;
		for (int j = 0; j < 7; j++) {
			sum += (basestats[j] + StatShifts[i * 7 + j])*StatMulti[i * 7 + j];
		}
		proto[i + 9] = StatFormulas[i * 2] + (int)floor(sum);
		if (proto[i + 9] < StatFormulas[i * 2 + 1]) proto[i + 9] = StatFormulas[i * 2 + 1];
	}
}

static void __declspec(naked) stat_recalc_derived() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call StatRecalcDerived;
		pop ecx;
		pop edx;
		retn;
	}
}

static const DWORD StatSetBaseRet = 0x4AF559;
static void __declspec(naked) stat_set_base_hack() {
	using namespace fo;
	__asm {
		cmp  ecx, STAT_unused;
		je   allow;
		cmp  ecx, STAT_better_crit;
		jl   notAllow;
		cmp  ecx, STAT_dmg_resist_explosion;
		jg   notAllow;
allow:
		pop  eax;      // destroy ret addr
		jmp  StatSetBaseRet;
notAllow:
		mov  eax, -1;  // overwritten engine code
		retn;
	}
}

static const DWORD SetCritterStatRet = 0x455D8A;
static void __declspec(naked) op_set_critter_stat_hack() {
	using namespace fo;
	__asm {
		cmp  dword ptr [esp + 0x2C - 0x28 + 4], STAT_unused;
		je   allow;
		mov  ebx, 3;  // overwritten engine code
		retn;
allow:
		add  esp, 4;  // destroy ret addr
		jmp  SetCritterStatRet;
	}
}

void StatsReset() {
	for (int i = 0; i < fo::STAT_max_stat; i++) {
		statMaximumsPC[i] = statMaximumsNPC[i] = fo::var::stat_data[i].maxValue;
		statMinimumsPC[i] = statMinimumsNPC[i] = fo::var::stat_data[i].minValue;
	}
	standardApAcBonus = 4;
	extraApAcBonus = 4;
}

void Stats::init() {
	StatsReset();

	LoadGameHook::OnGameReset() += []() {
		StatsReset();
		//Reset some settable game values back to the defaults
		//Exp mod
		SafeWrite8(0x4AFAB8, 0x53);
		SafeWrite32(0x4AFAB9, 0x55575651);
		//HP bonus
		SafeWrite8(0x4AFBC1, 2);
		//SkillPoints per level mod
		SafeWrite8(0x43C27A, 5);
	};

	MakeJump(0x4AEF48, GetCurrentStatHook1);
	MakeJump(0x4AF3AF, GetCurrentStatHook2, 2);
	MakeJump(0x4AF56A, SetCurrentStatHook, 2);
	MakeJump(0x4AF09C, ApplyApAcBonus);

	// Allow to change the statistics STAT_unused and STAT_better_crit - STAT_dmg_resist_explosion for set_critter_stat function
	MakeCall(0x4AF54E, stat_set_base_hack);
	MakeCall(0x455D65, op_set_critter_stat_hack); // STAT_unused for other critters

	auto xpTableList = GetConfigList("Misc", "XPTable", "", 2048);
	size_t numLevels = xpTableList.size();
	if (numLevels > 0) {
		HookCall(0x434AA7, GetNextLevelXPHook);
		HookCall(0x439642, GetNextLevelXPHook);
		HookCall(0x4AFB22, GetNextLevelXPHook);
		HookCall(0x496C8D, GetLevelXPHook);
		HookCall(0x4AFC53, GetLevelXPHook);

		for (size_t i = 0; i < 99; i++) {
			xpTable[i] = (i < numLevels)
				? atoi(xpTableList[i].c_str())
				: -1;
		}
		SafeWrite8(0x4AFB1B, static_cast<BYTE>(numLevels));
	}

	auto statsFile = GetConfigString("Misc", "DerivedStats", "", 2048);
	if (statsFile.size() > 0) {
		MakeJump(0x4AF6FC, stat_recalc_derived);
		memset(StatFormulas, 0, sizeof(StatFormulas));
		memset(StatShifts, 0, sizeof(StatShifts));
		memset(StatMulti, 0, sizeof(StatMulti));

		StatFormulas[7 * 2] = 15; //max hp
		StatMulti[7 * 7 + 0] = 1;
		StatMulti[7 * 7 + 2] = 2;
		StatFormulas[8 * 2] = 5; //max ap
		StatMulti[8 * 7 + 5] = 0.5;
		StatMulti[9 * 7 + 5] = 1; //ac
		StatFormulas[11 * 2 + 1] = 1; //melee damage
		StatShifts[11 * 7 + 0] = -5;
		StatMulti[11 * 7 + 0] = 1;
		StatFormulas[12 * 2] = 25; //carry weight
		StatMulti[12 * 7 + 0] = 25;
		StatMulti[13 * 7 + 1] = 2; //sequence
		StatFormulas[14 * 2 + 1] = 1; //heal rate
		StatMulti[14 * 7 + 2] = 1.0 / 3.0;
		StatMulti[15 * 7 + 6] = 1; //critical chance
		StatMulti[31 * 7 + 2] = 2; //rad resist
		StatMulti[32 * 7 + 2] = 5; //poison resist

		char key[6], buf2[256], buf3[256];
		statsFile = ".\\" + statsFile;
		for (int i = 7; i <= 32; i++) {
			if (i >= 17 && i <= 30) continue;

			_itoa(i, key, 10);
			StatFormulas[i * 2] = GetPrivateProfileInt(key, "base", StatFormulas[i * 2], statsFile.c_str());
			StatFormulas[i * 2 + 1] = GetPrivateProfileInt(key, "min", StatFormulas[i * 2 + 1], statsFile.c_str());
			for (int j = 0; j < 7; j++) {
				sprintf(buf2, "shift%d", j);
				StatShifts[i * 7 + j] = GetPrivateProfileInt(key, buf2, StatShifts[i * 7 + 0], statsFile.c_str());
				sprintf(buf2, "multi%d", j);
				_gcvt(StatMulti[i * 7 + j], 16, buf3);
				GetPrivateProfileStringA(key, buf2, buf3, buf2, 256, statsFile.c_str());
				StatMulti[i * 7 + j] = atof(buf2);
			}
		}
	}
}

void _stdcall SetPCStatMax(int stat, int i) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMaximumsPC[stat] = i;
	}
}

void _stdcall SetPCStatMin(int stat, int i) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMinimumsPC[stat] = i;
	}
}

void _stdcall SetNPCStatMax(int stat, int i) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMaximumsNPC[stat] = i;
	}
}

void _stdcall SetNPCStatMin(int stat, int i) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMinimumsNPC[stat] = i;
	}
}

}