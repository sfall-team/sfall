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
#include "ModuleManager.h"
#include "Modules\Module.h"
#include "Modules\AI.h"
#include "Modules\AmmoMod.h"
#include "Modules\AnimationsAtOnceLimit.h"
#include "Modules\BarBoxes.h"
#include "Modules\Books.h"
#include "Modules\BugFixes.h"
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
#include "Modules\Karma.h"
#include "Modules\KillCounter.h"
#include "Modules\knockback.h"
#include "Modules\LoadGameHook.h"
#include "Modules\LoadOrder.h"
#include "Modules\MainMenu.h"
#include "Modules\Message.h"
#include "Modules\MiscPatches.h"
#include "Modules\Movies.h"
#include "Modules\PartyControl.h"
#include "Modules\Perks.h"
#include "Modules\PlayerModel.h"
#include "Modules\Premade.h"
#include "Modules\QuestList.h"
#include "Modules\Reputations.h"
#include "Modules\ScriptExtender.h"
#include "Modules\Skills.h"
#include "Modules\Sound.h"
#include "Modules\SpeedPatch.h"
#include "Modules\Stats.h"
#include "Modules\ExtraSaveSlots.h"
#include "Modules\Tiles.h"
#include "Modules\Worldmap.h"
#include "SimplePatch.h"

#include "Logging.h"
#include "Version.h"

bool IsDebug = false;

char ini[65];
char translationIni[65];


static void DllMain2() {
	dlogr("In DllMain2", DL_MAIN);

	auto& manager = ModuleManager::getInstance();

	// initialize all modules
	manager.add<MiscPatches>();

	manager.add<SpeedPatch>();
	manager.add<BugFixes>();
	manager.add<Graphics>();
	manager.add<Movies>();
	manager.add<PlayerModel>();
	manager.add<Worldmap>();
	manager.add<Stats>();
	manager.add<ScriptExtender>();
	manager.add<LoadGameHook>();
	manager.add<Perks>();
	manager.add<Knockback>();
	manager.add<Skills>();
	manager.add<FileSystem>();
	manager.add<Criticals>();
	manager.add<LoadOrder>();
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
	manager.add<PartyControl>();
	manager.add<BurstMods>();
	manager.add<Books>();
	manager.add<Explosions>();

	manager.add<AI>();
	manager.add<AmmoMod>();
	manager.add<AnimationsAtOnce>();
	manager.add<BarBoxes>();
	manager.add<HeroAppearance>();

	manager.initAll();

	dlogr("Leave DllMain2", DL_MAIN);
}

// TODO: remove
static void _stdcall OnExit() {
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
