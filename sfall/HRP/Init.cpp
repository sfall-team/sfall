/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include <psapi.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Utils.h"
#include "..\Modules\LoadOrder.h"

#include "viewmap\ViewMap.h"
#include "SplashScreen.h"
#include "MainMenu.h"
#include "InterfaceBar.h"

#include "Init.h"

namespace sfall
{

static const char* f2ResIni = ".\\f2_res.ini";

static long SCR_WIDTH  = 640;
static long SCR_HEIGHT = 480;

bool HRP::Enabled;

long HRP::ScreenWidth()  { return SCR_WIDTH; }
long HRP::ScreenHeight() { return SCR_HEIGHT; }

DWORD HRP::hrpDLLBaseAddr = 0x10000000;

static void GetHRPModule() {
	static const DWORD loadFunc = 0x4FE1D0;
	HMODULE dll;
	__asm call loadFunc; // get HRP loading address
	__asm mov  dll, eax;
	if (dll != NULL) HRP::hrpDLLBaseAddr = (DWORD)dll;
	dlog_f("Loaded f2_res.dll library at the memory address: 0x%x\n", DL_MAIN, dll);
}

bool HRP::CheckExternalPatch() {
	bool isEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check if Mash's HRP is enabled
	if (isEnabled) {
		GetHRPModule();
		MODULEINFO info;
		if (GetModuleInformation(GetCurrentProcess(), (HMODULE)HRP::hrpDLLBaseAddr, &info, sizeof(info)) && info.SizeOfImage >= 0x39940 + 7) {
			if (GetByteHRPValue(HRP_VAR_VERSION_STR + 7) == 0 && std::strncmp((const char*)HRPAddress(HRP_VAR_VERSION_STR), "4.1.8", 5) == 0) {
				hrpVersionValid = true;
			}
		}
	}
	return isEnabled;
}

void HRP::init() {
	if (!hrpIsEnabled && IniReader::GetIntDefaultConfig("Main", "HiResMode", 1) == 0) return; // vanilla game mode

	SCR_WIDTH  = IniReader::GetInt("Main", "SCR_WIDTH", 640, f2ResIni);
	SCR_HEIGHT = IniReader::GetInt("Main", "SCR_HEIGHT", 480, f2ResIni);

	if (SCR_WIDTH < 640) SCR_WIDTH = 640;
	if (SCR_HEIGHT < 480) SCR_HEIGHT = 480;

	if (hrpIsEnabled) { // external
		// You have both the built-in and external HRP enabled at the same time. It is recommended to turn off the external HRP by Mash.
		// To continue using the external HRP, disable the HiResMode option in ddraw.ini
		return;
	}
	Enabled = true;

	// Read config

	// = IniReader::GetInt("Main", "WINDOWED", 0, f2ResIni);
	// = IniReader::GetInt("Main", "WINDOWED_FULLSCREEN", 0, f2ResIni);
	// = IniReader::GetInt("Main", "SCALE_2X", 0, f2ResIni);

	//gDirectDrawMode = IniReader::GetInt("Main", "GRAPHICS_MODE", 2, f2ResIni) == 1;

	MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU = (IniReader::GetInt("MAINMENU", "SCALE_BUTTONS_AND_TEXT_MENU", 0, f2ResIni) != 0);
	MainMenuScreen::USE_HIRES_IMAGES = (IniReader::GetInt("MAINMENU", "USE_HIRES_IMAGES", 1, f2ResIni) != 0);
	MainMenuScreen::MENU_BG_OFFSET_X += IniReader::GetInt("MAINMENU", "MENU_BG_OFFSET_X", -14, f2ResIni);
	MainMenuScreen::MENU_BG_OFFSET_Y += IniReader::GetInt("MAINMENU", "MENU_BG_OFFSET_Y", -4, f2ResIni);
	MainMenuScreen::MAIN_MENU_SIZE = IniReader::GetInt("MAINMENU", "MAIN_MENU_SIZE", 1, f2ResIni);

	SplashScreen::SPLASH_SCRN_SIZE = IniReader::GetInt("STATIC_SCREENS", "SPLASH_SCRN_SIZE", 1, f2ResIni);
	//DEATH_SCRN_SIZE
	//END_SLIDE_SIZE
	//HELP_SCRN_SIZE

	std::string x = trim(IniReader::GetString("MAPS", "SCROLL_DIST_X", "480", 16, f2ResIni));
	std::string y = trim(IniReader::GetString("MAPS", "SCROLL_DIST_Y", "400", 16, f2ResIni));
	ViewMap::SCROLL_DIST_X = (x == "HALF_SCRN") ? (SCR_WIDTH / 2) + 32 : std::atol(x.c_str());
	ViewMap::SCROLL_DIST_Y = (y == "HALF_SCRN") ? (SCR_HEIGHT / 2) + 24 : std::atol(y.c_str());

	ViewMap::IGNORE_PLAYER_SCROLL_LIMITS = (IniReader::GetInt("MAPS", "IGNORE_PLAYER_SCROLL_LIMITS", 0, f2ResIni) != 0);
	ViewMap::IGNORE_MAP_EDGES = (IniReader::GetInt("MAPS", "IGNORE_MAP_EDGES", 0, f2ResIni) != 0);
	ViewMap::EDGE_CLIPPING_ON = (IniReader::GetInt("MAPS", "EDGE_CLIPPING_ON", 1, f2ResIni) != 0);

	// add before sfall.dat and after critter.dat
	LoadOrder::AddResourcePatches(
		IniReader::GetString("Main", "f2_res_dat", "f2_res.dat", MAX_PATH, f2ResIni),
		IniReader::GetString("Main", "f2_res_patches", "data", MAX_PATH, f2ResIni)
	);

	// Inject hacks
	SafeWrite32(0x482E30, FO_VAR_mapEntranceTileNum); // map_load_file_ (_tile_center_tile to _mapEntranceTileNum)

	// Set the resolution for GNW95_init_mode_ex_
	SafeWrite32(0x4CAD6B, SCR_WIDTH);  // 640
	SafeWrite32(0x4CAD66, SCR_HEIGHT); // 480

	// Set the resolution for the overlapping temporary black window when loading/starting the game
	// main_load_new_
	SafeWrite32(0x480D6C, SCR_HEIGHT);
	SafeWrite32(0x480D84, SCR_WIDTH);
	// gnw_main_
	SafeWrite32(0x480AFA, SCR_HEIGHT); // -100 for ifacebar?
	SafeWrite32(0x480B04, SCR_WIDTH);

	// Inits
	SplashScreen::init();
	MainMenuScreen::init();
	ViewMap::init();
	IFaceBar::init();
}

}
