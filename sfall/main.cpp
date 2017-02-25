/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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

#include <stdio.h>
#include <memory>

#include "FalloutEngine\Fallout2.h"
#include "Modules\Module.h"
#include "Modules\AI.h"
#include "Modules\AmmoMod.h"
#include "Modules\AnimationsAtOnceLimit.h"
#include "Modules\BarBoxes.h"
#include "Modules\Books.h"
#include "Modules\Bugs.h"
#include "Modules\BurstMods.h"
#include "Modules\Console.h"
#include "Modules\CRC.h"
#include "Modules\Credits.h"
#include "Modules\Criticals.h"
#include "Modules\Elevators.h"
#include "Modules\Explosions.h"
#include "Modules\FileSystem.h"
#include "Modules\Graphics.h"
#include "Modules\HeroAppearance.h"
#include "Modules\Inventory.h"
#include "Modules\KillCounter.h"
#include "Modules\knockback.h"
#include "Modules\LoadGameHook.h"
#include "Modules\MainMenu.h"
#include "Modules\Message.h"
#include "Modules\Misc.h"
#include "Modules\Movies.h"
#include "Modules\PartyControl.h"
#include "Modules\Perks.h"
#include "Modules\Premade.h"
#include "Modules\QuestList.h"
#include "Modules\Reputations.h"
#include "Modules\ScriptExtender.h"
#include "Modules\Skills.h"
#include "Modules\Sound.h"
#include "Modules\Stats.h"
#include "Modules\SuperSave.h"
#include "Modules\Tiles.h"
#include "Modules\Timer.h"
#include "Modules\Patches\LoadOrder.h"
#include "Modules\Patches\Worldmap.h"

#include "Logging.h"
#include "Version.h"
#if (_MSC_VER < 1600)
#include "Cpp11_emu.h"
#endif

static std::vector<std::unique_ptr<Module>> modules;

bool IsDebug = false;

char ini[65];
char translationIni[65];

bool NpcAutoLevelEnabled;

void ApplyScriptExtenderPatches() {
	dlog("Applying script extender patch.", DL_INIT);
	StatsInit();
	dlog(".", DL_INIT);
	ScriptExtenderSetup();
	dlog(".", DL_INIT);
	LoadGameHookInit();
	dlog(".", DL_INIT);
	PerksInit();
	dlog(".", DL_INIT);
	KnockbackInit();
	dlog(".", DL_INIT);
	SkillsInit();
	dlog(".", DL_INIT);
}

static void DllMain2() {
	//SafeWrite8(0x4B15E8, 0xc3);
	//SafeWrite8(0x4B30C4, 0xc3); //this is the one I need to override for bigger tiles
	dlogr("In DllMain2", DL_MAIN);

	modules.emplace_back(std::unique_ptr<HeroAppearanceModule>(new HeroAppearanceModule()));

	dlogr("Running BugsInit.", DL_INIT);
	BugsInit();
	dlogr(" Done", DL_INIT);

	ApplySpeedPatch();
	ApplyInputPatch();

	ApplyGraphicsPatch();

	AmmoModInit();
	MoviesInit();

	ApplyPlayerModelPatches();

	ApplyStartingStatePatches();

	ApplyPathfinderFix();

	ApplyWorldmapFpsPatch();

	if (GetPrivateProfileIntA("Misc", "DialogueFix", 1, ini)) {
		dlog("Applying dialogue patch.", DL_INIT);
		SafeWrite8(0x00446848, 0x31);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "ExtraKillTypes", 0, ini)) {
		dlog("Applying extra kill types patch.", DL_INIT);
		KillCounterInit(true);
		dlogr(" Done", DL_INIT);
	} else {
		KillCounterInit(false);
	}

	ApplyScriptExtenderPatches();

	ApplyCombatProcFix();

	ApplyWorldLimitsPatches();

	ApplyTimeLimitPatch();

	ApplyDebugModePatch();

	ApplyNPCAutoLevelPatch();
	
	char elevPath[MAX_PATH];
	GetPrivateProfileString("Misc", "ElevatorsFile", "", elevPath, MAX_PATH, ini);
	if (strlen(elevPath)>0) {
		dlog("Applying elevator patch.", DL_INIT);
		ElevatorsInit(elevPath);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "UseFileSystemOverride", 0, ini)) {
		FileSystemInit();
	}

	/*if (GetPrivateProfileIntA("Misc", "PrintToFileFix", 0, ini)) {
		dlog("Applying print to file patch.", DL_INIT);
		SafeWrite32(0x6C0364, (DWORD)&FakeFindFirstFile);
		SafeWrite32(0x6C0368, (DWORD)&FakeFindNextFile);
		dlogr(" Done", DL_INIT);
	}*/

	ApplyAdditionalWeaponAnimsPatch();

	if (IsDebug && GetPrivateProfileIntA("Debugging", "DontDeleteProtos", 0, ".\\ddraw.ini")) {
		dlog("Applying permanent protos patch.", DL_INIT);
		SafeWrite8(0x48007E, 0xeb);
		dlogr(" Done", DL_INIT);
	}

	CritInit();

	ApplyMultiPatchesPatch();

	ApplyDataLoadOrderPatch();

	ApplyDisplayKarmaChangesPatch();

	if (GetPrivateProfileInt("Misc", "AlwaysReloadMsgs", 0, ini)) {
		dlog("Applying always reload messages patch.", DL_INIT);
		SafeWrite8(0x4A6B8A, 0xff);
		SafeWrite32(0x4A6B8B, 0x02eb0074);
		dlogr(" Done", DL_INIT);
	}

	ApplyPlayIdleAnimOnReloadPatch();

	ApplyCorpseLineOfFireFix();

	if (GetPrivateProfileIntA("Misc", "SkipOpeningMovies", 0, ini)) {
		dlog("Blocking opening movies.", DL_INIT);
		BlockCall(0x4809CB);
		BlockCall(0x4809D4);
		BlockCall(0x4809E0);
		dlogr(" Done", DL_INIT);
	}

	ApplyNpcExtraApPatch();

	ApplySkilldexImagesPatch();

	if(GetPrivateProfileIntA("Misc", "RemoveWindowRounding", 0, ini)) {
		SafeWrite32(0x4B8090, 0x90909090);
		SafeWrite16(0x4B8094, 0x9090);
	}

	dlogr("Running TilesInit().", DL_INIT);
	TilesInit();

	dlogr("Applying main menu text patch", DL_INIT);
	CreditsInit();

	if(GetPrivateProfileIntA("Misc", "UseScrollingQuestsList", 0, ini)) {
		dlog("Applying quests list patch ", DL_INIT);
		QuestListInit();
		dlogr(" Done", DL_INIT);
	}

	dlog("Applying premade characters patch", DL_INIT);
	PremadeInit();

	dlogr("Running SoundInit().", DL_INIT);
	SoundInit();

	dlogr("Running ReputationsInit().", DL_INIT);
	ReputationsInit();

	dlogr("Running ConsoleInit().", DL_INIT);
	ConsoleInit();

	if (GetPrivateProfileIntA("Misc", "ExtraSaveSlots", 0, ini)) {
		dlog("Running EnableSuperSaving()", DL_INIT);
		EnableSuperSaving();
		dlogr(" Done", DL_INIT);
	}

	ApplySpeedInterfaceCounterAnimsPatch();

	ApplyKarmaFRMsPatch();

	ApplyScienceOnCrittersPatch();

	DWORD tmp;
	tmp = GetPrivateProfileIntA("Misc", "SpeedInventoryPCRotation", 166, ini);
	if (tmp != 166 && tmp <= 1000) {
		dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
		SafeWrite32(0x47066B, tmp);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running BarBoxesInit().", DL_INIT);
	BarBoxesInit();

	dlogr("Patching out ereg call.", DL_INIT);
	BlockCall(0x4425E6);

	tmp = GetPrivateProfileIntA("Misc", "AnimationsAtOnceLimit", 32, ini);
	if ((signed char)tmp > 32) {
		dlog("Applying AnimationsAtOnceLimit patch.", DL_INIT);
		AnimationsAtOnceInit((signed char)tmp);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "RemoveCriticalTimelimits", 0, ini)) {
		dlog("Removing critical time limits.", DL_INIT);
		SafeWrite8(0x42412B, 0x90);
		BlockCall(0x42412C);
		SafeWrite16(0x4A3052, 0x9090);
		SafeWrite16(0x4A3093, 0x9090);
		dlogr(" Done", DL_INIT);
	}

	ApplyOverrideMusicDirPatch();

	ApplyNpcStage6Fix();

	ApplyFashShotTraitFix();

	ApplyBoostScriptDialogLimitPatch();

	dlog("Running InventoryInit.", DL_INIT);
	InventoryInit();
	dlogr(" Done", DL_INIT);

	ApplyMotionScannerFlagsPatch();

	ApplyEncounterTableSizePatch();

	dlog("Initing main menu patches.", DL_INIT);
	MainMenuInit();
	dlogr(" Done", DL_INIT);

	if (GetPrivateProfileIntA("Misc", "DisablePipboyAlarm", 0, ini)) {
		SafeWrite8(0x499518, 0xc3);
	}

	dlog("Initing AI patches.", DL_INIT);
	AIInit();
	dlogr(" Done", DL_INIT);

	dlog("Initing AI control.", DL_INIT);
	PartyControlInit();
	dlogr(" Done", DL_INIT);

	ApplyObjCanSeeShootThroughPatch();

	// phobos2077:
	ComputeSprayModInit();
	ExplosionLightingInit();
	tmp = SimplePatch<DWORD>(0x4A2873, "Misc", "Dynamite_DmgMax", 50, 0, 9999);
	SimplePatch<DWORD>(0x4A2878, "Misc", "Dynamite_DmgMin", 30, 0, tmp);
	tmp = SimplePatch<DWORD>(0x4A287F, "Misc", "PlasticExplosive_DmgMax", 80, 0, 9999);
	SimplePatch<DWORD>(0x4A2884, "Misc", "PlasticExplosive_DmgMin", 40, 0, tmp);
	BooksInit();
	DWORD addrs[2] = {0x45F9DE, 0x45FB33};
	SimplePatch<WORD>(addrs, 2, "Misc", "CombatPanelAnimDelay", 1000, 0, 65535);
	addrs[0] = 0x447DF4; addrs[1] = 0x447EB6;
	SimplePatch<BYTE>(addrs, 2, "Misc", "DialogPanelAnimDelay", 33, 0, 255);
	addrs[0] = 0x499B99; addrs[1] = 0x499DA8;
	SimplePatch<BYTE>(addrs, 2, "Misc", "PipboyTimeAnimDelay", 50, 0, 127);

	if (GetPrivateProfileIntA("Misc", "EnableMusicInDialogue", 0, ini)) {
		dlog("Applying music in dialogue patch.", DL_INIT);
		SafeWrite8(0x44525B, 0x0);
		//BlockCall(0x450627);
		dlogr(" Done", DL_INIT);
	}

	ApplyTownMapsHotkeyFix();

	ApplyInstantWeaponEquipPatch();	

	ApplyNumbersInDialoguePatch();

	dlogr("Leave DllMain2", DL_MAIN);
}

static void _stdcall OnExit() {
	MiscReset();
	ClearReadExtraGameMsgFiles();
	ConsoleExit();
	AnimationsAtOnceExit();
	HeroAppearanceModExit();
	//SoundExit();
}

static void __declspec(naked) OnExitFunc() {
	__asm {
		pushad;
		call OnExit;
		popad;
		jmp FuncOffs::DOSCmdLineDestroy_;
	}
}

static void CompatModeCheck(HKEY root, const char* filepath, int extra) {
	HKEY key;
	char buf[MAX_PATH];
	DWORD size = MAX_PATH;
	DWORD type;
	if (!(type = RegOpenKeyEx(root, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, extra | STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &key))) {
		if (!RegQueryValueEx(key, filepath, 0, &type, (BYTE*)buf, &size)) {
			if (size && (type == REG_EXPAND_SZ || type == REG_MULTI_SZ || type == REG_SZ)) {
				if (strstr(buf, "256COLOR") || strstr(buf, "640X480") || strstr(buf, "WIN")) {
					RegCloseKey(key);
					/*if(!RegOpenKeyEx(root, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, extra|KEY_READ|KEY_WRITE, &key)) {
						if((type=RegDeleteValueA(key, filepath))==ERROR_SUCCESS) {
							MessageBoxA(0, "Fallout was set to run in compatibility mode.\n"
								"Please restart fallout to ensure it runs correctly.", "Error", 0);
							RegCloseKey(key);
							ExitProcess(-1);
						} else {
							//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, type, 0, buf, 260, 0);
							//MessageBoxA(0, buf, "", 0);
						}
					}*/

					MessageBoxA(0, "Fallout appears to be running in compatibility mode.\n" //, and sfall was not able to disable it.\n"
								"Please check the compatibility tab of fallout2.exe, and ensure that the following settings are unchecked.\n"
								"Run this program in compatibility mode for..., run in 256 colours, and run in 640x480 resolution.\n"
								"If these options are disabled, click the 'change settings for all users' button and see if that enables them.", "Error", 0);
							//RegCloseKey(key);
					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	} else {
		//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, type, 0, buf, 260, 0);
		//MessageBoxA(0, buf, "", 0);
	}
}

bool _stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID  lpreserved) {
	if (dwReason == DLL_PROCESS_ATTACH) {
		// enabling debugging features
 		IsDebug = (GetPrivateProfileIntA("Debugging", "Enable", 0, ".\\ddraw.ini") != 0);
		if (IsDebug) {
			LoggingInit();
		}

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

		if (!IsDebug || !GetPrivateProfileIntA("Debugging", "SkipCompatModeCheck", 0, ".\\ddraw.ini")) {
			int is64bit;
			typedef int (_stdcall *chk64bitproc)(HANDLE, int*);
			HMODULE h=LoadLibrary("Kernel32.dll");
			chk64bitproc proc = (chk64bitproc)GetProcAddress(h, "IsWow64Process");
			if(proc) proc(GetCurrentProcess(), &is64bit);
			else is64bit=0;
			FreeLibrary(h);

			CompatModeCheck(HKEY_CURRENT_USER, filepath, is64bit?KEY_WOW64_64KEY:0);
			CompatModeCheck(HKEY_LOCAL_MACHINE, filepath, is64bit?KEY_WOW64_64KEY:0);
		}

		// ini file override
		bool cmdlineexists = false;
		char* cmdline = GetCommandLineA();
		if (GetPrivateProfileIntA("Main", "UseCommandLine", 0, ".\\ddraw.ini")) {
			while (cmdline[0] == ' ') cmdline++;
			bool InQuote = false;
			int count = -1;

			while (true) {
				count++;
				if (cmdline[count] == 0) break;;
				if (cmdline[count] == ' ' && !InQuote) break;
				if (cmdline[count] == '"') {
					InQuote = !InQuote;
					if (!InQuote) break;
				}
			}
			if (cmdline[count] != 0) {
				count++;
				while (cmdline[count] == ' ') count++;
				cmdline = &cmdline[count];
				cmdlineexists = true;
			}
		}

		if (cmdlineexists && strlen(cmdline)) {
			strcpy_s(ini, ".\\");
			strcat_s(ini, cmdline);
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
			else {
				MessageBox(0, "You gave a command line argument to fallout, but it couldn't be matched to a file\n" \
						   "Using default ddraw.ini instead", "Warning", MB_TASKMODAL);
				strcpy_s(ini, ".\\ddraw.ini");
			}
		} else {
			strcpy_s(ini, ".\\ddraw.ini");
		}

		GetPrivateProfileStringA("Main", "TranslationsINI", "./Translations.ini", translationIni, 65, ini);

		DllMain2();
	}
	return true;
}
