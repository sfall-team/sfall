/*
 *    sfall
 *    Copyright (C) 2008-2019  The sfall team
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

void WorldmapInit();
void Worldmap_OnGameLoad();

DWORD GetAddedYears(bool isCheck = true);
void SetAddedYears(DWORD years);

void Wmap_SetTerrainTypeName(long x, long y, const char* name);
//const char* Wmap_GetTerrainTypeName(long x, long y);
const char* Wmap_GetCurrentTerrainName();

bool Wmap_AreaTitlesIsEmpty();
const char* Wmap_GetCustomAreaTitle(long areaID);
void Wmap_SetCustomAreaTitle(long areaID, const char* msg);

void _stdcall SetMapMulti(float value);
