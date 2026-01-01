/*
 *    sfall
 *    Copyright (C) 2008-2026  The sfall team
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
#include "..\Delegate.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\Translate.h"
#include "..\version.h"

#include "BugFixes.h"
#include "CritterStats.h"
#include "ExtraSaveSlots.h"
#include "FileSystem.h"
#include "HeroAppearance.h"
#include "HookScripts.h"
#include "Objects.h"
#include "PartyControl.h"
#include "Perks.h"
#include "ScriptExtender.h"
#include "Scripting\Arrays.h"
#include "Worldmap.h"

#include "LoadGameHook.h"

#include <chrono>

namespace sfall
{

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

static Delegate<> onBeforeGameInit;
static Delegate<> onGameInit;
static Delegate<> onAfterGameInit;
static Delegate<> onGameExit;
static Delegate<> onGameReset;
static Delegate<> onBeforeGameStart;
static Delegate<> onAfterGameStarted;
static Delegate<> onAfterNewGame;
static Delegate<DWORD> onGameModeChange;
static Delegate<> onBeforeGameClose;
static Delegate<> onCombatStart;
static Delegate<> onCombatEnd;
static Delegate<> onBeforeMapLoad;

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
static bool gameLoaded = false;
static bool onLoadingMap = false;

static std::chrono::time_point<std::chrono::steady_clock> timeBeforeGameInit;
static std::chrono::time_point<std::chrono::steady_clock> timeBeforeGameStart;

char LoadGameHook::mapLoadingName[16]; // current loading/loaded map name

long LoadGameHook::interfaceWID = -1;

void saveTimeIntervalStart(std::chrono::time_point<std::chrono::steady_clock>& startTime) {
	startTime = std::chrono::high_resolution_clock::now();
}

void logTimeInterval(const char* action, const std::chrono::time_point<std::chrono::steady_clock>& startTime) {
	auto stopTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stopTime - startTime);
	dlog_f("%s in: %d us\n", DL_MAIN, action, duration.count());
}

bool LoadGameHook::IsMapLoading() {
	return onLoadingMap;
}

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

static void __stdcall GameModeChange(DWORD state) {
	onGameModeChange.invoke(state);
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

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", fo::var::patches, ExtraSaveSlots::GetSaveSlot() + 1, ftype); //add SuperSave Page offset
}

static void __stdcall SaveGame2() {
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Saving game: %s\n", DL_MAIN, buf);

	DWORD size, data;
	HANDLE h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		SaveGlobals(h);
		WriteFile(h, &Objects::uniqueID, 4, &size, 0); // save unique id counter
		data = Worldmap::GetAddedYears(false) << 16;   // save to high bytes (short)
		WriteFile(h, &data, 4, &size, 0);
		Perks::Save(h);
		script::SaveArrays(h);
		BugFixes::DrugsSaveFix(h);
		CloseHandle(h);
	} else {
		goto errorSave;
	}

	GetSavePath(buf, "db");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		Worldmap::SaveData(h);
		CritterStats::SaveStatData(h);

		// last marker
		data = 0xCC | VERSION_MAJOR << 8 | VERSION_MINOR << 16 | VERSION_BUILD << 24;
		WriteFile(h, &data, 4, &size, 0);
		CloseHandle(h);
	} else {
		goto errorSave;
	}

	if (FileSystem::IsEmpty()) return;
	GetSavePath(buf, "fs");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		FileSystem::Save(h);
		CloseHandle(h);
	} else {
		goto errorSave;
	}
	return;

/////////////////////////////////////////////////
errorSave:
	dlog_f("ERROR creating: %s\n", DL_MAIN, buf);
	fo::util::DisplayPrint(Translate::SfallSaveDataFailure());
	fo::func::gsound_play_sfx_file("IISXXXX1");
}

static DWORD __stdcall CombatSaveTest() {
	if (!saveInCombatFix && !PartyControl::IsNpcControlled()) return 1;
	if (inLoop & COMBAT) {
		if (saveInCombatFix == 2 || PartyControl::IsNpcControlled() || !(inLoop & PCOMBAT)) {
			fo::util::DisplayPrint(Translate::CombatSaveBlockMessage());
			return 0;
		}
		int ap = fo::func::stat_level(fo::var::obj_dude, fo::STAT_max_move_points);
		int bonusmove = fo::func::perk_level(fo::var::obj_dude, fo::PERK_bonus_move);
		if (fo::var::obj_dude->critter.movePoints != ap || bonusmove * 2 != fo::var::combat_free_move) {
			fo::util::DisplayPrint(Translate::CombatSaveBlockMessage());
			return 0;
		}
	}
	return 1;
}

static __declspec(naked) void SaveGame_hook() {
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
		call fo::funcoffs::SaveGame_;
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
static bool LoadGame_Before() {
	saveTimeIntervalStart(timeBeforeGameStart);
	onBeforeGameStart.invoke();

	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\n", DL_MAIN, buf);

	HANDLE h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD size, data;
		if (LoadGlobals(h)) goto errorLoad;
		long uID = 0;
		ReadFile(h, &uID, 4, &size, 0);
		if (uID > UniqueID::Start) Objects::uniqueID = uID;
		ReadFile(h, &data, 4, &size, 0);
		Worldmap::SetAddedYears(data >> 16);
		if (size != 4 || !Perks::Load(h)) goto errorLoad;
		long result = script::LoadArrays(h); // 1 - old save, -1 - broken save
		if (result == -1 || (!result && BugFixes::DrugsLoadFix(h))) goto errorLoad;
		CloseHandle(h);
	} else {
		dlogr("Cannot open sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}

	GetSavePath(buf, "db");
	dlogr("Loading data from sfalldb.sav...", DL_MAIN);
	h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		if (Worldmap::LoadData(h) || CritterStats::LoadStatData(h)) goto errorLoad;
		CloseHandle(h);
	} else {
		dlogr("Cannot open sfalldb.sav.", DL_MAIN);
	}
	return false;

/////////////////////////////////////////////////
errorLoad:
	CloseHandle(h);
	dlog_f("ERROR reading data: %s\n", DL_MAIN, buf);
	fo::func::debug_printf("\n[SFALL] ERROR reading data: %s", buf);
	return (true & !isDebug);
}

// called whenever game is being reset (prior to loading a save or when returning to main menu)
static bool __stdcall GameReset(DWORD isGameLoad) {
	onGameReset.invoke();
	if (isDebug) {
		char* str = (isGameLoad) ? "on Load" : "on Exit";
		fo::func::debug_printf("\nSFALL: [State reset %s]\n", str);
	}
	inLoop = 0;
	gameLoaded = false;

	return (isGameLoad) ? LoadGame_Before() : false;
}

// Called after game was loaded from a save
static void __stdcall LoadGame_After() {
	onAfterGameStarted.invoke();
	gameLoaded = true;
	logTimeInterval("Game loaded", timeBeforeGameStart);
}

static __declspec(naked) void LoadGame_hook() {
	__asm {
		_InLoop(1, LOADGAME);
		call fo::funcoffs::LoadGame_;
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

static __declspec(naked) void EndLoadHook() {
	__asm {
		call fo::funcoffs::EndLoad_;
		pushadc;
		call LoadHeroAppearance;
		popadc;
		retn;
	}
}

static void __stdcall NewGame_Before() {
	saveTimeIntervalStart(timeBeforeGameStart);
	onBeforeGameStart.invoke();
}

static void __stdcall NewGame_After() {
	onAfterNewGame.invoke();
	onAfterGameStarted.invoke();
	gameLoaded = true;
	logTimeInterval("New Game started", timeBeforeGameStart);
}

static __declspec(naked) void main_load_new_hook() {
	__asm {
		push eax;
		call NewGame_Before;
		pop  eax;
		call fo::funcoffs::main_load_new_;
		jmp  NewGame_After;
	}
}

static void __stdcall GameInitialization() {
	saveTimeIntervalStart(timeBeforeGameInit);
	onBeforeGameInit.invoke();
}

static void __stdcall game_init_hook() {
	onGameInit.invoke();
}

static void __stdcall GameInitialized(int initResult) {
	#ifdef NDEBUG
	if (!initResult) {
		MessageBoxA(0, "Game initialization failed!", 0, MB_TASKMODAL | MB_ICONERROR);
		return;
	}
	#endif
	onAfterGameInit.invoke();

	logTimeInterval("Game initalized", timeBeforeGameInit);
}

static void __stdcall GameExit() {
	onGameExit.invoke();
}

static void __stdcall GameClose() {
	onBeforeGameClose.invoke();
}

static void __stdcall MapLoadHook() {
	onBeforeMapLoad.invoke();
}

static __declspec(naked) void main_init_system_hook() {
	__asm {
		pushadc;
		call GameInitialization;
		popadc;
		call fo::funcoffs::main_init_system_;
		pushadc;
		push eax;
		call GameInitialized;
		popadc;
		retn;
	}
}

static __declspec(naked) void game_reset_hook() {
	__asm {
		pushadc;
		push 0;
		call GameReset; // reset all sfall modules before resetting the game data
		popadc;
		jmp  fo::funcoffs::game_reset_;
	}
}

static __declspec(naked) void game_reset_on_load_hook() {
	__asm {
		pushadc;
		push 1;
		call GameReset; // reset all sfall modules before resetting the game data
		test al, al;
		popadc;
		jnz  errorLoad;
		jmp  fo::funcoffs::game_reset_;
errorLoad:
		mov  eax, -1;
		add  esp, 4;
		pop  edx;
		retn;
	}
}

static __declspec(naked) void before_game_exit_hook() {
	__asm {
		pushadc;
		push 1;
		call GameModeChange;
		popadc;
		jmp  fo::funcoffs::map_exit_;
	}
}

static __declspec(naked) void after_game_exit_hook() {
	__asm {
		pushadc;
		call GameExit;
		popadc;
		jmp  fo::funcoffs::main_menu_create_;
	}
}

static __declspec(naked) void game_close_hook() {
	__asm {
		pushadc;
		call GameClose;
		popadc;
		jmp  fo::funcoffs::game_exit_;
	}
}


static __declspec(naked) void map_load_hook() {
	__asm {
		mov  esi, ebx;
		lea  edi, LoadGameHook::mapLoadingName;
		mov  ecx, 4;
		rep  movsd; // copy the name of the loaded map to mapLoadingName
		mov  onLoadingMap, 1;
		push eax;
		push edx;
		call MapLoadHook;
		pop  edx;
		pop  eax;
		call fo::funcoffs::map_load_file_;
		mov  onLoadingMap, 0;
		retn;
	}
}

static __declspec(naked) void WorldMapHook_Start() {
	__asm {
		call fo::funcoffs::wmInterfaceInit_;
		test eax, eax;
		jl   skip;
		push eax;
		_InLoop2(1, WORLDMAP);
		pop  eax;
skip:
		retn;
	}
}

static __declspec(naked) void WorldMapHook_End() {
	__asm {
		push eax;
		_InLoop2(0, WORLDMAP);
		pop  eax;
		jmp  fo::funcoffs::remove_bk_process_;
	}
}

static void __fastcall CombatInternal(fo::CombatGcsd* gcsd) {
	onCombatStart.invoke();
	SetInLoop(1, COMBAT);

	__asm mov  eax, gcsd;
	__asm call fo::funcoffs::combat_;

	onCombatEnd.invoke();
	SetInLoop(0, COMBAT);
}

static __declspec(naked) void CombatHook() {
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

static __declspec(naked) void PlayerCombatHook() {
	__asm {
		_InLoop(1, PCOMBAT);
		call fo::funcoffs::combat_input_;
		_InLoop(0, PCOMBAT);
		retn;
	}
}

static __declspec(naked) void EscMenuHook() {
	__asm {
		_InLoop(1, ESCMENU);
		call fo::funcoffs::do_optionsFunc_;
		_InLoop(0, ESCMENU);
		retn;
	}
}

static __declspec(naked) void EscMenuHook2() {
	__asm {
		_InLoop(1, ESCMENU);
		call fo::funcoffs::do_options_;
		_InLoop(0, ESCMENU);
		retn;
	}
}

static __declspec(naked) void OptionsMenuHook() {
	__asm {
		_InLoop(1, OPTIONS);
		call fo::funcoffs::do_prefscreen_;
		_InLoop(0, OPTIONS);
		retn;
	}
}

static __declspec(naked) void HelpMenuHook() {
	__asm {
		_InLoop(1, HELP);
		call fo::funcoffs::game_help_;
		_InLoop(0, HELP);
		retn;
	}
}

static __declspec(naked) void PauseWindowHook() {
	__asm {
		_InLoop(1, PAUSEWIN);
		call fo::funcoffs::PauseWindow_;
		_InLoop(0, PAUSEWIN);
		retn;
	}
}

static __declspec(naked) void CharacterHook() {
	__asm {
		push edx;
		_InLoop2(1, CHARSCREEN);
		call PerksEnterCharScreen;
		xor  eax, eax;
		call fo::funcoffs::editor_design_;
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

static __declspec(naked) void DialogHook_Start() {
	__asm {
		_InLoop2(1, DIALOG);
		mov ebx, 1;
		retn;
	}
}

static __declspec(naked) void DialogHook_End() {
	__asm {
		and inLoop, ~DIALOG;  // unset flag
		_InLoop2(1, SPECIAL); // set the flag before animating the panel when exiting the dialog
		call fo::funcoffs::gdDestroyHeadWindow_;
		_InLoop2(0, SPECIAL);
		retn;
	}
}

static __declspec(naked) void PipboyHook_Start() {
	__asm {
		push eax;
		_InLoop2(1, PIPBOY);
		pop  eax;
		jmp  fo::funcoffs::win_draw_;
	}
}

static __declspec(naked) void PipboyHook_End() {
	__asm {
		push eax;
		_InLoop2(0, PIPBOY);
		pop  eax;
		jmp  fo::funcoffs::win_delete_;
	}
}

static __declspec(naked) void SkilldexHook() {
	__asm {
		_InLoop(1, SKILLDEX);
		call fo::funcoffs::skilldex_select_;
		_InLoop(0, SKILLDEX);
		retn;
	}
}

static __declspec(naked) void HandleInventoryHook_Start() {
	__asm {
		_InLoop2(1, INVENTORY);
		xor eax, eax;
		jmp fo::funcoffs::inven_set_mouse_;
	}
}

static __declspec(naked) void HandleInventoryHook_End() {
	__asm {
		_InLoop2(0, INVENTORY);
		mov eax, esi;
		jmp fo::funcoffs::exit_inventory_;
	}
}

static __declspec(naked) void UseInventoryOnHook_Start() {
	__asm {
		_InLoop2(1, INTFACEUSE);
		xor eax, eax;
		jmp fo::funcoffs::inven_set_mouse_;
	}
}

static __declspec(naked) void UseInventoryOnHook_End() {
	__asm {
		_InLoop2(0, INTFACEUSE);
		mov eax, edi;
		jmp fo::funcoffs::exit_inventory_;
	}
}

static __declspec(naked) void LootContainerHook_Start() {
	__asm {
		_InLoop2(1, INTFACELOOT);
		xor eax, eax;
		jmp fo::funcoffs::inven_set_mouse_;
	}
}

static __declspec(naked) void LootContainerHook_End() {
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

static __declspec(naked) void BarterInventoryHook() {
	__asm {
		and inLoop, ~SPECIAL; // unset flag after animating the dialog panel
		_InLoop(1, BARTER);
		push [esp + 4];
		call fo::funcoffs::barter_inventory_;
		_InLoop(0, BARTER);
		call ResetBodyState;
		retn 4;
	}
}

static __declspec(naked) void AutomapHook_Start() {
	__asm {
		call fo::funcoffs::gmouse_set_cursor_;
		test edx, edx;
		jnz  skip;
		mov  LoadGameHook::interfaceWID, ebp;
		_InLoop(1, AUTOMAP);
skip:
		retn;
	}
}

static __declspec(naked) void AutomapHook_End() {
	__asm {
		_InLoop(0, AUTOMAP);
		mov LoadGameHook::interfaceWID, -1
		jmp fo::funcoffs::win_delete_;
	}
}

static __declspec(naked) void DialogReviewInitHook() {
	__asm {
		call fo::funcoffs::gdReviewInit_;
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

static __declspec(naked) void DialogReviewExitHook() {
	__asm {
		push ecx;
		push eax;
		_InLoop2(0, DIALOGVIEW);
		pop eax;
		pop ecx;
		jmp fo::funcoffs::gdReviewExit_;
	}
}

static __declspec(naked) void setup_move_timer_win_Hook() {
	__asm {
		_InLoop2(1, COUNTERWIN);
		jmp fo::funcoffs::text_curr_;
	}
}

static __declspec(naked) void exit_move_timer_win_Hook() {
	__asm {
		push eax;
		_InLoop2(0, COUNTERWIN);
		pop  eax;
		jmp  fo::funcoffs::win_delete_;
	}
}

static __declspec(naked) void gdialog_bk_hook() {
	__asm {
		_InLoop2(1, SPECIAL); // set the flag before switching from dialog mode to barter
		jmp fo::funcoffs::gdialog_window_destroy_;
	}
}

static __declspec(naked) void gdialogUpdatePartyStatus_hook1() {
	__asm {
		push edx;
		_InLoop2(1, SPECIAL); // set the flag before animating the dialog panel when a party member joins/leaves
		pop  edx;
		jmp  fo::funcoffs::gdialog_window_destroy_;
	}
}

static __declspec(naked) void gdialogUpdatePartyStatus_hook0() {
	__asm {
		call fo::funcoffs::gdialog_window_create_;
		_InLoop2(0, SPECIAL); // unset the flag when entering the party member control panel
		retn;
	}
}

void LoadGameHook::init() {
	saveInCombatFix = IniReader::GetConfigInt("Misc", "SaveInCombatFix", 1);
	if (saveInCombatFix > 2) saveInCombatFix = 0;

	HookCall(0x482AEC, map_load_hook);
	HookCall(0x4809BA, main_init_system_hook);
	HookCall(0x4426A6, game_init_hook);
	HookCall(0x480AAE, main_load_new_hook);

	HookCalls(LoadGame_hook, {0x443AE4, 0x443B89, 0x480B77, 0x48FD35});
	SafeWrite32(0x5194C0, (DWORD)&EndLoadHook);
	HookCalls(SaveGame_hook, {0x443AAC, 0x443B1C, 0x48FCFF});

	HookCalls(game_reset_hook, {
		0x47DD6B, // LoadSlot_ (on load error)
		0x47DDF3, // LoadSlot_ (on load error)
		//0x480708, // RestoreLoad_ (never called)
		0x480AD3, // gnw_main_ (game ended after playing via New Game)
		0x480BCC, // gnw_main_ (game ended after playing via Load Game)
		//0x480D0C, // main_reset_system_ (never called)
		0x481028, // main_selfrun_record_
		0x481062, // main_selfrun_record_
		0x48110B, // main_selfrun_play_
		0x481145  // main_selfrun_play_
	});
	HookCalls(game_reset_on_load_hook, {
		0x47F491, // PrepLoad_ (the very first step during save game loading)
	});

	HookCalls(before_game_exit_hook, {0x480ACE, 0x480BC7});
	HookCalls(after_game_exit_hook, {0x480AEB, 0x480BE4});
	HookCalls(game_close_hook, {
		0x480CA7,  // gnw_main_
		//0x480D45 // main_exit_system_ (never called)
	});

	/////////////// GAME MODES ///////////////
	HookCall(0x4BFE33, WorldMapHook_Start); // wmTownMap_ (old 0x483668, 0x4A4073)
	HookCall(0x4C2E4F, WorldMapHook_End);   // wmInterfaceExit_ (old 0x4C4855)

	HookCalls(CombatHook, {0x426A29, 0x4432BE, 0x45F6D2, 0x4A4020, 0x4A403D});
	HookCall(0x422B09, PlayerCombatHook);

	HookCall(0x480C16, EscMenuHook);   // gnw_main_
	HookCall(0x4433BE, EscMenuHook2);  // game_handle_input_
	HookCalls(OptionsMenuHook, {0x48FCE4, 0x48FD17, 0x48FD4D, 0x48FD6A, 0x48FD87, 0x48FDB3});
	HookCall(0x443A50, HelpMenuHook);  // game_handle_input_
	HookCall(0x443320, CharacterHook); // 0x4A73EB, 0x4A740A for character creation

	MakeCall(0x445285, DialogHook_Start); // gdialogInitFromScript_ (old 0x445748)
	HookCall(0x445317, DialogHook_End);   // gdialogExitFromScript_

	HookCalls(PipboyHook_Start, {0x49767F, 0x4977EF, 0x49780D}); // StartPipboy_ (old 0x443463, 0x443605)
	HookCall(0x497868, PipboyHook_End); // EndPipboy_

	HookCalls(SkilldexHook, {0x4434AC, 0x44C7BD});

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

	HookCall(0x443482, PauseWindowHook);
}

Delegate<>& LoadGameHook::OnBeforeGameInit() {
	return onBeforeGameInit;
}

Delegate<>& LoadGameHook::OnGameInit() {
	return onGameInit;
}

Delegate<>& LoadGameHook::OnAfterGameInit() {
	return onAfterGameInit;
}

Delegate<>& LoadGameHook::OnGameExit() {
	return onGameExit;
}

Delegate<>& LoadGameHook::OnGameReset() {
	return onGameReset;
}

Delegate<>& LoadGameHook::OnBeforeGameStart() {
	return onBeforeGameStart;
}

Delegate<>& LoadGameHook::OnAfterGameStarted() {
	return onAfterGameStarted;
}

Delegate<>& LoadGameHook::OnAfterNewGame() {
	return onAfterNewGame;
}

Delegate<DWORD>& LoadGameHook::OnGameModeChange() {
	return onGameModeChange;
}

Delegate<>& LoadGameHook::OnBeforeGameClose() {
	return onBeforeGameClose;
}

Delegate<>& LoadGameHook::OnCombatStart() {
	return onCombatStart;
}

Delegate<>& LoadGameHook::OnCombatEnd() {
	return onCombatEnd;
}

Delegate<>& LoadGameHook::OnBeforeMapLoad() {
	return onBeforeMapLoad;
}

}
