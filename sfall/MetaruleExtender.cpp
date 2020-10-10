/*
 *    sfall
 *    Copyright (C) 2008-2020  The sfall team
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

#define HorriganEncounterDays                   (0x4C06EA)
#define HorriganEncounterCheck                  (0x4C06D8)

static signed char HorriganEncounterDefaultDays = 35;
static signed char HorriganEncounterSetDays = 35;
static bool HorriganEncounterDisabled = false;

enum MetaruleFunction : long {
	SET_HORRIGAN_ENCOUNTER = 200, // sets the number of days for the Frank Horrigan encounter or disable encounter
	CLEAR_KEYBOARD_BUFFER  = 201, // clears the keyboard input buffer, should be used in the HOOK_KEYPRESS hook to clear keyboard events in some cases
};

/*
	args - contains a pointer to an array (size of 3) of arguments for the metarule3 function [located on the stack]
*/
static long __fastcall op_metarule3_ext(long metafunc, long* args) {
	long result = 0;

	switch (static_cast<MetaruleFunction>(metafunc)) {
		case SET_HORRIGAN_ENCOUNTER: {
			long argValue = args[0];        // arg1
			if (argValue <= 0) {            // disable Horrigan encounter
				if (*(BYTE*)HorriganEncounterCheck == CODETYPE_JumpNZ) {
					SafeWrite8(HorriganEncounterCheck, CODETYPE_JumpShort); // skip the Horrigan encounter check
					HorriganEncounterDisabled = true;
				}
			} else {
				if (argValue > 127) argValue = 127;
				SafeWrite8(HorriganEncounterDays, static_cast<BYTE>(argValue));
				HorriganEncounterSetDays = static_cast<signed char>(argValue);
			}
			break;
		}
		case CLEAR_KEYBOARD_BUFFER:
			__asm call kb_clear_;
			break;
		default:
			DebugPrintf("\nOPCODE ERROR: metarule3(%d, ...) - metarule function number does not exist.\n > Script: %s, procedure %s.",
						metafunc, (*ptr_currentProgram)->fileName, FindCurrentProc(*ptr_currentProgram));
			break;
	}
	return result;
}

static void __declspec(naked) op_metarule3_hack() {
	static const DWORD op_metarule3_hack_Ret = 0x45732C;
	__asm {
		cmp  ecx, 111;
		jnz  extended;
		retn;
extended:
		lea  edx, [esp + 0x4C - 0x4C + 4]; // pointer to args
		// swap arg1 <> arg3
		mov  eax, [edx];       // get: arg3
		xchg eax, [edx + 2*4]; // get: arg1, set: arg3 > arg1
		mov  [edx], eax;       // set: arg1 > arg3
		//
		call op_metarule3_ext; // ecx - metafunc arg
		add  esp, 4;
		jmp  op_metarule3_hack_Ret;
	}
}

void MetaruleExtenderReset() {
	if (HorriganEncounterSetDays != HorriganEncounterDefaultDays) {
		SafeWrite8(HorriganEncounterDays, HorriganEncounterDefaultDays);
	}
	if (HorriganEncounterDisabled) {
		HorriganEncounterDisabled = false;
		SafeWrite8(HorriganEncounterCheck, CODETYPE_JumpNZ); // enable
	}
}

void MetaruleExtenderInit() {
	// Keep default value
	HorriganEncounterDefaultDays = *(BYTE*)HorriganEncounterDays;

	MakeCall(0x457322, op_metarule3_hack);
}
