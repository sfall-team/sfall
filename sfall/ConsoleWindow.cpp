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

#include "ConsoleWindow.h"

#include "FalloutEngine\Fallout2.h"
#include "IniReader.h"
#include "Logging.h"
#include "SafeWrite.h"
#include "Utils.h"

#include <iostream>
#include <sstream>

namespace sfall
{

static constexpr char* IniSection = "Debugging";
static constexpr char* IniModeKey = "ConsoleWindow";
static constexpr char* IniPositionKey = "ConsoleWindowData";
static constexpr char* IniCodePageKey = "ConsoleCodePage";

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
	auto windowDataStr = IniReader::GetStringDefaultConfig(IniSection, IniPositionKey, "");
	auto windowDataSplit = split(windowDataStr, ',');
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

	IniReader::SetDefaultConfigString(IniSection, IniPositionKey, wndDataStr.c_str());
}

static void __fastcall PrintDebugLog(const char* a) {
	ConsoleWindow::instance().falloutLog(a);
}

static void __declspec(naked) debug_printf_hook() {
	__asm {
		call fo::funcoffs::vsprintf_;
		pushadc;
		lea ecx, [esp + 16];
		call PrintDebugLog;
		popadc;
		retn;
	}
}

void ConsoleWindow::init() {
	_mode = IniReader::GetIntDefaultConfig(IniSection, IniModeKey, 0);
	if (_mode == 0) return;
	if (!AllocConsole()) {
		dlog_f("Failed to allocate console: 0x%x\n", DL_MAIN, GetLastError());
		return;
	}
	int cp = IniReader::GetIntDefaultConfig(IniSection, IniCodePageKey, 0);
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

}
