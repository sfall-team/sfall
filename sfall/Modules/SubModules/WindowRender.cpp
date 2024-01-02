/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include <intrin.h>

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "..\..\Game\GUI\render.h"

#include "WindowRender.h"

namespace sfall
{

class OverlaySurface {
private:
	long size = 0;
	long surfWidth;
	long allocSize;
	BYTE* surface = nullptr;

public:
	long winType = -1;

	BYTE* Surface() { return surface; }

	void CreateSurface(fo::Window* win, long winType) {
		this->winType = winType;
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
			size_t strideD = sizeD << 2;
			size_t stride = surfWidth - rect.width;

			long height = rect.height;
			while (height--) {
				if (sizeD) {
					__stosd((DWORD*)surf, 0, sizeD);
					surf += strideD;
				}
				if (sizeB) {
					__stosb(surf, 0, sizeB);
					surf += sizeB;
				}
				surf += stride;
			};
		}
	}

	void DestroySurface() {
		delete[] surface;
		surface = nullptr;
	}

	~OverlaySurface() {
		delete[] surface;
	}
} overlaySurfaces[5];

static long indexPosition = 0;

void WindowRender::CreateOverlaySurface(fo::Window* win, long winType) {
	if (win->randY) return;
	if (overlaySurfaces[indexPosition].winType == winType) {
		overlaySurfaces[indexPosition].ClearSurface();
	} else {
		if (++indexPosition == 5) indexPosition = 0;
		overlaySurfaces[indexPosition].CreateSurface(win, winType);
	}
	win->randY = reinterpret_cast<long*>(&overlaySurfaces[indexPosition]);
}

BYTE* WindowRender::GetOverlaySurface(fo::Window* win) {
	return reinterpret_cast<OverlaySurface*>(win->randY)->Surface();
}

void WindowRender::ClearOverlay(fo::Window* win) {
	if (win->randY) reinterpret_cast<OverlaySurface*>(win->randY)->ClearSurface();
}

void WindowRender::ClearOverlay(fo::Window* win, Rectangle &rect) {
	if (win->randY) {
		reinterpret_cast<OverlaySurface*>(win->randY)->ClearSurface(rect);
		fo::BoundRect updateRect = rect;
		updateRect.x += win->rect.x;
		updateRect.y += win->rect.y;
		updateRect.offx += win->rect.x;
		updateRect.offy += win->rect.y;
		game::gui::Render::GNW_win_refresh(win, reinterpret_cast<RECT*>(&updateRect), 0);
	}
}

void WindowRender::DestroyOverlaySurface(fo::Window* win) {
	if (win->randY) {
		auto overlay = reinterpret_cast<OverlaySurface*>(win->randY);
		win->randY = nullptr;
		overlay->winType = -1;
		overlay->DestroySurface();
		game::gui::Render::GNW_win_refresh(win, &win->wRect, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////

static bool reCalculate = false;
static float fadeMulti = 1.0f;

static __declspec(naked) void palette_fade_to_hook() {
	__asm {
		cmp  reCalculate, 0;
		je   skipCalc;
		push eax;
		mov  eax, 0x493A4C; // palette_init_
		push return;        // ret addr
		push ebx;
		push ecx;
		push edx;
		push esi;
		push edi;
		sub  esp, 8;
		jmp  eax; // recalc
return:
		pop  eax;
		mov  ebx, ds:[FO_VAR_fade_steps];
skipCalc:
		cmp  fadeMulti, 0x3F800000; // 1.0f
		jne  mult;
		jmp  fo::funcoffs::fadeSystemPalette_;
mult:
		push ebx; // _fade_steps
		fild [esp];
		fmul fadeMulti;
		fistp [esp];
		pop  ebx;
		jmp  fo::funcoffs::fadeSystemPalette_;
	}
}

void WindowRender::EnableRecalculateFadeSteps() {
	reCalculate = true;
}

void WindowRender::init() {
	int multi = IniReader::GetConfigInt("Graphics", "FadeMultiplier", 100);
	if (multi != 100 || reCalculate) {
		dlogr("Applying fade patch.", DL_INIT);
		HookCall(0x493B16, palette_fade_to_hook);
		if (multi <= 0) multi = 1;
		fadeMulti = multi / 100.0f;
	}

	// Enable support for transparent interface windows
	SafeWriteBatch<WORD>(0x9090, {
		0x4D5D46, // win_init_ (create screen_buffer)
		0x4D75E6  // win_clip_ (remove _buffering checking)
	});
}

}
