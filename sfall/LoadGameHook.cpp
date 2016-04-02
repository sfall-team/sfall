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
#include "Console.h"
#include "Criticals.h"
#include "FalloutEngine.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "input.h"
#include "Inventory.h"
#include "Knockback.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "movies.h"
#include "PartyControl.h"
#include "perks.h"
#include "ScriptExtender.h"
#include "skills.h"
#include "sound.h"
#include "SuperSave.h"
#include "version.h"
#include "Message.h"

#define MAX_GLOBAL_SIZE (MaxGlobalVars*12 + 4)

static DWORD InLoop=0;
DWORD GainStatFix=0;
static DWORD SaveInCombatFix;

DWORD InWorldMap() { return (InLoop&WORLDMAP)?1:0; }
DWORD InCombat()   { return (InLoop&COMBAT)?1:0;   }
DWORD GetCurrentLoops() { return InLoop; }

static void ModifyGainXXXPerks() {
	SafeWrite8(0x004AF11F, 0xeb); //Strength
	SafeWrite8(0x004AF181, 0xeb); //Perception
	SafeWrite8(0x004AF19D, 0xeb); //Endurance
	SafeWrite8(0x004AF1BD, 0xeb); //Charisma
	SafeWrite8(0x004AF214, 0xeb); //Intelligance
	SafeWrite8(0x004AF230, 0xeb); //Agility
	SafeWrite8(0x004AF24B, 0xeb); //Luck
}
static void RestoreGainXXXPerks() {
	SafeWrite8(0x004AF11F, 0x74); //Strength
	SafeWrite8(0x004AF181, 0x74); //Perception
	SafeWrite8(0x004AF19D, 0x74); //Endurance
	SafeWrite8(0x004AF1BD, 0x74); //Charisma
	SafeWrite8(0x004AF214, 0x74); //Intelligance
	SafeWrite8(0x004AF230, 0x74); //Agility
	SafeWrite8(0x004AF24B, 0x74); //Luck
}

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
	GainStatFix=0;
	RestoreGainXXXPerks();
	PerksReset();
	InventoryReset();
	RegAnimCombatCheck(1);
	AfterAttackCleanup();
	PartyControlReset();
}

void GetSavePath(char* buf, int type) {
	int saveid = *(int*)_slot_cursor + 1 +LSPageOffset;//add SuperSave Page offset
	char buf2[6];
	//Fallout saving is independent of working directory
	struct sPath {
		char* path;
		int a;
		int b;
		sPath* next;
	};
	sPath* spath=*(sPath**)_paths;
	while(spath->a&&spath->next) spath=spath->next;

	//strcpy_s(buf, MAX_PATH, **(char***)_paths);
	strcpy_s(buf, MAX_PATH, spath->path);
	strcat_s(buf, MAX_PATH, "\\savegame\\slot");
	_itoa_s(saveid, buf2, 10);
	if(strlen(buf2)==1) strcat_s(buf, MAX_PATH, "0");
	strcat_s(buf, MAX_PATH, buf2);
	switch(type) {
		case 0:
			strcat_s(buf, MAX_PATH, "\\sfallgv.sav");
			break;
		case 1:
			strcat_s(buf, MAX_PATH, "\\sfallfs.sav");
			break;
	}
}

static char SaveSfallDataFailMsg[128];

static void _stdcall SaveGame2() {
	char buf[MAX_PATH];
	GetSavePath(buf, 0);

	dlog_f("Saving game: %s\r\n", DL_MAIN, buf);

	DWORD unused;
	DWORD unused2=0;
	HANDLE h=CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(h!=INVALID_HANDLE_VALUE) {
		SaveGlobals(h);
		WriteFile(h, &unused2, 4, &unused, 0);
		WriteFile(h, &GainStatFix, 4, &unused, 0);
		PerksSave(h);
		SaveArrays(h);
		CloseHandle(h);
	} else {
		dlogr("ERROR creating sfallgv!", DL_MAIN);
		DisplayConsoleMessage(SaveSfallDataFailMsg);
		PlaySfx("IISXXXX1");
	}
	GetSavePath(buf, 1);
	h=CreateFileA(buf, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(h!=INVALID_HANDLE_VALUE) {
		FileSystemSave(h);
	}
	CloseHandle(h);
}

static char SaveFailMsg[128];
static DWORD _stdcall combatSaveTest() {
	if(!SaveInCombatFix) return 1;
	if(InLoop&COMBAT) {
		if(SaveInCombatFix==2 || !(InLoop&PCOMBAT)) {
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
		if(*(DWORD*)(*(DWORD*)_obj_dude + 0x40) != ap || bonusmove*2!=*(DWORD*)_combat_free_move) {
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
	GetSavePath(buf, 0);

	dlog_f("Loading save game: %s\r\n", DL_MAIN, buf);

	ClearGlobals();
	HANDLE h=CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(h!=INVALID_HANDLE_VALUE) {
		DWORD size=0;
		DWORD unused;
		LoadGlobals(h);
		ReadFile(h, (&unused), 4, &size, 0);
		ReadFile(h, &GainStatFix, 4, &size, 0);
		if(!size) GainStatFix = 0;
		else {
			PerksLoad(h);
			LoadArrays(h);
		}
		CloseHandle(h);
	} else {
		GainStatFix = 0;
		dlogr("Cannot read sfallgv.sav - assuming non-sfall save.", DL_MAIN);
	}

	if(GainStatFix) ModifyGainXXXPerks();
	else RestoreGainXXXPerks();

	LoadGlobalScripts();
	CritLoad();
	LoadHeroAppearance();
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

void ReadExtraGameMsgFiles()
{
	int read;
	std::string names;

	names.resize(256);

	while ((read = GetPrivateProfileStringA("Misc", "ExtraGameMsgFileList", "",
		(LPSTR)names.data(), names.size(), ".\\ddraw.ini")) == names.size() - 1)
		names.resize(names.size() + 256);

	if (names.empty())
		return;

	names.resize(names.find_first_of('\0'));
	names.append(",");

	int begin = 0;
	int end;
	int length;

	while ((end = names.find_first_of(',', begin)) != std::string::npos)
	{
		length = end - begin;

		if (length > 0)
		{
			std::string path = "game\\" + names.substr(begin, length) + ".msg";
			MSGList* list = new MSGList;

			if (LoadMsgList(list, (char*)path.data()) == 1)
				gExtraGameMsgLists.insert(std::make_pair(0x2000 + gExtraGameMsgLists.size(), list));
		}

		begin = end + 1;
	}
}

static void NewGame2() {
	ResetState(0);

	dlogr("Starting new game", DL_MAIN);

	SetNewCharAppearanceGlobals();

	if(GetPrivateProfileInt("Misc", "GainStatPerkFix", 1, ini)) {
		ModifyGainXXXPerks();
		GainStatFix=1;
	} else {
		RestoreGainXXXPerks();
		GainStatFix=0;
	}

	if(GetPrivateProfileInt("Misc", "PipBoyAvailableAtGameStart", 0, ini)) {
		SafeWrite8(0x00596C7B, 1);
	}
	if(GetPrivateProfileInt("Misc", "DisableHorrigan", 0, ini)) {
		*(DWORD*)0x00672E04=1;
	}

	ReadExtraGameMsgFiles();

	LoadGlobalScripts();
	CritLoad();
}
static void __declspec(naked) NewGame() {
	__asm {
		pushad;
		call NewGame2;
		popad;
		call main_game_loop_;
		retn;
	}
}

static void __declspec(naked) MainMenu() {
	__asm {
		pushad;
		push 0;
		call ResetState;
		call LoadHeroAppearance;
		popad;
		call main_menu_loop_;
		retn;
	}
}
static void __declspec(naked) WorldMapHook() {
	__asm {
		or InLoop, WORLDMAP;
		xor eax, eax;
		call wmWorldMapFunc_;
		and InLoop, (-1^WORLDMAP);
		retn;
	}
}
static void __declspec(naked) WorldMapHook2() {
	__asm {
		or InLoop, WORLDMAP;
		call wmWorldMapFunc_;
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
		call combat_;
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
		call combat_input_;
		and InLoop, (-1^PCOMBAT);
		retn;
	}
}
static void __declspec(naked) EscMenuHook() {
	__asm {
		or InLoop, ESCMENU;
		call do_optionsFunc_;
		and InLoop, (-1^ESCMENU);
		retn;
	}
}
static void __declspec(naked) EscMenuHook2() {
	//Bloody stupid watcom compiler optimizations...
	__asm {
		or InLoop, ESCMENU;
		call do_options_;
		and InLoop, (-1^ESCMENU);
		retn;
	}
}
static void __declspec(naked) OptionsMenuHook() {
	__asm {
		or InLoop, OPTIONS;
		call do_prefscreen_;
		and InLoop, (-1^OPTIONS);
		retn;
	}
}
static void __declspec(naked) HelpMenuHook() {
	__asm {
		or InLoop, HELP;
		call game_help_;
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
		and InLoop, (-1^CHARSCREEN);
		retn;
	}
}
static void __declspec(naked) DialogHook() {
	__asm {
		or InLoop, DIALOG;
		call gdProcess_;
		and InLoop, (-1^DIALOG);
		retn;
	}
}
static void __declspec(naked) PipboyHook() {
	__asm {
		or InLoop, PIPBOY;
		call pipboy_;
		and InLoop, (-1^PIPBOY);
		retn;
	}
}
static void __declspec(naked) SkilldexHook() {
	__asm {
		or InLoop, SKILLDEX;
		call skilldex_select_;
		and InLoop, (-1^SKILLDEX);
		retn;
	}
}
static void __declspec(naked) InventoryHook() {
	__asm {
		or InLoop, INVENTORY;
		call handle_inventory_;
		and InLoop, (-1^INVENTORY);
		retn;
	}
}
static void __declspec(naked) AutomapHook() {
	__asm {
		or InLoop, AUTOMAP;
		call automap_;
		and InLoop, (-1^AUTOMAP);
		retn;
	}
}

void LoadGameHookInit() {
	SaveInCombatFix=GetPrivateProfileInt("Misc", "SaveInCombatFix", 1, ini);
	if(SaveInCombatFix>2) SaveInCombatFix=0;
	if(SaveInCombatFix) {
		GetPrivateProfileString("sfall", "SaveInCombat", "Cannot save at this time", SaveFailMsg, 128, translationIni);
	}
	GetPrivateProfileString("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! Check if other programs interfere with savegame files/folders and try again!", SaveSfallDataFailMsg, 128, translationIni);

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
