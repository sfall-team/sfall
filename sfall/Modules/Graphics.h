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

#define SAFERELEASE(a) { if (a) { a->Release(); a = 0; } }

class Graphics : public Module {
public:
	const char* name() { return "Graphics"; }
	void init();
	void exit() override;

	static DWORD mode;
	static DWORD GPUBlt;

	static long GetGameWidthRes();
	static long GetGameHeightRes();

	static const float* rcpresGet();

	static void SetHighlightTexture(IDirect3DTexture9* htex);
	static void SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff, int showHighlight);
	static void SetHeadTechnique();
	static void SetDefaultTechnique();

	static void ShowMovieFrame();
	static void SetMovieTexture(IDirect3DTexture9* tex);

	static bool PlayAviMovie;
};

extern IDirect3D9* d3d9;
extern IDirect3DDevice9* d3d9Device;

int _stdcall GetShaderVersion();

void RefreshGraphics();
HWND GetFalloutWindowInfo(RECT* rect);

}
