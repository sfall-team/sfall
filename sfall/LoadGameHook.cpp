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
#include "HookScripts.h"
#include "InputFuncs.h"
#include "Interface.h"
#include "Inventory.h"
#include "LoadOrder.h"
#include "Logging.h"
#include "Message.h"
#include "MetaruleExtender.h"
#include "Movies.h"
#include "Objects.h"
#include "PartyControl.h"
#include "Perks.h"
#include "QuestList.h"
#include "ScriptExtender.h"
#include "Skills.h"
#include "Sound.h"
#include "Stats.h"
#include "TalkingHeads.h"
#include "Translate.h"
#include "version.h"
#include "Worldmap.h"

#include "LoadGameHook.h"

#define _InLoop2(type, flag) __asm { \
	__asm push flag                  \
	__asm push type                  \
	__asm call SetInLoop             \
}
#define _InLoop(type, flag) __asm {  \
	pushadc                          \
	_InLoop2(type, flag)             \
	popadc                           \
}

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
static bool disableHorrigan = false;
static bool pipBoyAvailableAtGameStart = false;
static bool gameLoaded = false;

long gameInterfaceWID = -1;

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

static void __stdcall GameModeChange(DWORD state) { // OnGameModeChange
	GameModeChangeHook(state);
}

void __stdcall SetInLoop(DWORD mode, LoopFlag flag) {
	unsigned long _inLoop = inLoop;
	if (mode) {
		SetLoopFlag(flag);
	} else {
		ClearLoopFlag(flag);
	}
	if (_inLoop ^ inLoop) GameModeChange(0);
}

static void __stdcall RunOnBeforeGameStart() {
	BodypartHitChances(); // set on start & load
	CritLoad();
	ReadExtraGameMsgFiles();
	if (pipBoyAvailableAtGameStart) ptr_gmovie_played_list[3] = true; // PipBoy aquiring video
	LoadGlobalScripts(); // loading sfall scripts
}

static void __stdcall RunOnAfterGameStarted() {
	if (disableHorrigan) *ptr_Meet_Frank_Horrigan = true;
	InitGlobalScripts(); // running sfall scripts
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", *ptr_patches, *ptr_slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static void __stdcall SaveGame2() {
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Saving game: %s\n", DL_MAIN, buf);

	DWORD size, data;
	HANDLE h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		SaveGlobals(h);
		WriteFile(h, &Objects_uniqueID, 4, &size, 0); // save unique id counter
		data = Worldmap_GetAddedYears(false) << 16; // save to high bytes (short)
		WriteFile(h, &data, 4, &size, 0);
		PerksSave(h);
		SaveArrays(h);
		BugFixes_DrugsSaveFix(h);
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
	fo_display_print(Translate_SfallSaveDataFailure());
	fo_gsound_play_sfx_file("IISXXXX1");
}

static DWORD __stdcall CombatSaveTest() {
	if (!saveInCombatFix && !PartyControl_IsNpcControlled()) return 1;
	if (inLoop & COMBAT) {
		if (saveInCombatFix == 2 || PartyControl_IsNpcControlled() || !(inLoop & PCOMBAT)) {
			fo_display_print(Translate_CombatSaveBlockMessage());
			return 0;
		}
		int ap = fo_stat_level(*ptr_obj_dude, STAT_max_move_points);
		int bonusmove = fo_perk_level(*ptr_obj_dude, PERK_bonus_move);
		if ((*ptr_obj_dude)->critter.movePoints != ap || bonusmove * 2 != *ptr_combat_free_move) {
			fo_display_print(Translate_CombatSaveBlockMessage());
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
		push edx;
		_InLoop2(1, SAVEGAME);
		pop  eax;
		call SaveGame_;
		push eax;
		_InLoop2(0, SAVEGAME);
		pop  eax;
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

// Called right before savegame slot is being loaded
static bool __stdcall LoadGame_Before() {
	RunOnBeforeGameStart();

	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\n", DL_MAIN, buf);

	HANDLE h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD size, data;
		if (LoadGlobals(h)) goto errorLoad;
		long uID = 0;
		ReadFile(h, &uID, 4, &size, 0);
		if (uID > UID_START) Objects_uniqueID = uID;
		ReadFile(h, &data, 4, &size, 0);
		Worldmap_SetAddedYears(data >> 16);
		if (size != 4 || !PerksLoad(h)) goto errorLoad;
		long result = LoadArrays(h); // 1 - old save, -1 - broken save
		if (result == -1 || (!result && BugFixes_DrugsLoadFix(h))) goto errorLoad;
		CloseHandle(h);
	} else {
		dlogr("Cannot open sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}
	return false;

/////////////////////////////////////////////////
errorLoad:
	CloseHandle(h);
	dlog_f("ERROR reading data: %s\n", DL_MAIN, buf);
	fo_debug_printf("\n[SFALL] ERROR reading data: %s", buf);
	return (true & !isDebug);
}

// called whenever game is being reset (prior to loading a save or when returning to main menu)
static bool __stdcall GameReset(DWORD isGameLoad) {
	if (gameLoaded) { // prevent resetting when a new game has not been started (loading saved game from main menu)
		// OnGameReset
		BugFixes_OnGameLoad();
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
		ResetQuests();
		WipeSounds();
		InventoryReset();
		PartyControl_OnGameLoad();
		Explosions_OnGameLoad();
		ClearScriptAddedExtraGameMsg();
		BarBoxes_OnGameLoad();
		MetaruleExtenderReset();
		ScriptExtender_OnGameLoad();
		if (isDebug) {
			char* str = (isGameLoad) ? "on Load" : "on Exit";
			fo_debug_printf("\n[SFALL: State reset %s]", str);
		}
	}
	inLoop = 0;
	gameLoaded = false;

	return (isGameLoad) ? LoadGame_Before() : false;
}

// Called after game was loaded from a save
static void __stdcall LoadGame_After() {
	RunOnAfterGameStarted();
	gameLoaded = true;
}

static void __declspec(naked) LoadGame_hook() {
	__asm {
		_InLoop(1, LOADGAME);
		call LoadGame_;
		_InLoop(0, LOADGAME);
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

static void __stdcall NewGame_Before() {
	RunOnBeforeGameStart();
}

static void __stdcall NewGame_After() {
	dlogr("New Game started.", DL_MAIN);
	// OnAfterNewGame
	if (appModEnabled) {
		SetNewCharAppearanceGlobals();
		LoadHeroAppearance();
	}
	RunOnAfterGameStarted();
	gameLoaded = true;
}

static void __declspec(naked) main_load_new_hook() {
	__asm {
		push eax;
		call NewGame_Before;
		pop  eax;
		call main_load_new_;
		jmp  NewGame_After;
	}
}

static void __stdcall GameInitialization() { // OnBeforeGameInit
	BugFixes_Initialization();
	Interface_OnBeforeGameInit();
}

static void __stdcall game_init_hook() { // OnGameInit
	FallbackEnglishLoadMsgFiles();
}

static void __stdcall GameInitialized(int initResult) { // OnAfterGameInit
	#ifdef NDEBUG
	if (!initResult) {
		MessageBoxA(0, "Game initialization failed!", "Error", MB_TASKMODAL | MB_ICONERROR);
		return;
	}
	#endif
	RemoveSavFiles();
	Sound_OnAfterGameInit();
	BarBoxes_SetMaxSlots();
	if (Use32BitTalkingHeads) TalkingHeadsSetup();
	BuildSortedIndexList();
}
/*
static void __stdcall GameExit() { // OnGameExit
	// reserved
}
*/
static void __stdcall GameClose() { // OnBeforeGameClose
	WipeSounds();
	ClearReadExtraGameMsgFiles();
}

static void __declspec(naked) main_init_system_hook() {
	__asm {
		pushadc;
		call GameInitialization;
		popadc;
		call main_init_system_;
		pushadc;
		push eax;
		call GameInitialized;
		popadc;
		retn;
	}
}

static void __declspec(naked) game_reset_hook() {
	__asm {
		pushadc;
		push 0;
		call GameReset; // reset all sfall modules before resetting the game data
		popadc;
		jmp  game_reset_;
	}
}

static void __declspec(naked) game_reset_on_load_hook() {
	__asm {
		pushadc;
		push 1;
		call GameReset; // reset all sfall modules before resetting the game data
		test al, al;
		popadc;
		jnz  errorLoad;
		jmp  game_reset_;
errorLoad:
		mov  eax, -1;
		add  esp, 4;
		pop  edx;
		retn;
	}
}

static void __declspec(naked) before_game_exit_hook() {
	__asm {
		pushadc;
		push 1;
		call GameModeChange;
		popadc;
		jmp  map_exit_;
	}
}
/*
static void __declspec(naked) after_game_exit_hook() {
	__asm {
		pushadc;
		call GameExit;
		popadc;
		jmp  main_menu_create_;
	}
}
*/
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
		_InLoop2(1, WORLDMAP);
		pop  eax;
skip:
		retn;
	}
}

static void __declspec(naked) WorldMapHook_End() {
	__asm {
		push eax;
		_InLoop2(0, WORLDMAP);
		pop  eax;
		jmp  remove_bk_process_;
	}
}

static void __fastcall CombatInternal(CombatGcsd* gcsd) {
	// OnCombatStart
	AICombatClear();
	SetInLoop(1, COMBAT);

	__asm mov  eax, gcsd;
	__asm call combat_;

	// OnCombatEnd
	AICombatClear();
	SetInLoop(0, COMBAT);
}

static void __declspec(naked) CombatHook() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, eax;
		call CombatInternal;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) PlayerCombatHook() {
	__asm {
		_InLoop(1, PCOMBAT);
		call combat_input_;
		_InLoop(0, PCOMBAT);
		retn;
	}
}

static void __declspec(naked) EscMenuHook() {
	__asm {
		_InLoop(1, ESCMENU);
		call do_optionsFunc_;
		_InLoop(0, ESCMENU);
		retn;
	}
}

static void __declspec(naked) EscMenuHook2() {
	__asm {
		_InLoop(1, ESCMENU);
		call do_options_;
		_InLoop(0, ESCMENU);
		retn;
	}
}

static void __declspec(naked) OptionsMenuHook() {
	__asm {
		_InLoop(1, OPTIONS);
		call do_prefscreen_;
		_InLoop(0, OPTIONS);
		retn;
	}
}

static void __declspec(naked) HelpMenuHook() {
	__asm {
		_InLoop(1, HELP);
		call game_help_;
		_InLoop(0, HELP);
		retn;
	}
}

static void __declspec(naked) CharacterHook() {
	__asm {
		push edx;
		_InLoop2(1, CHARSCREEN);
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
		_InLoop2(0, CHARSCREEN);
		mov  tagSkill4LevelBase, -1; // for fixing Tag! perk exploit
		pop  edx;
		retn;
	}
}

static void __declspec(naked) DialogHook_Start() {
	__asm {
		_InLoop2(1, DIALOG);
		mov ebx, 1;
		retn;
	}
}

static void __declspec(naked) DialogHook_End() {
	__asm {
		and inLoop, ~DIALOG;  // unset flag
		_InLoop2(1, SPECIAL); // set the flag before animating the panel when exiting the dialog
		call gdDestroyHeadWindow_;
		_InLoop2(0, SPECIAL);
		retn;
	}
}

static void __declspec(naked) PipboyHook_Start() {
	__asm {
		push eax;
		_InLoop2(1, PIPBOY);
		pop  eax;
		jmp  win_draw_;
	}
}

static void __declspec(naked) PipboyHook_End() {
	__asm {
		push eax;
		_InLoop2(0, PIPBOY);
		pop  eax;
		jmp  win_delete_;
	}
}

static void __declspec(naked) SkilldexHook() {
	__asm {
		_InLoop(1, SKILLDEX);
		call skilldex_select_;
		_InLoop(0, SKILLDEX);
		retn;
	}
}

static void __declspec(naked) HandleInventoryHook_Start() {
	__asm {
		_InLoop2(1, INVENTORY);
		xor eax, eax;
		jmp inven_set_mouse_;
	}
}

static void __declspec(naked) HandleInventoryHook_End() {
	__asm {
		_InLoop2(0, INVENTORY);
		mov eax, esi;
		jmp exit_inventory_;
	}
}

static void __declspec(naked) UseInventoryOnHook_Start() {
	__asm {
		_InLoop2(1, INTFACEUSE);
		xor eax, eax;
		jmp inven_set_mouse_;
	}
}

static void __declspec(naked) UseInventoryOnHook_End() {
	__asm {
		_InLoop2(0, INTFACEUSE);
		mov eax, edi;
		jmp exit_inventory_;
	}
}

static void __declspec(naked) LootContainerHook_Start() {
	__asm {
		_InLoop2(1, INTFACELOOT);
		xor eax, eax;
		jmp inven_set_mouse_;
	}
}

static void __declspec(naked) LootContainerHook_End() {
	__asm {
		cmp  dword ptr [esp + 0x150 - 0x58 + 4], 0; // JESSE_CONTAINER
		jz   skip; // container is not created
		_InLoop2(0, INTFACELOOT);
		xor  eax, eax;
skip:
		call ResetBodyState; // reset object pointer used in calculating the weight/size of equipped and hidden items on NPCs after exiting loot/barter screens
		retn 0x13C;
	}
}

static void __declspec(naked) BarterInventoryHook() {
	__asm {
		and inLoop, ~SPECIAL; // unset flag after animating the dialog panel
		_InLoop(1, BARTER);
		push [esp + 4];
		call barter_inventory_;
		_InLoop(0, BARTER);
		call ResetBodyState;
		retn 4;
	}
}

static void __declspec(naked) AutomapHook_Start() {
	__asm {
		call gmouse_set_cursor_;
		test edx, edx;
		jnz  skip;
		mov  gameInterfaceWID, ebp;
		_InLoop(1, AUTOMAP);
skip:
		retn;
	}
}

static void __declspec(naked) AutomapHook_End() {
	__asm {
		_InLoop(0, AUTOMAP);
		mov gameInterfaceWID, -1
		jmp win_delete_;
	}
}

static void __declspec(naked) DialogReviewInitHook() {
	__asm {
		call gdReviewInit_;
		test eax, eax;
		jnz  error;
		push ecx;
		_InLoop2(1, DIALOGVIEW);
		pop ecx;
		xor eax, eax;
error:
		retn;
	}
}

static void __declspec(naked) DialogReviewExitHook() {
	__asm {
		push ecx;
		push eax;
		_InLoop2(0, DIALOGVIEW);
		pop eax;
		pop ecx;
		jmp gdReviewExit_;
	}
}

static void __declspec(naked) setup_move_timer_win_Hook() {
	__asm {
		_InLoop2(1, COUNTERWIN);
		jmp text_curr_;
	}
}

static void __declspec(naked) exit_move_timer_win_Hook() {
	__asm {
		push eax;
		_InLoop2(0, COUNTERWIN);
		pop  eax;
		jmp  win_delete_;
	}
}

static void __declspec(naked) gdialog_bk_hook() {
	__asm {
		_InLoop2(1, SPECIAL); // set the flag before switching from dialog mode to barter
		jmp gdialog_window_destroy_;
	}
}

static void __declspec(naked) gdialogUpdatePartyStatus_hook1() {
	__asm {
		push edx;
		_InLoop2(1, SPECIAL); // set the flag before animating the dialog panel when a party member joins/leaves
		pop  edx;
		jmp  gdialog_window_destroy_;
	}
}

static void __declspec(naked) gdialogUpdatePartyStatus_hook0() {
	__asm {
		call gdialog_window_create_;
		_InLoop2(0, SPECIAL); // unset the flag when entering the party member control panel
		retn;
	}
}

void LoadGameHook_Init() {
	saveInCombatFix = GetConfigInt("Misc", "SaveInCombatFix", 1);
	if (saveInCombatFix > 2) saveInCombatFix = 0;

	switch (GetConfigInt("Misc", "PipBoyAvailableAtGameStart", 0)) {
	case 1:
		pipBoyAvailableAtGameStart = true;
		break;
	case 2:
		SafeWrite8(0x497011, CODETYPE_JumpShort); // skip the vault suit movie check
		break;
	}

	if (GetConfigInt("Misc", "DisableHorrigan", 0)) {
		disableHorrigan = true;
	}

	HookCall(0x4809BA, main_init_system_hook);
	HookCall(0x4426A6, game_init_hook);
	HookCall(0x480AAE, main_load_new_hook);
	const DWORD loadGameAddr[] = {0x443AE4, 0x443B89, 0x480B77, 0x48FD35};
	HookCalls(LoadGame_hook, loadGameAddr);
	SafeWrite32(0x5194C0, (DWORD)&EndLoadHook);
	const DWORD saveGameAddr[] = {0x443AAC, 0x443B1C, 0x48FCFF};
	HookCalls(SaveGame_hook, saveGameAddr);
	const DWORD gameResetAddr[] = {
		0x47DD6B, // LoadSlot_ (on load error)
		0x47DDF3, // LoadSlot_ (on load error)
		//0x480708, // RestoreLoad_ (never called)
		0x480AD3, // gnw_main_ (game ended after playing via New Game)
		0x480BCC, // gnw_main_ (game ended after playing via Load Game)
		//0x480D0C, // main_reset_system_ (never called)
		0x481028, // main_selfrun_record_
		0x481062, // main_selfrun_record_
		0x48110B, // main_selfrun_play_
		0x481145 // main_selfrun_play_
	};
	HookCalls(game_reset_hook, gameResetAddr);
	HookCall(0x47F491, game_reset_on_load_hook); // PrepLoad_ (the very first step during save game loading)
	const DWORD beforeGameExitAddr[] = {0x480ACE, 0x480BC7}; // gnw_main_
	HookCalls(before_game_exit_hook, beforeGameExitAddr);
	//const DWORD afterGameExitAddr[] = {0x480AEB, 0x480BE4}; // gnw_main_
	//HookCalls(after_game_exit_hook, afterGameExitAddr);
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

	MakeCall(0x445285, DialogHook_Start); // gdialogInitFromScript_ (old 0x445748)
	HookCall(0x445317, DialogHook_End);   // gdialogExitFromScript_

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

	HookCall(0x41BAB6, AutomapHook_Start); // automap_
	HookCall(0x41BCDB, AutomapHook_End);   // automap_

	HookCall(0x445CA7, DialogReviewInitHook);
	HookCall(0x445D30, DialogReviewExitHook);

	HookCall(0x476AC6, setup_move_timer_win_Hook); // before init win
	HookCall(0x477067, exit_move_timer_win_Hook);

	// Set and unset the Special flag of game mode when animating the dialog interface panel
	HookCall(0x447A7E, gdialog_bk_hook); // set when switching from dialog mode to barter mode (unset when entering barter)
	HookCall(0x4457B1, gdialogUpdatePartyStatus_hook1); // set when a party member joins/leaves
	HookCall(0x4457BC, gdialogUpdatePartyStatus_hook0); // unset
}
