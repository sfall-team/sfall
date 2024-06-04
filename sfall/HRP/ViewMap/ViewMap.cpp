/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\Modules\LoadGameHook.h"
#include "..\..\Modules\MainLoopHook.h"

#include "..\Init.h"
#include "EdgeBorder.h"
#include "EdgeClipping.h"

#include "ViewMap.h"

namespace HRP
{

namespace sf = sfall;

const long grid_width = 200; // _grid_width
const long square_width = 100;
const long square_length = 100;

static sf::Rectangle obj_on_screen_rect;

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
	int x = tile % grid_width;                                      // 20100%200 = 100
	int y = (tile / grid_width) + (x / 2);                          // 20100/200 = 100+(100/2) = y:150 ???
	outY = y;
	outX = (2 * x) - y;                                             // 2*100 = 200-150 = x:50 ???
}

void ViewMap::GetTileCoordOffset(long tile, long &outX, long &outY) {
	int x = tile % grid_width;                                      // 20100%200 = 100
	int y = (tile / grid_width) + (x / 2);                          // 20100/200 = 100+(100/2) = y:150 ???
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

	if (modeFlags) tile = EdgeBorder::GetCenterTile(tile, fo::var::map_elevation); // get the center tile in what cases?

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

	long tile_y = tile / grid_width;
	long tile_x = 200 - (tile % grid_width) - 1;

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
		return -1; // necessary for the correct rendering of the map
	}
	return 0;
}

static __declspec(naked) void tile_set_center_hack_replacement() {
	__asm {
		push ecx;
		mov  ecx, eax;
		call tile_set_center;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void tile_scroll_to_hack_replacement() {
	__asm {
		mov  edx, 3; // modeFlags
		jmp  tile_set_center_hack_replacement;
	}
}

// Implementation from HRP 3.0.6 by Mash
static long __fastcall square_rect_render_floor(long y, long id, long x) {
	if (x < 0 || x >= square_width || y < 0 || y >= square_length) return -1;

	auto edge = EdgeBorder::CurrentMapEdge();
	if (EdgeBorder::EdgeVersion() && (x > edge->squareRect.left || x < edge->squareRect.right || y > edge->squareRect.bottom || y < edge->squareRect.top)) {
		return 1;
	}
	return id;
}

static __declspec(naked) void square_render_floor_hook_art_id() {
	__asm {
		mov  ecx, [esp + 0x24 + 4];
		push esi;
		call square_rect_render_floor;
		test eax, eax;
		js   skipDraw; // -1
		mov  edx, eax;
		xor  ecx, ecx;
		mov  eax, 4;
		jmp  fo::funcoffs::art_id_;

skipDraw:
		mov  dword ptr [esp], 0x4B2AD4;
		retn 4;
	}
}

// Implementation from HRP 3.0.6 by Mash
static long __fastcall square_rect_render_roof(long y, long id, long x) {
	if (x < 0 || x >= square_width || y < 0 || y >= square_length) return -1;

	auto edge = EdgeBorder::CurrentMapEdge();
	if (EdgeBorder::EdgeVersion() && (x > (edge->squareRect.left + 2) || x < (edge->squareRect.right + 2) ||
	    y > (edge->squareRect.bottom + 3) || y < (edge->squareRect.top + 3)))
	{
		return -1;
	}
	return id;
}

static __declspec(naked) void square_render_roof_hook_art_id() {
	using namespace fo;
	__asm {
		mov  ecx, [esp + 0x20 + 4];
		push edi;
		call square_rect_render_roof;
		test eax, eax;
		js   skipDraw; // -1
		mov  edx, eax;
		xor  ecx, ecx;
		mov  eax, OBJ_TYPE_TILE;
		jmp  fo::funcoffs::art_id_;

skipDraw:
		mov  eax, esi;
		retn 4;
	}
}

static __declspec(naked) void square_roof_intersect_hook_art_id() {
	using namespace fo;
	__asm {
		mov  ecx, [esp + 0x8 + 4];
		push [esp + 0x4 + 4];
		call square_rect_render_roof;
		test eax, eax;
		js   skipDraw; // -1
		mov  edx, eax;
		xor  ecx, ecx;
		mov  eax, OBJ_TYPE_TILE;
		jmp  fo::funcoffs::art_id_;

skipDraw:
		xor  eax, eax;
		mov  dword ptr [esp], 0x4B2C05;
		retn 4;
	}
}

// Implementation from HRP 3.0.6 by Mash
static void __fastcall square_obj_render(fo::BoundRect* rect, long tag) {
	if (EdgeBorder::EdgeVersion() == 0) return;

	long x0, y0, x1, y1, x2, y2, x3, y3;

	fo::func::square_xy(rect->x, rect->offy,    &x0, &y0);
	fo::func::square_xy(rect->x, rect->y,       &x1, &y1);
	fo::func::square_xy(rect->offx, rect->y,    &x2, &y2);
	fo::func::square_xy(rect->offx, rect->offy, &x3, &y3);

	if (++x0 > square_width) x0 = square_width - 1;
	if (--x2 < 0) x2 = 0;
	if (--y1 < 0) y1 = 0;
	if (++y3 > square_length) y3 = square_length - 1;

	if (y1 >= y3 || x2 >= x0) return; // top >= bottom / left >= right

	long Y = y1;
	long sY = square_width * Y;

	auto edge = EdgeBorder::CurrentMapEdge();
	do {
		long X = x2;
		do {
			if (X > edge->squareRect.left   && ((edge->clipData >> 24) & 1) == tag ||
			    Y < edge->squareRect.top    && ((edge->clipData >> 16) & 1) == tag ||
			    X < edge->squareRect.right  && ((edge->clipData >>  8) & 1) == tag ||
			    Y > edge->squareRect.bottom && (edge->clipData & 1) == tag)
			{
				long s_x, s_y;
				fo::func::square_coord(X + sY, &s_x, &s_y);
				__asm {
					mov  ecx, rect;
					mov  ebx, s_y;
					mov  edx, s_x;
					mov  eax, 0x4000001; // tile fid
					call fo::funcoffs::floor_draw_;
				}
			}
		} while (++X < x0); // x < right

		sY += square_width;
	} while (++Y < y3); // y < bottom
}

static __declspec(naked) void obj_render_pre_roof_hack_0() {
	__asm {
		mov  edx, 0;
		lea  ecx, [esp + 4];
		call square_obj_render;
		mov  edi, [esp + 0x2C + 4];
		test edi, edi;
		retn;
	}
}

static __declspec(naked) void obj_render_pre_roof_hack_1() {
	__asm {
		mov  edx, 1;
		lea  ecx, [esp + 4];
		call square_obj_render;
		pop  eax; // ret addr
		add  esp, 0x44;
		pop  ebp;
		pop  edi;
		jmp  eax;
	}
}

// An odd solution to the problem from HRP 3.06 (4.1.8 uses a different solution)
static __declspec(naked) void obj_render_pre_roof_hack() {
	__asm {
		mov  ebx, [eax];
		lea  eax, [edx + ebx]; // edx - tilenum
		cmp  eax, 40000;       // _grid_size
		jge  tileMax;
		test eax, eax;
		js   tileMin;
		retn;
tileMax:
		mov  eax, 39999;
		retn;
tileMin:
		xor  eax, eax;
		retn;
	}
}

static __declspec(naked) void obj_create_intersect_list_hack() {
	__asm {
		mov  ecx, [ebx + eax];
		lea  eax, [edx + ecx]; // edx - tilenum
		cmp  eax, 40000;       // _grid_size
		jge  tileMax;
		test eax, eax;
		js   tileMin;
		retn;
tileMax:
		mov  eax, 39999;
		retn;
tileMin:
		xor  eax, eax;
		retn;
	}
}

// Implementation of tile_num_ engine function without checking the grid boundary (from HRP 3.0.6 by Mash)
static long __fastcall tile_num_HRP(long x, long y) {
	long X, Y;

	long yTileOff = y - fo::var::getInt(FO_VAR_tile_offy);
	if (yTileOff >= 0) {
		Y = yTileOff / 12;
	} else {
		Y = (yTileOff + 1) / 12 - 1;
	}

	long xTileOff = x - fo::var::getInt(FO_VAR_tile_offx) - 16 * Y;
	if (xTileOff >= 0) {
		X = xTileOff / 64;
	} else {
		//__asm {
		//	mov eax, xTileOff;
		//	inc eax;
		//	mov edx, eax;
		//	sar edx, 31; // always -1
		//	shl edx, 6;  // always -64
		//	sbb eax, edx;
		//	sar eax, 6;
		//	dec eax;
		//	mov X, eax;
		//}
		X = ((xTileOff + 1) / 64) - 1;
	}

	long xOffset = xTileOff - (X * 64);
	long yOffset = yTileOff - 12 * Y;
	long tY = X + Y;
	long tX = X * 2;

	if (xOffset >= 32) {
		xOffset -= 32;
		tX++;
	}

	long xTile = fo::var::getInt(FO_VAR_tile_x) + tX;
	long yTile = fo::var::getInt(FO_VAR_tile_y) + tY;

	switch (fo::var::tile_mask[(32 * yOffset) + xOffset]) {
	case 1:
		yTile--;
		break;
	case 2:
		if (++xTile & 1) yTile--;
		break;
	case 3:
		if (!(--xTile & 1)) yTile++;
		break;
	case 4:
		yTile++;
		break;
	}
	return (grid_width * yTile) + (grid_width - 1 - xTile);
}

static __declspec(naked) void obj_render_pre_roof_hack_tile_num() {
	__asm {
		push ecx;
		call tile_num_HRP;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void obj_create_intersect_list_hack_tile_num() {
	__asm {
		mov  ecx, eax;
		jmp  tile_num_HRP;
	}
}

void ViewMap::init() {
	if (SCROLL_DIST_X <= 0) SCROLL_DIST_X = 480;
	else if (SCROLL_DIST_X < 320) SCROLL_DIST_X = 320;

	if (SCROLL_DIST_Y <= 0) SCROLL_DIST_Y = 400;
	else if (SCROLL_DIST_Y < 240) SCROLL_DIST_Y = 240;

	sf::MakeJump(fo::funcoffs::tile_set_center_, tile_set_center_hack_replacement); // 0x4B12F8
	sf::MakeJump(fo::funcoffs::tile_scroll_to_, tile_scroll_to_hack_replacement);   // 0x4B3924

	sf::HookCall(0x4B2AC0, square_render_floor_hook_art_id);
	sf::HookCall(0x4B2261, square_render_roof_hook_art_id);
	sf::HookCall(0x4B2BF7, square_roof_intersect_hook_art_id); // need to test object_under_mouse_
	sf::MakeCall(0x489730, obj_render_pre_roof_hack_0, 1);
	sf::MakeCall(0x4897E3, obj_render_pre_roof_hack_1);

	// Fixes from 3.06 (fix crashes when going beyond the limits of the vanilla grid size of the map)
	sf::MakeCall(0x489665, obj_render_pre_roof_hack);
	sf::HookCall(0x4895B7, obj_render_pre_roof_hack_tile_num); // replace the tile_num_ function
	sf::MakeCall(0x48C66B, obj_create_intersect_list_hack, 1);
	sf::HookCall(0x48C5F3, obj_create_intersect_list_hack_tile_num); // replace the tile_num_ function

	obj_on_screen_rect.width = Setting::ScreenWidth();
	obj_on_screen_rect.height = Setting::ScreenHeight(); // (maybe it should take into account the interface bar)
	sf::SafeWrite32(0x45C886, (DWORD)&obj_on_screen_rect); // replace rectangle in op_obj_on_screen_

	EdgeBorder::init();
	EdgeClipping::init();

	sf::LoadGameHook::OnAfterGameInit() += []() {
		if (IGNORE_PLAYER_SCROLL_LIMITS) fo::var::setInt(FO_VAR_scroll_limiting_on) = 0;
		if (IGNORE_MAP_EDGES) fo::var::setInt(FO_VAR_scroll_blocking_on) = 0;
	};

	// Dev block tile_set_border_
	//BlockCall(0x4B11A3); // tile_init_
}

static __declspec(naked) void obj_move_to_tile_hook_redraw() {
	__asm {
		mov  sf::MainLoopHook::displayWinUpdateState, 1;
		call fo::funcoffs::tile_set_center_;
		mov  eax, ds:[FO_VAR_display_win];
		jmp  fo::funcoffs::win_draw_; // update black edges after tile_set_center_
	}
}

static __declspec(naked) void map_check_state_hook_redraw() {
	__asm {
		cmp  sf::MainLoopHook::displayWinUpdateState, 0;
		je   obj_move_to_tile_hook_redraw;
		jmp  fo::funcoffs::tile_set_center_;
	}
}

void ViewMap::RedrawFix() {
	// Redraw the screen to update black edges of the map (HRP bug)
	// https://github.com/phobos2077/sfall/issues/282
	sf::HookCall(0x48A954, obj_move_to_tile_hook_redraw);
	sf::HookCall(0x483726, map_check_state_hook_redraw);
}

}
