/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\Perks.h"

#include "skills.h"

namespace game
{

namespace sf = sfall;

// TODO: skill_level_, perk_adjust_skill_

int __stdcall Skills::trait_adjust_skill(DWORD skillID) {
	int result = 0;
	if (sf::Perks::TraitsModEnable()) {
		if (fo::var::pc_trait[0] != -1) result += sf::Perks::GetTraitSkillBonus(skillID, 0);
		if (fo::var::pc_trait[1] != -1) result += sf::Perks::GetTraitSkillBonus(skillID, 1);
	}

	if (sf::Perks::DudeHasTrait(fo::TRAIT_gifted)) result -= 10;

	if (sf::Perks::DudeHasTrait(fo::TRAIT_good_natured)) {
		if (skillID <= fo::SKILL_THROWING) {
			result -= 10;
		} else if (skillID == fo::SKILL_FIRST_AID || skillID == fo::SKILL_DOCTOR || skillID == fo::SKILL_CONVERSANT || skillID == fo::SKILL_BARTER) {
			result += 15;
		}
	}
	return result;
}

static __declspec(naked) void trait_adjust_skill_replacement() {
	__asm {
		push edx;
		push ecx;
		push eax; // skillID
		call Skills::trait_adjust_skill;
		pop  ecx;
		pop  edx;
		retn;
	}
}

void Skills::init() {
	// Replace trait_adjust_skill_ function
	sf::MakeJump(fo::funcoffs::trait_adjust_skill_, trait_adjust_skill_replacement); // 0x4B40FC
}

}
