/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <ddraw.h>

namespace sfall
{

extern IDirect3D9* d3d9;
extern IDirect3DDevice9* d3d9Device;
extern IDirectDrawSurface* primarySurface;

class Graphics {
public:
	static const char* name() { return "Graphics"; }
	static void init();
	static void exit();

	static DWORD mode;
	static DWORD GPUBlt;
	static bool IsWindowedMode;

	static long GetGameWidthRes();
	static long GetGameHeightRes();

	static int __stdcall GetShaderVersion();

	static const float* rcpresGet();

	static void SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff);
	static void SetHeadTechnique();
	static void SetDefaultTechnique();

	static void ShowMovieFrame(IDirect3DTexture9* movieTex);

	static void SetMovieTexture(bool state);
	static HRESULT CreateMovieTexture(D3DSURFACE_DESC &desc);
	static void ReleaseMovieTexture();

	static bool PlayAviMovie;
	static bool AviMovieWidthFit;

	static void RefreshGraphics();
	static void __stdcall ForceGraphicsRefresh(DWORD d);

	static __forceinline void UpdateDDSurface(BYTE* surface, int width, int height, int widthFrom, RECT* rect) {
		long x = rect->left;
		long y = rect->top;
		if (Graphics::mode < 4) {
			__asm {
				xor  eax, eax;
				push y;
				push x;
				push height;
				push width;
				push eax; // yFrom
				push eax; // xFrom
				push height; // heightFrom
				push widthFrom;
				push surface;
				call ds:[FO_VAR_scr_blit]; // call GNW95_ShowRect_(int from, int widthFrom, int heightFrom, int xFrom, int yFrom, int width, int height, int x, int y)
				add  esp, 9*4;
			}
		} else {
			DDSURFACEDESC desc;
			RECT lockRect = { x, y, rect->right + 1, rect->bottom + 1 };

			if (primarySurface->Lock(&lockRect, &desc, 0, 0)) return; // lock error

			if (Graphics::GPUBlt == 0) desc.lpSurface = (BYTE*)desc.lpSurface + (desc.lPitch * y) + x;
			fo::func::buf_to_buf(surface, width, height, widthFrom, (BYTE*)desc.lpSurface, desc.lPitch);

			primarySurface->Unlock(desc.lpSurface);
		}
	}
};

class WindowRender {
public:
	static void CreateOverlaySurface(fo::Window* win, long winType);
	static void DestroyOverlaySurface(fo::Window* win);
	static void ClearOverlay(fo::Window* win);
	static void ClearOverlay(fo::Window* win, Rectangle &rect);
	static BYTE* GetOverlaySurface(fo::Window* win);
};

}
