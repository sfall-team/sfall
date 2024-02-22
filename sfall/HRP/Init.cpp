/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma comment(lib, "psapi.lib")

#include <psapi.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Translate.h"
#include "..\WinProc.h"
#include "..\Modules\Graphics.h"
#include "..\Modules\LoadOrder.h"
#include "..\Modules\MiscPatches.h"
#include "..\Modules\SubModules\WindowRender.h"

#include "..\Game\tilemap.h"

#include "viewmap\ViewMap.h"
#include "SplashScreen.h"
#include "MainMenu.h"
#include "InterfaceBar.h"
#include "Dialog.h"
#include "Inventory.h"
#include "Character.h"
#include "LoadSave.h"
#include "MiscInterface.h"
#include "Worldmap.h"
#include "HelpScreen.h"
#include "DeathScreen.h"
#include "SlidesScreen.h"
#include "CreditsScreen.h"
#include "MoviesScreen.h"

#include "Init.h"

namespace HRP
{

namespace sf = sfall;

static const char* f2ResIni = ".\\f2_res.ini";
static DWORD baseDLLAddr = 0; // 0x10000000

bool Setting::VersionIsValid = false;

static bool enabled;
static long SCR_WIDTH  = 640;
static long SCR_HEIGHT = 480;
static long COLOUR_BITS = 32;
static char SCALE_2X;

bool Setting::IsEnabled()    { return enabled; }
long Setting::ScreenWidth()  { return SCR_WIDTH; }
long Setting::ScreenHeight() { return SCR_HEIGHT; }
long Setting::ColorBits()    { return COLOUR_BITS; }
char Setting::ScaleX2()      { return SCALE_2X; }

static void GetHRPModule() {
	baseDLLAddr = (DWORD)GetModuleHandleA("f2_res.dll");
	if (baseDLLAddr) sf::dlog_f("Loaded f2_res.dll library at the memory address: 0x%x\n", DL_MAIN, baseDLLAddr);
}

bool Setting::CheckExternalPatch() {
	bool isEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check if Mash's HRP is enabled
	if (isEnabled) {
		GetHRPModule();
		MODULEINFO info;
		if (baseDLLAddr && GetModuleInformation(GetCurrentProcess(), (HMODULE)baseDLLAddr, &info, sizeof(info)) && info.SizeOfImage >= 0x39940 + 7) {
			if (sf::GetByteHRPValue(HRP_VAR_VERSION_STR + 7) == 0 && std::strncmp((const char*)GetAddress(HRP_VAR_VERSION_STR), "4.1.8", 5) == 0) {
				VersionIsValid = true;
			}
		}
	}
	return ExternalEnabled();
}

DWORD Setting::GetAddress(DWORD addr) {
	return (baseDLLAddr + (addr & 0xFFFFF));
}

bool Setting::ExternalEnabled() {
	return (baseDLLAddr != 0);
}

static std::string GetBackupFileName(const char* runFileName, bool wait) {
	std::string bakExeName(runFileName);
	size_t n = bakExeName.rfind('.');
	if (n == std::string::npos) return std::string(); // empty

	bakExeName.replace(n, 4, ".hrp");
	char c = 10;
	while (std::remove(bakExeName.c_str()) != 0 && wait && --c) Sleep(1000); // delete .hrp (if it exists)

	return bakExeName;
}

static bool DisableExtHRP(const char* runFileName, std::string &cmdline) {
	std::string bakExeName = std::move(GetBackupFileName(runFileName, false));
	if (bakExeName.empty()) return false;

	std::rename(runFileName, bakExeName.c_str());  // rename the process file to .hrp
	CopyFileA(bakExeName.c_str(), runFileName, 0); // restore .exe (already unoccupied by a running process)

	FILE* ft = std::fopen(runFileName,"r+b");
	if (!ft) return false;
	std::fseek(ft, 0xD4880, SEEK_SET); // 0x4E4480

	BYTE restore[] = {0xC7, 0x05, 0x88, 0x27, 0x6B, 0x00, 0x00, 0xE7, 0x4D, 0x00};
	_fwrite_nolock(restore, 1, 10, ft);

	// 0x4FE1C0 - 0x4FE1E7
	std::fseek(ft, 0xEE5C0, SEEK_SET);

	DWORD restore1[10] = {0};
	_fwrite_nolock(restore1, 4, 10, ft);

	std::fclose(ft);
	cmdline.append(" -restart");

	//MessageBoxA(0, "High Resolution Patch has been successfully deactivated.", "sfall", MB_TASKMODAL | MB_ICONINFORMATION);

	ShellExecuteA(0, 0, runFileName, cmdline.c_str(), 0, SW_SHOWDEFAULT); // restart game
	return true;
}

static __declspec(naked) void fadeSystemPalette_hook() {
	__asm {
		call fo::funcoffs::setSystemPalette_;
		jmp  sfall::WinProc::MessageWindow;
	}
}

static __declspec(naked) void combat_turn_run_hook() {
	__asm {
		push edx;
		push ecx;
		call sfall::WinProc::WaitMessageWindow;
		pop  ecx;
		pop  edx;
		jmp  fo::funcoffs::process_bk_;
	}
}

static __declspec(naked) void gmouse_bk_process() {
	__asm {
		push edx;
		push ecx;
		call sfall::WinProc::WaitMessageWindow;
		pop  ecx;
		pop  edx;
		jmp  fo::funcoffs::gmouse_bk_process_;
	}
}
/*
static __declspec(naked) void GNW95_process_message_hack() {
	__asm {
		call sfall::WinProc::WaitMessageWindow;
		xor  eax, eax;
		retn
	}
}
*/
void Setting::init(const char* exeFileName, std::string &cmdline) {
	ViewMap::RedrawFix();

	bool hiResMode = sf::IniReader::GetConfigInt("Main", "HiResMode", 1) != 0;

	if (!Setting::ExternalEnabled() && !hiResMode) return; // vanilla game mode

	SCR_WIDTH  = sf::IniReader::GetInt("Main", "SCR_WIDTH", 0, f2ResIni);
	SCR_HEIGHT = sf::IniReader::GetInt("Main", "SCR_HEIGHT", 0, f2ResIni);

	if (SCR_WIDTH == 0 || SCR_HEIGHT == 0) {
		SCR_WIDTH  = sf::IniReader::GetConfigInt("Graphics", "GraphicsWidth", 640);
		SCR_HEIGHT = sf::IniReader::GetConfigInt("Graphics", "GraphicsHeight", 480);
	}

	if (SCR_WIDTH < 640) SCR_WIDTH = 640;
	if (SCR_HEIGHT < 480) SCR_HEIGHT = 480;

	if (cmdline.find(" -restart") != std::string::npos) {
		GetBackupFileName(exeFileName, true); // delete after restart
	}
	if (!hiResMode) return;

	if (Setting::ExternalEnabled()) {
		char infoMsg[512];
		sf::Translate::Get("sfall", "HiResInfo",
			"This version of sfall has its own integrated High Resolution Patch mode, which is compatible with the settings of the High Resolution Patch by Mash.\n\n"
			"If you want to continue using the Hi-Res Patch by Mash without seeing this message, disable the 'HiResMode' option in ddraw.ini.\n"
			"Or you can disable the external Hi-Res Patch to get new graphics improvements from sfall.\n\n"
			"Do you want to disable the High Resolution Patch by Mash?", infoMsg, 512);

		// replace \n for translated message
		for (size_t i = 0; i < sizeof(infoMsg); i++) {
			if (infoMsg[i] == '\n' || infoMsg[i] == '\0') break;
			if (infoMsg[i] == '\\' && infoMsg[i + 1] == 'n') {
				infoMsg[i] = ' ';
				infoMsg[++i] = '\n';
			}
		}
		if (MessageBoxA(0, infoMsg, "sfall: Conflict of High Resolution patches", MB_TASKMODAL | MB_ICONWARNING | MB_YESNO) == IDYES) {
			if (!DisableExtHRP(exeFileName, cmdline)) {
				MessageBoxA(0, "An error occurred while trying to deactivate the High Resolution Patch.", "sfall", MB_TASKMODAL | MB_ICONERROR);
			} else {
				ExitProcess(EXIT_SUCCESS); //std::exit(EXIT_SUCCESS);
			}
		}
		return;
	}
	enabled = true;
	sf::dlog("Applying built-in High Resolution Patch.", DL_MAIN);

	// Read High Resolution config

	int windowed = (!sf::extWrapper && sf::IniReader::GetInt("Main", "WINDOWED", 0, f2ResIni) != 0) ? 1 : 0;
	if (windowed && sf::IniReader::GetInt("Main", "WINDOWED_FULLSCREEN", 0, f2ResIni)) {
		windowed += 1;
		SCR_WIDTH  = GetSystemMetrics(SM_CXSCREEN);
		SCR_HEIGHT = GetSystemMetrics(SM_CYSCREEN);
	}

	int gMode = sf::IniReader::GetInt("Main", "GRAPHICS_MODE", 0, f2ResIni);
	if (gMode < 0 || gMode > 2) gMode = 2;
	if (gMode <= 1) sf::Graphics::mode = 1 + windowed; // DD7: 1 or 2/3 (vanilla)
	if (gMode == 2) sf::Graphics::mode = 4 + windowed; // DX9: 4 or 5/6 (sfall)

	if (sf::Graphics::mode == 1) {
		COLOUR_BITS = sf::IniReader::GetInt("Main", "COLOUR_BITS", 32, f2ResIni);
		if (COLOUR_BITS != 32 && COLOUR_BITS != 24 && COLOUR_BITS != 16) COLOUR_BITS = 32;
	}

	if (sf::IniReader::GetInt("Main", "SCALE_2X", 0, f2ResIni)) {
		if (SCR_HEIGHT < 960 && SCR_WIDTH < 1280) {
			SCR_WIDTH = 640;
			SCR_HEIGHT = 480;
		} else {
			SCR_WIDTH /= 2;
			SCR_HEIGHT /= 2;
		}
		SCALE_2X = 1;
	}

	MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU = (sf::IniReader::GetInt("MAINMENU", "SCALE_BUTTONS_AND_TEXT_MENU", 0, f2ResIni) != 0);
	MainMenuScreen::USE_HIRES_IMAGES = (sf::IniReader::GetInt("MAINMENU", "USE_HIRES_IMAGES", 1, f2ResIni) != 0);
	MainMenuScreen::MENU_BG_OFFSET_X += sf::IniReader::GetInt("MAINMENU", "MENU_BG_OFFSET_X", -14, f2ResIni);
	MainMenuScreen::MENU_BG_OFFSET_Y += sf::IniReader::GetInt("MAINMENU", "MENU_BG_OFFSET_Y", -4, f2ResIni);
	MainMenuScreen::MAIN_MENU_SIZE = sf::IniReader::GetInt("MAINMENU", "MAIN_MENU_SIZE", 1, f2ResIni);

	SplashScreen::SPLASH_SCRN_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "SPLASH_SCRN_SIZE", 1, f2ResIni);
	HelpScreen::HELP_SCRN_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "HELP_SCRN_SIZE", 1, f2ResIni);
	DeathScreen::DEATH_SCRN_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "DEATH_SCRN_SIZE", 1, f2ResIni);
	SlidesScreen::END_SLIDE_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "END_SLIDE_SIZE", 1, f2ResIni);
	MoviesScreen::MOVIE_SIZE = sf::IniReader::GetInt("MOVIES", "MOVIE_SIZE", 1, f2ResIni);

	long x = sf::IniReader::GetInt("MAPS", "SCROLL_DIST_X", 0, f2ResIni);
	long y = sf::IniReader::GetInt("MAPS", "SCROLL_DIST_Y", 0, f2ResIni);
	ViewMap::SCROLL_DIST_X = (x <= 0) ? (SCR_WIDTH / 2) + 32 : x;
	ViewMap::SCROLL_DIST_Y = (y <= 0) ? (SCR_HEIGHT / 2) + 24 : y;

	ViewMap::IGNORE_PLAYER_SCROLL_LIMITS = (sf::IniReader::GetInt("MAPS", "IGNORE_PLAYER_SCROLL_LIMITS", 0, f2ResIni) != 0);
	ViewMap::IGNORE_MAP_EDGES = (sf::IniReader::GetInt("MAPS", "IGNORE_MAP_EDGES", 0, f2ResIni) != 0);
	ViewMap::EDGE_CLIPPING_ON = (sf::IniReader::GetInt("MAPS", "EDGE_CLIPPING_ON", 1, f2ResIni) != 0);

	IFaceBar::IFACE_BAR_MODE = sf::IniReader::GetInt("IFACE", "IFACE_BAR_MODE", 0, f2ResIni);
	IFaceBar::IFACE_BAR_SIDE_ART = sf::IniReader::GetInt("IFACE", "IFACE_BAR_SIDE_ART", 1, f2ResIni);
	IFaceBar::IFACE_BAR_WIDTH = sf::IniReader::GetInt("IFACE", "IFACE_BAR_WIDTH", (!sf::versionCHI && SCR_WIDTH >= 800) ? 800 : 640, f2ResIni);
	IFaceBar::IFACE_BAR_SIDES_ORI = (sf::IniReader::GetInt("IFACE", "IFACE_BAR_SIDES_ORI", 0, f2ResIni) != 0);

	IFaceBar::ALTERNATE_AMMO_METRE = sf::IniReader::GetInt("IFACE", "ALTERNATE_AMMO_METRE", 0, f2ResIni);
	IFaceBar::ALTERNATE_AMMO_LIGHT = (BYTE)sf::IniReader::GetInt("IFACE", "ALTERNATE_AMMO_LIGHT", 196, f2ResIni);
	IFaceBar::ALTERNATE_AMMO_DARK = (BYTE)sf::IniReader::GetInt("IFACE", "ALTERNATE_AMMO_DARK", 75, f2ResIni);

	Dialog::DIALOG_SCRN_ART_FIX = (sf::IniReader::GetInt("OTHER_SETTINGS", "DIALOG_SCRN_ART_FIX", 1, f2ResIni) != 0);
	Dialog::DIALOG_SCRN_BACKGROUND = (sf::IniReader::GetInt("OTHER_SETTINGS", "DIALOG_SCRN_BACKGROUND", 0, f2ResIni) != 0);

	if (sf::IniReader::GetInt("OTHER_SETTINGS", "FADE_TIME_RECALCULATE_ON_FADE", 0, f2ResIni)) {
		sf::WindowRender::EnableRecalculateFadeSteps();
	}

	int splashTime = sf::IniReader::GetInt("OTHER_SETTINGS", "SPLASH_SCRN_TIME", 0, f2ResIni);
	if (splashTime > 10) splashTime = 10;
	SplashScreen::SPLASH_SCRN_TIME = splashTime;

	int nodes = sf::IniReader::GetInt("MAPS", "NumPathNodes", 1, f2ResIni);
	if (nodes > 1) game::Tilemap::SetPathMaxNodes((nodes < 20) ? nodes * 2000 : 40000);

	// add: patchXXX.dat > sfall.dat > [add here] > critter.dat > master.dat
	sf::LoadOrder::AddResourcePatches(
		sf::IniReader::GetString("Main", "f2_res_dat", "f2_res.dat", f2ResIni),
		sf::IniReader::GetString("Main", "f2_res_patches", "", f2ResIni)
	);

	/* Inject hacks */
	if (sf::IniReader::GetInt("INPUT", "EXTRA_WIN_MSG_CHECKS", 0, f2ResIni)) {
		sf::HookCall(0x4C73B1, fadeSystemPalette_hook);
		sf::HookCall(0x4227E5, combat_turn_run_hook);
		sf::HookCalls(gmouse_bk_process, {
			0x460EB0, 0x460EFE, 0x460F54, 0x460FAD, 0x46101D, 0x4610F9, // intface_rotate_numbers_
			0x45FA4D, // intface_end_window_open_
			0x45FBA6, // intface_end_window_close_
		});
	}

	if (sf::IniReader::GetInt("OTHER_SETTINGS", "CPU_USAGE_FIX", 1, f2ResIni) != 0) {
		//sf::MakeCall(0x4C9DA9, GNW95_process_message_hack, 11); // implementation by Mash
		sf::MiscPatches::SetIdle(1);
	}

	sf::SafeWrite32(0x482E30, FO_VAR_mapEntranceTileNum); // map_load_file_ (_tile_center_tile to _mapEntranceTileNum)

	if (SCR_WIDTH != 640 || SCR_HEIGHT != 480) {
		// Set the resolution for GNW95_init_mode_ex_
		sf::SafeWrite32(0x4CAD6B, SCR_WIDTH);  // 640
		sf::SafeWrite32(0x4CAD66, SCR_HEIGHT); // 480
		// initWindow_ "Error initializing video mode ..."
		sf::SafeWrite32(0x4B9245, (DWORD)&SCR_WIDTH);
		sf::SafeWrite32(0x4B923F, (DWORD)&SCR_HEIGHT);

		// Set the resolution for the overlapping temporary black window when loading/starting the game
		// main_load_new_
		sf::SafeWrite32(0x480D6C, SCR_HEIGHT);
		sf::SafeWrite32(0x480D84, SCR_WIDTH);
		// gnw_main_
		sf::SafeWrite32(0x480AFA, SCR_HEIGHT);
		sf::SafeWrite32(0x480B04, SCR_WIDTH);
		// LoadGame_
		sf::SafeWrite32(0x47C6E0, SCR_HEIGHT);
		sf::SafeWrite32(0x47C6E5, SCR_WIDTH);
		sf::SafeWrite32(0x47C703, SCR_WIDTH);
		sf::SafeWrite32(0x47C70D, SCR_HEIGHT);
	}
	if (sf::isDebug) sf::SafeWrite8(0x480AF6, fo::WinFlags::Hidden); // gnw_main_

	// Inits
	SplashScreen::init();
	MainMenuScreen::init();
	ViewMap::init();
	Worldmap::init();
	IFaceBar::init();
	Dialog::init();
	Inventory::init();
	Character::init();
	LoadSave::init();
	MiscInterface::init();
	HelpScreen::init();
	DeathScreen::init();
	SlidesScreen::init();
	CreditsScreen::init();
	MoviesScreen::init();

	sf::dlogr(" Done", DL_MAIN);
}

}
