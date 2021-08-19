/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HookScripts\CombatHs.h"

#include "items.h"

#include "combatAI.h"

namespace game
{

namespace sf = sfall;

static const long aiUseItemAPCost = 2;

// Implementation of ai_can_use_weapon_ engine function with the HOOK_CANUSEWEAPON hook
bool CombatAI::ai_can_use_weapon(fo::GameObject* source, fo::GameObject* weapon, long hitMode) {
	bool result = fo::func::ai_can_use_weapon(source, weapon, hitMode);
	return sf::CanUseWeaponHook_Invoke(result, source, weapon, hitMode);
}

static long drugUsePerfFixMode;

void __stdcall CombatAI::ai_check_drugs(fo::GameObject* source) {
	if (fo::func::critter_body_type(source)) return; // Robotic/Quadruped cannot use drugs

	DWORD slot = -1;
	long noInvenDrug = 0;
	bool drugWasUsed = false;

	fo::GameObject* lastItem = nullptr; // combatAIInfoGetLastItem_(source); unused function, always returns 0
	if (!lastItem) {
		fo::AIcap* cap = fo::func::ai_cap(source);
		if (!cap) return;

		long hpPercent = 50;
		long chance = 0;

		switch ((fo::AIpref::chem_use_mode)cap->chem_use) {
		case fo::AIpref::chem_use_mode::stims_when_hurt_little: // use only healing drugs
			hpPercent = 60;
			break;
		case fo::AIpref::chem_use_mode::stims_when_hurt_lots:   // use only healing drugs
			hpPercent = 30;
			break;
		case fo::AIpref::chem_use_mode::sometimes:
			if (!(fo::var::combatNumTurns % 3)) chance = 25; // every three turns
			//hpPercent = 50;
			break;
		case fo::AIpref::chem_use_mode::anytime:
			if (!(fo::var::combatNumTurns % 3)) chance = 75; // every three turns
			//hpPercent = 50;
			break;
		case fo::AIpref::chem_use_mode::always:
			chance = 100; // 99%
			break;
		case fo::AIpref::chem_use_mode::clean:
			return; // exit: don't use drugs
		}

		long minHP = (hpPercent * fo::func::stat_level(source, fo::Stat::STAT_max_hit_points)) / 100;

		// [FIX] for AI not checking minimum hp properly for using healing drugs (prevents premature fleeing)
		if (cap->min_hp > minHP) minHP = cap->min_hp;

		while (fo::func::stat_level(source, fo::Stat::STAT_current_hp) < minHP && source->critter.movePoints >= aiUseItemAPCost) {
			fo::GameObject* itemFind = fo::func::inven_find_type(source, fo::ItemType::item_type_drug, &slot);
			if (!itemFind) {
				noInvenDrug = 2; // healing drugs were not found in the inventory (was 1)
				break;
			}

			if (game::Items::IsHealingItem(itemFind) && !fo::func::item_remove_mult(source, itemFind, 1)) {
				if (!game::Items::UseDrugItemFunc(source, itemFind)) {
					drugWasUsed = true;
				}

				if (source->critter.movePoints < aiUseItemAPCost) {
					source->critter.movePoints = 0;
				} else {
					source->critter.movePoints -= aiUseItemAPCost;
				}
				slot = -1;
			}
		}

		// use any drug (except healing drugs) if there is a chance of using it
		if (!drugWasUsed && chance > 0 && fo::func::roll_random(0, 100) < chance) {
			long usedCount = 0;
			while (source->critter.movePoints >= aiUseItemAPCost) {
				fo::GameObject* item = fo::func::inven_find_type(source, fo::ItemType::item_type_drug, &slot);
				if (!item) {
					noInvenDrug = 1;
					break;
				}

				long counter = 0;

				if (drugUsePerfFixMode > 0) {
					fo::GameObject* firstFoundDrug = item;
					do {
						// [FIX] Allow using only the drugs listed in chem_primary_desire and healing drugs (AIDrugUsePerfFix == 2)
						while (item->protoId != cap->chem_primary_desire[counter]) if (++counter > 2) break;
						if (counter <= 2) break; // there is a match

						// [FIX] for AI not taking chem_primary_desire in AI.txt as a preference list when using drugs in the inventory
						if (drugUsePerfFixMode == 1) {
							item = fo::func::inven_find_type(source, fo::ItemType::item_type_drug, &slot);
							if (!item) {
								item = firstFoundDrug;
								break;
							}
							counter = 0;
						}
					} while (counter < 3);
				} else {
					// if the drug is equal to the item in the preference list, then check the next (vanilla behavior)
					while (item->protoId == cap->chem_primary_desire[counter]) if (++counter > 2) break;
				}

				// if the preference counter is less than 3, then AI can use the drug
				if (counter < 3) {
					// if the item is NOT a healing drug
					if (!game::Items::IsHealingItem(item) && !fo::func::item_remove_mult(source, item, 1)) {
						if (!game::Items::UseDrugItemFunc(source, item)) {
							drugWasUsed = true;
							usedCount++;
						}

						if (source->critter.movePoints < aiUseItemAPCost) {
							source->critter.movePoints = 0;
						} else {
							source->critter.movePoints -= aiUseItemAPCost;
						}
						slot = -1;

						fo::AIpref::chem_use_mode chemUse = (fo::AIpref::chem_use_mode)cap->chem_use;
						if (chemUse == fo::AIpref::chem_use_mode::sometimes ||
						    (chemUse == fo::AIpref::chem_use_mode::anytime && usedCount >= 2))
						{
							break;
						}
					}
				}
			}
		}
	}
	// search for drugs on the map
	if (lastItem || (!drugWasUsed && noInvenDrug)) {
		do {
			if (!lastItem) lastItem = fo::func::ai_search_environ(source, fo::ItemType::item_type_drug);
			if (!lastItem) lastItem = fo::func::ai_search_environ(source, fo::ItemType::item_type_misc_item);
			if (lastItem) lastItem = fo::func::ai_retrieve_object(source, lastItem);

			// [FIX] Prevent the use of healing drugs when not necessary
			// noInvenDrug: is set to 2 that healing is required
			if (lastItem && noInvenDrug != 2 && game::Items::IsHealingItem(lastItem)) {
				long maxHP = fo::func::stat_level(source, fo::Stat::STAT_max_hit_points);
				if (10 + source->critter.health >= maxHP) { // quick check current HP
					return; // exit: don't use healing item
				}
			}

			if (lastItem && !fo::func::item_remove_mult(source, lastItem, 1)) {
				if (!game::Items::UseDrugItemFunc(source, lastItem)) lastItem = nullptr;

				if (source->critter.movePoints < aiUseItemAPCost) {
					source->critter.movePoints = 0;
				} else {
					source->critter.movePoints -= aiUseItemAPCost;
				}
			}
		} while (lastItem && source->critter.movePoints >= aiUseItemAPCost);
	}
}

static void __declspec(naked) ai_check_drugs_hack() {
	__asm {
		push ecx;
		push eax; // source
		call CombatAI::ai_check_drugs;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) ai_can_use_drug_hack() {
	__asm {
		push ecx; // item
		call Items::IsHealingItem;
		pop  ecx;
		test al, al;
		retn;
	}
}

void CombatAI::init() {
	// Replace ai_check_drugs_ function for code fixes and checking healing items
	sf::MakeJump(fo::funcoffs::ai_check_drugs_, ai_check_drugs_hack); // 0x428480

	// Change ai_can_use_drug_ function code to check healing items
	sf::MakeCall(0x429BDE, ai_can_use_drug_hack, 6);
	sf::SafeWrite8(0x429BE9, sf::CodeType::JumpNZ);    // jz > jnz
	sf::SafeWrite8(0x429BF1, sf::CodeType::JumpShort); // jnz > jmp

	drugUsePerfFixMode = sf::IniReader::GetConfigInt("Misc", "AIDrugUsePerfFix", 0);
	if (drugUsePerfFixMode > 0) sf::dlogr("Applying AI drug use preference fix.", DL_FIX);
}

}
