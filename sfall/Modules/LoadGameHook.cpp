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

#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\version.h"

#include "AI.h"
#include "Console.h"
#include "Criticals.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "Inventory.h"
#include "Knockback.h"
#include "LoadGameHook.h"
#include "Message.h"
#include "Movies.h"
#include "PartyControl.h"
#include "Perks.h"
#include "ScriptExtender.h"
#include "Scripting\Arrays.h"
#include "Skills.h"
#include "Sound.h"
#include "SuperSave.h"

#define MAX_GLOBAL_SIZE (MaxGlobalVars*12 + 4)

static DWORD InLoop=0;
static DWORD SaveInCombatFix;

DWORD InWorldMap() { return (InLoop&WORLDMAP)?1:0; }
DWORD InCombat()   { return (InLoop&COMBAT)?1:0;   }
DWORD GetCurrentLoops() { return InLoop; }

static void _stdcall ResetState(DWORD onLoad) {
	if(!onLoad) FileSystemReset();
	ClearGlobalScripts();
	ClearGlobals();
	ForceGraphicsRefresh(0);
	WipeSounds();
	if(GraphicsMode>3) graphics_OnGameLoad();
	Knockback_OnGameLoad();
	Skills_OnGameLoad();
	InLoop=0;
	PerksReset();
	InventoryReset();
	RegAnimCombatCheck(1);
	AfterAttackCleanup();
	PartyControlReset();
}

void GetSavePath(char* buf, char* ftype) {
	sprintf(buf, "%s\\savegame\\slot%.2d\\sfall%s.sav", *VarPtr::patches, *VarPtr::slot_cursor + 1 + LSPageOffset, ftype); //add SuperSave Page offset
}

static char SaveSfallDataFailMsg[128];

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
		PerksSave(h);
		SaveArrays(h);
		CloseHandle(h);
	} else {
		dlogr("ERROR creating sfallgv!", DL_MAIN);
		Wrapper::display_print(SaveSfallDataFailMsg);
		PlaySfx("IISXXXX1");
	}
	GetSavePath(buf, "fs");
	h = CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		FileSystemSave(h);
	}
	CloseHandle(h);
}

static char SaveFailMsg[128];
static DWORD _stdcall combatSaveTest() {
	if (!SaveInCombatFix && !IsNpcControlled()) return 1;
	if (InLoop & COMBAT) {
		if (SaveInCombatFix == 2 || IsNpcControlled() || !(InLoop & PCOMBAT)) {
			Wrapper::display_print(SaveFailMsg);
			return 0;
		}
		DWORD ap;
		DWORD bonusmove;
		__asm {
			mov edx, 8;
			mov eax, ds:[VarPtr::obj_dude];
			call FuncOffs::stat_level_;
			mov ap, eax;
			mov eax, ds:[VarPtr::obj_dude];
			mov edx, 3;
			call FuncOffs::perk_level_;
			mov bonusmove, eax;
		}
		if (*(DWORD*)(*VarPtr::obj_dude + 0x40) != ap || bonusmove * 2 != *VarPtr::combat_free_move) {
			Wrapper::display_print(SaveFailMsg);
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

		mov edx, eax;
		call combatSaveTest;
		test eax, eax;
		jz end;
		mov eax, edx;

		or InLoop, SAVEGAME;
		mov ebx, 0x0047B88C;
		call ebx;
		and InLoop, (-1^SAVEGAME);
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
}

static void _stdcall LoadGame2_After() {
	char buf[MAX_PATH];
	GetSavePath(buf, "gv");

	dlog_f("Loading save game: %s\r\n", DL_MAIN, buf);

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

	LoadGlobalScripts();
	CritLoad();
	LoadHeroAppearance();
}

static void __declspec(naked) LoadSlot() {
	__asm {
		pushad;
		call LoadGame2_Before;
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
		or InLoop, LOADGAME;
		call FuncOffs::LoadGame_;
		/*push eax;
		push 0x0000101f;
		push 0x0045E949;
		call SafeWrite32;*/
		and InLoop, (-1^LOADGAME);
		//pop eax;
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

static void NewGame2() {
	ResetState(0);

	dlogr("Starting new game", DL_MAIN);

	SetNewCharAppearanceGlobals();

	/*if (GetPrivateProfileInt("Misc", "PipBoyAvailableAtGameStart", 0, ini)) {
		SafeWrite8(0x596C7B, 1);
	}
	if (GetPrivateProfileInt("Misc", "DisableHorrigan", 0, ini)) {
		*(DWORD*)0x672E04 = 1;
	}*/

	LoadGlobalScripts();
	CritLoad();
}

static bool DisableHorrigan = false;
static void __declspec(naked) NewGame() {
	__asm {
		pushad;
		call NewGame2;
		mov  al, DisableHorrigan;
		mov  byte ptr ds:[VarPtr::Meet_Frank_Horrigan], al;
		popad;
		call FuncOffs::main_game_loop_;
		retn;
	}
}

static void ReadExtraGameMsgFilesIfNeeded() {
	if (gExtraGameMsgLists.empty())
		ReadExtraGameMsgFiles();
}

static bool PipBoyAvailableAtGameStart = false;
static void __declspec(naked) MainMenu() {
	__asm {
		pushad;
		push 0;
		call ResetState;
		mov  al, PipBoyAvailableAtGameStart;
		mov  byte ptr ds:[VarPtr::gmovie_played_list + 0x3], al;
		call ReadExtraGameMsgFilesIfNeeded;
		call LoadHeroAppearance;
		popad;
		call FuncOffs::main_menu_loop_;
		retn;
	}
}
static void __declspec(naked) WorldMapHook() {
	__asm {
		or InLoop, WORLDMAP;
		xor eax, eax;
		call FuncOffs::wmWorldMapFunc_;
		and InLoop, (-1^WORLDMAP);
		retn;
	}
}
static void __declspec(naked) WorldMapHook2() {
	__asm {
		or InLoop, WORLDMAP;
		call FuncOffs::wmWorldMapFunc_;
		and InLoop, (-1^WORLDMAP);
		retn;
	}
}
static void __declspec(naked) CombatHook() {
	__asm {
		pushad;
		call AICombatStart;
		popad
		or InLoop, COMBAT;
		call FuncOffs::combat_;
		pushad;
		call AICombatEnd;
		popad
		and InLoop, (-1^COMBAT);
		retn;
	}
}
static void __declspec(naked) PlayerCombatHook() {
	__asm {
		or InLoop, PCOMBAT;
		call FuncOffs::combat_input_;
		and InLoop, (-1^PCOMBAT);
		retn;
	}
}
static void __declspec(naked) EscMenuHook() {
	__asm {
		or InLoop, ESCMENU;
		call FuncOffs::do_optionsFunc_;
		and InLoop, (-1^ESCMENU);
		retn;
	}
}
static void __declspec(naked) EscMenuHook2() {
	//Bloody stupid watcom compiler optimizations...
	__asm {
		or InLoop, ESCMENU;
		call FuncOffs::do_options_;
		and InLoop, (-1^ESCMENU);
		retn;
	}
}
static void __declspec(naked) OptionsMenuHook() {
	__asm {
		or InLoop, OPTIONS;
		call FuncOffs::do_prefscreen_;
		and InLoop, (-1^OPTIONS);
		retn;
	}
}
static void __declspec(naked) HelpMenuHook() {
	__asm {
		or InLoop, HELP;
		call FuncOffs::game_help_;
		and InLoop, (-1^HELP);
		retn;
	}
}
static void __declspec(naked) CharacterHook() {
	__asm {
		or InLoop, CHARSCREEN;
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
		and InLoop, (-1^CHARSCREEN);
		retn;
	}
}
static void __declspec(naked) DialogHook() {
	__asm {
		or InLoop, DIALOG;
		call FuncOffs::gdProcess_;
		and InLoop, (-1^DIALOG);
		retn;
	}
}
static void __declspec(naked) PipboyHook() {
	__asm {
		or InLoop, PIPBOY;
		call FuncOffs::pipboy_;
		and InLoop, (-1^PIPBOY);
		retn;
	}
}
static void __declspec(naked) SkilldexHook() {
	__asm {
		or InLoop, SKILLDEX;
		call FuncOffs::skilldex_select_;
		and InLoop, (-1^SKILLDEX);
		retn;
	}
}
static void __declspec(naked) InventoryHook() {
	__asm {
		or InLoop, INVENTORY;
		call FuncOffs::handle_inventory_;
		and InLoop, (-1^INVENTORY);
		retn;
	}
}
static void __declspec(naked) AutomapHook() {
	__asm {
		or InLoop, AUTOMAP;
		call FuncOffs::automap_;
		and InLoop, (-1^AUTOMAP);
		retn;
	}
}

void LoadGameHookInit() {
	SaveInCombatFix = GetPrivateProfileInt("Misc", "SaveInCombatFix", 1, ini);
	if (SaveInCombatFix > 2) SaveInCombatFix = 0;
	GetPrivateProfileString("sfall", "SaveInCombat", "Cannot save at this time", SaveFailMsg, 128, translationIni);
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

	HookCall(0x480AB3, NewGame);

	HookCall(0x47C72C, LoadSlot);
	HookCall(0x47D1C9, LoadSlot);
	HookCall(0x443AE4, LoadGame);
	HookCall(0x443B89, LoadGame);
	HookCall(0x480B77, LoadGame);
	HookCall(0x48FD35, LoadGame);
	HookCall(0x443AAC, SaveGame);
	HookCall(0x443B1C, SaveGame);
	HookCall(0x48FCFF, SaveGame);
	
	HookCall(0x480A28, MainMenu);

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
