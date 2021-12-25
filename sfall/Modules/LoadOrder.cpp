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

#include "LoadOrder.h"

namespace sfall
{

long femaleMsgs;

static const char* cutsEndGameFemale = "text\\%s\\cuts_female\\";
static const char* cutsSubFemale     = "text\\%s\\cuts_female\\%s";
static const char* cutsDeathFemale   = "text\\%s\\cuts_female\\%s%s";
static const char* msgFemaleFolder   = "dialog_female\\%s.msg";

static bool isFemale    = false;
static bool femaleCheck = false;  // flag for check female dialog file
static DWORD format;
static bool cutsPatch   = false;

static char sfallRes[14] = "sfall.dat"; // for sfall_XX.dat filename
static std::vector<int> savPrototypes;

void PlayerGenderCutsRestore() {
	if (cutsPatch) { // restore
		SafeWrite32(0x43FA9F, FO_VAR_aTextSCuts);
		SafeWrite32(0x44EB5B, FO_VAR_aTextSCutsS);
		SafeWrite32(0x48152E, FO_VAR_aTextSCutsSS);
		cutsPatch = false;
	}
}

void CheckPlayerGender() {
	isFemale = fo::util::HeroIsFemale();

	if (femaleMsgs > 1) {
		if (isFemale) {
			if (cutsPatch) return;
			cutsPatch = true;
			SafeWrite32(0x43FA9F, (DWORD)cutsEndGameFemale);
			SafeWrite32(0x44EB5B, (DWORD)cutsSubFemale);
			SafeWrite32(0x48152E, (DWORD)cutsDeathFemale);
		} else {
			PlayerGenderCutsRestore();
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

////////////////////////////////////////////////////////////////////////////////

// compares paths without the trailing '\' character
static bool PatchesCompare(const char* p1, const char* p2) {
	size_t len1 = std::strlen(p1);
	size_t len2 = std::strlen(p2);

	if (len1 == 0 || len2 == 0) return false;

	// R-Trim
	while (p1[len1 - 1] == '\\') len1--;
	while (p2[len2 - 1] == '\\') len2--;

	if (len1 != len2) return false;

	size_t n = 0;
	while (true) {
		unsigned char c1 = p1[n];
		unsigned char c2 = p2[n];

		// upper to lower case for standard ASCII
		if (c1 >= 'A' && c1 <= 'Z') c1 |= 32;
		if (c2 >= 'A' && c2 <= 'Z') c2 |= 32;

		if (c1 != c2) return false;

		n++;
		if (n > len1 || n > len2) return false;
		if (n == len1 && n == len2) break; // paths are matched
	}
	return true;
}

static fo::PathNode* __fastcall RemoveDatabase(const char* pathPatches) {
	fo::PathNode* paths = *fo::ptr::paths; // curr.node (beginning of the chain of paths)
	fo::PathNode* _paths = paths;          // prev.node

	while (paths) {
		if (_stricmp(paths->path, pathPatches) == 0) { // found path
			fo::PathNode* nextPaths = paths->next;     // pointer to the node of the next path
// TODO: need to check if this condition is used correctly (everything seems to be in order here)
			if (paths != _paths)
				_paths->next = nextPaths;              // replace the pointer in the previous node, removing the current(found) path from the chain
			else                                       // if the current node is equal to the previous node
				*fo::ptr::paths = nextPaths;           // set the next node at the beginning of the chain
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
		mov  ds:[FO_VAR_master_db_handle], eax; // the pointer of master_patches node will be saved here
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
		dec  eax;                                 // 1 = remove path (critter_patches == master_patches)?
		jz   end;                                 // yes (jump if removed)
		mov  ecx, [esp + 0x104 + 4];              // path_patches
		call RemoveDatabase;
end:
		mov  ds:[FO_VAR_critter_db_handle], eax;  // the pointer of critter_patches node will be saved here
		xor  ebx, ebx;
		lea  eax, sfallRes;
		call fo::funcoffs::db_init_; // init sfall resource file
		retn;
	}
}

static void __fastcall game_init_databases_hook() { // eax = _master_db_handle
	fo::PathNode* master_patches = *fo::ptr::master_db_handle;
	fo::PathNode* critter_patches = *fo::ptr::critter_db_handle;
	fo::PathNode* paths = *fo::ptr::paths;    // beginning of the chain of paths
	// insert master_patches/critter_patches at the beginning of the chain of paths
	if (critter_patches) {
		critter_patches->next = paths;    // critter_patches.next -> paths
		paths = critter_patches;
	}
	master_patches->next = paths;         // master_patches.next -> paths
	*fo::ptr::paths = master_patches;     // set master_patches node at the beginning of the chain of paths

	// remove paths that are identical to master_patches (usually the "DATA" folder)
	fo::PathNode* parentPath = *fo::ptr::paths;
	paths = parentPath->next;
	while (paths) {
		if (!paths->isDat && PatchesCompare(paths->path, (*fo::ptr::paths)->path)) {
			fo::PathNode* nextPaths = paths->next;
			__asm {
				mov  eax, [paths];
				call fo::funcoffs::nfree_; // free path string
				mov  eax, paths;
				call fo::funcoffs::nfree_; // free self
			}
			parentPath->next = nextPaths;
			paths = nextPaths;
		} else {
			parentPath = paths;
			paths = paths->next;
		}
	}
}
/*
static void __fastcall game_init_databases_hook1() {
	char masterPatch[MAX_PATH];
	IniReader::GetString("system", "master_patches", "", masterPatch, MAX_PATH - 1, (const char*)FO_VAR_gconfig_file_name);

	fo::PathNode* node = *fo::ptr::paths;
	while (node->next) {
		if (!strcmp(node->path, masterPatch)) break;
		node = node->next;
	}
	*fo::ptr::master_db_handle = node; // set pointer to master_patches node
}
*/
static void MultiPatchesPatch() {
	//if (IniReader::GetConfigInt("Misc", "MultiPatches", 0)) {
		dlog("Applying load multiple patches patch.", DL_INIT);
		SafeWrite8(0x444354, CodeType::Nop); // Change step from 2 to 1
		SafeWrite8(0x44435C, 0xC4);          // Disable check
		dlogr(" Done", DL_INIT);
	//}
}

///////////////////////// SAVE PARTY MEMBER PROTOTYPES /////////////////////////

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
		retn 8;
	}
}

#define _F_SAV            (const char*)0x50A480
#define _F_PROTO_CRITTERS (const char*)0x50A490

static void RemoveSavFiles() {
	fo::func::MapDirErase(_F_PROTO_CRITTERS, _F_SAV);
}

////////////////////////////////////////////////////////////////////////////////

static DWORD aliasFID = -1;

static void __declspec(naked) art_get_name_hook() {
	__asm {
		call fo::funcoffs::art_alias_fid_;
		cmp  eax, -1;
		jne  artAlias;
		retn; // if aliasFID here is not equal to -1, then the algorithm is not working correctly
artAlias:
		cmp  eax, edx;
		je   skip;
		mov  aliasFID, eax;
skip:
		mov  eax, -1;
		retn;
	}
}

void __declspec(naked) LoadOrder::art_get_name_hack() {
	static const DWORD art_get_name_Alias = 0x41944A;
	__asm {
		mov  eax, FO_VAR_art_name;
		cmp  aliasFID, -1;
		jne  artHasAlias;
		retn;
artHasAlias:
		call fo::funcoffs::db_access_;
		test eax, eax;
		jz   artNotExist;
		mov  aliasFID, -1;
		mov  eax, FO_VAR_art_name;
		retn;
artNotExist:
		dec  eax; // -1
		xchg eax, aliasFID
		add  esp, 4;
		jmp  art_get_name_Alias; // get name of art alias
	}
}

static fo::DbFile* __fastcall LoadFont(const char* font, const char* mode) {
	char file[128];
	const char* lang;
	if (fo::func::get_game_config_string(&lang, "system", "language") && _stricmp(lang, "english") != 0) {
		std::sprintf(file, "fonts\\%s\\%s", lang, font);
		return fo::func::db_fopen(file, mode);
	}
	return nullptr;
}

static void __declspec(naked) load_font_hook() {
	__asm {
		mov  ebp, edx;
		mov  ebx, eax;
		mov  ecx, eax;
		call LoadFont;
		test eax, eax;
		jz   default;
		retn;
default:
		mov  edx, ebp;
		mov  eax, ebx;
		jmp  fo::funcoffs::db_fopen_;
	}
}

static void SfallResourceFile() {
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFileA("sfall_??.dat", &findData); // example: sfall_ru.dat, sfall_zh.dat
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (std::strlen(&findData.cFileName[6]) == 6) {
				dlog_f("Found a localized sfall resource file: %s\n", DL_MAIN, findData.cFileName);
				std::strcpy(sfallRes, findData.cFileName);
				break;
			}
		} while (FindNextFileA(hFind, &findData));
		FindClose(hFind);
	}
}

void LoadOrder::OnAfterGameInit() {
	RemoveSavFiles();
}

void LoadOrder::OnGameLoad() {
	savPrototypes.clear();
	RemoveSavFiles();
}

void LoadOrder::init() {
	SfallResourceFile(); // Add external sfall resource file (load order: > patchXXX.dat > sfall.dat > ... [last])
	MultiPatchesPatch();

	//if (IniReader::GetConfigInt("Misc", "DataLoadOrderPatch", 1)) {
		dlog("Applying data load order patch.", DL_INIT);
		MakeCall(0x444259, game_init_databases_hack1);
		MakeCall(0x4442F1, game_init_databases_hack2);
		HookCall(0x44436D, game_init_databases_hook);
		SafeWrite8(0x4DFAEC, 0x1D); // error correction (ecx > ebx)
		dlogr(" Done", DL_INIT);
	//} else {
	//	HookCall(0x44436D, game_init_databases_hook1);
	//}

	femaleMsgs = IniReader::GetConfigInt("Misc", "FemaleDialogMsgs", 0);
	if (femaleMsgs) {
		dlog("Applying alternative female dialog files patch.", DL_INIT);
		MakeJump(0x4A6BCD, scr_get_dialog_msg_file_hack1);
		MakeJump(0x4A6BF5, scr_get_dialog_msg_file_hack2);
		if (femaleMsgs > 1) {
			MakeCall(0x480A95, gnw_main_hack); // before new game start from main menu. TODO: need moved to address 0x480A9A (it busy in movies.cpp)
		}
		dlogr(" Done", DL_INIT);
	}

	// Redefined behavior for replacing art aliases for critters
	// first check the existence of the art file of the current critter and then replace the art alias if file not found
	HookCall(0x419440, art_get_name_hook);
	SafeWrite16(0x419521, 0x003B); // jmp 0x419560
	if (IniReader::GetConfigInt("Misc", "EnableHeroAppearanceMod", 0) <= 0) { // Hero Appearance mod uses an alternative code
		MakeCall(0x419560, art_get_name_hack);
	}

	dlog("Applying party member protos save/load patch.", DL_INIT);
	savPrototypes.reserve(25);
	HookCall(0x4A1CF2, proto_load_pid_hook);
	HookCall(0x4A1BEE, proto_save_pid_hook);
	MakeCall(0x47F5A5, GameMap2Slot_hack); // save game
	MakeCall(0x47FB80, SlotMap2Game_hack); // load game
	MakeCall(0x47FBBF, SlotMap2Game_hack_attr, 1);
	dlogr(" Done", DL_INIT);

	// Load fonts based on the game language
	const DWORD loadFontAddr[] = {
		0x4D5621, // load_font_
		0x441D58  // FMLoadFont_
	};
	HookCalls(load_font_hook, loadFontAddr);
}

}
