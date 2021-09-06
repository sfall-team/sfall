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

#pragma comment(lib, "psapi.lib")

#include <psapi.h>

#include "main.h"
#include "FalloutEngine.h"
#include "AI.h"
#include "Animations.h"
#include "BarBoxes.h"
#include "Books.h"
#include "BugFixes.h"
#include "BurstMods.h"
#include "Combat.h"
#include "Console.h"
#include "CRC.h"
#include "Credits.h"
#include "Criticals.h"
#include "DamageMod.h"
#include "DebugEditor.h"
#include "Elevators.h"
#include "EngineTweaks.h"
#include "Explosions.h"
#include "ExtraSaveSlots.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "HookScripts.h"
#include "Input.h"
#include "Interface.h"
#include "Inventory.h"
#include "Karma.h"
#include "KillCounter.h"
#include "LoadGameHook.h"
#include "LoadOrder.h"
#include "Logging.h"
#include "MainMenu.h"
#include "Message.h"
#include "MetaruleExtender.h"
#include "MiscPatches.h"
#include "Movies.h"
#include "Objects.h"
#include "PartyControl.h"
#include "Perks.h"
#include "PlayerModel.h"
#include "Premade.h"
#include "QuestList.h"
#include "ReplacementFuncs.h"
#include "Reputations.h"
#include "ScriptExtender.h"
#include "Skills.h"
#include "Sound.h"
#include "SpeedPatch.h"
#include "Stats.h"
#include "TalkingHeads.h"
#include "Tiles.h"
#include "Translate.h"
#include "version.h"
#include "Worldmap.h"

ddrawDll ddraw;

bool isDebug = false;

bool hrpIsEnabled = false;
bool hrpVersionValid = false; // HRP 4.1.8 version validation

static DWORD hrpDLLBaseAddr = 0x10000000;

DWORD HRPAddress(DWORD addr) {
	return (hrpDLLBaseAddr | (addr & 0xFFFFF));
}

char falloutConfigName[65];

static void InitModules() {
	dlogr("In InitModules", DL_MAIN);

	// fixes should be applied at the beginning
	dlogr("Running BugFixes_Init().", DL_INIT);
	BugFixes_Init();

	dlogr("Running FileSystem_Init().", DL_INIT);
	FileSystem_Init();

	dlogr("Running Graphics_Init().", DL_INIT);
	Graphics_Init();

	dlogr("Running Input_Init().", DL_INIT);
	Input_Init();

	dlogr("Running LoadOrder_Init().", DL_INIT);
	LoadOrder_Init();

	dlogr("Running LoadGameHook_Init().", DL_INIT);
	LoadGameHook_Init();

	dlogr("Running EngineTweaks_Init().", DL_INIT);
	EngineTweaks_Init();

	dlogr("Running Books_Init().", DL_INIT);
	Books_Init();

	dlogr("Running Criticals_Init().", DL_INIT);
	Criticals_Init();

	dlogr("Running Elevators_Init().", DL_INIT);
	Elevators_Init();

	dlogr("Running Movies_Init().", DL_INIT);
	Movies_Init();

	dlogr("Running MainMenu_Init().", DL_INIT);
	MainMenu_Init();

	dlogr("Running Interface_Init().", DL_INIT);
	Interface_Init();

	dlogr("Running Objects_Init().", DL_INIT);
	Objects_Init();

	dlogr("Running SpeedPatch_Init().", DL_INIT);
	SpeedPatch_Init();

	dlogr("Running PlayerModel_Init().", DL_INIT);
	PlayerModel_Init();

	dlogr("Running Worldmap_Init().", DL_INIT);
	Worldmap_Init();

	dlogr("Running Stats_Init().", DL_INIT);
	Stats_Init();

	dlogr("Running Perks_Init().", DL_INIT);
	Perks_Init();

	dlogr("Running Combat_Init().", DL_INIT);
	Combat_Init();

	dlogr("Running Skills_Init().", DL_INIT);
	Skills_Init();

	dlogr("Running Karma_Init().", DL_INIT);
	Karma_Init();

	dlogr("Running Tiles_Init().", DL_INIT);
	Tiles_Init();

	dlogr("Running Credits_Init().", DL_INIT);
	Credits_Init();

	dlogr("Running QuestList_Init().", DL_INIT);
	QuestList_Init();

	dlogr("Running Premade_Init().", DL_INIT);
	Premade_Init();

	dlogr("Running Sound_Init().", DL_INIT);
	Sound_Init();

	dlogr("Running Reputations_Init().", DL_INIT);
	Reputations_Init();

	dlogr("Running Console_Init().", DL_INIT);
	Console_Init();

	dlogr("Running ExtraSaveSlots_Init().", DL_INIT);
	ExtraSaveSlots_Init();

	dlogr("Running Inventory_Init().", DL_INIT);
	Inventory_Init();

	dlogr("Running PartyControl_Init().", DL_INIT);
	PartyControl_Init();

	dlogr("Running BurstMods_Init().", DL_INIT);
	BurstMods_Init();

	dlogr("Running Explosions_Init().", DL_INIT);
	Explosions_Init();

	dlogr("Running Message_Init().", DL_INIT);
	Message_Init();

	dlogr("Running KillCounter_Init().", DL_INIT);
	KillCounter_Init();

	dlogr("Running AI_Init().", DL_INIT);
	AI_Init();

	dlogr("Running DamageMod_Init().", DL_INIT);
	DamageMod_Init();

	dlogr("Running Animations_Init().", DL_INIT);
	Animations_Init();

	dlogr("Running BarBoxes_Init().", DL_INIT);
	BarBoxes_Init();

	dlogr("Running HeroAppearance_Init().", DL_INIT);
	HeroAppearance_Init();

	dlogr("Running MiscPatches_Init().", DL_INIT);
	MiscPatches_Init();

	dlogr("Running TalkingHeads_Init().", DL_INIT);
	TalkingHeads_Init();

	// most of modules should be initialized before running the script handlers
	dlogr("Running MetaruleExtender_Init().", DL_INIT);
	MetaruleExtender_Init();

	dlogr("Running HookScripts_Init().", DL_INIT);
	HookScripts_Init();

	dlogr("Running ScriptExtender_Init().", DL_INIT);
	ScriptExtender_Init();

	dlogr("Running DebugEditor_Init().", DL_INIT);
	DebugEditor_Init();

	dlogr("Leave InitModules", DL_MAIN);
}

static void __stdcall OnExit() {
	Graphics_Exit();
	//EngineTweaks_Exit();
	Books_Exit();
	//Movies_Exit();
	Interface_Exit();
	SpeedPatch_Exit();
	Skills_Exit();
	Tiles_Exit();
	Sound_Exit();
	Reputations_Exit();
	Console_Exit();
	ExtraSaveSlots_Exit();
	Message_Exit();
	//Animations_Exit();
	BarBoxes_Exit();
	HeroAppearance_Exit();
	MiscPatches_Exit();
	TalkingHeads_Exit();
}

static void __declspec(naked) OnExitFunc() {
	__asm {
		pushad;
		call OnExit;
		popad;
		jmp DOSCmdLineDestroy_;
	}
}

static void LoadHRPModule() {
	static const DWORD loadFunc = 0x4FE1D0;
	HMODULE dll;
	__asm call loadFunc; // get HRP loading address
	__asm mov  dll, eax;
	if (dll != NULL) hrpDLLBaseAddr = (DWORD)dll;
	dlog_f("Loaded f2_res.dll library at the memory address: 0x%x\n", DL_MAIN, dll);
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

					MessageBoxA(0, "Fallout appears to be running in compatibility mode.\n" //, and sfall was not able to disable it.\n"
					               "Please check the compatibility tab of fallout2.exe, and ensure that the following settings are unchecked:\n"
					               "Run this program in compatibility mode for..., run in 256 colours, and run in 640x480 resolution.\n"
					               "If these options are disabled, click the 'change settings for all users' button and see if that enables them.", "Error", MB_TASKMODAL | MB_ICONERROR);

					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	}
}

static bool LoadOriginalDll(DWORD dwReason) {
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			char path[MAX_PATH];
			CopyMemory(path + GetSystemDirectoryA(path , MAX_PATH - 10), "\\ddraw.dll", 11); // path to original dll
			ddraw.dll = LoadLibraryA(path);
			if (ddraw.dll) {
				ddraw.AcquireDDThreadLock          = GetProcAddress(ddraw.dll, "AcquireDDThreadLock");
				ddraw.CheckFullscreen              = GetProcAddress(ddraw.dll, "CheckFullscreen");
				ddraw.CompleteCreateSysmemSurface  = GetProcAddress(ddraw.dll, "CompleteCreateSysmemSurface");
				ddraw.D3DParseUnknownCommand       = GetProcAddress(ddraw.dll, "D3DParseUnknownCommand");
				ddraw.DDGetAttachedSurfaceLcl      = GetProcAddress(ddraw.dll, "DDGetAttachedSurfaceLcl");
				ddraw.DDInternalLock               = GetProcAddress(ddraw.dll, "DDInternalLock");
				ddraw.DDInternalUnlock             = GetProcAddress(ddraw.dll, "DDInternalUnlock");
				ddraw.DSoundHelp                   = GetProcAddress(ddraw.dll, "DSoundHelp");
				ddraw.DirectDrawCreateClipper      = GetProcAddress(ddraw.dll, "DirectDrawCreateClipper");
				ddraw.DirectDrawCreate             = GetProcAddress(ddraw.dll, "DirectDrawCreate");
				ddraw.DirectDrawCreateEx           = GetProcAddress(ddraw.dll, "DirectDrawCreateEx");
				ddraw.DirectDrawEnumerateA         = GetProcAddress(ddraw.dll, "DirectDrawEnumerateA");
				ddraw.DirectDrawEnumerateExA       = GetProcAddress(ddraw.dll, "DirectDrawEnumerateExA");
				ddraw.DirectDrawEnumerateExW       = GetProcAddress(ddraw.dll, "DirectDrawEnumerateExW");
				ddraw.DirectDrawEnumerateW         = GetProcAddress(ddraw.dll, "DirectDrawEnumerateW");
				ddraw.DllCanUnloadNow              = GetProcAddress(ddraw.dll, "DllCanUnloadNow");
				ddraw.DllGetClassObject            = GetProcAddress(ddraw.dll, "DllGetClassObject");
				ddraw.GetDDSurfaceLocal            = GetProcAddress(ddraw.dll, "GetDDSurfaceLocal");
				ddraw.GetOLEThunkData              = GetProcAddress(ddraw.dll, "GetOLEThunkData");
				ddraw.GetSurfaceFromDC             = GetProcAddress(ddraw.dll, "GetSurfaceFromDC");
				ddraw.RegisterSpecialCase          = GetProcAddress(ddraw.dll, "RegisterSpecialCase");
				ddraw.ReleaseDDThreadLock          = GetProcAddress(ddraw.dll, "ReleaseDDThreadLock");
				ddraw.SetAppCompatData             = GetProcAddress(ddraw.dll, "SetAppCompatData");
			}
			return true;
		case DLL_PROCESS_DETACH:
			if (ddraw.dll) FreeLibrary(ddraw.dll);
			break;
	}
	return false;
}

bool __stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	if (LoadOriginalDll(dwReason)) {
		// enabling debugging features
		isDebug = (GetIntDefaultConfig("Debugging", "Enable", 0) != 0);
		if (isDebug) {
			LoggingInit();
			if (!ddraw.dll) dlogr("Error: Cannot load the original ddraw.dll library.", DL_MAIN);
		}

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

		if (!isDebug || !GetIntDefaultConfig("Debugging", "SkipCompatModeCheck", 0)) {
			int is64bit;
			typedef int (__stdcall *chk64bitproc)(HANDLE, int*);
			HMODULE h = LoadLibrary("Kernel32.dll");
			chk64bitproc proc = (chk64bitproc)GetProcAddress(h, "IsWow64Process");
			if (proc)
				proc(GetCurrentProcess(), &is64bit);
			else
				is64bit = 0;
			FreeLibrary(h);

			CompatModeCheck(HKEY_CURRENT_USER, filepath, is64bit ? KEY_WOW64_64KEY : 0);
			CompatModeCheck(HKEY_LOCAL_MACHINE, filepath, is64bit ? KEY_WOW64_64KEY : 0);
		}

		// ini file override
		bool cmdlineexists = false;
		char* cmdline = GetCommandLineA();
		if (GetIntDefaultConfig("Main", "UseCommandLine", 0)) {
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

		if (cmdlineexists && *cmdline != 0) {
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (h != INVALID_HANDLE_VALUE) {
				CloseHandle(h);
				SetConfigFile(cmdline);
			} else {
				MessageBoxA(0, "You gave a command line argument to Fallout, but it couldn't be matched to a file.\n" \
				               "Using default ddraw.ini instead.", "Warning", MB_TASKMODAL | MB_ICONWARNING);
				goto defaultIni;
			}
		} else {
defaultIni:
			SetDefaultConfigFile();
		}

		hrpIsEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check if HRP is enabled
		if (hrpIsEnabled) {
			LoadHRPModule();
			MODULEINFO info;
			if (GetModuleInformation(GetCurrentProcess(), (HMODULE)hrpDLLBaseAddr, &info, sizeof(info)) && info.SizeOfImage >= 0x39940 + 7) {
				if (GetByteHRPValue(HRP_VAR_VERSION_STR + 7) == 0 && std::strncmp((const char*)HRPAddress(HRP_VAR_VERSION_STR), "4.1.8", 5) == 0) {
					hrpVersionValid = true;
				}
			}
		}
		//std::srand(GetTickCount());

		IniReader_Init();

		if (GetConfigString("Misc", "ConfigFile", "", falloutConfigName, 65)) {
			dlog("Applying config file patch.", DL_INIT);
			const DWORD configFileAddr[] = {0x444BA5, 0x444BCA};
			SafeWriteBatch<DWORD>((DWORD)&falloutConfigName, configFileAddr);
			dlogr(" Done", DL_INIT);
		} else {
			// if the ConfigFile is not assigned a value
			std::strcpy(falloutConfigName, (const char*)FO_VAR_fallout_config);
		}

		Translate_Init(falloutConfigName);

		InitReplacementHacks();
		InitModules();
	}
	return true;
}
