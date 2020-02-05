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
#define DEBUGMESS(a, b) MessageBoxA(0, "Unused function called.", b, MB_TASKMODAL) //fo::func::debug_printf(a, b)
#else
#define DEBUGMESS(a, b)
#endif
#endif


#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Version.h"
#include "LoadGameHook.h"
#include "ScriptShaders.h"

#include "Graphics.h"

namespace sfall
{

typedef HRESULT (_stdcall *DDrawCreateProc)(void*, IDirectDraw**, void*);
typedef IDirect3D9* (_stdcall *D3DCreateProc)(UINT version);

#define UNUSEDFUNCTION { DEBUGMESS("\n[SFALL] Unused function called: %s", __FUNCTION__); return DDERR_GENERIC; }

IDirectDrawSurface* primaryDDSurface = nullptr;

static DWORD ResWidth;
static DWORD ResHeight;

DWORD Graphics::GPUBlt;
DWORD Graphics::mode;

bool Graphics::PlayAviMovie = false;

static BYTE* titlesBuffer = nullptr;

static DWORD yoffset, movieHeight = 0;
//static DWORD xoffset, movieWidth = 0;

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
static HWND window;

static DWORD ShaderVersion;

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
static const char* gpuEffect =
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
	    "result = tex1D(s1, backdrop);"
	    "result = float3(result.b, result.g, result.r);"
	  "}"
	// blend highlights
	"if (showhl) {"
		"float4 h = tex2D(s3, saturate((Tex - cornerhl) / sizehl));"
		"result = saturate(result + h.rgb);" // saturate(result * (1 - h.a) * h.rgb * h.a)"
	"}"
	  "return float4(result.r, result.g, result.b, 1);"
	"}"

	"technique T1"
	"{"
	  "pass p1 { PixelShader = compile ps_2_0 P1(); }"
	"}"

	"float4 P0( in float2 Tex : TEXCOORD0 ) : COLOR0 {"
	  "float3 result = tex1D(s1, tex2D(s0, Tex).a);"
	  "return float4(result.b, result.g, result.r, 1);" // GPU swap R <> B
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
static D3DXHANDLE gpuBltHighlight;
static D3DXHANDLE gpuBltHighlightSize;
static D3DXHANDLE gpuBltHighlightCorner;
static D3DXHANDLE gpuBltShowHighlight;

static float rcpres[2];

#define _VERTEXFORMAT D3DFVF_XYZRHW|D3DFVF_TEX1

struct VertexFormat {
	float x, y, z, w, u, v;
};

static VertexFormat ShaderVertices[] = {
	// x      y    z rhw u  v
	{-0.5,  -0.5,  0, 1, 0, 0},
	{-0.5,  479.5, 0, 1, 0, 1},
	{639.5, -0.5,  0, 1, 1, 0},
	{639.5, 479.5, 0, 1, 1, 1}
};

HWND GetFalloutWindowInfo(RECT* rect) {
	if (rect) {
		rect->left = windowLeft;
		rect->top = windowTop;
		rect->right = gWidth;
		rect->bottom = gHeight;
	}
	return window;
}

long Graphics::GetGameWidthRes() {
	return (fo::var::scr_size.offx - fo::var::scr_size.x) + 1;
}

long Graphics::GetGameHeightRes() {
	return (fo::var::scr_size.offy - fo::var::scr_size.y) + 1;
}

int _stdcall GetShaderVersion() {
	return ShaderVersion;
}

static void WindowInit() {
	windowInit = true;
	rcpres[0] = 1.0f / (float)Graphics::GetGameWidthRes();
	rcpres[1] = 1.0f / (float)Graphics::GetGameHeightRes();
	ScriptShaders::LoadGlobalShader();
}

const float* Graphics::rcpresGet() {
	return rcpres;
}

static void GetDisplayMode(D3DDISPLAYMODE &ddm) {
	ZeroMemory(&ddm, sizeof(ddm));
	d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm);
	dlog_f("Display mode format id: %d\n", DL_INIT, ddm.Format);
}

static void ResetDevice(bool createNew) {
	D3DPRESENT_PARAMETERS params;
	ZeroMemory(&params, sizeof(params));

	D3DDISPLAYMODE dispMode;
	GetDisplayMode(dispMode);

	params.BackBufferCount = 1;
	params.BackBufferFormat = dispMode.Format; // (Graphics::mode == 5) ? D3DFMT_UNKNOWN : D3DFMT_X8R8G8B8;
	params.BackBufferWidth = gWidth;
	params.BackBufferHeight = gHeight;
	params.EnableAutoDepthStencil = false;
	//params.MultiSampleQuality = 0;
	//params.MultiSampleType = D3DMULTISAMPLE_NONE;
	params.Windowed = (Graphics::mode == 5);
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (!params.Windowed) params.FullScreen_RefreshRateInHz = dispMode.RefreshRate;

	bool software = false;
	if (createNew) {
		dlog("Creating D3D9 Device...", DL_MAIN);
		if (FAILED(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &params, &d3d9Device))) {
			dlog(" Failed to create D3D9 Device. Use software vertex processing.", DL_MAIN);
			software = true;
			d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE, &params, &d3d9Device);
		}

		D3DCAPS9 caps;
		d3d9Device->GetDeviceCaps(&caps);
		ShaderVersion = ((caps.PixelShaderVersion & 0x0000FF00) >> 8) * 10 + (caps.PixelShaderVersion & 0xFF);

		// Use: 0 - only CPU, 1 - force GPU, 2 - Auto Mode (GPU or switch to CPU)
		if (Graphics::GPUBlt == 2 && ShaderVersion < 20) Graphics::GPUBlt = 0;

		if (Graphics::GPUBlt) {
			D3DXCreateEffect(d3d9Device, gpuEffect, strlen(gpuEffect), 0, 0, 0, 0, &gpuBltEffect, 0);
			gpuBltBuf = gpuBltEffect->GetParameterByName(0, "image");
			gpuBltPalette = gpuBltEffect->GetParameterByName(0, "palette");
			// for head textures
			gpuBltHead = gpuBltEffect->GetParameterByName(0, "head");
			gpuBltHeadSize = gpuBltEffect->GetParameterByName(0, "size");
			gpuBltHeadCorner = gpuBltEffect->GetParameterByName(0, "corner");
			gpuBltHighlight = gpuBltEffect->GetParameterByName(0, "highlight");
			gpuBltHighlightSize = gpuBltEffect->GetParameterByName(0, "sizehl");
			gpuBltHighlightCorner = gpuBltEffect->GetParameterByName(0, "cornerhl");
			gpuBltShowHighlight = gpuBltEffect->GetParameterByName(0, "showhl");

			Graphics::SetDefaultTechnique();
		}
	} else {
		d3d9Device->Reset(&params);
		if (gpuBltEffect) gpuBltEffect->OnResetDevice();
		ScriptShaders::OnResetDevice();
	}

	ShaderVertices[1].y = ResHeight - 0.5f;
	ShaderVertices[2].x = ResWidth - 0.5f;
	ShaderVertices[3].y = ResHeight - 0.5f;
	ShaderVertices[3].x = ResWidth - 0.5f;
	if (d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, Graphics::GPUBlt ? D3DFMT_A8 : D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &Tex, 0) != D3D_OK) {
		d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &Tex, 0);
		Graphics::GPUBlt = 0;
		dlog(" Error: D3DFMT_A8 unsupported texture format. Now CPU is used to convert the palette.", DL_MAIN);
	}
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex1, 0);
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex2, 0);
	if (Graphics::GPUBlt) {
		d3d9Device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &gpuPalette, 0);
		gpuBltEffect->SetTexture(gpuBltBuf, Tex);
		gpuBltEffect->SetTexture(gpuBltPalette, gpuPalette);
	}

	sTex1->GetSurfaceLevel(0, &sSurf1);
	sTex2->GetSurfaceLevel(0, &sSurf2);
	d3d9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vBuffer, 0);
	byte* VertexPointer;
	vBuffer->Lock(0, 0, (void**)&VertexPointer, 0);
	CopyMemory(VertexPointer, ShaderVertices, sizeof(ShaderVertices));
	vBuffer->Unlock();

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &movieBuffer, 0);

	VertexFormat ShaderVertices2[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	ShaderVertices2[1].y = (float)gHeight - 0.5f;
	ShaderVertices2[2].x = (float)gWidth - 0.5f;
	ShaderVertices2[3].y = (float)gHeight - 0.5f;
	ShaderVertices2[3].x = (float)gWidth - 0.5f;

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vBuffer2, 0);
	vBuffer2->Lock(0, 0, (void**)&VertexPointer, 0);
	CopyMemory(VertexPointer, ShaderVertices2, sizeof(ShaderVertices2));
	vBuffer2->Unlock();

	d3d9Device->SetFVF(_VERTEXFORMAT);
	d3d9Device->SetTexture(0, Tex);
	d3d9Device->SetStreamSource(0, vBuffer, 0, sizeof(VertexFormat));

	//d3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false); // default false
	//d3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);  // default false
	d3d9Device->SetRenderState(D3DRS_ZENABLE, false);
	d3d9Device->SetRenderState(D3DRS_CULLMODE, 2);
	//d3d9Device->SetRenderState(D3DRS_TEXTUREFACTOR, 0);

	if (createNew) dlogr(" Done", DL_MAIN);
}

static void Present() {
	if (ScrollWindowKey != 0 && ((ScrollWindowKey > 0 && KeyDown((BYTE)ScrollWindowKey))
		|| (ScrollWindowKey == -1 && (KeyDown(DIK_LCONTROL) || KeyDown(DIK_RCONTROL)))
		|| (ScrollWindowKey == -2 && (KeyDown(DIK_LMENU)    || KeyDown(DIK_RMENU)))
		|| (ScrollWindowKey == -3 && (KeyDown(DIK_LSHIFT)   || KeyDown(DIK_RSHIFT)))))
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
		#ifndef NDEBUG
		dlogr("Present: DEVICELOST", DL_MAIN);
		#endif
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
		ScriptShaders::OnLostDevice();
		DeviceLost = true;
	}
}

void RefreshGraphics() {
	if (DeviceLost) return;

	d3d9Device->BeginScene();
	d3d9Device->SetStreamSource(0, vBuffer, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, sSurf1);

	UINT unused;
	if (Graphics::GPUBlt && ScriptShaders::Count()) {
		gpuBltEffect->Begin(&unused, 0);
		gpuBltEffect->BeginPass(0);
		d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		gpuBltEffect->EndPass();
		gpuBltEffect->End();

		d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE); // copy: sSurf1 to sSurf2
		d3d9Device->SetTexture(0, sTex2);
	} else {
		d3d9Device->SetTexture(0, Tex);
	}
	ScriptShaders::Refresh(sSurf1, sSurf2, sTex2);

	d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, backbuffer);
	if (Graphics::GPUBlt && !ScriptShaders::Count()) {
		gpuBltEffect->Begin(&unused, 0);
		gpuBltEffect->BeginPass(0);
	}
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (Graphics::GPUBlt && !ScriptShaders::Count()) {
		gpuBltEffect->EndPass();
		gpuBltEffect->End();
	}

	d3d9Device->EndScene();
	Present();
}

void Graphics::SetMovieTexture(IDirect3DTexture9* tex) {
	dlog("\nSet movie texture.", DL_INIT);
	movieTex = tex;
	if (!tex) {
		PlayAviMovie = false;
		return;
	}

	D3DSURFACE_DESC desc;
	movieTex->GetLevelDesc(0, &desc);

	float aspect = (float)desc.Width / (float)desc.Height;
	float winaspect = (float)gWidth / (float)gHeight;

	VertexFormat ShaderVertices2[4] = {
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

	byte* VertexPointer;
	movieBuffer->Lock(0, 0, (void**)&VertexPointer, 0);
	CopyMemory(VertexPointer, ShaderVertices2, sizeof(ShaderVertices2));
	movieBuffer->Unlock();

	PlayAviMovie = true;
}

void Graphics::ShowMovieFrame() {
	//d3d9Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 255), 1.0f, 0); // for debbuging
	d3d9Device->BeginScene();

	if (Graphics::GPUBlt && ScriptShaders::Count()) {
		d3d9Device->SetTexture(0, sTex2);
	} else {
		d3d9Device->SetTexture(0, Tex);
	}
	d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, backbuffer);

	if (Graphics::GPUBlt /*&& !ScriptShaders::Count()*/) {
		UINT unused;
		gpuBltEffect->Begin(&unused, 0);
		gpuBltEffect->BeginPass(0);
	}
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (Graphics::GPUBlt /*&& !ScriptShaders::Count()*/) {
		gpuBltEffect->EndPass();
		gpuBltEffect->End();
	}

	// for movie
	d3d9Device->SetTexture(0, movieTex);
	d3d9Device->SetStreamSource(0, movieBuffer, 0, sizeof(VertexFormat));
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	d3d9Device->EndScene();
	Present();
}

void Graphics::SetHighlightTexture(IDirect3DTexture9* htex) {
	gpuBltEffect->SetTexture(gpuBltHighlight, htex);

	float size[2];
	size[0] = 388.0f * rcpres[0];
	size[1] = 200.0f * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHighlightSize, size, 2);

	int xPos = ((GetGameWidthRes() - 640) / 2);
	size[0] = (126.0f + xPos) * rcpres[0];
	int h = GetGameHeightRes();
	int yPos = (h > 480) ? ((h - 480) / 2) - 33 : 14;
	size[1] = (float)yPos * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHighlightCorner, size, 2);
}

void Graphics::SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff, int showHighlight) {
	gpuBltEffect->SetInt(gpuBltShowHighlight, showHighlight);
	gpuBltEffect->SetTexture(gpuBltHead, tex);

	float size[2];
	size[0] = (float)width * rcpres[0];
	size[1] = (float)height * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadSize, size, 2);

	// adjust head texture position for HRP 4.1.8
	int h = GetGameHeightRes();
	if (h > 480) yoff += ((h - 480) / 2) - 47;
	xoff += ((GetGameWidthRes() - 640) / 2);

	size[0] = (126.0f + xoff + ((388 - width) / 2)) * rcpres[0];
	size[1] = (14.0f + yoff + ((200 - height) / 2)) * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadCorner, size, 2);

	SetHeadTechnique();
}

void Graphics::SetHeadTechnique() {
	gpuBltEffect->SetTechnique("T1");
}

void Graphics::SetDefaultTechnique() {
	gpuBltEffect->SetTechnique("T0");
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

	/* Called from:
		0x4CB5C7 GNW95_SetPalette_
		0x4CB36B GNW95_SetPaletteEntries_
	*/
	HRESULT _stdcall SetEntries(DWORD a, DWORD b, DWORD c, LPPALETTEENTRY destPal) { // used to set palette for splash screen, fades, subtitles
		if (!windowInit || c == 0 || b + c > 256) return DDERR_INVALIDPARAMS;

		CopyMemory(&palette[b], destPal, c * 4);
		if (Graphics::GPUBlt) {
			if (gpuPalette) {
				D3DLOCKED_RECT rect;
				if (!FAILED(gpuPalette->LockRect(0, &rect, 0, D3DLOCK_DISCARD))) {
					CopyMemory(rect.pBits, palette, 256 * 4);
					gpuPalette->UnlockRect(0);
				}
			}
		} else {
			// X8B8G8R8 format
			for (DWORD i = b; i < b + c; i++) { // swap color R <> B
				BYTE clr = *(BYTE*)((DWORD)&palette[i]); // B
				*(BYTE*)((DWORD)&palette[i]) = *(BYTE*)((DWORD)&palette[i] + 2); // R
				*(BYTE*)((DWORD)&palette[i] + 2) = clr;
			}
			primaryDDSurface->SetPalette(0); // update
		}
		if (!Graphics::PlayAviMovie) {
			RefreshGraphics();
		} else {
			Graphics::ShowMovieFrame();
		}
		return DD_OK;
	}
};

class FakeSurface2 : IDirectDrawSurface {
private:
	ULONG Refs;
	bool Primary;
	BYTE* lockTarget;

public:
	static bool IsPlayMovie;
	static bool subTitlesShow;

	FakeSurface2(bool primary) {
		Refs = 1;
		Primary = primary;
		lockTarget = new BYTE[ResWidth * ResHeight];
		if (!windowInit) std::memset(lockTarget, 0, ResWidth * ResHeight);
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

	/*
		0x4868DA movie_MVE_ShowFrame_
	*/
	HRESULT _stdcall Blt(LPRECT a, LPDIRECTDRAWSURFACE b, LPRECT c, DWORD d, LPDDBLTFX e) { // used for game movies (w/o HRP)
		//long addrs;
		//__asm mov eax, dword ptr [ebp + 4];
		//__asm mov addrs, eax;
		//dlog_f("\nBlt(0x%x)", DL_INIT, addrs);

		IsPlayMovie = true;
		if (Graphics::PlayAviMovie) return DD_OK;

		//movieWidth = (a->right - a->left);
		//xoffset = (ResWidth - movieWidth) / 2;
		//movieHeight = (a->bottom - a->top);
		yoffset = (ResHeight - movieDesc.dwHeight) / 2;

		BYTE* lockTarget = ((FakeSurface2*)b)->lockTarget;
		D3DLOCKED_RECT dRect;
		Tex->LockRect(0, &dRect, a, 0);
		DWORD width = movieDesc.lPitch; // the current size of the width of the mve movie //ResWidth;
		int pitch = dRect.Pitch;
		if (Graphics::GPUBlt) {
			char* pBits = (char*)dRect.pBits;
			if (subTitlesShow) {
				subTitlesShow = false;
				DWORD bottom = yoffset + movieDesc.dwHeight;
				long x_shift = (ResWidth - width) / 2; // shift the position of subtitles relative to the width difference
				for (DWORD y = 0; y < ResHeight; y++) {
					if (y < yoffset || y > bottom) { // paste subtitles excluding video region
						CopyMemory(&pBits[(y - yoffset) * pitch], &titlesBuffer[(y * ResWidth) + x_shift], width);
					}
				}
			}
			for (DWORD y = 0; y < movieDesc.dwHeight; y++) {
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
				DWORD bottom = yoffset + movieDesc.dwHeight;
				long x_shift = (ResWidth - width) / 2; // shift the position of subtitles relative to the width difference
				for (DWORD y = 0; y < ResHeight; y++) {
					if (y < yoffset || y > bottom) {
						int yyp = (y - yoffset) * pitch;
						int yw = (y * ResWidth) + x_shift;
						for (DWORD x = 0; x < width; x++) {
							((DWORD*)dRect.pBits)[yyp + x] = palette[titlesBuffer[yw + x]];
						}
					}
				}
			}
			for (DWORD y = 0; y < movieDesc.dwHeight; y++) {
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
			d3d9Device->SetStreamSource(0, vBuffer2, 0, sizeof(VertexFormat));
			d3d9Device->SetTexture(0, Tex);
			d3d9Device->BeginScene();
			if (Graphics::GPUBlt) {
				UINT unused;
				gpuBltEffect->Begin(&unused, 0);
				gpuBltEffect->BeginPass(0);
			}
			d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			if (Graphics::GPUBlt) {
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

	/* Called from:
		0x4CB887 GNW95_ShowRect_ (c=1)
		0x48699D movieShowFrame_ (c=0)
		0x4CBBFA GNW95_zero_vid_mem_ (c=1)
		0x4F5E91/0x4F5EBB sub_4F5E60 (c=0)
	*/
	HRESULT _stdcall Lock(LPRECT a, LPDDSURFACEDESC b, DWORD c, HANDLE d) {
		if (Primary) {
			//dlog_f("\nLock(%d) use surfaceDesc.", DL_INIT, c);
			*b = surfaceDesc;
		} else {
			//dlog_f("\nLock(%d) use movieDesc.", DL_INIT, c);
			movieDesc.lPitch = *(DWORD*)FO_VAR_lastMovieW;
			movieDesc.dwHeight = *(DWORD*)FO_VAR_lastMovieH;
			*b = movieDesc;
		}
		b->lpSurface = lockTarget;
		return DD_OK;
	}

	HRESULT _stdcall ReleaseDC(HDC) { UNUSEDFUNCTION; }
	HRESULT _stdcall Restore() { UNUSEDFUNCTION; } // call from fallout2.exe - 0x4CB907
	HRESULT _stdcall SetClipper(LPDIRECTDRAWCLIPPER) { UNUSEDFUNCTION; }
	HRESULT _stdcall SetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT _stdcall SetOverlayPosition(LONG, LONG) { UNUSEDFUNCTION; }

	HRESULT _stdcall SetPalette(LPDIRECTDRAWPALETTE a) {
		if (a) return DD_OK; // prevents executing the function when called from outside of sfall

		D3DLOCKED_RECT dRect;
		Tex->LockRect(0, &dRect, 0, 0);

		DWORD* pBits = (DWORD*)dRect.pBits;
		int pitch = dRect.Pitch / 4;
		DWORD width = ResWidth;

		for (DWORD y = 0; y < ResHeight; y++) {
			int yp = y * pitch;
			int yw = y * width;
			for (DWORD x = 0; x < width; x++) {
				pBits[yp + x] = palette[lockTarget[yw + x]];
			}
		}
		Tex->UnlockRect(0);
		return DD_OK;
	}

#define FASTCOPY(a) __asm {                    \
	_asm movzx eax, byte ptr ds:[esi]          \
	_asm mov eax, dword ptr ds:[ebx + eax * 4] \
	_asm inc esi                               \
	_asm mov dword ptr ds:[edi + a], eax       \
}

	HRESULT _stdcall Unlock(LPVOID) { // common game (is primary)
		//dlog("\nUnlock()", DL_INIT);
		if (Primary && d3d9Device) {
			//dlog(" is primary.", DL_INIT);
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
				if (Graphics::GPUBlt) {
					char* pBits = (char*)dRect.pBits;
					if (IsPlayMovie) { // for subtitles
						subTitlesShow = true;
						if (!titlesBuffer) titlesBuffer = new BYTE[ResWidth * ResHeight]();
						DWORD bottom = (yoffset + movieDesc.dwHeight);
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
						DWORD bottom = (yoffset + movieDesc.dwHeight);
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
				if (!IsPlayMovie && !Graphics::PlayAviMovie) {
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

	ULONG _stdcall Release() { // called from game on exit
		if (!--Refs) {
			ScriptShaders::Release();
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
		//dlog("\nCreateSurface", DL_INIT);
		if (a->dwFlags == 1 && a->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE) {
			//dlog(" primary.", DL_INIT);
			*b = primaryDDSurface = (IDirectDrawSurface*)new FakeSurface2(true);
		} else {
			*b = (IDirectDrawSurface*)new FakeSurface2(false);
		}
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
			CoInitialize(0);
			ResetDevice(true); // create
		}
		dlog("Creating D3D9 Device window...", DL_MAIN);

		if (Graphics::mode == 5) {
			SetWindowLong(a, GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX);
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
			SetWindowPos(a, HWND_NOTOPMOST, r.left, r.top, r.right, r.bottom, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}

		dlogr(" Done", DL_MAIN);
		return DD_OK;
	}

	HRESULT _stdcall SetDisplayMode(DWORD, DWORD, DWORD) { return DD_OK; }
	HRESULT _stdcall WaitForVerticalBlank(DWORD, HANDLE) { UNUSEDFUNCTION; }
};

HRESULT _stdcall FakeDirectDrawCreate2_Init(void*, IDirectDraw** b, void*) {
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
	surfaceDesc.ddpfPixelFormat.dwRBitMask = 0xF800; // 1111100000000000
	surfaceDesc.ddpfPixelFormat.dwGBitMask = 0x7E0;  // 0000011111100000
	surfaceDesc.ddpfPixelFormat.dwBBitMask = 0x1F;   // 0000000000011111
	surfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	surfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	surfaceDesc.lPitch = ResWidth;

	// set params for .mve surface
	movieDesc = surfaceDesc;
	movieDesc.lPitch = 640;
	movieDesc.dwWidth = 640;
	movieDesc.dwHeight = 480;

	gWidth = GetConfigInt("Graphics", "GraphicsWidth", 0);
	gHeight = GetConfigInt("Graphics", "GraphicsHeight", 0);
	if (!gWidth || !gHeight) {
		gWidth = ResWidth;
		gHeight = ResHeight;
	}

	Graphics::GPUBlt = GetConfigInt("Graphics", "GPUBlt", 0); // 0 - auto, 1 - GPU, 2 - CPU
	if (!Graphics::GPUBlt || Graphics::GPUBlt > 2)
		Graphics::GPUBlt = 2; // Swap them around to keep compatibility with old ddraw.ini
	else if (Graphics::GPUBlt == 2) Graphics::GPUBlt = 0; // Use CPU

	if (Graphics::mode == 5) {
		ScrollWindowKey = GetConfigInt("Input", "WindowScrollKey", 0);
	} else ScrollWindowKey = 0;

	rcpres[0] = 1.0f / (float)gWidth;
	rcpres[1] = 1.0f / (float)gHeight;

	*b = (IDirectDraw*)new FakeDirectDraw2();

	dlogr(" Done", DL_MAIN);
	return DD_OK;
}

static __declspec(naked) void game_init_hook() {
	__asm {
		push ecx;
		call WindowInit;
		pop  ecx;
		jmp  fo::funcoffs::palette_init_;
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
		jmp  fo::funcoffs::fadeSystemPalette_;
	}
}

void Graphics::init() {
	Graphics::mode = GetConfigInt("Graphics", "Mode", 0);
	if (Graphics::mode != 4 && Graphics::mode != 5) {
		Graphics::mode = 0;
	}
	if (Graphics::mode) {
		dlog("Applying DX9 graphics patch.", DL_INIT);
#define _DLL_NAME "d3dx9_43.dll"
		HMODULE h = LoadLibraryEx(_DLL_NAME, 0, LOAD_LIBRARY_AS_DATAFILE);
		if (!h) {
			MessageBoxA(0, "You have selected graphics mode 4 or 5, but " _DLL_NAME " is missing.\n"
						   "Switch back to mode 0, or install an up to date version of DirectX.", "Error", MB_TASKMODAL | MB_ICONERROR);
#undef _DLL_NAME
			ExitProcess(-1);
		} else {
			FreeLibrary(h);
		}
		SafeWrite8(0x50FB6B, '2'); // Set call DirectDrawCreate2
		HookCall(0x44260C, game_init_hook);
		dlogr(" Done", DL_INIT);
	}

	fadeMulti = GetConfigInt("Graphics", "FadeMultiplier", 100);
	if (fadeMulti != 100) {
		dlog("Applying fade patch.", DL_INIT);
		HookCall(0x493B16, palette_fade_to_hook);
		fadeMulti = ((double)fadeMulti) / 100.0;
		dlogr(" Done", DL_INIT);
	}
}

void Graphics::exit() {
	if (Graphics::mode) {
		if (titlesBuffer) delete[] titlesBuffer;
		CoUninitialize();
	}
}

}

// This should be in global namespace
HRESULT _stdcall FakeDirectDrawCreate2(void* a, IDirectDraw** b, void* c) {
	return sfall::FakeDirectDrawCreate2_Init(a, b, c);
}
