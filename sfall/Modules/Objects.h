/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

enum UniqueID : long {
	Start = 0x0FFFFFFF, // start at 0x10000000
	End   = 0x7FFFFFFF
};

class Objects : public Module {
public:
	const char* name() { return "Objects"; }
	void init();

	static long uniqueID;

	static bool IsUniqueID(long id);

	static long __fastcall SetObjectUniqueID(fo::GameObject* obj);
	static long __fastcall SetSpecialID(fo::GameObject* obj);
	static void SetNewEngineID(fo::GameObject* obj);

	static void SetAutoUnjamLockTime(DWORD time);
	static void LoadProtoAutoMaxLimit();
};

}
