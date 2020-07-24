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

#include <algorithm>
#include <math.h>
#include <stdio.h>

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
#include "Explosions.h"
#include "ExtraSaveSlots.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
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
#include "Reputations.h"
#include "ScriptExtender.h"
#include "Skills.h"
#include "Sound.h"
#include "SpeedPatch.h"
#include "Stats.h"
#include "TalkingHeads.h"
#include "Tiles.h"
#include "Utils.h"
#include "version.h"
#include "Worldmap.h"

ddrawDll ddraw;

bool isDebug = false;

bool hrpIsEnabled = false;
bool hrpVersionValid = false; // HRP 4.1.8 version validation

const char ddrawIniDef[] = ".\\ddraw.ini";
static char ini[65] = ".\\";
static char translationIni[65];

DWORD modifiedIni;
DWORD hrpDLLBaseAddr = 0x10000000;

DWORD HRPAddress(DWORD addr) {
	return (hrpDLLBaseAddr + (addr & 0xFFFFF));
}

int iniGetInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	return GetPrivateProfileIntA(section, setting, defaultValue, iniFile);
}

size_t iniGetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	return GetPrivateProfileStringA(section, setting, defaultValue, buf, bufSize, iniFile);
}

std::string GetIniString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile) {
	char* buf = new char[bufSize];
	iniGetString(section, setting, defaultValue, buf, bufSize, iniFile);
	std::string str(buf);
	delete[] buf;
	return str;
}

std::vector<std::string> GetIniList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile) {
	std::vector<std::string> list = split(GetIniString(section, setting, defaultValue, bufSize, iniFile), delimiter);
	std::transform(list.cbegin(), list.cend(), list.begin(), trim);
	return list;
}

/*
	For ddraw.ini config
*/
unsigned int GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return iniGetInt(section, setting, defaultValue, ini);
}

std::string GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return trim(GetIniString(section, setting, defaultValue, bufSize, ini));
}

size_t GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize) {
	return iniGetString(section, setting, defaultValue, buf, bufSize, ini);
}

std::vector<std::string> GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniList(section, setting, defaultValue, bufSize, ',', ini);
}

std::string Translate(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniString(section, setting, defaultValue, bufSize, translationIni);
}

size_t Translate(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize) {
	return iniGetString(section, setting, defaultValue, buffer, bufSize, translationIni);
}

int SetConfigInt(const char* section, const char* setting, int value) {
	char* buf = new char[33];
	_itoa_s(value, buf, 33, 10);
	int result = WritePrivateProfileStringA(section, setting, buf, ini);
	delete[] buf;
	return result;
}

static void DllMain2() {
	dlogr("In DllMain2", DL_MAIN);

	// fixes should be applied at the beginning
	dlogr("Running BugFixesInit().", DL_INIT);
	BugFixesInit();

	dlogr("Running GraphicsInit().", DL_INIT);
	GraphicsInit();

	//if (GetConfigInt("Input", "Enable", 0)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(0x50FB70, "ddraw.dll");
		availableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}

	dlogr("Running LoadOrderInit().", DL_INIT);
	LoadOrderInit();

	dlogr("Running LoadGameHookInit().", DL_INIT);
	LoadGameHookInit();

	dlogr("Running MoviesInit().", DL_INIT);
	MoviesInit();

	dlogr("Running MainMenuInit().", DL_INIT);
	MainMenuInit();

	dlogr("Running InterfaceInit().", DL_INIT);
	InterfaceInit();

	dlogr("Running ObjectsInit().", DL_INIT);
	ObjectsInit();

	dlogr("Running SpeedPatchInit().", DL_INIT);
	SpeedPatchInit();

	dlogr("Running PlayerModelInit().", DL_INIT);
	PlayerModelInit();

	dlogr("Running WorldmapInit().", DL_INIT);
	WorldmapInit();

	dlogr("Running StatsInit().", DL_INIT);
	StatsInit();

	dlogr("Running PerksInit().", DL_INIT);
	PerksInit();

	dlogr("Running CombatInit().", DL_INIT);
	CombatInit();

	dlogr("Running SkillsInit().", DL_INIT);
	SkillsInit();

	dlogr("Running FileSystemInit().", DL_INIT);
	FileSystemInit();

	dlogr("Running CriticalsInit().", DL_INIT);
	CriticalsInit();

	dlogr("Running KarmaInit().", DL_INIT);
	KarmaInit();

	dlogr("Running TilesInit().", DL_INIT);
	TilesInit();

	dlogr("Running CreditsInit().", DL_INIT);
	CreditsInit();

	dlogr("Running QuestListInit().", DL_INIT);
	QuestListInit();

	dlogr("Running PremadeInit().", DL_INIT);
	PremadeInit();

	dlogr("Running SoundInit().", DL_INIT);
	SoundInit();

	dlogr("Running ReputationsInit().", DL_INIT);
	ReputationsInit();

	dlogr("Running ConsoleInit().", DL_INIT);
	ConsoleInit();

	dlogr("Running ExtraSaveSlotsInit().", DL_INIT);
	ExtraSaveSlotsInit();

	dlogr("Running InventoryInit().", DL_INIT);
	InventoryInit();

	dlogr("Running PartyControlInit().", DL_INIT);
	PartyControlInit();

	dlogr("Running BurstModsInit().", DL_INIT);
	BurstModsInit();

	dlogr("Running BooksInit().", DL_INIT);
	BooksInit();

	dlogr("Running ExplosionInit().", DL_INIT);
	ExplosionInit();

	dlogr("Running ElevatorsInit().", DL_INIT);
	ElevatorsInit();

	dlogr("Running KillCounterInit().", DL_INIT);
	KillCounterInit();

	dlogr("Running AIInit().", DL_INIT);
	AIInit();

	dlogr("Running DamageModInit().", DL_INIT);
	DamageModInit();

	dlogr("Running AnimationsInit().", DL_INIT);
	AnimationsInit();

	dlogr("Running BarBoxesInit().", DL_INIT);
	BarBoxesInit();

	dlogr("Running HeroAppearanceModInit().", DL_INIT);
	HeroAppearanceModInit();

	dlogr("Running MiscPatchesInit().", DL_INIT);
	MiscPatchesInit();

	dlogr("Running TalkingHeadsInit().", DL_INIT);
	TalkingHeadsInit();

	// most of modules should be initialized before running the script handlers
	dlogr("Running MetaruleExtenderInit().", DL_INIT);
	MetaruleExtenderInit();

	dlogr("Running ScriptExtenderInit().", DL_INIT);
	ScriptExtenderInit();

	dlogr("Running DebugEditorInit().", DL_INIT);
	DebugEditorInit();

	dlogr("Leave DllMain2", DL_MAIN);
}

static void __stdcall OnExit() {
	GraphicsExit();
	InterfaceExit();
	SpeedPatchExit();
	SkillsExit();
	SoundExit();
	ReputationsExit();
	ConsoleExit();
	ExtraSaveSlotsExit();
	BooksExit();
	//gExtraGameMsgLists.clear();
	AnimationsExit();
	BarBoxesExit();
	HeroAppearanceModExit();
	MiscPatchesExit();
	TalkingHeadsExit();
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
		isDebug = (iniGetInt("Debugging", "Enable", 0, ddrawIniDef) != 0);
		if (isDebug) {
			LoggingInit();
			if (!ddraw.dll) dlog("Error: Cannot load the original ddraw.dll library.\n", DL_MAIN);
		}

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

		if (!isDebug || !iniGetInt("Debugging", "SkipCompatModeCheck", 0, ddrawIniDef)) {
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
		if (iniGetInt("Main", "UseCommandLine", 0, ddrawIniDef)) {
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
				strcat_s(ini, cmdline);
			} else {
				MessageBoxA(0, "You gave a command line argument to Fallout, but it couldn't be matched to a file.\n" \
							   "Using default ddraw.ini instead.", "Warning", MB_TASKMODAL | MB_ICONWARNING);
				goto defaultIni;
			}
		} else {
defaultIni:
			strcpy(&ini[2], &ddrawIniDef[2]);
		}

		GetConfigString("Main", "TranslationsINI", ".\\Translations.ini", translationIni, 65);
		modifiedIni = GetConfigInt("Main", "ModifiedIni", 0);

		hrpIsEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check if HRP is enabled
		if (hrpIsEnabled) {
			LoadHRPModule();
			MODULEINFO info;
			if (GetModuleInformation(GetCurrentProcess(), (HMODULE)hrpDLLBaseAddr, &info, sizeof(info)) && info.SizeOfImage >= 0x39940 + 7) {
				if (*(BYTE*)HRPAddress(0x10039940 + 7) == 0 && strncmp((const char*)HRPAddress(0x10039940), "4.1.8", 5) == 0) {
					hrpVersionValid = true;
				}
			}
		}

		DllMain2();
	}
	return true;
}
