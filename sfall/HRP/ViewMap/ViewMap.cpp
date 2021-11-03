/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\Modules\LoadGameHook.h"

#include "..\Init.h"
#include "EdgeBorder.h"
#include "EdgeClipping.h"

#include "ViewMap.h"

namespace sfall
{

static Rectangle obj_on_screen_rect;

long ViewMap::SCROLL_DIST_X;
long ViewMap::SCROLL_DIST_Y;
bool ViewMap::IGNORE_PLAYER_SCROLL_LIMITS;
bool ViewMap::IGNORE_MAP_EDGES;
bool ViewMap::EDGE_CLIPPING_ON;

long ViewMap::mapWidthModSize;
long ViewMap::mapHeightModSize;
long ViewMap::mapModWidth;
long ViewMap::mapModHeight;

// Gets the x/y coordinates of the tile from the x/y offset position values
void ViewMap::GetCoordFromOffset(long &inOutX, long &inOutY) {
	int y = inOutY / 24;
	int x = (inOutX / 32) + y - 100;
	inOutX = x;
	inOutY = (2 * y) - (x / 2);
}

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

void ViewMap::GetWinMapHalfSize(long &outW, long &outH) {
	long mapWinWidth = fo::var::getInt(FO_VAR_buf_width_2);
	long mapWinHeight = fo::var::getInt(FO_VAR_buf_length_2);

	long wHalf = mapWinWidth >> 1;
	mapWidthModSize = wHalf & 31;
	outW = wHalf - mapWidthModSize;  // truncated by 32 units

	long hHalf = mapWinHeight >> 1;
	mapHeightModSize = hHalf % 24;
	outH = hHalf - mapHeightModSize; // reduced by the remainder
}

// Implementation from HRP by Mash
static long __fastcall tile_set_center(long tile, long modeFlags) {
	if (tile < 0 || tile >= 40000) return -1; // _grid_size: 40000

	long mapElevation = fo::var::map_elevation;

	if (modeFlags) tile = EdgeBorder::GetCenterTile(tile, mapElevation);

	if (!(modeFlags & 2) && fo::var::getInt(FO_VAR_scroll_limiting_on)) {
		long x, y;
		ViewMap::GetTileCoord(tile, x, y);

		long dudeX, dudeY;
		ViewMap::GetTileCoord(fo::var::obj_dude->tile, dudeX, dudeY);

		long distanceX = 16 * std::abs(x - dudeX);
		long distanceY = 12 * std::abs(y - dudeY);

		if (distanceX >= ViewMap::SCROLL_DIST_X || distanceY >= ViewMap::SCROLL_DIST_Y) {
			long centerX, centerY;
			ViewMap::GetTileCoord(fo::var::getInt(FO_VAR_tile_center_tile), centerX, centerY);

			if ((16 * std::abs(centerX - dudeX)) < distanceX || (12 * std::abs(centerY - dudeY)) < distanceY) {
				return -1; // scroll block
			}
		}
	}

	if (!(modeFlags & 2) && fo::var::getInt(FO_VAR_scroll_blocking_on)) {
		long result = EdgeBorder::CheckBorder(tile);
		if (!result) return -1; // scroll block
		if (result == 1) modeFlags |= 1; // redraw
	}

	long mapWinW = fo::var::getInt(FO_VAR_buf_width_2);
	long mapWinH = fo::var::getInt(FO_VAR_buf_length_2);

	long tile_offx = ViewMap::mapModWidth + (mapWinW - 32) / 2;
	long tile_offy = ViewMap::mapModHeight + (mapWinH - 16) / 2;

	/* vanilla code */

	fo::var::setInt(FO_VAR_tile_center_tile) = tile;

	long tile_y = tile / 200; // _grid_width
	long tile_x = 200 - (tile % 200) - 1;

	if (tile_x & 1) {
		tile_x--;
		tile_offx -= 32;
	}

	fo::var::setInt(FO_VAR_tile_x) = tile_x;
	fo::var::setInt(FO_VAR_tile_y) = tile_y;
	fo::var::setInt(FO_VAR_tile_offx) = tile_offx;
	fo::var::setInt(FO_VAR_tile_offy) = tile_offy;

	if (tile_y & 1) {
		tile_offy -= 12;
		tile_offx -= 16;
	}

	// set square variables
	fo::var::square_rect.y = tile_y / 2;
	fo::var::square_rect.x = tile_x / 2;
	fo::var::square_rect.offx = tile_offx - 16;
	fo::var::square_rect.offy = tile_offy - 2;

	if (modeFlags & 1 && fo::var::getInt(FO_VAR_refresh_enabled)) {
		fo::func::tile_refresh_display();
		return -1; // this fixes a map rendering bug, but why?
	}
	return 0;
}

static void __declspec(naked) tile_set_center_hack_replacement() {
	__asm {
		push ecx;
		mov  ecx, eax;
		call tile_set_center;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) tile_scroll_to_hack_replacement() {
	__asm {
		//mov  edx, 3; // modeFlags is always 2
		jmp  tile_set_center_hack_replacement;
	}
}

void ViewMap::init() {
	if (SCROLL_DIST_X <= 0) SCROLL_DIST_X = 480;
	else if (SCROLL_DIST_X < 320) SCROLL_DIST_X = 320;

	if (SCROLL_DIST_Y <= 0) SCROLL_DIST_Y = 400;
	else if (SCROLL_DIST_Y < 240) SCROLL_DIST_Y = 240;

	MakeJump(fo::funcoffs::tile_set_center_, tile_set_center_hack_replacement); // 0x4B12F8
	MakeJump(fo::funcoffs::tile_scroll_to_, tile_scroll_to_hack_replacement);   // 0x4B3924

	obj_on_screen_rect.width = HRP::ScreenWidth();
	obj_on_screen_rect.height = HRP::ScreenHeight(); // maybe take into account the interface bar and be: height - 100?
	SafeWrite32(0x45C886, (DWORD)&obj_on_screen_rect); // replace rectangle in op_obj_on_screen_

	EdgeBorder::init();
	EdgeClipping::init();

	LoadGameHook::OnAfterGameInit() += []() {
		if (IGNORE_PLAYER_SCROLL_LIMITS) fo::var::setInt(FO_VAR_scroll_limiting_on) = 0;
		if (IGNORE_MAP_EDGES) fo::var::setInt(FO_VAR_scroll_blocking_on) = 0;
	};

	// Dev block tile_set_border_
	//BlockCall(0x4B11A3); // tile_init_
}

}
