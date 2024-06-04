/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#define PC_LEVEL_MAX    (99)

static bool engineDerivedStats = true;
static bool derivedHPwBonus = false; // recalculate the hit points with bonus stat values

static DWORD statMaximumsPC[fo::STAT_max_stat];
static DWORD statMinimumsPC[fo::STAT_max_stat];
static DWORD statMaximumsNPC[fo::STAT_max_stat];
static DWORD statMinimumsNPC[fo::STAT_max_stat];

static DWORD xpTable[PC_LEVEL_MAX];

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

static __declspec(naked) void stat_level_hack() {
	static const DWORD StatLevelHack_Ret = 0x4AEF52;
	__asm {
		mov cCritter, eax;
		sub esp, 8;
		mov ebx, eax;
		jmp StatLevelHack_Ret;
	}
}

static int __fastcall check_stat_level(int value, DWORD stat) {
	int valLimit;
	if (cCritter->protoId == fo::PID_Player) {
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

static __declspec(naked) void stat_level_hack_check() {
	__asm {
		mov  edx, esi;         // stat
		push 0x4AF3D7;         // return address
		jmp  check_stat_level; // ecx - value
	}
}

static __declspec(naked) void stat_set_base_hack_check() {
	static const DWORD StatSetBaseHack_RetMin = 0x4AF57E;
	static const DWORD StatSetBaseHack_RetMax = 0x4AF591;
	static const DWORD StatSetBaseHack_Ret    = 0x4AF59C;
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

static __declspec(naked) void GetLevelXPHook() {
	__asm {
		cmp eax, PC_LEVEL_MAX;
		jge lvlMax;
		dec eax;
		mov eax, [xpTable + eax * 4];
		retn;
lvlMax:
		mov eax, -1; // for printing "------"
		retn;
	}
}

static __declspec(naked) void GetNextLevelXPHook() {
	__asm {
		mov eax, ds:[FO_VAR_Level_pc];
		jmp GetLevelXPHook;
	}
}

static __declspec(naked) void CalcApToAcBonus() {
	using namespace fo;
	using namespace Fields;
	__asm {
		xor  eax, eax;
		mov  edi, [ebx + movePoints];
		test edi, edi;
		jz   end;
		cmp  dword ptr [esp + 0x1C - 0x18 + 4], 2; // has HtH Evade perk (2 - vanilla bonus)
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

static long RecalcStat(int stat, int statsValue[]) {
	double sum = 0;
	for (int i = fo::Stat::STAT_st; i <= fo::Stat::STAT_lu; i++) {
		sum += (statsValue[i] + statFormulas[stat].shift[i]) * statFormulas[stat].multi[i];
	}
	long calcStatValue = statFormulas[stat].base + (int)floor(sum);
	return (calcStatValue < statFormulas[stat].min) ? statFormulas[stat].min : calcStatValue;
}

static void __stdcall StatRecalcDerived(fo::GameObject* critter) {
	long* proto = CritterStats::GetProto(critter);
	if (!proto && !fo::util::GetProto(critter->protoId, (fo::Proto**)&proto)) return;

	int baseStats[7], levelStats[7];
	for (int stat = fo::Stat::STAT_st; stat <= fo::Stat::STAT_lu; stat++) {
		levelStats[stat] = fo::func::stat_level(critter, stat);
		if (!derivedHPwBonus) baseStats[stat] = fo::func::stat_get_base(critter, stat);
	}

	((fo::Proto*)proto)->critter.base.health = RecalcStat(fo::Stat::STAT_max_hit_points, (derivedHPwBonus) ? levelStats : baseStats);

	for (int stat = fo::Stat::STAT_max_move_points; stat <= fo::Stat::STAT_poison_resist; stat++) {
		if (stat >= fo::Stat::STAT_dmg_thresh && stat <= fo::Stat::STAT_dmg_resist_explosion) continue;
		// offset from base_stat_srength
		proto[OffsetStat::base + stat] = RecalcStat(stat, levelStats);
	}
}

static __declspec(naked) void stat_recalc_derived_hack() {
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

void Stats::UpdateHPStat(fo::GameObject* critter) {
	if (fo::util::IsPartyMember(critter)) return;

	if (engineDerivedStats) {
		if (critter->critter.health > 0) {
			long maxHP = fo::func::stat_level(critter, fo::Stat::STAT_max_hit_points);
			if (critter->critter.health != maxHP) {
				fo::func::debug_printf("\nWarning: %s (PID: %d, ID: %d) has an incorrect value of the max HP stat: %d, adjusted to %d.",
				                       fo::func::critter_name(critter), critter->protoId, critter->id, critter->critter.health, maxHP);

				critter->critter.health = maxHP;
			}
		}
	} else {
		auto getStatFunc = (derivedHPwBonus) ? fo::func::stat_level : fo::func::stat_get_base;

		double sum = 0;
		for (int stat = fo::Stat::STAT_st; stat <= fo::Stat::STAT_lu; stat++) {
			sum += (getStatFunc(critter, stat) + statFormulas[fo::Stat::STAT_max_hit_points].shift[stat]) * statFormulas[fo::Stat::STAT_max_hit_points].multi[stat];
		}
		long calcStatValue = statFormulas[fo::Stat::STAT_max_hit_points].base + (int)floor(sum);
		if (calcStatValue < statFormulas[fo::Stat::STAT_max_hit_points].min) {
			calcStatValue = statFormulas[fo::Stat::STAT_max_hit_points].min;
		}

		fo::Proto* proto;
		if (fo::util::GetProto(critter->protoId, &proto) && proto->critter.base.health != calcStatValue) {
			proto->critter.base.health = calcStatValue;
			critter->critter.health = calcStatValue + proto->critter.bonus.health;
		}
	}
}

static __declspec(naked) void stat_set_base_hack_allow() {
	static const DWORD StatSetBaseRet = 0x4AF559;
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

static __declspec(naked) void op_set_critter_stat_hack() {
	static const DWORD SetCritterStatRet = 0x455D8A;
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

	// Allow set_critter_stat function to change the base stats of STAT_unused and STAT_dmg_* for the player
	MakeCall(0x4AF54E, stat_set_base_hack_allow);
	MakeCall(0x455D65, op_set_critter_stat_hack); // STAT_unused for other critters

	auto xpTableList = IniReader::GetConfigList("Misc", "XPTable", "");
	size_t numLevels = xpTableList.size();
	if (numLevels > 0) {
		if (numLevels >= PC_LEVEL_MAX) numLevels = PC_LEVEL_MAX - 1;
		HookCalls(GetNextLevelXPHook, {0x434AA7, 0x439642, 0x4AFB22});
		HookCalls(GetLevelXPHook, {0x496C8D, 0x4AFC53});

		for (size_t i = 0; i < PC_LEVEL_MAX; i++) {
			xpTable[i] = (i < numLevels)
			           ? atoi(xpTableList[i].c_str())
			           : -1;
		}
		SafeWrite8(0x4AFB1B, static_cast<BYTE>(numLevels + 1));
	}

	auto statsFile = IniReader::GetConfigString("Misc", "DerivedStats", "");
	if (!statsFile.empty()) {
		const char* statFile = statsFile.insert(0, ".\\").c_str();
		if (GetFileAttributesA(statFile) != INVALID_FILE_ATTRIBUTES) { // check if file exists
			derivedHPwBonus = (IniReader::GetInt("Main", "HPDependOnBonusStats", 0, statFile) != 0);
			engineDerivedStats = false;

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

			for (int i = fo::Stat::STAT_max_hit_points; i <= fo::Stat::STAT_poison_resist; i++) {
				if (i >= fo::Stat::STAT_dmg_thresh && i <= fo::Stat::STAT_dmg_resist_explosion) continue;

				_itoa(i, key, 10);
				statFormulas[i].base = IniReader::GetInt(key, "base", statFormulas[i].base, statFile);
				statFormulas[i].min = IniReader::GetInt(key, "min", statFormulas[i].min, statFile);
				for (int j = 0; j < fo::Stat::STAT_max_hit_points; j++) {
					sprintf(buf2, "shift%d", j);
					statFormulas[i].shift[j] = IniReader::GetInt(key, buf2, statFormulas[i].shift[j], statFile);
					sprintf(buf2, "multi%d", j);
					_gcvt(statFormulas[i].multi[j], 16, buf3);
					IniReader::GetString(key, buf2, buf3, buf2, 256, statFile);
					statFormulas[i].multi[j] = atof(buf2);
				}
			}
		}
	}
	if (engineDerivedStats) {
		CritterStats::RecalcDerivedHook();
	} else {
		MakeJump(0x4AF6FC, stat_recalc_derived_hack); // overrides function
	}
}

long Stats::GetStatMax(int stat, int isNPC) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		return (isNPC) ? statMaximumsNPC[stat] : statMaximumsPC[stat];
	}
	return 0;
}

long Stats::GetStatMin(int stat, int isNPC) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		return (isNPC) ? statMinimumsNPC[stat] : statMinimumsPC[stat];
	}
	return 0;
}

void __stdcall SetPCStatMax(int stat, int value) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMaximumsPC[stat] = value;
	}
}

void __stdcall SetPCStatMin(int stat, int value) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMinimumsPC[stat] = value;
	}
}

void __stdcall SetNPCStatMax(int stat, int value) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMaximumsNPC[stat] = value;
	}
}

void __stdcall SetNPCStatMin(int stat, int value) {
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		statMinimumsNPC[stat] = value;
	}
}

}
