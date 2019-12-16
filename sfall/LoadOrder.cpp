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

#include "main.h"
#include "FalloutEngine.h"

static std::vector<int> savPrototypes;

static PathNode* __fastcall RemoveDatabase(const char* pathPatches) {
	PathNode* paths = *ptr_paths; // curr.node (beginning of the chain of paths)
	PathNode* pPaths = paths;     // prev.node

	while (paths) {
		if (_stricmp(paths->path, pathPatches) == 0) { // found path
			PathNode* nextPaths = paths->next;         // pointer to the node of the next path
// TODO: need to check if this condition is used correctly
			if (paths != pPaths)
				pPaths->next = nextPaths;              // replace the pointer in the previous node, removing the current(found) path from the chain
			else                                       // if the current node is equal to the previous node
				*ptr_paths = nextPaths;                // set the next node at the beginning of the chain
			return paths;                              // return the pointer of the current removed node (save the pointer)
		}
		pPaths = paths;      // prev.node <- curr.node
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
		mov  ds:[_master_db_handle], eax;         // the pointer of master_patches node will be saved here
		retn;
	}
}

// Remove critter_patches from the chain
static void __declspec(naked) game_init_databases_hack2() {
	__asm {
		cmp  eax, -1;
		je   end;
		mov  eax, ds:[_master_db_handle];         // pointer to master_patches node
		mov  eax, [eax];                          // eax = master_patches.path
		call xremovepath_;
		dec  eax;                                 // remove path (critter_patches == master_patches)?
		jz   end;                                 // Yes (jump if 0)
		mov  ecx, [esp + 0x104 + 4];              // path_patches
		call RemoveDatabase;
end:
		mov  ds:[_critter_db_handle], eax;        // the pointer of critter_patches node will be saved here
		retn;
	}
}

static void __fastcall game_init_databases_hook() { // eax = _master_db_handle
	PathNode* master_patches = *ptr_master_db_handle;
	PathNode* critter_patches = *ptr_critter_db_handle;
	PathNode* paths = *ptr_paths; // beginning of the chain of paths
	// insert master_patches/critter_patches at the beginning of the chain of paths
	if (critter_patches) {
		critter_patches->next = paths;    // critter_patches.next -> paths
		paths = critter_patches;
	}
	master_patches->next = paths;         // master_patches.next -> paths
	*ptr_paths = master_patches;          // set master_patches node at the beginning of the chain of paths
}

static void __fastcall game_init_databases_hook1() {
	char masterPatch[MAX_PATH];
	iniGetString("system", "master_patches", "", masterPatch, MAX_PATH - 1, (const char*)_gconfig_file_name);

	PathNode* node = *ptr_paths;
	while (node->next) {
		if (!strcmp(node->path, masterPatch)) break;
		node = node->next;
	}
	*ptr_master_db_handle = node; // set pointer to master_patches node
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
	for (std::vector<int>::const_iterator it = savPrototypes.begin(); it != savPrototypes.end(); ++it) {
		if (*it == pid) return;
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
	for (std::vector<int>::const_iterator it = savPrototypes.begin(); it != savPrototypes.end(); ++it) {
		if (*it == pid) {
			ChangePrototypeExt(path);
			break;
		}
	}
}

static long __fastcall CheckProtoType(long pid, char* path) {
	if (pid >> 24 != OBJ_TYPE_CRITTER) return 0;
	return ChangePrototypeExt(path);
}

// saves prototypes (all party members) before saving game or exiting the map
static void __declspec(naked) proto_save_pid_hook() {
	__asm {
		push ecx;
		call proto_list_str_;
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
	__asm { // eax - party pid
		push ecx;
		mov  ecx, eax;
		call proto_list_str_;
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

#define _F_SAV (const char*)0x50A480
#define _F_PROTO_CRITTERS (const char*)0x50A490

void RemoveSavFiles() {
	MapDirErase(_F_PROTO_CRITTERS, _F_SAV);
}

void ClearSavPrototypes() {
	savPrototypes.clear();
	RemoveSavFiles();
}

void LoadOrderInit() {
	MultiPatchesPatch();

	if (GetConfigInt("Misc", "DataLoadOrderPatch", 1)) {
		dlog("Applying data load order patch.", DL_INIT);
		MakeCall(0x444259, game_init_databases_hack1);
		MakeCall(0x4442F1, game_init_databases_hack2);
		HookCall(0x44436D, game_init_databases_hook);
		SafeWrite8(0x4DFAEC, 0x1D); // error correction (ecx > ebx)
		dlogr(" Done", DL_INIT);
	} else {
		HookCall(0x44436D, game_init_databases_hook1);
	}

	dlog("Applying party member protos save/load patch.", DL_INIT);
	savPrototypes.reserve(25);
	HookCall(0x4A1CF2, proto_load_pid_hook);
	HookCall(0x4A1BEE, proto_save_pid_hook);
	MakeCall(0x47F5A5, GameMap2Slot_hack); // save game
	MakeCall(0x47FB80, SlotMap2Game_hack); // load game
	MakeCall(0x47FBBF, SlotMap2Game_hack_attr, 1);
	dlogr(" Done", DL_INIT);
}
