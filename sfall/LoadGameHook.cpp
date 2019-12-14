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

#include "Define.h"
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
#include "input.h"
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

DWORD LoadGameHook_LootTarget = 0;

static DWORD inLoop = 0;
static DWORD saveInCombatFix;
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

static void _stdcall ResetState(DWORD onLoad) {
	if (!onLoad) FileSystemReset();
	ClearGlobalScripts();
	ClearGlobals();
	ForceGraphicsRefresh(0); // disable refresh
	WipeSounds();
	if (GraphicsMode > 3) GraphicsResetOnGameLoad();
	StatsReset();
	StatsResetMiscValues();
	PerksReset();
	Combat_OnGameLoad();
	Skills_OnGameLoad();
	inLoop = 0;
	ClearSavPrototypes();
	InventoryReset();
	RegAnimCombatCheck(1);
	ForceEncounterRestore(); // restore if the encounter did not happen
	ResetExplosionRadius();
	PartyControlReset();
	NpcEngineLevelUpReset();
	RestoreObjUnjamAllLocks();
	ResetBoxes();
	ResetBoxCount();
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", *(char**)_patches, *ptr_slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static char SaveSfallDataFailMsg[128];
static void _stdcall SaveGame2() {
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
	DisplayConsoleMessage(SaveSfallDataFailMsg);
	PlaySfx("IISXXXX1");
}

static char SaveFailMsg[128];
static DWORD _stdcall CombatSaveTest() {
	if (!saveInCombatFix && !IsNpcControlled()) return 1;
	if (inLoop & COMBAT) {
		if (saveInCombatFix == 2 || IsNpcControlled() || !(inLoop & PCOMBAT)) {
			DisplayConsoleMessage(SaveFailMsg);
			return 0;
		}
		int ap = StatLevel(*ptr_obj_dude, STAT_max_move_points);
		int bonusmove = PerkLevel(*ptr_obj_dude, PERK_bonus_move);
		if (*(DWORD*)(*(DWORD*)_obj_dude + 0x40) != ap || bonusmove * 2 != *ptr_combat_free_move) {
			DisplayConsoleMessage(SaveFailMsg);
			return 0;
		}
	}
	return 1;
}

static void __declspec(naked) SaveGame() {
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
static bool _stdcall LoadGame2_Before() {
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

static void _stdcall LoadGame2_After() {
	CritLoad();
	LoadGlobalScripts();
	mapLoaded = true;
}

static void __declspec(naked) LoadSlot() {
	__asm {
		pushad;
		call LoadGame2_Before;
		test al, al;
		popad;
		jnz  errorLoad;
		jmp  LoadSlot_;
errorLoad:
		mov  eax, -1;
		retn;
	}
}

static void __declspec(naked) LoadGame() {
	__asm {
		push ecx;
		push edx;
		or inLoop, LOADGAME;
		call LoadGame_;
		and inLoop, (-1 ^ LOADGAME);
		cmp  eax, 1;
		jne  end;
		call LoadGame2_After;
		mov  eax, 1;
end:
		pop  edx;
		pop  ecx;
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
		jmp  main_game_loop_;
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

static void __declspec(naked) WorldMapHook() {
	__asm {
		or inLoop, WORLDMAP;
		xor  eax, eax; // unused
		call wmWorldMapFunc_;
		and inLoop, (-1 ^ WORLDMAP);
		retn;
	}
}

static void __declspec(naked) WorldMapHook2() {
	__asm {
		or inLoop, WORLDMAP;
		call wmWorldMapFunc_;
		and inLoop, (-1 ^ WORLDMAP);
		retn;
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
	//Bloody stupid watcom compiler optimizations...
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
		pushadc;
		or inLoop, CHARSCREEN;
		call PerksEnterCharScreen;
		popadc;
		call editor_design_;
		pushadc;
		test eax, eax;
		jz success;
		call PerksCancelCharScreen;
		jmp end;
success:
		call PerksAcceptCharScreen;
end:
		and inLoop, (-1 ^ CHARSCREEN);
		mov tagSkill4LevelBase, -1; // for fixing Tag! perk exploit
		popadc;
		retn;
	}
}

static void __declspec(naked) DialogHook() {
	__asm {
		test inLoop, DIALOG; // check byte flag
		jz   changeMode;
		jmp  gdProcess_;
changeMode:
		or inLoop, DIALOG;
		call gdProcess_;
		and inLoop, (-1 ^ DIALOG);
		retn;
	}
}

static void __declspec(naked) PipboyHook() {
	__asm {
		or inLoop, PIPBOY;
		call pipboy_;
		and inLoop, (-1 ^ PIPBOY);
		retn;
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

static void __declspec(naked) HandleInventoryHook() {
	__asm {
		or inLoop, INVENTORY;
		call handle_inventory_;
		and inLoop, (-1 ^ INVENTORY);
		retn;
	}
}

static void __declspec(naked) UseInventoryOnHook() {
	__asm {
		or inLoop, INTFACEUSE;
		call use_inventory_on_;
		and inLoop, (-1 ^ INTFACEUSE);
		retn;
	}
}

static void __declspec(naked) LootContainerHook() {
	__asm {
		mov  LoadGameHook_LootTarget, edx;
		or inLoop, INTFACELOOT;
		call loot_container_;
		and inLoop, (-1 ^ INTFACELOOT);
		jmp  ResetBodyState; // reset object pointer used in calculating the weight/size of equipped and hidden items on NPCs after exiting loot/barter screens
		//retn;
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
	Translate("sfall", "SaveInCombat", "Cannot save at this time.", SaveFailMsg);
	Translate("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!", SaveSfallDataFailMsg);

	switch (GetConfigInt("Misc", "PipBoyAvailableAtGameStart", 0)) {
	case 1:
		PipBoyAvailableAtGameStart = true;
		break;
	case 2:
		SafeWrite8(0x497011, 0xEB); // skip the vault suit movie check
		break;
	}

	if (GetConfigInt("Misc", "DisableHorrigan", 0)) {
		DisableHorrigan = true;
		SafeWrite8(0x4C06D8, 0xEB); // skip the Horrigan encounter check
	}

	// 4.x backport
	HookCall(0x4809BA, main_init_system_hook);
	HookCall(0x4426A6, game_init_hook);

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
	// 4.x backport
	HookCall(0x480CA7, game_close_hook); // gnw_main_
	//HookCall(0x480D45, game_close_hook); // main_exit_system_ (never called)

	// game modes
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
	//HookCall(0x4A73EB, CharacterHook); // character creation
	//HookCall(0x4A740A, CharacterHook); // character creation
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
	HookCall(0x445CA7, DialogReviewInitHook);
	HookCall(0x445D30, DialogReviewExitHook);
	HookCall(0x476AC6, setup_move_timer_win_Hook); // before init win
	HookCall(0x477067, exit_move_timer_win_Hook);
}
