#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "LoadGameHook.h"

#include "Interface.h"

namespace sfall
{

bool hrpIsEnabled = false;

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
		if (!strcmp((const char*)0x10039358, "HR_IFACE_%i.frm")) {
			SafeWriteStr(0x10039363, "E.frm"); // patching HRP
		} else {
			dlog(" Incorrect HRP version!", DL_INIT);
		}
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

////////////////////////////////////// WORLDMAP INTERFACE /////////////////////////////////////////

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

static DWORD wmTownMapSubButtonIds[WMAP_TOWN_BUTTONS + 1]; // _wmTownMapSubButtonIds replaced (index 0 - unsed element)
static int worldmapInterface;

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
	0x4C528B, 0x4C529F, 0x4C52AA,
	// wmInterfaceDrawSubTileList_
	0x4C41C1, 0x4C41D2,
	// wmTownMapRefresh_
	0x4C4BDF,
	// wmDrawCursorStopped_
	0x4C42EE, 0x4C43C8, 0x4C445F,
	// wmTownMapRefresh_
	0x4C4BDF
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
		sub  eax, 5; // y adjust
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
	dlog("Applying world map interface patch.", DL_INIT);

	mapSlotsScrollMax -= 216;
	if (mapSlotsScrollMax < 0) mapSlotsScrollMax = 0;

	fo::var::wmViewportRightScrlLimit = (350 * fo::var::wmNumHorizontalTiles) - (WMAP_WIN_WIDTH - (640 - 450));
	fo::var::wmViewportBottomtScrlLimit = (300 * (fo::var::wmMaxTileNum / fo::var::wmNumHorizontalTiles)) - (WMAP_WIN_HEIGHT - (480 - 443));

	SafeWriteBatch<DWORD>(135, {0x4C23BD, 0x4C2408}); // use unused worldmap.frm for new worldmap interface (wmInterfaceInit_)

	// offset x/y axis of interface window
	MakeJump(0x4C23A2, wmInterfaceInit_hack);
	// size of the created window/buffer
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH, wmWinWidth); // width
	SafeWrite32(0x4C238B, WMAP_WIN_HEIGHT);            // height (wmInterfaceInit_)

	// Mouse scrolling region (wmMouseBkProc_)
	SafeWrite32(0x4C331D, WMAP_WIN_WIDTH - 1);
	SafeWrite32(0x4C3337, WMAP_WIN_HEIGHT - 1);

	MakeCall(0x4C41A4, wmInterfaceDrawSubTileList_hack);   // 640 * 21
	MakeCall(0x4C4082, wmInterfaceDrawCircleOverlay_hack); // 640 * y
	MakeCall(0x4C4452, wmDrawCursorStopped_hack1);
	MakeCalls(wmDrawCursorStopped_hack0, { 0x4C43BB, 0x4C42E1 });
	MakeCall(0x4C5325, wmRefreshTabs_hook);

	HookCall(0x4C4BFF, wmTownMapRefresh_hook);
	if (worldmapInterface == 1) {
		HookCall(0x4C4CD5, wmTownMapRefresh_hook_textpos);
		HookCall(0x4C4B8F, wmTownMapInit_hook);
	}
	// towns scroll up/down buttons (wmInterfaceInit_)
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 480), { // offset by X (480)
		0x4C2D3C,
		0x4C2D7A
	});

	// button city/worldmap (wmInterfaceInit_)
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
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 450), wmViewportEndRight);   // 890 - 190 = 700 + 22 = 722
	// bottom limit of viewport (443)
	SafeWriteBatch<DWORD>(WMAP_WIN_HEIGHT - (480 - 443), wmViewportEndBottom); // 720 - 37 = 683 + 21 = 704

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

	// Town frm images (wmTownMapRefresh_)
	SafeWrite32(0x4C4BE4, (WMAP_WIN_WIDTH * 21) + 22); // start offset for town image (13462)
	dlogr(" Done", DL_INIT);
}

static void WorldMapInterfacePatch() {
	if (GetConfigInt("Misc", "WorldMapFontPatch", 0)) {
		dlog("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
		dlogr(" Done", DL_INIT);
	}
	// Fixes images for up/down buttons
	SafeWrite32(0x4C2C0A, 199); // index UPARWOFF.FRM
	SafeWrite8(0x4C2C7C, 0x43); // dec ebx >> inc ebx
	SafeWrite32(0x4C2C92, 181); // index DNARWOFF.FRM
	SafeWrite8(0x4C2D04, 0x46); // dec esi >> inc esi

	//if(GetConfigInt("Misc", "WorldMapCitiesListFix", 0)) {
	dlog("Applying world map cities list patch.", DL_INIT);
	HookCalls(ScrollCityListFix, {0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D});
	dlogr(" Done", DL_INIT);
	//}

	DWORD wmSlots = GetConfigInt("Interface", "WorldMapSlots", -1);
	if (wmSlots == -1) wmSlots = GetConfigInt("Misc", "WorldMapSlots", 0); // for compatibility
	if (wmSlots && wmSlots < 128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27; // height value after which scrolling is not possible
		mapSlotsScrollLimit = wmSlots * 27;
		SafeWrite32(0x4C21FD, 189); // 27 * 7
		SafeWrite32(0x4C21F1, (DWORD)&mapSlotsScrollLimit);
		dlogr(" Done", DL_INIT);
	}

	if (hrpIsEnabled) {
		if (worldmapInterface = GetConfigInt("Interface", "WorldMapInterface", 0)) {
			LoadGameHook::OnAfterGameInit() += WorldmapViewportPatch; // Note: Must be applyling after WorldMapSlots patch
		}
	}
}

void Interface::init() {
	hrpIsEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check enabled HRP

	if (GetConfigInt("Interface", "ActionPointsBar", 0)) {
		ActionPointsBarPatch();
		if (hrpIsEnabled) LoadGameHook::OnAfterGameInit() += APBarRectPatch;
	}

	WorldMapInterfacePatch();
}

void Interface::exit() {
	if (ifaceFrm) delete ifaceFrm;
}

}
