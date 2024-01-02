/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace game
{
namespace ai
{

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
}
