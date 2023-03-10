/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

#include <array>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"
#include "..\Utils.h"
#include "LoadGameHook.h"
#include "Worldmap.h"

#include "Interface.h"

namespace sfall
{

long Interface::ActiveInterfaceWID() {
	return LoadGameHook::interfaceWID;
}

enum WinNameType {
	Inventory = 0, // any inventory window (player/loot/use/barter)
	Dialog    = 1,
	PipBoy    = 2,
	WorldMap  = 3,
	IfaceBar  = 4, // the interface bar
	Character = 5,
	Skilldex  = 6,
	EscMenu   = 7, // escape menu
	Automap   = 8,

	// Inventory types
	Inven     = 50, // player inventory
	Loot      = 51,
	Use       = 53,
	Barter    = 54
};

fo::Window* Interface::GetWindow(long winType) {
	long winID = 0;
	switch (winType) {
	case WinNameType::Inventory:
		if (GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER)) winID = *fo::ptr::i_wid;
		break;
	case WinNameType::Dialog:
		if (GetLoopFlags() & DIALOG) winID = *fo::ptr::dialogueBackWindow;
		break;
	case WinNameType::PipBoy:
		if (GetLoopFlags() & PIPBOY) winID = *fo::ptr::pip_win;
		break;
	case WinNameType::WorldMap:
		if (GetLoopFlags() & WORLDMAP) winID = *fo::ptr::wmBkWin;
		break;
	case WinNameType::IfaceBar:
		winID = *fo::ptr::interfaceWindow;
		break;
	case WinNameType::Character:
		if (GetLoopFlags() & CHARSCREEN) winID = *fo::ptr::edit_win;
		break;
	case WinNameType::Skilldex:
		if (GetLoopFlags() & SKILLDEX) winID = *fo::ptr::skldxwin;
		break;
	case WinNameType::EscMenu:
		if (GetLoopFlags() & ESCMENU) winID = *fo::ptr::optnwin;
		break;
	case WinNameType::Automap:
		if (GetLoopFlags() & AUTOMAP) winID = ActiveInterfaceWID();
		break;
	default:
		return (fo::Window*)(-1); // unsupported type
	}
	return (winID > 0) ? fo::func::GNW_find(winID) : nullptr;
}

static long costAP = -1;
static void __declspec(naked) intface_redraw_items_hack0() {
	__asm {
		sub  eax, esi;
		shl  eax, 4;
		//
		cmp  dword ptr [esp + 0x80 - 0x5C + 4], 99; // art width
		jg   newArt;
		mov  edx, 10;  // width
		retn;
newArt:
		mov  costAP, edx;
		cmp  edx, 10;
		je   noShift;
		ja   skip;
		mov  edx, 10;  // width
		retn;
skip:
		sub  edx, 10;
		imul edx, 5;   // shift
		add  ebx, edx; // add width shift to 'from'
noShift:
		mov  edx, 15;  // width
		retn;
	}
}

static void __declspec(naked) intface_redraw_items_hack1() {
	__asm {
		mov  edx, 10;
		cmp  costAP, edx;
		jl   skip;
		add  edx, 5; // width 15
skip:
		retn;
	}
}

static void DrawActionPointsNumber() {
	MakeCall(0x4604B0, intface_redraw_items_hack0);
	MakeCall(0x460504, intface_redraw_items_hack1);
	SafeWrite16(0x4604D4, 0x9052); // push 10 > push edx
	SafeWrite8(0x46034B, 20);      // draw up to 19 AP
}

////////////////////////////// WORLDMAP INTERFACE //////////////////////////////

static int mapSlotsScrollMax = 27 * (17 - 7);
static int mapSlotsScrollLimit = 0;

static __declspec(naked) void ScrollCityListFix() {
	__asm {
		push ebx;
		mov  ebx, ds:[0x672F10]; // _wmLastTabsYOffset
		test eax, eax;
		jl   up;
		cmp  ebx, mapSlotsScrollMax;
		pop  ebx;
		jl   run;
		retn;
up:
		test ebx, ebx;
		pop  ebx;
		jnz  run;
		retn;
run:
		jmp  fo::funcoffs::wmInterfaceScrollTabsStart_;
	}
}

static void __declspec(naked) wmInterfaceInit_text_font_hook() {
	__asm {
		mov  eax, 101; // normal text font
		jmp  fo::funcoffs::text_font_;
	}
}

// const because no expanded world map patch
static const long wmapWinWidth = 640;
static const long wmapWinHeight = 480;
static const long wmapViewPortWidth = 450;
static const long wmapViewPortHeight = 443;

///////////////////////// FALLOUT 1 WORLD MAP FEATURES /////////////////////////
static bool showTerrainType = false;

enum DotStyleDefault {
	DotLen   = 2,
	SpaceLen = 2
};

enum TerrainHoverImage {
	width  = 200,
	height = 15,
	size = width * height,
	x_shift = (width / 4) + 25 // adjust x position
};

static std::tr1::array<unsigned char, TerrainHoverImage::size> wmTmpBuffer;
static bool isHoveringHotspot = false;
static bool backImageIsCopy = false;

struct DotPosition {
	long x;
	long y;
};
static std::vector<DotPosition> dots;

static unsigned char colorDot = 0;
static long spaceLen = DotStyleDefault::SpaceLen;
static long dotLen   = DotStyleDefault::DotLen;
static long dot_xpos = 0;
static long dot_ypos = 0;
static size_t terrainCount = 0;

static struct DotStyle {
	long dotLen;
	long spaceLen;
} *dotStyle = nullptr;

static void AddNewDot() {
	dot_xpos = *fo::ptr::world_xpos;
	dot_ypos = *fo::ptr::world_ypos;

	long* terrain = *(long**)FO_VAR_world_subtile;
	size_t id = (terrain) ? *terrain : 0;

	// Reinitialize if current terrain has smaller values than previous
	if (id < terrainCount) {
		if (dotLen > dotStyle[id].dotLen) dotLen = dotStyle[id].dotLen;
		if (spaceLen > dotStyle[id].spaceLen) spaceLen = dotStyle[id].spaceLen;
	}

	if (dotLen <= 0 && spaceLen) {
		spaceLen--;
		if (!spaceLen) { // set dot length
			dotLen = (id < terrainCount) ? dotStyle[id].dotLen : DotStyleDefault::DotLen;
		};
		return;
	}
	dotLen--;
	spaceLen = (id < terrainCount) ? dotStyle[id].spaceLen : DotStyleDefault::SpaceLen;

	DotPosition dot;
	dot.x = dot_xpos;
	dot.y = dot_ypos;
	dots.push_back(dot);
}

static void __declspec(naked) DrawingDots() {
	long x_offset,  y_offset;
	__asm {
		mov ebp, esp; // prolog
		sub esp, __LOCAL_SIZE;
	}

	if (dot_xpos != *fo::ptr::world_xpos || dot_ypos != *fo::ptr::world_ypos) {
		AddNewDot();
	}
	x_offset = 22 - *fo::ptr::wmWorldOffsetX;
	y_offset = 21 - *fo::ptr::wmWorldOffsetY;

	for (std::vector<DotPosition>::const_iterator it = dots.begin(); it != dots.end(); ++it) { // redraws all dots
		if (it->x < *fo::ptr::wmWorldOffsetX || it->y < *fo::ptr::wmWorldOffsetY) continue; // the pixel is out of viewport
		if (it->x > *fo::ptr::wmWorldOffsetX + wmapViewPortWidth || it->y > *fo::ptr::wmWorldOffsetY + wmapViewPortHeight) continue;

		long wmPixelX = (it->x + x_offset);
		long wmPixelY = (it->y + y_offset);

		wmPixelY *= wmapWinWidth;

		BYTE* wmWinBuf = *fo::ptr::wmBkWinBuf;
		BYTE* wmWinBuf_xy = (wmPixelY + wmPixelX) + wmWinBuf;

		// put pixel to interface window buffer
		if (wmWinBuf_xy > wmWinBuf) *wmWinBuf_xy = colorDot;

		// TODO: fix dots for car travel
	}
	__asm {
		mov esp, ebp; // epilog
		retn;
	}
}

static bool PrintHotspotText(long x, long y, bool backgroundCopy = false) {
	long area = *fo::ptr::WorldMapCurrArea;
	char* text = (area != -1 || !showTerrainType) ? (char*)Worldmap::GetCustomAreaTitle(area) : (char*)Worldmap::GetCurrentTerrainName();
	if (!text) return false;

	if (backgroundCopy) { // copy background image to memory (size 200 x 15)
		backImageIsCopy = true;
		fo::util::SurfaceCopyToMem(x - TerrainHoverImage::x_shift, y, TerrainHoverImage::width, TerrainHoverImage::height, wmapWinWidth, *fo::ptr::wmBkWinBuf, wmTmpBuffer.data());
	}

	long txtWidth = fo::util::GetTextWidthFM(text);
	if (txtWidth > TerrainHoverImage::width) txtWidth = TerrainHoverImage::width;

	// offset text position
	y += 4;
	x += 25 - (txtWidth / 2);

	// prevent printing text outside of viewport
	/*if ((x + txtWidth) > wmapViewPortWidth - 20) {
		txtWidth -= (x + txtWidth) - wmapViewPortWidth - 20;
	} else if (x < 20) {
		long x_cut = abs(20 - x);
		long width = 0;
		do {
			width += fo::util::GetCharWidthFM(*text++);
		} while (width < x_cut);
		x += x_cut;
	}*/

	fo::util::PrintTextFM(text, 228, x, y, txtWidth, wmapWinWidth, *fo::ptr::wmBkWinBuf); // shadow
	fo::util::PrintTextFM(text, 215, x - 1, y - 1, txtWidth, wmapWinWidth, *fo::ptr::wmBkWinBuf);

	if (backgroundCopy) fo::func::wmRefreshInterfaceOverlay(0); // prevent printing text over the interface
	return true;
}

static void __declspec(naked) wmInterfaceRefresh_hook() {
	if (colorDot && *fo::ptr::target_xpos != -1) {
		if (*fo::ptr::In_WorldMap) {
			DrawingDots();
		} else if (!*fo::ptr::target_xpos && !*fo::ptr::target_ypos) {
			// player stops moving
			dots.clear();
			// Reinitialize on next AddNewDot
			if (terrainCount)
				dotLen = spaceLen = 99;
			else {
				dotLen = DotStyleDefault::DotLen;
				spaceLen = DotStyleDefault::SpaceLen;
			}
		}
	}
	if (isHoveringHotspot && !*fo::ptr::In_WorldMap) {
		PrintHotspotText(*fo::ptr::world_xpos - *fo::ptr::wmWorldOffsetX, *fo::ptr::world_ypos - *fo::ptr::wmWorldOffsetY);
		isHoveringHotspot = backImageIsCopy = false;
	}
	__asm jmp fo::funcoffs::wmDrawCursorStopped_;
}

static void __fastcall wmDetectHotspotHover(long wmMouseX, long wmMouseY) {
	if (!showTerrainType && Worldmap::AreaTitlesIsEmpty()) return;

	long deltaX = 20, deltaY = 20;

	// mouse cursor is out of viewport area (the zero values of wmMouseX and wmMouseY correspond to the top-left corner of the worldmap interface)
	if ((wmMouseX < 20 || wmMouseY < 20 || wmMouseX > wmapViewPortWidth + 15 || wmMouseY > wmapViewPortHeight + 20) == false) {
		deltaX = abs((long)*fo::ptr::world_xpos - (wmMouseX - deltaX + *fo::ptr::wmWorldOffsetX));
		deltaY = abs((long)*fo::ptr::world_ypos - (wmMouseY - deltaY + *fo::ptr::wmWorldOffsetY));
	}

	bool isHovered = isHoveringHotspot;
	isHoveringHotspot = deltaX < 8 && deltaY < 6;
	if (isHoveringHotspot != isHovered) { // if value has changed
		// upper left corner
		long y = *fo::ptr::world_ypos - *fo::ptr::wmWorldOffsetY;
		long x = *fo::ptr::world_xpos - *fo::ptr::wmWorldOffsetX;
		long x_offset = x - TerrainHoverImage::x_shift;
		if (!backImageIsCopy) {
			if (!PrintHotspotText(x, y, true)) return;
		} else {
			// restore background image
			fo::util::DrawToSurface(x_offset, y, TerrainHoverImage::width, TerrainHoverImage::height, wmapWinWidth, wmapWinHeight, *fo::ptr::wmBkWinBuf, wmTmpBuffer.data());
			backImageIsCopy = false;
		}
		// redraw rectangle on worldmap interface
		RECT rect;
		rect.top = y;
		rect.left = x_offset;
		rect.right = x + TerrainHoverImage::width;
		rect.bottom = y + TerrainHoverImage::height;
		fo::func::win_draw_rect(*fo::ptr::wmBkWin, &rect);
	}
}

static void __declspec(naked) wmWorldMap_hack() {
	__asm {
		cmp  ds:[FO_VAR_In_WorldMap], 1; // player is moving
		jne  checkHover;
		mov  eax, dword ptr ds:[FO_VAR_wmWorldOffsetY]; // overwritten code
		retn;
checkHover:
		cmp  esi, 328;
		je   isScroll;
		cmp  esi, 331;
		je   isScroll;
		cmp  esi, 333;
		je   isScroll;
		cmp  esi, 336;
		je   isScroll;
		push ecx;
		mov  ecx, [esp + 0x38 - 0x30 + 8]; // x
		mov  edx, [esp + 0x38 - 0x34 + 8]; // y
		call wmDetectHotspotHover;
		pop  ecx;
		mov  eax, dword ptr ds:[FO_VAR_wmWorldOffsetY];
		retn;
isScroll:
		mov  isHoveringHotspot, 0;
		mov  backImageIsCopy, 0;
		mov  eax, dword ptr ds:[FO_VAR_wmWorldOffsetY];
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void __declspec(naked) wmInterfaceRefreshCarFuel_hack_empty() {
	__asm {
		mov byte ptr [eax - 1], 13;
		mov byte ptr [eax + 1], 13;
		add eax, wmapWinWidth;
		dec ebx;
		mov byte ptr [eax], 14;
		mov byte ptr [eax - 1], 15;
		mov byte ptr [eax + 1], 15;
		add eax, wmapWinWidth;
		retn;
	}
}

static void __declspec(naked) wmInterfaceRefreshCarFuel_hack() {
	__asm {
		mov byte ptr [eax - 1], 196;
		mov byte ptr [eax + 1], 196;
		add eax, wmapWinWidth;
		mov byte ptr [eax - 1], 200;
		mov byte ptr [eax + 1], 200;
		retn;
	}
}

static void __declspec(naked) wmInterfaceInit_hook() {
	static DWORD retAddr;
	__asm {
		pop  retAddr;
		call fo::funcoffs::win_register_button_;
		push eax;
		mov  ebx, fo::funcoffs::gsound_red_butt_release_;
		mov  edx, fo::funcoffs::gsound_red_butt_press_;
		call fo::funcoffs::win_register_button_sound_func_;
		pop  eax;
		jmp  retAddr;
	}
}

static void __declspec(naked) wmWorldMap_hook() {
	__asm {
		push eax;
		mov  eax, 0x503E14; // 'ib1p1xx1'
		call fo::funcoffs::gsound_play_sfx_file_;
		pop  eax;
		jmp  fo::funcoffs::wmPartyInitWalking_;
	}
}

static void __declspec(naked) wmDrawCursorStopped_hack_hotspot() {
	__asm {
		mov  eax, 0x503E34; // 'ib2p1xx1'
		call fo::funcoffs::gsound_play_sfx_file_;
		mov  eax, dword ptr ds:[0x672E90]; // hotspot2_pic
		retn;
	}
}

static void __declspec(naked) wmTownMapInit_hack() {
	__asm {
		mov  dword ptr ds:[edi + 0x672DD8], eax; // _wmTownMapButtonId
		push eax;
		mov  edx, fo::funcoffs::gsound_med_butt_press_;
		xor  ebx, ebx; // no button release sfx
		call fo::funcoffs::win_register_button_sound_func_;
		pop  eax;
		retn;
	}
}

static void WorldMapInterfacePatch() {
	if (IniReader::GetConfigInt("Misc", "WorldMapFontPatch", 0)) {
		dlogr("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
	}

	// Add missing sounds for the buttons on the world map interface (wmInterfaceInit_)
	const DWORD wmInterfaceBtnsAddr[] = {
		0x4C2BF4, // location labels
		0x4C2BB5, // town/world
		0x4C2D4C, // up
		0x4C2D8A  // down
	};
	HookCalls(wmInterfaceInit_hook, wmInterfaceBtnsAddr);
	HookCall(0x4C02DA, wmWorldMap_hook); // destination marker
	MakeCall(0x4C4257, wmDrawCursorStopped_hack_hotspot); // triangle markers on the world map
	MakeCall(0x4C4B94, wmTownMapInit_hack, 1);            // triangle markers on the town map

	// Fix images for up/down buttons
	SafeWrite32(0x4C2C0A, 199); // index of UPARWOFF.FRM
	SafeWrite8(0x4C2C7C, 0x43); // dec ebx > inc ebx
	SafeWrite32(0x4C2C92, 181); // index of DNARWOFF.FRM
	SafeWrite8(0x4C2D04, 0x46); // dec esi > inc esi

	//if (IniReader::GetConfigInt("Misc", "WorldMapCitiesListFix", 0)) {
		dlogr("Applying world map cities list patch.", DL_INIT);
		const DWORD scrollCityListAddr[] = {0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D};
		HookCalls(ScrollCityListFix, scrollCityListAddr);
	//}

	DWORD wmSlots = IniReader::GetConfigInt("Misc", "WorldMapSlots", 0);
	if (wmSlots && wmSlots < 128) {
		dlogr("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27; // height value after which scrolling is not possible
		mapSlotsScrollLimit = wmSlots * 27;
		SafeWrite32(0x4C21FD, 189); // 27 * 7
		SafeWrite32(0x4C21F1, (DWORD)&mapSlotsScrollLimit);
	}

	// Fallout 1 features, travel markers and displaying terrain types or town titles
	if (IniReader::GetConfigInt("Interface", "WorldMapTravelMarkers", 0)) {
		dlogr("Applying world map travel markers patch.", DL_INIT);

		int color = IniReader::GetConfigInt("Interface", "TravelMarkerColor", 134); // color index in palette: R = 224, G = 0, B = 0
		if (color > 228) color = 228; else if (color < 1) color = 1; // no palette animation colors
		colorDot = color;

		std::vector<std::string> dotList = IniReader::GetConfigList("Interface", "TravelMarkerStyles", "", 512);
		if (!dotList.empty()) {
			terrainCount = dotList.size();
			dotStyle = new DotStyle[terrainCount];

			std::vector<std::string> pair;
			for (size_t i = 0; i < terrainCount; i++) {
				split(dotList[i], ':', std::back_inserter(pair), 2);
				if (pair.size() >= 2) {
					int len = atoi(pair[0].c_str());
					if (len < 1) len = 1; else if (len > 10) len = 10;
					dotStyle[i].dotLen = len;
					len = atoi(pair[1].c_str());
					if (len < 1) len = 1; else if (len > 10) len = 10;
					dotStyle[i].spaceLen = len;
				} else {
					dotStyle[i].dotLen = DotStyleDefault::DotLen;
					dotStyle[i].spaceLen = DotStyleDefault::SpaceLen;
				}
				pair.clear();
			}
		}
		dots.reserve(512);
	}
	showTerrainType = (IniReader::GetConfigInt("Interface", "WorldMapTerrainInfo", 0) != 0);
	HookCall(0x4C3C7E, wmInterfaceRefresh_hook); // when calling wmDrawCursorStopped_
	MakeCall(0x4BFE84, wmWorldMap_hack);

	// Car fuel gauge graphics patch
	MakeCall(0x4C528A, wmInterfaceRefreshCarFuel_hack_empty);
	MakeCall(0x4C529E, wmInterfaceRefreshCarFuel_hack);
	SafeWrite8(0x4C52A8, 197);
	SafeWrite8(0x4C5289, 12);
}

static void __declspec(naked) intface_rotate_numbers_hack() {
	__asm {
		push edi;
		push ebp;
		sub  esp, 0x54;
		mov  edi, 0x460BA6;
		// ebx - old value, ecx - new value
		cmp  ebx, ecx;
		je   end;
		mov  ebx, ecx;
		jg   decrease;
		dec  ebx;
end:
		jmp  edi;
decrease:
		test ecx, ecx;
		jl   negative;
		inc  ebx;
		jmp  edi;
negative:
		xor  ebx, ebx;
		jmp  edi;
	}
}

static void SpeedInterfaceCounterAnimsPatch() {
	switch (IniReader::GetConfigInt("Misc", "SpeedInterfaceCounterAnims", 0)) {
	case 1:
		dlogr("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
		MakeJump(0x460BA1, intface_rotate_numbers_hack);
		break;
	case 2:
		dlogr("Applying SpeedInterfaceCounterAnims patch (Instant).", DL_INIT);
		SafeWrite32(0x460BB6, 0xDB319090); // xor ebx, ebx
		break;
	}
}

static bool IFACE_BAR_MODE = false;

static long gmouse_handle_event_hook() {
	long countWin = *fo::ptr::num_windows;
	long ifaceWin = *fo::ptr::interfaceWindow;
	fo::Window* win = nullptr;

	for (int n = 1; n < countWin; n++) {
		win = fo::ptr::window[n];
		if ((win->wID == ifaceWin || (win->flags & fo::WinFlags::ScriptWindow && !(win->flags & fo::WinFlags::Transparent))) // also check the script windows
			&& !(win->flags & fo::WinFlags::Hidden)) {
			RECT *rect = &win->wRect;
			if (fo::func::mouse_click_in(rect->left, rect->top, rect->right, rect->bottom)) return 0; // 0 - block clicking in the window area
		}
	}

	if (IFACE_BAR_MODE) return 1;

	// if IFACE_BAR_MODE is not enabled, check the display_win window area
	win = fo::func::GNW_find(fo::var::getInt(FO_VAR_display_win));
	RECT *rect = &win->wRect;
	return fo::func::mouse_click_in(rect->left, rect->top, rect->right, rect->bottom); // 1 - click in the display window area
}

static void __declspec(naked) gmouse_bk_process_hook() {
	__asm {
		push 1; // bypass Transparent
		mov  ecx, eax;
		call fo::util::GetTopWindowAtPos;
		mov  eax, [eax]; // wID
		retn;
	}
}

static void __declspec(naked) intface_update_ammo_lights_hack() {
	__asm {
		mov  eax, 70; // 70 - full ammo bar
		cmp  edx, eax;
		cmovg edx, eax;
		mov  eax, 463; // overwritten engine code
		retn;
	}
}

static void __declspec(naked) display_body_hook() {
	__asm {
		mov  ebx, [esp + 0x60 - 0x28 + 8];
		cmp  ebx, 1; // check mode 0 or 1
		jbe  fix;
		xor  ebx, ebx;
		jmp  fo::funcoffs::art_id_;
fix:
		dec  edx;     // USE.FRM
		mov  ecx, 48; // INVBOX.FRM
		test ebx, ebx;
		cmovz edx, ecx;
		xor  ebx, ebx;
		xor  ecx, ecx;
		jmp  fo::funcoffs::art_id_;
	}
}

static void InterfaceWindowPatch() {
	// Remove MoveOnTop flag for interfaces
	SafeWrite8(0x46ECE9, (*(BYTE*)0x46ECE9) ^ fo::WinFlags::MoveOnTop); // Player Inventory/Loot/UseOn
	SafeWrite8(0x41B966, (*(BYTE*)0x41B966) ^ fo::WinFlags::MoveOnTop); // Automap

	// Set OwnerFlag flag
	SafeWrite8(0x4D5EBF, fo::WinFlags::OwnerFlag); // win_init_ (main win)
	SafeWrite8(0x481CEC, (*(BYTE*)0x481CEC) | fo::WinFlags::OwnerFlag); // _display_win (map win)
	SafeWrite8(0x44E7D2, (*(BYTE*)0x44E7D2) | fo::WinFlags::OwnerFlag); // gmovie_play_ (movie win)

	// Remove OwnerFlag flag
	SafeWrite8(0x4B801B, (*(BYTE*)0x4B801B) ^ fo::WinFlags::OwnerFlag); // createWindow_
	// Remove OwnerFlag and Transparent flags
	SafeWrite8(0x42F869, (*(BYTE*)0x42F869) ^ (fo::WinFlags::Transparent | fo::WinFlags::OwnerFlag)); // addWindow_

	// Cosmetic fix for the background image of the character portrait on the player's inventory screen
	HookCall(0x47093C, display_body_hook);
	BYTE code[11] = {
		0x8B, 0xD3,             // mov  edx, ebx
		0x66, 0x8B, 0x58, 0xF4, // mov  bx, [eax - 12] [sizeof(frame)]
		0x0F, 0xAF, 0xD3,       // imul edx, ebx (y * frame width)
		0x53, 0x90              // push ebx (frame width)
	};
	SafeWriteBytes(0x470971, code, 11); // calculates the offset in the pixel array for x/y coordinates

	// Increase the max text width of the player name on the character screen
	const DWORD printBignameAddr[] = {0x435160, 0x435189}; // 100 (PrintBigname_)
	SafeWriteBatch<BYTE>(127, printBignameAddr);

	// Increase the max text width of the information card on the character screen
	const DWORD drawCardAddr[] = {0x43ACD5, 0x43DD37}; // 136, 133 (DrawCard_, DrawCard2_)
	SafeWriteBatch<BYTE>(146, drawCardAddr);

	// Increase the width of the mouse drop area from 64px to 80px for the PC's and NPC's inventory on the barter screen
	// barter_move_from_table_inventory_
	SafeWrite32(0x47523D, 98);       // x_start was 80
	SafeWrite32(0x475231, 98 + 80);  // x_end   was 144
	SafeWrite32(0x4752BE, 460);      // x_start was 475
	SafeWrite32(0x4752B2, 460 + 80); // x_end   was 539
}

static void InventoryCharacterRotationSpeedPatch() {
	SimplePatch<DWORD>(0x47066B, "Misc", "SpeedInventoryPCRotation", 166, 0, 1000);
}

static void UIAnimationSpeedPatch() {
	DWORD addrs[] = {
		0x45F9DE, 0x45FB33,
		0x447DF4, 0x447EB6,
		0x499B99, 0x499DA8
	};
	SimplePatch<WORD>(addrs, 2, "Misc", "CombatPanelAnimDelay", 1000, 0, 65535);
	SimplePatch<BYTE>(&addrs[2], 2, "Misc", "DialogPanelAnimDelay", 33, 0, 255);
	SimplePatch<BYTE>(&addrs[4], 2, "Misc", "PipboyTimeAnimDelay", 50, 0, 127);
}

void Interface::OnGameLoad() {
	dots.clear();
}

void Interface::init() {
	InterfaceWindowPatch();
	InventoryCharacterRotationSpeedPatch();
	UIAnimationSpeedPatch();

	// Remove window position rounding for script-created windows
	//if (IniReader::GetConfigInt("Misc", "RemoveWindowRounding", 1)) {
		const DWORD windowRoundingAddr[] = {0x4D6EDD, 0x4D6F12};
		SafeWriteBatch<BYTE>(CodeType::JumpShort, windowRoundingAddr);
	//}

	DrawActionPointsNumber();
	WorldMapInterfacePatch();
	SpeedInterfaceCounterAnimsPatch();

	// Fix for interface windows with 'Transparent', 'Hidden' and 'ScriptWindow' flags
	// Transparent/Hidden - will not toggle the mouse cursor when the cursor hovers over a transparent/hidden window
	// ScriptWindow - prevents the player from moving when clicking on the window if the 'Transparent' flag is not set
	HookCall(0x44B737, gmouse_bk_process_hook);
	HookCall(0x44C018, gmouse_handle_event_hook); // replaces hack function from HRP by Mash
	if (hrpVersionValid) IFACE_BAR_MODE = (GetIntHRPValue(HRP_VAR_IFACE_BAR_MODE) != 0);

	// Fix crash when the player equips a weapon overloaded with ammo (ammo bar overflow)
	MakeCall(0x45F94F, intface_update_ammo_lights_hack);
}

void Interface::exit() {
	if (dotStyle) delete[] dotStyle;
}

}
