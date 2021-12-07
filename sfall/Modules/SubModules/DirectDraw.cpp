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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\WinProc.h"
#include "..\Graphics.h"

#include "..\..\HRP\Init.h"

#include "DirectDraw.h"

namespace sfall
{

class DirectDrawChild;

class IConvertPalette {
public:
	virtual DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) = 0;
	virtual void BufToSurface(BYTE* src, int sPitch, int x, int y, int width, int height, DDSURFACEDESC2* desc) = 0;
};

DirectDrawChild* draw = nullptr;
static bool paletteInitialized = false;

class DirectDrawChild {
private:
	class ConvertPalette32 : public IConvertPalette {
	public:
		virtual DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) {
			return (r << 16) | (g << 8) | b;
		}

		virtual void BufToSurface(BYTE* src, int sPitch, int x, int y, int width, int height, DDSURFACEDESC2* desc) {
			int dPitch = desc->lPitch >> 2;
			DWORD* dst = (DWORD*)desc->lpSurface;
			dst += (x + (y * dPitch));

			sPitch -= width;
			dPitch -= width;
			while (height--) {
				int x = width;
				while (x--) *dst++ = draw->paletteRGB[*src++].xRGB;
				src += sPitch;
				dst += dPitch;
			};
		}
	};

	class ConvertPalette16 : public IConvertPalette {
	public:
		// R5G6B5
		DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) {
			DWORD color = r / 8;
			color <<= 6;
			color |= (g / 4);
			color <<= 5;
			return color | (b / 8);
		}

		virtual void BufToSurface(BYTE* src, int sPitch, int x, int y, int width, int height, DDSURFACEDESC2* desc) {
		}
	};

	class ConvertPalette15 : public ConvertPalette16 {
		// R5G5B5
		DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) {
			DWORD color = r / 8;
			color <<= 5;
			color |= (g / 8);
			color <<= 5;
			return color | (b / 8);
		}
	};

	BYTE* buffSurface;

	~DirectDrawChild() {
		delete[] paletteRGB;
		delete[] buffSurface;
		delete[] convertFunc;
	}

public:
	IDirectDraw7* ddObject; // same as _GNW95_DDObject
	IDirectDrawSurface7* ddPrimarySurface;
	IDirectDrawSurface7* ddBackSurface;
	//IDirectDrawSurface7* ddBuffSurface;
	IDirectDrawPalette* ddPalette;
	IDirectDrawClipper* ddClipper;

	DirectDraw::PALCOLOR* paletteRGB;
	IConvertPalette* convertFunc;

	DirectDrawChild(long dwWidth, long dwHeight) {
		buffSurface = new BYTE[dwWidth * dwHeight];
		paletteRGB = new DirectDraw::PALCOLOR[256]();
	}

	BYTE* BufferSurface() {
		return buffSurface;
	}

	void SetDrawTrueColor() {
		convertFunc = new ConvertPalette32();
	}

	void SetDrawHightColor(long bits) {
		if (bits == 0x7E0) { // 0000011111100000
			convertFunc = new ConvertPalette16();
		} else {
			convertFunc = new ConvertPalette15();
		}
	}

	// Copy and swap color B <> R
	void UpdatePalette(fo::PALETTE* pal, long start, long count) {
		while (count--) {
			paletteRGB[start++].xRGB = convertFunc->PaletteToRGB(pal->B << 2, pal->G << 2, pal->R << 2);
			pal++;
		};
		//ddPalette->SetEntries(0, start, count, (PALETTEENTRY*)paletteRGB);
	}

	void UpdateSurface() {
		DDSURFACEDESC2 desc;
		//std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
		desc.dwSize = sizeof(DDSURFACEDESC2);

		while (true) {
			HRESULT hr = ddBackSurface->Lock(0, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
			if (!hr) break;
			if (hr != DDERR_SURFACELOST || ddBackSurface->Restore()) return;
		}

		convertFunc->BufToSurface(buffSurface, desc.dwWidth, 0, 0, desc.dwWidth, desc.dwHeight, &desc);
		ddBackSurface->Unlock(0);

		const POINT* win = WinProc::GetClientPos();
		RECT primRect;
		primRect.left = win->x;
		primRect.top = win->y;
		primRect.right = win->x + desc.dwWidth;
		primRect.bottom = win->y + desc.dwHeight;

		// Draw
		while (ddPrimarySurface->Blt(&primRect, ddBackSurface, 0, DDBLT_WAIT, 0) == DDERR_SURFACELOST) ddPrimarySurface->Restore();
	}
};


void SetWindow(HWND window) {
	WinProc::SetHWND(window);
	WinProc::SetSize(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight());
	WinProc::SetTitle(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight());
	if (Graphics::mode == 2) WinProc::LoadPosition();

	if (Graphics::IsWindowedMode) {
		DWORD windowStyle = (Graphics::mode == 3) ? WS_OVERLAPPED : (WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
		WinProc::SetStyle(windowStyle);
	}
}

static long __stdcall DirectDrawInit(DWORD, IDirectDraw7* _ddObject, DWORD) {
	HWND window = (HWND)fo::var::getInt(FO_VAR_GNW95_hwnd);
	SetWindow(window);

	draw = new DirectDrawChild(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight());

	HRESULT hr = DirectDrawCreateEx(NULL, (LPVOID*)&draw->ddObject, IID_IDirectDraw7, NULL);
	if (FAILED(hr)) return -1;

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	draw->ddObject->GetDisplayMode(&ddsd);

	if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8) {
		MessageBoxA(window, "The current 8-bit video mode is not supported.", "sfall: DirectDraw", 0);
		return -1;
	}
	_ddObject = draw->ddObject;

	if (ddsd.ddpfPixelFormat.dwRGBBitCount != 32) {
		draw->SetDrawHightColor(ddsd.ddpfPixelFormat.dwGBitMask);
	} else {
		draw->SetDrawTrueColor();
	}

	hr = draw->ddObject->SetCooperativeLevel(window, (Graphics::IsWindowedMode) ? DDSCL_NORMAL : (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_MULTITHREADED));
	if (FAILED(hr)) return -1;

	if (!Graphics::IsWindowedMode) {
		hr = draw->ddObject->SetDisplayMode(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight(), 32, 0, 0);
		if (FAILED(hr)) return -1;
	}

	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = draw->ddObject->CreateSurface(&ddsd, &draw->ddPrimarySurface, 0);
	if (FAILED(hr)) return -1;

	fo::var::setInt(0x51E2B4) = (DWORD)draw->ddPrimarySurface; //FO_VAR_GNW95_DDPrimarySurface
	fo::var::setInt(0x51E2B8) = (DWORD)draw->ddPrimarySurface; //FO_VAR_GNW95_DDRestoreSurface

	// Create DirectDraw clipper for windowed mode
	if (Graphics::IsWindowedMode) {
		draw->ddObject->CreateClipper(0, &draw->ddClipper, 0);
		draw->ddClipper->SetHWnd(0, window);
		hr = draw->ddPrimarySurface->SetClipper(draw->ddClipper);
		if (FAILED(hr)) return -1;
	}

	// Create the backbuffer surface
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = HRP::Setting::ScreenWidth();
	ddsd.dwHeight = HRP::Setting::ScreenHeight();

	hr = draw->ddObject->CreateSurface(&ddsd, &draw->ddBackSurface, 0);

	//ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;// | DDSD_PIXELFORMAT;
	//ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	//ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
	//ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
	//hr = ddObject->CreateSurface(&ddsd, &ddBuffSurface, 0);

	if (FAILED(hr)) return -1;

	// set greyscale (delete)
	//for (size_t i = 0; i < 256; i++) {
	//	paletteRGB[i].xRGB = (i << 24);// | (i << 16) | (i << 8) | i;
	//}

	hr = draw->ddObject->CreatePalette(DDPCAPS_ALLOW256 | DDPCAPS_8BIT, (PALETTEENTRY*)draw->paletteRGB, &draw->ddPalette, 0);
	if (FAILED(hr)) return -1;

	fo::var::setInt(0x51E2BC) = (DWORD)draw->ddPalette; // FO_VAR_GNW95_DDPrimaryPalette

	return 0;
}

static void __cdecl GNW95_ShowRect(BYTE* srcSurface, int sWidth, int sHeight, int sX, int sY, int width, int height, int x, int y) {
	WinProc::Moving();

	if (sHeight > 0 && sWidth > 0 && fo::var::getInt(FO_VAR_GNW95_isActive)) {
		BYTE* src = srcSurface;
		src += sX + (sY * sWidth);
		fo::func::buf_to_buf(src, width, height, sWidth, draw->BufferSurface() + (x + (y * HRP::Setting::ScreenWidth())), HRP::Setting::ScreenWidth());

		DDSURFACEDESC2 desc;
		//std::memset(&desc, 0, sizeof(DDSURFACEDESC));
		desc.dwSize = sizeof(DDSURFACEDESC2);

		while (true) {
			HRESULT hr = draw->ddBackSurface->Lock(0, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
			if (!hr) break;
			// 0x887601C2
			if (hr != DDERR_SURFACELOST || draw->ddBackSurface->Restore()) return;
		}

		if ((y + height) > (int)desc.dwHeight) {
			if (y > (int)desc.dwHeight) BREAKPOINT; //return;
			height = desc.dwHeight - y;
		}
		if ((x + width) > (int)desc.dwWidth) {
			if (x > (int)desc.dwWidth) BREAKPOINT; // return;
			width = desc.dwWidth - x;
		}

		draw->convertFunc->BufToSurface(src, sWidth, x, y, width, height, &desc);
		draw->ddBackSurface->Unlock(0);

		RECT backRect;
		backRect.left = x;
		backRect.top = y;
		backRect.right = x + width;
		backRect.bottom = y + height;

		const POINT* win = WinProc::GetClientPos();
		RECT primRect;
		primRect.left = win->x + x;
		primRect.top = win->y + y;
		primRect.right = win->x + x + width;
		primRect.bottom = win->y + y + height;

		// Draw
		while (draw->ddPrimarySurface->Blt(&primRect, draw->ddBackSurface, &backRect, DDBLT_WAIT, 0) == DDERR_SURFACELOST) draw->ddPrimarySurface->Restore();
	}
}

static void __cdecl GNW95_zero_vid_mem() {

}

static void __fastcall SetPalette(fo::PALETTE* pal, long start, long count) {
	draw->UpdatePalette(pal, start, count);

	if (count > 0) {
		if (paletteInitialized) {
			draw->UpdateSurface();
		} else {
			__asm mov  eax, FO_VAR_scr_size;
			__asm call fo::funcoffs::win_refresh_all_;
		}
	}
}

static __declspec(naked) void GNW95_SetPaletteEntries_hack_replacement() {
	__asm {
		push ebx;
		mov  ecx, eax;
		call SetPalette;
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
		pop  edx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void game_init_hook_palette_init() {
	__asm {
		call fo::funcoffs::palette_init_;
		mov  paletteInitialized, 1
		retn;
	}
}

static IDirectDraw7* GNW95_reset_mode_hack() {
	draw->ddBackSurface->Release();
	if (draw->ddClipper) {
		draw->ddClipper->Release();
		draw->ddClipper = nullptr;
	}
	draw->ddPrimarySurface = nullptr;
	draw->ddPalette = nullptr;
	draw->ddBackSurface = nullptr;

	return draw->ddObject;
}

void DirectDraw::init() {

	if (Graphics::mode == 2) WinProc::SetMoveKeys();

	// GNW95_init_DirectDraw_
	MakeCall(0x4CAFE6, DirectDrawInit, 1); // GNW95_init_DirectDraw_
	SafeWrite8(0x4CAFEE, CodeType::Jump);
	SafeWrite32(0x4CAFEF, 433); // jmp 0x4CB1A4

	// GNW95_init_mode_ex_
	SafeWrite32(0x4CAE72, (DWORD)&GNW95_ShowRect);     // replace engine GNW95_ShowRect_ with sfall function
	SafeWrite32(0x4CAE77, (DWORD)&GNW95_zero_vid_mem); // replace engine GNW95_zero_vid_mem_ with sfall function

	HookCall(0x44260C, game_init_hook_palette_init);
	MakeJump(fo::funcoffs::GNW95_SetPaletteEntries_ + 1, GNW95_SetPaletteEntries_hack_replacement); // 0x4CB310
	MakeJump(fo::funcoffs::GNW95_SetPalette_, GNW95_SetPalette_hack_replacement); // 0x4CB568

	MakeCall(0x4CB1FD, GNW95_reset_mode_hack);
}

}
