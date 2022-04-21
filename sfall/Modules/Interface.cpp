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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"
#include "..\Utils.h"
#include "Graphics.h"
#include "LoadGameHook.h"
#include "Worldmap.h"

#include "..\HRP\InterfaceBar.h"
#include "..\HRP\viewmap\EdgeClipping.h"

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
	// TODO
	DialogView,
	DialogPanel,
	MemberPanel,

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
		if (GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER)) winID = fo::var::i_wid;
		break;
	case WinNameType::Dialog:
		if (GetLoopFlags() & DIALOG) winID = fo::var::dialogueBackWindow;
		break;
	case WinNameType::PipBoy:
		if (GetLoopFlags() & PIPBOY) winID = fo::var::pip_win;
		break;
	case WinNameType::WorldMap:
		if (GetLoopFlags() & WORLDMAP) winID = fo::var::wmBkWin;
		break;
	case WinNameType::IfaceBar:
		winID = fo::var::interfaceWindow;
		break;
	case WinNameType::Character:
		if (GetLoopFlags() & CHARSCREEN) winID = fo::var::edit_win;
		break;
	case WinNameType::Skilldex:
		if (GetLoopFlags() & SKILLDEX) winID = fo::var::skldxwin;
		break;
	case WinNameType::EscMenu:
		if (GetLoopFlags() & ESCMENU) winID = fo::var::optnwin;
		break;
	case WinNameType::Automap:
		if (GetLoopFlags() & AUTOMAP) winID = ActiveInterfaceWID();
		break;
	default:
		return (fo::Window*)(-1); // unsupported type
	}
	return (winID > 0) ? fo::func::GNW_find(winID) : nullptr;
}

static BYTE movePointBackground[16 * 9 * 5];
static fo::UnlistedFrm* ifaceFrm = nullptr;

static void* LoadIfaceFrm() {
	ifaceFrm = fo::util::LoadUnlistedFrm("IFACE_E.frm", fo::OBJ_TYPE_INTRFACE);
	if (!ifaceFrm) return nullptr;
	return ifaceFrm->frames[0].indexBuff;
}

static void __declspec(naked) intface_init_hook_lock() {
	__asm {
		pushadc;
		call LoadIfaceFrm;
		test eax, eax;
		jz   skip;
		pop  ecx;
		add  esp, 8;
		mov  dword ptr [ecx], 0;
		retn;
skip:
		popadc;
		jmp  fo::funcoffs::art_ptr_lock_data_;
	}
}

static void __declspec(naked) intface_init_hack() {
	__asm {
		add  eax, 9276 - (54 / 2); // x offset
		mov  edx, 144 - 90;        // width
		add  [esp + 4], edx;
		add  [esp + 0x10 + 4], edx;
		retn;
	}
}

static void __declspec(naked) intface_update_move_points_hack() {
	static const DWORD intface_update_move_points_ret = 0x45EE3E;
	__asm {
		mov  eax, 16 * 9
		push eax;
		push 5;
		push eax;
		jmp  intface_update_move_points_ret;
	}
}

static void APBarRectPatch() {
	fo::var::movePointRect.x -= (54 / 2); // 54 = 144(new width) - 90(old width)
	fo::var::movePointRect.offx += (54 / 2);
}

static void ActionPointsBarPatch() {
	HRP::IFaceBar::SetExpandAPBar();

	dlog("Applying expanded action points bar patch.", DL_INIT);
	if (HRP::Setting::ExternalEnabled()) {
		// check valid data
		if (HRP::Setting::VersionIsValid && !_stricmp((const char*)HRP::Setting::GetAddress(HRP_VAR_HR_IFACE_FRM_STR), "HR_IFACE_%i.frm")) {
			SafeWriteStr(HRP::Setting::GetAddress(HRP_VAR_HR_IFACE_FRM_STR + 11), "E.frm"); // patching HRP
		} else {
			dlogr(" Incorrect HRP version!", DL_INIT);
			return;
		}
		LoadGameHook::OnAfterGameInit() += APBarRectPatch;
	} else {
		APBarRectPatch();
	}
	// intface_init_
	SafeWriteBatch<DWORD>((DWORD)&movePointBackground, {0x45E343, 0x45EE3F});
	// intface_update_move_points_
	SafeWriteBatch<BYTE>(16, {0x45EE55, 0x45EE7B, 0x45EE82, 0x45EE9C, 0x45EEA0});
	SafeWriteBatch<DWORD>(9276 - (54 / 2), {0x45EE33, 0x45EEC8, 0x45EF16});

	HookCall(0x45D918, intface_init_hook_lock);
	MakeCall(0x45E356, intface_init_hack);
	MakeJump(0x45EE38, intface_update_move_points_hack, 1);
	dlogr(" Done", DL_INIT);
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

#define WMAP_WIN_WIDTH    (890)
#define WMAP_WIN_HEIGHT   (720)
#define WMAP_TOWN_BUTTONS (15)

static DWORD wmTownMapSubButtonIds[WMAP_TOWN_BUTTONS + 1]; // replace _wmTownMapSubButtonIds (index 0 - unused element)
static int worldmapInterface;
static long wmapWinWidth = 640;
static long wmapWinHeight = 480;
static long wmapViewPortWidth = 450;
static long wmapViewPortHeight = 443;

// Window width
static const DWORD wmWinWidth[] = {
	// wmInterfaceInit_
	0x4C239E, 0x4C247A,
	// wmInterfaceRefresh_
	0x4C38EA, 0x4C3978,
	// wmInterfaceDrawCircleOverlay_
	0x4C3FCA, 0x4C408E,
	// wmRefreshInterfaceDial_
	0x4C5757,
	// wmInterfaceRefreshDate_
	0x4C3D0A, 0x4C3D79, 0x4C3DBB, 0x4C3E87, 0x4C3E1F,
	// wmRefreshTabs_
	0x4C52BF, 0x4C53F5, 0x4C55A5, 0x4C557E, 0x4C54B2, 0x4C53E8,
	// wmRefreshInterfaceOverlay_
	0x4C50FD, 0x4C51CF, 0x4C51F8, 0x4C517F,
	// wmInterfaceRefreshCarFuel_
	0x4C52AA, /*0x4C528B, 0x4C529F, - Conflict with fuel gauge patch*/
	// wmInterfaceDrawSubTileList_
	0x4C41C1, 0x4C41D2,
	// wmTownMapRefresh_
	0x4C4BDF,
	// wmDrawCursorStopped_
	0x4C42EE, 0x4C43C8, 0x4C445F,
};

// Right limit of the viewport (450)
static const DWORD wmViewportEndRight[] = {
//	0x4BC91F,                                                   // wmWorldMap_init_
	0x4C3937, 0x4C393E, 0x4C39BB, 0x4C3B2F, 0x4C3B36, 0x4C3C4B, // wmInterfaceRefresh_
	0x4C4288, 0x4C436A, 0x4C4409,                               // wmDrawCursorStopped_
	0x4C44B4,                                                   // wmCursorIsVisible_
};

// Bottom limit of viewport (443)
static const DWORD wmViewportEndBottom[] = {
//	0x4BC947,                                                   // wmWorldMap_init_
	0x4C3963, 0x4C38D7, 0x4C39DA, 0x4C3B62, 0x4C3AE7, 0x4C3C74, // wmInterfaceRefresh_
	0x4C429A, 0x4C4378, 0x4C4413,                               // wmDrawCursorStopped_
	0x4C44BE,                                                   // wmCursorIsVisible_
};
/*
static void __declspec(naked) wmInterfaceInit_hack() {
	static const DWORD wmInterfaceInit_Ret = 0x4C23A7;
	__asm {
		push eax;
		mov  eax, 640 - WMAP_WIN_WIDTH;
		mov  edx, 480 - WMAP_WIN_HEIGHT;
		jmp  wmInterfaceInit_Ret;
	}
}
*/
static void __declspec(naked) wmInterfaceDrawSubTileList_hack() {
	__asm {
		mov  edx, [esp + 0x10 - 0x10 + 4];
		imul edx, WMAP_WIN_WIDTH;
		add  edx, ecx;
		retn;
	}
}

static void __declspec(naked) wmInterfaceDrawCircleOverlay_hack() {
	__asm {
		mov  eax, ecx;
		imul eax, WMAP_WIN_WIDTH;
		retn;
	}
}

static void __declspec(naked) wmDrawCursorStopped_hack0() {
	__asm {
		mov  ebx, ecx;
		imul ebx, WMAP_WIN_WIDTH;
		retn;
	}
}

static void __declspec(naked) wmDrawCursorStopped_hack1() {
	__asm {
		mov  ebx, eax;
		imul ebx, WMAP_WIN_WIDTH;
		retn;
	}
}

static void __declspec(naked) wmRefreshTabs_hook() {
	__asm {
		mov  eax, edx;
		imul eax, WMAP_WIN_WIDTH;
		sub  ebp, eax;
		retn;
	}
}

static void __declspec(naked) wmTownMapRefresh_hook() {
	__asm {
		cmp  edx, 700; //_wmTownWidth
		jl   scale;
		cmp  ebx, 682; //_wmTownHeight
		jl   scale;
		jmp  fo::funcoffs::buf_to_buf_;
scale:
		push WMAP_WIN_WIDTH; // to_width
		push 684;            // height
		push 702;            // width
		push eax;            // to_buff
		mov  ecx, edx;       // from_width
		mov  eax, esi;       // from_buff
		call fo::funcoffs::cscale_;
		retn; // don't delete
	}
}

static void __declspec(naked) wmTownMapRefresh_hook_textpos() {
	__asm {
		push eax;
		push ebx;
		mov  eax, ecx; // xpos
		shr  ebx, 2;   // text_width / 4
		test ebx, ebx;
		jz   skipX;
		sub  ebx, 5;   // x adjust
skipX:
		sar  eax, 1;
		add  ecx, eax; // xpos * 1.5
		add  ecx, ebx;
		pop  ebx;
		mov  eax, dword ptr [esp + 8]; // ypos
		sar  eax, 1;
		test eax, eax;
		jz   skipY;
		sub  eax, 5;   // y adjust
skipY:
		add  dword ptr [esp + 8], eax; // ypos * 1.5
		pop  eax;
		jmp  fo::funcoffs::win_print_;
	}
}

static void __declspec(naked) wmTownMapInit_hook() {
	__asm {
		push eax;
		mov  eax, edx; // xpos
		shr  eax, 1;
		add  edx, eax; // xpos * 1.5
		mov  eax, ebx; // ypos
		shr  eax, 1;
		add  ebx, eax; // ypos * 1.5
		pop  eax;
		jmp  fo::funcoffs::win_register_button_;
	}
}

static void WorldmapViewportPatch() {
	if (Graphics::GetGameHeightRes() < WMAP_WIN_HEIGHT || Graphics::GetGameWidthRes() < WMAP_WIN_WIDTH) return;
	if (!fo::func::db_access("art\\intrface\\worldmap.frm")) return;
	dlog("Applying expanded world map interface patch.", DL_INIT);

	wmapWinWidth = WMAP_WIN_WIDTH;
	wmapWinHeight = WMAP_WIN_HEIGHT;
	mapSlotsScrollMax -= 216;
	if (mapSlotsScrollMax < 0) mapSlotsScrollMax = 0;

	fo::var::wmViewportRightScrlLimit = (350 * fo::var::wmNumHorizontalTiles) - (WMAP_WIN_WIDTH - (640 - 450));
	fo::var::wmViewportBottomtScrlLimit = (300 * (fo::var::wmMaxTileNum / fo::var::wmNumHorizontalTiles)) - (WMAP_WIN_HEIGHT - (480 - 443));

	SafeWriteBatch<DWORD>(135, {0x4C23BD, 0x4C2408}); // use unused worldmap.frm for new world map interface (wmInterfaceInit_)

	// x/y axis offset of interface window
	//MakeJump(0x4C23A2, wmInterfaceInit_hack);
	// size of the created window/buffer
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH, wmWinWidth); // width
	SafeWrite32(0x4C238B, WMAP_WIN_HEIGHT);            // height (wmInterfaceInit_)

	// Mouse scrolling area (wmMouseBkProc_)
	SafeWrite32(0x4C331D, WMAP_WIN_WIDTH - 1);
	SafeWrite32(0x4C3337, WMAP_WIN_HEIGHT - 1);

	MakeCall(0x4C41A4, wmInterfaceDrawSubTileList_hack);   // 640 * 21
	MakeCall(0x4C4082, wmInterfaceDrawCircleOverlay_hack); // 640 * y
	MakeCall(0x4C4452, wmDrawCursorStopped_hack1);
	MakeCalls(wmDrawCursorStopped_hack0, {0x4C43BB, 0x4C42E1});
	MakeCall(0x4C5325, wmRefreshTabs_hook);

	HookCall(0x4C4BFF, wmTownMapRefresh_hook);
	if (worldmapInterface == 1) {
		HookCall(0x4C4CD5, wmTownMapRefresh_hook_textpos);
		HookCall(0x4C4B8F, wmTownMapInit_hook);
	}
	// up/down buttons of the location list (wmInterfaceInit_)
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 480), { // offset by X (480)
		0x4C2D3C,
		0x4C2D7A
	});

	// town/world button (wmInterfaceInit_)
	SafeWrite32(0x4C2B9B, WMAP_WIN_HEIGHT - (480 - 439)); // offset by Y (439)
	SafeWrite32(0x4C2BAF, WMAP_WIN_WIDTH - (640 - 519));  // offset by X (508)

	// viewport size for mouse click
	SafeWriteBatch<DWORD>(WMAP_WIN_HEIGHT - (480 - 465), { // height/offset by Y (465 - 21 = 444 (443))
		0x4C0154, 0x4C02BA, // wmWorldMap_
		0x4C3A47,           // wmInterfaceRefresh_
	});
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 472), { // width/offset by X (472 - 22 = 450)
		0x4C0159, 0x4C02BF, // wmWorldMap_
		0x4C3A3A,           // wmInterfaceRefresh_
		0x4C417C, 0x4C4184  // wmInterfaceDrawSubTileList_
	});
	SafeWriteBatch<DWORD>(WMAP_WIN_HEIGHT - (480 - 464), { // width/offset by X (464)
		0x4C3FED,           // wmInterfaceDrawCircleOverlay_
		0x4C4157, 0x4C415F, // wmInterfaceDrawSubTileList_
	});
	// right limit of the viewport (450)
	wmapViewPortWidth = WMAP_WIN_WIDTH - (640 - 450); // 890 - 190 = 700 + 22 = 722
	SafeWriteBatch<DWORD>(wmapViewPortWidth, wmViewportEndRight);
	// bottom limit of the viewport (443)
	wmapViewPortHeight = WMAP_WIN_HEIGHT - (480 - 443); // 720 - 37 = 683 + 21 = 704
	SafeWriteBatch<DWORD>(wmapViewPortHeight, wmViewportEndBottom);

	// Night/Day frm (wmRefreshInterfaceDial_)
	SafeWrite32(0x4C577F, WMAP_WIN_WIDTH - (640 - 532));                 // X offset (532)
	SafeWrite32(0x4C575D, (WMAP_WIN_WIDTH * 49) - ((640 * 49) - 31252)); // start offset in buffer (31252 / 640 = 49)

	// Date/Time frm
	SafeWrite32(0x4C3EC7, WMAP_WIN_WIDTH - (640 - 487));                 // start offset by X (487)
	SafeWrite32(0x4C3ED1, WMAP_WIN_WIDTH - (640 - 630));                 // end offset by X (630)
	SafeWrite32(0x4C3D10, (WMAP_WIN_WIDTH * 13) - ((640 * 13) - 8167));  // 8167 start offset in buffer (12327)
	SafeWrite32(0x4C3DC1, WMAP_WIN_WIDTH + (666 - 640)); // 666

	// WMCARMVE/WMGLOBE/WMSCREEN frms (wmRefreshInterfaceOverlay_)
	SafeWrite32(0x4C51D5, (WMAP_WIN_WIDTH * 577) - ((640 * 337) - 215554)); // start offset for image WMCARMVE
	SafeWrite32(0x4C5184, (WMAP_WIN_WIDTH * 571) - ((640 * 331) - 211695)); // start offset for image WMGLOBE
	SafeWrite32(0x4C51FD, (WMAP_WIN_WIDTH * 571) - ((640 * 331) - 211699)); // start offset for image WMSCREEN
	// Car gas indicator
	SafeWrite32(0x4C527E, (WMAP_WIN_WIDTH * 580) - ((640 * 340) - 217460)); // start offset in buffer (217460 / 640 = 340 + (720 - 480) = 580 Y-axes)

	// WMTABS.frm
	SafeWriteBatch<DWORD>((WMAP_WIN_WIDTH * 136) - ((640 * 136) - 86901), {
		0x4C52C4, 0x4C55AA // wmRefreshTabs_
	});
	SafeWrite32(0x4C52FF, (WMAP_WIN_WIDTH * 139) - ((640 * 139) - 88850)); // wmRefreshTabs_
	SafeWriteBatch<DWORD>((WMAP_WIN_WIDTH * 27), { // start offset in buffer (17280)
		0x4C54DE, 0x4C5424 // wmRefreshTabs_
	});
	SafeWrite32(0x4C52E5, WMAP_WIN_HEIGHT - 480 + 178 - 19); // height (178)

	// Buttons of cities, now 15
	SafeWrite32(0x4C2BD9, WMAP_WIN_WIDTH - (640 - 508)); // offset of the buttons by X (508)
	SafeWriteBatch<BYTE>(WMAP_TOWN_BUTTONS * 4, {0x4C2C01, 0x4C21B6, 0x4C2289}); // number of buttons (28 = 7 * 4)
	int btn = (mapSlotsScrollLimit) ? WMAP_TOWN_BUTTONS : WMAP_TOWN_BUTTONS + 1;
	SafeWrite32(0x4C21FD, 27 * btn); // scroll limit for buttons
	// number of city labels (6) wmRefreshTabs_
	SafeWriteBatch<BYTE>(WMAP_TOWN_BUTTONS - 1, {0x4C54F6, 0x4C542A});
	SafeWrite8(0x4C555E, 0);
	SafeWrite32(0x4C0348, 350 + WMAP_TOWN_BUTTONS); // buttons input code (wmWorldMap_)

	SafeWrite32(0x4C2BFB, (DWORD)&wmTownMapSubButtonIds[0]); // wmInterfaceInit_
	SafeWriteBatch<DWORD>((DWORD)&wmTownMapSubButtonIds[1], {
		0x4C22DD, 0x4C230A, // wmInterfaceScrollTabsUpdate_ (never called)
		0x4C227B,           // wmInterfaceScrollTabsStop_
		0x4C21A8            // wmInterfaceScrollTabsStart_
	});
	// WMTBEDGE.frm (wmRefreshTabs_)
	BlockCall(0x4C55BF);

	// Town map frm images (wmTownMapRefresh_)
	SafeWrite32(0x4C4BE4, (WMAP_WIN_WIDTH * 21) + 22); // start offset for town map image (13462)
	dlogr(" Done", DL_INIT);
}

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

static std::array<unsigned char, TerrainHoverImage::size> wmTmpBuffer;
static bool isHoveringHotspot = false;
static bool backImageIsCopy = false;

struct DotPosition {
	long x;
	long y;

	DotPosition(long x, long y) : x(x), y(y) {}
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
	dot_xpos = fo::var::world_xpos;
	dot_ypos = fo::var::world_ypos;

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

	dots.emplace_back(dot_xpos, dot_ypos);
}

static void __declspec(naked) DrawingDots() noexcept {
	long x_offset,  y_offset;
	__asm {
		mov ebp, esp; // prolog
		sub esp, __LOCAL_SIZE;
	}

	if (dot_xpos != fo::var::world_xpos || dot_ypos != fo::var::world_ypos) {
		AddNewDot();
	}
	x_offset = 22 - fo::var::wmWorldOffsetX;
	y_offset = 21 - fo::var::wmWorldOffsetY;

	for (const auto &dot : dots) { // redraws all dots
		if (dot.x < fo::var::wmWorldOffsetX || dot.y < fo::var::wmWorldOffsetY) continue; // the pixel is out of viewport
		if (dot.x > fo::var::wmWorldOffsetX + wmapViewPortWidth || dot.y > fo::var::wmWorldOffsetY + wmapViewPortHeight) continue;

		long wmPixelX = (dot.x + x_offset);
		long wmPixelY = (dot.y + y_offset);

		wmPixelY *= wmapWinWidth;

		BYTE* wmWinBuf = fo::var::wmBkWinBuf;
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
	long area = fo::var::WorldMapCurrArea;
	char* text = (area != -1 || !showTerrainType) ? (char*)Worldmap::GetCustomAreaTitle(area) : (char*)Worldmap::GetCurrentTerrainName();
	if (!text) return false;

	if (backgroundCopy) { // copy background image to memory (size 200 x 15)
		backImageIsCopy = true;
		fo::util::SurfaceCopyToMem(x - TerrainHoverImage::x_shift, y, TerrainHoverImage::width, TerrainHoverImage::height, wmapWinWidth, fo::var::wmBkWinBuf, wmTmpBuffer.data());
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

	fo::util::PrintTextFM(text, 228, x, y, txtWidth, wmapWinWidth, fo::var::wmBkWinBuf); // shadow
	fo::util::PrintTextFM(text, 215, x - 1, y - 1, txtWidth, wmapWinWidth, fo::var::wmBkWinBuf);

	if (backgroundCopy) fo::func::wmRefreshInterfaceOverlay(0); // prevent printing text over the interface
	return true;
}

static void __declspec(naked) wmInterfaceRefresh_hook() {
	if (colorDot && fo::var::target_xpos != -1) {
		if (fo::var::In_WorldMap) {
			DrawingDots();
		} else if (!fo::var::target_xpos && !fo::var::target_ypos) {
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
	if (isHoveringHotspot && !fo::var::In_WorldMap) {
		PrintHotspotText(fo::var::world_xpos - fo::var::wmWorldOffsetX, fo::var::world_ypos - fo::var::wmWorldOffsetY);
		isHoveringHotspot = backImageIsCopy = false;
	}
	__asm jmp fo::funcoffs::wmDrawCursorStopped_;
}

static void __fastcall wmDetectHotspotHover(long wmMouseX, long wmMouseY) {
	if (!showTerrainType && Worldmap::AreaTitlesIsEmpty()) return;

	long deltaX = 20, deltaY = 20;

	// mouse cursor is out of viewport area (the zero values of wmMouseX and wmMouseY correspond to the top-left corner of the worldmap interface)
	if ((wmMouseX < 20 || wmMouseY < 20 || wmMouseX > wmapViewPortWidth + 15 || wmMouseY > wmapViewPortHeight + 20) == false) {
		deltaX = abs((long)fo::var::world_xpos - (wmMouseX - deltaX + fo::var::wmWorldOffsetX));
		deltaY = abs((long)fo::var::world_ypos - (wmMouseY - deltaY + fo::var::wmWorldOffsetY));
	}

	bool isHovered = isHoveringHotspot;
	isHoveringHotspot = deltaX < 8 && deltaY < 6;
	if (isHoveringHotspot != isHovered) { // if value has changed
		// upper left corner
		long y = fo::var::world_ypos - fo::var::wmWorldOffsetY;
		long x = fo::var::world_xpos - fo::var::wmWorldOffsetX;
		long x_offset = x - TerrainHoverImage::x_shift;
		if (!backImageIsCopy) {
			if (!PrintHotspotText(x, y, true)) return;
		} else {
			// restore background image
			fo::util::DrawToSurface(x_offset, y, TerrainHoverImage::width, TerrainHoverImage::height, wmapWinWidth, wmapWinHeight, fo::var::wmBkWinBuf, wmTmpBuffer.data());
			backImageIsCopy = false;
		}
		// redraw rectangle on worldmap interface
		RECT rect;
		rect.top = y;
		rect.left = x_offset;
		rect.right = x + TerrainHoverImage::width;
		rect.bottom = y + TerrainHoverImage::height;
		fo::func::win_draw_rect(fo::var::wmBkWin, &rect);
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
		dlog("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
		dlogr(" Done", DL_INIT);
	}

	// Add missing sounds for the buttons on the world map interface (wmInterfaceInit_)
	HookCalls(wmInterfaceInit_hook, {
		0x4C2BF4, // location labels
		0x4C2BB5, // town/world
		0x4C2D4C, // up
		0x4C2D8A  // down
	});
	HookCall(0x4C02DA, wmWorldMap_hook); // destination marker
	MakeCall(0x4C4257, wmDrawCursorStopped_hack_hotspot); // triangle markers on the world map
	MakeCall(0x4C4B94, wmTownMapInit_hack, 1);            // triangle markers on the town map

	// Fix images for up/down buttons
	SafeWrite32(0x4C2C0A, 199); // index of UPARWOFF.FRM
	SafeWrite8(0x4C2C7C, 0x43); // dec ebx > inc ebx
	SafeWrite32(0x4C2C92, 181); // index of DNARWOFF.FRM
	SafeWrite8(0x4C2D04, 0x46); // dec esi > inc esi

	//if (IniReader::GetConfigInt("Misc", "WorldMapCitiesListFix", 0)) {
		dlog("Applying world map cities list patch.", DL_INIT);
		HookCalls(ScrollCityListFix, {0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D});
		dlogr(" Done", DL_INIT);
	//}

	DWORD wmSlots = IniReader::GetConfigInt("Misc", "WorldMapSlots", 0);
	if (wmSlots && wmSlots < 128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27; // height value after which scrolling is not possible
		mapSlotsScrollLimit = wmSlots * 27;
		SafeWrite32(0x4C21FD, 189); // 27 * 7
		SafeWrite32(0x4C21F1, (DWORD)&mapSlotsScrollLimit);
		dlogr(" Done", DL_INIT);
	}

	if (HRP::Setting::IsEnabled() || HRP::Setting::VersionIsValid) { // was available only for 4.1.8?
		if (worldmapInterface = IniReader::GetConfigInt("Interface", "ExpandWorldMap", 0)) {
			LoadGameHook::OnAfterGameInit() += WorldmapViewportPatch; // Note: must be applied after WorldMapSlots patch
		}
	}

	// Fallout 1 features, travel markers and displaying terrain types or town titles
	if (IniReader::GetConfigInt("Interface", "WorldMapTravelMarkers", 0)) {
		dlog("Applying world map travel markers patch.", DL_INIT);

		int color = IniReader::GetConfigInt("Interface", "TravelMarkerColor", 134); // color index in palette: R = 224, G = 0, B = 0
		if (color > 228) color = 228; else if (color < 1) color = 1; // no palette animation colors
		colorDot = color;

		auto dotList = IniReader::GetConfigList("Interface", "TravelMarkerStyles", "", 512);
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
					dotStyle[i] = {DotStyleDefault::DotLen, DotStyleDefault::SpaceLen};
				}
				pair.clear();
			}
		}
		dots.reserve(512);
		LoadGameHook::OnGameReset() += []() {
			dots.clear();
		};
		dlogr(" Done", DL_INIT);
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

static long gmouse_handle_event_hook() {
	// check whether the player clicks on the clipping area of the map
	if (!HRP::EdgeClipping::CheckMapClipping()) return 0; // block

	long countWin = fo::var::num_windows;
	long ifaceWin = fo::var::interfaceWindow;
	fo::Window* win = nullptr;

	for (int n = 1; n < countWin; n++) {
		win = fo::var::window[n];
		if ((win->wID == ifaceWin || (win->flags & fo::WinFlags::ScriptWindow && !(win->flags & fo::WinFlags::Transparent))) // also check the script windows
			&& !(win->flags & fo::WinFlags::Hidden)) {
			RECT *rect = &win->wRect;
			if (fo::func::mouse_click_in(rect->left, rect->top, rect->right, rect->bottom)) return 0; // 0 - block clicking in the window area
		}
	}

	if (HRP::IFaceBar::IFACE_BAR_MODE) return 1;

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
	dlog("Applying flags patch for interface windows.", DL_INIT);

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

	dlogr(" Done", DL_INIT);

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
	SafeWriteBatch<BYTE>(127, {0x435160, 0x435189}); // 100 (PrintBigname_)

	// Increase the max text width of the information card on the character screen
	SafeWriteBatch<BYTE>(145, {0x43ACD5, 0x43DD37}); // 136, 133 (DrawCard_, DrawCard2_)

	// Increase the width of the mouse drop area from 64px to 80px for the PC's and NPC's inventory on the barter screen
	// barter_move_from_table_inventory_
	SafeWrite32(0x47523D, 98);       // x_start was 80
	SafeWrite32(0x475231, 98 + 80);  // x_end   was 144
	SafeWrite32(0x4752BE, 460);      // x_start was 475
	SafeWrite32(0x4752B2, 460 + 80); // x_end   was 539
}

static void InventoryCharacterRotationSpeedPatch() {
	long setting = IniReader::GetConfigInt("Misc", "SpeedInventoryPCRotation", 166);
	if (setting != 166 && setting <= 1000) {
		dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
		SafeWrite32(0x47066B, setting);
		dlogr(" Done", DL_INIT);
	}
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

void Interface::init() {
	InterfaceWindowPatch();
	InventoryCharacterRotationSpeedPatch();
	UIAnimationSpeedPatch();

	// Remove window position rounding for script-created windows
	//if (IniReader::GetConfigInt("Misc", "RemoveWindowRounding", 1)) {
		SafeWriteBatch<BYTE>(CodeType::JumpShort, {0x4D6EDD, 0x4D6F12});
	//}

	if (IniReader::GetConfigInt("Interface", "ActionPointsBar", 0)) {
		ActionPointsBarPatch();
	}
	DrawActionPointsNumber();
	WorldMapInterfacePatch();
	SpeedInterfaceCounterAnimsPatch();

	// Fix for interface windows with 'Transparent', 'Hidden' and 'ScriptWindow' flags
	// Transparent/Hidden - will not toggle the mouse cursor when the cursor hovers over a transparent/hidden window
	// ScriptWindow - prevents the player from moving when clicking on the window if the 'Transparent' flag is not set
	HookCall(0x44B737, gmouse_bk_process_hook);
	HookCall(0x44C018, gmouse_handle_event_hook); // replaces hack function from HRP by Mash
	if (HRP::Setting::VersionIsValid) HRP::IFaceBar::IFACE_BAR_MODE = (GetIntHRPValue(HRP_VAR_IFACE_BAR_MODE) != 0);
}

void Interface::exit() {
	if (ifaceFrm) delete ifaceFrm;
	if (dotStyle) delete[] dotStyle;
}

}
