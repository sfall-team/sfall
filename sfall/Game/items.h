/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace game
{

class Items {
public:
	static void init();

	static long GetHealingPID(long index);
	static void SetHealingPID(long index, long pid);

	static bool __fastcall IsHealingItem(fo::GameObject* item);

	// True - use failed
	static bool UseDrugItemFunc(fo::GameObject* source, fo::GameObject* item);

	// Implementation of item_d_take_ engine function with the HOOK_USEOBJON hook
	static long item_d_take_drug(fo::GameObject* source, fo::GameObject* item);

	static long item_remove_mult(fo::GameObject* source, fo::GameObject* item, long count, long rmType);

	static long item_count(fo::GameObject* who, fo::GameObject* item);

	static long item_weapon_range(fo::GameObject* source, fo::GameObject* weapon, long hitMode);

	//static long item_w_range(fo::GameObject* source, long hitMode);

	// Implementation of item_w_primary_mp_cost_ and item_w_secondary_mp_cost_ engine functions in a single function with the HOOK_CALCAPCOST hook
	// Note: Use only for weapons
	static long __fastcall item_weapon_mp_cost(fo::GameObject* source, fo::GameObject* weapon, long hitMode, long isCalled);

	// Implementation of item_w_mp_cost_ engine function with the HOOK_CALCAPCOST hook
	// Note: Can use the generic item_mp_cost_ function which has a hook call
	static long __fastcall item_w_mp_cost(fo::GameObject* source, fo::AttackType hitMode, long isCalled);

	static long __fastcall item_w_curr_ammo(fo::GameObject* item);
};

}
