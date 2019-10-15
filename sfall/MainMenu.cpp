/*
 *    sfall
 *    Copyright (C) 2012  The sfall team
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
#include "version.h"

#ifdef NDEBUG
static const char* VerString1 = "SFALL " VERSION_STRING;
#else
static const char* VerString1 = "SFALL " VERSION_STRING " Debug Build";
#endif

static DWORD MainMenuYOffset;
static DWORD MainMenuTextOffset;

static long OverrideColour, OverrideColour2;

static const DWORD MainMenuButtonYHookRet = 0x48184A;
static void __declspec(naked) MainMenuButtonYHook() {
	__asm {
		xor edi, edi;
		xor esi, esi;
		mov ebp, MainMenuYOffset;
		jmp MainMenuButtonYHookRet;
	}
}

static void __declspec(naked) MainMenuTextYHook() {
	__asm {
		add eax, MainMenuTextOffset;
		jmp dword ptr ds:[_text_to_buf];
	}
}

static void __declspec(naked) FontColour() {
	__asm {
		test OverrideColour, 0xFF;
		jnz  override;
		movzx eax, byte ptr ds:[0x6A8B33];
		or   eax, 0x6000000;
		retn;
override:
		mov  eax, OverrideColour;
		retn;
	}
}

static const DWORD MainMenuTextRet = 0x4817B0;
static void __declspec(naked) MainMenuTextHook() {
	__asm {
		mov  esi, eax;                // winptr
		mov  ebp, ecx;                // keep xpos
		mov  edi, [esp];              // ypos
		mov  eax, edi;
		sub  eax, 12;                 // shift y position up by 12
		mov  [esp], eax;
		call FontColour;
		mov  [esp + 4], eax;          // colour
		mov  eax, esi;
		mov  esi, edx;                // keep fallout buff
		call win_print_;
		// sfall print
		mov  eax, esi;
		call ds:[_text_width];
		add  ebp, eax;               // xpos shift (right align)
		call FontColour;
		push eax;                    // colour
		mov  edx, VerString1;        // msg
		mov  eax, edx;
		call ds:[_text_width];
		mov  ecx, ebp;               // xpos
		sub  ecx, eax;               // left shift position
		push edi;                    // ypos
		xor  ebx, ebx;               // font
		mov  eax, dword ptr ds:[_main_window]; // winptr
		call win_print_;
		jmp  MainMenuTextRet;
	}
}

void MainMenuInit() {
	int offset;
	if (offset = GetConfigInt("Misc", "MainMenuCreditsOffsetX", 0)) {
		SafeWrite32(0x481753, 15 + offset);
	}
	if (offset = GetConfigInt("Misc", "MainMenuCreditsOffsetY", 0)) {
		SafeWrite32(0x48175C, 460 + offset);
	}
	if (offset = GetConfigInt("Misc", "MainMenuOffsetX", 0)) {
		SafeWrite32(0x48187C, 30 + offset);
		MainMenuTextOffset = offset;
	}
	if (offset = GetConfigInt("Misc", "MainMenuOffsetY", 0)) {
		MainMenuYOffset = offset;
		MainMenuTextOffset += offset * 640;
		MakeJump(0x481844, MainMenuButtonYHook);
	}
	if (MainMenuTextOffset) {
		MakeCall(0x481933, MainMenuTextYHook, 1);
	}

	MakeJump(0x4817AB, MainMenuTextHook);

	OverrideColour = GetConfigInt("Misc", "MainMenuFontColour", 0);
	if (OverrideColour & 0xFF) {
		OverrideColour &= 0x00FF00FF;
		OverrideColour |= 0x06000000;
		unsigned char flags = static_cast<unsigned char>((OverrideColour & 0xFF0000) >> 16);
		if (!(flags & 1)) SafeWrite32(0x481748, (DWORD)&OverrideColour);
	}
	OverrideColour2 = GetConfigInt("Misc", "MainMenuBigFontColour", 0) & 0xFF;
	if (OverrideColour2) SafeWrite32(0x481906, (DWORD)&OverrideColour2);
}
