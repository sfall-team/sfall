/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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

#include <math.h>
#include <stdio.h>

#include "AI.h"
#include "AnimationsAtOnceLimit.h"
#include "BarBoxes.h"
#include "Books.h"
#include "BugFixes.h"
#include "BurstMods.h"
#include "console.h"
#include "CRC.h"
#include "Credits.h"
#include "Criticals.h"
#include "DamageMod.h"
#include "Define.h"
#include "DebugEditor.h"
#include "Elevators.h"
#include "Explosions.h"
#include "FalloutEngine.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "Inventory.h"
#include "KillCounter.h"
#include "knockback.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "MainMenu.h"
#include "Message.h"
#include "movies.h"
#include "PartyControl.h"
#include "perks.h"
#include "Premade.h"
#include "QuestList.h"
#include "Reputations.h"
#include "ScriptExtender.h"
#include "skills.h"
#include "sound.h"
#include "SpeedPatch.h"
#include "stats.h"
#include "SuperSave.h"
#include "TalkingHeads.h"
#include "Tiles.h"
#include "version.h"
#include "Worldmap.h"

ddrawDll ddraw;

bool isDebug = false;

char ini[65] = ".\\";
char translationIni[65];
DWORD modifiedIni;

static std::vector<int> savPrototypes;

static char mapName[65];
static char configName[65];
static char patchName[65];
static char versionString[65];

static char smModelName[65];
char dmModelName[65];
static char sfModelName[65];
char dfModelName[65];

static const char* musicOverridePath = "data\\sound\\music\\";

static int* scriptDialog = nullptr;

static const DWORD FastShotFixF1[] = {
	0x478BB8, 0x478BC7, 0x478BD6, 0x478BEA, 0x478BF9, 0x478C08, 0x478C2F,
};

static const DWORD script_dialog_msgs[] = {
	0x4A50C2, 0x4A5169, 0x4A52FA, 0x4A5302, 0x4A6B86, 0x4A6BE0, 0x4A6C37,
};

static const DWORD EncounterTableSize[] = {
	0x4BD1A3, 0x4BD1D9, 0x4BD270, 0x4BD604, 0x4BDA14, 0x4BDA44, 0x4BE707,
	0x4C0815, 0x4C0D4A, 0x4C0FD4,
};

static const DWORD PutAwayWeapon[] = {
	0x411EA2, // action_climb_ladder_
	0x412046, // action_use_an_item_on_object_
	0x41224A, // action_get_an_object_
	0x4606A5, // intface_change_fid_animate_
	0x472996, // invenWieldFunc_
};

static const DWORD WalkDistanceAddr[] = {
	0x411FF0, 0x4121C4, 0x412475, 0x412906,
};

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

// Remove master_patches from the load order
static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		call RemoveDatabase;
		mov  ds:[_master_db_handle], eax;         // the pointer of master_patches node will be saved here
		retn;
	}
}

// Remove critter_patches from the load order
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

////////////////////////////////////////////////////////////////////////////////

static void __declspec(naked) apply_damage_hack() {
	__asm {
		xor  edx, edx;
		inc  edx;              // COMBAT_SUBTYPE_WEAPON_USED
		test [esi + 0x15], dl; // ctd.flags2Source & DAM_HIT_
		jz   end;              // no hit
		inc  edx;              // COMBAT_SUBTYPE_HIT_SUCCEEDED
end:
		retn;
	}
}

static void __declspec(naked) WeaponAnimHook() {
	__asm {
		cmp edx, 11;
		je  c11;
		cmp edx, 15;
		je  c15;
		jmp art_get_code_;
c11:
		mov edx, 16;
		jmp art_get_code_;
c15:
		mov edx, 17;
		jmp art_get_code_;
	}
}

static char KarmaGainMsg[128];
static char KarmaLossMsg[128];
static void _stdcall SetKarma(int value) {
	int old;
	__asm {
		xor eax, eax;
		call game_get_global_var_;
		mov old, eax;
	}
	old = value - old;
	char buf[64];
	if (old == 0) return;
	if (old > 0) {
		sprintf_s(buf, KarmaGainMsg, old);
	} else {
		sprintf_s(buf, KarmaLossMsg, -old);
	}
	DisplayConsoleMessage(buf);
}

static void __declspec(naked) SetGlobalVarWrapper() {
	__asm {
		test eax, eax;
		jnz end;
		pushad;
		push edx;
		call SetKarma;
		popad;
end:
		jmp game_set_global_var_;
	}
}

static void __declspec(naked) ReloadHook() {
	__asm {
		push eax;
		push ebx;
		push edx;
		mov eax, dword ptr ds:[_obj_dude];
		call register_clear_;
		xor eax, eax;
		inc eax;
		call register_begin_;
		xor edx, edx;
		xor ebx, ebx;
		mov eax, dword ptr ds:[_obj_dude];
		dec ebx;
		call register_object_animate_;
		call register_end_;
		pop edx;
		pop ebx;
		pop eax;
		jmp gsound_play_sfx_file_;
	}
}

static const DWORD CorpseHitFix2_continue_loop1 = 0x48B99B;
static void __declspec(naked) CorpseHitFix2() {
	__asm {
		push eax;
		mov eax, [eax];
		call critter_is_dead_; // found some object, check if it's a dead critter
		test eax, eax;
		pop eax;
		jz really_end; // if not, allow breaking the loop (will return this object)
		jmp CorpseHitFix2_continue_loop1; // otherwise continue searching

really_end:
		mov     eax, [eax];
		pop     ebp;
		pop     edi;
		pop     esi;
		pop     ecx;
		retn;
	}
}

static const DWORD CorpseHitFix2_continue_loop2 = 0x48BA0B;
// same logic as above, for different loop
static void __declspec(naked) CorpseHitFix2b() {
	__asm {
		mov eax, [edx];
		call critter_is_dead_;
		test eax, eax;
		jz really_end;
		jmp CorpseHitFix2_continue_loop2;

really_end:
		mov     eax, [edx];
		pop     ebp;
		pop     edi;
		pop     esi;
		pop     ecx;
		retn;
	}
}

static void __declspec(naked) intface_rotate_numbers_hack() {
	__asm {
		push edi
		push ebp
		sub  esp, 0x54
// ebx=old value, ecx=new value
		cmp  ebx, ecx
		je   end
		mov  ebx, ecx
		jg   decrease
		dec  ebx
		jmp  end
decrease:
		test ecx, ecx
		jl   negative
		inc  ebx
		jmp  end
negative:
		xor  ebx, ebx
end:
		push 0x460BA6
		retn
	}
}

static DWORD KarmaFrmCount;
static DWORD* KarmaFrms;
static int* KarmaPoints;
static DWORD _stdcall DrawCard() {
	int rep = **(int**)_game_global_vars;
	for (DWORD i = 0; i < KarmaFrmCount - 1; i++) {
		if (rep < KarmaPoints[i]) return KarmaFrms[i];
	}
	return KarmaFrms[KarmaFrmCount - 1];
}

static void __declspec(naked) DrawInfoWin_hook() {
	__asm {
		cmp  ds:[0x5707D0], 10;
		jne  skip;
		cmp  eax, 0x30;
		jne  skip;
		push ecx;
		push edx;
		call DrawCard;
		pop  edx;
		pop  ecx;
skip:
		jmp  DrawCard_;
	}
}

static void __declspec(naked) ScienceCritterCheckHook() {
	__asm {
		cmp esi, ds:[_obj_dude];
		jne end;
		mov eax, 10;
		retn;
end:
		jmp critter_kill_count_type_;
	}
}

static const DWORD NPCStage6Fix1End = 0x493D16;
static void __declspec(naked) NPCStage6Fix1() {
	__asm {
		mov  eax, 204;                  // set record size to 204 bytes
		imul eax, edx;                  // multiply by number of NPC records in party.txt
		mov  ebx, eax;                  // copy total record size for later memset
		call mem_malloc_;               // malloc the necessary memory
		jmp  NPCStage6Fix1End;          // call memset to set all malloc'ed memory to 0
	}
}

static const DWORD NPCStage6Fix2End = 0x49423A;
static void __declspec(naked) NPCStage6Fix2() {
	__asm {
		mov  eax, 204;                  // record size is 204 bytes
		imul edx, eax;                  // multiply by NPC number as listed in party.txt
		mov  eax, dword ptr ds:[_partyMemberAIOptions]; // get starting offset of internal NPC table
		jmp  NPCStage6Fix2End;          // eax+edx = offset of specific NPC record
	}
}

static const DWORD FastShotTraitFixEnd1 = 0x478E7F;
static const DWORD FastShotTraitFixEnd2 = 0x478E7B;
static void __declspec(naked) FastShotTraitFix() {
	__asm {
		test eax, eax;				// does player have Fast Shot trait?
		je ajmp;				// skip ahead if no
		mov edx, ecx;				// argument for item_w_range_: hit_mode
		mov eax, ebx;				// argument for item_w_range_: pointer to source_obj (always dude_obj due to code path)
		call item_w_range_;			// get weapon's range
		cmp eax, 0x2;				// is weapon range less than or equal 2 (i.e. melee/unarmed attack)?
		jle ajmp;				// skip ahead if yes
		xor eax, eax;				// otherwise, disallow called shot attempt
		jmp bjmp;
ajmp:
		jmp FastShotTraitFixEnd1;		// continue processing called shot attempt
bjmp:
		jmp FastShotTraitFixEnd2;		// clean up and exit function item_w_called_shot
	}
}

static const DWORD ScannerHookRet = 0x41BC1D;
static const DWORD ScannerHookFail = 0x41BC65;
static void __declspec(naked) ScannerAutomapHook() {
	__asm {
		mov eax, ds:[_obj_dude];
		mov edx, PID_MOTION_SENSOR;
		call inven_pid_is_carried_ptr_;
		test eax, eax;
		jz fail;
		mov edx, eax;
		jmp ScannerHookRet;
fail:
		jmp ScannerHookFail;
	}
}

static void __declspec(naked) op_obj_can_see_obj_hook() {
	__asm {
		push obj_shoot_blocking_at_;      // check hex objects func pointer
		push 0x20;                        // flags, 0x20 = check ShootThru
		mov  ecx, dword ptr [esp + 0x0C]; // buf **ret_objStruct
		push ecx;
		xor  ecx, ecx;
		call make_straight_path_func_;    // (EAX *objStruct, EDX hexNum1, EBX hexNum2, ECX 0, stack1 **ret_objStruct, stack2 flags, stack3 *check_hex_objs_func)
		retn 8;
	}
}

static void __declspec(naked) register_object_take_out_hack() {
	__asm {
		push ecx
		push eax
		mov  ecx, edx                             // ID1
		mov  edx, [eax+0x1C]                      // cur_rot
		inc  edx
		push edx                                  // ID3
		xor  ebx, ebx                             // ID2
		mov  edx, [eax+0x20]                      // fid
		and  edx, 0xFFF                           // Index
		xor  eax, eax
		inc  eax                                  // Obj_Type
		call art_id_
		xor  ebx, ebx
		dec  ebx
		xchg edx, eax
		pop  eax
		call register_object_change_fid_
		pop  ecx
		xor  eax, eax
		retn
	}
}

static void __declspec(naked) gdAddOptionStr_hack() {
	__asm {
		mov  ecx, ds:[_gdNumOptions]
		add  ecx, '1'
		push ecx
		push 0x4458FA
		retn
	}
}

static DWORD __fastcall GetWeaponSlotMode(DWORD itemPtr, DWORD mode) {
	int slot = (mode > 0) ? 1 : 0;
	DWORD* itemButton = ptr_itemButtonItems;
	if (itemButton[slot * 6] == itemPtr) {
		int slotMode = itemButton[(slot * 6) + 4];
		if (slotMode == 3 || slotMode == 4) {
			mode++;
		}
	}
	return mode;
}

static void __declspec(naked) display_stats_hook() {
	__asm {
		push eax;
		push ecx;
		mov ecx, ds:[esp + edi + 0xA8 + 0xC];   // get itemPtr
		call GetWeaponSlotMode;                 // ecx - itemPtr, edx - mode;
		mov edx, eax;
		pop ecx;
		pop eax;
		jmp item_w_range_;
	}
}

static void DllMain2() {
	DWORD tmp;
	dlogr("In DllMain2", DL_MAIN);

	dlogr("Running BugFixesInit().", DL_INIT);
	BugFixesInit();

	dlogr("Running SpeedPatchInit().", DL_INIT);
	SpeedPatchInit();

	dlogr("Running GraphicsInit().", DL_INIT);
	GraphicsInit();

	dlogr("Running TalkingHeadsInit().", DL_INIT);
	TalkingHeadsInit();

	//if (GetPrivateProfileIntA("Input", "Enable", 0, ini)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(0x50FB70, "ddraw.dll");
		AvailableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}

	if (GetPrivateProfileIntA("Misc", "DataLoadOrderPatch", 1, ini)) {
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

	dlogr("Running DamageModInit().", DL_INIT);
	DamageModInit();

	dlogr("Running MoviesInit().", DL_INIT);
	MoviesInit();

	dlog("Applying main menu patches.", DL_INIT);
	MainMenuInit();
	dlogr(" Done", DL_INIT);

	mapName[64] = 0;
	if (GetPrivateProfileString("Misc", "StartingMap", "", mapName, 64, ini)) {
		dlog("Applying starting map patch.", DL_INIT);
		SafeWrite32(0x480AAA, (DWORD)&mapName);
		dlogr(" Done", DL_INIT);
	}

	versionString[64] = 0;
	if (GetPrivateProfileString("Misc", "VersionString", "", versionString, 64, ini)) {
		dlog("Applying version string patch.", DL_INIT);
		SafeWrite32(0x4B4588, (DWORD)&versionString);
		dlogr(" Done", DL_INIT);
	}

	configName[64] = 0;
	if (GetPrivateProfileString("Misc", "ConfigFile", "", configName, 64, ini)) {
		dlog("Applying config file patch.", DL_INIT);
		SafeWrite32(0x444BA5, (DWORD)&configName);
		SafeWrite32(0x444BCA, (DWORD)&configName);
		dlogr(" Done", DL_INIT);
	}

	patchName[64] = 0;
	if (GetPrivateProfileString("Misc", "PatchFile", "", patchName, 64, ini)) {
		dlog("Applying patch file patch.", DL_INIT);
		SafeWrite32(0x444323, (DWORD)&patchName);
		dlogr(" Done", DL_INIT);
	}

	smModelName[64] = 0;
	if (GetPrivateProfileString("Misc", "MaleStartModel", "", smModelName, 64, ini)) {
		dlog("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x418B88, (DWORD)&smModelName);
		dlogr(" Done", DL_INIT);
	}

	sfModelName[64] = 0;
	if (GetPrivateProfileString("Misc", "FemaleStartModel", "", sfModelName, 64, ini)) {
		dlog("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x418BAB, (DWORD)&sfModelName);
		dlogr(" Done", DL_INIT);
	}

	dmModelName[64] = 0;
	GetPrivateProfileString("Misc", "MaleDefaultModel", "hmjmps", dmModelName, 64, ini);
	dlog("Applying male model patch.", DL_INIT);
	SafeWrite32(0x418B50, (DWORD)&dmModelName);
	dlogr(" Done", DL_INIT);

	dfModelName[64] = 0;
	GetPrivateProfileString("Misc", "FemaleDefaultModel", "hfjmps", dfModelName, 64, ini);
	dlog("Applying female model patch.", DL_INIT);
	SafeWrite32(0x418B6D, (DWORD)&dfModelName);
	dlogr(" Done", DL_INIT);

	dlogr("Running WorldmapInit().", DL_INIT);
	WorldmapInit();

	if (GetPrivateProfileIntA("Misc", "DialogueFix", 1, ini)) {
		dlog("Applying dialogue patch.", DL_INIT);
		SafeWrite8(0x446848, 0x31);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running CritInit().", DL_INIT);
	CritInit();

	if (GetPrivateProfileIntA("Misc", "ExtraKillTypes", 0, ini)) {
		dlog("Applying extra kill types patch.", DL_INIT);
		KillCounterInit();
		dlogr(" Done", DL_INIT);
	}

	//if (GetPrivateProfileIntA("Misc", "ScriptExtender", 0, ini)) {
		dlog("Applying script extender patch.", DL_INIT);
		StatsInit();
		dlog(".", DL_INIT);
		ScriptExtenderSetup();
		dlog(".", DL_INIT);
		LoadGameHookInit();
		dlog(".", DL_INIT);
		PerksInit();
		dlog(".", DL_INIT);
		KnockbackInit();
		dlog(".", DL_INIT);
		SkillsInit();
		dlog(".", DL_INIT);
		dlogr(" Done", DL_INIT);

		//Ray's combat_p_proc fix
		dlog("Applying Ray's combat_p_proc patch.", DL_INIT);
		MakeCall(0x424DD9, apply_damage_hack);
		SafeWrite8(0x424DC7, 0x0);
		dlogr(" Done", DL_INIT);
	//}

	dlogr("Running FileSystemInit().", DL_INIT);
	FileSystemInit();

	dlogr("Running DebugEditorInit().", DL_INIT);
	DebugEditorInit();

	if (GetPrivateProfileIntA("Misc", "SingleCore", 1, ini)) {
		dlog("Applying single core patch.", DL_INIT);
		HANDLE process = GetCurrentProcess();
		SetProcessAffinityMask(process, 1);
		CloseHandle(process);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "OverrideArtCacheSize", 0, ini)) {
		dlog("Applying override art cache size patch.", DL_INIT);
		SafeWrite8(0x41886A, 0x0);
		SafeWrite32(0x418872, 256);
		dlogr(" Done", DL_INIT);
	}

	char elevPath[MAX_PATH - 3];
	GetPrivateProfileString("Misc", "ElevatorsFile", "", elevPath, MAX_PATH - 3, ini);
	if (strlen(elevPath) > 0) {
		dlog("Applying elevator patch.", DL_INIT);
		ElevatorsInit(elevPath);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "AdditionalWeaponAnims", 0, ini)) {
		dlog("Applying additional weapon animations patch.", DL_INIT);
		SafeWrite8(0x419320, 0x12);
		HookCall(0x4194CC, WeaponAnimHook);
		HookCall(0x451648, WeaponAnimHook);
		HookCall(0x451671, WeaponAnimHook);
		dlogr(" Done", DL_INIT);
	}

	//if (GetPrivateProfileIntA("Misc", "MultiPatches", 0, ini)) {
		dlog("Applying load multiple patches patch.", DL_INIT);
		SafeWrite8(0x444354, 0x90); // Change step from 2 to 1
		SafeWrite8(0x44435C, 0xC4); // Disable check
		dlogr(" Done", DL_INIT);
	//}

	if (GetPrivateProfileInt("Misc", "DisplayKarmaChanges", 0, ini)) {
		dlog("Applying display karma changes patch.", DL_INIT);
		GetPrivateProfileString("sfall", "KarmaGain", "You gained %d karma.", KarmaGainMsg, 128, translationIni);
		GetPrivateProfileString("sfall", "KarmaLoss", "You lost %d karma.", KarmaLossMsg, 128, translationIni);
		HookCall(0x455A6D, SetGlobalVarWrapper);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "AlwaysReloadMsgs", 0, ini)) {
		dlog("Applying always reload messages patch.", DL_INIT);
		SafeWrite8(0x4A6B8D, 0x0);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "PlayIdleAnimOnReload", 0, ini)) {
		dlog("Applying idle anim on reload patch.", DL_INIT);
		HookCall(0x460B8C, ReloadHook);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "CorpseLineOfFireFix", 0, ini)) {
		dlog("Applying corpse line of fire patch.", DL_INIT);
		MakeJump(0x48B994, CorpseHitFix2);
		MakeJump(0x48BA04, CorpseHitFix2b);
		dlogr(" Done", DL_INIT);
	}

	dlog("Checking for changed skilldex images.", DL_INIT);
	tmp = GetPrivateProfileIntA("Misc", "Lockpick", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D54, tmp);
	tmp = GetPrivateProfileIntA("Misc", "Steal", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D58, tmp);
	tmp = GetPrivateProfileIntA("Misc", "Traps", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D5C, tmp);
	tmp = GetPrivateProfileIntA("Misc", "FirstAid", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D4C, tmp);
	tmp = GetPrivateProfileIntA("Misc", "Doctor", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D50, tmp);
	tmp = GetPrivateProfileIntA("Misc", "Science", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D60, tmp);
	tmp = GetPrivateProfileIntA("Misc", "Repair", 293, ini);
	if (tmp != 293) SafeWrite32(0x518D64, tmp);
	dlogr(" Done", DL_INIT);

	dlogr("Running HeroAppearanceModInit().", DL_INIT);
	HeroAppearanceModInit();

	dlogr("Running TilesInit().", DL_INIT);
	TilesInit();

	dlogr("Running CreditsInit().", DL_INIT);
	CreditsInit();

	dlogr("Running QuestListInit().", DL_INIT);
	QuestListInit();

	dlog("Applying premade characters patch.", DL_INIT);
	PremadeInit();
	dlogr(" Done", DL_INIT);

	dlogr("Running SoundInit().", DL_INIT);
	SoundInit();

	dlogr("Running ReputationsInit().", DL_INIT);
	ReputationsInit();

	dlogr("Running ConsoleInit().", DL_INIT);
	ConsoleInit();

	if (GetPrivateProfileIntA("Misc", "ExtraSaveSlots", 0, ini)) {
		dlogr("Running EnableSuperSaving().", DL_INIT);
		EnableSuperSaving();
	}

	switch (GetPrivateProfileIntA("Misc", "SpeedInterfaceCounterAnims", 0, ini)) {
	case 1:
		dlog("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
		MakeJump(0x460BA1, intface_rotate_numbers_hack);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying SpeedInterfaceCounterAnims patch. (Instant)", DL_INIT);
		SafeWrite32(0x460BB6, 0x90DB3190); // xor ebx, ebx
		dlogr(" Done", DL_INIT);
		break;
	}

	KarmaFrmCount = GetPrivateProfileIntA("Misc", "KarmaFRMsCount", 0, ini);
	if (KarmaFrmCount) {
		KarmaFrms = new DWORD[KarmaFrmCount];
		KarmaPoints = new int[KarmaFrmCount - 1];
		dlog("Applying karma FRM patch.", DL_INIT);
		char buf[512];
		GetPrivateProfileStringA("Misc", "KarmaFRMs", "", buf, 512, ini);
		char *ptr = buf, *ptr2;
		for (DWORD i = 0; i < KarmaFrmCount - 1; i++) {
			ptr2 = strchr(ptr, ',');
			*ptr2 = '\0';
			KarmaFrms[i] = atoi(ptr);
			ptr = ptr2 + 1;
		}
		KarmaFrms[KarmaFrmCount - 1] = atoi(ptr);
		GetPrivateProfileStringA("Misc", "KarmaPoints", "", buf, 512, ini);
		ptr = buf;
		for (DWORD i = 0; i < KarmaFrmCount - 2; i++) {
			ptr2 = strchr(ptr, ',');
			*ptr2 = '\0';
			KarmaPoints[i] = atoi(ptr);
			ptr = ptr2 + 1;
		}
		KarmaPoints[KarmaFrmCount - 2] = atoi(ptr);
		HookCall(0x4367A9, DrawInfoWin_hook);
		dlogr(" Done", DL_INIT);
	}

	switch (GetPrivateProfileIntA("Misc", "ScienceOnCritters", 0, ini)) {
	case 1:
		HookCall(0x41276E, ScienceCritterCheckHook);
		break;
	case 2:
		SafeWrite8(0x41276A, 0xEB);
		break;
	}

	tmp = GetPrivateProfileIntA("Misc", "SpeedInventoryPCRotation", 166, ini);
	if (tmp != 166 && tmp <= 1000) {
		dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
		SafeWrite32(0x47066B, tmp);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running BarBoxesInit().", DL_INIT);
	BarBoxesInit();

	dlogr("Patching out ereg call.", DL_INIT);
	BlockCall(0x4425E6);

	dlogr("Running AnimationsAtOnceInit().", DL_INIT);
	AnimationsAtOnceInit();

	if (tmp = GetPrivateProfileIntA("Sound", "OverrideMusicDir", 0, ini)) {
		SafeWrite32(0x4449C2, (DWORD)musicOverridePath);
		SafeWrite32(0x4449DB, (DWORD)musicOverridePath);
		if (tmp == 2) {
			SafeWrite32(0x518E78, (DWORD)musicOverridePath);
			SafeWrite32(0x518E7C, (DWORD)musicOverridePath);
		}
	}

	if (GetPrivateProfileIntA("Misc", "NPCStage6Fix", 0, ini)) {
		dlog("Applying NPC Stage 6 Fix.", DL_INIT);
		MakeJump(0x493CE9, NPCStage6Fix1);
		SafeWrite8(0x494063, 6);   // loop should look for a potential 6th stage
		SafeWrite8(0x4940BB, 204); // move pointer by 204 bytes instead of 200
		MakeJump(0x494224, NPCStage6Fix2);
		dlogr(" Done", DL_INIT);
	}

	switch (GetPrivateProfileIntA("Misc", "FastShotFix", 1, ini)) {
	case 1:
		dlog("Applying Fast Shot Trait Fix.", DL_INIT);
		MakeJump(0x478E75, FastShotTraitFix);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying Fast Shot Trait Fix. (Fallout 1 version)", DL_INIT);
		SafeWrite8(0x478CA0, 0x0);
		for (int i = 0; i < sizeof(FastShotFixF1) / 4; i++) {
			HookCall(FastShotFixF1[i], (void*)0x478C7D);
		}
		dlogr(" Done", DL_INIT);
		break;
	}

	if (GetPrivateProfileIntA("Misc", "BoostScriptDialogLimit", 0, ini)) {
		const int scriptDialogCount = 10000;
		dlog("Applying script dialog limit patch.", DL_INIT);
		scriptDialog = new int[scriptDialogCount * 2]; // Because the msg structure is 8 bytes, not 4.
		SafeWrite32(0x4A50E3, scriptDialogCount); // scr_init
		SafeWrite32(0x4A519F, scriptDialogCount); // scr_game_init
		SafeWrite32(0x4A534F, scriptDialogCount * 8); // scr_message_free
		for (int i = 0; i < sizeof(script_dialog_msgs) / 4; i++) {
			SafeWrite32(script_dialog_msgs[i], (DWORD)scriptDialog); // scr_get_dialog_msg_file
		}
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running InventoryInit().", DL_INIT);
	InventoryInit();

	if (tmp = GetPrivateProfileIntA("Misc", "MotionScannerFlags", 1, ini)) {
		dlog("Applying MotionScannerFlags patch.", DL_INIT);
		if (tmp & 1) MakeJump(0x41BBE9, ScannerAutomapHook);
		if (tmp & 2) {
			// automap_
			SafeWrite8(0x41BC25, 0x0);
			BlockCall(0x41BC3C);
			// item_m_use_charged_item_
			SafeWrite8(0x4794B3, 0x5E); // jbe short 0x479512
		}
		dlogr(" Done", DL_INIT);
	}

	tmp = GetPrivateProfileIntA("Misc", "EncounterTableSize", 0, ini);
	if (tmp > 40 && tmp <= 127) {
		dlog("Applying EncounterTableSize patch.", DL_INIT);
		SafeWrite8(0x4BDB17, (BYTE)tmp);
		DWORD nsize = (tmp + 1) * 180 + 0x50;
		for (int i = 0; i < sizeof(EncounterTableSize) / 4; i++) {
			SafeWrite32(EncounterTableSize[i], nsize);
		}
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "DisablePipboyAlarm", 0, ini)) {
		dlog("Applying Disable Pip-Boy alarm button patch.", DL_INIT);
		SafeWrite8(0x499518, 0xC3);
		SafeWrite8(0x443601, 0x0);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running AIInit().", DL_INIT);
	AIInit();

	dlogr("Initializing party control.", DL_INIT);
	PartyControlInit();

	if (GetPrivateProfileIntA("Misc", "ObjCanSeeObj_ShootThru_Fix", 0, ini)) {
		dlog("Applying ObjCanSeeObj ShootThru Fix.", DL_INIT);
		HookCall(0x456BC6, op_obj_can_see_obj_hook);
		dlogr(" Done", DL_INIT);
	}

	// phobos2077:
	dlogr("Running ComputeSprayModInit().", DL_INIT);
	ComputeSprayModInit();
	dlogr("Running ExplosionInit().", DL_INIT);
	ExplosionInit();
	dlogr("Running BooksInit().", DL_INIT);
	BooksInit();
	DWORD addrs[2] = {0x45F9DE, 0x45FB33};
	SimplePatch<WORD>(addrs, 2, "Misc", "CombatPanelAnimDelay", 1000, 0, 65535);
	addrs[0] = 0x447DF4; addrs[1] = 0x447EB6;
	SimplePatch<BYTE>(addrs, 2, "Misc", "DialogPanelAnimDelay", 33, 0, 255);
	addrs[0] = 0x499B99; addrs[1] = 0x499DA8;
	SimplePatch<BYTE>(addrs, 2, "Misc", "PipboyTimeAnimDelay", 50, 0, 127);

	if (GetPrivateProfileIntA("Misc", "EnableMusicInDialogue", 0, ini)) {
		dlog("Applying music in dialogue patch.", DL_INIT);
		SafeWrite8(0x44525B, 0x0);
		//BlockCall(0x450627);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "DontTurnOffSneakIfYouRun", 0, ini)) {
		dlog("Applying DontTurnOffSneakIfYouRun patch.", DL_INIT);
		SafeWrite8(0x418135, 0xEB);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "InstantWeaponEquip", 0, ini)) {
	//Skip weapon equip/unequip animations
		dlog("Applying instant weapon equip patch.", DL_INIT);
		for (int i = 0; i < sizeof(PutAwayWeapon) / 4; i++) {
			SafeWrite8(PutAwayWeapon[i], 0xEB);   // jmps
		}
		BlockCall(0x472AD5);                      //
		BlockCall(0x472AE0);                      // invenUnwieldFunc_
		BlockCall(0x472AF0);                      //
		MakeJump(0x415238, register_object_take_out_hack);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "NumbersInDialogue", 0, ini)) {
		dlog("Applying numbers in dialogue patch.", DL_INIT);
		SafeWrite32(0x502C32, 0x2000202E);
		SafeWrite8(0x446F3B, 0x35);
		SafeWrite32(0x5029E2, 0x7325202E);
		SafeWrite32(0x446F03, 0x2424448B);        // mov  eax, [esp+0x24]
		SafeWrite8(0x446F07, 0x50);               // push eax
		SafeWrite32(0x446FE0, 0x2824448B);        // mov  eax, [esp+0x28]
		SafeWrite8(0x446FE4, 0x50);               // push eax
		MakeJump(0x4458F5, gdAddOptionStr_hack);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "DisplaySecondWeaponRange", 1, ini)) {
		dlog("Applying display second weapon range patch.", DL_INIT);
		HookCall(0x472201, display_stats_hook);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "InterfaceDontMoveOnTop", 0, ini)) {
		dlog("Applying InterfaceDontMoveOnTop patch.", DL_INIT);
		SafeWrite8(0x46ECE9, 0x10); // set only Exclusive flag for Player Inventory/Loot/UseOn
		SafeWrite8(0x41B966, 0x10); // set only Exclusive flag for Automap
		dlogr(" Done", DL_INIT);
	}

	tmp = GetPrivateProfileIntA("Misc", "UseWalkDistance", 3, ini) + 2;
	if (tmp > 1 && tmp < 5) {
		dlog("Applying walk distance for using objects patch.", DL_INIT);
		for (int i = 0; i < sizeof(WalkDistanceAddr) / 4; i++) {
			SafeWrite8(WalkDistanceAddr[i], (BYTE)tmp); // default is 5
		}
		dlogr(" Done", DL_INIT);
	}

	int time = GetPrivateProfileIntA("Misc", "CorpseDeleteTime", 6, ini); // time in days
	if (time != 6) {
		dlog("Applying corpse deletion time patch.", DL_INIT);
		if (time <= 0) {
			time = 12; // hours
		} else if (time > 13) {
			time = 13 * 24;
		} else {
			time *= 24;
		}
		SafeWrite32(0x483348, time);
		dlogr(" Done", DL_INIT);
	}

	SimplePatch<DWORD>(0x440C2A, "Misc", "SpecialDeathGVAR", 491); // GVAR_MODOC_SHITTY_DEATH

	// Remove hardcoding for maps with IDs 19 and 37
	if (GetPrivateProfileIntA("Misc", "DisableSpecialMapIDs", 0, ini)) {
		dlog("Applying disable special map IDs patch.", DL_INIT);
		SafeWrite8(0x4836D6, 0);
		SafeWrite8(0x4836DB, 0);
		dlogr(" Done", DL_INIT);
	}

	SimplePatch<DWORD>(0x424FA7, "Misc", "KnockoutTime", 35, 35, 100);

	// Increase the max text width of the information card in the character screen
	SafeWrite8(0x43ACD5, 144); // 136
	SafeWrite8(0x43DD37, 144); // 133

	dlogr("Leave DllMain2", DL_MAIN);
}

static void _stdcall OnExit() {
	if (scriptDialog != nullptr) {
		delete[] scriptDialog;
	}
	SpeedPatchExit();
	ClearReadExtraGameMsgFiles();
	ReputationsExit();
	ConsoleExit();
	BooksExit();
	AnimationsAtOnceExit();
	HeroAppearanceModExit();
	MoviesExit();
	GraphicsExit();
	TalkingHeadsExit();
	//SoundExit();
}

static void __declspec(naked) OnExitFunc() {
	__asm {
		pushad;
		call OnExit;
		popad;
		jmp DOSCmdLineDestroy_;
	}
}

static void CompatModeCheck(HKEY root, const char* filepath, int extra) {
	HKEY key;
	char buf[MAX_PATH];
	DWORD size = MAX_PATH;
	DWORD type;
	if (!(type = RegOpenKeyEx(root, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, extra | STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &key))) {
		if (!RegQueryValueEx(key, filepath, 0, &type, (BYTE*)buf, &size)) {
			if (size && (type == REG_EXPAND_SZ || type == REG_MULTI_SZ || type == REG_SZ)) {
				if (strstr(buf, "256COLOR") || strstr(buf, "640X480") || strstr(buf, "WIN")) {
					RegCloseKey(key);

					MessageBoxA(0, "Fallout appears to be running in compatibility mode.\n" //, and sfall was not able to disable it.\n"
								"Please check the compatibility tab of fallout2.exe, and ensure that the following settings are unchecked.\n"
								"Run this program in compatibility mode for..., run in 256 colours, and run in 640x480 resolution.\n"
								"If these options are disabled, click the 'change settings for all users' button and see if that enables them.", "Error", 0);

					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	}
}

static bool LoadOriginalDll(DWORD dwReason) {
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			char path[MAX_PATH];
			CopyMemory(path + GetSystemDirectoryA(path , MAX_PATH - 10), "\\ddraw.dll", 11); // path to original dll
			ddraw.dll = LoadLibraryA(path);
			if (ddraw.dll) {
				ddraw.AcquireDDThreadLock          = GetProcAddress(ddraw.dll, "AcquireDDThreadLock");
				ddraw.CheckFullscreen              = GetProcAddress(ddraw.dll, "CheckFullscreen");
				ddraw.CompleteCreateSysmemSurface  = GetProcAddress(ddraw.dll, "CompleteCreateSysmemSurface");
				ddraw.D3DParseUnknownCommand       = GetProcAddress(ddraw.dll, "D3DParseUnknownCommand");
				ddraw.DDGetAttachedSurfaceLcl      = GetProcAddress(ddraw.dll, "DDGetAttachedSurfaceLcl");
				ddraw.DDInternalLock               = GetProcAddress(ddraw.dll, "DDInternalLock");
				ddraw.DDInternalUnlock             = GetProcAddress(ddraw.dll, "DDInternalUnlock");
				ddraw.DSoundHelp                   = GetProcAddress(ddraw.dll, "DSoundHelp");
				ddraw.DirectDrawCreateClipper      = GetProcAddress(ddraw.dll, "DirectDrawCreateClipper");
				ddraw.DirectDrawCreate             = GetProcAddress(ddraw.dll, "DirectDrawCreate");
				ddraw.DirectDrawCreateEx           = GetProcAddress(ddraw.dll, "DirectDrawCreateEx");
				ddraw.DirectDrawEnumerateA         = GetProcAddress(ddraw.dll, "DirectDrawEnumerateA");
				ddraw.DirectDrawEnumerateExA       = GetProcAddress(ddraw.dll, "DirectDrawEnumerateExA");
				ddraw.DirectDrawEnumerateExW       = GetProcAddress(ddraw.dll, "DirectDrawEnumerateExW");
				ddraw.DirectDrawEnumerateW         = GetProcAddress(ddraw.dll, "DirectDrawEnumerateW");
				//ddraw.DllCanUnloadNow            = GetProcAddress(ddraw.dll, "DllCanUnloadNow");
				//ddraw.DllGetClassObject          = GetProcAddress(ddraw.dll, "DllGetClassObject");
				ddraw.GetDDSurfaceLocal            = GetProcAddress(ddraw.dll, "GetDDSurfaceLocal");
				ddraw.GetOLEThunkData              = GetProcAddress(ddraw.dll, "GetOLEThunkData");
				ddraw.GetSurfaceFromDC             = GetProcAddress(ddraw.dll, "GetSurfaceFromDC");
				ddraw.RegisterSpecialCase          = GetProcAddress(ddraw.dll, "RegisterSpecialCase");
				ddraw.ReleaseDDThreadLock          = GetProcAddress(ddraw.dll, "ReleaseDDThreadLock");
			}
			return true;
		case DLL_PROCESS_DETACH:
			if (ddraw.dll) FreeLibrary(ddraw.dll);
			break;
	}
	return false;
}

bool _stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	if (LoadOriginalDll(dwReason)) {
		// enabling debugging features
		isDebug = (GetPrivateProfileIntA("Debugging", "Enable", 0, ".\\ddraw.ini") != 0);
		if (isDebug) {
			LoggingInit();
			if (!ddraw.dll) dlog("Error: Cannot load the original ddraw.dll library.\n", DL_MAIN);
		}

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

		if (!isDebug || !GetPrivateProfileIntA("Debugging", "SkipCompatModeCheck", 0, ".\\ddraw.ini")) {
			int is64bit;
			typedef int (_stdcall *chk64bitproc)(HANDLE, int*);
			HMODULE h = LoadLibrary("Kernel32.dll");
			chk64bitproc proc = (chk64bitproc)GetProcAddress(h, "IsWow64Process");
			if (proc)
				proc(GetCurrentProcess(), &is64bit);
			else
				is64bit = 0;
			FreeLibrary(h);

			CompatModeCheck(HKEY_CURRENT_USER, filepath, is64bit ? KEY_WOW64_64KEY : 0);
			CompatModeCheck(HKEY_LOCAL_MACHINE, filepath, is64bit ? KEY_WOW64_64KEY : 0);
		}

		// ini file override
		bool cmdlineexists = false;
		char* cmdline = GetCommandLineA();
		if (GetPrivateProfileIntA("Main", "UseCommandLine", 0, ".\\ddraw.ini")) {
			while (cmdline[0] == ' ') cmdline++;
			bool InQuote = false;
			int count = -1;

			while (true) {
				count++;
				if (cmdline[count] == 0) break;;
				if (cmdline[count] == ' ' && !InQuote) break;
				if (cmdline[count] == '"') {
					InQuote = !InQuote;
					if (!InQuote) break;
				}
			}
			if (cmdline[count] != 0) {
				count++;
				while (cmdline[count] == ' ') count++;
				cmdline = &cmdline[count];
				cmdlineexists = true;
			}
		}

		if (cmdlineexists && *cmdline != 0) {
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (h != INVALID_HANDLE_VALUE) {
				CloseHandle(h);
				strcat_s(ini, cmdline);
			} else {
				MessageBox(0, "You gave a command line argument to fallout, but it couldn't be matched to a file\n" \
							  "Using default ddraw.ini instead", "Warning", MB_TASKMODAL);
				goto defaultIni;
			}
		} else {
defaultIni:
			strcpy_s(ini, ".\\ddraw.ini");
		}

		GetPrivateProfileStringA("Main", "TranslationsINI", ".\\Translations.ini", translationIni, 65, ini);
		modifiedIni = GetPrivateProfileIntA("Main", "ModifiedIni", 0, ini);

		DllMain2();
	}
	return true;
}
