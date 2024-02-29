/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace game
{

#include "..\FalloutEngine\Fallout2.h"

class CombatAI {
public:
	static void init();

	static bool ai_can_use_weapon(fo::GameObject* source, fo::GameObject* weapon, long hitMode);

	static void __stdcall ai_check_drugs(fo::GameObject* source);
};

}
