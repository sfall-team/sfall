/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HookScripts\CombatHs.h"
#include "..\Modules\Perks.h"

#include "..\Game\stats.h"

#include "items.h"

namespace game
{

namespace sf = sfall;

static constexpr int reloadCostAP = 2; // engine default reload AP cost

long Items::item_weapon_range(fo::GameObject* source, fo::GameObject* weapon, long hitMode) {
	fo::Proto* wProto;
	if (!GetProto(weapon->protoId, &wProto)) return 0;

	long isSecondMode = (hitMode && hitMode != fo::AttackType::ATKTYPE_RWEAPON_PRIMARY) ? 1 : 0;
	long range = wProto->item.weapon.maxRange[isSecondMode];

	long flagExt = wProto->item.flagsExt;
	if (isSecondMode) flagExt = (flagExt >> 4);
	long type = fo::GetWeaponType(flagExt);

	if (type == fo::AttackSubType::THROWING) {
		// TODO: add perkHeaveHoModFix from perks.cpp
		long heaveHoMod = Stats::perk_level(source, fo::Perk::PERK_heave_ho);
		if (heaveHoMod > 0) heaveHoMod *= 2;

		long stRange = (fo::func::stat_level(source, fo::Stat::STAT_st) + heaveHoMod);
		if (stRange > 10) stRange = 10; // fix for Heave Ho!
		stRange *= 3;
		if (stRange < range) range = stRange;
	}
	return range;
}

// Implementation of item_w_primary_mp_cost_ and item_w_secondary_mp_cost_ engine functions in a single function with the HOOK_CALCAPCOST hook
long __fastcall Items::item_weapon_mp_cost(fo::GameObject* source, fo::GameObject* weapon, long hitMode, long isCalled) {
	long cost = 0;

	switch (hitMode) {
	case fo::AttackType::ATKTYPE_LWEAPON_PRIMARY:
	case fo::AttackType::ATKTYPE_RWEAPON_PRIMARY:
		cost = fo::func::item_w_primary_mp_cost(weapon);
		break;
	case fo::AttackType::ATKTYPE_LWEAPON_SECONDARY:
	case fo::AttackType::ATKTYPE_RWEAPON_SECONDARY:
		cost = fo::func::item_w_secondary_mp_cost(weapon);
		break;
	case fo::AttackType::ATKTYPE_LWEAPON_RELOAD:
	case fo::AttackType::ATKTYPE_RWEAPON_RELOAD:
		if (weapon && weapon->protoId != fo::ProtoID::PID_SOLAR_SCORCHER) { // Solar Scorcher has no reload AP cost
			cost = reloadCostAP;
			if (fo::GetProto(weapon->protoId)->item.weapon.perk == fo::Perk::PERK_weapon_fast_reload) {
				cost--;
			}
		}
	}
	if (hitMode < fo::AttackType::ATKTYPE_LWEAPON_RELOAD) {
		if (isCalled) cost++;
		if (cost < 0) cost = 0;

		long type = fo::func::item_w_subtype(weapon, hitMode);

		if (source->id == fo::PLAYER_ID && sf::Perks::DudeHasTrait(fo::Trait::TRAIT_fast_shot)) {
			// Fallout 1 behavior and Alternative behavior (allowed for all weapons)
			bool allow = false; // TODO: add FastShotFix variable

						// Fallout 2 behavior (with fix) and Haenlomal's fix
			if (allow || (fo::func::item_w_range(source, hitMode) >= 2 && type > fo::AttackSubType::MELEE)) cost--;
		}
		if ((type == fo::AttackSubType::MELEE || type == fo::AttackSubType::UNARMED) && Stats::perk_level(source, fo::Perk::PERK_bonus_hth_attacks)) {
			cost--;
		}
		if (type == fo::AttackSubType::GUNS && Stats::perk_level(source, fo::Perk::PERK_bonus_rate_of_fire)) {
			cost--;
		}
		if (cost < 1) cost = 1;
	}
	return sf::CalcApCostHook_Invoke(source, hitMode, isCalled, cost, weapon);
}

// Implementation of item_w_mp_cost_ engine function with the HOOK_CALCAPCOST hook
long Items::item_w_mp_cost(fo::GameObject* source, long hitMode, long isCalled) {
	long cost = fo::func::item_w_mp_cost(source, hitMode, isCalled);
	return sf::CalcApCostHook_Invoke(source, hitMode, isCalled, cost, nullptr);
}

static void __declspec(naked) ai_search_inven_weap_hook() {
	using namespace fo;
	__asm {
		push 0;        // no called
		push ATKTYPE_RWEAPON_PRIMARY;
		mov  edx, esi; // found weapon
		mov  ecx, edi; // source
		call Items::item_weapon_mp_cost;
		retn;
	}
}

void Items::init() {
	// Replace the item_w_primary_mp_cost_ function with the sfall implementation
	sf::HookCall(0x429A08, ai_search_inven_weap_hook);
}

}
