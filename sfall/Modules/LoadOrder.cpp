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

static long femaleMsgs;

static const char* cutsEndGameFemale = "text\\%s\\cuts_female\\";
static const char* cutsSubFemale     = "text\\%s\\cuts_female\\%s";
static const char* cutsDeathFemale   = "text\\%s\\cuts_female\\%s%s";
static const char* msgFemaleFolder   = "dialog_female\\%s.msg";

static bool isFemale    = false;
static bool femaleCheck = false;  // flag for check female dialog file
static DWORD format;
static bool cutsPatch   = false;

static std::vector<std::string> patchFiles;

static void CheckPlayerGender() {
	isFemale = fo::HeroIsFemale();

	if (femaleMsgs > 1) {
		if (isFemale) {
			if (cutsPatch) return;
			cutsPatch = true;
			SafeWrite32(0x43FA9F, (DWORD)cutsEndGameFemale);
			SafeWrite32(0x44EB5B, (DWORD)cutsSubFemale);
			SafeWrite32(0x48152E, (DWORD)cutsDeathFemale);
		} else if (cutsPatch) {
			SafeWrite32(0x43FA9F, FO_VAR_aTextSCuts);
			SafeWrite32(0x44EB5B, FO_VAR_aTextSCutsS);
			SafeWrite32(0x48152E, FO_VAR_aTextSCutsSS);
			cutsPatch = false;
		}
	}
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
		push FO_VAR_aDialogS_msg;          // default "dialog\%s.msg"
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

static void __declspec(naked) gnw_main_hack() {
	__asm {
		push eax;
		call CheckPlayerGender;
		pop  eax;
		mov  edx, 4; // overwritten engine code
		retn;
	}
}

static fo::PathNode* __fastcall RemoveDatabase(const char* pathPatches) {
	auto paths = fo::var::paths; // curr.node (beginning chain of paths)
	auto _paths = paths;         // prev.node

	while (paths) {
		if (_stricmp(paths->path, pathPatches) == 0) { // found path
			fo::PathNode* nextPaths = paths->next;     // pointer to the node of the next path
// TODO: need to check this condition, did i use it correctly?
			if (paths != _paths)
				_paths->next = nextPaths;              // replace the pointer in the previous node, removing the current(found) path from the chain
			else                                       // if the current node is equal to the previous node
				fo::var::paths = nextPaths;            // set the next node as the beginning of the chain
			return paths;                              // return the pointer of the current removed node (save the pointer)
		}
		_paths = paths;      // prev.node <- curr.node
		paths = paths->next; // take a pointer to the next path from the current node
	}
	return nullptr; // it is possible that this will create an exceptional situation for the game, although such a situation should not arise
}

// Remove master_patches from the chain
static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		cmp  eax, -1;
		je   skip;
		mov  ecx, [esp + 0x104 + 4]; // path_patches
		call RemoveDatabase;
skip:
		mov  ds:[FO_VAR_master_db_handle], eax; // pointer of the master_patches node will be saved here
		retn;
	}
}

// Remove critter_patches from the chain
static void __declspec(naked) game_init_databases_hack2() {
	__asm {
		cmp  eax, -1;
		je   end;
		mov  eax, ds:[FO_VAR_master_db_handle];   // pointer to master_patches node
		mov  eax, [eax];                          // eax = master_patches.path
		call fo::funcoffs::xremovepath_;
		dec  eax;                                 // remove path (critter_patches == master_patches)?
		jz   end;                                 // yes (jump if 0)
		mov  ecx, [esp + 0x104 + 4];              // path_patches
		call RemoveDatabase;
end:
		mov  ds:[FO_VAR_critter_db_handle], eax;  // the pointer of the critter_patches node will be saved here
		retn;
	}
}

static void __stdcall InitExtraPatches() {
	for (auto it = patchFiles.begin(); it != patchFiles.end(); it++) {
		fo::func::db_init(it->c_str(), 0);
	}
}

static void __fastcall game_init_databases_hook() {
	fo::PathNode* master_patches;
	_asm mov master_patches, eax;         // eax = _master_db_handle

	if (!patchFiles.empty()) InitExtraPatches();

	fo::PathNode* critter_patches = (fo::PathNode*)fo::var::critter_db_handle;
	fo::PathNode* paths = fo::var::paths; // beginning chain of paths
	// insert master_patches/critter_patches at the beginning of the path chain
	if (critter_patches) {
		critter_patches->next = paths;    // critter_patches.next -> paths
		paths = critter_patches;
	}
	master_patches->next = paths;         // master_patches.next -> paths
	fo::var::paths = master_patches;      // set master_patches node to the beginning of the chain of paths
}

static void GetExtraPatches() {
	char patchFile[12] = "PatchFile";
	for (int i = 0; i < 100; i++) {
		_itoa(i, &patchFile[9], 10);
		auto patch = GetConfigString("ExtraPatches", patchFile, "", MAX_PATH);
		if (patch.empty() || GetFileAttributes(patch.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
		patchFiles.push_back(patch);
	}
}

void LoadOrder::init() {
	GetExtraPatches();

	if (GetConfigInt("Misc", "DataLoadOrderPatch", 0)) {
		dlog("Applying data load order patch.", DL_INIT);
		MakeCall(0x444259, game_init_databases_hack1);
		MakeCall(0x4442F1, game_init_databases_hack2);
		HookCall(0x44436D, game_init_databases_hook);
		SafeWrite8(0x4DFAEC, 0x1D); // error correction (ecx > ebx)
		dlogr(" Done", DL_INIT);
	} else if (!patchFiles.empty()) {
		HookCall(0x44436D, InitExtraPatches);
	}

	femaleMsgs = GetConfigInt("Misc", "FemaleDialogMsgs", 0);
	if (femaleMsgs) {
		dlog("Applying alternative female dialog files patch.", DL_INIT);
		MakeJump(0x4A6BCD, scr_get_dialog_msg_file_hack1);
		MakeJump(0x4A6BF5, scr_get_dialog_msg_file_hack2);
		LoadGameHook::OnAfterGameStarted() += CheckPlayerGender;
		if (femaleMsgs > 1) {
			MakeCall(0x480A95, gnw_main_hack); // before new game start from main menu. TODO: need moved to address 0x480A9A (it busy in movies.cpp)
			LoadGameHook::OnGameExit() += []() {
				if (cutsPatch) { // restore
					SafeWrite32(0x43FA9F, FO_VAR_aTextSCuts);
					SafeWrite32(0x44EB5B, FO_VAR_aTextSCutsS);
					SafeWrite32(0x48152E, FO_VAR_aTextSCutsSS);
					cutsPatch = false;
				}
			};
		}
		dlogr(" Done", DL_INIT);
	}
}

}
