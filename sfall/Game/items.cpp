/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

//#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HookScripts\CombatHs.h"

#include "items.h"

namespace game
{

namespace sf = sfall;

// Implementation of item_w_primary_mp_cost_ and item_w_secondary_mp_cost_ engine functions in a single function with the hook
// returns -1 in case of an error
long Items::item_weapon_mp_cost(fo::GameObject* source, fo::GameObject* weapon, long hitMode, long isCalled) {
	long cost = -1;

	switch (hitMode) {
	case fo::AttackType::ATKTYPE_LWEAPON_PRIMARY:
	case fo::AttackType::ATKTYPE_RWEAPON_PRIMARY:
		cost = fo::func::item_w_primary_mp_cost(weapon);
		if (isCalled && cost != -1) cost++;
		break;
	case fo::AttackType::ATKTYPE_LWEAPON_SECONDARY:
	case fo::AttackType::ATKTYPE_RWEAPON_SECONDARY:
		cost = fo::func::item_w_secondary_mp_cost(weapon);
		if (isCalled && cost != -1) cost++;
		break;
	case fo::AttackType::ATKTYPE_LWEAPON_RELOAD:
	case fo::AttackType::ATKTYPE_RWEAPON_RELOAD:
		if (weapon) cost = 2; // default reload AP cost
	}

	return (cost != -1) ? sf::CalcApCostHook_CheckScript(source, hitMode, isCalled, cost, weapon) : cost;
}

// Implementation of item_w_mp_cost_ engine function with the hook
long __fastcall Items::item_w_mp_cost(fo::GameObject* source, long hitMode, long isCalled) {
	long cost = fo::func::item_w_mp_cost(source, hitMode, isCalled);
	return sf::CalcApCostHook_CheckScript(source, hitMode, isCalled, cost, nullptr);
}

void Items::init() {

}

}
