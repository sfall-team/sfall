/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include <array>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HookScripts\CombatHs.h"
#include "..\Modules\HookScripts\InventoryHs.h"
#include "..\Modules\HookScripts\ObjectHs.h"
#include "..\Modules\Perks.h"
#include "..\Modules\Unarmed.h"

#include "..\Game\stats.h"

#include "items.h"

namespace game
{

namespace sf = sfall;

constexpr int reloadAPCost = 2;  // engine default reload AP cost
constexpr int unarmedAPCost = 3; // engine default unarmed AP cost

static std::array<long, 3> healingItemPids = {fo::PID_STIMPAK, fo::PID_SUPER_STIMPAK, fo::PID_HEALING_POWDER};

long Items::GetHealingPID(long index) {
	return healingItemPids[index];
}

void Items::SetHealingPID(long index, long pid) {
	healingItemPids[index] = pid;
}

bool __fastcall Items::IsHealingItem(fo::GameObject* item) {
	//for each (long pid in healingItemPids) if (pid == item->protoId) return true;
	if (healingItemPids[0] == item->protoId || healingItemPids[1] == item->protoId || healingItemPids[2] == item->protoId) {
		return true;
	}

	fo::Proto* proto;
	if (fo::util::GetProto(item->protoId, &proto)) {
		return (proto->item.flagsExt & fo::ItemFlags::HealingItem) != 0;
	}
	return false;
}

bool Items::UseDrugItemFunc(fo::GameObject* source, fo::GameObject* item) {
	bool result = (Items::item_d_take_drug(source, item) == -1); // HOOK_USEOBJON
	if (result) {
		fo::func::item_add_force(source, item, 1);
	} else {
		fo::func::ai_magic_hands(source, item, 5000);
		fo::func::obj_connect(item, source->tile, source->elevation, 0);
		fo::func::obj_destroy(item);
	}
	return result;
}

// Implementation of item_d_take_ engine function with the HOOK_USEOBJON hook
long Items::item_d_take_drug(fo::GameObject* source, fo::GameObject* item) {
	if (sf::UseObjOnHook_Invoke(source, item, source) == -1) { // default handler
		return fo::func::item_d_take_drug(source, item);
	}
	return -1; // cancel the drug use
}

long Items::item_remove_mult(fo::GameObject* source, fo::GameObject* item, long count, long rmType) {
	sf::SetRemoveObjectType(rmType);
	return fo::func::item_remove_mult(source, item, count);
}

long Items::item_count(fo::GameObject* who, fo::GameObject* item) {
	for (int i = 0; i < who->invenSize; i++) {
		auto tableItem = &who->invenTable[i];
		if (tableItem->object == item) {
			if (tableItem->count <= 0) {
				tableItem->count = 1; // fix stack count
			}
			return tableItem->count; // fix
		} else if (fo::func::item_get_type(tableItem->object) == fo::item_type_container) {
			int count = item_count(tableItem->object, item);
			if (count > 0) return count;
		}
	}
	return 0;
}

long Items::item_weapon_range(fo::GameObject* source, fo::GameObject* weapon, long hitMode) {
	fo::Proto* wProto;
	if (!fo::util::GetProto(weapon->protoId, &wProto)) return 0;

	long isSecondMode = (hitMode && hitMode != fo::AttackType::ATKTYPE_RWEAPON_PRIMARY) ? 1 : 0;
	long range = wProto->item.weapon.maxRange[isSecondMode];

	long flagExt = wProto->item.flagsExt;
	if (isSecondMode) flagExt = (flagExt >> 4);
	long type = fo::util::GetWeaponType(flagExt);

	if (type == fo::AttackSubType::THROWING) {
		long heaveHoMod = Stats::perk_level(source, fo::Perk::PERK_heave_ho);
		long stRange = fo::func::stat_level(source, fo::Stat::STAT_st);

		if (sf::Perks::perkHeaveHoModTweak) {
			stRange *= 3;
			if (stRange > range) stRange = range;
			return stRange + (heaveHoMod * 6);
		}

		// vanilla
		stRange += (heaveHoMod * 2);
		if (stRange > 10) stRange = 10; // fix for Heave Ho!
		stRange *= 3;
		if (stRange < range) range = stRange;
	}
	return range;
}

// TODO
//long Items::item_w_range(fo::GameObject* source, long hitMode) {
//	return item_weapon_range(source, fo::func::item_hit_with(source, hitMode), hitMode);
//}

static long item_w_mp_cost_sub(fo::GameObject* source, fo::GameObject* item, long hitMode, long isCalled, long cost) {
	if (isCalled) cost++;
	if (cost < 0) cost = 0;

	long type = fo::func::item_w_subtype(item, hitMode);

	if (source->protoId == fo::ProtoID::PID_Player && sf::Perks::DudeHasTrait(fo::Trait::TRAIT_fast_shot)) {
		// Alternative behaviors of the Fast Shot trait
		if (item && sf::Perks::fastShotTweak > 2) { // Fallout 1 behavior (allowed for all weapons)
			cost--;
		} else if (sf::Perks::fastShotTweak == 2) { // Alternative behavior (allowed for all attacks)
			cost--;
		} else if (sf::Perks::fastShotTweak < 2 && type > fo::AttackSubType::MELEE && fo::func::item_w_range(source, hitMode) >= 2) { // Fallout 2 behavior (with Haenlomal's fix)
			cost--;
		}
	}
	if ((type == fo::AttackSubType::MELEE || type == fo::AttackSubType::UNARMED) && Stats::perk_level(source, fo::Perk::PERK_bonus_hth_attacks)) {
		cost--;
	}
	if (type == fo::AttackSubType::RANGED && Stats::perk_level(source, fo::Perk::PERK_bonus_rate_of_fire)) {
		cost--;
	}
	if (cost < 1) cost = 1;

	return cost;
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
			cost = reloadAPCost;
			if (fo::util::GetProto(weapon->protoId)->item.weapon.perk == fo::Perk::PERK_weapon_fast_reload) {
				cost--;
			}
		}
		goto endReload;
	}

	cost = item_w_mp_cost_sub(source, weapon, hitMode, isCalled, cost);

endReload:
	return sf::CalcApCostHook_Invoke(source, hitMode, isCalled, cost, weapon); // return cost
}

static __declspec(naked) void ai_search_inven_weap_hook() {
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

// Implementation of item_w_mp_cost_ engine function with the HOOK_CALCAPCOST hook
long __fastcall Items::item_w_mp_cost(fo::GameObject* source, fo::AttackType hitMode, long isCalled) {
	fo::GameObject* handItem = nullptr;

	switch (hitMode) {
	case fo::AttackType::ATKTYPE_LWEAPON_PRIMARY:
	case fo::AttackType::ATKTYPE_LWEAPON_SECONDARY:
	case fo::AttackType::ATKTYPE_LWEAPON_RELOAD:
		handItem = fo::func::inven_left_hand(source);
		break;
	case fo::AttackType::ATKTYPE_RWEAPON_PRIMARY:
	case fo::AttackType::ATKTYPE_RWEAPON_SECONDARY:
	case fo::AttackType::ATKTYPE_RWEAPON_RELOAD:
		handItem = fo::func::inven_right_hand(source);
		break;
	default:
		break;
	}
	if (handItem) {
		return Items::item_weapon_mp_cost(source, handItem, hitMode, isCalled);
	}

	// unarmed hits
	long cost = unarmedAPCost;
	if (hitMode == fo::AttackType::ATKTYPE_PUNCH || hitMode == fo::AttackType::ATKTYPE_KICK || hitMode >= fo::AttackType::ATKTYPE_STRONGPUNCH) {
		cost = sf::Unarmed::GetHitAPCost(hitMode);
	}

	// return cost
	return sf::CalcApCostHook_Invoke(
		source,
		hitMode,
		isCalled,
		item_w_mp_cost_sub(source, nullptr, hitMode, isCalled, cost),
		nullptr
	);
}

static __declspec(naked) void item_w_mp_cost_replacement() {
	__asm {
		push ebx;      // isCalled
		mov  ecx, eax; // source
		call Items::item_w_mp_cost;
		pop  ecx;
		retn;
	}
}


// Simplified implementation of item_w_curr_ammo_ engine function
long __fastcall Items::item_w_curr_ammo(fo::GameObject* item) {
	if (item) return item->item.charges;
	return 0;
}

static __declspec(naked) void item_w_curr_ammo_replacement() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, eax; // item
		call Items::item_w_curr_ammo;
		pop  edx;
		pop  ecx;
		retn;
	}
}

void Items::init() {
	// Replace the item_w_primary_mp_cost_ function with the sfall implementation in ai_search_inven_weap_
	sf::HookCall(0x429A08, ai_search_inven_weap_hook);

	// Replace the item_w_mp_cost_ function with the sfall implementation
	sf::MakeJump(fo::funcoffs::item_w_mp_cost_ + 1, item_w_mp_cost_replacement); // 0x478B25

	// Replace the item_w_curr_ammo_ function with a simplified implementation
	sf::MakeJump(fo::funcoffs::item_w_curr_ammo_, item_w_curr_ammo_replacement); // 0x4786A0
}

}
