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

static fo::GameObject* cCritter;

static DWORD xpTable[99];
static int StatFormulas[33 * 2];
static int StatShifts[33 * 7];
static double StatMulti[33 * 7];

DWORD standardApAcBonus = 4;
DWORD extraApAcBonus = 4;

static const DWORD StatLevelHack_Ret = 0x4AEF52;
static void __declspec(naked) stat_level_hack() {
	__asm {
		mov cCritter, eax;
		sub esp, 8;
		mov ebx, eax;
		jmp StatLevelHack_Ret;
	}
}

static DWORD __fastcall check_stat_level(register DWORD value, DWORD stat) {
	DWORD valLimit;
	if (cCritter == fo::var::obj_dude) {
		valLimit = statMinimumsPC[stat];
		if (value < valLimit) return valLimit;
		valLimit = statMaximumsPC[stat];
		if (value > valLimit) return valLimit;
	} else {
		valLimit = statMinimumsNPC[stat];
		if (value < valLimit) return valLimit;
		valLimit = statMaximumsNPC[stat];
		if (value > valLimit) return valLimit;
	}
	return value;
}

static const DWORD StatLevelHackCheck_Ret = 0x4AF3D7;
static void __declspec(naked) stat_level_hack_check() {
	__asm {
		mov  edx, esi;
		call check_stat_level; // ecx - value stat
		jmp  StatLevelHackCheck_Ret;
	}
}

static const DWORD StatSetBaseHack_RetMin = 0x4AF57E;
static const DWORD StatSetBaseHack_RetMax = 0x4AF591;
static const DWORD StatSetBaseHack_Ret    = 0x4AF59C;
static void __declspec(naked) stat_set_base_hack_check() {
	__asm {
		cmp esi, dword ptr ds:[FO_VAR_obj_dude];
		jz  pc;
		cmp ebx, statMinimumsNPC[eax];
		jl  failMin;
		cmp ebx, statMaximumsNPC[eax];
		jg  failMax;
		jmp StatSetBaseHack_Ret;
pc:
		cmp ebx, statMinimumsPC[eax];
		jl  failMin;
		cmp ebx, statMaximumsPC[eax];
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
		mov eax, ds:[FO_VAR_Level_];
		jmp GetLevelXPHook;
	}
}

static const DWORD ApAcRetAddr = 0x4AF0A4;
static void __declspec(naked) CalcApToAcBonus() {
	using namespace fo;
	using namespace Fields;
	__asm {
		xor  eax, eax;
		mov  edi, [ebx + movePoints];
		test edi, edi;
		jz   end;
		cmp  [esp + 0x1C - 0x18], 2;     // pc have perk h2hEvade (2 - vanilla bonus)
		jb   standard;
		mov  edx, PERK_hth_evade_perk;
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];
		call fo::funcoffs::perk_level_;
		imul eax, extraApAcBonus;        // bonus = perkLvl * extraApBonus
		imul eax, edi;                   // perkBonus = bonus * curAP
standard:
		imul edi, standardApAcBonus;     // stdBonus = curAP * standardApBonus
		add  eax, edi;                   // bonus = perkBonus + stdBonus
		shr  eax, 2;                     // acBonus = bonus / 4
end:
		jmp  ApAcRetAddr;
	}
}

static void _stdcall StatRecalcDerived(fo::GameObject* critter) {
	int basestats[7];
	for (int i = fo::Stat::STAT_st; i <= fo::Stat::STAT_lu; i++) basestats[i] = fo::func::stat_level(critter, i);

	int* proto;
	fo::func::proto_ptr(critter->protoId, (fo::Proto**)&proto);

	for (int i = fo::Stat::STAT_max_hit_points; i <= fo::Stat::STAT_poison_resist; i++) {
		if (i >= fo::Stat::STAT_dmg_thresh && i <= fo::Stat::STAT_dmg_resist_explosion) continue;

		double sum = 0;
		for (int j = fo::Stat::STAT_st; j <= fo::Stat::STAT_lu; j++) {
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
		//xp mod
		SafeWrite8(0x4AFAB8, 0x53);
		SafeWrite32(0x4AFAB9, 0x55575651);
		//HP bonus
		SafeWrite8(0x4AFBC1, 2);
		//skill points per level mod
		SafeWrite8(0x43C27A, 5);
	};

	MakeJump(0x4AEF4D, stat_level_hack);
	MakeJump(0x4AF3AF, stat_level_hack_check);
	MakeJump(0x4AF571, stat_set_base_hack_check);
	MakeJump(0x4AF09C, CalcApToAcBonus); // stat_level_

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
		MakeJump(0x4AF6FC, stat_recalc_derived_hack); // overrides function

		memset(StatFormulas, 0, sizeof(StatFormulas));
		memset(StatShifts, 0, sizeof(StatShifts));
		memset(StatMulti, 0, sizeof(StatMulti));

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
		statsFile = ".\\" + statsFile;
		for (int i = fo::Stat::STAT_max_hit_points; i <= fo::Stat::STAT_poison_resist; i++) {
			if (i >= fo::Stat::STAT_dmg_thresh && i <= fo::Stat::STAT_dmg_resist_explosion) continue;

			_itoa(i, key, 10);
			StatFormulas[i * 2] = GetPrivateProfileInt(key, "base", StatFormulas[i * 2], statsFile.c_str());
			StatFormulas[i * 2 + 1] = GetPrivateProfileInt(key, "min", StatFormulas[i * 2 + 1], statsFile.c_str());
			for (int j = 0; j < fo::Stat::STAT_max_hit_points; j++) {
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