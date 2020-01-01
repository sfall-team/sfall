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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "LoadGameHook.h"

#include "Interface.h"

namespace sfall
{

static BYTE movePointBackground[16 * 9 * 5];
static fo::UnlistedFrm* ifaceFrm = nullptr;

static void* LoadIfaceFrm() {
	ifaceFrm = fo::LoadUnlistedFrm("IFACE_E.frm", fo::OBJ_TYPE_INTRFACE);
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
		add eax, 9276 - (54 / 2); // x offset
		mov edx, 144 - 90;        // width
		add [esp + 4], edx;
		add [esp + 0x10 + 4], edx;
		retn;
	}
}

static const DWORD intface_update_move_points_ret = 0x45EE3E;
static void __declspec(naked) intface_update_move_points_hack() {
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
	dlog("Applying expanded action points bar patch.", DL_INIT);
	if (hrpIsEnabled) {
		// check valid data
		if (hrpVersionValid && !_stricmp((const char*)HRPAddress(0x10039358), "HR_IFACE_%i.frm")) {
			SafeWriteStr(HRPAddress(0x10039363), "E.frm"); // patching HRP
		} else {
			dlogr(" Incorrect HRP version!", DL_INIT);
			return;
		}
		LoadGameHook::OnAfterGameInit() += APBarRectPatch;
	} else {
		APBarRectPatch();
	}
	SafeWrite32(0x45E343, (DWORD)&movePointBackground);
	SafeWrite32(0x45EE3F, (DWORD)&movePointBackground);
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
		mov  eax, 0x65; // normal text font
		jmp  fo::funcoffs::text_font_;
	}
}

#define WMAP_WIN_WIDTH    (890)
#define WMAP_WIN_HEIGHT   (720)
#define WMAP_TOWN_BUTTONS (15)

static DWORD wmTownMapSubButtonIds[WMAP_TOWN_BUTTONS + 1]; // replace _wmTownMapSubButtonIds (index 0 - unused element)
static int worldmapInterface;
static long wmapWinWidth = 640;
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

static const DWORD wmInterfaceInit_Ret = 0x4C23A7;
static void __declspec(naked) wmInterfaceInit_hack() {
	__asm {
		push eax;
		mov  eax, 640 - WMAP_WIN_WIDTH;
		mov  edx, 480 - WMAP_WIN_HEIGHT;
		jmp  wmInterfaceInit_Ret;
	}
}

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
	mapSlotsScrollMax -= 216;
	if (mapSlotsScrollMax < 0) mapSlotsScrollMax = 0;

	fo::var::wmViewportRightScrlLimit = (350 * fo::var::wmNumHorizontalTiles) - (WMAP_WIN_WIDTH - (640 - 450));
	fo::var::wmViewportBottomtScrlLimit = (300 * (fo::var::wmMaxTileNum / fo::var::wmNumHorizontalTiles)) - (WMAP_WIN_HEIGHT - (480 - 443));

	SafeWriteBatch<DWORD>(135, {0x4C23BD, 0x4C2408}); // use unused worldmap.frm for new world map interface (wmInterfaceInit_)

	// x/y axis offset of interface window
	MakeJump(0x4C23A2, wmInterfaceInit_hack);
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

struct DotPosition {
	long x;
	long y;
};
static std::vector<DotPosition> dots;

static long spaceLen = 2;
static long dotLen = 1;
static long dot_xpos = 0;
static long dot_ypos = 0;

static void AddNewDot() {
	dot_xpos = fo::var::world_xpos;
	dot_ypos = fo::var::world_ypos;

	if (dotLen <= 0 && spaceLen) {
		spaceLen--;
		if (!spaceLen) dotLen = 1; // set dot length
		return;
	}
	dotLen--;

	DotPosition dot;
	dot.x = dot_xpos;
	dot.y = dot_ypos;
	dots.push_back(std::move(dot));

	spaceLen = 2;
}

static void __declspec(naked) wmInterfaceRefresh_hook() {
	long x_offset,  y_offset;
	__asm {
		pushad;
		mov ebp, esp; // prolog
		sub esp, __LOCAL_SIZE;
	}

	if ((fo::var::target_xpos || fo::var::target_ypos) && (dot_xpos != fo::var::world_xpos || dot_ypos != fo::var::world_ypos)) {
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

		BYTE* wmWinBuf = *(BYTE**)FO_VAR_wmBkWinBuf;
		BYTE* wmWinBuf_xy = (wmPixelY + wmPixelX) + wmWinBuf;

		// put pixel to interface window buffer
		if (wmWinBuf_xy > wmWinBuf) *wmWinBuf_xy = 133; // index color in palette: R = 252, G = 0, B = 0

		// TODO: fix dots for car travel
	}

	__asm {
		mov esp, ebp; // epilog
		popad;
		jmp fo::funcoffs::wmInterfaceDrawSubTileList_;
	}
}

static void __declspec(naked) wmInterfaceRefresh_hook_stop() {
	if (!fo::var::target_xpos && !fo::var::target_ypos) {
		dots.clear();
		dotLen = 1;
		spaceLen = 2;
	}
	__asm jmp fo::funcoffs::wmDrawCursorStopped_;
}

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
	BlockCall(0x4C2380); // Remove disabling palette animations (can be used as a place to call a hack function in wmInterfaceInit_)

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
		HookCalls(ScrollCityListFix, {0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D});
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

	if (hrpIsEnabled && hrpVersionValid) {
		if (worldmapInterface = GetConfigInt("Interface", "ExpandWorldMap", 0)) {
			LoadGameHook::OnAfterGameInit() += WorldmapViewportPatch; // Note: must be applied after WorldMapSlots patch
		}
	}

	if (GetConfigInt("Interface", "WorldTravelMarkers", 0)) {
		HookCall(0x4C3BE6, wmInterfaceRefresh_hook); // when calling wmInterfaceDrawSubTileList_
		HookCall(0x4C3C7E, wmInterfaceRefresh_hook_stop);
		dots.reserve(512);
		LoadGameHook::OnGameReset() += []() {
			dots.clear();
		};
	}

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
	long countWin = *(DWORD*)FO_VAR_num_windows;
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
	if (IFACE_BAR_MODE) return 1;
	// if IFACE_BAR_MODE is not enabled, check the display_win window area
	win = fo::func::GNW_find(*(DWORD*)FO_VAR_display_win);
	RECT *rect = &win->wRect;
	return fo::func::mouse_click_in(rect->left, rect->top, rect->right, rect->bottom); // 1 - click in the display_win area
}

static void __declspec(naked) gmouse_bk_process_hook() {
	__asm {
		mov ecx, eax;
		jmp fo::GetTopWindowID;
	}
}

void Interface::init() {
	if (GetConfigInt("Interface", "ActionPointsBar", 0)) {
		ActionPointsBarPatch();
	}
	DrawActionPointsNumber();
	WorldMapInterfacePatch();
	SpeedInterfaceCounterAnimsPatch();

	// Fix for interface windows with 'Hidden' and 'ScriptWindow' flags
	// Hidden - will not toggle the mouse cursor when the cursor hovers over a hidden window
	// ScriptWindow - prevents the player from moving when clicking on the window if the 'Transparent' flag is not set
	HookCall(0x44B737, gmouse_bk_process_hook);
	LoadGameHook::OnBeforeGameInit() += []() {
		if (hrpVersionValid) IFACE_BAR_MODE = *(BYTE*)HRPAddress(0x1006EB0C) != 0;
		HookCall(0x44C018, gmouse_handle_event_hook); // replaces hack function from HRP
	};
}

void Interface::exit() {
	if (ifaceFrm) delete ifaceFrm;
}

}
