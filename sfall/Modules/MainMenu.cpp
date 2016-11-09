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

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "..\Version.h"

static DWORD MainMenuYOffset;
static DWORD MainMenuTextOffset;

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
		jmp dword ptr ds:[VARPTR_text_to_buf];
	}
}

static const DWORD MainMenuTextRet = 0x4817B0;
static const char* VerString1 = "SFALL " VERSION_STRING;
static DWORD OverrideColour;
static void __declspec(naked) FontColour() {
	__asm {
		cmp OverrideColour, 0;
		je skip;
		mov eax, OverrideColour;
		retn;
skip:
		movzx eax, byte ptr ds:[0x6A8B33];
		or eax, 0x6000000;
		retn;
	}
}

static void __declspec(naked) MainMenuTextHook() {
	__asm {
		mov edi, [esp];
		sub edi, 12; //shift yposition up by 12
		mov [esp], edi;
		mov ebp, ecx;
		push eax;
		call FontColour;
		mov [esp+8], eax;
		pop eax;
		call FuncOffs::win_print_;
		call FontColour;
		push eax;//colour
		mov edx, VerString1;//msg
		xor ebx, ebx;//font
		mov ecx, ebp;
		dec ecx; //xpos
		add edi, 12;
		push edi; //ypos
		mov eax, dword ptr ds:[VARPTR_main_window];//winptr
		call FuncOffs::win_print_;
		jmp MainMenuTextRet;
	}
}

void MainMenuInit() {
	int tmp;

	if(tmp=GetPrivateProfileIntA("Misc", "MainMenuCreditsOffsetX", 0, ini)) {
		SafeWrite32(0x481753, 0xf+tmp);
	}
	if(tmp=GetPrivateProfileIntA("Misc", "MainMenuCreditsOffsetY", 0, ini)) {
		SafeWrite32(0x48175C, 0x1cc+tmp);
	}
	if(tmp=GetPrivateProfileIntA("Misc", "MainMenuOffsetX", 0, ini)) {
		SafeWrite32(0x48187C, 0x1e+tmp);
		MainMenuTextOffset=tmp;
	}
	if(tmp=GetPrivateProfileIntA("Misc", "MainMenuOffsetY", 0, ini)) {
		MainMenuYOffset=tmp;
		MainMenuTextOffset+=tmp*640;
		MakeCall(0x481844, &MainMenuButtonYHook, true);
	}
	if(MainMenuTextOffset) {
		SafeWrite8(0x481933, 0x90);
		MakeCall(0x481934, &MainMenuTextYHook, false);
	}

	MakeCall(0x4817AB, MainMenuTextHook, true);
	OverrideColour=GetPrivateProfileInt("Misc", "MainMenuFontColour", 0, ini);
	if(OverrideColour) MakeCall(0x48174C, &FontColour, false);
}
