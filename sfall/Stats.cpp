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

#include "main.h"
#include "FalloutEngine.h"

#include "Stats.h"

static DWORD StatMaximumsPC[STAT_max_stat];
static DWORD StatMinimumsPC[STAT_max_stat];
static DWORD StatMaximumsNPC[STAT_max_stat];
static DWORD StatMinimumsNPC[STAT_max_stat];

static DWORD xpTable[99];

float ExperienceMod = 1.0f; // set_xp_mod func
DWORD StandardApAcBonus = 4;
DWORD ExtraApAcBonus = 4;

static int StatFormulas[33 * 2] = {0};
static int StatShifts[33 * 7] = {0};
static double StatMulti[33 * 7] = {0};

static TGameObj* cCritter;

static const DWORD StatLevelHack_Ret = 0x4AEF52;
static void __declspec(naked) stat_level_hack() {
	__asm {
		mov cCritter, eax;
		sub esp, 8;
		mov ebx, eax;
		jmp StatLevelHack_Ret;
	}
}

static int __fastcall check_stat_level(register int value, DWORD stat) {
	int valLimit;
	if (cCritter == *ptr_obj_dude) {
		valLimit = StatMinimumsPC[stat];
		if (value < valLimit) return valLimit;
		valLimit = StatMaximumsPC[stat];
		if (value > valLimit) return valLimit;
	} else {
		valLimit = StatMinimumsNPC[stat];
		if (value < valLimit) return valLimit;
		valLimit = StatMaximumsNPC[stat];
		if (value > valLimit) return valLimit;
	}
	return value;
}

static const DWORD StatLevelHackCheck_Ret = 0x4AF3D7;
static void __declspec(naked) stat_level_hack_check() {
	__asm {
		mov  edx, esi;         // stat
		call check_stat_level; // ecx - value
		jmp  StatLevelHackCheck_Ret;
	}
}

static const DWORD StatSetBaseHack_RetMin = 0x4AF57E;
static const DWORD StatSetBaseHack_RetMax = 0x4AF591;
static const DWORD StatSetBaseHack_Ret    = 0x4AF59C;
static void __declspec(naked) stat_set_base_hack_check() {
	__asm {
		cmp esi, dword ptr ds:[_obj_dude];
		jz  pc;
		cmp ebx, StatMinimumsNPC[eax];
		jl  failMin;
		cmp ebx, StatMaximumsNPC[eax];
		jg  failMax;
		jmp StatSetBaseHack_Ret;
pc:
		cmp ebx, StatMinimumsPC[eax];
		jl  failMin;
		cmp ebx, StatMaximumsPC[eax];
		jg  failMax;
		jmp StatSetBaseHack_Ret;
failMin:
		jmp StatSetBaseHack_RetMin;
failMax:
		jmp StatSetBaseHack_RetMax;
	}
}

static void __declspec(naked) GetLevelXPHook() {
	__asm {
		dec eax;
		mov eax, [xpTable + eax * 4];
		retn;
	}
}

static void __declspec(naked) GetNextLevelXPHook() {
	__asm {
		mov eax, ds:[_Level_];
		jmp GetLevelXPHook;
	}
}

static void __declspec(naked) CalcApToAcBonus() {
	__asm {
		xor  eax, eax;
		mov  edi, [ebx + 0x40];
		test edi, edi;
		jz   end;
		cmp  [esp + 0x1C - 0x18 + 4], 2; // pc have perk h2hEvade (2 - vanilla bonus)
		jb   standard;
		mov  edx, PERK_hth_evade_perk;
		mov  eax, dword ptr ds:[_obj_dude];
		call perk_level_;
		imul eax, ExtraApAcBonus;        // bonus = perkLvl * ExtraApBonus
		imul eax, edi;                   // perkBonus = bonus * curAP
standard:
		imul edi, StandardApAcBonus;     // stdBonus = curAP * StandardApBonus
		add  eax, edi;                   // bonus = perkBonus + stdBonus
		shr  eax, 2;                     // acBonus = bonus / 4
end:
		retn;
	}
}

static void __declspec(naked) __stdcall ProtoPtr(DWORD pid, int** proto) {
	__asm {
		mov eax, [esp + 4];
		mov edx, [esp + 8];
		call proto_ptr_;
		retn 8;
	}
}

static void __stdcall StatRecalcDerived(TGameObj* critter) {
	int basestats[7];
	for (int i = STAT_st; i <= STAT_lu; i++) basestats[i] = StatLevel(critter, i);

	int* proto;
	ProtoPtr(critter->pid, &proto);

	for (int i = STAT_max_hit_points; i <= STAT_poison_resist; i++) {
		if (i >= STAT_dmg_thresh && i <= STAT_dmg_resist_explosion) continue;

		double sum = 0;
		for (int j = STAT_st; j <= STAT_lu; j++) {
			sum += (basestats[j] + StatShifts[i * 7 + j]) * StatMulti[i * 7 + j];
		}
		proto[i + 9] = StatFormulas[i * 2] + (int)floor(sum);
		if (proto[i + 9] < StatFormulas[i * 2 + 1]) proto[i + 9] = StatFormulas[i * 2 + 1];
	}
}

static void __declspec(naked) stat_recalc_derived_hack() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call StatRecalcDerived;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static const DWORD StatSetBaseRet = 0x4AF559;
static void __declspec(naked) stat_set_base_hack_allow() {
	__asm {
		cmp  ecx, STAT_unused;
		je   allow;
		cmp  ecx, STAT_dmg_thresh;
		jl   notAllow;
		cmp  ecx, STAT_dmg_resist_explosion;
		jg   notAllow;
allow:
		pop  eax;      // destroy return address
		jmp  StatSetBaseRet;
notAllow:
		mov  eax, -1;  // overwritten engine code
		retn;
	}
}

static const DWORD SetCritterStatRet = 0x455D8A;
static void __declspec(naked) op_set_critter_stat_hack() {
	__asm {
		cmp  dword ptr [esp + 0x2C - 0x28 + 4], STAT_unused;
		je   allow;
		mov  ebx, 3;  // overwritten engine code
		retn;
allow:
		add  esp, 4;  // destroy return address
		jmp  SetCritterStatRet;
	}
}

static void StatsReset() {
	for (size_t i = 0; i < STAT_max_stat; i++) {
		StatMaximumsPC[i] = StatMaximumsNPC[i] = *(DWORD*)(_stat_data + 16 + i * 24);
		StatMinimumsPC[i] = StatMinimumsNPC[i] = *(DWORD*)(_stat_data + 12 + i * 24);
	}
}

void Stats_OnGameLoad() {
	StatsReset();
	// Reset some settable game values back to the defaults
	StandardApAcBonus = 4;
	ExtraApAcBonus = 4;
	// XP mod set to 100%
	ExperienceMod = 1.0f;
	// HP bonus
	SafeWrite8(0x4AFBC1, 2);
	// Skill points per level mod
	SafeWrite8(0x43C27A, 5);
}

void StatsInit() {
	StatsReset();

	MakeJump(0x4AEF4D, stat_level_hack);
	MakeJump(0x4AF3AF, stat_level_hack_check, 2);
	MakeJump(0x4AF571, stat_set_base_hack_check);

	MakeCall(0x4AF09C, CalcApToAcBonus, 3); // stat_level_

	// Allow set_critter_stat function to change STAT_unused and STAT_dmg_* stats for the player
	MakeCall(0x4AF54E, stat_set_base_hack_allow);
	MakeCall(0x455D65, op_set_critter_stat_hack); // STAT_unused for other critters

	std::vector<std::string> xpTableList = GetConfigList("Misc", "XPTable", "", 2048);
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
		SafeWrite8(0x4AFB1B, static_cast<BYTE>(numLevels + 1));
	}

	std::string statsFile = GetConfigString("Misc", "DerivedStats", "", MAX_PATH);
	if (!statsFile.empty()) {
		MakeJump(0x4AF6FC, stat_recalc_derived_hack); // overrides function

		// STAT_st + STAT_en * 2 + 15
		StatFormulas[7 * 2]          = 15; // max hp
		StatMulti[7 * 7 + STAT_st]   = 1;
		StatMulti[7 * 7 + STAT_en]   = 2;
		// STAT_ag / 2 + 5
		StatFormulas[8 * 2]          = 5;  // max ap
		StatMulti[8 * 7 + STAT_ag]   = 0.5;

		StatMulti[9 * 7 + STAT_ag]   = 1;  // ac
		// STAT_st - 5
		StatFormulas[11 * 2 + 1]     = 1;  // melee damage
		StatShifts[11 * 7 + STAT_st] = -5;
		StatMulti[11 * 7 + STAT_st]  = 1;
		// STAT_st * 25 + 25
		StatFormulas[12 * 2]         = 25; // carry weight
		StatMulti[12 * 7 + STAT_st]  = 25;
		// STAT_pe * 2
		StatMulti[13 * 7 + STAT_pe]  = 2;  // sequence
		// STAT_en / 3
		StatFormulas[14 * 2 + 1]     = 1;  // heal rate
		StatMulti[14 * 7 + STAT_en]  = 1.0 / 3.0;

		StatMulti[15 * 7 + STAT_lu]  = 1;  // critical chance
		// STAT_en * 2
		StatMulti[31 * 7 + STAT_en]  = 2;  // rad resist
		// STAT_en * 5
		StatMulti[32 * 7 + STAT_en]  = 5;  // poison resist

		char key[6], buf2[256], buf3[256];
		const char* statFile = statsFile.insert(0, ".\\").c_str();
		if (GetFileAttributes(statFile) == INVALID_FILE_ATTRIBUTES) return;

		for (int i = STAT_max_hit_points; i <= STAT_poison_resist; i++) {
			if (i >= STAT_dmg_thresh && i <= STAT_dmg_resist_explosion) continue;

			_itoa(i, key, 10);
			StatFormulas[i * 2] = iniGetInt(key, "base", StatFormulas[i * 2], statFile);
			StatFormulas[i * 2 + 1] = iniGetInt(key, "min", StatFormulas[i * 2 + 1], statFile);
			for (int j = 0; j < STAT_max_hit_points; j++) {
				sprintf(buf2, "shift%d", j);
				StatShifts[i * 7 + j] = iniGetInt(key, buf2, StatShifts[i * 7 + j], statFile);
				sprintf(buf2, "multi%d", j);
				_gcvt(StatMulti[i * 7 + j], 16, buf3);
				iniGetString(key, buf2, buf3, buf2, 256, statFile);
				StatMulti[i * 7 + j] = atof(buf2);
			}
		}
	}
}

void __stdcall SetPCStatMax(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMaximumsPC[stat] = i;
	}
}

void __stdcall SetPCStatMin(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMinimumsPC[stat] = i;
	}
}

void __stdcall SetNPCStatMax(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMaximumsNPC[stat] = i;
	}
}

void __stdcall SetNPCStatMin(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMinimumsNPC[stat] = i;
	}
}
