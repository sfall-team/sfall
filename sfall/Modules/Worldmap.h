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

#include "..\Delegate.h"
#include "Module.h"

namespace sfall
{

class Worldmap : public Module {
public:
	const char* name() { return "Worldmap"; }
	void init();

	static Delegate<>& OnWorldmapLoop();

	static void SaveData(HANDLE);
	static bool LoadData(HANDLE);

	static void SetCarInterfaceArt(DWORD artIndex);
	static void SetRestHealTime(long minutes);
	static void SetWorldMapHealTime(long minutes);
	static void SetRestMode(DWORD mode);
	static void SetRestMapLevel(int mapId, long elev, bool canRest);
	static long __fastcall GetRestMapLevel(long elev, int mapId);

	static DWORD GetAddedYears(bool isCheck = true);
	static void SetAddedYears(DWORD years);

	static void SetTerrainTypeName(long x, long y, const char* name);
	static const char* GetTerrainTypeName(long x, long y);
	static const char* GetCurrentTerrainName();

	static bool AreaTitlesIsEmpty();
	static const char* GetCustomAreaTitle(long areaID);
	static void SetCustomAreaTitle(long areaID, const char* msg);

	static long AreaMarkStateIsNoRadius();
	static void SetMapMulti(float value);
};

}
