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

#include <stdio.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\version.h"

#include "Credits.h"

namespace sfall
{

static bool InCredits = false;
static DWORD CreditsLine = 0;

static const char* ExtraLines[] = {
	"#SFALL " VERSION_STRING,
	"",
	"sfall is free software, licensed under the GPL",
	LEGAL_COPYRIGHT,
	"",
	"@Author",
	"Timeslip",
	"",
	"@Contributors",
	"@(in chronological order)",
	"ravachol",
	"Noid",
	"Glovz",
	"Dream",
	"Ray",
	"Kanhef",
	"KLIMaka",
	"Mash",
	"Helios",
	"Haenlomal",
	"NVShacker",
	"NovaRain",
	"JimTheDinosaur",
	"phobos2077",
	"Tehnokrat",
	"Crafty",
	"Slider2k",
	"Vennor",
	"Oppen",
	"Mr.Stalin",
	"Ghosthack",
	"",
	"@Additional thanks",
	"Nirran",
	"killap",
	"MIB88",
	"Rain man",
	"Continuum",
	"Drobovik",
	"burn",
	"Lexx",
	"Anyone who has used sfall in their own mods",
	"The bug reporters and feature requesters",
	"",
	"",
	"",
	"#FALLOUT 2",
	""
};

static DWORD ExtraLineCount = sizeof(ExtraLines) / 4;

static DWORD __fastcall CreditsNextLine(char* buf, DWORD* font, DWORD* colour) {
	if (!InCredits || CreditsLine >= ExtraLineCount) return 0;
	const char* line = ExtraLines[CreditsLine++];
	if (strlen(line)) {
		if (line[0] == '#') {
			line++;
			*font = *fo::ptr::name_font;
			*colour = *(BYTE*)0x6A7F01;
		} else if (line[0] == '@') {
			line++;
			*font = *fo::ptr::title_font;
			*colour = *fo::ptr::title_color;
		} else {
			*font = *fo::ptr::name_font;
			*colour = *fo::ptr::name_color;
		}
	}
	strcpy_s(buf, 256, line);
	return 1;
}

// Additional lines will be at the top of CREDITS.TXT contents
static void __declspec(naked) CreditsNextLineHook_Top() {
	__asm {
		pushadc;
		push ebx;
		mov  ecx, eax;
		call CreditsNextLine; // edx - font
		test eax, eax;
		popadc;
		jz   fail;
		xor  eax, eax;
		inc  eax;
		retn;
fail:
		jmp  fo::funcoffs::credits_get_next_line_;
	}
}

// Additional lines will be at the bottom of CREDITS.TXT contents
static void __declspec(naked) CreditsNextLineHook_Bottom() {
	__asm {
		push eax;
		push edx;
		push ebx;
		call fo::funcoffs::credits_get_next_line_;  // call default function
		test eax, eax;
		pop  ebx;
		pop  edx;
		pop  eax;
		jnz  morelines;               // if not the end yet, skip custom code
		pushadc;
		push ebx;
		mov  ecx, eax;
		call CreditsNextLine;         // otherwise call out function
		test eax, eax;                // if any extra lines left, return 1 (from function), 0 otherwise
		popadc;
		jnz  morelines;
		xor  eax, eax;
		retn;
morelines:
		mov  eax, 1;
		retn;
	}
}

static void __stdcall SetCreditsPosition(DWORD addr) {
	void* func;
	if (addr == (0x43F881 + 5)) { // called from endgame_movie_
		func = CreditsNextLineHook_Bottom;
	} else {
		func = CreditsNextLineHook_Top;
	}
	HookCall(0x42CB49, func);
}

static void __declspec(naked) ShowCreditsHook() {
	__asm {
		push eax;
		push edx;
		push ebx;
		push [esp + 12]; // return address
		call SetCreditsPosition;
		pop  ebx;
		pop  edx;
		pop  eax;
		mov  InCredits, 1;
		mov  CreditsLine, 0;
		call fo::funcoffs::credits_;
		mov  InCredits, 0;
		retn;
	}
}

void Credits::init() {
	const DWORD showCreditsAddr[] = {0x480C49, 0x43F881};
	HookCalls(ShowCreditsHook, showCreditsAddr);
}

}
