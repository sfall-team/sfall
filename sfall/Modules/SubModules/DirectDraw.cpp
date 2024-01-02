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

#pragma comment(lib, "ddraw.lib")

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\WinProc.h"
#include "..\Graphics.h"

#include "..\..\HRP\Init.h"

#include "DirectDraw.h"

namespace sfall
{

class DirectDrawObj;

static bool paletteInitialized = false;
static DirectDrawObj* ddObject = nullptr;

class DirectDrawObj {
private:
	class IConvertPalette {
	public:
		virtual DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) = 0;
		virtual void BufToSurface(BYTE* src, int sPitch, int x, int y, int width, int height, DDSURFACEDESC2* desc) = 0;
	};

	class ConvertPalette32 : public IConvertPalette {
		DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) {
			return (r << 16) | (g << 8) | b;
		}

		void BufToSurface(BYTE* src, int sPitch, int x, int y, int width, int height, DDSURFACEDESC2* desc) {
			int dPitch = desc->lPitch >> 2;
			DWORD* dst = (DWORD*)desc->lpSurface;
			dst += (x + (y * dPitch));

			sPitch -= width;
			dPitch -= width;
			while (height--) {
				int x = width;
				while (x--) *dst++ = ddObject->paletteRGB[*src++].xRGB;
				src += sPitch;
				dst += dPitch;
			};
		}
	};

	class ConvertPalette16 : public IConvertPalette {
		// R5G6B5
		DWORD PaletteToRGB(BYTE r, BYTE g, BYTE b) {
			DWORD color = r / 8;
			color <<= 6;
			color |= (g / 4);
			color <<= 5;
			return color | (b / 8);
		}

		void BufToSurface(BYTE* src, int sPitch, int x, int y, int width, int height, DDSURFACEDESC2* desc) {
			int dPitch = desc->lPitch >> 1;
			WORD* dst = (WORD*)desc->lpSurface;
			dst += (x + (y * dPitch));

			sPitch -= width;
			dPitch -= width;
			while (height--) {
				int x = width;
				while (x--) *dst++ = (WORD)(ddObject->paletteRGB[*src++].xRGB);
				src += sPitch;
				dst += dPitch;
			};
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

	BYTE* buffSurface; // intermediate buffer for updating image colors

public:
	IDirectDraw7* ddContext = nullptr; // same as _GNW95_DDObject
	IDirectDrawSurface7* ddPrimarySurface = nullptr;
	IDirectDrawSurface7* ddBackSurface = nullptr;
	IDirectDrawPalette* ddPalette = nullptr;
	IDirectDrawClipper* ddClipper = nullptr;

	DirectDraw::PALCOLOR* paletteRGB;
	IConvertPalette* convertFunc;

	DirectDrawObj(long dwWidth, long dwHeight) {
		buffSurface = new BYTE[dwWidth * dwHeight];
		paletteRGB = new DirectDraw::PALCOLOR[256]();
	}

	BYTE* BufferSurface() {
		return buffSurface;
	}

	void SetDrawTrueColor() {
		convertFunc = new ConvertPalette32();
	}

	void SetDrawHighColor(long bits) {
		if (bits == 0x7E0) { // R5G6B5 (0000011111100000)
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
			HRESULT hr = ddBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
			if (!hr) break;
			if (hr != DDERR_SURFACELOST || ddBackSurface->Restore()) return;
		}

		convertFunc->BufToSurface(buffSurface, desc.dwWidth, 0, 0, desc.dwWidth, desc.dwHeight, &desc);
		ddBackSurface->Unlock(0);

		const POINT* win = WinProc::GetClientPos();
		RECT primRect;
		primRect.left = win->x;
		primRect.top = win->y;
		primRect.right = win->x + (desc.dwWidth << HRP::Setting::ScaleX2());
		primRect.bottom = win->y + (desc.dwHeight << HRP::Setting::ScaleX2());

		// Draw
		while (ddPrimarySurface->Blt(&primRect, ddBackSurface, 0, DDBLT_WAIT, 0) == DDERR_SURFACELOST) ddPrimarySurface->Restore();
	}

	~DirectDrawObj() {
		delete[] paletteRGB;
		delete[] buffSurface;
		delete convertFunc;
	}
};

void SetWindow(HWND window) {
	WinProc::SetHWND(window);
	WinProc::SetSize(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight(), HRP::Setting::ScaleX2());
	WinProc::SetTitle(HRP::Setting::ScreenWidth() << HRP::Setting::ScaleX2(), HRP::Setting::ScreenHeight() << HRP::Setting::ScaleX2(), Graphics::mode);

	if (Graphics::mode == 2) WinProc::LoadPosition();

	if (Graphics::IsWindowedMode) {
		long windowStyle = (Graphics::mode == 3) ? WS_OVERLAPPED : (WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
		WinProc::SetStyle(windowStyle);
	}
}

static long __stdcall DirectDrawInit(DWORD, IDirectDraw7** _ddContext, DWORD) {
	HWND window = (HWND)fo::var::getInt(FO_VAR_GNW95_hwnd);
	SetWindow(window);

	ddObject = new DirectDrawObj(HRP::Setting::ScreenWidth(), HRP::Setting::ScreenHeight());

	HRESULT hr = DirectDrawCreateEx(NULL, (LPVOID*)&ddObject->ddContext, IID_IDirectDraw7, NULL);
	if (FAILED(hr)) return -1;

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddObject->ddContext->GetDisplayMode(&ddsd);

	if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8) {
		MessageBoxA(window, "The current 8-bit video mode is not supported.", "sfall: DirectDraw", 0);
		return -1;
	}

	*_ddContext = ddObject->ddContext;

	hr = ddObject->ddContext->SetCooperativeLevel(window, (Graphics::IsWindowedMode) ? DDSCL_NORMAL : (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_MULTITHREADED));
	if (FAILED(hr)) return -1;

	if (!Graphics::IsWindowedMode) {
		long width = HRP::Setting::ScreenWidth();
		long height = HRP::Setting::ScreenHeight();

		if (HRP::Setting::ScaleX2()) {
			width *= 2;
			height *= 2;
		}
		hr = ddObject->ddContext->SetDisplayMode(width, height, HRP::Setting::ColorBits(), 0, 0);
		if (FAILED(hr)) return -1;
	}

	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = ddObject->ddContext->CreateSurface(&ddsd, &ddObject->ddPrimarySurface, 0);
	if (FAILED(hr)) return -1;

	fo::var::setInt(FO_VAR_GNW95_DDPrimarySurface) = (DWORD)ddObject->ddPrimarySurface;
	fo::var::setInt(FO_VAR_GNW95_DDRestoreSurface) = (DWORD)ddObject->ddPrimarySurface;

	DDPIXELFORMAT pxf;
	std::memset(&pxf, 0, sizeof(DDPIXELFORMAT));
	pxf.dwSize = sizeof(DDPIXELFORMAT);
	ddObject->ddPrimarySurface->GetPixelFormat(&pxf);

	if (pxf.dwRGBBitCount < 24) {
		ddObject->SetDrawHighColor(pxf.dwGBitMask);
	} else {
		ddObject->SetDrawTrueColor();
	}

	// Create DirectDraw clipper for windowed mode
	if (Graphics::IsWindowedMode) {
		ddObject->ddContext->CreateClipper(0, &ddObject->ddClipper, 0);
		ddObject->ddClipper->SetHWnd(0, window);
		hr = ddObject->ddPrimarySurface->SetClipper(ddObject->ddClipper);
		if (FAILED(hr)) return -1;
	}

	// Create the backbuffer surface
	std::memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = HRP::Setting::ScreenWidth();
	ddsd.dwHeight = HRP::Setting::ScreenHeight();

	hr = ddObject->ddContext->CreateSurface(&ddsd, &ddObject->ddBackSurface, 0);

	//ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	//ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	//ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
	//ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
	//hr = ddContext->CreateSurface(&ddsd, &ddBuffSurface, 0);
	if (FAILED(hr)) return -1;

	hr = ddObject->ddContext->CreatePalette(DDPCAPS_ALLOW256 | DDPCAPS_8BIT, (PALETTEENTRY*)ddObject->paletteRGB, &ddObject->ddPalette, 0);
	if (FAILED(hr)) return -1;

	fo::var::setInt(FO_VAR_GNW95_DDPrimaryPalette) = (DWORD)ddObject->ddPalette;

	return 0;
}

static void __cdecl GNW95_ShowRect(BYTE* srcSurface, int sWidth, int sHeight, int sX, int sY, int width, int height, int x, int y) {
	WinProc::Moving();

	if (sHeight > 0 && sWidth > 0 && fo::var::getInt(FO_VAR_GNW95_isActive)) {
		BYTE* src = srcSurface;
		src += sX + (sY * sWidth);
		fo::func::buf_to_buf(src, width, height, sWidth, ddObject->BufferSurface() + (x + (y * HRP::Setting::ScreenWidth())), HRP::Setting::ScreenWidth());

		DDSURFACEDESC2 desc;
		//std::memset(&desc, 0, sizeof(DDSURFACEDESC2));
		desc.dwSize = sizeof(DDSURFACEDESC2);

		while (true) {
			// NOTE: Not locking the entire surface reduces the performance
			HRESULT hr = ddObject->ddBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
			if (!hr) break;
			// 0x887601C2
			if (hr != DDERR_SURFACELOST || ddObject->ddBackSurface->Restore()) return;
		}

		//if ((y + height) > (int)desc.dwHeight) {
		//	if (y > (int)desc.dwHeight) BREAKPOINT;
		//	height = desc.dwHeight - y;
		//}
		//if ((x + width) > (int)desc.dwWidth) {
		//	if (x > (int)desc.dwWidth) BREAKPOINT;
		//	width = desc.dwWidth - x;
		//}

		ddObject->convertFunc->BufToSurface(src, sWidth, x, y, width, height, &desc);
		ddObject->ddBackSurface->Unlock(0);

		RECT backRect;
		backRect.left = x;
		backRect.top = y;
		backRect.right = x + width;
		backRect.bottom = y + height;

		const POINT* win = WinProc::GetClientPos();
		RECT primRect;
		primRect.left = win->x + (x << HRP::Setting::ScaleX2());
		primRect.top = win->y + (y << HRP::Setting::ScaleX2());
		primRect.right = win->x + (backRect.right << HRP::Setting::ScaleX2());
		primRect.bottom = win->y + (backRect.bottom << HRP::Setting::ScaleX2());

		// Draw
		while (ddObject->ddPrimarySurface->Blt(&primRect, ddObject->ddBackSurface, &backRect, DDBLT_WAIT, 0) == DDERR_SURFACELOST) ddObject->ddPrimarySurface->Restore();
	}
}

static void __cdecl GNW95_zero_vid_mem() {
/*
	Surface cleaning is not required.
	This is also not used in HRP by Mash.
*/
}

static void __fastcall SetPalette(fo::PALETTE* pal, long start, long count) {
	ddObject->UpdatePalette(pal, start, count);

	if (count > 0) {
		if (paletteInitialized) {
			ddObject->UpdateSurface();
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

//static __declspec(naked) void loadColorTable_hook() {
//	__asm {
//		call
//		jmp  fo::funcoffs::rebuildColorBlendTables_;
//	}
//}

static HRESULT __stdcall nfConfig_hack_crate_surface(DDSURFACEDESC* desc, LPDIRECTDRAWSURFACE7* nf_mve, long) {
	DDSURFACEDESC2 desc2;
	std::memcpy(&desc2, desc, sizeof(DDSURFACEDESC));
	std::memset(&desc2.ddsCaps.dwCaps2, 0, sizeof(DDSURFACEDESC2) - sizeof(DDSURFACEDESC));

	desc2.dwSize = sizeof(DDSURFACEDESC2);
	return ddObject->ddContext->CreateSurface(&desc2, nf_mve, 0);
}

static IDirectDraw7* GNW95_reset_mode_hack() {
	if (ddObject->ddBackSurface) {
		ddObject->ddBackSurface->Release();
		ddObject->ddBackSurface = nullptr;
	}
	if (ddObject->ddClipper) {
		ddObject->ddClipper->Release();
		ddObject->ddClipper = nullptr;
	}
	ddObject->ddPrimarySurface = nullptr;
	ddObject->ddPalette = nullptr;

	return ddObject->ddContext;
}

void DirectDraw::Clear(long iColor) {
	std::memset(ddObject->BufferSurface(), iColor, HRP::Setting::ScreenWidth() * HRP::Setting::ScreenHeight());

	//DDBLTFX fx;
	//std::memset(&fx, 0, sizeof(DDBLTFX));
	//fx.dwSize = sizeof(DDBLTFX);
	//fx.dwFillColor = ddObject->paletteRGB[iColor].xRGB;
	//HRESULT hr = ddObject->ddBackSurface->Blt(0, 0, 0, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
}

void DirectDraw::init() {
	// GNW95_init_DirectDraw_
	MakeCall(0x4CAFE6, DirectDrawInit, 1); // GNW95_init_DirectDraw_
	SafeWrite8(0x4CAFEE, CodeType::Jump);
	SafeWrite32(0x4CAFEF, 433); // jmp 0x4CB1A4

	// GNW95_init_mode_ex_
	SafeWrite32(0x4CAE72, (DWORD)&GNW95_ShowRect);     // replace engine GNW95_ShowRect_ with sfall function
	SafeWrite32(0x4CAE77, (DWORD)&GNW95_zero_vid_mem); // replace engine GNW95_zero_vid_mem_ with sfall function

	HookCall(0x44260C, game_init_hook_palette_init);
	//HookCall(0x4C7A8D, loadColorTable_hook);

	MakeJump(fo::funcoffs::GNW95_SetPaletteEntries_ + 1, GNW95_SetPaletteEntries_hack_replacement); // 0x4CB310
	MakeJump(fo::funcoffs::GNW95_SetPalette_, GNW95_SetPalette_hack_replacement); // 0x4CB568

	MakeCall(0x4CB1FD, GNW95_reset_mode_hack);

	// nfConfig_
	MakeCall(0x4F5DD1, nfConfig_hack_crate_surface, 1);
	MakeCall(0x4F5DF6, nfConfig_hack_crate_surface, 1);
}

void DirectDraw::exit() {
	if (ddObject) delete ddObject;
}

}
