/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2011, 2012  The sfall team
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
#include "AI.h"
#include "Arrays.h"
#include "BugFixes.h"
#include "BarBoxes.h"
#include "Combat.h"
#include "Console.h"
#include "Criticals.h"
#include "Explosions.h"
#include "ExtraSaveSlots.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "InputFuncs.h"
#include "Interface.h"
#include "Inventory.h"
#include "LoadGameHook.h"
#include "LoadOrder.h"
#include "Logging.h"
#include "Message.h"
#include "Movies.h"
#include "Objects.h"
#include "PartyControl.h"
#include "Perks.h"
#include "ScriptExtender.h"
#include "Skills.h"
#include "Sound.h"
#include "Stats.h"
#include "TalkingHeads.h"
#include "version.h"
#include "Worldmap.h"

#define MAX_GLOBAL_SIZE (MaxGlobalVars * 12 + 4)

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
static bool disableHorrigan = false;
static bool pipBoyAvailableAtGameStart = false;
static bool gameLoaded = false;

// True if game was started, false when on the main menu
bool IsGameLoaded() {
	return gameLoaded;
}

DWORD InWorldMap() {
	return (inLoop & WORLDMAP) ? 1 : 0;
}

DWORD InCombat() {
	return (inLoop & COMBAT) ? 1 : 0;
}

DWORD InDialog() {
	return (inLoop & DIALOG) ? 1 : 0;
}

DWORD GetLoopFlags() {
	return inLoop;
}

void SetLoopFlag(LoopFlag flag) {
	inLoop |= flag;
}

void ClearLoopFlag(LoopFlag flag) {
	inLoop &= ~flag;
}

static void __stdcall ResetState(DWORD onLoad) { // OnGameReset & OnBeforeGameStart
	if (GraphicsMode > 3) Graphics_OnGameLoad();
	ForceGraphicsRefresh(0); // disable refresh
	LoadOrder_OnGameLoad();
	Interface_OnGameLoad();
	RestoreObjUnjamAllLocks();
	Worldmap_OnGameLoad();
	Stats_OnGameLoad();
	PerksReset();
	Combat_OnGameLoad();
	Skills_OnGameLoad();
	FileSystemReset();
	WipeSounds();
	InventoryReset();
	PartyControl_OnGameLoad();
	ResetExplosionRadius();
	BarBoxes_OnGameLoad();
	ScriptExtender_OnGameLoad();
	inLoop = 0;
	if (isDebug) {
		char* str = (onLoad) ? "on Load" : "on Exit";
		DebugPrintf("\n[SFALL: State reset %s]", str);
	}
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", *ptr_patches, *ptr_slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static char saveSfallDataFailMsg[128];

static void __stdcall SaveGame2() {
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Saving game: %s\n", DL_MAIN, buf);

	DWORD size, data;
	HANDLE h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		SaveGlobals(h);
		WriteFile(h, &objUniqueID, 4, &size, 0); // save unique id counter
		data = GetAddedYears(false) << 16; // save to high bytes (short)
		WriteFile(h, &data, 4, &size, 0);
		PerksSave(h);
		SaveArrays(h);
		DrugsSaveFix(h);
		CloseHandle(h);
	} else {
		goto errorSave;
	}

	if (FileSystemIsEmpty()) return;
	GetSavePath(buf, "fs");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		FileSystemSave(h);
		CloseHandle(h);
	} else {
		goto errorSave;
	}
	return;

/////////////////////////////////////////////////
errorSave:
	dlog_f("ERROR creating: %s\n", DL_MAIN, buf);
	DisplayPrint(saveSfallDataFailMsg);
	GsoundPlaySfxFile("IISXXXX1");
}

static char saveFailMsg[128];

static DWORD __stdcall CombatSaveTest() {
	if (!saveInCombatFix && !IsNpcControlled()) return 1;
	if (inLoop & COMBAT) {
		if (saveInCombatFix == 2 || IsNpcControlled() || !(inLoop & PCOMBAT)) {
			DisplayPrint(saveFailMsg);
			return 0;
		}
		int ap = StatLevel(*ptr_obj_dude, STAT_max_move_points);
		int bonusmove = PerkLevel(*ptr_obj_dude, PERK_bonus_move);
		if ((*ptr_obj_dude)->critter.movePoints != ap || bonusmove * 2 != *ptr_combat_free_move) {
			DisplayPrint(saveFailMsg);
			return 0;
		}
	}
	return 1;
}

static void __declspec(naked) SaveGame_hook() {
	__asm {
		push ecx;
		push edx;
		push eax; // save Mode parameter
		call CombatSaveTest;
		test eax, eax;
		pop  edx; // recall Mode parameter (pop eax)
		jz   end;
		mov  eax, edx;
		or inLoop, SAVEGAME;
		call SaveGame_;
		and inLoop, (-1 ^ SAVEGAME);
		cmp  eax, 1;
		jne  end;
		call SaveGame2; // save sfall.sav
		mov  eax, 1;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

// should be called before savegame is loaded
static bool __stdcall LoadGame_Before() {
	ResetState(1);

	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\n", DL_MAIN, buf);

	HANDLE h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD size, data;
		if (LoadGlobals(h)) goto errorLoad;
		long uID = 0;
		ReadFile(h, &uID, 4, &size, 0);
		if (uID > UID_START) objUniqueID = uID;
		ReadFile(h, &data, 4, &size, 0);
		SetAddedYears(data >> 16);
		if (size != 4 || !PerksLoad(h)) goto errorLoad;
		long result = LoadArrays(h); // 1 - old save, -1 - broken save
		if (result == -1 || (!result && DrugsLoadFix(h))) goto errorLoad;
		CloseHandle(h);
	} else {
		dlogr("Cannot open sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}
	return false;

/////////////////////////////////////////////////
errorLoad:
	CloseHandle(h);
	dlog_f("ERROR reading data: %s\n", DL_MAIN, buf);
	DebugPrintf("\n[SFALL] ERROR reading data: %s", buf);
	return (true & !isDebug);
}

static void __stdcall LoadGame_After() {
	CritLoad();
	LoadGlobalScripts();
	gameLoaded = true;
}

static void __declspec(naked) LoadSlot() {
	__asm {
		pushad;
		call LoadGame_Before;
		test al, al;
		popad;
		jnz  errorLoad;
		jmp  LoadSlot_;
errorLoad:
		mov  eax, -1;
		retn;
	}
}

static void __declspec(naked) LoadGame_hook() {
	__asm {
		or inLoop, LOADGAME;
		call LoadGame_;
		and inLoop, (-1 ^ LOADGAME);
		cmp  eax, 1;
		jne  end;
		// Invoked
		push ecx;
		push edx;
		call LoadGame_After;
		mov  eax, 1;
		pop  edx;
		pop  ecx;
end:
		retn;
	}
}

static void __declspec(naked) EndLoadHook() {
	__asm {
		call EndLoad_;
		pushadc;
		call LoadHeroAppearance;
		popadc;
		retn;
	}
}

static void __stdcall GameInitialization() { // OnBeforeGameInit
	BugFixes_Initialization();
	InterfaceGmouseHandleHook();
}

static void __stdcall game_init_hook() { // OnGameInit
	FallbackEnglishLoadMsgFiles();
}

static void __stdcall GameInitialized() { // OnAfterGameInit
	RemoveSavFiles();
	SetBoxMaxSlots();
	if (Use32BitTalkingHeads) TalkingHeadsSetup();
}

static void __declspec(naked) main_init_system_hook() {
	__asm {
		pushadc;
		call GameInitialization;
		popadc;
		call main_init_system_;
		pushadc;
		call GameInitialized;
		popadc;
		retn;
	}
}

static void NewGame2() {
	ResetState(0);

	dlogr("Starting new game", DL_MAIN);

	SetNewCharAppearanceGlobals();

	CritLoad();
	LoadGlobalScripts();
	LoadHeroAppearance();
	gameLoaded = true;
}

static void __declspec(naked) NewGame() {
	__asm {
		pushad;
		call NewGame2;
		mov  al, disableHorrigan;
		mov  byte ptr ds:[_Meet_Frank_Horrigan], al;
		popad;
		jmp  main_game_loop_;
	}
}

static void ReadExtraGameMsgFilesIfNeeded() {
	if (gExtraGameMsgLists.empty())
		ReadExtraGameMsgFiles();
}

static void __declspec(naked) MainMenuHook() {
	__asm {
		pushad;
		push 0;
		call ResetState;
		mov  al, pipBoyAvailableAtGameStart;
		mov  byte ptr ds:[_gmovie_played_list + 0x3], al;
		call ReadExtraGameMsgFilesIfNeeded;
		popad;
		jmp  main_menu_loop_;
	}
}

static void __stdcall GameClose() { // OnBeforeGameClose
	WipeSounds();
	ClearReadExtraGameMsgFiles();
}

static void __declspec(naked) game_close_hook() {
	__asm {
		pushadc;
		call GameClose;
		popadc;
		jmp game_exit_;
	}
}

static void __declspec(naked) WorldMapHook_Start() {
	__asm {
		call wmInterfaceInit_;
		test eax, eax;
		jl   skip;
		push eax;
		or inLoop, WORLDMAP;
		pop  eax;
skip:
		retn;
	}
}

static void __declspec(naked) WorldMapHook_End() {
	__asm {
		push eax;
		and inLoop, (-1 ^ WORLDMAP);
		pop  eax;
		jmp  remove_bk_process_;
	}
}

static void __declspec(naked) CombatHook() {
	__asm {
		pushadc;
		call AICombatStart;
		or inLoop, COMBAT;
		popadc;
		call combat_;
		pushadc;
		call AICombatEnd;
		and inLoop, (-1 ^ COMBAT);
		popadc;
		retn;
	}
}

static void __declspec(naked) PlayerCombatHook() {
	__asm {
		or inLoop, PCOMBAT;
		call combat_input_;
		and inLoop, (-1 ^ PCOMBAT);
		retn;
	}
}

static void __declspec(naked) EscMenuHook() {
	__asm {
		or inLoop, ESCMENU;
		call do_optionsFunc_;
		and inLoop, (-1 ^ ESCMENU);
		retn;
	}
}

static void __declspec(naked) EscMenuHook2() {
	__asm {
		or inLoop, ESCMENU;
		call do_options_;
		and inLoop, (-1 ^ ESCMENU);
		retn;
	}
}

static void __declspec(naked) OptionsMenuHook() {
	__asm {
		or inLoop, OPTIONS;
		call do_prefscreen_;
		and inLoop, (-1 ^ OPTIONS);
		retn;
	}
}

static void __declspec(naked) HelpMenuHook() {
	__asm {
		or inLoop, HELP;
		call game_help_;
		and inLoop, (-1 ^ HELP);
		retn;
	}
}

static void __declspec(naked) CharacterHook() {
	__asm {
		push edx;
		or inLoop, CHARSCREEN;
		call PerksEnterCharScreen;
		xor  eax, eax;
		call editor_design_;
		test eax, eax;
		jz   success;
		call PerksCancelCharScreen;
		jmp  end;
success:
		call PerksAcceptCharScreen;
end:
		and inLoop, (-1 ^ CHARSCREEN);
		mov  tagSkill4LevelBase, -1; // for fixing Tag! perk exploit
		pop  edx;
		retn;
	}
}

static void __declspec(naked) DialogHook_Start() {
	__asm {
		or inLoop, DIALOG;
		mov ebx, 1;
		retn;
	}
}

static void __declspec(naked) DialogHook_End() {
	__asm {
		and inLoop, (-1 ^ DIALOG);
		jmp gdialogFreeSpeech_;
	}
}

static void __declspec(naked) PipboyHook_Start() {
	__asm {
		push eax;
		or inLoop, PIPBOY;
		pop  eax;
		jmp  win_draw_;
	}
}

static void __declspec(naked) PipboyHook_End() {
	__asm {
		push eax;
		and inLoop, (-1 ^ PIPBOY);
		pop  eax;
		jmp  win_delete_;
	}
}

static void __declspec(naked) SkilldexHook() {
	__asm {
		or inLoop, SKILLDEX;
		call skilldex_select_;
		and inLoop, (-1 ^ SKILLDEX);
		retn;
	}
}

static void __declspec(naked) HandleInventoryHook_Start() {
	__asm {
		or inLoop, INVENTORY;
		xor eax, eax;
		jmp inven_set_mouse_;
	}
}

static void __declspec(naked) HandleInventoryHook_End() {
	__asm {
		and inLoop, (-1 ^ INVENTORY);
		mov eax, esi;
		jmp exit_inventory_;
	}
}

static void __declspec(naked) UseInventoryOnHook_Start() {
	__asm {
		or inLoop, INTFACEUSE;
		xor eax, eax;
		jmp inven_set_mouse_;
	}
}

static void __declspec(naked) UseInventoryOnHook_End() {
	__asm {
		and inLoop, (-1 ^ INTFACEUSE);
		mov eax, edi;
		jmp exit_inventory_;
	}
}

static void __declspec(naked) LootContainerHook_Start() {
	__asm {
		or inLoop, INTFACELOOT;
		xor eax, eax;
		jmp inven_set_mouse_;
	}
}

static void __declspec(naked) LootContainerHook_End() {
	__asm {
		cmp  dword ptr [esp + 0x150 - 0x58 + 4], 0; // JESSE_CONTAINER
		jz   skip; // container is not created
		and inLoop, (-1 ^ INTFACELOOT);
		xor  eax,eax;
skip:
		call ResetBodyState; // reset object pointer used in calculating the weight/size of equipped and hidden items on NPCs after exiting loot/barter screens
		retn 0x13C;
	}
}

static void __declspec(naked) BarterInventoryHook() {
	__asm {
		or inLoop, BARTER;
		push [esp + 4];
		call barter_inventory_;
		and inLoop, (-1 ^ BARTER);
		call ResetBodyState;
		retn 4;
	}
}

static void __declspec(naked) AutomapHook() {
	__asm {
		or inLoop, AUTOMAP;
		call automap_;
		and inLoop, (-1 ^ AUTOMAP);
		retn;
	}
}

static void __declspec(naked) DialogReviewInitHook() {
	__asm {
		call gdReviewInit_;
		test eax, eax;
		jnz  error;
		or inLoop, DIALOGVIEW;
		xor eax, eax;
error:
		retn;
	}
}

static void __declspec(naked) DialogReviewExitHook() {
	__asm {
		and inLoop, (-1 ^ DIALOGVIEW);
		jmp gdReviewExit_;
	}
}

static void __declspec(naked) setup_move_timer_win_Hook() {
	__asm {
		or inLoop, COUNTERWIN;
		jmp text_curr_;
	}
}

static void __declspec(naked) exit_move_timer_win_Hook() {
	__asm {
		and inLoop, (-1 ^ COUNTERWIN);
		jmp  win_delete_;
	}
}

void LoadGameHookInit() {
	saveInCombatFix = GetConfigInt("Misc", "SaveInCombatFix", 1);
	if (saveInCombatFix > 2) saveInCombatFix = 0;
	Translate("sfall", "SaveInCombat", "Cannot save at this time.", saveFailMsg);
	Translate("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!", saveSfallDataFailMsg);

	switch (GetConfigInt("Misc", "PipBoyAvailableAtGameStart", 0)) {
	case 1:
		pipBoyAvailableAtGameStart = true;
		break;
	case 2:
		SafeWrite8(0x497011, 0xEB); // skip the vault suit movie check
		break;
	}

	if (GetConfigInt("Misc", "DisableHorrigan", 0)) {
		disableHorrigan = true;
		SafeWrite8(0x4C06D8, 0xEB); // skip the Horrigan encounter check
	}

	// 4.x backport
	HookCall(0x4809BA, main_init_system_hook);
	HookCall(0x4426A6, game_init_hook);

	HookCall(0x480AB3, NewGame);
	const DWORD loadSlotAddr[] = {0x47C72C, 0x47D1C9};
	HookCalls(LoadSlot, loadSlotAddr);
	const DWORD loadGameAddr[] = {0x443AE4, 0x443B89, 0x480B77, 0x48FD35};
	HookCalls(LoadGame_hook, loadGameAddr);
	SafeWrite32(0x5194C0, (DWORD)&EndLoadHook);
	const DWORD saveGameAddr[] = {0x443AAC, 0x443B1C, 0x48FCFF};
	HookCalls(SaveGame_hook, saveGameAddr);

	HookCall(0x480A28, MainMenuHook);
	// 4.x backport
	HookCall(0x480CA7, game_close_hook); // gnw_main_
	//HookCall(0x480D45, game_close_hook); // main_exit_system_ (never called)

	/////////////// GAME MODES ///////////////
	HookCall(0x4BFE33, WorldMapHook_Start); // wmTownMap_ (old 0x483668, 0x4A4073)
	HookCall(0x4C2E4F, WorldMapHook_End);   // wmInterfaceExit_ (old 0x4C4855)

	const DWORD combatHkAddr[] = {0x426A29, 0x4432BE, 0x45F6D2, 0x4A4020, 0x4A403D};
	HookCalls(CombatHook, combatHkAddr);
	HookCall(0x422B09, PlayerCombatHook);

	HookCall(0x480C16, EscMenuHook);   // gnw_main_
	HookCall(0x4433BE, EscMenuHook2);  // game_handle_input_
	const DWORD optionsMenuHkAddr[] = {0x48FCE4, 0x48FD17, 0x48FD4D, 0x48FD6A, 0x48FD87, 0x48FDB3};
	HookCalls(OptionsMenuHook, optionsMenuHkAddr);
	HookCall(0x443A50, HelpMenuHook);  // game_handle_input_
	HookCall(0x443320, CharacterHook); // 0x4A73EB, 0x4A740A for character creation

	MakeCall(0x445285, DialogHook_Start); // gdialogInitFromScript_
	HookCall(0x4452CD, DialogHook_End);   // gdialogExitFromScript_ (old 0x445748)

	const DWORD pipboyHkStartAddr[] = {0x49767F, 0x4977EF, 0x49780D}; // StartPipboy_ (old 0x443463, 0x443605)
	HookCalls(PipboyHook_Start, pipboyHkStartAddr);
	HookCall(0x497868, PipboyHook_End); // EndPipboy_

	const DWORD skilldexHkAddr[] = {0x4434AC, 0x44C7BD};
	HookCalls(SkilldexHook, skilldexHkAddr);

	HookCall(0x46E8F3, HandleInventoryHook_Start); // handle_inventory_ (old 0x443357)
	HookCall(0x46EC2D, HandleInventoryHook_End);

	HookCall(0x471823, UseInventoryOnHook_Start); // use_inventory_on_ (old 0x44C6FB)
	HookCall(0x471B2C, UseInventoryOnHook_End);

	HookCall(0x473E0D, LootContainerHook_Start); // loot_container_ (old 0x4746EC, 0x4A4369, 0x4A4565)
	MakeCall(0x474692, LootContainerHook_End, 1);

	HookCall(0x4466C7, BarterInventoryHook); // gdProcess_

	const DWORD automapHkAddr[] = {0x44396D, 0x479519};
	HookCalls(AutomapHook, automapHkAddr);

	HookCall(0x445CA7, DialogReviewInitHook);
	HookCall(0x445D30, DialogReviewExitHook);

	HookCall(0x476AC6, setup_move_timer_win_Hook); // before init win
	HookCall(0x477067, exit_move_timer_win_Hook);
}
