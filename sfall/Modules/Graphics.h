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

extern IDirect3D9* d3d9;
extern IDirect3DDevice9* d3d9Device;
extern IDirectDrawSurface* primarySurface;

extern DWORD GraphicsMode;
extern DWORD GPUBlt;

extern bool Gfx_PlayAviMovie;
extern bool Gfx_AviMovieWidthFit;

void Graphics_Init();
void Graphics_Exit();

HWND Gfx_GetFalloutWindowInfo(RECT* rect);
long Gfx_GetGameWidthRes();
long Gfx_GetGameHeightRes();

int __stdcall Gfx_GetShaderVersion();

const float* Gfx_rcpresGet();

void Gfx_SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff);
void Gfx_SetHeadTechnique();
void Gfx_SetDefaultTechnique();

void Gfx_ShowMovieFrame(IDirect3DTexture9* movieTex);

void Gfx_SetMovieTexture(bool state);
HRESULT Gfx_CreateMovieTexture(D3DSURFACE_DESC &desc);
void Gfx_ReleaseMovieTexture();

void Gfx_RefreshGraphics();
void __stdcall Gfx_ForceGraphicsRefresh(DWORD d);

void WinRender_CreateOverlaySurface(WINinfo* win, long winType);
void WinRender_DestroyOverlaySurface(WINinfo* win);
void WinRender_ClearOverlay(WINinfo* win);
void WinRender_ClearOverlay(WINinfo* win, sRectangle &rect);
BYTE* WinRender_GetOverlaySurface(WINinfo* win);

__forceinline void UpdateDDSurface(BYTE* surface, int width, int height, int widthFrom, RECT* rect) {
	long x = rect->left;
	long y = rect->top;
	if (GraphicsMode == 0) {
		__asm {
			xor  eax, eax;
			push y;
			push x;
			push height;
			push width;
			push eax; // yFrom
			push eax; // xFrom
			push eax; // heightFrom
			push widthFrom;
			push surface;
			call ds:[FO_VAR_scr_blit]; // GNW95_ShowRect_(int from, int widthFrom, int heightFrom, int xFrom, int yFrom, int width, int height, int x, int y)
			add  esp, 9*4;
		}
	} else {
		DDSURFACEDESC desc;
		RECT lockRect = { x, y, rect->right + 1, rect->bottom + 1 };

		if (primarySurface->Lock(&lockRect, &desc, 0, 0)) return; // lock error

		if (GPUBlt == 0) desc.lpSurface = (BYTE*)desc.lpSurface + (desc.lPitch * y) + x;
		fo_buf_to_buf(surface, width, height, widthFrom, (BYTE*)desc.lpSurface, desc.lPitch);

		primarySurface->Unlock(desc.lpSurface);
	}
}

static const char* gpuEffectA8 =
	"texture image;"
	"texture palette;"
	"texture head;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; minFilter=none; magFilter=none; addressU=clamp; addressV=clamp; };"
	"sampler s2 = sampler_state { texture=<head>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"float2 size;"
	"float2 corner;"
	// shader for displaying head textures
	"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float backdrop = tex2D(s0, Tex).a;"
	  "float3 result;"
	  "if (abs(backdrop - 1.0) < 0.001) {" // (48.0 / 255.0) // 48 - key index color
	    "result = tex2D(s2, saturate((Tex - corner) / size));"
	  "} else {"
	    "result = tex1D(s1, backdrop);" // get color in palette
	  "}"
	  "return float4(result, 1);"
	"}"
	"technique T1"
	"{"
	  "pass p1 { PixelShader = compile ps_2_0 P1(); }"
	"}"

	// main shader
	"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float3 result = tex1D(s1, tex2D(s0, Tex).a);" // get color in palette
	  "return float4(result, 1);"
	"}"
	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";

static const char* gpuEffectL8 =
	"texture image;"
	"texture palette;"
	"texture head;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; minFilter=none; magFilter=none; addressU=clamp; addressV=clamp; };"
	"sampler s2 = sampler_state { texture=<head>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"float2 size;"
	"float2 corner;"
	// shader for displaying head textures
	"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float backdrop = tex2D(s0, Tex).r;"
	  "float3 result;"
	  "if (abs(backdrop - 1.0) < 0.001) {"
	    "result = tex2D(s2, saturate((Tex - corner) / size));"
	  "} else {"
	    "result = tex1D(s1, backdrop);"
	  "}"
	  "return float4(result, 1);"
	"}"
	"technique T1"
	"{"
	  "pass p1 { PixelShader = compile ps_2_0 P1(); }"
	"}"

	// main shader
	"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float3 result = tex1D(s1, tex2D(s0, Tex).r);"
	  "return float4(result, 1);"
	"}"
	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";
