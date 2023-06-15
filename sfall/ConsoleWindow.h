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

#pragma once

namespace sfall
{

enum ConsoleSource : int {
	GAME = 1,
	SFALL = 2
};

class ConsoleWindow {
public:
	static constexpr char* IniSection = "Debugging";
	static constexpr char* IniModeKey = "ConsoleWindow";
	static constexpr char* IniPositionKey = "ConsoleWindowData";

	static ConsoleWindow& instance() { return _instance; }

	ConsoleWindow() {}
	~ConsoleWindow();

	void init();

	void loadPosition();
	void savePosition();

	void sfallLog(const std::string& a, int type);
	void falloutLog(const char* a);

private:
	static ConsoleWindow _instance;

	int _mode = 0;
	ConsoleSource _lastSource;

	bool tryGetWindow(HWND* wnd);
};

}
