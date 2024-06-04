/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "..\InterfaceBar.h"
#include "EdgeBorder.h"
#include "ViewMap.h"

#include "EdgeClipping.h"

namespace HRP
{

namespace sf = sfall;

static RECT mapVisibleArea;

// Returns -1 if the scrRect is not in the rectangle of inRect (same as rect_inside_bound_)
static __inline long rect_inside_bound(RECT* srcRect, RECT* inRect, RECT* outRect) {
	if (srcRect != outRect) *outRect = *srcRect; // srcRect copy to outRect

	if (inRect->right  < srcRect->left  ||
	    inRect->left   > srcRect->right ||
	    inRect->bottom < srcRect->top   ||
	    inRect->top    > srcRect->bottom)
	{
		return -1; // srcRect is not within the boundaries of the rectangle of inRect
	}

	// sets the size so that the boundaries of outRect(srcRect) do not go beyond the boundaries of inRect?
	if (inRect->left   > srcRect->left)   outRect->left   = inRect->left;
	if (inRect->right  < srcRect->right)  outRect->right  = inRect->right;
	if (inRect->top    > srcRect->top)    outRect->top    = inRect->top;
	if (inRect->bottom < srcRect->bottom) outRect->bottom = inRect->bottom;

	return 0;
}

// Implementation from HRP by Mash
static long CheckRect(RECT* rect) {
	const long gridWidth = 200;
	const long gridLength = 200;

	long cX, cY;
	ViewMap::GetTileCoordOffset(fo::var::getInt(FO_VAR_tile_center_tile), cX, cY);

	long width = fo::var::getInt(FO_VAR_buf_width_2) >> 1;
	long height = fo::var::getInt(FO_VAR_buf_length_2) >> 1;

	long xLeft = (cX + width) - rect->left;
	long yTop = (cY + rect->top) - height;

	long xRight = (cX + width) - rect->right;
	long yBottom = (cY + rect->bottom) - height;

	long x = xLeft;
	long y = yTop;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	x = xRight;
	y = yTop;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	x = xLeft;
	y = yBottom;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	x = xRight;
	y = yBottom;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	return 0;
}

static void ClearRect(long width, long height, RECT* outRect) {
	long mapWinWidth = fo::var::getInt(FO_VAR_buf_width_2);
	BYTE* dst = (BYTE*)fo::var::getInt(FO_VAR_display_buf) + (outRect->top * mapWinWidth) + outRect->left;
	do {
		std::memset(dst, 0, width);
		dst += mapWinWidth;
	} while (--height);
}

// Implementation from HRP by Mash
static long __fastcall rect_inside_bound_clip(RECT* srcRect, RECT* inRect, RECT* outRect) {
	if (rect_inside_bound(srcRect, inRect, outRect)) return -1;

		long x, y;
		ViewMap::GetTileCoordOffset(fo::var::getInt(FO_VAR_tile_center_tile), x, y);

		x += ViewMap::mapModWidth;
		y -= ViewMap::mapModHeight;

		EdgeBorder::Edge* cEdge = EdgeBorder::CurrentMapEdge();

		mapVisibleArea.left = x - cEdge->rect_2.left;
		mapVisibleArea.right = x - cEdge->rect_2.right;

		mapVisibleArea.top = cEdge->rect_2.top - y;
		mapVisibleArea.bottom = cEdge->rect_2.bottom - y;

	if (ViewMap::EDGE_CLIPPING_ON) {
		if (CheckRect(outRect)) { // when do you need the fill below?
			long height = (outRect->bottom - outRect->top) + 1;
			if (height > 0) {
				long width = (outRect->right - outRect->left) + 1;
				if (width > 0) {
					ClearRect(width, height, outRect);
				}
			}
		}
		return rect_inside_bound(outRect, &mapVisibleArea, outRect);
	}
	return 0;
}

static __declspec(naked) void refresh_game_hook_rect_inside_bound() {
	__asm {
		push ecx;
		push ebx; // outRect
		mov  ecx, eax;
		call rect_inside_bound_clip;
		pop  ecx;
		retn;
	}
}

// Implementation from HRP by Mash
static long __fastcall rect_inside_bound_scroll_clip(RECT* srcRect, RECT* inRect, RECT* outRect) {
	long height = (srcRect->bottom - srcRect->top) + 1;
	if (height > 0) {
		long width = (srcRect->right - srcRect->left) + 1;
		if (width > 0) {
			ClearRect(width, height, srcRect);
		}
	}
	return rect_inside_bound_clip(srcRect, inRect, outRect);
}

static __declspec(naked) void map_scroll_refresh_game_hook_rect_inside_bound() {
	__asm {
		push ecx;
		push ebx; // outRect
		mov  ecx, eax;
		call rect_inside_bound_scroll_clip;
		pop  ecx;
		retn;
	}
}

static long __fastcall MouseCheckArea(long x, long y) {
	if (!ViewMap::EDGE_CLIPPING_ON) return 0;
	if (x < mapVisibleArea.left || x > mapVisibleArea.right || y < mapVisibleArea.top || y >= mapVisibleArea.bottom) {
		if (fo::func::win_get_top_win(x, y) == fo::var::getInt(FO_VAR_display_win)) {
			return 40000;
		}
	}
	return 0;
}

static __declspec(naked) void gmouse_check_scrolling_hack() {
	__asm {
		jl   skip;
		or   cl, 8;
		retn;
skip:
		test ecx, ecx;
		jz   check
		retn;
check:
		mov  edx, esi; // y
		mov  ecx, ebp; // x
		call MouseCheckArea;
		xor  edx, edx;
		xor  ecx, ecx;
		test eax, eax;
		jnz  set;
		retn;
set:
		mov  ebx, -7;
		//mov  ecx, 3;
		retn;
	}
}

// Implementation from HRP by Mash
static long __fastcall post_roof_rect_inside_bound(RECT* srcRect, RECT* inRect, RECT* outRect) {
	long result = rect_inside_bound(srcRect, inRect, outRect);
	if (result == -1 || !ViewMap::EDGE_CLIPPING_ON) return result;

	RECT visible;
	visible.left   = mapVisibleArea.left + 1;
	visible.top    = mapVisibleArea.top  + 1;
	visible.right  = mapVisibleArea.right  - 1;
	visible.bottom = mapVisibleArea.bottom - 1;

	return rect_inside_bound(outRect, &visible, outRect);
}

static __declspec(naked) void obj_render_post_roof_hook_rect_inside_bound() {
	__asm {
		push ebx; // outRect
		mov  ecx, eax;
		call post_roof_rect_inside_bound;
		retn;
	}
}

static long rightLimit = -1;

static long __fastcall gmouse_scr_offy_map_limit() {
	rightLimit = -1;

	long bottom = fo::var::scr_size.offy;
	if (bottom > mapVisibleArea.bottom) {
		return mapVisibleArea.bottom + 1 + 99; // +99 takes into account the subtraction in the engine
	}

	if (IFaceBar::IFACE_BAR_MODE) {
		long x, y;
		fo::func::mouse_get_position(&x, &y);
		fo::Window* win = fo::func::GNW_find(fo::var::interfaceWindow);

		if ((x + 25) > win->rect.offx || (x + 68) < win->rect.x) {
			bottom += 100; // does not enter the interface area
		} else if ((x + 25) < win->rect.x) {
			bottom += 100;
			rightLimit = win->rect.x;
		}
	}
	return bottom;
}

static __declspec(naked) void gmouse_scr_offx_map_limit() {
	__asm {
		mov  eax, ds:[FO_VAR_scr_size + 8]; // scr_size.offx
		cmp  eax, mapVisibleArea.right;
		jle  skip;
		mov  eax, mapVisibleArea.right;
		//inc  eax;
		retn;
skip:
		cmp  rightLimit, eax;
		cmovb eax, rightLimit;
		retn;
	}
}

long EdgeClipping::CheckMapClipping() {
	if (!ViewMap::EDGE_CLIPPING_ON) return 1;
	return fo::func::mouse_click_in(mapVisibleArea.left, mapVisibleArea.top, mapVisibleArea.right, mapVisibleArea.bottom);
}

void EdgeClipping::init() {
	sf::HookCall(0x4B15F6, refresh_game_hook_rect_inside_bound);
	sf::HookCall(0x4B1565, refresh_game_hook_rect_inside_bound); // refresh_mapper_

	sf::HookCall(0x483EF0, map_scroll_refresh_game_hook_rect_inside_bound);
	sf::HookCall(0x483F53, map_scroll_refresh_game_hook_rect_inside_bound); // map_scroll_refresh_mapper_

	sf::MakeCall(0x44E481, gmouse_check_scrolling_hack); // from HRP 3.06 (TODO: redo the implementation so that the scrolling of the map works)

	// Prevents the rendering of "post roof" objects (fixes the red pixels from the hex cursor remaining at the edges of the cropped part of the map)
	sf::HookCall(0x489802, obj_render_post_roof_hook_rect_inside_bound);

	// Limits the creation of the list of action icons beyond the visible map area or the bottom panel of the interface bar
	// (this should be removed after changing the implementation of the list of action icons)
	// gmouse_handle_event_
	sf::MakeCall(0x44C597, gmouse_scr_offy_map_limit);
	sf::MakeCall(0x44C5AC, gmouse_scr_offx_map_limit);
	// the same thing, only when hovering the cursor over objects
	// gmouse_bk_process_
	sf::MakeCall(0x44BA40, gmouse_scr_offy_map_limit);
	sf::MakeCall(0x44BA55, gmouse_scr_offx_map_limit);
}

}
