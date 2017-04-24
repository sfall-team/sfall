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
#include "FileSystem.h"
#include "PartyControl.h"
#include "Perks.h"
#include "ScriptExtender.h"
#include "Scripting\Arrays.h"
#include "ExtraSaveSlots.h"

#include "LoadGameHook.h"

namespace sfall
{

static Delegate<> onGameInit;
static Delegate<> onGameReset;
static Delegate<> onBeforeGameStart;
static Delegate<> onAfterGameStarted;
static Delegate<> onAfterNewGame;

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
static bool disableHorrigan = false;
static bool pipBoyAvailableAtGameStart = false;
static bool mapLoaded = false;

bool IsMapLoaded() {
	return mapLoaded;
}

DWORD InWorldMap() {
	return (inLoop&WORLDMAP) ? 1 : 0;
}

DWORD InCombat() {
	return (inLoop&COMBAT) ? 1 : 0;
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

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", fo::var::patches, fo::var::slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static std::string saveSfallDataFailMsg;
static void _stdcall SaveGame2() {
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Saving game: %s\n", DL_MAIN, buf);

	DWORD size, unused = 0;
	HANDLE h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		SaveGlobals(h);
		WriteFile(h, &unused, 4, &size, 0);
		unused++;
		WriteFile(h, &unused, 4, &size, 0);
		Perks::save(h);
		script::SaveArrays(h);
		CloseHandle(h);
	} else {
		dlogr("ERROR creating sfallgv!", DL_MAIN);
		fo::DisplayPrint(saveSfallDataFailMsg);
		fo::func::gsound_play_sfx_file("IISXXXX1");
	}
	GetSavePath(buf, "fs");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		FileSystem::save(h);
	}
	CloseHandle(h);
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
		push ebx;
		push ecx;
		push edx;
		push eax; // save Mode parameter
		call CombatSaveTest;
		test eax, eax;
		pop edx; // recall Mode parameter
		jz end;
		mov eax, edx;

		or inLoop, SAVEGAME;
		call fo::funcoffs::SaveGame_;
		and inLoop, (-1^SAVEGAME);
		cmp eax, 1;
		jne end;
		call SaveGame2;
		mov eax, 1;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

// Called right before savegame slot is being loaded
static void _stdcall LoadGame_Before() {
	onBeforeGameStart.invoke();
	
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\n", DL_MAIN, buf);

	HANDLE h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD size, unused[2];
		LoadGlobals(h);
		ReadFile(h, &unused, 8, &size, 0);
		if (size == 8) {
			Perks::load(h);
			script::LoadArrays(h);
		}
		CloseHandle(h);
	} else {
		dlogr("Cannot read sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}
}

// called whenever game is being reset (prior to loading a save or when returning to main menu)
static void _stdcall GameReset(DWORD isGameLoad) {
	onGameReset.invoke();
	inLoop = 0;
	mapLoaded = false;

	if (isGameLoad) {
		LoadGame_Before();
	}
}

// Called after game was loaded from a save
static void _stdcall LoadGame_After() {
	onAfterGameStarted.invoke();
	mapLoaded = true;
}

static void __declspec(naked) LoadGame_hook() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		or inLoop, LOADGAME;
		call fo::funcoffs::LoadGame_;
		and inLoop, (-1^LOADGAME);
		cmp eax, 1;
		jne end;
		call LoadGame_After;
		mov eax, 1;
end:
		pop edx;
		pop ecx;
		pop ebx;
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
		pushad;
		push eax;
		call NewGame_Before;
		pop eax;
		call fo::funcoffs::main_load_new_;
		call NewGame_After;
		popad;
		retn;
	}
}

static void __stdcall GameInitialized() {
	onGameInit.invoke();
}

static void __declspec(naked) main_init_system_hook() {
	__asm {
		pushad;
		call GameInitialized;
		popad;
		jmp fo::funcoffs::main_init_system_;
	}
}

static void __declspec(naked) game_reset_hook() {
	__asm {
		pushad;
		push 0;
		call GameReset; // reset all sfall modules before resetting the game data
		popad;
		jmp fo::funcoffs::game_reset_;
	}
}

static void __declspec(naked) game_reset_on_load_hook() {
	__asm {
		pushad;
		push 1;
		call GameReset; // reset all sfall modules before resetting the game data
		popad;
		jmp fo::funcoffs::game_reset_;
	}
}

static void __declspec(naked) WorldMapHook() {
	__asm {
		or inLoop, WORLDMAP;
		xor eax, eax;
		call fo::funcoffs::wmWorldMapFunc_;
		and inLoop, (-1^WORLDMAP);
		retn;
	}
}

static void __declspec(naked) WorldMapHook2() {
	__asm {
		or inLoop, WORLDMAP;
		call fo::funcoffs::wmWorldMapFunc_;
		and inLoop, (-1^WORLDMAP);
		retn;
	}
}

static void __declspec(naked) CombatHook() {
	__asm {
		pushad;
		call AICombatStart;
		popad
		or inLoop, COMBAT;
		call fo::funcoffs::combat_;
		pushad;
		call AICombatEnd;
		popad
		and inLoop, (-1^COMBAT);
		retn;
	}
}

static void __declspec(naked) PlayerCombatHook() {
	__asm {
		or inLoop, PCOMBAT;
		call fo::funcoffs::combat_input_;
		and inLoop, (-1^PCOMBAT);
		retn;
	}
}

static void __declspec(naked) EscMenuHook() {
	__asm {
		or inLoop, ESCMENU;
		call fo::funcoffs::do_optionsFunc_;
		and inLoop, (-1^ESCMENU);
		retn;
	}
}

static void __declspec(naked) EscMenuHook2() {
	//Bloody stupid watcom compiler optimizations...
	__asm {
		or inLoop, ESCMENU;
		call fo::funcoffs::do_options_;
		and inLoop, (-1^ESCMENU);
		retn;
	}
}

static void __declspec(naked) OptionsMenuHook() {
	__asm {
		or inLoop, OPTIONS;
		call fo::funcoffs::do_prefscreen_;
		and inLoop, (-1^OPTIONS);
		retn;
	}
}

static void __declspec(naked) HelpMenuHook() {
	__asm {
		or inLoop, HELP;
		call fo::funcoffs::game_help_;
		and inLoop, (-1^HELP);
		retn;
	}
}

static void __declspec(naked) CharacterHook() {
	__asm {
		or inLoop, CHARSCREEN;
		pushad;
		call PerksEnterCharScreen;
		popad;
		call fo::funcoffs::editor_design_;
		pushad;
		test eax, eax;
		jz success;
		call PerksCancelCharScreen;
		jmp end;
success:
		call PerksAcceptCharScreen;
end:
		popad;
		and inLoop, (-1^CHARSCREEN);
		retn;
	}
}

static void __declspec(naked) DialogHook() {
	__asm {
		or inLoop, DIALOG;
		call fo::funcoffs::gdProcess_;
		and inLoop, (-1^DIALOG);
		retn;
	}
}

static void __declspec(naked) PipboyHook() {
	__asm {
		or inLoop, PIPBOY;
		call fo::funcoffs::pipboy_;
		and inLoop, (-1^PIPBOY);
		retn;
	}
}

static void __declspec(naked) SkilldexHook() {
	__asm {
		or inLoop, SKILLDEX;
		call fo::funcoffs::skilldex_select_;
		and inLoop, (-1^SKILLDEX);
		retn;
	}
}

static void __declspec(naked) HandleInventoryHook() {
	__asm {
		or inLoop, INVENTORY;
		call fo::funcoffs::handle_inventory_;
		and inLoop, (-1^INVENTORY);
		retn;
	}
}

static void __declspec(naked) UseInventoryOnHook() {
	__asm {
		or inLoop, INTFACEUSE;
		call fo::funcoffs::use_inventory_on_;
		and inLoop, (-1 ^ INTFACEUSE);
		retn;
	}
}

static void __declspec(naked) LootContainerHook() {
	__asm {
		or inLoop, INTFACELOOT;
		call fo::funcoffs::loot_container_;
		and inLoop, (-1 ^ INTFACELOOT);
		retn;
	}
}

static void __declspec(naked) BarterInventoryHook() {
	__asm {
		or inLoop, BARTER;
		push [ESP + 4];
		call fo::funcoffs::barter_inventory_;
		and inLoop, (-1 ^ BARTER);
		retn 4;
	}
}

static void __declspec(naked) AutomapHook() {
	__asm {
		or inLoop, AUTOMAP;
		call fo::funcoffs::automap_;
		and inLoop, (-1 ^ AUTOMAP);
		retn;
	}
}

void LoadGameHook::init() {
	saveInCombatFix = GetConfigInt("Misc", "SaveInCombatFix", 1);
	if (saveInCombatFix > 2) saveInCombatFix = 0;
	saveFailMsg = Translate("sfall", "SaveInCombat", "Cannot save at this time");
	saveSfallDataFailMsg = Translate("sfall", "SaveSfallDataFail", 
		"ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!");

	HookCalls(main_init_system_hook, {0x4809BA});
	HookCalls(main_load_new_hook, {0x480AAE});
	HookCalls(LoadGame_hook, {0x443AE4, 0x443B89, 0x480B77, 0x48FD35});
	HookCalls(SaveGame_hook, {0x443AAC, 0x443B1C, 0x48FCFF});
	HookCalls(game_reset_hook, {
				0x47DD6B, // LoadSlot_ (on error)
				0x47DDF3, // LoadSlot_ (on error)
				0x480708, // RestoreLoad_ (never called)
				0x480AD3, // gnw_main_ (game ended after playing via New Game)
				0x480BCC, // gnw_main_ (game ended after playing via Load Game)
				0x480D0C, // main_reset_system_ (never called)
				0x481028, // main_selfrun_record_
				0x481062, // main_selfrun_record_
				0x48110B, // main_selfrun_play_
				0x481145 // main_selfrun_play_
			}); 
	HookCalls(game_reset_on_load_hook, {
				0x47F491, // PrepLoad_ (the very first step during save game loading)
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
}

Delegate<>& LoadGameHook::OnGameInit() {
	return onGameInit;
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

}
