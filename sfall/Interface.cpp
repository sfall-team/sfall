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
#include "FalloutEngine.h"

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

void InterfaceInit() {
	WorldMapInterfacePatch();
	SpeedInterfaceCounterAnimsPatch();
}
