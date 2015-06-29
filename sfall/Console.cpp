/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include "Console.h"
#include <fstream>

using namespace std;

static ofstream consolefile;

static void _stdcall ConsoleFilePrint(const char* msg) {
	consolefile << msg << endl;
}

static const DWORD ConsoleHookRet=0x431871;
static void __declspec(naked) ConsoleHook() {
	__asm {
		pushad;
		push eax;
		call ConsoleFilePrint;
		popad;
		push ebx;
		push ecx;
		push edx;
		push esi;
		push edi;
		jmp ConsoleHookRet;
	}
}

void ConsoleInit() {
	char path[MAX_PATH];
	GetPrivateProfileString("Misc", "ConsoleOutputPath", "", path, MAX_PATH, ini);
	if(strlen(path)>0) {
		consolefile.open(path);
		if(consolefile.is_open()) MakeCall(0x43186C, &ConsoleHook, true);
	}
}

void ConsoleExit() {
	if(consolefile.is_open()) consolefile.close();
}



/*  An attempt to make popup display function.. failed to make it work, though all arguments are there. 
	I missed something...

static const DWORD dialog_out_Addr = 0x41CF20;
void DisplayAlertPopup(const char* msg, const char* line2) {
	__asm {
		push	1
		mov		al, ds:[0x6AB718]
		push	eax
		push	0
		push	eax
		mov		ecx, 0x0A9
		mov		ebx, 2
		push	0x74
		mov		edx, msg
		mov		eax, line2
		mov		esi, 0FFFFFFFFh
		mov		edi, 2
		call	dialog_out_Addr
    }
}*/