/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\Perks.h"

#include "stats.h"

namespace game
{

namespace sf = sfall;

static bool smallFrameTraitFix = false;

int Stats::trait_level(DWORD traitID) {
	return sf::Perks::DudeHasTrait(traitID);
}

// Support the player's party members (disabled until fully implemented, to avoid unexpected errors)
int Stats::perk_level(fo::GameObject* source, DWORD perkID) {
	if (source != fo::var::obj_dude /*&& !fo::util::IsPartyMember(source)*/) return 0;
	return fo::func::perk_level(source, perkID);
}

static int DudeGetBaseStat(DWORD statID) {
	return fo::func::stat_get_base_direct(fo::var::obj_dude, statID);
}

int __stdcall Stats::trait_adjust_stat(DWORD statID) {
	if (statID > fo::STAT_max_derived) return 0;

	int result = 0;
	if (sf::Perks::TraitsModEnable()) {
		if (fo::var::pc_trait[0] != -1) result += sf::Perks::GetTraitStatBonus(statID, 0);
		if (fo::var::pc_trait[1] != -1) result += sf::Perks::GetTraitStatBonus(statID, 1);
	}

	switch (statID) {
	case fo::STAT_st:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		if (Stats::trait_level(fo::TRAIT_bruiser)) result += 2;
		break;
	case fo::STAT_pe:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		break;
	case fo::STAT_en:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		break;
	case fo::STAT_ch:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		break;
	case fo::STAT_iq:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		break;
	case fo::STAT_ag:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		if (Stats::trait_level(fo::TRAIT_small_frame)) result++;
		break;
	case fo::STAT_lu:
		if (Stats::trait_level(fo::TRAIT_gifted)) result++;
		break;
	case fo::STAT_max_move_points:
		if (Stats::trait_level(fo::TRAIT_bruiser)) result -= 2;
		break;
	case fo::STAT_ac:
		if (Stats::trait_level(fo::TRAIT_kamikaze)) return -DudeGetBaseStat(fo::STAT_ac);
		break;
	case fo::STAT_melee_dmg:
		if (Stats::trait_level(fo::TRAIT_heavy_handed)) result += 4;
		break;
	case fo::STAT_carry_amt:
		if (Stats::trait_level(fo::TRAIT_small_frame)) {
			int st;
			if (smallFrameTraitFix) {
				st = fo::func::stat_level(fo::var::obj_dude, fo::STAT_st);
			} else {
				st = DudeGetBaseStat(fo::STAT_st);
			}
			result -= st * 10;
		}
		break;
	case fo::STAT_sequence:
		if (Stats::trait_level(fo::TRAIT_kamikaze)) result += 5;
		break;
	case fo::STAT_heal_rate:
		if (Stats::trait_level(fo::TRAIT_fast_metabolism)) result += 2;
		break;
	case fo::STAT_crit_chance:
		if (Stats::trait_level(fo::TRAIT_finesse)) result += 10;
		break;
	case fo::STAT_better_crit:
		if (Stats::trait_level(fo::TRAIT_heavy_handed)) result -= 30;
		break;
	case fo::STAT_rad_resist:
		if (Stats::trait_level(fo::TRAIT_fast_metabolism)) return -DudeGetBaseStat(fo::STAT_rad_resist);
		break;
	case fo::STAT_poison_resist:
		if (Stats::trait_level(fo::TRAIT_fast_metabolism)) return -DudeGetBaseStat(fo::STAT_poison_resist);
		break;
	}
	return result;
}

static __declspec(naked) void trait_adjust_stat_replacement() {
	__asm {
		push edx;
		push ecx;
		push eax; // statID
		call Stats::trait_adjust_stat;
		pop  ecx;
		pop  edx;
		retn;
	}
}

void Stats::init() {
	// Replace trait_adjust_stat_ function
	sf::MakeJump(fo::funcoffs::trait_adjust_stat_, trait_adjust_stat_replacement); // 0x4B3C7C

	// Fix the carry weight penalty of the Small Frame trait not being applied to bonus Strength points
	smallFrameTraitFix = (sf::IniReader::GetConfigInt("Misc", "SmallFrameFix", 0) != 0);
}

}
