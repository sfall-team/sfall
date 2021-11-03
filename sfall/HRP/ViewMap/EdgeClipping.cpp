/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "EdgeBorder.h"
#include "ViewMap.h"

#include "EdgeClipping.h"

namespace sfall
{

// Returns -1 if the scrRect is not in the rectangle of inRect (same as rect_inside_bound_)
static __inline long rect_inside_bound(RECT* srcRect, RECT* inRect, RECT* outRect) {
	*outRect = *srcRect;  // srcRect copy to outRect

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
static long sub_1000BA60(RECT* rect) {
	const long gridWidth = 200;
	const long gridLength = 200;

	long cX, cY;
	ViewMap::GetTileCoordOffset(fo::var::getInt(FO_VAR_tile_center_tile), cX, cY);

	long width = fo::var::getInt(FO_VAR_buf_width_2) >> 1;
	long height = fo::var::getInt(FO_VAR_buf_length_2) >> 1;

	long v6 = (cX + width) - rect->left;
	long v7 = (cY + rect->top) - height;

	long v8 = (cX + width) - rect->right;
	long v9 = (cY + rect->bottom) - height;

	long x = v6;
	long y = v7;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	x = v8;
	y = v7;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	x = v6;
	y = v9;
	ViewMap::GetCoordFromOffset(x, y);

	if (x < 0 || x >= gridLength || y < 0 || y >= gridWidth) return 1;

	x = v8;
	y = v9;
	ViewMap::GetCoordFromOffset(x, y);

	if (x >= 0 && x < gridLength && y >= 0 && y < gridWidth) return 0;

	return 1;
}

// Implementation from HRP by Mash
static long __fastcall rect_inside_bound_clip(RECT* srcRect, RECT* inRect, RECT* outRect) {
	if (rect_inside_bound(srcRect, inRect, outRect)) return -1;

	if (ViewMap::EDGE_CLIPPING_ON) {
		long x, y;
		ViewMap::GetTileCoordOffset(fo::var::getInt(FO_VAR_tile_center_tile), x, y);

		EdgeBorder::Edge* cEdge = EdgeBorder::CurrentMapEdge();

		RECT visibleRect;
		visibleRect.left = x + ViewMap::mapModWidth - cEdge->rect_2.left;
		visibleRect.right = x + ViewMap::mapModWidth - cEdge->rect_2.right;

		visibleRect.top = ViewMap::mapModHeight + cEdge->rect_2.top - y;
		visibleRect.bottom = ViewMap::mapModHeight + cEdge->rect_2.bottom - y;

		if (sub_1000BA60(outRect)) {
			long height = (outRect->bottom - outRect->top) + 1;
			if (height > 0) {
				long mapWinWidth = fo::var::getInt(FO_VAR_buf_width_2);
				//long mapWinHeight = fo::var::getInt(FO_VAR_buf_length_2);
				BYTE* dst = (BYTE*)fo::var::getInt(FO_VAR_display_buf) + (outRect->top * mapWinWidth) + outRect->left;

				long width = outRect->right - outRect->left + 1;
				long h = height;
				do {
					std::memset(dst, 0, width);
					dst += mapWinWidth;
				} while (--h);
			}
		}
		return rect_inside_bound(outRect, &visibleRect, outRect);
	}
	return 0;
}

static void __declspec(naked) refresh_game_hook_rect_inside_bound() {
	__asm {
		push ecx;
		push ebx; // outRect
		mov  ecx, eax;
		call rect_inside_bound_clip;
		pop  ecx;
		retn;
	}
}

static long __fastcall rect_inside_bound_scroll_clip(RECT* srcRect, RECT* inRect, RECT* outRect) {
	long height = (srcRect->bottom - srcRect->top) + 1;
	if (height > 0) {
		long mapWinWidth = fo::var::getInt(FO_VAR_buf_width_2);
		BYTE* dst = (BYTE*)fo::var::getInt(FO_VAR_display_buf) + (srcRect->top * mapWinWidth) + srcRect->left;

		long width = (srcRect->right - srcRect->left) + 1;
		long h = height;
		do {
			std::memset(dst, 0, width);
			dst += mapWinWidth;
		} while (--h);
	}
	return rect_inside_bound_clip(srcRect, inRect, outRect);
}

static void __declspec(naked) map_scroll_refresh_game_hook_rect_inside_bound() {
	__asm {
		push ecx;
		push ebx; // outRect
		mov  ecx, eax;
		call rect_inside_bound_scroll_clip;
		pop  ecx;
		retn;
	}
}

void EdgeClipping::init() {
	HookCall(0x4B15F6, refresh_game_hook_rect_inside_bound);
	HookCall(0x483EF0, map_scroll_refresh_game_hook_rect_inside_bound);
}

}
