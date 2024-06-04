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
#include "ExtraSaveSlots.h"
#include "LoadGameHook.h"
#include "PartyControl.h"

#include "MetaruleExtender.h"

namespace sfall
{

#define HorriganEncounterDays                   (0x4C06EA)
#define HorriganEncounterCheck                  (0x4C06D8)

static signed char HorriganEncounterDefaultDays = 35;
static signed char HorriganEncounterSetDays = 35;
static bool HorriganEncounterDisabled = false;

enum class MetaruleFunction : long {
	SET_HORRIGAN_ENCOUNTER = 200, // sets the number of days for the Frank Horrigan encounter or disable encounter
	CLEAR_KEYBOARD_BUFFER  = 201, // clears the keyboard input buffer, should be used in the HOOK_KEYPRESS hook to clear keyboard events in some cases

	GET_CURRENT_SAVE_SLOT  = 210,
	SET_CURRENT_SAVE_SLOT  = 211,
	GET_CURRENT_QSAVE_PAGE = 212,
	GET_CURRENT_QSAVE_SLOT = 213,
	SET_CURRENT_QSAVE_SLOT = 214,

	PARTY_ORDER_ATTACK     = 999,
};

/*
	args - contains a pointer to an array (size of 3) of arguments for the metarule3 function [located on the stack]
*/
static long __fastcall op_metarule3_ext(long metafunc, long* args) {
	long result = 0;

	switch (static_cast<MetaruleFunction>(metafunc)) {
	case MetaruleFunction::SET_HORRIGAN_ENCOUNTER: {
		long argValue = args[0];        // arg1
		if (argValue <= 0) {            // disable Horrigan encounter
			if (*(BYTE*)HorriganEncounterCheck == CodeType::JumpNZ) {
				SafeWrite8(HorriganEncounterCheck, CodeType::JumpShort); // skip the Horrigan encounter check
				HorriganEncounterDisabled = true;
			}
		} else {
			if (argValue > 127) argValue = 127;
			SafeWrite8(HorriganEncounterDays, static_cast<BYTE>(argValue));
			HorriganEncounterSetDays = static_cast<signed char>(argValue);
		}
		break;
	}
	case MetaruleFunction::CLEAR_KEYBOARD_BUFFER:
		__asm call fo::funcoffs::kb_clear_;
		break;
	case MetaruleFunction::PARTY_ORDER_ATTACK:
		PartyControl::OrderAttackPatch();
		break;
	case MetaruleFunction::GET_CURRENT_SAVE_SLOT:
		result = ExtraSaveSlots::GetSaveSlot();
		break;
	case MetaruleFunction::SET_CURRENT_SAVE_SLOT:
		ExtraSaveSlots::SetSaveSlot(args[0], args[1]);
		break;
	case MetaruleFunction::GET_CURRENT_QSAVE_PAGE:
		result = ExtraSaveSlots::GetQuickSavePage();
		break;
	case MetaruleFunction::GET_CURRENT_QSAVE_SLOT:
		result = ExtraSaveSlots::GetQuickSaveSlot();
		break;
	case MetaruleFunction::SET_CURRENT_QSAVE_SLOT:
		ExtraSaveSlots::SetQuickSaveSlot(args[0], args[1], args[2]);
		break;
	default:
		fo::func::debug_printf("\nOPCODE ERROR: metarule3(%d, ...) - metarule function number does not exist.\n > Script: %s, procedure %s.",
		                       metafunc, fo::var::currentProgram->fileName, fo::func::findCurrentProc(fo::var::currentProgram));
		break;
	}
	return result;
}

static __declspec(naked) void op_metarule3_hack() {
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

static void Reset() {
	if (HorriganEncounterSetDays != HorriganEncounterDefaultDays) {
		SafeWrite8(HorriganEncounterDays, HorriganEncounterDefaultDays);
	}
	if (HorriganEncounterDisabled) {
		HorriganEncounterDisabled = false;
		SafeWrite8(HorriganEncounterCheck, CodeType::JumpNZ); // enable
	}
}

void MetaruleExtender::init() {
	// Keep default value
	HorriganEncounterDefaultDays = *(BYTE*)HorriganEncounterDays;

	MakeCall(0x457322, op_metarule3_hack);

	LoadGameHook::OnGameReset() += Reset;
}

}
