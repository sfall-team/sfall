/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace game
{

class Items {
public:
	static void init();

	static long item_weapon_range(fo::GameObject* source, fo::GameObject* weapon, long hitMode);

	// Implementation of item_w_primary_mp_cost_ and item_w_secondary_mp_cost_ engine functions in a single function with the HOOK_CALCAPCOST hook
	// Note: Use only for weapons
	static long __fastcall item_weapon_mp_cost(fo::GameObject* source, fo::GameObject* weapon, long hitMode, long isCalled);

	// Implementation of item_w_mp_cost_ engine function with the HOOK_CALCAPCOST hook
	// Note: Use the generic item_mp_cost function which has a hook call
	static long item_w_mp_cost(fo::GameObject* source, long hitMode, long isCalled);
};

}