/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

namespace sfall
{

class PartyControl {
public:
	static const char* name() { return "PartyControl"; }
	static void init();

	static void OnGameLoad();

	static int __fastcall SwitchHandHook(fo::GameObject* item);

	static bool IsNpcControlled();

	// Returns pointer to "real" dude, which is different from "dude_obj" when controlling another critter
	static fo::GameObject* RealDudeObject();
};

extern bool npcAutoLevelEnabled;
extern bool npcEngineLevelUp;

}
