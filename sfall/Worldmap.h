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

DWORD Worldmap_GetAddedYears(bool isCheck = true);
void Worldmap_SetAddedYears(DWORD years);

void Worldmap_SetTerrainTypeName(long x, long y, const char* name);
//const char* Worldmap_GetTerrainTypeName(long x, long y);
const char* Worldmap_GetCurrentTerrainName();

bool Worldmap_AreaTitlesIsEmpty();
const char* Worldmap_GetCustomAreaTitle(long areaID);
void Worldmap_SetCustomAreaTitle(long areaID, const char* msg);

long Worldmap_AreaMarkStateIsNoRadius();

void __stdcall SetMapMulti(float value);
