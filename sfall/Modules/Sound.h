/*
 *    sfall
 *    Copyright (C) 2008-2020  The sfall team
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

class Sound {
public:
	static const char* name() { return "Sounds"; }
	static void init();
	static void exit();

	static void OnGameLoad();
	static void OnAfterGameInit();
	static void OnBeforeGameClose();

	static DWORD __stdcall PlaySfallSound(const char* path, long mode);
	static void __stdcall StopSfallSound(DWORD id);

	static long CalculateVolumeDB(long masterVolume, long passVolume);

	static void SoundLostFocus(long isActive);
};

}
