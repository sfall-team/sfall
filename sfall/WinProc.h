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

class WinProc {
public:
	static void init();

	static void __stdcall MessageWindow();
	static void __stdcall WaitMessageWindow();

	static void SetWindowProc();

	static void SetHWND(HWND _window);
	static void SetTitle(long wWidth, long wHeight, long gMode);
	static void SetSize(long w, long h, char scaleX2);

	// Sets the window style and its position
	static void SetStyle(long windowStyle);

	static void SetMoveKeys();
	static void Moving();

	static void SetToCenter(long wWidth, long wHeight, long* outX, long* outY);
	static void LoadPosition();
	static void SavePosition(long mode);

	static const POINT* GetClientPos();
};

}
