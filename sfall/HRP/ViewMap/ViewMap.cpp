/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "EdgeBorder.h"

#include "ViewMap.h"

namespace sfall
{

long ViewMap::SCROLL_DIST_X;
long ViewMap::SCROLL_DIST_Y;

static long mapDisplayWinWidthMod;
static long mapDisplayWinHeightMod;

void ViewMap::GetTileCoord(long tile, long &outX, long &outY) {
	int x = tile % 200;             // tile % _grid_width           // 20100%200 = 100
	int y = (tile / 200) + (x / 2); // tile / _grid_width + x / 2   // 20100/200 = 100+(100/2) = y:150 ???
	outY = y;
	outX = (2 * x) - y;                                             // 2*100 = 200-150 = x:50 ???
}

void ViewMap::GetTileCoordOffset(long tile, long &outX, long &outY) {
	int x = tile % 200;             // tile % _grid_width           // 20100%200 = 100
	int y = (tile / 200) + (x / 2); // tile / _grid_width + x / 2   // 20100/200 = 100+(100/2) = y:150 ???
	y &= 0xFFFFFFFE; // even to down
	x = (2 * x) + 200 - y;                                          // 2*100 = 200+200-150 = x:250 ???

	outY = 12 * y;
	outX = 16 * x;
}

void ViewMap::GetMapWindowSize(long &outW, long &outH) {
	//win = fo::func::GNW_find(fo::var::display_win);
	long mapWinWidth = fo::var::getInt(FO_VAR_buf_width_2);
	long mapWinHeight = fo::var::getInt(FO_VAR_buf_length_2);

	long wHalf = mapWinWidth >> 1;         // 1280/2 = 640
	mapDisplayWinWidthMod = wHalf & 31;    // 640&31 = 0
	outW = wHalf - mapDisplayWinWidthMod;  // truncate by 32 units

	long hHalf = mapWinHeight >> 1;        // 720/2 = 360
	mapDisplayWinHeightMod = hHalf % 24;   // 360%24=0
	outH = hHalf - mapDisplayWinHeightMod; // truncate by 24 units
}

void ViewMap::init() {

	if (SCROLL_DIST_X < 480) SCROLL_DIST_X = 480;
	if (SCROLL_DIST_Y < 400) SCROLL_DIST_Y = 400;


	// Dev block tile_set_border_
	BlockCall(0x4B11A3); // tile_init_

	EdgeBorder::init();
}

}
