/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "..\..\Modules\Graphics.h"
#include "..\..\Modules\SubModules\WindowRender.h"

#include "render.h"

namespace game
{
namespace gui
{

namespace sf = sfall;

static BYTE* GetBuffer() {
	return (BYTE*)fo::var::getInt(FO_VAR_screen_buffer);
}

static void Draw(fo::Window* win, BYTE* surface, long width, long height, long widthFrom, BYTE* toBuffer, long toWidth, RECT &rect, RECT* updateRect) {
	auto drawFunc = (win->flags & fo::WinFlags::Transparent && win->wID) ? fo::func::trans_buf_to_buf : fo::func::buf_to_buf;
	if (toBuffer) {
		drawFunc(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		drawFunc(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth); // copy to buffer instead of DD surface (buffering)
	}

	if (!win->randY) return;
	surface = &sf::WindowRender::GetOverlaySurface(win)[rect.left - win->rect.x] + ((rect.top - win->rect.y) * win->width);

	if (toBuffer) {
		fo::func::trans_buf_to_buf(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		fo::func::trans_buf_to_buf(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth);
	}
}

void __fastcall Render::GNW_win_refresh(fo::Window* win, RECT* updateRect, BYTE* toBuffer) {
	if (win->flags & fo::WinFlags::Hidden) return;
	fo::RectList* rects;

	if (win->flags & fo::WinFlags::Transparent && !fo::var::getInt(FO_VAR_doing_refresh_all)) {
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

			sf::Graphics::UpdateDDSurface(GetBuffer(), w, h, w, updateRect); // update the entire rectangle area

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

				sf::Graphics::UpdateDDSurface(&GetBuffer()[rects->wRect.left - updateRect->left] + (rects->wRect.top - updateRect->top) * w, wRect, hRect, w, &rects->wRect);

				fo::RectList* free = rects;
				rects = rects->nextRect;
				fo::util::rect_free(free);
			}
		}
		return;
	}

	/* Allocates memory for 10 RectList (if no memory was allocated), returns the first Rect and removes it from the list */
	__asm call fo::funcoffs::rect_malloc_;
	__asm mov  rects, eax;
	if (!rects) return;

	rects->rect = updateRect;
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
		fo::util::rect_free(rects);
		return;
	}

	int widthFrom = win->width;
	int toWidth = (toBuffer) ? (updateRect->right - updateRect->left) + 1 : sf::Graphics::GetGameWidthRes();

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
			surface = new BYTE[height * width](); // black background (for main menu)
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

			sf::Graphics::UpdateDDSurface(&GetBuffer()[rects->rect.x] + (rects->rect.y * widthFrom), width, height, widthFrom, &rects->wRect);
		}
		fo::RectList* next = rects->nextRect;
		fo::util::rect_free(rects);
		rects = next;
	}

	if (!toBuffer && !fo::var::getInt(FO_VAR_doing_refresh_all) && !fo::var::mouse_is_hidden && fo::func::mouse_in(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
		fo::func::mouse_show();
	}
}

static __declspec(naked) void GNW_win_refresh_hack() {
	__asm {
		push ebx; // toBuffer
		mov  ecx, eax;
		call Render::GNW_win_refresh;
		pop  ecx;
		retn;
	}
}

void Render::init() {
	// Replace the srcCopy_ function with a pure SSE implementation
	sf::MakeJump(fo::funcoffs::buf_to_buf_, fo::func::buf_to_buf); // 0x4D36D4
	// Replace the transSrcCopy_ function
	sf::MakeJump(fo::funcoffs::trans_buf_to_buf_, fo::func::trans_buf_to_buf); // 0x4D3704

	// Custom implementation of the GNW_win_refresh function
	sf::MakeJump(0x4D6FD9, GNW_win_refresh_hack, 1);
	// Replace _screendump_buf with _screen_buffer for creating screenshots (for sfall DX9 mode with HRP by Mash)
	sf::SafeWriteBatch<DWORD>(FO_VAR_screen_buffer, {0x4C8FD1, 0x4C900D});
}

}
}
