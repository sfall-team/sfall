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

void LoadGameHookInit();

bool IsMapLoaded();
DWORD InWorldMap();
DWORD InCombat();

#define WORLDMAP   (1<<0) //0x1
#define LOCALMAP   (1<<1) //0x2 No point hooking this: would always be 1 at any point at which scripts are running
#define DIALOG     (1<<2) //0x4
#define ESCMENU    (1<<3) //0x8
#define SAVEGAME   (1<<4) //0x10
#define LOADGAME   (1<<5) //0x20
#define COMBAT     (1<<6) //0x40
#define OPTIONS    (1<<7) //0x80
#define HELP       (1<<8) //0x100
#define CHARSCREEN (1<<9) //0x200
#define PIPBOY     (1<<10)//0x400
#define PCOMBAT    (1<<11)//0x800
#define INVENTORY  (1<<12)//0x1000
#define AUTOMAP    (1<<13)//0x2000
#define SKILLDEX   (1<<14)//0x4000
#define RESERVED   (1<<31)

DWORD GetCurrentLoops();