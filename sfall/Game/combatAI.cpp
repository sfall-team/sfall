/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

//#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HookScripts\CombatHs.h"

#include "combatAI.h"

namespace game
{

namespace sf = sfall;

// Implementation of ai_can_use_weapon_ engine function with the HOOK_CANUSEWEAPON hook
long CombatAI::ai_can_use_weapon(fo::GameObject* source, fo::GameObject* weapon, long hitMode) {
	long result = fo::func::ai_can_use_weapon(source, weapon, hitMode);
	return sf::CanUseWeaponHook_Invoke(result, source, weapon, hitMode);
}

void CombatAI::init() { // TODO: add to main.cpp
}

}
