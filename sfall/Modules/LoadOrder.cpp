/*
*    sfall
*    Copyright (C) 2008-2017  The sfall team
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
#include "..\Logging.h"
#include "LoadGameHook.h"

#include "LoadOrder.h"

namespace sfall
{

static const char* msgFemaleFolder = "dialog_female\\%s.msg";
static bool isFemale    = false;
static bool femaleCheck = false;  // flag for check female dialog file
static DWORD format;

static void CheckPlayerGender() {
	isFemale = fo::HeroIsFemale();
}

static const DWORD scr_get_dialog_msg_file_Back = 0x4A6BD2;
static void __declspec(naked) scr_get_dialog_msg_file_hack1() {
	__asm {
		cmp  isFemale, 1;
		jnz  default;
		mov  format, eax;
		mov  femaleCheck, 1;
		push msgFemaleFolder;
		jmp  scr_get_dialog_msg_file_Back;
default:
		push FO_VAR_aDialogS_msg;            // default "dialog\%s.msg"
		jmp  scr_get_dialog_msg_file_Back;
	}
}

static void __declspec(naked) scr_get_dialog_msg_file_hack2() {
	__asm {
		cmp  eax, 1;          // checking existence of msg file
		mov  eax, 0x4A6C0E;
		jz   exist;
		cmp  femaleCheck, 1;
		jnz  error;           // no exist default msg file
		push format;
		push FO_VAR_aDialogS_msg;          // default "dialog\%s.msg"
		mov  femaleCheck, 0;               // reset flag
		jmp  scr_get_dialog_msg_file_Back; // check default msg file
error:
		mov  eax, 0x4A6BFA;   // jump to Error
exist:
		jmp  eax;
	}
}

static void __declspec(naked) removeDatabase() {
	__asm {
		cmp  eax, -1
		je   end
		mov  ebx, ds:[FO_VAR_paths]
		mov  ecx, ebx
nextPath:
		mov  edx, [esp+0x104+4+4]                 // path_patches
		mov  eax, [ebx]                           // database.path
		call fo::funcoffs::stricmp_
		test eax, eax                             // found path?
		jz   skip                                 // Yes
		mov  ecx, ebx
		mov  ebx, [ebx+0xC]                       // database.next
		jmp  nextPath
skip:
		mov  eax, [ebx+0xC]                       // database.next
		mov  [ecx+0xC], eax                       // database.next
		xchg ebx, eax
		cmp  eax, ecx
		jne  end
		mov  ds:[FO_VAR_paths], ebx
end:
		retn
	}
}

static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		call removeDatabase
		mov  ds:[FO_VAR_master_db_handle], eax
		retn
	}
}

static void __declspec(naked) game_init_databases_hack2() {
	__asm {
		cmp  eax, -1
		je   end
		mov  eax, ds:[FO_VAR_master_db_handle]
		mov  eax, [eax]                           // eax = master_patches.path
		call fo::funcoffs::xremovepath_
		dec  eax                                  // remove path (critter_patches == master_patches)?
		jz   end                                  // Yes
		inc  eax
		call removeDatabase
end:
		mov  ds:[FO_VAR_critter_db_handle], eax
		retn
	}
}

static void __declspec(naked) game_init_databases_hook() {
// eax = _master_db_handle
	__asm {
		mov  ecx, ds:[FO_VAR_critter_db_handle]
		mov  edx, ds:[FO_VAR_paths]
		jecxz skip
		mov  [ecx+0xC], edx                       // critter_patches.next->_paths
		mov  edx, ecx
skip:
		mov  [eax+0xC], edx                       // master_patches.next
		mov  ds:[FO_VAR_paths], eax
		retn
	}
}

void LoadOrder::init() {
	if (GetConfigInt("Misc", "DataLoadOrderPatch", 0)) {
		dlog("Applying data load order patch.", DL_INIT);
		MakeCall(0x444259, game_init_databases_hack1);
		MakeCall(0x4442F1, game_init_databases_hack2);
		HookCall(0x44436D, &game_init_databases_hook);
		SafeWrite8(0x4DFAEC, 0x1D); // error correction
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "DialogFemaleMsgFile", 0)) {
		dlog("Applying dialog files load order patch.", DL_INIT);
		MakeJump(0x4A6BCD, scr_get_dialog_msg_file_hack1);
		MakeJump(0x4A6BF5, scr_get_dialog_msg_file_hack2);
		dlogr(" Done", DL_INIT);
		LoadGameHook::OnAfterGameStarted() += CheckPlayerGender;
	}
}

}
