/*
 *    sfall
 *    Copyright (C) 2008-2025  The sfall team
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

#include "Module.h"

namespace sfall
{

class PartyControl : public Module {
public:
	const char* name() { return "PartyControl"; }
	void init();
	void exit() override;

	static int __fastcall SwitchHandHook(fo::GameObject* item);

	static bool IsNpcControlled();

	// Take control of given NPC or switch back to "Real" dude if nullptr is passed
	static void SwitchToCritter(fo::GameObject* critter);

	// Returns pointer to "real" dude, which is different from "dude_obj" when controlling another critter
	static fo::GameObject* RealDudeObject();

	static void OrderAttackPatch();
};

extern bool npcEngineLevelUp;

}
