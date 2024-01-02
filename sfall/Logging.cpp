/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "Logging.h"

#include "main.h"
#include "FalloutEngine\Fallout2.h"
#include "ConsoleWindow.h"
#include "Utils.h"

#include <fstream>

namespace sfall
{

static int DebugTypes = 0;
static std::ofstream Log;

static int LastType = -1;
static int LastNewLine;

template <class T>
static void OutLog(T a, int type, bool newLine = false) {
	std::ostringstream ss;
	if (LastNewLine || type != LastType) {
		ss << "[" << DebugTypeToStr(type) << "] ";
	}
	ss << a;
	if (newLine) ss << "\n";
	std::string str = ss.str();

	ConsoleWindow::instance().write(str.c_str(), ConsoleWindow::Source::SFALL);

	Log << str;
	Log.flush();

	LastType = type;
	LastNewLine = str.back() == '\n';
}

const char* DebugTypeToStr(int type) {
	switch (type) {
	case DL_INIT: return "Init";
	case DL_HOOK: return "Hook";
	case DL_SCRIPT: return "Script";
	case DL_CRITICALS: return "Crits";
	case DL_FIX: return "Fix";
	default: return "Main";
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
		int written = vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
		if (written > 0) {
			OutLog(buf, type);
		}
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
		int written = vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
		if (written > 0) {
			OutLog(buf, type);
		}
		va_end(args);
	}
}
#else
void devlog_f(...) {}
#endif

void LoggingInit() {
	Log.open("sfall-log.txt", std::ios_base::out | std::ios_base::trunc);

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
}

}
