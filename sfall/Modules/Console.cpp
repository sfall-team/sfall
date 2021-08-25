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
#include "..\Modules\LoadGameHook.h"

#include "Console.h"

namespace sfall
{

static bool fileIsOpen = false;
static std::ofstream consoleFile;

static void __fastcall ConsoleFilePrint(const char* msg) {
	consoleFile << msg << '\n';
}

static void __declspec(naked) display_print_hack() {
	static const DWORD display_print_Ret = 0x431871;
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		push edi;
		mov  ebx, eax;
		mov  ecx, eax;
		call ConsoleFilePrint;
		mov  eax, ebx;
		jmp  display_print_Ret;
	}
}

void Console::PrintFile(const char* msg) {
	if (fileIsOpen) ConsoleFilePrint(msg);
}

void Console::init() {
	auto path = IniReader::GetConfigString("Misc", "ConsoleOutputPath", "", MAX_PATH);
	if (!path.empty()) {
		consoleFile.open(path);
		if (consoleFile.is_open()) {
			fileIsOpen = true;
			MakeJump(0x43186C, display_print_hack);

			LoadGameHook::OnGameReset() += []() {
				consoleFile.flush();
			};
		}
	}
}

void Console::exit() {
	if (consoleFile.is_open()) {
		consoleFile.close();
	}
}

}
