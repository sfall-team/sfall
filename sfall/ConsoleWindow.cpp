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
	std::string wndDataStr = ss.str();
	dlog_f("Saving console window position & size: %s\n", DL_MAIN, wndDataStr.c_str());

	IniReader::SetDefaultConfigString("Debugging", "ConsoleWindowData", wndDataStr.c_str());
}

static void __fastcall WriteGameLog(const char* a) {
	ConsoleWindow::instance().write(a, ConsoleWindow::Source::GAME);
}

static void __declspec(naked) debug_printf_hook() {
	__asm {
		call fo::funcoffs::vsprintf_;
		pushadc;
		lea ecx, [esp + 16];
		call WriteGameLog;
		popadc;
		retn;
	}
}

void ConsoleWindow::OnBeforeGameClose() {
	if (instance()._mode) instance().savePosition();
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

	if (_mode & Source::GAME) {
		std::cout << "Displaying debug_printf output.\n";
		HookCall(0x4C6F77, debug_printf_hook);
	}
	if (_mode & Source::SFALL) {
		std::cout << "Displaying sfall debug output.\n";
	}
	if (_mode & Source::DEBUG_MSG) {
		std::cout << "Displaying debug_msg output.\n";
	}
	if (_mode & Source::DISPLAY_MSG) {
		std::cout << "Displaying display_msg output.\n";
	}
	std::cout << std::endl;

	loadPosition();
}

void ConsoleWindow::write(const char* message, ConsoleWindow::Source source) {
	if (!(_mode & source)) return;

	if (source == Source::SFALL && _lastSource != Source::SFALL) {
		std::cout << "\n"; // To make logs prettier, because debug_msg places newline before the message.
	}
	std::cout << message;
	_lastSource = source;
}

}
