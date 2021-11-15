/*
 *    sfall
 *    Copyright (C) 2008, 2009  The sfall team
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

class LoadGameHook {
public:
	static const char* name() { return "LoadGameHook"; }
	static void init();

	static bool IsMapLoading();

	static char mapLoadingName[16];
	static long interfaceWID;
};

// True if game was started, false when on the main menu
bool IsGameLoaded();

DWORD InWorldMap();

DWORD InCombat();

DWORD InDialog();

enum LoopFlag : unsigned long {
	WORLDMAP    = 1 << 0,   // 0x1
//	RESERVED    = 1 << 1,   // 0x2 (unused)
	DIALOG      = 1 << 2,   // 0x4
	ESCMENU     = 1 << 3,   // 0x8
	SAVEGAME    = 1 << 4,   // 0x10
	LOADGAME    = 1 << 5,   // 0x20
	COMBAT      = 1 << 6,   // 0x40
	OPTIONS     = 1 << 7,   // 0x80
	HELP        = 1 << 8,   // 0x100
	CHARSCREEN  = 1 << 9,   // 0x200
	PIPBOY      = 1 << 10,  // 0x400
	PCOMBAT     = 1 << 11,  // 0x800
	INVENTORY   = 1 << 12,  // 0x1000
	AUTOMAP     = 1 << 13,  // 0x2000
	SKILLDEX    = 1 << 14,  // 0x4000
	INTFACEUSE  = 1 << 15,  // 0x8000
	INTFACELOOT = 1 << 16,  // 0x10000
	BARTER      = 1 << 17,  // 0x20000
	HEROWIN     = 1 << 18,  // 0x40000 Hero Appearance mod
	DIALOGVIEW  = 1 << 19,  // 0x80000
	COUNTERWIN  = 1 << 20,  // 0x100000 Counter window for moving multiple items or setting a timer

	SPECIAL     = 1UL << 31 // 0x80000000 Additional special flag for all modes
};

DWORD GetLoopFlags();

// set the given flag
void SetLoopFlag(LoopFlag flag);

// unset the given flag
void ClearLoopFlag(LoopFlag flag);

void GetSavePath(char* buf, char* ftype);

}
