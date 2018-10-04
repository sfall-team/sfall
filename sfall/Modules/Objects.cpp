/*
 *    sfall
 *    Copyright (C) 2008-2018  The sfall team
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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Objects.h"

namespace sfall
{

static int unjamTimeState;

void Objects::SetAutoUnjamLockTime(DWORD time) {
	if (!unjamTimeState) {
		BlockCall(0x4A364A); // disable auto unjam at midnight
	}

	if (time > 0) {
		SafeWrite8(0x4831D9, (BYTE)time);
		if (unjamTimeState == 2) {
			SafeWrite8(0x4831DA, 0x7C);
		}
		unjamTimeState = 1;
	} else {
		SafeWrite8(0x4831DA, 0xEB); // disable auto unjam
		unjamTimeState = 2;
	}
}

void RestoreObjUnjamAllLocks() {
	if (unjamTimeState) {
		SafeWrite8(0x4A364A, 0xE8);
		SafeWrite32(0x4A364B, 0xFFFF9E69);
		SafeWrite8(0x4831DA, 0x7C);
		SafeWrite8(0x4831D9, 24);
		unjamTimeState = 0;
	}
}

void Objects::init() {
	LoadGameHook::OnGameReset() += [] () {
		RestoreObjUnjamAllLocks();
	};
}

}
