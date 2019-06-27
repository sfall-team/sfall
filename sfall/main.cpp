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

#include <algorithm>
#include <stdio.h>
#include <memory>

#include "FalloutEngine\Fallout2.h"
#include "ModuleManager.h"
#include "Modules\Module.h"
#include "Modules\AI.h"
#include "Modules\AnimationsAtOnceLimit.h"
#include "Modules\BarBoxes.h"
#include "Modules\Books.h"
#include "Modules\BugFixes.h"
#include "Modules\BurstMods.h"
#include "Modules\Combat.h"
#include "Modules\Console.h"
#include "Modules\CRC.h"
#include "Modules\Credits.h"
#include "Modules\Criticals.h"
#include "Modules\DamageMod.h"
#include "Modules\DebugEditor.h"
#include "Modules\Drugs.h"
#include "Modules\Elevators.h"
#include "Modules\Explosions.h"
#include "Modules\ExtraSaveSlots.h"
#include "Modules\FileSystem.h"
#include "Modules\Graphics.h"
#include "Modules\HookScripts.h"
#include "Modules\HeroAppearance.h"
#include "Modules\Input.h"
#include "Modules\Interface.h"
#include "Modules\Inventory.h"
#include "Modules\Karma.h"
#include "Modules\KillCounter.h"
#include "Modules\LoadGameHook.h"
#include "Modules\LoadOrder.h"
#include "Modules\MainMenu.h"
#include "Modules\MainLoopHook.h"
#include "Modules\Message.h"
#include "Modules\MiscPatches.h"
#include "Modules\Movies.h"
#include "Modules\Objects.h"
#include "Modules\PartyControl.h"
#include "Modules\Perks.h"
#include "Modules\PlayerModel.h"
#include "Modules\Premade.h"
#include "Modules\QuestList.h"
#include "Modules\Reputations.h"
#include "Modules\ScriptExtender.h"
#include "Modules\ScriptShaders.h"
#include "Modules\Skills.h"
#include "Modules\Sound.h"
#include "Modules\SpeedPatch.h"
#include "Modules\Stats.h"
#include "Modules\TalkingHeads.h"
#include "Modules\Tiles.h"
#include "Modules\Worldmap.h"
#include "SimplePatch.h"

#include "Logging.h"
#include "Utils.h"
#include "Version.h"

#include "main.h"

ddrawDll ddraw;

namespace sfall
{

bool isDebug = false;

const char ddrawIni[] = ".\\ddraw.ini";
static char ini[65]   = ".\\";
static char translationIni[65];
DWORD modifiedIni;

unsigned int GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return GetPrivateProfileIntA(section, setting, defaultValue, ini);
}

std::string GetIniString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile) {
	char* buf = new char[bufSize];
	GetPrivateProfileStringA(section, setting, defaultValue, buf, bufSize, iniFile);
	std::string str(buf);
	delete[] buf;
	return trim(str);
}

std::vector<std::string> GetIniList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile) {
	auto list = split(GetIniString(section, setting, defaultValue, bufSize, iniFile), delimiter);
	std::transform(list.cbegin(), list.cend(), list.begin(), trim);
	return list;
}

std::string GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniString(section, setting, defaultValue, bufSize, ini);
}

size_t GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize) {
	return GetPrivateProfileStringA(section, setting, defaultValue, buf, bufSize, ini);
}

std::vector<std::string> GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniList(section, setting, defaultValue, bufSize, ',', ini);
}

std::string Translate(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniString(section, setting, defaultValue, bufSize, translationIni);
}

size_t Translate(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize) {
	return GetPrivateProfileStringA(section, setting, defaultValue, buffer, bufSize, translationIni);
}

static void InitModules() {
	dlogr("In InitModules", DL_MAIN);

	auto& manager = ModuleManager::getInstance();

	// initialize all modules
	manager.add<BugFixes>();    // fixes should be applied at the beginning
	manager.add<Graphics>();
	manager.add<Input>();
	manager.add<LoadOrder>();
	manager.add<LoadGameHook>();
	manager.add<MainLoopHook>();
	manager.add<Movies>();
	manager.add<Objects>();
	manager.add<SpeedPatch>();
	manager.add<PlayerModel>();
	manager.add<Worldmap>();
	manager.add<Stats>();
	manager.add<Perks>();
	manager.add<Combat>();
	manager.add<Skills>();
	manager.add<FileSystem>();
	manager.add<Criticals>();
	manager.add<Karma>();
	manager.add<Tiles>();
	manager.add<Credits>();
	manager.add<QuestList>();
	manager.add<Premade>();
	manager.add<Sound>();
	manager.add<Reputations>();
	manager.add<Console>();
	manager.add<ExtraSaveSlots>();
	manager.add<Inventory>();
	manager.add<MainMenu>();
	manager.add<Drugs>();       // should be above than PartyControl
	manager.add<PartyControl>();
	manager.add<BurstMods>();
	manager.add<Books>();
	manager.add<Explosions>();
	manager.add<Message>();
	manager.add<Elevators>();
	manager.add<KillCounter>();
	manager.add<Interface>();
	//
	manager.add<AI>();
	manager.add<DamageMod>();
	manager.add<AnimationsAtOnce>();
	manager.add<BarBoxes>();
	manager.add<HeroAppearance>();
	manager.add<MiscPatches>();
	manager.add<TalkingHeads>();
	manager.add<ScriptShaders>();

	// should be last (reason: all build-in events(delegates) of modules must be executed before running the script handlers)
	manager.add<HookScripts>();
	manager.add<ScriptExtender>();

	manager.add<DebugEditor>();

	manager.initAll();

	dlogr("Leave InitModules", DL_MAIN);
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
								"Please check the compatibility tab of fallout2.exe, and ensure that the following settings are unchecked.\n"
								"Run this program in compatibility mode for..., run in 256 colours, and run in 640x480 resolution.\n"
								"If these options are disabled, click the 'change settings for all users' button and see if that enables them.", "Error", 0);

					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	}
}

inline void SfallInit() {
	// enabling debugging features
	isDebug = (GetPrivateProfileIntA("Debugging", "Enable", 0, ::sfall::ddrawIni) != 0);
	if (isDebug) {
		LoggingInit();
		if (!ddraw.dll) dlog("Error: Cannot load the original ddraw.dll library.\n", DL_MAIN);
	}

	char filepath[MAX_PATH];
	GetModuleFileName(0, filepath, MAX_PATH);

	CRC(filepath);

	if (!isDebug || !GetPrivateProfileIntA("Debugging", "SkipCompatModeCheck", 0, ::sfall::ddrawIni)) {
		int is64bit;
		typedef int (_stdcall *chk64bitproc)(HANDLE, int*);
		HMODULE h = LoadLibrary("Kernel32.dll");
		chk64bitproc proc = (chk64bitproc)GetProcAddress(h, "IsWow64Process");
		if(proc)
			proc(GetCurrentProcess(), &is64bit);
		else
			is64bit=0;
		FreeLibrary(h);

		CompatModeCheck(HKEY_CURRENT_USER, filepath, is64bit?KEY_WOW64_64KEY:0);
		CompatModeCheck(HKEY_LOCAL_MACHINE, filepath, is64bit?KEY_WOW64_64KEY:0);
	}

	// ini file override
	bool cmdlineexists = false;
	char* cmdline = GetCommandLineA();
	if (GetPrivateProfileIntA("Main", "UseCommandLine", 0, ::sfall::ddrawIni)) {
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
		HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);
			strcat_s(ini, cmdline);
		} else {
			MessageBox(0, "You gave a command line argument to fallout, but it couldn't be matched to a file\n" \
						"Using default ddraw.ini instead", "Warning", MB_TASKMODAL);
			strcpy_s(ini, ::sfall::ddrawIni);
		}
	} else {
		strcpy_s(ini, ::sfall::ddrawIni);
	}

	GetConfigString("Main", "TranslationsINI", "./Translations.ini", translationIni, 65);
	modifiedIni = GetConfigInt("Main", "ModifiedIni", 0);

	InitModules();
}

}

static bool LoadOriginalDll(DWORD dwReason) {
	switch (dwReason)
	{
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
				//ddraw.DllCanUnloadNow            = GetProcAddress(ddraw.dll, "DllCanUnloadNow");
				//ddraw.DllGetClassObject          = GetProcAddress(ddraw.dll, "DllGetClassObject");
				ddraw.GetDDSurfaceLocal            = GetProcAddress(ddraw.dll, "GetDDSurfaceLocal");
				ddraw.GetOLEThunkData              = GetProcAddress(ddraw.dll, "GetOLEThunkData");
				ddraw.GetSurfaceFromDC             = GetProcAddress(ddraw.dll, "GetSurfaceFromDC");
				ddraw.RegisterSpecialCase          = GetProcAddress(ddraw.dll, "RegisterSpecialCase");
				ddraw.ReleaseDDThreadLock          = GetProcAddress(ddraw.dll, "ReleaseDDThreadLock");
			}
			return true;
		case DLL_PROCESS_DETACH:
			if (ddraw.dll) FreeLibrary(ddraw.dll);
			break;
	}
	return false;
}

bool _stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	if (LoadOriginalDll(dwReason)) {
		sfall::SfallInit();
	}
	return true;
}
