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

#include "Module.h"

namespace sfall
{

class AI : public Module {
public:
	const char* name() { return "AI"; }
	void init();

	static fo::GameObject* sf_check_critters_in_lof(fo::GameObject* object, DWORD checkTile, DWORD team);
	static fo::GameObject* CheckFriendlyFire(fo::GameObject* target, fo::GameObject* attacker);

	// TODO: use subscription instead
	static void __stdcall AICombatStart();
	static void __stdcall AICombatEnd();

	static fo::GameObject* __stdcall AIGetLastAttacker(fo::GameObject* target);
	static fo::GameObject* __stdcall AIGetLastTarget(fo::GameObject* source);
};

}
