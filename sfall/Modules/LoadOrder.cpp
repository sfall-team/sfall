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
static std::vector<int> savPrototypes;

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
	auto paths = fo::var::paths; // curr.node (beginning of the chain of paths)
	auto _paths = paths;         // prev.node

	while (paths) {
		if (_stricmp(paths->path, pathPatches) == 0) { // found path
			fo::PathNode* nextPaths = paths->next;     // pointer to the node of the next path
// TODO: need to check if this condition is used correctly (everything seems to be in order here)
			if (paths != _paths)
				_paths->next = nextPaths;              // replace the pointer in the previous node, removing the current(found) path from the chain
			else                                       // if the current node is equal to the previous node
				fo::var::paths = nextPaths;            // set the next node at the beginning of the chain
			return paths;                              // return the pointer of the current removed node (save the pointer)
		}
		_paths = paths;      // prev.node <- curr.node
		paths = paths->next; // take a pointer to the next path from the current node
	}
	return nullptr; // it's possible that this will create an exceptional situation for the game, although such a situation should not arise
}

// Remove master_patches from the chain
static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		cmp  eax, -1;
		je   skip;
		mov  ecx, [esp + 0x104 + 4]; // path_patches
		call RemoveDatabase;
skip:
		mov  ds:[FO_VAR_master_db_handle], eax;   // the pointer of master_patches node will be saved here
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
		jz   end;                                 // Yes (jump if 0)
		mov  ecx, [esp + 0x104 + 4];              // path_patches
		call RemoveDatabase;
end:
		mov  ds:[FO_VAR_critter_db_handle], eax;  // the pointer of critter_patches node will be saved here
		retn;
	}
}

static void InitExtraPatches() {
	for (auto it = patchFiles.begin(); it != patchFiles.end(); ++it) {
		if (!it->empty()) fo::func::db_init(it->c_str(), 0);
	}
	// free memory
	patchFiles.clear();
	patchFiles.shrink_to_fit();
}

static void __fastcall game_init_databases_hook() { // eax = _master_db_handle
	fo::PathNode* master_patches = fo::var::master_db_handle;

	/*if (!patchFiles.empty())*/ InitExtraPatches();

	fo::PathNode* critter_patches = fo::var::critter_db_handle;
	fo::PathNode* paths = fo::var::paths; // beginning of the chain of paths
	// insert master_patches/critter_patches at the beginning of the chain of paths
	if (critter_patches) {
		critter_patches->next = paths;    // critter_patches.next -> paths
		paths = critter_patches;
	}
	master_patches->next = paths;         // master_patches.next -> paths
	fo::var::paths = master_patches;      // set master_patches node at the beginning of the chain of paths
}

static void __fastcall game_init_databases_hook1() {
	char masterPatch[MAX_PATH];
	iniGetString("system", "master_patches", "", masterPatch, MAX_PATH - 1, (const char*)FO_VAR_gconfig_file_name);

	fo::PathNode* node = fo::var::paths;
	while (node->next) {
		if (!strcmp(node->path, masterPatch)) break;
		node = node->next;
	}
	fo::var::master_db_handle = node; // set pointer to master_patches node

	InitExtraPatches();
}

static bool NormalizePath(std::string &path) {
	if (path.find(':') != std::string::npos) return false;
	int pos = 0;
	do { // replace all '/' char to '\'
		pos = path.find('/', pos);
		if (pos != std::string::npos) path[pos] = '\\';
	} while (pos != std::string::npos);
	if (path.find(".\\") != std::string::npos || path.find("..\\") != std::string::npos) return false;
	while (path.front() == '\\') path.erase(0, 1); // remove firsts '\'
	return true;
}

// Patches placed at the back of the vector will have priority in the chain over the front(previous) patches
static void GetExtraPatches() {
	char patchFile[12] = "PatchFile";
	for (int i = 0; i < 100; i++) {
		_itoa(i, &patchFile[9], 10);
		auto patch = GetConfigString("ExtraPatches", patchFile, "", MAX_PATH);
		if (patch.empty() || !NormalizePath(patch) || GetFileAttributes(patch.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
		patchFiles.push_back(patch);
	}
	std::string searchPath = "mods\\"; //GetConfigString("ExtraPatches", "AutoSearchPath", "mods\\", MAX_PATH);
	//if (!searchPath.empty() && NormalizePath(searchPath)) {
		//if (searchPath.back() != '\\') searchPath += "\\";

		std::string pathMask(".\\mods\\*.dat");
		dlogr("Loading custom patches:", DL_MAIN);
		WIN32_FIND_DATA findData;
		HANDLE hFind = FindFirstFile(pathMask.c_str(), &findData);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				std::string name(searchPath + findData.cFileName);
				if ((name.length() - name.find_last_of('.')) > 4) continue;
				dlog_f("> %s\n", DL_MAIN, name.c_str());
				patchFiles.push_back(name);
			} while (FindNextFile(hFind, &findData));
			FindClose(hFind);
		}
	//}
	// Remove first duplicates
	size_t size = patchFiles.size();
	for (size_t i = 1; i < size; ++i) {
		for(size_t j = size - 1; j > i; --j) {
			if (patchFiles[j] == patchFiles[i]) {
				patchFiles[i].clear();
			}
		}
	}
}

static void MultiPatchesPatch() {
	//if (GetConfigInt("Misc", "MultiPatches", 0)) {
		dlog("Applying load multiple patches patch.", DL_INIT);
		SafeWrite8(0x444354, 0x90); // Change step from 2 to 1
		SafeWrite8(0x44435C, 0xC4); // Disable check
		dlogr(" Done", DL_INIT);
	//}
}

////////////////////////////// SAVE PARTY MEMBER PROTOTYPES //////////////////////////////

static void __fastcall AddSavPrototype(long pid) {
	for (const auto& _pid : savPrototypes) {
		if (_pid == pid) return;
	}
	savPrototypes.push_back(pid);
}

static long ChangePrototypeExt(char* path) {
	long len = strlen(path);
	if (len) {
		len -= 4;
		if (path[len] == '.') {
			path[++len] = 's';
			path[++len] = 'a';
			path[++len] = 'v';
		} else {
			len = 0;
		}
	}
	return len;
}

static void __fastcall ExistSavPrototype(long pid, char* path) {
	if (savPrototypes.empty()) return;
	for (const auto& _pid : savPrototypes) {
		if (_pid == pid) {
			ChangePrototypeExt(path);
			break;
		}
	}
}

static long __fastcall CheckProtoType(long pid, char* path) {
	if (pid >> 24 != fo::OBJ_TYPE_CRITTER) return 0;
	return ChangePrototypeExt(path);
}

// saves prototypes (all party members) before saving game or exiting the map
static void __declspec(naked) proto_save_pid_hook() {
	__asm {
		push ecx;
		call fo::funcoffs::proto_list_str_;
		test eax, eax;
		jnz  end;
		mov  ecx, ebx; // party pid
		mov  edx, edi; // path buffer
		call CheckProtoType;
		test eax, eax;
		jz   end;
		mov  ecx, ebx; // party pid
		call AddSavPrototype;
end:
		pop  ecx;
		retn;
	}
}

#define _F_PATHFILE 0x6143F4
static void __declspec(naked) GameMap2Slot_hack() { // save party pids
	__asm {
		push ecx;
		mov  edx, _F_PATHFILE; // path buffer
		call CheckProtoType;   // ecx - party pid
		pop  ecx;
		lea  eax, [esp + 0x14 + 4];
		retn 0x14;
	}
}

static void __declspec(naked) SlotMap2Game_hack() { // load party pids
	__asm {
		push edx;
		mov  ecx, edi;         // party pid
		mov  edx, _F_PATHFILE; // path buffer
		call CheckProtoType;
		test eax, eax;
		jz   end;
		mov  ecx, edi;
		call AddSavPrototype;
end:
		pop  edx;
		lea  eax, [esp + 0x14 + 4];
		retn 0x14;
	}
}

static void __declspec(naked) proto_load_pid_hook() {
using namespace fo;
	__asm { // eax - party pid
		push ecx;
		mov  ecx, eax;
		call fo::funcoffs::proto_list_str_;
		test eax, eax;
		jnz  end;
		// check pid type
		mov  edx, ecx;
		shr  edx, 24;
		cmp  edx, OBJ_TYPE_CRITTER;
		jnz  end;
		mov  edx, edi;          // path buffer
		call ExistSavPrototype; // ecx - party pid
		xor  eax, eax;
end:
		pop  ecx;
		retn;
	}
}

static void ResetReadOnlyAttr() {
	DWORD attr = GetFileAttributesA((const char*)_F_PATHFILE);
	if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_READONLY)) {
		SetFileAttributesA((const char*)_F_PATHFILE, (attr & ~FILE_ATTRIBUTE_READONLY));
	}
}

static void __declspec(naked) SlotMap2Game_hack_attr() {
using namespace fo;
	__asm {
		cmp  eax, -1;
		je   end;
		cmp  ebx, OBJ_TYPE_CRITTER;
		jne  end;
		call ResetReadOnlyAttr;
		or   eax, 1; // reset ZF
end:
		retn 0x8;
	}
}

#define _F_SAV            (const char*)0x50A480
#define _F_PROTO_CRITTERS (const char*)0x50A490

static void RemoveSavFiles() {
	fo::func::MapDirErase(_F_PROTO_CRITTERS, _F_SAV);
}

void LoadOrder::init() {
	// Load external sfall resource file (load order is before patchXXX.dat)
	patchFiles.push_back("sfall.dat");

	GetExtraPatches();
	MultiPatchesPatch();

	if (GetConfigInt("Misc", "DataLoadOrderPatch", 1)) {
		dlog("Applying data load order patch.", DL_INIT);
		MakeCall(0x444259, game_init_databases_hack1);
		MakeCall(0x4442F1, game_init_databases_hack2);
		HookCall(0x44436D, game_init_databases_hook);
		SafeWrite8(0x4DFAEC, 0x1D); // error correction (ecx > ebx)
		dlogr(" Done", DL_INIT);
	} else /*if (!patchFiles.empty())*/ {
		HookCall(0x44436D, game_init_databases_hook1);
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

	dlog("Applying party member protos save/load patch.", DL_INIT);
	savPrototypes.reserve(25);
	HookCall(0x4A1CF2, proto_load_pid_hook);
	HookCall(0x4A1BEE, proto_save_pid_hook);
	MakeCall(0x47F5A5, GameMap2Slot_hack); // save game
	MakeCall(0x47FB80, SlotMap2Game_hack); // load game
	MakeCall(0x47FBBF, SlotMap2Game_hack_attr, 1);
	dlogr(" Done", DL_INIT);

	LoadGameHook::OnAfterGameInit() += RemoveSavFiles;
	LoadGameHook::OnGameReset() += []() {
		savPrototypes.clear();
		RemoveSavFiles();
	};
}

}
