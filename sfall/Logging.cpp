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

#include "main.h"
#include "Logging.h"
#include "FalloutEngine\Fallout2.h"

#ifndef NO_SFALL_DEBUG

#include <fstream>
#include <stdio.h>
#include <iostream>

namespace sfall
{

static int DebugTypes = 0;
static std::ofstream Log;
static int ConsoleWindowMode = 0;
static bool NewLine = true;

template <class T>
static inline void WriteLog(std::ostream& str, T a, int type, const char* prefix, bool newLine) {
	str << prefix << DebugTypeToStr(type) << "] " << a;
	if (newLine) str << "\n";
}

template <class T>
static void OutLog(T a, int type, bool newLine = false) {
	if (ConsoleWindowMode & 2) {
		WriteLog(std::cout, a, type, (ConsoleWindowMode & 1 ? "[sfall:" : "["), newLine);
	}
	WriteLog(Log, a, type, "[", newLine);
	Log.flush();
}

const char* DebugTypeToStr(int type) {
	switch (type) {
	case DL_INIT: return "init";
	case DL_HOOK: return "hook";
	case DL_SCRIPT: return "script";
	case DL_CRITICALS: return "crits";
	case DL_FIX: return "fix";
	default: return "main";
	}
}

void dlog(const char* a, int type) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		OutLog(a, type);
	}
}

void dlog(const std::string& a, int type) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		OutLog(a, type);
	}
}

void dlogr(const char* a, int type) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		OutLog(a, type, true);
	}
}

void dlogr(const std::string& a, int type) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		OutLog(a, type, true);
	}
}

void dlog_f(const char* fmt, int type, ...) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		va_list args;
		va_start(args, type);
		char buf[1024];
		vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
		OutLog(buf, type);
		va_end(args);
	}
}

// Prints debug message to sfall log file for develop build
#ifndef NDEBUG
void devlog_f(const char* fmt, int type, ...) {
	if (type == DL_MAIN || (type & DebugTypes)) {
		va_list args;
		va_start(args, type);
		char buf[1024];
		vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
		OutLog(buf, type);
		va_end(args);
	}
}
#else
void devlog_f(...) {}
#endif

static void __fastcall OutLogC(const char* a) {
	std::cout << a;
}

static void __declspec(naked) debug_printf_hack() {
	static const DWORD backRet = 0x4C6F7C;
	__asm {
		call fo::funcoffs::vsprintf_;
		pushadc;
		mov ecx, esp;
		add ecx, 12;
		call OutLogC;
		popadc;
		jmp backRet;
	}
}

void LoggingInit() {
	Log.open("sfall-log.txt", std::ios_base::out | std::ios_base::trunc);

	ConsoleWindowMode = IniReader::GetIntDefaultConfig("Debugging", "ConsoleWindow", 0);

	if (IniReader::GetIntDefaultConfig("Debugging", "Init", 0)) {
		DebugTypes |= DL_INIT;
	}
	if (IniReader::GetIntDefaultConfig("Debugging", "Hook", 0)) {
		DebugTypes |= DL_HOOK;
	}
	if (IniReader::GetIntDefaultConfig("Debugging", "Script", 0)) {
		DebugTypes |= DL_SCRIPT;
	}
	if (IniReader::GetIntDefaultConfig("Debugging", "Criticals", 0)) {
		DebugTypes |= DL_CRITICALS;
	}
	if (IniReader::GetIntDefaultConfig("Debugging", "Fixes", 0)) {
		DebugTypes |= DL_FIX;
	}
	if (ConsoleWindowMode > 0) {
		if (AllocConsole()) {
			freopen("CONOUT$", "w", stdout);

			if (ConsoleWindowMode & 1) {
				std::cout << "Displaying debug_printf output." << std::endl;
				MakeJump(0x4C6F77, debug_printf_hack);
			}
			if (ConsoleWindowMode & 2) {
				std::cout << "Displaying sfall debug output." << std::endl;
			}
		}
		else {
			dlog_f("Console Failed: %x", DL_MAIN, GetLastError());
		}
	}
}

}

#endif
