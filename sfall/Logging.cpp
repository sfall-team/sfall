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
#include "Logging.h"

#ifndef NO_SFALL_DEBUG

#include <fstream>

namespace sfall
{

static int DebugTypes = 0;
static std::ofstream Log;

template <class T>
static void OutLog(T a) {
	Log << a;
	Log.flush();
}

template <class T>
static void OutLogN(T a) {
	Log << a << "\n";
	Log.flush();
}

void dlog(const char* a, int type) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		OutLog(a);
	}
}

void dlogr(const char* a, int type) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		OutLogN(a);
	}
}

void dlog_f(const char* fmt, int type, ...) {
	if (type == DL_MAIN || (isDebug && (type & DebugTypes))) {
		va_list args;
		va_start(args, type);
		char buf[1024];
		vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
		OutLog(buf);
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
		OutLog(buf);
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

#endif
