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

#include "ConsoleWindow.h"

#include "FalloutEngine\Fallout2.h"
#include "Modules\LoadGameHook.h"
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

bool ConsoleWindow::tryGetWindow(HWND* wnd) {
	*wnd = GetConsoleWindow();
	if (!*wnd) {
		dlogr("Error getting console window.", DL_MAIN);
		return false;
	}
	return true;
}

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_CLOSE_EVENT) {
		ConsoleWindow::instance().savePosition();
	}
	return TRUE;
}

void ConsoleWindow::loadPosition() {
	HWND wnd;
	if (!tryGetWindow(&wnd)) return;

	if (HMENU hMenu = GetSystemMenu(wnd, FALSE)) {
		EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
	if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
		dlog_f("Error setting console ctrl handler: 0x%x\n", DL_MAIN, GetLastError());
	}

	auto windowDataStr = IniReader::GetStringDefaultConfig(IniSection, IniPositionKey, "");
	auto windowDataSplit = split(windowDataStr, ',');

	int windowData[5];
	for (size_t i = 0; i < 5; i++) {
		windowData[i] = i < windowDataSplit.size() ? atoi(windowDataSplit.at(i).c_str()) : 0;
	}
	LONG w = max(windowData[2], 640),
	     h = max(windowData[3], 480),
	     x = windowData[0],
	     y = windowData[1];
	UINT showCmd = windowData[4] != 0 ? windowData[4] : SW_SHOWNORMAL;

	dlog_f("Setting console window position: (%d, %d), size: %dx%d, showCmd: %d\n", DL_MAIN, x, y, w, h, showCmd);
	WINDOWPLACEMENT wPlacement{};
	wPlacement.length = sizeof(WINDOWPLACEMENT);
	auto& rect = wPlacement.rcNormalPosition;
	rect.left = x;
	rect.top = y;
	rect.right = x + w;
	rect.bottom = y + h;
	wPlacement.showCmd = showCmd;
	if (!SetWindowPlacement(wnd, &wPlacement)) {
		dlog_f("Error repositioning console window: 0x%x\n", DL_MAIN, GetLastError());
	}
}

void ConsoleWindow::savePosition() {
	HWND wnd;
	if (!tryGetWindow(&wnd)) return;

	WINDOWPLACEMENT wPlacement;
	wPlacement.length = sizeof(WINDOWPLACEMENT);
	if (!GetWindowPlacement(wnd, &wPlacement)) {
		dlog_f("Error getting console window placement: 0x%x\n", DL_MAIN, GetLastError());
		return;
	}
	RECT wndRect;
	if (wPlacement.showCmd != SW_SHOWNORMAL) {
		wndRect = wPlacement.rcNormalPosition;
	} else if (!GetWindowRect(wnd, &wndRect)) {
		dlog_f("Error getting console window rect: 0x%x\n", DL_MAIN, GetLastError());
		return;
	}
	int width = wndRect.right - wndRect.left;
	int height = wndRect.bottom - wndRect.top;
	std::ostringstream ss;
	ss << wndRect.left << "," << wndRect.top << "," << width << "," << height << "," << wPlacement.showCmd;
	auto wndDataStr = ss.str();
	dlog_f("Saving console window position & size: %s\n", DL_MAIN, wndDataStr.c_str());

	IniReader::SetDefaultConfigString(IniSection, IniPositionKey, wndDataStr.c_str());
}

static void __fastcall WriteGameLog(const char* a) {
	ConsoleWindow::instance().write(a, ConsoleWindow::Source::GAME);
}

static __declspec(naked) void debug_printf_hook() {
	__asm {
		call fo::funcoffs::vsprintf_;
		pushadc;
		lea ecx, [esp + 16];
		call WriteGameLog;
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

	LoadGameHook::OnBeforeGameClose() += [this] { savePosition(); };
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
