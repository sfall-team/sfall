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

//for holding window info
typedef struct WINinfo {
	DWORD ref;
	DWORD flags;
	RECT wRect;
	DWORD width;
	DWORD height;
	DWORD clearColour;
	DWORD unknown2;
	DWORD unknown3;
	BYTE *surface; //bytes frame data ref to palette
	DWORD buttonListP;
	DWORD unknown5; //buttonptr?
	DWORD unknown6;
	DWORD unknown7;
	DWORD drawFuncP;
} WINinfo;

WINinfo *GetWinStruct(int WinRef);
void RedrawWin(int WinRef);
void PrintText(char *DisplayText, BYTE ColourIndex, DWORD Xpos, DWORD Ypos, DWORD TxtWidth, DWORD ToWidth, BYTE *ToSurface);
DWORD GetTextWidth(char *TextMsg);
DWORD GetMaxCharWidth();
int check_buttons(void);

void EnableHeroAppearanceMod();
void HeroAppearanceModExit();

void _stdcall HeroSelectWindow(int RaceStyleFlag);
void _stdcall SetHeroStyle(int newStyleVal);
void _stdcall SetHeroRace(int newRaceVal);
void _stdcall LoadHeroAppearance(void);
void _stdcall SetNewCharAppearanceGlobals(void);

void _stdcall RefreshPCArt();
