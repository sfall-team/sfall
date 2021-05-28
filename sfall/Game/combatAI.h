/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace game
{

class CombatAI {
public:
	static void init();

	static bool ai_can_use_weapon(fo::GameObject* source, fo::GameObject* weapon, long hitMode);
};

}