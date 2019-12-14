/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#define DEBUGMESS(a, b) OutputDebugStringA(a b)
#else
#ifndef NDEBUG
#define DEBUGMESS(a, b) DebugPrintf(a, b)
#else
#define DEBUGMESS(a, b)
#endif
#endif

#include <math.h>
#include <stdio.h>
#include <vector>

#include "main.h"

#include "FalloutEngine.h"
#include "Graphics.h"
#include "input.h"
#include "LoadGameHook.h"
#include "TalkingHeads.h"
#include "Version.h"

typedef HRESULT (_stdcall *DDrawCreateProc)(void*, IDirectDraw**, void*);
typedef IDirect3D9* (_stdcall *D3DCreateProc)(UINT version);

#define UNUSEDFUNCTION { DEBUGMESS("\n[SFALL] Unused function called: %s", __FUNCTION__); return DDERR_GENERIC; }

static DWORD ResWidth;
static DWORD ResHeight;

DWORD GPUBlt;
DWORD GraphicsMode;

static BYTE* titlesBuffer = nullptr;
static DWORD movieHeight = 0;
//static DWORD movieWidth = 0;
static DWORD yoffset;
//static DWORD xoffset;

static bool DeviceLost = false;
static DDSURFACEDESC surfaceDesc;
static DDSURFACEDESC movieDesc;

static DWORD palette[256];
//static bool paletteInit = false;

static DWORD gWidth;
static DWORD gHeight;

static int ScrollWindowKey;
static bool windowInit = false;
static DWORD windowLeft = 0;
static DWORD windowTop = 0;

static DWORD ShaderVersion;

static HWND window;
IDirect3D9* d3d9 = 0;
IDirect3DDevice9* d3d9Device = 0;
static IDirect3DTexture9* Tex = 0;
static IDirect3DTexture9* sTex1 = 0;
static IDirect3DTexture9* sTex2 = 0;
static IDirect3DSurface9* sSurf1 = 0;
static IDirect3DSurface9* sSurf2 = 0;
static IDirect3DSurface9* backbuffer = 0;
static IDirect3DVertexBuffer9* vBuffer;
static IDirect3DVertexBuffer9* vBuffer2;
static IDirect3DVertexBuffer9* movieBuffer;
static IDirect3DTexture9* gpuPalette;
static IDirect3DTexture9* movieTex = 0;

static ID3DXEffect* gpuBltEffect;
static const char* gpuEffect=
	"texture image;"
	"texture palette;"
	"texture head;"
	"sampler s0 = sampler_state { texture=<image>; };"
	"sampler s1 = sampler_state { texture=<palette>; };"
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
	    "result = tex1D(s1, backdrop);"
	    "result = float3(result.b, result.g, result.r);"
	  "}"
	  "return float4(result.r, result.g, result.b, 1);"
	"}"

	"technique T1"
	"{"
	  "pass p1 { PixelShader = compile ps_2_0 P1(); }"
	"}"

	"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float3 result = tex1D(s1, tex2D(s0, Tex).a);"
	  "return float4(result.b, result.g, result.r, 1);"
	"}"

	"technique T0"
	"{"
	  "pass p0 { PixelShader = compile ps_2_0 P0(); }"
	"}";

static D3DXHANDLE gpuBltBuf;
static D3DXHANDLE gpuBltPalette;
static D3DXHANDLE gpuBltHead;
static D3DXHANDLE gpuBltHeadSize;
static D3DXHANDLE gpuBltHeadCorner;

static float rcpres[2];

static size_t shadersSize;

static bool globalShaderActive = false;
std::string gShaderFile;

struct sShader {
	ID3DXEffect* Effect;
	D3DXHANDLE ehTicks;
	DWORD mode;
	DWORD mode2;
	bool Active;

	sShader() : Effect(0), ehTicks(0), mode(0), mode2(0), Active(false) {}
};

static std::vector<sShader> shaders;
static std::vector<IDirect3DTexture9*> shaderTextures;

#define MYVERTEXFORMAT D3DFVF_XYZRHW|D3DFVF_TEX1
struct MyVertex {
	float x,y,z,w,u,v;
};

static MyVertex ShaderVertices[] = {
	{-0.5,  -0.5,  0, 1, 0, 0},
	{-0.5,  479.5, 0, 1, 0, 1},
	{639.5, -0.5,  0, 1, 1, 0},
	{639.5, 479.5, 0, 1, 1, 1}
};

void GetFalloutWindowInfo(DWORD* width, DWORD* height, HWND* wnd) {
	*width = gWidth;
	*height = gHeight;
	*wnd = window;
}

long Gfx_GetGameWidthRes() {
	return (ptr_scr_size->offx - ptr_scr_size->x) + 1;
}

long Gfx_GetGameHeightRes() {
	return (ptr_scr_size->offy - ptr_scr_size->y) + 1;
}

int _stdcall GetShaderVersion() {
	return ShaderVersion;
}

void _stdcall SetShaderMode(DWORD d, DWORD mode) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	if (mode & 0x80000000) {
		shaders[d].mode2 = mode ^ 0x80000000;
	} else {
		shaders[d].mode = mode;
	}
}

int _stdcall LoadShader(const char* file) {
	if (!GraphicsMode || strstr(file, "..") || strstr(file, ":")) return -1;
	char buf[MAX_PATH];
	PathNode* pathsPtr = *ptr_paths;
	sprintf_s(buf, "%s\\shaders\\%s", pathsPtr->path, file);
	for (DWORD d = 0; d < shadersSize; d++) {
		if (!shaders[d].Effect) {
			if (FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shaders[d].Effect, 0))) {
				return -1;
			} else {
				return d;
			}
		}
	}
	sShader shader = sShader();
	if (FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shader.Effect, 0))) return -1;

	shader.Effect->SetFloatArray("rcpres", rcpres, 2);

	for (int i = 1; i < 128; i++) {
		const char* name;
		IDirect3DTexture9* tex;

		sprintf(buf, "texname%d", i);
		if (FAILED(shader.Effect->GetString(buf, &name))) break;
		sprintf_s(buf, "%s\\art\\stex\\%s", *(char**)_patches, name);
		if (FAILED(D3DXCreateTextureFromFileA(d3d9Device, buf, &tex))) continue;
		sprintf(buf, "tex%d", i);
		shader.Effect->SetTexture(buf, tex);
		shaderTextures.push_back(tex);
	}

	shader.ehTicks = shader.Effect->GetParameterByName(0, "tickcount");
	shaders.push_back(shader);
	shadersSize = shaders.size();
	return shadersSize - 1;
}

static void LoadGlobalShader() {
	if (!globalShaderActive) return;
	long index = LoadShader(gShaderFile.c_str());
	if (index != -1) {
		shaders[index].Effect->SetInt("w", Gfx_GetGameWidthRes());
		shaders[index].Effect->SetInt("h", Gfx_GetGameHeightRes());
		shaders[index].Active = true;
		dlogr("Global shader file loaded.", DL_INIT);
	}
}

void _stdcall ActivateShader(DWORD d) {
	if (d < shadersSize && shaders[d].Effect) shaders[d].Active = true;
}

void _stdcall DeactivateShader(DWORD d) {
	if (d < shadersSize) shaders[d].Active = false;
}

int _stdcall GetShaderTexture(DWORD d, DWORD id) {
	if (id < 1 || id > 128 || d >= shadersSize || !shaders[d].Effect) return -1;
	IDirect3DBaseTexture9* tex = 0;
	char buf[8] = "tex";
	_itoa_s(id, &buf[3], 4, 10);
	if (FAILED(shaders[d].Effect->GetTexture(buf, &tex)) || !tex) return -1;
	tex->Release();
	for (DWORD i = 0; i < shaderTextures.size(); i++) {
		if (shaderTextures[i] == tex) return i;
	}
	return -1;
}

void _stdcall FreeShader(DWORD d) {
	if (d < shadersSize) {
		SAFERELEASE(shaders[d].Effect);
		shaders[d].Active = false;
	}
}

void _stdcall SetShaderInt(DWORD d, const char* param, int value) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetInt(param, value);
}

void _stdcall SetShaderFloat(DWORD d, const char* param, float value) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetFloat(param, value);
}

void _stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetFloatArray(param, &f1, 4);
}

void _stdcall SetShaderTexture(DWORD d, const char* param, DWORD value) {
	if (d >= shadersSize || !shaders[d].Effect || value >= shaderTextures.size()) return;
	shaders[d].Effect->SetTexture(param, shaderTextures[value]);
}

static void WindowInit() {
	windowInit = true;
	rcpres[0] = 1.0f / (float)Gfx_GetGameWidthRes();
	rcpres[1] = 1.0f / (float)Gfx_GetGameHeightRes();
	LoadGlobalShader();
}

static void ResetDevice(bool CreateNew) {
	D3DPRESENT_PARAMETERS params;
	ZeroMemory(&params, sizeof(params));
	params.BackBufferCount = 1;
	params.BackBufferFormat = (GraphicsMode == 5) ? D3DFMT_UNKNOWN : D3DFMT_X8R8G8B8;
	params.BackBufferWidth = gWidth;
	params.BackBufferHeight = gHeight;
	params.EnableAutoDepthStencil = false;
	params.MultiSampleQuality = 0;
	params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.Windowed = (GraphicsMode == 5);
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	bool software = false;
	if (CreateNew) {
		dlog("Creating D3D9 Device...", DL_MAIN);
		if (FAILED(d3d9->CreateDevice(0, D3DDEVTYPE_HAL, window, D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &params, &d3d9Device))) {
			software = true;
			d3d9->CreateDevice(0, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &params, &d3d9Device);
		}
		D3DCAPS9 caps;
		d3d9Device->GetDeviceCaps(&caps);
		ShaderVersion = ((caps.PixelShaderVersion & 0x0000ff00) >> 8) * 10 + (caps.PixelShaderVersion & 0xff);

		if (GPUBlt == 2) {
			if (ShaderVersion < 20) GPUBlt = 0;
		}

		if (GPUBlt) {
			D3DXCreateEffect(d3d9Device, gpuEffect, strlen(gpuEffect), 0, 0, 0, 0, &gpuBltEffect, 0);
			gpuBltBuf = gpuBltEffect->GetParameterByName(0, "image");
			gpuBltPalette = gpuBltEffect->GetParameterByName(0, "palette");
			// for head textures
			gpuBltHead = gpuBltEffect->GetParameterByName(0, "head");
			gpuBltHeadSize = gpuBltEffect->GetParameterByName(0, "size");
			gpuBltHeadCorner = gpuBltEffect->GetParameterByName(0, "corner");

			Gfx_SetDefaultTechnique();
		}
	} else {
		d3d9Device->Reset(&params);
		if (gpuBltEffect) gpuBltEffect->OnResetDevice();
		for (DWORD d = 0; d < shadersSize; d++) {
			if (shaders[d].Effect) shaders[d].Effect->OnResetDevice();
		}
	}

	ShaderVertices[1].y = ResHeight - 0.5f;
	ShaderVertices[2].x = ResWidth - 0.5f;
	ShaderVertices[3].y = ResHeight - 0.5f;
	ShaderVertices[3].x = ResWidth - 0.5f;
	if (d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, GPUBlt ? D3DFMT_A8 : D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &Tex, 0) != D3D_OK) {
		d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &Tex, 0);
		GPUBlt = 0;
		dlog(" Error: D3DFMT_A8 unsupported texture format. Now CPU is used to convert the palette.", DL_MAIN);
	}
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex1, 0);
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex2, 0);
	if (GPUBlt) {
		d3d9Device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &gpuPalette, 0);
		gpuBltEffect->SetTexture(gpuBltBuf, Tex);
		gpuBltEffect->SetTexture(gpuBltPalette, gpuPalette);
	}

	sTex1->GetSurfaceLevel(0, &sSurf1);
	sTex2->GetSurfaceLevel(0, &sSurf2);
	d3d9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);

	d3d9Device->CreateVertexBuffer(4 * sizeof(MyVertex), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), MYVERTEXFORMAT, D3DPOOL_DEFAULT, &vBuffer, 0);
	byte* VertexPointer;
	vBuffer->Lock(0, 0, (void**)&VertexPointer, 0);
	CopyMemory(VertexPointer, ShaderVertices, sizeof(ShaderVertices));
	vBuffer->Unlock();

	d3d9Device->CreateVertexBuffer(4 * sizeof(MyVertex), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), MYVERTEXFORMAT, D3DPOOL_DEFAULT, &movieBuffer, 0);

	MyVertex ShaderVertices2[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	ShaderVertices2[1].y = (float)gHeight - 0.5f;
	ShaderVertices2[2].x = (float)gWidth - 0.5f;
	ShaderVertices2[3].y = (float)gHeight - 0.5f;
	ShaderVertices2[3].x = (float)gWidth - 0.5f;

	d3d9Device->CreateVertexBuffer(4 * sizeof(MyVertex), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), MYVERTEXFORMAT, D3DPOOL_DEFAULT, &vBuffer2, 0);
	vBuffer2->Lock(0, 0, (void**)&VertexPointer, 0);
	CopyMemory(VertexPointer, ShaderVertices2, sizeof(ShaderVertices2));
	vBuffer2->Unlock();

	d3d9Device->SetFVF(MYVERTEXFORMAT);
	d3d9Device->SetTexture(0, Tex);
	d3d9Device->SetStreamSource(0, vBuffer, 0, sizeof(MyVertex));

	d3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false); // default false
	d3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);  // default false
	d3d9Device->SetRenderState(D3DRS_ZENABLE, false);
	d3d9Device->SetRenderState(D3DRS_CULLMODE, 2);
	//d3d9Device->SetRenderState(D3DRS_TEXTUREFACTOR, 0);

	if (CreateNew) dlogr(" Done.", DL_MAIN);
}

static void Present() {
	if (ScrollWindowKey != 0 && ((ScrollWindowKey > 0 && KeyDown((BYTE)ScrollWindowKey))
		|| (ScrollWindowKey == -1 && (KeyDown(DIK_LCONTROL) || KeyDown(DIK_RCONTROL)))
		|| (ScrollWindowKey == -2 && (KeyDown(DIK_LMENU) || KeyDown(DIK_RMENU)))
		|| (ScrollWindowKey == -3 && (KeyDown(DIK_LSHIFT) || KeyDown(DIK_RSHIFT)))))
	{
		int winx, winy;
		GetMouse(&winx, &winy);
		windowLeft += winx;
		windowTop += winy;
		RECT r, r2;
		r.left = windowLeft;
		r.right = windowLeft + gWidth;
		r.top = windowTop;
		r.bottom = windowTop + gHeight;
		AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_BORDER, false);
		r.right -= (r.left - windowLeft);
		r.left = windowLeft;
		r.bottom -= (r.top - windowTop);
		r.top = windowTop;
		if (GetWindowRect(GetShellWindow(), &r2)) {
			if (r.right > r2.right) {
				DWORD move = r.right - r2.right;
				r.left -= move;
				r.right -= move;
				windowLeft -= move;
			}
			if (r.left < r2.left) {
				DWORD move = r2.left - r.left;
				r.left += move;
				r.right += move;
				windowLeft += move;
			}
			if (r.bottom > r2.bottom) {
				DWORD move = r.bottom - r2.bottom;
				r.top -= move;
				r.bottom -= move;
				windowTop -= move;
			}
			if (r.top < r2.top) {
				DWORD move = r2.top - r.top;
				r.top += move;
				r.bottom += move;
				windowTop += move;
			}
		}
		MoveWindow(window, r.left, r.top, r.right - r.left, r.bottom - r.top, true);
	}

	if (d3d9Device->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) {
		d3d9Device->SetTexture(0, 0);
		SAFERELEASE(Tex)
		SAFERELEASE(backbuffer);
		SAFERELEASE(sSurf1);
		SAFERELEASE(sSurf2);
		SAFERELEASE(sTex1);
		SAFERELEASE(sTex2);
		SAFERELEASE(vBuffer);
		SAFERELEASE(vBuffer2);
		SAFERELEASE(movieBuffer);
		SAFERELEASE(gpuPalette);
		if (gpuBltEffect) gpuBltEffect->OnLostDevice();
		for (DWORD d = 0; d < shadersSize; d++) {
			if (shaders[d].Effect) shaders[d].Effect->OnLostDevice();
		}
		DeviceLost = true;
	}
}

void RefreshGraphics() {
	if (DeviceLost) return;
	//Tex->UnlockRect(0);
	d3d9Device->BeginScene();
	//d3d9Device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
	d3d9Device->SetStreamSource(0, vBuffer, 0, sizeof(MyVertex));
	d3d9Device->SetRenderTarget(0, sSurf1);

	if (GPUBlt && shadersSize) {
		UINT unused;
		gpuBltEffect->Begin(&unused, 0);
		gpuBltEffect->BeginPass(0);
		d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		gpuBltEffect->EndPass();
		gpuBltEffect->End();
		d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE);
		d3d9Device->SetTexture(0, sTex2);
	} else {
		d3d9Device->SetTexture(0, Tex);
	}
	for (int d = shadersSize - 1; d >= 0; d--) {
		if (!shaders[d].Effect || !shaders[d].Active) continue;
		if (shaders[d].mode2 && !(shaders[d].mode2 & GetLoopFlags())) continue;
		if (shaders[d].mode & GetLoopFlags()) continue;

		if (shaders[d].ehTicks) shaders[d].Effect->SetInt(shaders[d].ehTicks, GetTickCount());
		UINT passes;
		shaders[d].Effect->Begin(&passes, 0);
		for (DWORD pass = 0; pass < passes; pass++) {
			shaders[d].Effect->BeginPass(pass);
			d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			shaders[d].Effect->EndPass();
			d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE);
			d3d9Device->SetTexture(0, sTex2);
		}
		shaders[d].Effect->End();
		d3d9Device->SetTexture(0, sTex2);
	}

	d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(MyVertex));
	d3d9Device->SetRenderTarget(0, backbuffer);
	if (GPUBlt && !shadersSize) {
		UINT unused;
		gpuBltEffect->Begin(&unused, 0);
		gpuBltEffect->BeginPass(0);
	}
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (GPUBlt && !shadersSize) {
		gpuBltEffect->EndPass();
		gpuBltEffect->End();
	}
	d3d9Device->EndScene();
	Present();
}

void SetMovieTexture(void* tex) {
	movieTex = (IDirect3DTexture9*)tex;
	D3DSURFACE_DESC desc;
	movieTex->GetLevelDesc(0, &desc);
	float aspect = (float)desc.Width / (float)desc.Height;
	float winaspect = (float)gWidth / (float)gHeight;

	byte* VertexPointer;
	movieBuffer->Lock(0, 0, (void**)&VertexPointer, 0);
	MyVertex ShaderVertices2[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	ShaderVertices2[1].y = (float)gHeight - 0.5f;
	ShaderVertices2[2].x = (float)gWidth - 0.5f;
	ShaderVertices2[3].y = (float)gHeight - 0.5f;
	ShaderVertices2[3].x = (float)gWidth - 0.5f;

	DWORD gap;
	if (aspect > winaspect) {
		aspect = (float)desc.Width / (float)gWidth;
		desc.Height = (int)(desc.Height / aspect);
		gap = (gHeight - desc.Height) / 2;

		ShaderVertices2[0].y += gap;
		ShaderVertices2[2].y += gap;
		ShaderVertices2[1].y -= gap;
		ShaderVertices2[3].y -= gap;
	} else if (aspect < winaspect) {
		aspect = (float)desc.Height / (float)gHeight;
		desc.Width = (int)(desc.Width / aspect);
		gap = (gWidth - desc.Width) / 2;
		ShaderVertices2[0].x += gap;
		ShaderVertices2[2].x -= gap;
		ShaderVertices2[3].x -= gap;
		ShaderVertices2[1].x += gap;
	}
	CopyMemory(VertexPointer, ShaderVertices2, sizeof(ShaderVertices2));
	movieBuffer->Unlock();
}

void _stdcall PlayMovieFrame() {
	d3d9Device->SetTexture(0, movieTex);
	d3d9Device->SetStreamSource(0, movieBuffer, 0, sizeof(MyVertex));
	d3d9Device->BeginScene();
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	d3d9Device->EndScene();
	Present();
}

void Gfx_SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff) {
	gpuBltEffect->SetTexture(gpuBltHead, tex);

	float size[2];
	size[0] = (float)width * rcpres[0];
	size[1] = (float)height * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadSize, size, 2);

	// adjust head texture position for HRP 4.1.8
	int h = Gfx_GetGameHeightRes();
	if (h > 480) yoff += ((h - 480) / 2) - 47;
	xoff += ((Gfx_GetGameWidthRes() - 640) / 2);

	size[0] = (126.0f + xoff + ((388 - width) / 2)) * rcpres[0];
	size[1] = (14.0f + yoff + ((200 - height) / 2)) * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadCorner, size, 2);

	Gfx_SetHeadTechnique();
}

void Gfx_SetHeadTechnique() {
	gpuBltEffect->SetTechnique("T1");
}

void Gfx_SetDefaultTechnique() {
	gpuBltEffect->SetTechnique("T0");
}

void GraphicsResetOnGameLoad() {
	for (DWORD d = 0; d < shadersSize; d++) SAFERELEASE(shaders[d].Effect);
	shaders.clear();
	shadersSize = 0;
	LoadGlobalShader();
}

class FakePalette2 : IDirectDrawPalette {
private:
	ULONG Refs;
public:
	FakePalette2() {
		Refs = 1;
	}

	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID, LPVOID*) {
		return E_NOINTERFACE;
	}

	ULONG _stdcall AddRef() {
		return ++Refs;
	}

	ULONG _stdcall Release() {
		if (!--Refs) {
			delete this;
			return 0;
		} else return Refs;
	}

	// IDirectDrawPalette methods
	HRESULT _stdcall GetCaps(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetEntries(DWORD, DWORD, DWORD, LPPALETTEENTRY) { UNUSEDFUNCTION; }
	HRESULT _stdcall Initialize(LPDIRECTDRAW, DWORD, LPPALETTEENTRY) { UNUSEDFUNCTION; }

	HRESULT _stdcall SetEntries(DWORD, DWORD b, DWORD c, LPPALETTEENTRY destPal) {
		if (!windowInit || c == 0 || b + c > 256) return DDERR_INVALIDPARAMS;

		CopyMemory(&palette[b], destPal, c * 4);
		if (GPUBlt) {
			if (gpuPalette) {
				D3DLOCKED_RECT rect;
				if (!FAILED(gpuPalette->LockRect(0, &rect, 0, D3DLOCK_DISCARD))) {
					CopyMemory(rect.pBits, palette, 256 * 4);
					gpuPalette->UnlockRect(0);
				}
			}
		} else {
			for (DWORD i = b; i < b + c; i++) { // swap color R <> B
				//palette[i]&=0x00ffffff;
				BYTE clr = *(BYTE*)((DWORD)&palette[i]); // B
				*(BYTE*)((DWORD)&palette[i]) = *(BYTE*)((DWORD)&palette[i] + 2); // R
				*(BYTE*)((DWORD)&palette[i] + 2) = clr;
			}
		}
		RefreshGraphics();
		return DD_OK;
	}
};

class FakeSurface2 : IDirectDrawSurface {
private:
	ULONG Refs;
	bool Primary;
	BYTE* lockTarget;
	static bool IsPlayMovie;
	static bool subTitlesShow;

public:
	FakeSurface2(bool primary) {
		Refs = 1;
		Primary = primary;
		lockTarget = new BYTE[ResWidth * ResHeight];
	}

	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID, LPVOID *) {
		return E_NOINTERFACE;
	}

	ULONG _stdcall AddRef() {
		return ++Refs;
	}

	ULONG _stdcall Release() {
		if (!--Refs) {
			delete[] lockTarget;
			delete this;
			return 0;
		} else return Refs;
	}

	// IDirectDrawSurface methods
	HRESULT _stdcall AddAttachedSurface(LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
	HRESULT _stdcall AddOverlayDirtyRect(LPRECT) { UNUSEDFUNCTION; }

	HRESULT _stdcall Blt(LPRECT a, LPDIRECTDRAWSURFACE b, LPRECT c, DWORD d, LPDDBLTFX e) { // for game movies (is not primary)
		//if (((FakeSurface2*)b)->Primary) return DD_OK; // for testing

		IsPlayMovie = true;
		movieHeight = (a->bottom - a->top);
		yoffset = (ResHeight - movieHeight) / 2;
		//movieWidth = (a->right - a->left);
		//xoffset = (ResWidth - movieWidth) / 2;

		BYTE* lockTarget = ((FakeSurface2*)b)->lockTarget;
		D3DLOCKED_RECT dRect;
		Tex->LockRect(0, &dRect, a, 0);
		DWORD width = ResWidth;
		int pitch = dRect.Pitch;
		if (GPUBlt) {
			char* pBits = (char*)dRect.pBits;
			if (subTitlesShow) {
				subTitlesShow = false;
				DWORD bottom = yoffset + movieHeight;
				for (DWORD y = 0; y < ResHeight; y++) {
					if (y < yoffset || y > bottom) {  // copy excluding video region
						CopyMemory(&pBits[(y - yoffset) * pitch], &titlesBuffer[y * width], width);
					}
				}
			}
			for (DWORD y = 0; y < movieHeight; y++) {
				CopyMemory(&pBits[y * pitch], &lockTarget[y * width], width);
			}
			//if (ResWidth > 640) {
			//	for (DWORD y = yoffset; y < ResHeight - yoffset; y++) {
			//		ZeroMemory(&pBits[y*dRect.Pitch], xoffset);
			//		ZeroMemory(&pBits[y*dRect.Pitch + (ResWidth - xoffset)], xoffset);
			//	}
			//}
		} else {
			pitch /= 4;
			if (subTitlesShow) {
				subTitlesShow = false;
				DWORD bottom = yoffset + movieHeight;
				for (DWORD y = 0; y < ResHeight; y++) {
					if (y < yoffset || y > bottom) {
						int yyp = (y - yoffset) * pitch;
						int yw = y * width;
						for (DWORD x = 0; x < width; x++) {
							((DWORD*)dRect.pBits)[yyp + x] = palette[titlesBuffer[yw + x]];
						}
					}
				}
			}
			for (DWORD y = 0; y < movieHeight; y++) {
				int yp = y * pitch;
				int yw = y * width;
				for (DWORD x = 0; x < width; x++) {
					((DWORD*)dRect.pBits)[yp + x] = palette[lockTarget[yw + x]];
				}
			}
			//if (ResWidth > 640) {
			//	for (DWORD y = yoffset; y < ResHeight - yoffset; y++) {
			//		for (DWORD x = 0; x < xoffset; x++) ((DWORD*)dRect.pBits)[(y)*dRect.Pitch + x] = 0;
			//		for (DWORD x = ResWidth - xoffset; x < ResWidth; x++) ((DWORD*)dRect.pBits)[(y)*dRect.Pitch + x] = 0;
			//	}
			//}
		}
		Tex->UnlockRect(0);

		if (!DeviceLost) {
			d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(MyVertex));
			d3d9Device->SetTexture(0, Tex);
			d3d9Device->BeginScene();
			if (GPUBlt) {
				UINT unused;
				gpuBltEffect->Begin(&unused, 0);
				gpuBltEffect->BeginPass(0);
			}
			d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			if (GPUBlt) {
				gpuBltEffect->EndPass();
				gpuBltEffect->End();
			}
			d3d9Device->EndScene();
			Present();
		}
		return DD_OK;
	}

	HRESULT _stdcall BltBatch(LPDDBLTBATCH, DWORD, DWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall BltFast(DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
	HRESULT _stdcall EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT _stdcall EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT _stdcall Flip(LPDIRECTDRAWSURFACE, DWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetBltStatus(DWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetCaps(LPDDSCAPS) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetClipper(LPDIRECTDRAWCLIPPER *) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetDC(HDC *) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetFlipStatus(DWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetOverlayPosition(LPLONG, LPLONG) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetPalette(LPDIRECTDRAWPALETTE *) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetPixelFormat(LPDDPIXELFORMAT) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetSurfaceDesc(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT _stdcall Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT _stdcall IsLost() { UNUSEDFUNCTION; }

	HRESULT _stdcall Lock(LPRECT a, LPDDSURFACEDESC b, DWORD c, HANDLE d) {
		*b = (Primary) ? surfaceDesc : movieDesc;
		b->lpSurface = lockTarget;
		return DD_OK;
	}

	HRESULT _stdcall ReleaseDC(HDC) { UNUSEDFUNCTION; }
	HRESULT _stdcall Restore() { UNUSEDFUNCTION; } // call from fallout2.exe - 0x4CB907
	HRESULT _stdcall SetClipper(LPDIRECTDRAWCLIPPER) { UNUSEDFUNCTION; }
	HRESULT _stdcall SetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT _stdcall SetOverlayPosition(LONG, LONG) { UNUSEDFUNCTION; }
	HRESULT _stdcall SetPalette(LPDIRECTDRAWPALETTE) { return DD_OK; }

#define FASTCOPY(a) __asm {                    \
	_asm movzx eax, byte ptr ds:[esi]          \
	_asm mov eax, dword ptr ds:[ebx + eax * 4] \
	_asm inc esi                               \
	_asm mov dword ptr ds:[edi + a], eax       \
}

	HRESULT _stdcall Unlock(LPVOID) { // common game (is primary)
		if (Primary && d3d9Device) {
			if (DeviceLost) {
				if (d3d9Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
					ResetDevice(false);
					DeviceLost = false;
				}
			}
			if (!DeviceLost) {
				D3DLOCKED_RECT dRect;
				Tex->LockRect(0, &dRect, 0, 0);
				int pitch = dRect.Pitch;
				DWORD width = ResWidth;
				if (GPUBlt) {
					char* pBits = (char*)dRect.pBits;
					if (IsPlayMovie) { // for subtitles
						subTitlesShow = true;
						if (!titlesBuffer) titlesBuffer = new BYTE[ResWidth * ResHeight]();
						DWORD bottom = (yoffset + movieHeight);
						for (DWORD y = 0; y < ResHeight; y++) {
							if (y < yoffset || y > bottom) CopyMemory(&titlesBuffer[y * width], &lockTarget[y * width], width); //copy subtitles region to buffer
						}
					} else {
						for (DWORD y = 0; y < ResHeight; y++) {
							CopyMemory(&pBits[y * pitch], &lockTarget[y * width], width);
						}
					}
				} else {
					DWORD* pBits = (DWORD*)dRect.pBits;
					pitch /= 4;
					if (IsPlayMovie) { // for subtitles
						subTitlesShow = true;
						if (!titlesBuffer) titlesBuffer = new BYTE[ResWidth * ResHeight]();
						DWORD bottom = (yoffset + movieHeight);
						for (DWORD y = 0; y < ResHeight; y++) {
							if (y >= yoffset && y <= bottom) continue;
							int yw = y * width;
							for (DWORD x = 0; x < width; x++) {
								titlesBuffer[yw + x] = lockTarget[yw + x];
							}
						}
					} else if (!(ResWidth % 8)) {
						DWORD* target = (DWORD*)&lockTarget[0];
						pitch = (pitch - ResWidth) * 4;
						DWORD width = ResWidth / 8;
						__asm {
							mov esi, target;
							mov edi, dRect.pBits;
							lea ebx, [palette];
							mov edx, ResHeight;
start:
							mov ecx, width;
start2:
							movzx eax, byte ptr ds:[esi];
							mov eax, dword ptr ds:[ebx + eax * 4];
							inc esi;
							mov dword ptr ds:[edi], eax;
							FASTCOPY(4)
							FASTCOPY(8)
							FASTCOPY(12)
							FASTCOPY(16)
							FASTCOPY(20)
							FASTCOPY(24)
							FASTCOPY(28)
							lea edi, [edi + 32];

							dec ecx;
							jnz start2;
							add edi, pitch;
							dec edx;
							jnz start;
						}
					} else {
						for (DWORD y = 0; y < ResHeight; y++) {
							int yp = y * pitch;
							int yw = y * width;
							for (DWORD x = 0; x < width; x++) {
								pBits[yp + x] = palette[lockTarget[yw + x]];
							}
						}
					}
				}
				Tex->UnlockRect(0);
				if (!IsPlayMovie) {
					subTitlesShow = false;
					RefreshGraphics();
				};
			}
			IsPlayMovie = false;
		}
		return DD_OK;
	}

	HRESULT _stdcall UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) { UNUSEDFUNCTION; }
	HRESULT _stdcall UpdateOverlayDisplay(DWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
};

bool FakeSurface2::IsPlayMovie;
bool FakeSurface2::subTitlesShow;

class FakeDirectDraw2 : IDirectDraw
{
private:
	ULONG Refs;
public:
	FakeDirectDraw2() {
		Refs=1;
	}

	// IUnknown methods
	HRESULT _stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }
	ULONG _stdcall AddRef()  { return ++Refs; }

	ULONG _stdcall Release() {
		if (!--Refs) {
			globalShaderActive = false;
			GraphicsResetOnGameLoad();
			for (DWORD d = 0; d < shaderTextures.size(); d++) shaderTextures[d]->Release();
			shaderTextures.clear();
			SAFERELEASE(backbuffer);
			SAFERELEASE(sSurf1);
			SAFERELEASE(sSurf2);
			SAFERELEASE(Tex);
			SAFERELEASE(sTex1);
			SAFERELEASE(sTex2);
			SAFERELEASE(vBuffer);
			SAFERELEASE(vBuffer2);
			SAFERELEASE(d3d9Device);
			SAFERELEASE(d3d9);
			SAFERELEASE(gpuPalette);
			SAFERELEASE(gpuBltEffect);
			SAFERELEASE(movieBuffer);
			SAFERELEASE(movieTex);
			delete this;
			return 0;
		} else return Refs;
	}

	// IDirectDraw methods
	HRESULT _stdcall Compact() { UNUSEDFUNCTION; }
	HRESULT _stdcall CreateClipper(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) { UNUSEDFUNCTION; }

	HRESULT _stdcall CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE* c, IUnknown*) {
		*c = (IDirectDrawPalette*)new FakePalette2();
		return DD_OK;
	}

	HRESULT _stdcall CreateSurface(LPDDSURFACEDESC a, LPDIRECTDRAWSURFACE* b, IUnknown* c) {
		if (a->dwFlags == 1 && a->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE)
			*b = (IDirectDrawSurface*)new FakeSurface2(true);
		else
			*b = (IDirectDrawSurface*)new FakeSurface2(false);

		return DD_OK;
	}

	HRESULT _stdcall DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT _stdcall EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT _stdcall EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT _stdcall FlipToGDISurface() { UNUSEDFUNCTION; }
	HRESULT _stdcall GetCaps(LPDDCAPS, LPDDCAPS b) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetDisplayMode(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetFourCCCodes(LPDWORD,LPDWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetGDISurface(LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetMonitorFrequency(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetScanLine(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT _stdcall GetVerticalBlankStatus(LPBOOL) { UNUSEDFUNCTION; }
	HRESULT _stdcall Initialize(GUID *) { UNUSEDFUNCTION; }
	HRESULT _stdcall RestoreDisplayMode() { return DD_OK; }

	HRESULT _stdcall SetCooperativeLevel(HWND a, DWORD b) {
		window = a;

		if (!d3d9Device) {
			ResetDevice(true);
			CoInitialize(0);
		}
		dlog("Creating D3D9 Device window...", DL_MAIN);

		if (GraphicsMode == 5) {
			SetWindowLong(a, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_BORDER);
			RECT r;
			r.left = 0;
			r.right = gWidth;
			r.top = 0;
			r.bottom = gHeight;
			AdjustWindowRect(&r, WS_OVERLAPPED | WS_CAPTION | WS_BORDER, false);
			r.right -= r.left;
			r.left = 0;
			r.bottom -= r.top;
			r.top = 0;
			SetWindowPos(a, HWND_NOTOPMOST, 0, 0, r.right, r.bottom, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}

		dlogr(" Done.", DL_MAIN);
		return DD_OK;
	}

	HRESULT _stdcall SetDisplayMode(DWORD, DWORD, DWORD) { return DD_OK; }
	HRESULT _stdcall WaitForVerticalBlank(DWORD, HANDLE) { UNUSEDFUNCTION; }
};

HRESULT _stdcall FakeDirectDrawCreate2(void*, IDirectDraw** b, void*) {
	dlog("Initializing Direct3D...", DL_MAIN);

	ResWidth = *(DWORD*)0x4CAD6B;  // 640
	ResHeight = *(DWORD*)0x4CAD66; // 480

	if (!d3d9) {
		d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	}

	ZeroMemory(&surfaceDesc, sizeof(DDSURFACEDESC));
	surfaceDesc.dwSize = sizeof(DDSURFACEDESC);
	surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH;
	surfaceDesc.dwWidth = ResWidth;
	surfaceDesc.dwHeight = ResHeight;
	surfaceDesc.ddpfPixelFormat.dwRGBBitCount = 16;
	surfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	surfaceDesc.ddpfPixelFormat.dwRBitMask = 0xf800;
	surfaceDesc.ddpfPixelFormat.dwGBitMask = 0x7e0;
	surfaceDesc.ddpfPixelFormat.dwBBitMask = 0x1f;
	surfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	surfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	surfaceDesc.lPitch = ResWidth;
	movieDesc = surfaceDesc;
	movieDesc.lPitch = 640;
	movieDesc.dwHeight = 480;
	movieDesc.dwWidth = 640;

	gWidth = GetConfigInt("Graphics", "GraphicsWidth", 0);
	gHeight = GetConfigInt("Graphics", "GraphicsHeight", 0);
	if (!gWidth || !gHeight) {
		gWidth = ResWidth;
		gHeight = ResHeight;
	}
	GPUBlt = GetConfigInt("Graphics", "GPUBlt", 0);
	if (!GPUBlt || GPUBlt > 2) GPUBlt = 2; // Swap them around to keep compatibility with old ddraw.ini's
	else if (GPUBlt == 2) GPUBlt = 0;      // Use CPU

	if (GraphicsMode == 5) {
		ScrollWindowKey = GetConfigInt("Input", "WindowScrollKey", 0);
	} else ScrollWindowKey = 0;

	rcpres[0] = 1.0f / (float)gWidth;
	rcpres[1] = 1.0f / (float)gHeight;

	*b = (IDirectDraw*)new FakeDirectDraw2();

	dlogr(" Done.", DL_MAIN);
	return DD_OK;
}

static __declspec(naked) void game_init_hook() {
	__asm {
		push ecx;
		call WindowInit;
		pop  ecx;
		jmp  palette_init_;
	}
}

static double fadeMulti;
static __declspec(naked) void palette_fade_to_hook() {
	__asm {
		push ebx; // _fade_steps
		fild [esp];
		fmul fadeMulti;
		fistp [esp];
		pop  ebx;
		jmp  fadeSystemPalette_;
	}
}

void GraphicsInit() {
	GraphicsMode = GetConfigInt("Graphics", "Mode", 0);
	if (GraphicsMode != 4 && GraphicsMode != 5) {
		GraphicsMode = 0;
	}
	if (GraphicsMode) {
		dlog("Applying DX9 graphics patch.", DL_INIT);
#ifdef WIN2K
#define _DLL_NAME "d3dx9_42.dll"
#else
#define _DLL_NAME "d3dx9_43.dll"
#endif
		HMODULE h = LoadLibraryEx(_DLL_NAME, 0, LOAD_LIBRARY_AS_DATAFILE);
		if (!h) {
			MessageBoxA(0, "You have selected graphics mode 4 or 5, but " _DLL_NAME " is missing.\n"
						   "Switch back to mode 0, or install an up to date version of DirectX.", "Error", MB_TASKMODAL | MB_ICONERROR);
			ExitProcess(-1);
		} else {
			FreeLibrary(h);
		}
#undef _DLL_NAME
		SafeWrite8(0x50FB6B, '2'); // Set call DirectDrawCreate2
		HookCall(0x44260C, game_init_hook);
		dlogr(" Done", DL_INIT);

		gShaderFile = GetConfigString("Graphics", "GlobalShaderFile", "", MAX_PATH);
		globalShaderActive = (gShaderFile.length() > 3);
	}

	fadeMulti = GetConfigInt("Graphics", "FadeMultiplier", 100);
	if (fadeMulti != 100) {
		dlog("Applying fade patch.", DL_INIT);
		HookCall(0x493B16, palette_fade_to_hook);
		fadeMulti = ((double)fadeMulti) / 100.0;
		dlogr(" Done", DL_INIT);
	}
}

void GraphicsExit() {
	if (GraphicsMode) {
		if (titlesBuffer) delete[] titlesBuffer;
		CoUninitialize();
	}
}
