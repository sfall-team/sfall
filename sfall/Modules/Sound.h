/*
 *    sfall
 *    Copyright (C) 2008-2025  The sfall team
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

class Sound : public Module {
public:
	const char* name() { return "Sounds"; }
	void init();
	void exit() override;

	static void OnGameReset();
	static void OnAfterGameInit();
	static void OnBeforeGameClose();

	static DWORD PlaySfallSound(const char* path, long mode);
	static void __stdcall StopSfallSound(DWORD id);

	static long CalculateVolumeDB(long masterVolume, long passVolume);

	static void SoundLostFocus(long isActive);
};

}
