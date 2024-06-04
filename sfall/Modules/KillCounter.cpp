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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\version.h"

#include "KillCounter.h"

namespace sfall
{

static bool usingExtraKillTypes = false;

static __declspec(naked) DWORD ReadKillCounter() {
	__asm {
		movzx eax, word ptr ds:[FO_VAR_pc_kill_counts][eax * 2];
		retn;
	}
}

static __declspec(naked) void IncKillCounter() {
	__asm {
		inc  bx;
		mov  word ptr ds:[FO_VAR_pc_kill_counts][edx], bx;
		retn;
	}
}

static void KillCounterInit() {
	usingExtraKillTypes = true;

	// Overwrite the critter_kill_count_ function that reads the kill counter
	MakeCall(0x42D8B5, ReadKillCounter, 2);

	// Overwrite the critter_kill_count_inc_ function that increments the kill counter
	MakeCall(0x42D89C, IncKillCounter, 1);
	SafeWrite8(0x42D88E, 0x45);          // lea edx, [eax * 2]
	SafeWrite8(0x42D899, CodeType::Nop); // inc ebx > nop

	const DWORD extraKillTypesCountAddr[] = {
		0x42D8AF, // critter_kill_count_
		0x42D881, // critter_kill_count_inc_
		0x42D980, // GetKillTypeName
		0x42D990,
		0x42D9C0, // GetKillTypeDesc
		0x42D9D0,
		0x4344E4, // Change char sheet to loop through the extra kill types
	};

	// Edit the functions to accept kill types over 19
	SafeWriteBatch<BYTE>(38, extraKillTypesCountAddr);
	SafeWrite32(0x42D9DD, 1488); // critter_kill_info_
}

bool KillCounter::UsingExtraKillTypes() {
	return usingExtraKillTypes;
}

void KillCounter::init() {
	if (IniReader::GetConfigInt("Misc", "ExtraKillTypes", 0)) {
		dlogr("Applying extra kill types patch.", DL_INIT);
		KillCounterInit();
	}
}

}
