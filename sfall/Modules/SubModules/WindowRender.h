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

class WindowRender {
public:
	static void init();

	static void CreateOverlaySurface(fo::Window* win, long winType);
	static void DestroyOverlaySurface(fo::Window* win);
	static void ClearOverlay(fo::Window* win);
	static void ClearOverlay(fo::Window* win, Rectangle &rect);
	static BYTE* GetOverlaySurface(fo::Window* win);

	static void EnableRecalculateFadeSteps();
};

}
