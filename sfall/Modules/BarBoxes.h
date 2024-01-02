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

#include "Module.h"

namespace sfall 
{

class BarBoxes : public Module {
private:
	static int boxCount;

public:
	const char* name() { return "BarBoxes"; }
	void init();
	void exit() override;

	static int  MaxBox() { return boxCount - 1; }
	static void SetText(int box, const char* text, DWORD color);

	static bool GetBox(int i);
	static void AddBox(int i);
	static void RemoveBox(int i);
	static long AddExtraBox();
};

}
