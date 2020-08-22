#pragma once

#include "..\Delegate.h"
#include "Module.h"

namespace sfall
{

class MainLoopHook : public Module {
public:
	const char* name() { return "MainLoopHook"; }

	void init();

	// Main game loop (real-time action)
	static Delegate<>& OnMainLoop();

	// Turn-based combat loop
	static Delegate<>& OnCombatLoop();

	// Called after each attack action in combat (at the end of combat_attack() function)
	static Delegate<>& OnAfterCombatAttack();

	static bool displayWinUpdateState;
};

}
