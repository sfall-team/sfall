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
#include "Utils.h"

#ifndef NO_SFALL_DEBUG

#include <fstream>
#include <iostream>
#include <sstream>

namespace sfall
{

enum ConsoleSource : int {
	GAME = 1,
	SFALL = 2
};

static int DebugTypes = 0;
static std::ofstream Log;

static int LastType = -1;
static int LastNewLine;

class ConsoleWindow {
public:
	static ConsoleWindow& instance() { return _instance; }

	ConsoleWindow() : _mode(0) {}
	~ConsoleWindow();

	void init();

	void loadPosition();
	void savePosition();

	void sfallLog(const std::string& a, int type);
	void falloutLog(const char* a);

private:
	static ConsoleWindow _instance;

	int _mode;
	ConsoleSource _lastSource;

	bool tryGetWindow(HWND* wnd);
};

ConsoleWindow ConsoleWindow::_instance;

bool ConsoleWindow::tryGetWindow(HWND* wnd) {
	*wnd = GetConsoleWindow();
	if (!*wnd) {
		dlogr("Error getting console window.", DL_MAIN);
		return false;
	}
	return true;
}

void ConsoleWindow::loadPosition() {
	std::string windowDataStr = IniReader::GetStringDefaultConfig("Debugging", "ConsoleWindowData", "");
	std::vector<std::string> windowDataSplit = split(windowDataStr, ',');
	if (windowDataSplit.size() < 4) return;

	HWND wnd;
	if (!tryGetWindow(&wnd)) return;

	int windowData[4];
	for (size_t i = 0; i < 4; i++) {
		windowData[i] = atoi(windowDataSplit.at(i).c_str());
	}
	if (!SetWindowPos(wnd, HWND_TOP, windowData[0], windowData[1], windowData[2], windowData[3], 0)) {
		dlog_f("Error repositioning console window: 0x%x\n", DL_MAIN, GetLastError());
	}
}

void ConsoleWindow::savePosition() {
	HWND wnd;
	if (!tryGetWindow(&wnd)) return;

	RECT wndRect;
	if (!GetWindowRect(wnd, &wndRect)) {
		dlog_f("Error getting console window position: 0x%x\n", DL_MAIN, GetLastError());
	}
	int width = wndRect.right - wndRect.left;
	int height = wndRect.bottom - wndRect.top;
	std::ostringstream ss;
	ss << wndRect.left << "," << wndRect.top << "," << width << "," << height;
	auto wndDataStr = ss.str();
	dlog_f("Saving console window position & size: %s\n", DL_MAIN, wndDataStr.c_str());

	IniReader::SetDefaultConfigString("Debugging", "ConsoleWindowData", wndDataStr.c_str());
}

static void __fastcall PrintToConsole(const char* a) {
	ConsoleWindow::instance().falloutLog(a);
}

static void __declspec(naked) debug_printf_hook() {
	__asm {
		call fo::funcoffs::vsprintf_;
		pushadc;
		lea ecx, [esp + 16];
		call PrintToConsole;
		popadc;
		retn;
	}
}

void ConsoleWindow::init() {
	_mode = IniReader::GetIntDefaultConfig("Debugging", "ConsoleWindow", 0);
	if (_mode == 0) return;
	if (!AllocConsole()) {
		dlog_f("Failed to allocate console: 0x%x\n", DL_MAIN, GetLastError());
		return;
	}
	int cp = IniReader::GetIntDefaultConfig("Debugging", "ConsoleCodePage", 0);
	if (cp > 0) SetConsoleOutputCP(cp);

	freopen("CONOUT$", "w", stdout); // this allows to print to console via std::cout

	if (_mode & ConsoleSource::GAME) {
		std::cout << "Displaying debug_printf output.\n";
		HookCall(0x4C6F77, debug_printf_hook);
	}
	if (_mode & ConsoleSource::SFALL) {
		std::cout << "Displaying sfall debug output.\n";
	}
	std::cout << std::endl;

	loadPosition();
}

ConsoleWindow::~ConsoleWindow() {
	if (_mode == 0) return;

	savePosition();
}

void ConsoleWindow::falloutLog(const char* a) {
	std::cout << a;
	_lastSource = ConsoleSource::GAME;
}

void ConsoleWindow::sfallLog(const std::string& a, int type) {
	if (!(_mode & ConsoleSource::SFALL)) return;

	if (_lastSource == ConsoleSource::GAME) {
		std::cout << "\n"; // To make logs prettier, because debug_msg places newline before the message.
	}
	std::cout << a;
	_lastSource = ConsoleSource::SFALL;
}


template <class T>
static void OutLog(T a, int type, bool newLine = false) {
	std::ostringstream ss;
	if (LastNewLine || type != LastType) {
		ss << "[" << DebugTypeToStr(type) << "] ";
	}
	ss << a;
	if (newLine) ss << "\n";
	std::string str = ss.str();

	ConsoleWindow::instance().sfallLog(str, type);

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

	ConsoleWindow::instance().init();
}

}

#endif
