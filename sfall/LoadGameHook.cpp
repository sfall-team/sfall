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

#include "AI.h"
#include "Arrays.h"
#include "Bugs.h"
#include "Console.h"
#include "Criticals.h"
#include "Explosions.h"
#include "FalloutEngine.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "input.h"
#include "Inventory.h"
#include "Knockback.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "Message.h"
#include "movies.h"
#include "PartyControl.h"
#include "perks.h"
#include "ScriptExtender.h"
#include "skills.h"
#include "sound.h"
#include "SuperSave.h"
#include "version.h"

#define MAX_GLOBAL_SIZE (MaxGlobalVars * 12 + 4)

static DWORD InLoop = 0;
static DWORD SaveInCombatFix;
static bool mapLoaded = false;

bool IsMapLoaded() {
	return mapLoaded;
}

DWORD InWorldMap() {
	return (InLoop & WORLDMAP) ? 1 : 0;
}

DWORD InCombat() {
	return (InLoop & COMBAT) ? 1 : 0;
}

DWORD GetCurrentLoops() {
	return InLoop;
}

static void _stdcall ResetState(DWORD onLoad) {
	if (!onLoad) FileSystemReset();
	ClearGlobalScripts();
	ClearGlobals();
	ForceGraphicsRefresh(0);
	WipeSounds();
	if (GraphicsMode > 3) GraphicsResetOnGameLoad();
	Knockback_OnGameLoad();
	Skills_OnGameLoad();
	InLoop = 0;
	PerksReset();
	InventoryReset();
	RegAnimCombatCheck(1);
	AfterAttackCleanup();
	ResetExplosionRadius();
	PartyControlReset();
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", *(char**)_patches, *(DWORD*)_slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static char SaveSfallDataFailMsg[128];

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
		PerksSave(h);
		SaveArrays(h);
		CloseHandle(h);
	} else {
		dlogr("ERROR creating sfallgv!", DL_MAIN);
		DisplayConsoleMessage(SaveSfallDataFailMsg);
		PlaySfx("IISXXXX1");
	}

	if (!UsingFileSystem) return;
	GetSavePath(buf, "fs");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		FileSystemSave(h);
		CloseHandle(h);
	}
}

static char SaveFailMsg[128];
static DWORD _stdcall combatSaveTest() {
	if (!SaveInCombatFix && !IsNpcControlled()) return 1;
	if (InLoop & COMBAT) {
		if (SaveInCombatFix == 2 || IsNpcControlled() || !(InLoop & PCOMBAT)) {
			DisplayConsoleMessage(SaveFailMsg);
			return 0;
		}
		DWORD ap;
		DWORD bonusmove;
		__asm {
			mov edx, 8;
			mov eax, ds:[_obj_dude];
			call stat_level_;
			mov ap, eax;
			mov eax, ds:[_obj_dude];
			mov edx, 3;
			call perk_level_;
			mov bonusmove, eax;
		}
		if (*(DWORD*)(*(DWORD*)_obj_dude + 0x40) != ap || bonusmove * 2 != *(DWORD*)_combat_free_move) {
			DisplayConsoleMessage(SaveFailMsg);
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
		call combatSaveTest;
		test eax, eax;
		pop edx; // recall Mode parameter
		jz end;
		mov eax, edx;

		or InLoop, SAVEGAME;
		call SaveGame_;
		and InLoop, (-1 ^ SAVEGAME);
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

// should be called before savegame is loaded
static void _stdcall LoadGame2_Before() {
	ResetState(1);

	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\n", DL_MAIN, buf);

	ClearGlobals();
	HANDLE h = CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD size, unused[2];
		LoadGlobals(h);
		ReadFile(h, &unused, 8, &size, 0);
		if (size == 8) {
			PerksLoad(h);
			LoadArrays(h);
		}
		CloseHandle(h);
	} else {
		dlogr("Cannot read sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}
}

static void _stdcall LoadGame2_After() {
	LoadGlobalScripts();
	CritLoad();
	mapLoaded = true;
}

static void __declspec(naked) LoadSlot() {
	__asm {
		pushad;
		call LoadGame2_Before;
		popad;
		call LoadSlot_;
		retn;
	}
}

static void __declspec(naked) LoadGame() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		or InLoop, LOADGAME;
		call LoadGame_;
		and InLoop, (-1 ^ LOADGAME);
		cmp eax, 1;
		jne end;
		call LoadGame2_After;
		mov eax, 1;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) EndLoadHook() {
	__asm {
		call EndLoad_;
		pushad;
		call LoadHeroAppearance;
		popad;
		retn;
	}
}

static void _stdcall GameInitialized() {
	GameInitialization();
}

static void __declspec(naked) GameInitHook() {
	__asm {
		pushad;
		call GameInitialized;
		popad;
		jmp main_init_system_;
	}
}

static void NewGame2() {
	ResetState(0);

	dlogr("Starting new game", DL_MAIN);

	SetNewCharAppearanceGlobals();

	LoadGlobalScripts();
	CritLoad();
	LoadHeroAppearance();
	mapLoaded = true;
}

static bool DisableHorrigan = false;
static void __declspec(naked) NewGame() {
	__asm {
		pushad;
		call NewGame2;
		mov  al, DisableHorrigan;
		mov  byte ptr ds:[_Meet_Frank_Horrigan], al;
		popad;
		call main_game_loop_;
		retn;
	}
}

static void ReadExtraGameMsgFilesIfNeeded() {
	if (gExtraGameMsgLists.empty())
		ReadExtraGameMsgFiles();
}

static bool PipBoyAvailableAtGameStart = false;
static void __declspec(naked) MainMenuHook() {
	__asm {
		pushad;
		push 0;
		call ResetState;
		mov  al, PipBoyAvailableAtGameStart;
		mov  byte ptr ds:[_gmovie_played_list + 0x3], al;
		call ReadExtraGameMsgFilesIfNeeded;
		popad;
		call main_menu_loop_;
		retn;
	}
}

static void _stdcall GameClose() {
	WipeSounds();
}

static void __declspec(naked) GameCloseHook() {
	__asm {
		pushad;
		call GameClose;
		popad;
		jmp game_exit_;
	}
}

static void __declspec(naked) WorldMapHook() {
	__asm {
		or InLoop, WORLDMAP;
		xor eax, eax;
		call wmWorldMapFunc_;
		and InLoop, (-1 ^ WORLDMAP);
		retn;
	}
}

static void __declspec(naked) WorldMapHook2() {
	__asm {
		or InLoop, WORLDMAP;
		call wmWorldMapFunc_;
		and InLoop, (-1 ^ WORLDMAP);
		retn;
	}
}

static void __declspec(naked) CombatHook() {
	__asm {
		pushad;
		call AICombatStart;
		popad
		or InLoop, COMBAT;
		call combat_;
		pushad;
		call AICombatEnd;
		popad
		and InLoop, (-1 ^ COMBAT);
		retn;
	}
}

static void __declspec(naked) PlayerCombatHook() {
	__asm {
		or InLoop, PCOMBAT;
		call combat_input_;
		and InLoop, (-1 ^ PCOMBAT);
		retn;
	}
}

static void __declspec(naked) EscMenuHook() {
	__asm {
		or InLoop, ESCMENU;
		call do_optionsFunc_;
		and InLoop, (-1 ^ ESCMENU);
		retn;
	}
}

static void __declspec(naked) EscMenuHook2() {
	//Bloody stupid watcom compiler optimizations...
	__asm {
		or InLoop, ESCMENU;
		call do_options_;
		and InLoop, (-1 ^ ESCMENU);
		retn;
	}
}

static void __declspec(naked) OptionsMenuHook() {
	__asm {
		or InLoop, OPTIONS;
		call do_prefscreen_;
		and InLoop, (-1 ^ OPTIONS);
		retn;
	}
}

static void __declspec(naked) HelpMenuHook() {
	__asm {
		or InLoop, HELP;
		call game_help_;
		and InLoop, (-1 ^ HELP);
		retn;
	}
}

static void __declspec(naked) CharacterHook() {
	__asm {
		or InLoop, CHARSCREEN;
		pushad;
		call PerksEnterCharScreen;
		popad;
		call editor_design_;
		pushad;
		test eax, eax;
		jz success;
		call PerksCancelCharScreen;
		jmp end;
success:
		call PerksAcceptCharScreen;
end:
		popad;
		and InLoop, (-1 ^ CHARSCREEN);
		mov tagSkill4LevelBase, -1; // for fixing Tag! perk exploit
		retn;
	}
}

static void __declspec(naked) DialogHook() {
	__asm {
		or InLoop, DIALOG;
		call gdProcess_;
		and InLoop, (-1 ^ DIALOG);
		retn;
	}
}

static void __declspec(naked) PipboyHook() {
	__asm {
		or InLoop, PIPBOY;
		call pipboy_;
		and InLoop, (-1 ^ PIPBOY);
		retn;
	}
}

static void __declspec(naked) SkilldexHook() {
	__asm {
		or InLoop, SKILLDEX;
		call skilldex_select_;
		and InLoop, (-1 ^ SKILLDEX);
		retn;
	}
}

static void __declspec(naked) HandleInventoryHook() {
	__asm {
		or InLoop, INVENTORY;
		call handle_inventory_;
		and InLoop, (-1 ^ INVENTORY);
		retn;
	}
}

static void __declspec(naked) UseInventoryOnHook() {
	__asm {
		or InLoop, INTFACEUSE;
		call use_inventory_on_;
		and InLoop, (-1 ^ INTFACEUSE);
		retn;
	}
}

static void __declspec(naked) LootContainerHook() {
	__asm {
		or InLoop, INTFACELOOT;
		call loot_container_;
		and InLoop, (-1 ^ INTFACELOOT);
		jmp  ResetBodyState; // reset object pointer used in calculating the weight/size of equipped and hidden items on NPCs after exiting loot/barter screens
		//retn;
	}
}

static void __declspec(naked) BarterInventoryHook() {
	__asm {
		or InLoop, BARTER;
		push [esp + 4];
		call barter_inventory_;
		and InLoop, (-1 ^ BARTER);
		call ResetBodyState;
		retn 4;
	}
}

static void __declspec(naked) AutomapHook() {
	__asm {
		or InLoop, AUTOMAP;
		call automap_;
		and InLoop, (-1 ^ AUTOMAP);
		retn;
	}
}

void LoadGameHookInit() {
	SaveInCombatFix = GetPrivateProfileInt("Misc", "SaveInCombatFix", 1, ini);
	if (SaveInCombatFix > 2) SaveInCombatFix = 0;
	GetPrivateProfileString("sfall", "SaveInCombat", "Cannot save at this time.", SaveFailMsg, 128, translationIni);
	GetPrivateProfileString("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!", SaveSfallDataFailMsg, 128, translationIni);

	switch (GetPrivateProfileInt("Misc", "PipBoyAvailableAtGameStart", 0, ini)) {
	case 1:
		PipBoyAvailableAtGameStart = true;
		break;
	case 2:
		SafeWrite8(0x497011, 0xEB); // skip the vault suit movie check
		break;
	}

	if (GetPrivateProfileInt("Misc", "DisableHorrigan", 0, ini)) {
		DisableHorrigan = true;
		SafeWrite8(0x4C06D8, 0xEB); // skip the Horrigan encounter check
	}

	HookCall(0x4809BA, GameInitHook); // 4.x backporting
	HookCall(0x480AB3, NewGame);

	HookCall(0x47C72C, LoadSlot);
	HookCall(0x47D1C9, LoadSlot);
	HookCall(0x443AE4, LoadGame);
	HookCall(0x443B89, LoadGame);
	HookCall(0x480B77, LoadGame);
	HookCall(0x48FD35, LoadGame);
	SafeWrite32(0x5194C0, (DWORD)&EndLoadHook);
	HookCall(0x443AAC, SaveGame);
	HookCall(0x443B1C, SaveGame);
	HookCall(0x48FCFF, SaveGame);

	HookCall(0x480A28, MainMenuHook);
	// 4.x backporting
	HookCall(0x480CA7, GameCloseHook); // gnw_main_
	//HookCall(0x480D45, GameCloseHook); // main_exit_system_ (never called)

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
	HookCall(0x443357, HandleInventoryHook);
	HookCall(0x44C6FB, UseInventoryOnHook);
	HookCall(0x4746EC, LootContainerHook);
	HookCall(0x4A4369, LootContainerHook);
	HookCall(0x4A4565, LootContainerHook);
	HookCall(0x4466C7, BarterInventoryHook);
	HookCall(0x44396D, AutomapHook);
	HookCall(0x479519, AutomapHook);
}
