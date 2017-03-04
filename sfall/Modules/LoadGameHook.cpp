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

Delegate<> LoadGameHook::onGameInit;
Delegate<> LoadGameHook::onGameReset;
Delegate<> LoadGameHook::onBeforeGameStart;
Delegate<> LoadGameHook::onAfterGameStarted;
Delegate<> LoadGameHook::onAfterNewGame;

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
static bool disableHorrigan = false;
static bool pipBoyAvailableAtGameStart = false;
static bool mapLoaded = false;
static bool gameLoaded = false;

bool IsMapLoaded() {
	return mapLoaded;
}

DWORD InWorldMap() {
	return (inLoop&WORLDMAP) ? 1 : 0;
}

DWORD InCombat() {
	return (inLoop&COMBAT) ? 1 : 0;
}

DWORD GetCurrentLoops() {
	return inLoop;
}

static void _stdcall ResetState() {
	inLoop = 0;
	LoadGameHook::onGameReset.invoke();
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", VarPtr::patches, VarPtr::slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static char saveSfallDataFailMsg[128];

static void _stdcall SaveGame2() {
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Saving game: %s\r\n", DL_MAIN, buf);

	DWORD size, unused = 0;
	HANDLE h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		SaveGlobals(h);
		WriteFile(h, &unused, 4, &size, 0);
		unused++;
		WriteFile(h, &unused, 4, &size, 0);
		Perks::save(h);
		SaveArrays(h);
		CloseHandle(h);
	} else {
		dlogr("ERROR creating sfallgv!", DL_MAIN);
		Wrapper::display_print(saveSfallDataFailMsg);
		Wrapper::gsound_play_sfx_file("IISXXXX1");
	}
	GetSavePath(buf, "fs");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		FileSystem::save(h);
	}
	CloseHandle(h);
}

static char saveFailMsg[128];
static DWORD _stdcall CombatSaveTest() {
	if (!saveInCombatFix && !IsNpcControlled()) return 1;
	if (inLoop & COMBAT) {
		if (saveInCombatFix == 2 || IsNpcControlled() || !(inLoop & PCOMBAT)) {
			Wrapper::display_print(saveFailMsg);
			return 0;
		}
		int ap = Wrapper::stat_level(VarPtr::obj_dude, STAT_max_move_points);
		int bonusmove = Wrapper::perk_level(VarPtr::obj_dude, PERK_bonus_move);
		if (VarPtr::obj_dude->critterAP_weaponAmmoPid != ap || bonusmove * 2 != VarPtr::combat_free_move) {
			Wrapper::display_print(saveFailMsg);
			return 0;
		}
	}
	return 1;
}

static void __declspec(naked) SaveGame() {
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
		call FuncOffs::SaveGame_;
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
	ResetState();
	LoadGameHook::onBeforeGameStart.invoke();
	
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\r\n", DL_MAIN, buf);

	HANDLE h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD size, unused[2];
		LoadGlobals(h);
		ReadFile(h, &unused, 8, &size, 0);
		if (size == 8) {
			Perks::load(h);
			LoadArrays(h);
		}
		CloseHandle(h);
	} else {
		dlogr("Cannot read sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}
}

// Called after game was loaded from a save
static void _stdcall LoadGame_After() {
	LoadGameHook::onAfterGameStarted.invoke();
	mapLoaded = true;
}

static void __declspec(naked) LoadSlot() {
	__asm {
		pushad;
		call LoadGame_Before;
		popad;
		call FuncOffs::LoadSlot_;
		retn;
	}
}

static void __declspec(naked) LoadGame() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		or inLoop, LOADGAME;
		call FuncOffs::LoadGame_;
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
	ResetState();
	LoadGameHook::onBeforeGameStart.invoke();
}

static void __stdcall NewGame_After() {	
	LoadGameHook::onAfterNewGame.invoke();
	LoadGameHook::onAfterGameStarted.invoke();

	dlogr("New Game started.", DL_MAIN);
	
	mapLoaded = true;
}

static void __declspec(naked) main_load_new_hook() {
	__asm {
		pushad;
		push eax;
		call NewGame_Before;
		pop eax;
		call FuncOffs::main_load_new_;
		call NewGame_After;
		popad;
		retn;
	}
}

static void __stdcall MainMenuLoop() {
	mapLoaded = false;
	if (!gameLoaded) {
		gameLoaded = true;
		// called only once
		LoadGameHook::onGameInit.invoke();
	}
}

static void __declspec(naked) main_menu_loop_hook() {
	__asm {
		pushad;
		call MainMenuLoop;
		popad;
		call FuncOffs::main_menu_loop_;
		retn;
	}
}


static void __declspec(naked) WorldMapHook() {
	__asm {
		or inLoop, WORLDMAP;
		xor eax, eax;
		call FuncOffs::wmWorldMapFunc_;
		and inLoop, (-1^WORLDMAP);
		retn;
	}
}

static void __declspec(naked) WorldMapHook2() {
	__asm {
		or inLoop, WORLDMAP;
		call FuncOffs::wmWorldMapFunc_;
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
		call FuncOffs::combat_;
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
		call FuncOffs::combat_input_;
		and inLoop, (-1^PCOMBAT);
		retn;
	}
}

static void __declspec(naked) EscMenuHook() {
	__asm {
		or inLoop, ESCMENU;
		call FuncOffs::do_optionsFunc_;
		and inLoop, (-1^ESCMENU);
		retn;
	}
}

static void __declspec(naked) EscMenuHook2() {
	//Bloody stupid watcom compiler optimizations...
	__asm {
		or inLoop, ESCMENU;
		call FuncOffs::do_options_;
		and inLoop, (-1^ESCMENU);
		retn;
	}
}

static void __declspec(naked) OptionsMenuHook() {
	__asm {
		or inLoop, OPTIONS;
		call FuncOffs::do_prefscreen_;
		and inLoop, (-1^OPTIONS);
		retn;
	}
}

static void __declspec(naked) HelpMenuHook() {
	__asm {
		or inLoop, HELP;
		call FuncOffs::game_help_;
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
		call FuncOffs::editor_design_;
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
		call FuncOffs::gdProcess_;
		and inLoop, (-1^DIALOG);
		retn;
	}
}

static void __declspec(naked) PipboyHook() {
	__asm {
		or inLoop, PIPBOY;
		call FuncOffs::pipboy_;
		and inLoop, (-1^PIPBOY);
		retn;
	}
}

static void __declspec(naked) SkilldexHook() {
	__asm {
		or inLoop, SKILLDEX;
		call FuncOffs::skilldex_select_;
		and inLoop, (-1^SKILLDEX);
		retn;
	}
}

static void __declspec(naked) InventoryHook() {
	__asm {
		or inLoop, INVENTORY;
		call FuncOffs::handle_inventory_;
		and inLoop, (-1^INVENTORY);
		retn;
	}
}

static void __declspec(naked) AutomapHook() {
	__asm {
		or inLoop, AUTOMAP;
		call FuncOffs::automap_;
		and inLoop, (-1^AUTOMAP);
		retn;
	}
}

void LoadGameHook::init() {
	saveInCombatFix = GetPrivateProfileInt("Misc", "SaveInCombatFix", 1, ini);
	if (saveInCombatFix > 2) saveInCombatFix = 0;
	GetPrivateProfileString("sfall", "SaveInCombat", "Cannot save at this time", saveFailMsg, 128, translationIni);
	GetPrivateProfileString("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!", saveSfallDataFailMsg, 128, translationIni);

	HookCall(0x480AAE, main_load_new_hook);

	HookCall(0x47C72C, LoadSlot);
	HookCall(0x47D1C9, LoadSlot);
	HookCall(0x443AE4, LoadGame);
	HookCall(0x443B89, LoadGame);
	HookCall(0x480B77, LoadGame);
	HookCall(0x48FD35, LoadGame);
	HookCall(0x443AAC, SaveGame);
	HookCall(0x443B1C, SaveGame);
	HookCall(0x48FCFF, SaveGame);
	HookCall(0x480A28, main_menu_loop_hook);

	HookCall(0x483668, WorldMapHook);
	HookCall(0x4A4073, WorldMapHook);
	HookCall(0x4C4855, WorldMapHook2);
	HookCall(0x426A29, CombatHook);
	HookCall(0x4432BE, CombatHook);
	HookCall(0x45F6D2, CombatHook);
	HookCall(0x4A4020, CombatHook);
	HookCall(0x4A403D, CombatHook);
	HookCall(0x422B09, PlayerCombatHook);
	HookCall(0x480C16, EscMenuHook);
	HookCall(0x4433BE, EscMenuHook2);
	HookCall(0x48FCE4, OptionsMenuHook);
	HookCall(0x48FD17, OptionsMenuHook);
	HookCall(0x48FD4D, OptionsMenuHook);
	HookCall(0x48FD6A, OptionsMenuHook);
	HookCall(0x48FD87, OptionsMenuHook);
	HookCall(0x48FDB3, OptionsMenuHook);
	HookCall(0x443A50, HelpMenuHook);
	HookCall(0x443320, CharacterHook);
	HookCall(0x4A73EB, CharacterHook);
	HookCall(0x4A740A, CharacterHook);
	HookCall(0x445748, DialogHook);
	HookCall(0x443463, PipboyHook);
	HookCall(0x443605, PipboyHook);
	HookCall(0x4434AC, SkilldexHook);
	HookCall(0x44C7BD, SkilldexHook);
	HookCall(0x443357, InventoryHook);
	HookCall(0x44396D, AutomapHook);
	HookCall(0x479519, AutomapHook);
}
