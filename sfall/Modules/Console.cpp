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

#include <fstream>

#include "..\main.h"

#include "Console.h"

namespace sfall
{

static std::ofstream consoleFile;

static void __stdcall ConsoleFilePrint(const char* msg) {
	consoleFile << msg << std::endl;
}

static const DWORD ConsoleHookRet = 0x431871;
static void __declspec(naked) ConsoleHook() {
	__asm {
		pushadc;
		push eax;
		call ConsoleFilePrint;
		popadc;
		push ebx;
		push ecx;
		push edx;
		push esi;
		push edi;
		jmp ConsoleHookRet;
	}
}

void Console::init() {
	auto path = GetConfigString("Misc", "ConsoleOutputPath", "", MAX_PATH);
	if (!path.empty()) {
		consoleFile.open(path);
		if (consoleFile.is_open()) {
			MakeJump(0x43186C, ConsoleHook);
		}
	}
}

void Console::exit() {
	if (consoleFile.is_open()) {
		consoleFile.close();
	}
}

}
