/*
 *    sfall
 *    Copyright (C) 2008-2020  The sfall team
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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\Graphics.h"

#include "GameRender.h"

namespace sfall
{

class OverlaySurface
{
private:
	long size = 0;
	long surfWidth;
	long allocSize;
	BYTE* surface = nullptr;

public:
	//long winType = -1;

	BYTE* Surface() { return surface; }

	void CreateSurface(fo::Window* win, long winType) {
		//this->winType = winType;
		this->surfWidth = win->width;
		this->size = win->height * win->width;

		if (surface != nullptr) {
			if (size <= allocSize) {
				std::memset(surface, 0, size);
				return;
			}
			delete[] surface;
		}

		this->allocSize = size;
		surface = new BYTE[size]();
	}

	void ClearSurface() {
		if (surface != nullptr) std::memset(surface, 0, size);
	}

	void ClearSurface(Rectangle &rect) {
		if (surface != nullptr) {
			if (rect.width > surfWidth || rect.height > (size / surfWidth)) return; // going beyond the surface size
			BYTE* surf = surface + (surfWidth * rect.y) + rect.x;

			size_t sizeD = rect.width >> 2;
			size_t sizeB = rect.width & 3;
			size_t stride = sizeD << 2;

			long height = rect.height;
			while (height--) {
				if (sizeD) {
					__stosd((DWORD*)surf, 0, sizeD);
					surf += stride;
				}
				if (sizeB) {
					__stosb(surf, 0, sizeB);
					surf += sizeB;
				}
			};
		}
	}

	/*void DestroySurface() {
		delete[] surface;
		surface = nullptr;
	}*/

	~OverlaySurface() {
		delete[] surface;
	}
} overlaySurfaces[5];

static long indexPosition = 0;

void GameRender::CreateOverlaySurface(fo::Window* win, long winType) {
	overlaySurfaces[indexPosition].CreateSurface(win, winType);
	win->randY = reinterpret_cast<long*>(&overlaySurfaces[indexPosition]);
	if (++indexPosition == 5) indexPosition = 0;
};

BYTE* GameRender::GetOverlaySurface(fo::Window* win) {
	return reinterpret_cast<OverlaySurface*>(win->randY)->Surface();
};

void GameRender::ClearOverlay(fo::Window* win) {
	if (win->randY) reinterpret_cast<OverlaySurface*>(win->randY)->ClearSurface();
};

void GameRender::ClearOverlay(fo::Window* win, Rectangle &rect) {
	if (win->randY) reinterpret_cast<OverlaySurface*>(win->randY)->ClearSurface(rect);
};

/*void GameRender::DestroyOverlaySurface(long winType) {
	for (size_t i = 0; i < 5; i++) {
		if (overlaySurfaces[i].winType == winType) {
			overlaySurfaces[i].DestroySurface();
			overlaySurfaces[i].winType = -1;
			//break;
		}
	}
};*/

static BYTE* GetBuffer() {
	return (BYTE*)*(DWORD*)FO_VAR_screen_buffer;
}

static void Draw(fo::Window* win, BYTE* surface, long width, long height, long widthFrom, BYTE* toBuffer, long toWidth, RECT &rect, RECT* updateRect) {
	auto drawFunc = (win->flags & fo::WinFlags::Transparent && win->wID) ? fo::func::trans_buf_to_buf : fo::func::buf_to_buf;
	if (toBuffer) {
		drawFunc(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		drawFunc(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth); // copy to buffer instead of DD surface (buffering)
	}

	if (!win->randY) return;
	surface = &GameRender::GetOverlaySurface(win)[rect.left - win->rect.x] + ((rect.top - win->rect.y) * win->width);

	if (toBuffer) {
		fo::func::trans_buf_to_buf(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		fo::func::trans_buf_to_buf(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth);
	}
}

static void __fastcall sf_GNW_win_refresh(fo::Window* win, RECT* updateRect, BYTE* toBuffer) {
	if (win->flags & fo::WinFlags::Hidden) return;
	fo::RectList* rects;

	if (win->flags & fo::WinFlags::Transparent && !*(DWORD*)FO_VAR_doing_refresh_all) {
		__asm {
			mov  eax, updateRect;
			mov  edx, ds:[FO_VAR_screen_buffer];
			call fo::funcoffs::refresh_all_;
		}
		int w = (updateRect->right - updateRect->left) + 1;

		if (fo::var::mouse_is_hidden || !fo::func::mouse_in(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
			/*__asm {
				mov  eax, win;
				mov  edx, updateRect;
				call fo::funcoffs::GNW_button_refresh_;
			}*/
			int h = (updateRect->bottom - updateRect->top) + 1;

			Graphics::UpdateDDSurface(GetBuffer(), w, h, w, updateRect); // update the entire rectangle area

		} else {
			fo::func::mouse_show(); // for updating background cursor area
			RECT mouseRect;
			__asm {
				lea  eax, mouseRect;
				mov  edx, eax;
				call fo::funcoffs::mouse_get_rect_;
				mov  eax, updateRect;
				call fo::funcoffs::rect_clip_;
				mov  rects, eax;
			}
			while (rects) { // updates everything except the cursor area
				/*__asm {
					mov  eax, win;
					mov  edx, rects;
					call fo::funcoffs::GNW_button_refresh_;
				}*/

				int wRect = (rects->wRect.right - rects->wRect.left) + 1;
				int hRect = (rects->wRect.bottom - rects->wRect.top) + 1;

				Graphics::UpdateDDSurface(&GetBuffer()[rects->wRect.left - updateRect->left] + (rects->wRect.top - updateRect->top) * w, wRect, hRect, w, &rects->wRect);

				fo::RectList* free = rects;
				rects = rects->nextRect;
				fo::sf_rect_free(free);
			}
		}
		return;
	}

	/* Allocates memory for 10 RectList (if no memory was allocated), returns the first Rect and removes it from the list */
	__asm call fo::funcoffs::rect_malloc_;
	__asm mov  rects, eax;
	if (!rects) return;

	rects->rect = { updateRect->left, updateRect->top, updateRect->right, updateRect->bottom };
	rects->nextRect = nullptr;

	/*
		If the border of the updateRect rectangle is located outside the window, then assign to rects->rect the border of the window rectangle
		Otherwise, rects->rect contains the borders from the update rectangle (updateRect)
	*/
	if (rects->wRect.left   < win->wRect.left)   rects->wRect.left   = win->wRect.left;
	if (rects->wRect.top    < win->wRect.top)    rects->wRect.top    = win->wRect.top;
	if (rects->wRect.right  > win->wRect.right)  rects->wRect.right  = win->wRect.right;
	if (rects->wRect.bottom > win->wRect.bottom) rects->wRect.bottom = win->wRect.bottom;

	if (rects->wRect.right < rects->wRect.left || rects->wRect.bottom < rects->wRect.top) {
		fo::sf_rect_free(rects);
		return;
	}

	int widthFrom = win->width;
	int toWidth = (toBuffer) ? (updateRect->right - updateRect->left) + 1 : Graphics::GetGameWidthRes();

	fo::func::win_clip(win, &rects, toBuffer);

	fo::RectList* currRect = rects;
	while (currRect) {
		int width = (currRect->wRect.right - currRect->wRect.left) + 1;   // for current rectangle
		int height = (currRect->wRect.bottom - currRect->wRect.top) + 1;; // for current rectangle

		BYTE* surface;
		if (win->wID > 0) {
			__asm {
				mov  eax, win;
				mov  edx, currRect;
				call fo::funcoffs::GNW_button_refresh_;
			}
			surface = &win->surface[currRect->wRect.left - win->rect.x] + ((currRect->wRect.top - win->rect.y) * win->width);
		} else {
			surface = new BYTE[height * width](); // black background
			widthFrom = width; // replace with rectangle
		}

		Draw(win, surface, width, height, widthFrom, toBuffer, toWidth, currRect->wRect, updateRect);

		if (win->wID == 0) delete[] surface;

		currRect = currRect->nextRect;
	}

	while (rects) {
		// copy all rectangles from the buffer to the DD surface (buffering)
		if (!toBuffer) {
			int width = (rects->rect.offx - rects->rect.x) + 1;
			int height = (rects->rect.offy - rects->rect.y) + 1;
			int widthFrom = toWidth;

			Graphics::UpdateDDSurface(&GetBuffer()[rects->rect.x] + (rects->rect.y * widthFrom), width, height, widthFrom, &rects->wRect);
		}
		fo::RectList* next = rects->nextRect;
		fo::sf_rect_free(rects);
		rects = next;
	}

	if (!toBuffer && !*(DWORD*)FO_VAR_doing_refresh_all && !fo::var::mouse_is_hidden && fo::func::mouse_in(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
		fo::func::mouse_show();
	}
}

static __declspec(naked) void GNW_win_refresh_hack() {
	__asm {
		push ebx; // toBuffer
		mov  ecx, eax;
		call sf_GNW_win_refresh;
		pop  ecx;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static double fadeMulti;

static __declspec(naked) void palette_fade_to_hook() {
	__asm {
		push ebx; // _fade_steps
		fild [esp];
		fmul fadeMulti;
		fistp [esp];
		pop  ebx;
		jmp  fo::funcoffs::fadeSystemPalette_;
	}
}

void GameRender::init() {
	fadeMulti = GetConfigInt("Graphics", "FadeMultiplier", 100);
	if (fadeMulti != 100) {
		dlog("Applying fade patch.", DL_INIT);
		HookCall(0x493B16, palette_fade_to_hook);
		fadeMulti = ((double)fadeMulti) / 100.0;
		dlogr(" Done", DL_INIT);
	}

	// Replace the srcCopy_ function with a pure MMX implementation
	MakeJump(0x4D36D4, fo::func::buf_to_buf); // buf_to_buf_
	// Replace the transSrcCopy_ function
	MakeJump(0x4D3704, fo::func::trans_buf_to_buf); // trans_buf_to_buf_

	// Enable support for transparent interface windows
	SafeWriteBatch<WORD>(0x9090, {
		0x4D5D46, // win_init_ (create screen_buffer)
		0x4D75E6  // win_clip_ (remove _buffering checking)
	});
	SafeWrite8(0x42F869, fo::WinFlags::MoveOnTop | fo::WinFlags::OwnerFlag); // addWindow_ (remove Transparent flag)

	// Custom implementation of the GNW_win_refresh function
	MakeJump(0x4D6FD9, GNW_win_refresh_hack, 1);
	// Replace _screendump_buf with _screen_buffer for creating screenshots
	SafeWriteBatch<DWORD>(FO_VAR_screen_buffer, {0x4C8FD1, 0x4C900D});

	// Disable unused code for the RandX and RandY window structure fields (these fields can now be used for other purposes)
	SafeWrite32(0x4D630C, 0x9090C031); // xor eax, eax
	SafeWrite8(0x4D6310, 0x90);
	BlockCall(0x4D6319);
}

}
