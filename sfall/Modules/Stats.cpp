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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "CritterStats.h"
#include "LoadGameHook.h"

#include "Stats.h"

namespace sfall
{

static DWORD statMaximumsPC[fo::STAT_max_stat];
static DWORD statMinimumsPC[fo::STAT_max_stat];
static DWORD statMaximumsNPC[fo::STAT_max_stat];
static DWORD statMinimumsNPC[fo::STAT_max_stat];

static DWORD xpTable[99];

float Stats::experienceMod = 1.0f; // set_xp_mod func
DWORD Stats::standardApAcBonus = 4;
DWORD Stats::extraApAcBonus = 4;

static struct StatFormula {
	long base;
	long min;
	long shift[fo::STAT_lu + 1];
	double multi[fo::STAT_lu + 1];
} statFormulas[fo::STAT_max_derived + 1] = {0};

static fo::GameObject* cCritter;

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

static void __declspec(naked) CalcApToAcBonus() {
	using namespace fo;
	using namespace Fields;
	__asm {
		xor  eax, eax;
		mov  edi, [ebx + movePoints];
		test edi, edi;
		jz   end;
		cmp  [esp + 0x1C - 0x18 + 4], 2; // pc have perk h2hEvade (2 - vanilla bonus)
		jb   standard;
		mov  edx, PERK_hth_evade_perk;
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];
		call fo::funcoffs::perk_level_;
		imul eax, Stats::extraApAcBonus;    // bonus = perkLvl * extraApBonus
		imul eax, edi;                      // perkBonus = bonus * curAP
standard:
		imul edi, Stats::standardApAcBonus; // stdBonus = curAP * standardApBonus
		add  eax, edi;                      // bonus = perkBonus + stdBonus
		shr  eax, 2;                        // acBonus = bonus / 4
end:
		retn;
	}
}

static void _stdcall StatRecalcDerived(fo::GameObject* critter) {
	int baseStats[7];
	for (int stat = fo::Stat::STAT_st; stat <= fo::Stat::STAT_lu; stat++) baseStats[stat] = fo::func::stat_level(critter, stat);

	long* proto = CritterStats::GetProto(critter);
	if (!proto) fo::func::proto_ptr(critter->protoId, (fo::Proto**)&proto);

	for (int i = fo::Stat::STAT_max_hit_points; i <= fo::Stat::STAT_poison_resist; i++) {
		if (i >= fo::Stat::STAT_dmg_thresh && i <= fo::Stat::STAT_dmg_resist_explosion) continue;

		double sum = 0;
		for (int stat = fo::Stat::STAT_st; stat <= fo::Stat::STAT_lu; stat++) {
			sum += (baseStats[stat] + statFormulas[i].shift[stat]) * statFormulas[i].multi[stat];
		}
		long calcStat = statFormulas[i].base + (int)floor(sum);
		if (calcStat < statFormulas[i].min) calcStat = statFormulas[i].min;
		proto[OffsetStat::base + i] = calcStat; // offset from base_stat_srength
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
	using namespace fo;
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
	using namespace fo;
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
	for (size_t i = 0; i < fo::STAT_max_stat; i++) {
		statMaximumsPC[i] = statMaximumsNPC[i] = fo::var::stat_data[i].maxValue;
		statMinimumsPC[i] = statMinimumsNPC[i] = fo::var::stat_data[i].minValue;
	}
}

void Stats::init() {
	using namespace fo;

	StatsReset();

	LoadGameHook::OnGameReset() += []() {
		StatsReset();
		// Reset some settable game values back to the defaults
		standardApAcBonus = 4;
		extraApAcBonus = 4;
		// XP mod set to 100%
		experienceMod = 1.0f;
		// HP bonus
		SafeWrite8(0x4AFBC1, 2);
		// Skill points per level mod
		SafeWrite8(0x43C27A, 5);
	};

	MakeJump(0x4AEF4D, stat_level_hack);
	MakeJump(0x4AF3AF, stat_level_hack_check, 2);
	MakeJump(0x4AF571, stat_set_base_hack_check);

	MakeCall(0x4AF09C, CalcApToAcBonus, 3); // stat_level_

	// Allow set_critter_stat function to change STAT_unused and STAT_dmg_* stats for the player
	MakeCall(0x4AF54E, stat_set_base_hack_allow);
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
		SafeWrite8(0x4AFB1B, static_cast<BYTE>(numLevels + 1));
	}

	auto statsFile = GetConfigString("Misc", "DerivedStats", "", MAX_PATH);
	if (!statsFile.empty()) {
		MakeJump(0x4AF6FC, stat_recalc_derived_hack); // overrides function

		// STAT_st + STAT_en * 2 + 15
		statFormulas[STAT_max_hit_points].base            = 15; // max hp
		statFormulas[STAT_max_hit_points].multi[STAT_st]  = 1;
		statFormulas[STAT_max_hit_points].multi[STAT_en]  = 2;
		// STAT_ag / 2 + 5
		statFormulas[STAT_max_move_points].base           = 5;  // max ap
		statFormulas[STAT_max_move_points].multi[STAT_ag] = 0.5;

		statFormulas[STAT_ac].multi[STAT_ag]              = 1;  // ac
		// STAT_st - 5
		statFormulas[STAT_melee_dmg].min                  = 1;  // melee damage
		statFormulas[STAT_melee_dmg].shift[STAT_st]       = -5;
		statFormulas[STAT_melee_dmg].multi[STAT_st]       = 1;
		// STAT_st * 25 + 25
		statFormulas[STAT_carry_amt].base                 = 25; // carry weight
		statFormulas[STAT_carry_amt].multi[STAT_st]       = 25;
		// STAT_pe * 2
		statFormulas[STAT_sequence].multi[STAT_pe]        = 2;  // sequence
		// STAT_en / 3
		statFormulas[STAT_heal_rate].min                  = 1;  // heal rate
		statFormulas[STAT_heal_rate].multi[STAT_en]       = 1.0 / 3.0;

		statFormulas[STAT_crit_chance].multi[STAT_lu]     = 1;  // critical chance
		// STAT_en * 2
		statFormulas[STAT_rad_resist].multi[STAT_en]      = 2;  // rad resist
		// STAT_en * 5
		statFormulas[STAT_poison_resist].multi[STAT_en]   = 5;  // poison resist

		char key[6], buf2[256], buf3[256];
		const char* statFile = statsFile.insert(0, ".\\").c_str();
		if (GetFileAttributes(statFile) == INVALID_FILE_ATTRIBUTES) return;

		for (int i = fo::Stat::STAT_max_hit_points; i <= fo::Stat::STAT_poison_resist; i++) {
			if (i >= fo::Stat::STAT_dmg_thresh && i <= fo::Stat::STAT_dmg_resist_explosion) continue;

			_itoa(i, key, 10);
			statFormulas[i].base = iniGetInt(key, "base", statFormulas[i].base, statFile);
			statFormulas[i].min = iniGetInt(key, "min", statFormulas[i].min, statFile);
			for (int j = 0; j < fo::Stat::STAT_max_hit_points; j++) {
				sprintf(buf2, "shift%d", j);
				statFormulas[i].shift[j] = iniGetInt(key, buf2, statFormulas[i].shift[j], statFile);
				sprintf(buf2, "multi%d", j);
				_gcvt(statFormulas[i].multi[j], 16, buf3);
				iniGetString(key, buf2, buf3, buf2, 256, statFile);
				statFormulas[i].multi[j] = atof(buf2);
			}
		}
	} else {
		CritterStats::RecalcDerivedHook();
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
