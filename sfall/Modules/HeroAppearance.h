/*
 *    sfall
 *    Copyright (C) 2009  Mash (Matt Wells, mashw at bigpond dot net dot au)
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

extern bool appModEnabled;

void HeroAppearance_Init();
void HeroAppearance_Exit();

void __stdcall AdjustHeroArmorArt(DWORD fid);

void __stdcall HeroSelectWindow(int raceStyleFlag);
void __stdcall SetHeroStyle(int newStyleVal);
void __stdcall SetHeroRace(int newRaceVal);
void __stdcall LoadHeroAppearance(void);
void __stdcall SetNewCharAppearanceGlobals(void);

void __stdcall RefreshPCArt();
