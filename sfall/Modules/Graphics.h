/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2012  The sfall team
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

#include "Module.h"

namespace sfall
{

extern IDirect3D9* d3d9;
extern IDirect3DDevice9* d3d9Device;
extern IDirectDrawSurface* primaryDDSurface;
extern bool DeviceLost;

class Graphics : public Module {
public:
	const char* name() { return "Graphics"; }
	void init();
	void exit() override;

	static DWORD mode;
	static DWORD GPUBlt;

	static HWND GetFalloutWindowInfo(RECT* rect);
	static long GetGameWidthRes();
	static long GetGameHeightRes();

	static int __stdcall GetShaderVersion();

	static const float* rcpresGet();

	static void SetHighlightTexture(IDirect3DTexture9* htex, int xPos, int yPos);
	static void SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff, int showHighlight);
	static void SetHeadTechnique();
	static void SetDefaultTechnique();

	static void ShowMovieFrame(IDirect3DTexture9* movieTex);

	static void SetMovieTexture(bool state);
	static HRESULT CreateMovieTexture(D3DSURFACE_DESC &desc);
	static void ReleaseMovieTexture();

	static bool PlayAviMovie;
	static bool AviMovieWidthFit;

	static void RefreshGraphics();

	static __forceinline void UpdateDDSurface(BYTE* surface, int width, int height, int widthFrom, RECT* rect) {
		if (!DeviceLost) {
			DDSURFACEDESC desc;
			RECT lockRect = { rect->left, rect->top, rect->right + 1, rect->bottom + 1 };

			primaryDDSurface->Lock(&lockRect, &desc, 0, 0);

			if (Graphics::GPUBlt == 0) desc.lpSurface = (BYTE*)desc.lpSurface + (desc.lPitch * rect->top) + rect->left;
			fo::func::buf_to_buf(surface, width, height, widthFrom, (BYTE*)desc.lpSurface, desc.lPitch);

			primaryDDSurface->Unlock(desc.lpSurface);
		}
	}
};

}
