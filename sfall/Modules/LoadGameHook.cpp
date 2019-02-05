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

#include "..\main.h"
#include "..\Delegate.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\version.h"

#include "AI.h"
#include "BugFixes.h"
#include "FileSystem.h"
#include "HeroAppearance.h"
#include "HookScripts.h"
#include "Objects.h"
#include "PartyControl.h"
#include "Perks.h"
#include "ScriptExtender.h"
#include "Scripting\Arrays.h"
#include "ExtraSaveSlots.h"
#include "Worldmap.h"

#include "LoadGameHook.h"

namespace sfall
{

#define _InLoop2(type, flag) __asm { \
	_asm push flag \
	_asm push type \
	_asm call SetInLoop }
#define _InLoop(type, flag) __asm { \
	pushadc              \
	_InLoop2(type, flag) \
	popadc }

static Delegate<> onGameInit;
static Delegate<> onGameExit;
static Delegate<> onGameReset;
static Delegate<> onBeforeGameStart;
static Delegate<> onAfterGameStarted;
static Delegate<> onAfterNewGame;
static Delegate<DWORD> onGameModeChange;
static Delegate<> onBeforeGameClose;

DWORD LoadGameHook::LootTarget = 0;

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
static bool disableHorrigan = false;
static bool pipBoyAvailableAtGameStart = false;
static bool mapLoaded = false;

bool IsMapLoaded() {
	return mapLoaded;
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

void _stdcall SetInLoop(DWORD mode, LoopFlag flag) {
	if (mode) {
		SetLoopFlag(flag);
	} else {
		ClearLoopFlag(flag);
	}
	HookScripts::GameModeChangeHook(0);
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", fo::var::patches, fo::var::slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static std::string saveSfallDataFailMsg;
static void _stdcall SaveGame2() {
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
		Perks::save(h);
		script::SaveArrays(h);
		CloseHandle(h);
	} else {
		goto errorSave;
	}

	GetSavePath(buf, "db");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		Worldmap::SaveData(h);
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
	fo::DisplayPrint(saveSfallDataFailMsg);
	fo::func::gsound_play_sfx_file("IISXXXX1");
}

static std::string saveFailMsg;
static DWORD _stdcall CombatSaveTest() {
	if (!saveInCombatFix && !PartyControl::IsNpcControlled()) return 1;
	if (inLoop & COMBAT) {
		if (saveInCombatFix == 2 || PartyControl::IsNpcControlled() || !(inLoop & PCOMBAT)) {
			fo::DisplayPrint(saveFailMsg);
			return 0;
		}
		int ap = fo::func::stat_level(fo::var::obj_dude, fo::STAT_max_move_points);
		int bonusmove = fo::func::perk_level(fo::var::obj_dude, fo::PERK_bonus_move);
		if (fo::var::obj_dude->critter.movePoints != ap || bonusmove * 2 != fo::var::combat_free_move) {
			fo::DisplayPrint(saveFailMsg);
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
		if (size != 4 || !Perks::load(h) || script::LoadArrays(h)) goto errorLoad;
		CloseHandle(h);
	} else {
		dlogr("Cannot open sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}

	GetSavePath(buf, "db");
	dlogr("Loading data from sfalldb.sav...", DL_MAIN);
	h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		if (Worldmap::LoadData(h)) goto errorLoad;
		CloseHandle(h);
	} else {
		dlogr("Cannot open sfalldb.sav.", DL_MAIN);
	}

	return false;
/////////////////////////////////////////////////
errorLoad:
	CloseHandle(h);
	dlog_f("ERROR reading data: %s\n", DL_MAIN, buf);
	return (true & !isDebug);
}

// called whenever game is being reset (prior to loading a save or when returning to main menu)
static bool _stdcall GameReset(DWORD isGameLoad) {
	if (mapLoaded) { // prevent resetting when a new game has not been started (loading saved game from main menu)
		onGameReset.invoke();
		if (isDebug) {
			char* str = (isGameLoad) ? "on Load" : "on Exit";
			fo::func::debug_printf("\n[SFALL: State reset %s]\n", str);
		}
	}
	inLoop = 0;
	mapLoaded = false;

	return (isGameLoad) ? LoadGame_Before() : false;
}

// Called after game was loaded from a save
static void _stdcall LoadGame_After() {
	onAfterGameStarted.invoke();
	mapLoaded = true;
}

static void __declspec(naked) LoadGame_hook() {
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

static void __declspec(naked) EndLoadHook() {
	__asm {
		call fo::funcoffs::EndLoad_;
		pushadc;
		call LoadHeroAppearance;
		popadc;
		retn;
	}
}

static void __stdcall NewGame_Before() {
	onBeforeGameStart.invoke();
}

static void __stdcall NewGame_After() {
	onAfterNewGame.invoke();
	onAfterGameStarted.invoke();

	dlogr("New Game started.", DL_MAIN);

	mapLoaded = true;
}

static void __declspec(naked) main_load_new_hook() {
	__asm {
		push eax;
		call NewGame_Before;
		pop  eax;
		call fo::funcoffs::main_load_new_;
		jmp  NewGame_After;
		//retn;
	}
}

static void __stdcall GameInitialized() {
	onGameInit.invoke();
}

static void __stdcall GameExit() {
	onGameExit.invoke();
}

static void __stdcall GameClose() {
	onBeforeGameClose.invoke();
}

static void __declspec(naked) main_init_system_hook() {
	__asm {
		pushadc;
		call GameInitialized;
		popadc;
		jmp fo::funcoffs::main_init_system_;
	}
}

static void __declspec(naked) game_reset_hook() {
	__asm {
		pushadc;
		push 0;
		call GameReset; // reset all sfall modules before resetting the game data
		popadc;
		jmp fo::funcoffs::game_reset_;
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
		jmp fo::funcoffs::game_reset_;
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
		jmp fo::funcoffs::map_exit_;
	}
}

static void __declspec(naked) after_game_exit_hook() {
	__asm {
		pushadc;
		call GameExit;
		popadc;
		jmp fo::funcoffs::main_menu_create_;
	}
}

static void __declspec(naked) game_close_hook() {
	__asm {
		pushadc;
		call GameClose;
		popadc;
		jmp fo::funcoffs::game_exit_;
	}
}

static void __declspec(naked) WorldMapHook() {
	__asm {
		_InLoop(1, WORLDMAP);
		xor  eax, eax;
		call fo::funcoffs::wmWorldMapFunc_;
		_InLoop(0, WORLDMAP);
		retn;
	}
}

static void __declspec(naked) WorldMapHook2() {
	__asm {
		_InLoop(1, WORLDMAP);
		call fo::funcoffs::wmWorldMapFunc_;
		_InLoop(0, WORLDMAP);
		retn;
	}
}

static void __declspec(naked) CombatHook() {
	__asm {
		pushadc;
		call AICombatStart;
		_InLoop2(1, COMBAT);
		popadc;
		call fo::funcoffs::combat_;
		pushadc;
		call AICombatEnd;
		_InLoop2(0, COMBAT);
		popadc;
		retn;
	}
}

static void __declspec(naked) PlayerCombatHook() {
	__asm {
		_InLoop(1, PCOMBAT);
		call fo::funcoffs::combat_input_;
		_InLoop(0, PCOMBAT);
		retn;
	}
}

static void __declspec(naked) EscMenuHook() {
	__asm {
		_InLoop(1, ESCMENU);
		call fo::funcoffs::do_optionsFunc_;
		_InLoop(0, ESCMENU);
		retn;
	}
}

static void __declspec(naked) EscMenuHook2() {
	//Bloody stupid watcom compiler optimizations...
	__asm {
		_InLoop(1, ESCMENU);
		call fo::funcoffs::do_options_;
		_InLoop(0, ESCMENU);
		retn;
	}
}

static void __declspec(naked) OptionsMenuHook() {
	__asm {
		_InLoop(1, OPTIONS);
		call fo::funcoffs::do_prefscreen_;
		_InLoop(0, OPTIONS);
		retn;
	}
}

static void __declspec(naked) HelpMenuHook() {
	__asm {
		_InLoop(1, HELP);
		call fo::funcoffs::game_help_;
		_InLoop(0, HELP);
		retn;
	}
}

static void __declspec(naked) CharacterHook() {
	__asm {
		pushadc;
		_InLoop2(1, CHARSCREEN);
		call PerksEnterCharScreen;
		popadc;
		call fo::funcoffs::editor_design_;
		pushadc;
		test eax, eax;
		jz success;
		call PerksCancelCharScreen;
		jmp end;
success:
		call PerksAcceptCharScreen;
end:
		_InLoop2(0, CHARSCREEN);
		mov tagSkill4LevelBase, -1; // for fixing Tag! perk exploit
		popadc;
		retn;
	}
}

static void __declspec(naked) DialogHook() {
	__asm {
		_InLoop(1, DIALOG);
		call fo::funcoffs::gdProcess_;
		_InLoop(0, DIALOG);
		retn;
	}
}

static void __declspec(naked) PipboyHook() {
	__asm {
		_InLoop(1, PIPBOY);
		call fo::funcoffs::pipboy_;
		_InLoop(0, PIPBOY);
		retn;
	}
}

static void __declspec(naked) SkilldexHook() {
	__asm {
		_InLoop(1, SKILLDEX);
		call fo::funcoffs::skilldex_select_;
		_InLoop(0, SKILLDEX);
		retn;
	}
}

static void __declspec(naked) HandleInventoryHook() {
	__asm {
		_InLoop(1, INVENTORY);
		call fo::funcoffs::handle_inventory_;
		_InLoop(0, INVENTORY);
		retn;
	}
}

static void __declspec(naked) UseInventoryOnHook() {
	__asm {
		_InLoop(1, INTFACEUSE);
		call fo::funcoffs::use_inventory_on_;
		_InLoop(0, INTFACEUSE);
		retn;
	}
}

static void __declspec(naked) LootContainerHook() {
	__asm {
		mov  LoadGameHook::LootTarget, edx;
		_InLoop(1, INTFACELOOT);
		call fo::funcoffs::loot_container_;
		_InLoop(0, INTFACELOOT);
		jmp  ResetBodyState; // reset object pointer used in calculating the weight/size of equipped and hidden items on NPCs after exiting loot/barter screens
		//retn;
	}
}

static void __declspec(naked) BarterInventoryHook() {
	__asm {
		_InLoop(1, BARTER);
		push [esp + 4];
		call fo::funcoffs::barter_inventory_;
		_InLoop(0, BARTER);
		call ResetBodyState;
		retn 4;
	}
}

static void __declspec(naked) AutomapHook() {
	__asm {
		_InLoop(1, AUTOMAP);
		call fo::funcoffs::automap_;
		_InLoop(0, AUTOMAP);
		retn;
	}
}

static void __declspec(naked) DialogReviewInitHook() {
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

static void __declspec(naked) DialogReviewExitHook() {
	__asm {
		push ecx;
		push eax;
		_InLoop2(0, DIALOGVIEW);
		pop eax;
		pop ecx;
		jmp fo::funcoffs::gdReviewExit_;
	}
}

void LoadGameHook::init() {
	saveInCombatFix = GetConfigInt("Misc", "SaveInCombatFix", 1);
	if (saveInCombatFix > 2) saveInCombatFix = 0;
	saveFailMsg = Translate("sfall", "SaveInCombat", "Cannot save at this time.");
	saveSfallDataFailMsg = Translate("sfall", "SaveSfallDataFail",
		"ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!");

	HookCalls(main_init_system_hook, {0x4809BA});
	HookCalls(main_load_new_hook, {0x480AAE});
	HookCalls(LoadGame_hook, {0x443AE4, 0x443B89, 0x480B77, 0x48FD35});
	SafeWrite32(0x5194C0, (DWORD)&EndLoadHook);
	HookCalls(SaveGame_hook, {0x443AAC, 0x443B1C, 0x48FCFF});
	HookCalls(game_reset_hook, {
				0x47DD6B, // LoadSlot_ (on error)
				0x47DDF3, // LoadSlot_ (on error)
				//0x480708, // RestoreLoad_ (never called)
				0x480AD3, // gnw_main_ (game ended after playing via New Game)
				0x480BCC, // gnw_main_ (game ended after playing via Load Game)
				//0x480D0C, // main_reset_system_ (never called)
				0x481028, // main_selfrun_record_
				0x481062, // main_selfrun_record_
				0x48110B, // main_selfrun_play_
				0x481145 // main_selfrun_play_
			});
	HookCalls(game_reset_on_load_hook, {
				0x47F491, // PrepLoad_ (the very first step during save game loading)
			});
	HookCalls(before_game_exit_hook, {0x480ACE, 0x480BC7});
	HookCalls(after_game_exit_hook, {0x480AEB, 0x480BE4});
	HookCalls(game_close_hook, {
				0x480CA7, // gnw_main_
				//0x480D45 // main_exit_system_ (never called)
			});

	HookCalls(WorldMapHook, {0x483668, 0x4A4073});
	HookCalls(WorldMapHook2, {0x4C4855});
	HookCalls(CombatHook, {0x426A29, 0x4432BE, 0x45F6D2, 0x4A4020, 0x4A403D});
	HookCalls(PlayerCombatHook, {0x422B09});
	HookCalls(EscMenuHook, {0x480C16});
	HookCalls(EscMenuHook2, {0x4433BE});
	HookCalls(OptionsMenuHook, {0x48FCE4, 0x48FD17, 0x48FD4D, 0x48FD6A, 0x48FD87, 0x48FDB3});
	HookCalls(HelpMenuHook, {0x443A50});
	HookCalls(CharacterHook, {0x443320, 0x4A73EB, 0x4A740A});
	HookCalls(DialogHook, {0x445748});
	HookCalls(PipboyHook, {0x443463, 0x443605});
	HookCalls(SkilldexHook, {0x4434AC, 0x44C7BD});
	HookCalls(HandleInventoryHook, {0x443357});
	HookCalls(UseInventoryOnHook, {0x44C6FB});
	HookCalls(LootContainerHook, {
			0x4746EC,
			0x4A4369,
			0x4A4565});
	HookCalls(BarterInventoryHook, {0x4466C7});
	HookCalls(AutomapHook, {0x44396D, 0x479519});
	HookCall(0x445CA7, DialogReviewInitHook);
	HookCall(0x445D30, DialogReviewExitHook);
}

Delegate<>& LoadGameHook::OnGameInit() {
	return onGameInit;
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

}
