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

/*
static void DDSurfaceToDXTexture(BYTE* src, long width, long height, long src_width, DWORD* dst, long dst_width) {
	if (width <= 0 || height <= 0) return;

	dst_width /= 4;
	size_t s_pitch = src_width - width;
	size_t d_pitch = dst_width - width;

	while (height--) {
		while (width--) {
			*dst++ = *src++ << 24; // write palette color index to texture alpha channel
		}
		dst += d_pitch;
		src += s_pitch;
	}
}
*/

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
	static void __stdcall ForceGraphicsRefresh(DWORD d);

	static __forceinline void UpdateDDSurface(BYTE* surface, int width, int height, int widthFrom, RECT* rect) {
		long x = rect->left;
		long y = rect->top;
		if (Graphics::mode == 0) {
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

			if (primaryDDSurface->Lock(&lockRect, &desc, 0, 0)) return; // lock error

			if (Graphics::GPUBlt == 0) desc.lpSurface = (BYTE*)desc.lpSurface + (desc.lPitch * y) + x;
			fo::func::buf_to_buf(surface, width, height, widthFrom, (BYTE*)desc.lpSurface, desc.lPitch);

			primaryDDSurface->Unlock(desc.lpSurface);
		}
	}
};

static const char* gpuEffectA8 =
	"texture image;"
	"texture palette;"
	"texture head;"
	"texture highlight;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; minFilter=none; magFilter=none; addressU=clamp; addressV=clamp; };"
	"sampler s2 = sampler_state { texture=<head>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"sampler s3 = sampler_state { texture=<highlight>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"float2 size;"
	"float2 corner;"
	"float2 sizehl;"
	"float2 cornerhl;"
	"int showhl;"
	// shader for displaying head textures
	"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float backdrop = tex2D(s0, Tex).a;"
	  "float3 result;"
	  "if (abs(backdrop - 1.0) < 0.001) {" // (48.0 / 255.0) // 48 - key index color
	    "result = tex2D(s2, saturate((Tex - corner) / size));"
	  "} else {"
	    "result = tex1D(s1, backdrop).bgr;" // get color in palette and swap R <> B
	  "}"
	  // blend highlights
	  "if (showhl) {"
	    "float4 h = tex2D(s3, saturate((Tex - cornerhl) / sizehl));"
	    "result = saturate(result + h);" // saturate(result * (1 - h.a) * h.rgb * h.a)"
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
	  "return float4(result.bgr, 1);"                // swap R <> B
	"}"
	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";

static const char* gpuEffectL8 =
	"texture image;"
	"texture palette;"
	"texture head;"
	"texture highlight;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; minFilter=none; magFilter=none; addressU=clamp; addressV=clamp; };"
	"sampler s2 = sampler_state { texture=<head>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"sampler s3 = sampler_state { texture=<highlight>; minFilter=linear; magFilter=linear; addressU=clamp; addressV=clamp; };"
	"float2 size;"
	"float2 corner;"
	"float2 sizehl;"
	"float2 cornerhl;"
	"int showhl;"
	// shader for displaying head textures
	"float4 P1( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float backdrop = tex2D(s0, Tex).r;"
	  "float3 result;"
	  "if (abs(backdrop - 1.0) < 0.001) {"
	    "result = tex2D(s2, saturate((Tex - corner) / size));"
	  "} else {"
	    "result = tex1D(s1, backdrop).bgr;"
	  "}"
	  // blend highlights
	  "if (showhl) {"
	    "float4 h = tex2D(s3, saturate((Tex - cornerhl) / sizehl));"
	    "result = saturate(result + h);"
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
	  "return float4(result.bgr, 1);"
	"}"
	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";

}
