/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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

#pragma comment(lib, "ddraw.lib")

#include <ddraw.h>

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\WinProc.h"

#include "..\..\HRP\Init.h"

#include "DirectDraw.h"

namespace sfall
{

static HWND window;
static DWORD mode;
static bool windowedMode;

static IDirectDraw* ddObject; // same as _GNW95_DDObject
static IDirectDrawSurface* ddPrimarySurface;
static IDirectDrawSurface* ddBackSurface;
static IDirectDrawPalette* ddPalette;
static IDirectDrawClipper* ddClipper;

static DirectDraw::PALCOLOR* paletteRGB;

//class ConvertPalette {
	DWORD PaletteToRGB24(BYTE r, BYTE g, BYTE b) {
		return (r << 16) | (g << 8) | b;
	}

	// R5G5B5
	DWORD PaletteToRGB15(BYTE r, BYTE g, BYTE b) {
		DWORD color = r / 8;
		color <<= 5;
		color |= (g / 8);
		color <<= 5;
		return color | (b / 8);
	}

	// R5G6B5
	DWORD PaletteToRGB16(BYTE r, BYTE g, BYTE b) {
		DWORD color = r / 8;
		color <<= 6;
		color |= (g / 4);
		color <<= 5;
		return color | (b / 8);
	}

	void CopyPaletteSurface16bit() {
	}
//};

//static void CopyPaletteSurfaceFunc;
//static void ConvertPaletteFunc;

void SetWindow() {
	WinProc::SetHWND(window);
	WinProc::SetSize(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight());
	WinProc::SetTitle(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight());
	if (mode == 2) WinProc::LoadPosition();

	if (windowedMode) {
		DWORD windowStyle = (mode == 3) ? WS_OVERLAPPED : (WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
		WinProc::SetStyle(windowStyle);
	}
}

static long __stdcall DirectDrawInit(DWORD, IDirectDraw* _ddObject, DWORD) {
	window = (HWND)fo::var::getInt(FO_VAR_GNW95_hwnd);
	SetWindow();

	HRESULT hr = DirectDrawCreate(0, &_ddObject, 0);
	if (FAILED(hr)) return -1;

	ddObject = _ddObject;

	hr = ddObject->SetCooperativeLevel(window, (windowedMode) ? DDSCL_NORMAL : DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_MULTITHREADED);
	if (FAILED(hr)) return -1;

	if (!windowedMode) {
		hr = ddObject->SetDisplayMode(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight(), 32);
		if (FAILED(hr)) return -1;
	}

	DDSURFACEDESC ddsd;
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = ddObject->CreateSurface(&ddsd, &ddPrimarySurface, 0);
	if (FAILED(hr)) return -1;

	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddPrimarySurface->GetSurfaceDesc(&ddsd);

	if (ddsd.ddpfPixelFormat.dwRGBBitCount != 32) {
		MessageBoxA(window, "The current video mode (not TrueColor) is not supported.", "DirectDraw", 0);
		return -1;
	}

	fo::var::setInt(0x51E2B4) = (DWORD)ddPrimarySurface; //FO_VAR_GNW95_DDPrimarySurface
	fo::var::setInt(0x51E2B8) = (DWORD)ddPrimarySurface; //FO_VAR_GNW95_DDRestoreSurface

	// Create DirectDraw clipper for windowed mode
	if (windowedMode) {
		ddObject->CreateClipper(0, &ddClipper, 0);
		ddClipper->SetHWnd(0, window);
		hr = ddPrimarySurface->SetClipper(ddClipper);
		if (FAILED(hr)) return -1;
	}

	// Create the backbuffer surface
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT; // | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN; // | DDSCAPS_SYSTEMMEMORY
	ddsd.dwWidth = HRP::Setting::ScreenWidth() + 1;
	ddsd.dwHeight = HRP::Setting::ScreenHeight() + 1;
	//ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8;
	//ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

	hr = ddObject->CreateSurface(&ddsd, &ddBackSurface, 0);
	if (FAILED(hr)) return -1;

	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddBackSurface->GetSurfaceDesc(&ddsd);

	// set greyscale (delete)
	for (size_t i = 0; i < 256; i++) {
		paletteRGB[i].xRGB = (i << 16) | (i << 8) | i;
	}

	hr = ddObject->CreatePalette(DDPCAPS_ALLOW256 | DDPCAPS_8BIT, (PALETTEENTRY*)paletteRGB, &ddPalette, 0);
	if (FAILED(hr)) return -1;

	fo::var::setInt(0x51E2BC) = (DWORD)ddPalette; // FO_VAR_GNW95_DDPrimaryPalette

	return 0;
}

void BufToBuf_32bit(BYTE* src, int width, int height, DWORD* dst, int pitch) {
	while (height--) {
		int x = width;
		while (x--) dst[x] = paletteRGB[src[x]].xRGB;
		src += width;
		dst += pitch;
	};
}

static void __cdecl GNW95_ShowRect(BYTE* srcSurface, int sWidth, int sHeight, int sX, int sY, int width, int height, int x, int y) {
	WinProc::Moving();

	if (sHeight > 0 && sWidth > 0 && fo::var::getInt(FO_VAR_GNW95_isActive)) {
		DDSURFACEDESC desc;
		std::memset(&desc, 0, sizeof(DDSURFACEDESC));
		desc.dwSize = sizeof(DDSURFACEDESC);

		while (true) {
			HRESULT hr = ddBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
			if (!hr) break;
			// 0x887601C2
			if (hr != DDERR_SURFACELOST || ddBackSurface->Restore()) return;
		}

		int pitch = desc.lPitch / 4;

		DWORD* dst = (DWORD*)desc.lpSurface;
		dst += (x + (y * pitch));

		BYTE* src = srcSurface;
		src += sX + (sY * sWidth);

		BufToBuf_32bit(src, sWidth, sHeight, dst, pitch);
		ddBackSurface->Unlock(desc.lpSurface);

		RECT backRect;
		backRect.left = x;
		backRect.top = y;
		backRect.right = x + width;
		backRect.bottom = y + height;

		POINT point;
		point.x = 0;
		point.y = 0;
		if (mode == 2) ClientToScreen(window, &point);

		RECT primRect;
		primRect.left = point.x + x;
		primRect.top = point.y + y;
		primRect.right = point.x + x + width;
		primRect.bottom = point.y + y + height;

		// Draw
		while (ddPrimarySurface->Blt(&primRect, ddBackSurface, &backRect, DDBLT_WAIT, 0) == DDERR_SURFACELOST) ddPrimarySurface->Restore();
   }
}

static void __fastcall SetPalette(fo::PALETTE* pal, long start, long count) {
	// copy and swap color B <> R
	while (count--) {
		paletteRGB[start++].xRGB = PaletteToRGB24(pal->B << 2, pal->G << 2, pal->R << 2);
		pal++;
	};

	//ddPalette->SetEntries(0, start, count, (PALETTEENTRY*)paletteRGB);
}

static __declspec(naked) void GNW95_SetPaletteEntries_hack_replacement() {
	__asm {
		push ebx;
		mov  ecx, eax;
		call SetPalette;
		test ebx, ebx;
		jz   skip;
		// update
		mov  eax, FO_VAR_scr_size;
		call fo::funcoffs::win_refresh_all_;
skip:
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GNW95_SetPalette_hack_replacement() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, eax;
		xor  edx, edx;
		push 256;
		call SetPalette;
		// update
		mov  eax, FO_VAR_scr_size;
		call fo::funcoffs::win_refresh_all_;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static IDirectDraw* GNW95_reset_mode_hack() {
	ddBackSurface->Release();
	if (ddClipper) {
		ddClipper->Release();
		ddClipper = nullptr;
	}
	ddPrimarySurface = nullptr;
	ddPalette = nullptr;
	ddBackSurface = nullptr;

	return ddObject;
}

void DirectDraw::init(long gMode) {
	mode = gMode;
	windowedMode = (gMode == 2 || gMode == 3);

	if (gMode == 2) WinProc::SetMoveKeys();

	paletteRGB = new DirectDraw::PALCOLOR[256];

	// GNW95_init_DirectDraw_
	MakeCall(0x4CAFE6, DirectDrawInit, 1); // GNW95_init_DirectDraw_
	SafeWrite8(0x4CAFEE, CodeType::Jump);
	SafeWrite32(0x4CAFEF, 433); // jmp 0x4CB1A4

	// GNW95_init_mode_ex_
	SafeWrite32(0x4CAE72, (DWORD)&GNW95_ShowRect); // replace engine GNW95_ShowRect_ with sfall function

	MakeJump(fo::funcoffs::GNW95_SetPaletteEntries_ + 1, GNW95_SetPaletteEntries_hack_replacement); // 0x4CB310
	MakeJump(fo::funcoffs::GNW95_SetPalette_, GNW95_SetPalette_hack_replacement); // 0x4CB568

	MakeCall(0x4CB1FD, GNW95_reset_mode_hack);
}

}
