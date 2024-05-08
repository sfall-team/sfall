/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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
#include "FalloutEngine\Fallout2.h"

#include "ModuleManager.h"
#include "Modules\Module.h"
#include "Modules\AI.h"
#include "Modules\Animations.h"
#include "Modules\BarBoxes.h"
#include "Modules\Books.h"
#include "Modules\BugFixes.h"
#include "Modules\BurstMods.h"
#include "Modules\Combat.h"
#include "Modules\Console.h"
#include "Modules\Credits.h"
#include "Modules\Criticals.h"
#include "Modules\CritterStats.h"
#include "Modules\CritterPoison.h"
#include "Modules\DamageMod.h"
#include "Modules\DebugEditor.h"
#include "Modules\Drugs.h"
#include "Modules\Elevators.h"
#include "Modules\EngineTweaks.h"
#include "Modules\Explosions.h"
#include "Modules\ExtraArt.h"
#include "Modules\ExtraSaveSlots.h"
#include "Modules\FileSystem.h"
#include "Modules\Graphics.h"
#include "Modules\HeroAppearance.h"
#include "Modules\HookScripts.h"
#include "Modules\Input.h"
#include "Modules\Interface.h"
#include "Modules\Inventory.h"
#include "Modules\Karma.h"
#include "Modules\KillCounter.h"
#include "Modules\LoadGameHook.h"
#include "Modules\LoadOrder.h"
#include "Modules\MainLoopHook.h"
#include "Modules\MainMenu.h"
#include "Modules\Message.h"
#include "Modules\MetaruleExtender.h"
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
#include "Modules\Unarmed.h"
#include "Modules\Worldmap.h"

#include "ConsoleWindow.h"
#include "CRC.h"
#include "InputFuncs.h"
#include "Logging.h"
#include "ReplacementFuncs.h"
#include "Translate.h"
#include "Utils.h"
#include "version.h"
#include "WinProc.h"

#include "HRP\Init.h"

#include <chrono>

ddrawDll ddraw;
static void LoadOriginalDll(DWORD fdwReason);

namespace sfall
{

bool isDebug = false;

bool versionCHI = false;

bool extWrapper = false;

static char falloutConfigName[65];

static void InitModules() {
	dlogr("In InitModules", DL_INIT);

	auto& manager = ModuleManager::instance();

	// initialize all modules
	manager.add<BugFixes>();    // fixes should be applied at the beginning
	manager.add<SpeedPatch>();
	manager.add<Graphics>();
	manager.add<Input>();
	manager.add<FileSystem>();
	manager.add<LoadOrder>();
	manager.add<LoadGameHook>();
	manager.add<MainLoopHook>();
	manager.add<ExtraArt>();

	manager.add<EngineTweaks>();
	manager.add<Books>();
	manager.add<Criticals>();
	manager.add<Elevators>();
	manager.add<Unarmed>();

	manager.add<Animations>();
	manager.add<BarBoxes>();
	manager.add<Explosions>();
	manager.add<Message>();
	manager.add<Interface>();
	manager.add<Worldmap>();
	manager.add<Tiles>();
	manager.add<Movies>();
	manager.add<Sound>();
	manager.add<MiscPatches>();

	manager.add<AI>();
	manager.add<DamageMod>();
	manager.add<BurstMods>();

	manager.add<Inventory>();
	manager.add<Objects>();
	manager.add<Stats>();
	manager.add<CritterStats>();
	manager.add<CritterPoison>();
	manager.add<Perks>();
	manager.add<Skills>();
	manager.add<Drugs>();       // should be loaded before PartyControl
	manager.add<PartyControl>();
	manager.add<Combat>();

	manager.add<PlayerModel>();
	manager.add<Karma>();
	manager.add<Premade>();
	manager.add<Reputations>();
	manager.add<KillCounter>();

	manager.add<MainMenu>();
	manager.add<HeroAppearance>();
	manager.add<TalkingHeads>();
	manager.add<ScriptShaders>();

	manager.add<ExtraSaveSlots>();
	manager.add<QuestList>();
	manager.add<Credits>();
	manager.add<Console>();

	// all built-in events(delegates) of modules should be executed before running the script handlers
	manager.add<MetaruleExtender>();
	manager.add<HookScripts>();
	manager.add<ScriptExtender>();

	manager.add<DebugEditor>();

	manager.initAll();

	dlogr("Leave InitModules", DL_INIT);
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
					               "If these options are disabled, click the 'change settings for all users' button and see if that enables them.", 0, MB_TASKMODAL | MB_ICONERROR);

					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	}
}

static int CheckEXE() {
	return std::strncmp((const char*)0x53C938, "FALLOUT Mapper", 14);
}

static HMODULE SfallInit() {
	char filepath[MAX_PATH];

	LoadOriginalDll(DLL_PROCESS_ATTACH);

	GetModuleFileName(0, filepath, MAX_PATH);

	auto initStart = std::chrono::high_resolution_clock::now();

	SetCursor(LoadCursorA(0, IDC_ARROW));
	ShowCursor(1);

	if (!CRC(filepath)) return 0;

	IniReader::instance().init();
	LoggingInit();
	ConsoleWindow::instance().init();

	// enabling debugging features
	isDebug = (IniReader::GetIntDefaultConfig("Debugging", "Enable", 0) != 0);

	if (!ddraw.dll) dlogr("Error: Cannot load the original ddraw.dll library.", DL_MAIN);

	if (!HRP::Setting::CheckExternalPatch()) {
		WinProc::init();
	}

	versionCHI = (*(DWORD*)0x4CAF23 == 0x225559); // check if the exe is modified for Chinese support

	if (IniReader::GetIntDefaultConfig("Debugging", "SkipCompatModeCheck", 0) == 0) {
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
	std::string overrideIni;

	std::string cmdline(GetCommandLineA());
	ToLowerCase(cmdline);

	size_t n = cmdline.find(".ini");
	if (n != std::string::npos && (n + 4) < cmdline.length() && cmdline[n + 4] != ' ') {
		n = cmdline.find(".ini", n + 4);
	}
	if (n != std::string::npos) {
		size_t m = n + 3;
		while (n > 0 && cmdline[n] != ' ') n--;
		if (n > 0) overrideIni = cmdline.substr(n + 1, m - n);
	}

	if (!overrideIni.empty()) {
		HANDLE h = CreateFileA(overrideIni.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);
			IniReader::instance().setConfigFile(overrideIni.c_str());
		} else {
			MessageBoxA(0, "You gave a command line argument to Fallout, but the configuration ini file was not found.\n"
			               "Using default ddraw.ini instead.", "Warning", MB_TASKMODAL | MB_ICONWARNING);
			goto defaultIni;
		}
	} else {
defaultIni:
		IniReader::instance().setDefaultConfigFile();
	}
	//std::srand(GetTickCount());

	if (IniReader::GetConfigString("Misc", "ConfigFile", "", falloutConfigName, 65)) {
		dlogr("Applying config file patch.", DL_INIT);
		SafeWriteBatch<DWORD>((DWORD)&falloutConfigName, {0x444BA5, 0x444BCA});
	} else {
		// if the ConfigFile is not assigned a value
		std::strcpy(falloutConfigName, (const char*)FO_VAR_fallout_config);
	}

	Translate::init(falloutConfigName);
	HRP::Setting::init(filepath, cmdline);

	InitReplacementHacks();
	InitInput();
	InitModules();

	if (HRP::Setting::ExternalEnabled()) ShowCursor(0);

	fo::var::setInt(FO_VAR_GNW95_hDDrawLib) = (long)ddraw.sfall;

	auto initEnd = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(initEnd - initStart);
	dlog_f("Sfall initialized in: %d us\n", DL_INIT, duration.count());

	return ddraw.sfall;
}

}

static void LoadOriginalDll(DWORD fdwReason) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		char path[MAX_PATH];
		GetFullPathNameA("wrapper\\ddraw.dll", MAX_PATH, path, NULL);
		ddraw.dll = LoadLibraryA(path); // external DirectDraw wrapper
		if (ddraw.dll) {
			sfall::extWrapper = true;
		} else {
			CopyMemory(path + GetSystemDirectoryA(path, MAX_PATH - 10), "\\ddraw.dll", 11); // path to original dll
			ddraw.dll = LoadLibraryA(path);
		}
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
		break;
	case DLL_PROCESS_DETACH:
		if (ddraw.dll) FreeLibrary(ddraw.dll);
		break;
	}
}

bool __stdcall DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		if (sfall::CheckEXE()) {
			ddraw.sfall = hinstDLL;
			sfall::MakeCall(0x4DE8DE, sfall::SfallInit); // LoadDirectX_
		}
	} else {
		LoadOriginalDll(fdwReason);
	}
	return true;
}
