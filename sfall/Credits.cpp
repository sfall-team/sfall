/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "Version.h"
#include <stdio.h>

static DWORD InCredits=0;
static DWORD CreditsLine=0;

static const char* ExtraLines[]={
	"#SFALL " VERSION_STRING,
	"",
	"sfall is free software, licensed under the GPL",
	"Copyright 2008-2015  The sfall team",
	"",
	"@Author",
	"Timeslip",
	"",
	"@Contributors",
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
	"",
	"@Additional thanks to",
	"Nirran",
	"killap",
	"MIB88",
	"Rain man",
	"Continuum",
	"Fakeman",
	"Drobovik",
	"Anyone who has used sfall in their own mods",
	"The bug reporters and feature requesters",
	""
	"",
	"",
	"#FALLOUT 2",
	""
};
static DWORD ExtraLineCount=sizeof(ExtraLines)/4;

static const char* creditsFile="credits.txt";

static void _stdcall ShowCreditsHook() {
	InCredits=1;
	CreditsLine=0;
	__asm {
		mov eax, creditsFile;
		mov ebx, 0x42C860;
		call ebx;
	}
	InCredits=0;
}

static DWORD _stdcall CreditsNextLine(char* buf, DWORD* font, DWORD* colour) {
	if(!InCredits||CreditsLine>=ExtraLineCount) return 0;
	const char* line=ExtraLines[CreditsLine++];
	if(strlen(line)) {
		if(line[0]=='#') {
			line++;
			*font=*(DWORD*)0x56D74C;
			*colour=*(BYTE*)0x6A7F01;
		} else if(line[0]=='@') {
			line++;
			*font=*(DWORD*)0x56D748;
			*colour=*(DWORD*)0x56D750;
		} else {
			*font=*(DWORD*)0x56D74C;
			*colour=*(DWORD*)0x56D744;
		}
	}
	strcpy_s(buf, 256, line);
	return 1;
}

static const DWORD _credits_get_next_line=0x42CE6C;
static void __declspec(naked) CreditsNextLineHook() {
	__asm {
		pushad;
		push ebx;
		push edx;
		push eax;
		call CreditsNextLine;
		test eax, eax;
		jz fail;
		popad;
		xor eax, eax;
		inc eax;
		retn;
fail:
		popad;
		jmp _credits_get_next_line;
	}
}

void CreditsInit() {
	HookCall(0x480C49, &ShowCreditsHook);
	HookCall(0x43F881, &ShowCreditsHook);
	HookCall(0x42CB49, &CreditsNextLineHook);
}