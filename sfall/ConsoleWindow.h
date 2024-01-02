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

#pragma once

namespace sfall
{

class ConsoleWindow {
public:

	enum Source : int {
		GAME = 1,
		SFALL = 2,
		DEBUG_MSG = 4,
		DISPLAY_MSG = 8,
	};

	static ConsoleWindow& instance() {
		static ConsoleWindow instance;
		return instance;
	}

	ConsoleWindow() {}

	void init();

	void loadPosition();
	void savePosition();

	void write(const char* message, Source source);

private:
	int _mode = 0;
	Source _lastSource;

	bool tryGetWindow(HWND* wnd);
};

}
