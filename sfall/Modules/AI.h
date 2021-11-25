/*
 *    sfall
 *    Copyright (C) 2012  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "..\FalloutEngine\Fallout2.h"

namespace sfall
{

class AI {
public:
	static const char* name() { return "AI"; }
	static void init();

	static void AICombatClear();

	static fo::GameObject* __stdcall AIGetLastAttacker(fo::GameObject* target);
	static fo::GameObject* __stdcall AIGetLastTarget(fo::GameObject* source);
};

class AIHelpers {
public:
	// Returns the friendly critter or any blocking object in the line of fire
	static fo::GameObject* CheckShootAndFriendlyInLineOfFire(fo::GameObject* object, long targetTile, long team);

	// Returns the friendly critter in the line of fire
	static fo::GameObject* CheckFriendlyFire(fo::GameObject* target, fo::GameObject* attacker);

	static bool AttackInRange(fo::GameObject* source, fo::GameObject* weapon, long distance);
	static bool AttackInRange(fo::GameObject* source, fo::GameObject* weapon, fo::GameObject* target);
};

}
