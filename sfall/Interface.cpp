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

#include "main.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "HeroAppearance.h"

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

static const DWORD ScrollCityListAddr[] = {
	0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D,
};

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
		mov  eax, 0x65; // normal text font
		jmp  text_font_;
	}
}

static void __declspec(naked) wmInterfaceRefreshCarFuel_hack_empty() {
	__asm {
		mov byte ptr [eax - 1], 13;
		mov byte ptr [eax + 1], 13;
		add eax, 640;
		dec ebx;
		mov byte ptr [eax], 14;
		mov byte ptr [eax - 1], 15;
		mov byte ptr [eax + 1], 15;
		add eax, 640;
		retn;
	}
}

static void __declspec(naked) wmInterfaceRefreshCarFuel_hack() {
	__asm {
		mov byte ptr [eax - 1], 196;
		mov byte ptr [eax + 1], 196;
		add eax, 640;
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
		for (int i = 0; i < sizeof(ScrollCityListAddr) / 4; i++) {
			HookCall(ScrollCityListAddr[i], ScrollCityListFix);
		}
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
static long __stdcall gmouse_handle_event_hook() {
	long countWin = *(DWORD*)_num_windows;
	long ifaceWin = *ptr_interfaceWindow;
	WINinfo* win = nullptr;

	for (int n = 1; n < countWin; n++) {
		win = (WINinfo*)ptr_window[n];
		if ((win->wID == ifaceWin || (win->flags & WIN_ScriptWindow && !(win->flags & WIN_Transparent))) // also check scripted windows
			&& !(win->flags & WIN_Hidden)) {
			RECT *rect = &win->wRect;
			if (MouseClickIn(rect->left, rect->top, rect->right, rect->bottom)) return 0; // 0 - block clicking in the window area
		}
	}
	if (IFACE_BAR_MODE) return 1;
	// if IFACE_BAR_MODE is not enabled, check the display_win window area
	win = GetWinStruct(*(DWORD*)_display_win);
	RECT *rect = &win->wRect;
	return MouseClickIn(rect->left, rect->top, rect->right, rect->bottom); // 1 - click in the display_win area
}

static void __declspec(naked) gmouse_bk_process_hook() {
	__asm {
		call win_get_top_win_;
		cmp  eax, ds:[_display_win];
		jnz  checkFlag;
		retn;
checkFlag:
		call GNW_find_;
		test [eax + 4], WIN_Hidden; // window flags
		jz   skip;
		mov  eax, ds:[_display_win]; // window is hidden, so return the number of the display_win
skip:
		retn;
	}
}

void InterfaceGmouseHandleHook() {
	if (hrpVersionValid) IFACE_BAR_MODE = *(BYTE*)HRPAddress(0x1006EB0C) != 0;
	HookCall(0x44C018, gmouse_handle_event_hook); // replaces hack function from HRP
}

void InterfaceInit() {
	DrawActionPointsNumber();
	WorldMapInterfacePatch();
	SpeedInterfaceCounterAnimsPatch();

	// Fix for interface windows with 'Hidden' and 'ScriptWindow' flags
	// Hidden - will not toggle the mouse cursor when the cursor hovers over a hidden window
	// ScriptWindow - prevents the player from moving when clicking on the window if the 'Transparent' flag is not set
	HookCall(0x44B737, gmouse_bk_process_hook);
	// InterfaceGmouseHandleHook will be run before game initialization
}
