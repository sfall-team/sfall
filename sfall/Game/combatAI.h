/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
 *
 */

#pragma once

namespace game
{

#include "..\FalloutEngine\Fallout2.h"

class CombatAI {
public:
	static void init();

	static void __stdcall ai_check_drugs(fo::GameObject* source);
};

}
