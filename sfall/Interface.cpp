/*
 *    sfall
 *    Copyright (C) 2008-2019  The sfall team
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

#include "main.h"
#include "FalloutEngine.h"
#include "LoadGameHook.h"
#include "Utils.h"
#include "Worldmap.h"

long Interface_ActiveInterfaceWID() {
	return LoadGameHook_interfaceWID;
}

enum WinNameType {
	WINTYPE_Inventory = 0, // any inventory window (player/loot/use/barter)
	WINTYPE_Dialog    = 1,
	WINTYPE_PipBoy    = 2,
	WINTYPE_WorldMap  = 3,
	WINTYPE_IfaceBar  = 4, // the interface bar
	WINTYPE_Character = 5,
	WINTYPE_Skilldex  = 6,
	WINTYPE_EscMenu   = 7, // escape menu
	WINTYPE_Automap   = 8,
};

WINinfo* Interface_GetWindow(long winType) {
	long winID = 0;
	switch (winType) {
	case WINTYPE_Inventory:
		if (GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER)) winID = *ptr_i_wid;
		break;
	case WINTYPE_Dialog:
		if (GetLoopFlags() & DIALOG) winID = *ptr_dialogueBackWindow;
		break;
	case WINTYPE_PipBoy:
		if (GetLoopFlags() & PIPBOY) winID = *ptr_pip_win;
		break;
	case WINTYPE_WorldMap:
		if (GetLoopFlags() & WORLDMAP) winID = *ptr_wmBkWin;
		break;
	case WINTYPE_IfaceBar:
		winID = *ptr_interfaceWindow;
		break;
	case WINTYPE_Character:
		if (GetLoopFlags() & CHARSCREEN) winID = *ptr_edit_win;
		break;
	case WINTYPE_Skilldex:
		if (GetLoopFlags() & SKILLDEX) winID = *ptr_skldxwin;
		break;
	case WINTYPE_EscMenu:
		if (GetLoopFlags() & ESCMENU) winID = *ptr_optnwin;
		break;
	case WINTYPE_Automap:
		if (GetLoopFlags() & AUTOMAP) winID = Interface_ActiveInterfaceWID();
		break;
	default:
		return (WINinfo*)(-1);
	}
	return (winID > 0) ? GNWFind(winID) : nullptr;
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
		jmp  wmInterfaceScrollTabsStart_;
	}
}

static void __declspec(naked) wmInterfaceInit_text_font_hook() {
	__asm {
		mov  eax, 101; // normal text font
		jmp  text_font_;
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
	STYDEF_DotLen   = 2,
	STYDEF_SpaceLen = 2
};

enum TerrainHoverImage {
	HVRIMG_width  = 200,
	HVRIMG_height = 15,
	HVRIMG_size = HVRIMG_width * HVRIMG_height,
	HVRIMG_x_shift = (HVRIMG_width / 4) + 25 // adjust x position
};

static std::array<unsigned char, HVRIMG_size> wmTmpBuffer;
static bool isHoveringHotspot = false;
static bool backImageIsCopy = false;

struct DotPosition {
	long x;
	long y;
};
static std::vector<DotPosition> dots;

static unsigned char colorDot = 0;
static long spaceLen = STYDEF_SpaceLen;
static long dotLen   = STYDEF_DotLen;
static long dot_xpos = 0;
static long dot_ypos = 0;
static size_t terrainCount = 0;

static struct DotStyle {
	long dotLen;
	long spaceLen;
} *dotStyle = nullptr;

static void AddNewDot() {
	dot_xpos = *ptr_world_xpos;
	dot_ypos = *ptr_world_ypos;

	long* terrain = *(long**)_world_subtile;
	size_t id = (terrain) ? *terrain : 0;

	// Reinitialize if current terrain has smaller values than previous
	if (id < terrainCount) {
		if (dotLen > dotStyle[id].dotLen) dotLen = dotStyle[id].dotLen;
		if (spaceLen > dotStyle[id].spaceLen) spaceLen = dotStyle[id].spaceLen;
	}

	if (dotLen <= 0 && spaceLen) {
		spaceLen--;
		if (!spaceLen) { // set dot length
			dotLen = (id < terrainCount) ? dotStyle[id].dotLen : STYDEF_DotLen;
		};
		return;
	}
	dotLen--;
	spaceLen = (id < terrainCount) ? dotStyle[id].spaceLen : STYDEF_SpaceLen;

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

	if (dot_xpos != *ptr_world_xpos || dot_ypos != *ptr_world_ypos) {
		AddNewDot();
	}
	x_offset = 22 - *ptr_wmWorldOffsetX;
	y_offset = 21 - *ptr_wmWorldOffsetY;

	for (std::vector<DotPosition>::const_iterator it = dots.begin(); it != dots.end(); ++it) { // redraws all dots
		if (it->x < *ptr_wmWorldOffsetX || it->y < *ptr_wmWorldOffsetY) continue; // the pixel is out of viewport
		if (it->x > *ptr_wmWorldOffsetX + wmapViewPortWidth || it->y > *ptr_wmWorldOffsetY + wmapViewPortHeight) continue;

		long wmPixelX = (it->x + x_offset);
		long wmPixelY = (it->y + y_offset);

		wmPixelY *= wmapWinWidth;

		BYTE* wmWinBuf = *ptr_wmBkWinBuf;
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
	long area = *ptr_WorldMapCurrArea;
	char* text = (area != -1 || !showTerrainType) ? (char*)Worldmap_GetCustomAreaTitle(area) : (char*)Worldmap_GetCurrentTerrainName();
	if (!text) return false;

	if (backgroundCopy) { // copy background image to memory (size 200 x 15)
		backImageIsCopy = true;
		SurfaceCopyToMem(x - HVRIMG_x_shift, y, HVRIMG_width, HVRIMG_height, wmapWinWidth, *ptr_wmBkWinBuf, wmTmpBuffer.data());
	}

	long txtWidth = GetTextWidthFM(text);
	if (txtWidth > HVRIMG_width) txtWidth = HVRIMG_width;

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
			width += GetCharWidthFM(*text++);
		} while (width < x_cut);
		x += x_cut;
	}*/

	PrintTextFM(text, 228, x, y, txtWidth, wmapWinWidth, *ptr_wmBkWinBuf); // shadow
	PrintTextFM(text, 215, x - 1, y - 1, txtWidth, wmapWinWidth, *ptr_wmBkWinBuf);

	if (backgroundCopy) WmRefreshInterfaceOverlay(0); // prevent printing text over the interface
	return true;
}

static void __declspec(naked) wmInterfaceRefresh_hook() {
	if (colorDot && *ptr_target_xpos != -1) {
		if (*ptr_In_WorldMap) {
			DrawingDots();
		} else if (!*ptr_target_xpos && !*ptr_target_ypos) {
			// player stops moving
			dots.clear();
			// Reinitialize on next AddNewDot
			if (terrainCount)
				dotLen = spaceLen = 99;
			else {
				dotLen = STYDEF_DotLen;
				spaceLen = STYDEF_SpaceLen;
			}
		}
	}
	if (isHoveringHotspot && !*ptr_In_WorldMap) {
		PrintHotspotText(*ptr_world_xpos - *ptr_wmWorldOffsetX, *ptr_world_ypos - *ptr_wmWorldOffsetY);
		isHoveringHotspot = backImageIsCopy = false;
	}
	__asm jmp wmDrawCursorStopped_;
}

static void __fastcall wmDetectHotspotHover(long wmMouseX, long wmMouseY) {
	if (!showTerrainType && Worldmap_AreaTitlesIsEmpty()) return;

	long deltaX = 20, deltaY = 20;

	// mouse cursor is out of viewport area (the zero values of wmMouseX and wmMouseY correspond to the top-left corner of the worldmap interface)
	if ((wmMouseX < 20 || wmMouseY < 20 || wmMouseX > wmapViewPortWidth + 15 || wmMouseY > wmapViewPortHeight + 20) == false) {
		deltaX = abs((long)*ptr_world_xpos - (wmMouseX - deltaX + *ptr_wmWorldOffsetX));
		deltaY = abs((long)*ptr_world_ypos - (wmMouseY - deltaY + *ptr_wmWorldOffsetY));
	}

	bool isHovered = isHoveringHotspot;
	isHoveringHotspot = deltaX < 8 && deltaY < 6;
	if (isHoveringHotspot != isHovered) { // if value has changed
		// upper left corner
		long y = *ptr_world_ypos - *ptr_wmWorldOffsetY;
		long x = *ptr_world_xpos - *ptr_wmWorldOffsetX;
		long x_offset = x - HVRIMG_x_shift;
		if (!backImageIsCopy) {
			if (!PrintHotspotText(x, y, true)) return;
		} else {
			// restore background image
			DrawToSurface(x_offset, y, HVRIMG_width, HVRIMG_height, wmapWinWidth, wmapWinHeight, *ptr_wmBkWinBuf, wmTmpBuffer.data());
			backImageIsCopy = false;
		}
		// redraw rectangle on worldmap interface
		RECT rect;
		rect.top = y;
		rect.left = x_offset;
		rect.right = x + HVRIMG_width;
		rect.bottom = y + HVRIMG_height;
		WinDrawRect(*ptr_wmBkWin, &rect);
	}
}

static void __declspec(naked) wmWorldMap_hack() {
	__asm {
		cmp  ds:[_In_WorldMap], 1; // player is moving
		jne  checkHover;
		mov  eax, dword ptr ds:[_wmWorldOffsetY]; // overwritten code
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
		mov  eax, dword ptr ds:[_wmWorldOffsetY];
		retn;
isScroll:
		mov  isHoveringHotspot, 0;
		mov  backImageIsCopy, 0;
		mov  eax, dword ptr ds:[_wmWorldOffsetY];
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

static void WorldMapInterfacePatch() {
	if (GetConfigInt("Misc", "WorldMapFontPatch", 0)) {
		dlog("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
		dlogr(" Done", DL_INIT);
	}

	// Fix images for up/down buttons
	SafeWrite32(0x4C2C0A, 199); // index of UPARWOFF.FRM
	SafeWrite8(0x4C2C7C, 0x43); // dec ebx > inc ebx
	SafeWrite32(0x4C2C92, 181); // index of DNARWOFF.FRM
	SafeWrite8(0x4C2D04, 0x46); // dec esi > inc esi

	//if (GetConfigInt("Misc", "WorldMapCitiesListFix", 0)) {
		dlog("Applying world map cities list patch.", DL_INIT);
		const DWORD scrollCityListAddr[] = {0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D};
		HookCalls(ScrollCityListFix, scrollCityListAddr);
		dlogr(" Done", DL_INIT);
	//}

	DWORD wmSlots = GetConfigInt("Misc", "WorldMapSlots", 0);
	if (wmSlots && wmSlots < 128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27; // height value after which scrolling is not possible
		mapSlotsScrollLimit = wmSlots * 27;
		SafeWrite32(0x4C21FD, 189); // 27 * 7
		SafeWrite32(0x4C21F1, (DWORD)&mapSlotsScrollLimit);
		dlogr(" Done", DL_INIT);
	}

	// Fallout 1 features, travel markers and displaying terrain types or town titles
	if (GetConfigInt("Interface", "WorldMapTravelMarkers", 0)) {
		dlog("Applying world map travel markers patch.", DL_INIT);

		int color = GetConfigInt("Interface", "TravelMarkerColor", 134); // color index in palette: R = 224, G = 0, B = 0
		if (color > 228) color = 228; else if (color < 1) color = 1; // no palette animation colors
		colorDot = color;

		std::vector<std::string> dotList = GetConfigList("Interface", "TravelMarkerStyles", "", 512);
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
					dotStyle[i].dotLen = STYDEF_DotLen;
					dotStyle[i].spaceLen = STYDEF_SpaceLen;
				}
				pair.clear();
			}
		}
		dots.reserve(512);
		dlogr(" Done", DL_INIT);
	}
	showTerrainType = (GetConfigInt("Interface", "WorldMapTerrainInfo", 0) != 0);
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
	switch (GetConfigInt("Misc", "SpeedInterfaceCounterAnims", 0)) {
	case 1:
		dlog("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
		MakeJump(0x460BA1, intface_rotate_numbers_hack);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying SpeedInterfaceCounterAnims patch (Instant).", DL_INIT);
		SafeWrite32(0x460BB6, 0xDB319090); // xor ebx, ebx
		dlogr(" Done", DL_INIT);
		break;
	}
}

static bool IFACE_BAR_MODE = false;
static long gmouse_handle_event_hook() {
	long countWin = *ptr_num_windows;
	long ifaceWin = *ptr_interfaceWindow;
	WINinfo* win = nullptr;

	for (int n = 1; n < countWin; n++) {
		win = ptr_window[n];
		if ((win->wID == ifaceWin || (win->flags & WinFlags::ScriptWindow && !(win->flags & WinFlags::Transparent))) // also check the script windows
			&& !(win->flags & WinFlags::Hidden)) {
			RECT *rect = &win->wRect;
			if (MouseClickIn(rect->left, rect->top, rect->right, rect->bottom)) return 0; // 0 - block clicking in the window area
		}
	}
	if (IFACE_BAR_MODE) return 1;
	// if IFACE_BAR_MODE is not enabled, check the display_win window area
	win = GNWFind(*(DWORD*)_display_win);
	RECT *rect = &win->wRect;
	return MouseClickIn(rect->left, rect->top, rect->right, rect->bottom); // 1 - click in the display window area
}

static void __declspec(naked) gmouse_bk_process_hook() {
	__asm {
		push 1; // bypass Transparent
		mov  ecx, eax;
		call GetTopWindowAtPos;
		mov  eax, [eax]; // wID
		retn;
	}
}

static void __declspec(naked) main_death_scene_hook() {
	__asm {
		mov  eax, 101;
		call text_font_;
		jmp  debug_printf_;
	}
}

static void __declspec(naked) display_body_hook() {
	__asm {
		mov  ebx, [esp + 0x60 - 0x28 + 8];
		cmp  ebx, 1; // check mode 0 or 1
		jbe  fix;
		xor  ebx, ebx;
		jmp  art_id_;
fix:
		dec  edx;     // USE.FRM
		mov  ecx, 48; // INVBOX.FRM
		test ebx, ebx;
		cmovz edx, ecx;
		xor  ebx, ebx;
		xor  ecx, ecx;
		jmp  art_id_;
	}
}

void Interface_OnBeforeGameInit() {
	if (hrpVersionValid) IFACE_BAR_MODE = *(BYTE*)HRPAddress(0x1006EB0C) != 0;
	HookCall(0x44C018, gmouse_handle_event_hook); // replaces hack function from HRP
}

void Interface_OnGameLoad() {
	dots.clear();
}

void Interface_Init() {
	DrawActionPointsNumber();
	WorldMapInterfacePatch();
	SpeedInterfaceCounterAnimsPatch();

	// Fix for interface windows with 'Transparent', 'Hidden' and 'ScriptWindow' flags
	// Transparent/Hidden - will not toggle the mouse cursor when the cursor hovers over a transparent/hidden window
	// ScriptWindow - prevents the player from moving when clicking on the window if the 'Transparent' flag is not set
	HookCall(0x44B737, gmouse_bk_process_hook);
	// Interface_OnBeforeGameInit will be run before game initialization

	// Set the normal font for death screen subtitles
	if (GetConfigInt("Misc", "DeathScreenFontPatch", 0)) {
		dlog("Applying death screen font patch.", DL_INIT);
		HookCall(0x4812DF, main_death_scene_hook);
		dlogr(" Done", DL_INIT);
	}

	// Corrects the height of the black background for death screen subtitles
	if (hrpIsEnabled == false) SafeWrite32(0x48134D, 38 - (640 * 3));      // main_death_scene_ (shift y-offset 2px up, w/o HRP)
	if (hrpIsEnabled == false || hrpVersionValid) SafeWrite8(0x481345, 4); // main_death_scene_
	if (hrpVersionValid) SafeWrite8(HRPAddress(0x10011738), 10);

	// Cosmetic fix for the background image of the character portrait on the player's inventory screen
	HookCall(0x47093C, display_body_hook);
	BYTE code[11] = {
		0x8B, 0xD3,             // mov  edx, ebx
		0x66, 0x8B, 0x58, 0xF4, // mov  bx, [eax - 12] [sizeof(frame)]
		0x0F, 0xAF, 0xD3,       // imul edx, ebx (y * frame width)
		0x53, 0x90              // push ebx (frame width)
	};
	SafeWriteBytes(0x470971, code, 11); // calculates the offset in the pixel array for x/y coordinates
}

void Interface_Exit() {
	if (dotStyle) delete[] dotStyle;
}
