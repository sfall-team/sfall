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
#include "..\Translate.h"

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
static bool SCALE_2X;
static long SCR_WIDTH  = 640;
static long SCR_HEIGHT = 480;

bool Setting::IsEnabled()    { return enabled; }
long Setting::ScreenWidth()  { return SCR_WIDTH; }
long Setting::ScreenHeight() { return SCR_HEIGHT; }

static void GetHRPModule() {
	static const DWORD loadFunc = 0x4FE1D0;
	//HMODULE dll;
	__asm call loadFunc; // get HRP loading address
	__asm mov  baseDLLAddr, eax;
	sf::dlog_f("Loaded f2_res.dll library at the memory address: 0x%x\n", DL_MAIN, baseDLLAddr);
}

bool Setting::CheckExternalPatch() {
	bool isEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check if Mash's HRP is enabled
	if (isEnabled) {
		GetHRPModule();
		MODULEINFO info;
		if (GetModuleInformation(GetCurrentProcess(), (HMODULE)baseDLLAddr, &info, sizeof(info)) && info.SizeOfImage >= 0x39940 + 7) {
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
	if (n != std::string::npos) {
		bakExeName.replace(n, 4, ".hrp");
		char c = 10;
		while (std::remove(bakExeName.c_str()) != 0 && wait && --c) Sleep(1000); // delete .hrp (if it exists)
	}
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
	_fwrite_nolock(restore, 1, sizeof(restore), ft);

	// 0x4FE1C0 - 0x4FE1E7
	std::fseek(ft, 0xEE5C0, SEEK_SET);

	DWORD restore1[10] = {0};
	_fwrite_nolock(restore1, 4, sizeof(restore1), ft);

	fclose(ft);
	cmdline.append(" -restart");

	MessageBoxA(0, "High Resolution Patch has been successfully deactivated.", "sfall", MB_TASKMODAL | MB_ICONINFORMATION);

	ShellExecuteA(0, 0, runFileName, cmdline.c_str(), 0, SW_SHOWDEFAULT); // restart game
	return true;
}

/*
static void __declspec(naked) mem_copy() {
	__asm {
		cmp  edx, eax;
		jz   end;

		push ecx;
		push esi;
		push edi;
		mov  ecx, ebx;
		jnb  forward;

		lea  esi, [edx + ecx];
//		and  ebx, 3;
		cmp  esi, eax;
		jbe  forward; // src <= dst

		// backward copy
		dec  esi; //lea     esi, [edi - 1];
		lea  edi, [eax + ecx]; // dst + num

		std;
		dec  edi;
		dec  esi;
		dec  edi;
		shr  ecx, 1;
		rep movsw;

		adc  ecx, ecx;
		inc  esi;
		inc  edi;
//		mov  ecx, ebx;
		rep movsb
		cld;
		pop  edi;
		pop  esi;
		pop  ecx;
end:
		retn;

forward:
		and  ebx, 3;
		mov  esi, edx;
		mov  edi, eax;
		shr  ecx, 2;
		rep movsd;
		mov  ecx, ebx;
		rep movsb;
		pop  edi;
		pop  esi;
		pop  ecx;
		retn;
	}
}
*/

void Setting::init(const char* exeFileName, std::string &cmdline) {
	//HookCall(0x482899, mem_copy);
	//SafeWrite16(0x4B2EA8, 0x9090); // _show_grid

	bool resMode = sf::IniReader::GetIntDefaultConfig("Main", "HiResMode", 1) != 0;

	if (!Setting::ExternalEnabled() && !resMode) return; // vanilla game mode

	SCR_WIDTH  = sf::IniReader::GetInt("Main", "SCR_WIDTH", 640, f2ResIni);
	SCR_HEIGHT = sf::IniReader::GetInt("Main", "SCR_HEIGHT", 480, f2ResIni);

	if (SCR_WIDTH < 640) SCR_WIDTH = 640;
	if (SCR_HEIGHT < 480) SCR_HEIGHT = 480;

	if (cmdline.find(" -restart") != std::string::npos) {
		GetBackupFileName(exeFileName, true); // delete after restart
	}
	if (resMode == false) return;

	if (Setting::ExternalEnabled()) {
		char infoMsg[512];
		sf::Translate::Get("sfall", "HiResInfo",
			"This version of sfall has its own integrated High Resolution mode patch, which is compatible with the High Resolution Patch by Mash.\n\n"
			"If you want to continue using the High Resolution Patch by Mash without seeing this message, disable the 'HiResMode' option in the ddraw.ini file.\n"
			"Or you can disable the external HRP to get new graphic improvements from sfall.\n\n"
			"Do you want to disable the High Resolution Patch by Mash?", infoMsg, 512);

		// replace \n for translated message
		for (size_t i = 0; i < sizeof(infoMsg); i++) {
			if (infoMsg[i] == '\0') break;
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

	// Read High Resolution config

	// = IniReader::GetInt("Main", "WINDOWED", 0, f2ResIni);
	// = IniReader::GetInt("Main", "WINDOWED_FULLSCREEN", 0, f2ResIni);
	/*
	if (IniReader::GetInt("Main", "SCALE_2X", 0, f2ResIni)) {
		SCR_WIDTH /= 2;
		SCR_HEIGHT /= 2;

		if (SCR_WIDTH < 640) SCR_WIDTH = 640;
		if (SCR_HEIGHT < 480) SCR_HEIGHT = 480;

		SCALE_2X = true;
	};
	*/
	//gDirectDrawMode = IniReader::GetInt("Main", "GRAPHICS_MODE", 2, f2ResIni) == 1;

	MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU = (sf::IniReader::GetInt("MAINMENU", "SCALE_BUTTONS_AND_TEXT_MENU", 0, f2ResIni) != 0);
	MainMenuScreen::USE_HIRES_IMAGES = (sf::IniReader::GetInt("MAINMENU", "USE_HIRES_IMAGES", 1, f2ResIni) != 0);
	MainMenuScreen::MENU_BG_OFFSET_X += sf::IniReader::GetInt("MAINMENU", "MENU_BG_OFFSET_X", -14, f2ResIni);
	MainMenuScreen::MENU_BG_OFFSET_Y += sf::IniReader::GetInt("MAINMENU", "MENU_BG_OFFSET_Y", -4, f2ResIni);
	MainMenuScreen::MAIN_MENU_SIZE = sf::IniReader::GetInt("MAINMENU", "MAIN_MENU_SIZE", 1, f2ResIni);

	SplashScreen::SPLASH_SCRN_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "SPLASH_SCRN_SIZE", 1, f2ResIni);
	HelpScreen::HELP_SCRN_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "HELP_SCRN_SIZE", 0, f2ResIni);
	DeathScreen::DEATH_SCRN_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "DEATH_SCRN_SIZE", 1, f2ResIni);
	SlidesScreen::END_SLIDE_SIZE = sf::IniReader::GetInt("STATIC_SCREENS", "END_SLIDE_SIZE", 1, f2ResIni);
	MoviesScreen::MOVIE_SIZE = sf::IniReader::GetInt("MOVIES", "MOVIE_SIZE", 1, f2ResIni);

	std::string x = sf::trim(sf::IniReader::GetString("MAPS", "SCROLL_DIST_X", "480", 16, f2ResIni));
	std::string y = sf::trim(sf::IniReader::GetString("MAPS", "SCROLL_DIST_Y", "400", 16, f2ResIni));
	ViewMap::SCROLL_DIST_X = (!x.compare(0, 9, "HALF_SCRN")) ? (SCR_WIDTH / 2) + 32 : std::atol(x.c_str());
	ViewMap::SCROLL_DIST_Y = (!y.compare(0, 9, "HALF_SCRN")) ? (SCR_HEIGHT / 2) + 24 : std::atol(y.c_str());

	ViewMap::IGNORE_PLAYER_SCROLL_LIMITS = (sf::IniReader::GetInt("MAPS", "IGNORE_PLAYER_SCROLL_LIMITS", 0, f2ResIni) != 0);
	ViewMap::IGNORE_MAP_EDGES = (sf::IniReader::GetInt("MAPS", "IGNORE_MAP_EDGES", 0, f2ResIni) != 0);
	ViewMap::EDGE_CLIPPING_ON = (sf::IniReader::GetInt("MAPS", "EDGE_CLIPPING_ON", 1, f2ResIni) != 0);

	IFaceBar::IFACE_BAR_MODE = sf::IniReader::GetInt("IFACE", "IFACE_BAR_MODE", 0, f2ResIni);
	IFaceBar::IFACE_BAR_SIDE_ART = sf::IniReader::GetInt("IFACE", "IFACE_BAR_SIDE_ART", 2, f2ResIni);
	IFaceBar::IFACE_BAR_WIDTH = sf::IniReader::GetInt("IFACE", "IFACE_BAR_WIDTH", (SCR_WIDTH >= 800) ? 800 : 640, f2ResIni);
	IFaceBar::IFACE_BAR_SIDES_ORI = (sf::IniReader::GetInt("IFACE", "IFACE_BAR_SIDES_ORI", 0, f2ResIni) != 0);

	Dialog::DIALOG_SCRN_ART_FIX = (sf::IniReader::GetInt("OTHER_SETTINGS", "DIALOG_SCRN_ART_FIX", 1, f2ResIni) != 0);
	Dialog::DIALOG_SCRN_BACKGROUND = (sf::IniReader::GetInt("OTHER_SETTINGS", "DIALOG_SCRN_BACKGROUND", 0, f2ResIni) != 0);

	if (sf::IniReader::GetInt("OTHER_SETTINGS", "BARTER_PC_INV_DROP_FIX", 1, f2ResIni)) {
		// barter_move_from_table_inventory_
		if (fo::var::getInt(0x47523D) == 80)  sf::SafeWrite32(0x47523D, 100); // x_start
		if (fo::var::getInt(0x475231) == 144) sf::SafeWrite32(0x475231, 164); // x_end
	}

	// add before sfall.dat and after critter.dat
	sf::LoadOrder::AddResourcePatches(
		sf::IniReader::GetString("Main", "f2_res_dat", "f2_res.dat", MAX_PATH, f2ResIni),
		sf::IniReader::GetString("Main", "f2_res_patches", "", MAX_PATH, f2ResIni)
	);

	/* Inject hacks */
	sf::SafeWrite32(0x482E30, FO_VAR_mapEntranceTileNum); // map_load_file_ (_tile_center_tile to _mapEntranceTileNum)

	if (SCR_WIDTH != 640 || SCR_HEIGHT != 480) {
		// Set the resolution for GNW95_init_mode_ex_
		sf::SafeWrite32(0x4CAD6B, SCR_WIDTH);  // 640
		sf::SafeWrite32(0x4CAD66, SCR_HEIGHT); // 480

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
}

}
