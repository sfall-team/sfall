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

static void __declspec(naked) RemoveDatabase() {
	__asm {
		cmp  eax, -1;
		je   end;
		mov  ebx, ds:[_paths];
		mov  ecx, ebx;
nextPath:
		mov  edx, [esp + 0x104 + 4 + 4];          // path_patches
		mov  eax, [ebx];                          // database.path
		call stricmp_;
		test eax, eax;                            // found path?
		jz   skip;                                // Yes
		mov  ecx, ebx;
		mov  ebx, [ebx + 0xC];                    // database.next
		jmp  nextPath;
skip:
		mov  eax, [ebx + 0xC];                    // database.next
		mov  [ecx + 0xC], eax;                    // database.next
		xchg ebx, eax;
		cmp  eax, ecx;
		jne  end;
		mov  ds:[_paths], ebx;
end:
		retn;
	}
}

// Remove master_patches from the chain
static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		call RemoveDatabase;
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
		inc  eax;
		call RemoveDatabase;
end:
		mov  ds:[_critter_db_handle], eax;        // the pointer of critter_patches node will be saved here
		retn;
	}
}

static void __declspec(naked) game_init_databases_hook() {
	// eax = _master_db_handle
	__asm {
		mov  ecx, ds:[_critter_db_handle];
		mov  edx, ds:[_paths];
		test ecx, ecx;
		jz   skip;
		mov  [ecx + 0xC], edx;                    // critter_patches.next->_paths
		mov  edx, ecx;
skip:
		mov  [eax + 0xC], edx;                    // master_patches.next
		mov  ds:[_paths], eax;
		retn;
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
