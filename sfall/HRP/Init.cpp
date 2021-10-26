/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\LoadOrder.h"

#include "viewmap\ViewMap.h"
#include "SplashScreen.h"
#include "MainMenu.h"

#include "Init.h"

namespace sfall
{

static const char* f2ResIni = ".\\f2_res.ini";

static long SCR_WIDTH  = 640;
static long SCR_HEIGHT = 480;

bool HRP::Enabled;

long HRP::ScreenWidth()  { return SCR_WIDTH; }
long HRP::ScreenHeight() { return SCR_HEIGHT; }

void HRP::init() {
	if (!hrpIsEnabled && IniReader::GetIntDefaultConfig("Main", "HiResMode", 1) == 0) return; // vanilla game mode

	SCR_WIDTH  = IniReader::GetInt("Main", "SCR_WIDTH", 640, f2ResIni);
	SCR_HEIGHT = IniReader::GetInt("Main", "SCR_HEIGHT", 480, f2ResIni);

	if (SCR_WIDTH < 640) SCR_WIDTH = 640;
	if (SCR_HEIGHT < 480) SCR_HEIGHT = 480;

	if (hrpIsEnabled) { // external
		//MessageBoxA(0, "", "Error", MB_TASKMODAL | MB_ICONERROR);
		//ExitProcess(-1);
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

	std::string x = IniReader::GetString("MAPS", "SCROLL_DIST_X", "480", 16, f2ResIni);
	std::string y = IniReader::GetString("MAPS", "SCROLL_DIST_Y", "400", 16, f2ResIni);
	ViewMap::SCROLL_DIST_X = (x == "HALF_SCRN") ? (SCR_WIDTH / 2) + 32 : std::atol(x.c_str());
	ViewMap::SCROLL_DIST_Y = (y == "HALF_SCRN") ? (SCR_HEIGHT / 2) + 24 : std::atol(y.c_str());

	// add before sfall.dat and after critter.dat
	LoadOrder::AddResourcePatches(
		IniReader::GetString("Main", "f2_res_dat", "f2_res.dat", MAX_PATH, f2ResIni),
		IniReader::GetString("Main", "f2_res_patches", "data", MAX_PATH, f2ResIni)
	);

	// Inject hacks

	// Set resolution for GNW95_init_mode_ex_
	SafeWrite32(0x4CAD6B, SCR_WIDTH);  // 640
	SafeWrite32(0x4CAD66, SCR_HEIGHT); // 480

	// Inits
	SplashScreen::init();
	MainMenuScreen::init();
	ViewMap::init();
}

}
