/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include "main.h"
#include "FalloutEngine.h"
#include "version.h"

static bool usingExtraKillTypes = false;

static DWORD __declspec(naked) ReadKillCounter() {
	__asm {
		movzx eax, word ptr ds:[_pc_kill_counts][eax * 2];
		retn;
	}
}

static void __declspec(naked) IncKillCounter() {
	__asm {
		inc  bx;
		mov  word ptr ds:[_pc_kill_counts][edx], bx;
		retn;
	}
}

void KillCounterInit() {
	usingExtraKillTypes = true;

	// Overwrite the critter_kill_count_ function that reads the kill counter
	MakeCall(0x42D8B5, ReadKillCounter, 2);

	// Overwrite the critter_kill_count_inc_ function that increments the kill counter
	MakeCall(0x42D89C, IncKillCounter, 1);
	SafeWrite8(0x42D88E, 0x45); // lea edx, [eax * 2]
	SafeWrite8(0x42D899, 0x90); // inc ebx > nop

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
	for (int i = 0; i < sizeof(extraKillTypesCountAddr) / 4; i++) {
		SafeWrite8(extraKillTypesCountAddr[i], 38);
	}
	SafeWrite32(0x42D9DD, 1488); // critter_kill_info_
}

bool UsingExtraKillTypes() {
	return usingExtraKillTypes;
}
