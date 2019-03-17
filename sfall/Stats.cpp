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

#include <math.h>
#include <stdio.h>
#include "Define.h"
#include "FalloutEngine.h"
#include "Stats.h"

static DWORD StatMaximumsPC[STAT_max_stat];
static DWORD StatMinimumsPC[STAT_max_stat];
static DWORD StatMaximumsNPC[STAT_max_stat];
static DWORD StatMinimumsNPC[STAT_max_stat];

static TGameObj* cCritter;

static DWORD xpTable[99];
static int StatFormulas[33 * 2] = {0};
static int StatShifts[33 * 7] = {0};
static double StatMulti[33 * 7] = {0};

DWORD StandardApAcBonus = 4;
DWORD ExtraApAcBonus = 4;

static const DWORD StatLevelHack_Ret = 0x4AEF52;
static void __declspec(naked) stat_level_hack() {
	__asm {
		mov cCritter, eax;
		sub esp, 8;
		mov ebx, eax;
		jmp StatLevelHack_Ret;
	}
}

static int __fastcall check_stat_level(register int value, int stat) {
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

static void __declspec(naked) _stdcall ProtoPtr(DWORD pid, int** proto) {
	__asm {
		mov eax, [esp + 4];
		mov edx, [esp + 8];
		call proto_ptr_;
		retn 8;
	}
}

static void _stdcall StatRecalcDerived(TGameObj* critter) {
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

void StatsReset() {
	for (int i = 0; i < STAT_max_stat; i++) {
		StatMaximumsPC[i] = StatMaximumsNPC[i] = *(DWORD*)(_stat_data + 16 + i * 24);
		StatMinimumsPC[i] = StatMinimumsNPC[i] = *(DWORD*)(_stat_data + 12 + i * 24);
	}
	StandardApAcBonus = 4;
	ExtraApAcBonus = 4;
}

void StatsInit() {
	StatsReset();

	MakeJump(0x4AEF4D, stat_level_hack);
	MakeJump(0x4AF3AF, stat_level_hack_check, 2);
	MakeJump(0x4AF571, stat_set_base_hack_check);

	MakeCall(0x4AF09C, CalcApToAcBonus, 3); // stat_level_

	char table[2048];
	GetPrivateProfileString("Misc", "XPTable", "", table, 2048, ini);
	if (strlen(table) > 0) {
		char *ptr = table, *ptr2;
		DWORD level = 0;

		HookCall(0x434AA7, GetNextLevelXPHook);
		HookCall(0x439642, GetNextLevelXPHook);
		HookCall(0x4AFB22, GetNextLevelXPHook);
		HookCall(0x496C8D, GetLevelXPHook);
		HookCall(0x4AFC53, GetLevelXPHook);

		while ((ptr2 = strstr(ptr, ",")) && level < 99) {
			ptr2[0] = '\0';
			xpTable[level++] = atoi(ptr);
			ptr = ptr2 + 1;
		}
		if (level < 99 && ptr[0] != '\0') {
			xpTable[level++] = atoi(ptr);
		}
		for (int i = level; i < 99; i++) xpTable[i] = -1;
		SafeWrite8(0x4AFB1B, (BYTE)(level + 1));
	}

	GetPrivateProfileStringA("Misc", "DerivedStats", "", table, MAX_PATH, ini);
	if (strlen(table)) {
		MakeJump(0x4AF6FC, stat_recalc_derived_hack); // overrides function

		StatFormulas[7 * 2]      = 15;      // max hp
		StatMulti[7 * 7 + 0]     = 1;
		StatMulti[7 * 7 + 2]     = 2;

		StatFormulas[8 * 2]      = 5;       // max ap
		StatMulti[8 * 7 + 5]     = 0.5;

		StatMulti[9 * 7 + 5]     = 1;       // ac
		StatFormulas[11 * 2 + 1] = 1;       // melee damage
		StatShifts[11 * 7 + 0]   = -5;
		StatMulti[11 * 7 + 0]    = 1;

		StatFormulas[12 * 2]     = 25;      // carry weight
		StatMulti[12 * 7 + 0]    = 25;

		StatMulti[13 * 7 + 1]    = 2;       // sequence
		StatFormulas[14 * 2 + 1] = 1;       // heal rate
		StatMulti[14 * 7 + 2]    = 1.0 / 3.0;

		StatMulti[15 * 7 + 6]    = 1;       // critical chance
		StatMulti[31 * 7 + 2]    = 2;       // rad resist
		StatMulti[32 * 7 + 2]    = 5;       // poison resist

		char key[6], buf2[256], buf3[256];
		strcpy_s(buf3, table);
		sprintf(table, ".\\%s", buf3);
		for (int i = STAT_max_hit_points; i <= STAT_poison_resist; i++) {
			if (i >= STAT_dmg_thresh && i <= STAT_dmg_resist_explosion) continue;

			_itoa(i, key, 10);
			StatFormulas[i * 2] = GetPrivateProfileInt(key, "base", StatFormulas[i * 2], table);
			StatFormulas[i * 2 + 1] = GetPrivateProfileInt(key, "min", StatFormulas[i * 2 + 1], table);
			for (int j = 0; j < STAT_max_hit_points; j++) {
				sprintf(buf2, "shift%d", j);
				StatShifts[i * 7 + j] = GetPrivateProfileInt(key, buf2, StatShifts[i * 7 + j], table);
				sprintf(buf2, "multi%d", j);
				_gcvt(StatMulti[i * 7 + j], 16, buf3);
				GetPrivateProfileStringA(key, buf2, buf3, buf2, 256, table);
				StatMulti[i * 7 + j] = atof(buf2);
			}
		}
	}
}

void _stdcall SetPCStatMax(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMaximumsPC[stat] = i;
	}
}

void _stdcall SetPCStatMin(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMinimumsPC[stat] = i;
	}
}

void _stdcall SetNPCStatMax(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMaximumsNPC[stat] = i;
	}
}

void _stdcall SetNPCStatMin(int stat, int i) {
	if (stat >= 0 && stat < STAT_max_stat) {
		StatMinimumsNPC[stat] = i;
	}
}
