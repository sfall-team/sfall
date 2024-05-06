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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\WinProc.h"
#include "LoadGameHook.h"
#include "ScriptShaders.h"
#include "Movies.h"

#include "SubModules\DirectDraw.h"
#include "SubModules\WindowRender.h"

#include "..\HRP\Init.h"
#include "..\HRP\Image.h"
#include "..\HRP\MoviesScreen.h"

#include "..\HLSL\A8PixelShader.h"
#include "..\HLSL\L8PixelShader.h"

#include "Graphics.h"

namespace sfall
{

//typedef HRESULT (__stdcall *DDrawCreateProc)(void*, IDirectDraw**, void*);
//typedef IDirect3D9* (__stdcall *D3DCreateProc)(UINT version);

#define UNUSEDFUNCTION          { return DDERR_GENERIC; }
#define SAFERELEASE(a)          { if (a) { a->Release(); a = nullptr; } }

#define ShowMessageBox(text)    if (!Graphics::IsWindowedMode) ShowWindow(window, SW_MINIMIZE); \
                                MessageBoxA(window, text, "sfall DirectX 9", MB_TASKMODAL | MB_ICONWARNING); \
                                if (!Graphics::IsWindowedMode) ShowWindow(window, SW_RESTORE)

#if !(NDEBUG) && !(_DEBUG)
static LPD3DXFONT font;
static RECT fontPosition;

#define FPSOnLostDevice  font->OnLostDevice();
#define FPSOnResetDevice font->OnResetDevice();
#else
#define FPSOnLostDevice
#define FPSOnResetDevice
#endif

IDirectDrawSurface* primarySurface = nullptr; // aka _GNW95_DDPrimarySurface
IDirectDrawPalette* primaryPalette = nullptr; // aka _GNW95_DDPrimaryPalette

static DWORD ResWidth;
static DWORD ResHeight;

static DWORD gWidth;
static DWORD gHeight;

DWORD Graphics::GPUBlt;
DWORD Graphics::mode;
bool Graphics::IsWindowedMode;

bool Graphics::PlayAviMovie = false;
bool Graphics::AviMovieWidthFit = false;
static bool dShowMovies;

static bool DeviceLost = false;
static char textureFilter; // 1 - auto, 2 - force

static DDSURFACEDESC surfaceDesc;
static DDSURFACEDESC mveDesc;
static D3DSURFACE_DESC movieDesc;

static DirectDraw::PALCOLOR* palette;
static bool paletteInit = false;

static HWND window;

static DWORD ShaderVersion;

IDirect3D9* d3d9;
IDirect3DDevice9* d3d9Device = nullptr;

static IDirect3DTexture9* mainTex;
static IDirect3DTexture9* mainTexD;
static IDirect3DTexture9* sTex1;
static IDirect3DTexture9* sTex2;
static IDirect3DTexture9* movieTex;

static IDirect3DSurface9* sSurf1;
static IDirect3DSurface9* sSurf2;
static IDirect3DSurface9* backBuffer;

static IDirect3DVertexBuffer9* vertexOrigRes;
static IDirect3DVertexBuffer9* vertexSfallRes;
static IDirect3DVertexBuffer9* vertexMovie; // for AVI
//static IDirect3DVertexBuffer9* vertexStaticScreen; // splash/death/endslides

static IDirect3DTexture9* gpuPalette;
static ID3DXEffect* gpuBltEffect;

static D3DXHANDLE gpuBltMainTex;
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
	{-0.5,  -0.5,  0, 1, 0, 0}, // 0 - top left
	{-0.5,  479.5, 0, 1, 0, 1}, // 1 - bottom left
	{639.5, -0.5,  0, 1, 1, 0}, // 2 - top right
	{639.5, 479.5, 0, 1, 1, 1}  // 3 - bottom right
};

long Graphics::GetGameWidthRes() {
	return (fo::var::scr_size.offx - fo::var::scr_size.x) + 1;
}

long Graphics::GetGameHeightRes() {
	return (fo::var::scr_size.offy - fo::var::scr_size.y) + 1;
}

int __stdcall Graphics::GetShaderVersion() {
	return ShaderVersion;
}

static void WindowInit() {
	paletteInit = true;
	rcpres[0] = 1.0f / (float)Graphics::GetGameWidthRes();
	rcpres[1] = 1.0f / (float)Graphics::GetGameHeightRes();
	ScriptShaders::LoadGlobalShader();
}

// pixel size for the current game resolution
const float* Graphics::rcpresGet() {
	return rcpres;
}

static void GetDisplayMode(D3DDISPLAYMODE &ddm) {
	ZeroMemory(&ddm, sizeof(ddm));
	d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm);
	dlog_f("Display mode format ID: %d\n", DL_INIT, ddm.Format);
}

static void ResetDevice(bool create) {
	D3DPRESENT_PARAMETERS params;
	ZeroMemory(&params, sizeof(params));

	D3DDISPLAYMODE dispMode;
	GetDisplayMode(dispMode);

	params.BackBufferCount = 1;
	params.BackBufferFormat = dispMode.Format;
	params.BackBufferWidth = gWidth;
	params.BackBufferHeight = gHeight;
	params.Windowed = (Graphics::mode != 4);
	params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	params.hDeviceWindow = window;
	params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (!params.Windowed) params.FullScreen_RefreshRateInHz = dispMode.RefreshRate;

	static bool software = false;
	static D3DFORMAT textureFormat = D3DFMT_X8R8G8B8;

	if (create) {
		DWORD deviceFlags = D3DCREATE_FPU_PRESERVE;
		if (dShowMovies) deviceFlags |= D3DCREATE_MULTITHREADED;

		dlog("Creating D3D9 Device...", DL_MAIN);
		if (FAILED(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING | deviceFlags, &params, &d3d9Device))) { // D3DCREATE_PUREDEVICE
			if (FAILED(d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_SOFTWARE_VERTEXPROCESSING | deviceFlags, &params, &d3d9Device))) {
				d3d9Device = nullptr;
				dlogr(" Failed!", DL_MAIN);
				return;
			}
			software = true;
			ShowMessageBox("Failed to create hardware vertex processing device.\nUsing software vertex processing instead.");
		}

		D3DCAPS9 caps;
		d3d9Device->GetDeviceCaps(&caps);
		ShaderVersion = ((caps.PixelShaderVersion & 0x0000FF00) >> 8) * 10 + (caps.PixelShaderVersion & 0xFF);

		bool npow2 = ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) != 0); //(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
		if (npow2) dlog("Warning: The graphics card does not support non-power-of-two textures.", DL_MAIN);

		// Use: 0 - only CPU, 1 - force GPU, 2 - Auto Mode (GPU or switch to CPU)
		if (Graphics::GPUBlt == 2 && ShaderVersion < 20) Graphics::GPUBlt = 0;

		bool A8IsSupported = Graphics::GPUBlt && (d3d9Device->CreateTexture(ResWidth, ResHeight, 1, 0, D3DFMT_A8, D3DPOOL_SYSTEMMEM, &mainTex, 0) == D3D_OK);

		if (Graphics::GPUBlt) {
			const BYTE* shader = (A8IsSupported) ? gpuEffectA8 : gpuEffectL8;
			const UINT size = (A8IsSupported) ? sizeof(gpuEffectA8) : sizeof(gpuEffectL8);
			if (D3DXCreateEffect(d3d9Device, shader, size, 0, 0, 0, 0, &gpuBltEffect, 0) == D3D_OK) {
				gpuBltMainTex = gpuBltEffect->GetParameterByName(0, "image");
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

				textureFormat = (A8IsSupported) ? D3DFMT_A8 : D3DFMT_L8; // D3DFMT_A8 - not supported on some older video cards
			} else {
				ShowMessageBox("Failed to create shader effects.\nSwitching to CPU for the palette conversion.");
				if (mainTex) SAFERELEASE(mainTex); // release D3DFMT_A8 format texture
				Graphics::GPUBlt = 0;
				A8IsSupported = false;
			}
		}

		if (!A8IsSupported && d3d9Device->CreateTexture(ResWidth, ResHeight, 1, 0, textureFormat, D3DPOOL_SYSTEMMEM, &mainTex, 0) != D3D_OK) {
			textureFormat = D3DFMT_X8R8G8B8;
			d3d9Device->CreateTexture(ResWidth, ResHeight, 1, 0, textureFormat, D3DPOOL_SYSTEMMEM, &mainTex, 0);
			ShowMessageBox("Texture format error.\nGPU does not support the D3DFMT_L8 texture format.\nNow CPU is used to convert the palette.\n"
			               "Set 'GPUBlt' option to CPU to bypass this warning message.");
			Graphics::GPUBlt = 0;
		}

		palette = new DirectDraw::PALCOLOR[256];

		#if !(NDEBUG) && !(_DEBUG)
			D3DXCreateFontA(d3d9Device, 24, 0, 500, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font);

			fontPosition.top = 10;
			fontPosition.left = 10;
			fontPosition.right = 610;
			fontPosition.bottom = 30;
		#endif
	} else {
		dlog("Resetting D3D9 Device...", DL_MAIN);
		d3d9Device->Reset(&params);
		if (gpuBltEffect) gpuBltEffect->OnResetDevice();
		ScriptShaders::OnResetDevice();
		FPSOnResetDevice
	}

	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, 0, textureFormat, D3DPOOL_DEFAULT, &mainTexD, 0);
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex1, 0);
	d3d9Device->CreateTexture(ResWidth, ResHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &sTex2, 0);
	sTex1->GetSurfaceLevel(0, &sSurf1);
	sTex2->GetSurfaceLevel(0, &sSurf2);

	if (Graphics::GPUBlt) {
		d3d9Device->CreateTexture(256, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &gpuPalette, 0);
		gpuBltEffect->SetTexture(gpuBltMainTex, mainTexD);
		gpuBltEffect->SetTexture(gpuBltPalette, gpuPalette);
	}

	d3d9Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);

	ShaderVertices[1].y = ResHeight - 0.5f;
	ShaderVertices[2].x = ResWidth - 0.5f;
	ShaderVertices[3].y = ResHeight - 0.5f;
	ShaderVertices[3].x = ResWidth - 0.5f;

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vertexOrigRes, 0);
	void* vertexPointer;
	vertexOrigRes->Lock(0, 0, &vertexPointer, 0);
	CopyMemory(vertexPointer, ShaderVertices, sizeof(ShaderVertices));
	vertexOrigRes->Unlock();

	VertexFormat shaderVertices[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	shaderVertices[1].y = (float)gHeight - 0.5f;
	shaderVertices[2].x = (float)gWidth - 0.5f;
	shaderVertices[3].y = (float)gHeight - 0.5f;
	shaderVertices[3].x = (float)gWidth - 0.5f;

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vertexSfallRes, 0);
	vertexSfallRes->Lock(0, 0, &vertexPointer, 0);
	CopyMemory(vertexPointer, shaderVertices, sizeof(shaderVertices));
	vertexSfallRes->Unlock();

	d3d9Device->CreateVertexBuffer(4 * sizeof(VertexFormat), D3DUSAGE_WRITEONLY | (software ? D3DUSAGE_SOFTWAREPROCESSING : 0), _VERTEXFORMAT, D3DPOOL_DEFAULT, &vertexMovie, 0);

	d3d9Device->SetFVF(_VERTEXFORMAT);
	d3d9Device->SetTexture(0, mainTexD);
	d3d9Device->SetStreamSource(0, vertexOrigRes, 0, sizeof(VertexFormat));

	//d3d9Device->SetRenderState(D3DRS_ALPHABLENDENABLE, false); // default false
	//d3d9Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);  // default false
	d3d9Device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	d3d9Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3d9Device->SetRenderState(D3DRS_LIGHTING, false);

	if (textureFilter) {
		d3d9Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		d3d9Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	}
	//d3d9Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 0, 255), 1.0f, 0); // for debbuging
	dlogr(" Done", DL_MAIN);
}

#if !(NDEBUG) && !(_DEBUG)
static DWORD lastTime = GetTickCount();
static long frameCount;
static long elapsedTime;
static long fps;

static void CalcFPS() {
	frameCount++;

	DWORD time = GetTickCount();
	elapsedTime += time - lastTime;
	lastTime = time;

	if (elapsedTime >= 1000) {
		fps = (1000 * frameCount) / elapsedTime;
		elapsedTime = 0;
		frameCount = 0;
	}
}

static char text[64] = "FPS: ";

static void DrawFPS() {
	_itoa(fps, &text[5], 10);
	//sprintf(text, "FPS: %d | Lock/PAL: %d/%d", fps, lockCounter, palCounter);
	font->DrawTextA(0, text, -1, &fontPosition, DT_LEFT, D3DCOLOR_RGBA(255, 255, 128, 192));
}
#else
static void CalcFPS() {}
static void DrawFPS() {}
#endif

static void Present() {
	WinProc::Moving();

	if (d3d9Device->Present(0, 0, 0, 0) == D3DERR_DEVICELOST) {
		#ifndef NDEBUG
		dlogr("\nPresent: D3DERR_DEVICELOST", DL_MAIN);
		#endif
		DeviceLost = true;
		d3d9Device->SetTexture(0, 0);
		SAFERELEASE(mainTexD)
		SAFERELEASE(backBuffer);
		SAFERELEASE(sSurf1);
		SAFERELEASE(sSurf2);
		SAFERELEASE(sTex1);
		SAFERELEASE(sTex2);
		SAFERELEASE(vertexOrigRes);
		SAFERELEASE(vertexSfallRes);
		SAFERELEASE(vertexMovie);
		SAFERELEASE(gpuPalette);
		Graphics::ReleaseMovieTexture();
		if (gpuBltEffect) gpuBltEffect->OnLostDevice();
		ScriptShaders::OnLostDevice();
		FPSOnLostDevice
	}
	CalcFPS();
}

static void Refresh() {
	if (DeviceLost) return;

	d3d9Device->BeginScene();
	d3d9Device->SetStreamSource(0, vertexOrigRes, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, sSurf1);
	d3d9Device->SetTexture(0, mainTexD);

	UINT passes;
	if (Graphics::GPUBlt) {
		// converts the palette index in mainTexD to RGB colors on the target surface (sSurf1/sTex1)
		gpuBltEffect->Begin(&passes, 0);
		gpuBltEffect->BeginPass(0);
		d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		gpuBltEffect->EndPass();
		gpuBltEffect->End();

		if (ScriptShaders::Count()) {
			d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE); // copy sSurf1 to sSurf2
			d3d9Device->SetTexture(0, sTex2);
		} else {
			d3d9Device->SetTexture(0, sTex1);
		}
	}
	ScriptShaders::Refresh(sSurf1, sSurf2, sTex2);

	d3d9Device->SetStreamSource(0, vertexSfallRes, 0, sizeof(VertexFormat));
	d3d9Device->SetRenderTarget(0, backBuffer);

	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2); // render square with a texture image (mainTexD/sTex1/sTex2) to the backBuffer
	DrawFPS();

	d3d9Device->EndScene();
	Present();
}

HRESULT Graphics::CreateMovieTexture(D3DSURFACE_DESC &desc) {
	HRESULT hr = d3d9Device->CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, D3DPOOL_DEFAULT, &movieTex, nullptr);
	if (movieTex) movieTex->GetLevelDesc(0, &movieDesc);
	return hr;
}

void Graphics::ReleaseMovieTexture() {
	SAFERELEASE(movieTex);
}

void Graphics::SetMovieTexture(bool state) {
	dlog("\nSet movie texture.", DL_INIT);
	if (!state) {
		PlayAviMovie = false;
		return;
	} else if (PlayAviMovie) return;

	if (!movieTex) Graphics::CreateMovieTexture(movieDesc);
	D3DSURFACE_DESC &desc = movieDesc;

	float aviAspect = (float)desc.Width / (float)desc.Height;
	float winAspect = (float)gWidth / (float)gHeight;

	VertexFormat shaderVertices[4] = {
		ShaderVertices[0],
		ShaderVertices[1],
		ShaderVertices[2],
		ShaderVertices[3]
	};

	shaderVertices[1].y = (float)gHeight - 0.5f;
	shaderVertices[2].x = (float)gWidth - 0.5f;
	shaderVertices[3].y = (float)gHeight - 0.5f;
	shaderVertices[3].x = (float)gWidth - 0.5f;

	bool subtitleShow = (fo::var::subtitleList != nullptr);

	long offset;
	if (aviAspect > winAspect) {
		// scales height proportionally and places the movie surface at the center of the window along the Y-axis
		aviAspect = (float)desc.Width / (float)gWidth;
		desc.Height = (int)(desc.Height / aviAspect);

		offset = (gHeight - desc.Height) / 2;

		shaderVertices[0].y += offset;
		shaderVertices[2].y += offset;
		shaderVertices[1].y -= offset;
		shaderVertices[3].y -= offset;

		subtitleShow = false;
	} else if (aviAspect < winAspect) {
		if (Graphics::AviMovieWidthFit) {
			//desc.Width = gWidth; // scales the movie surface to screen width
		} else {
			// scales width proportionally and places the movie surface at the center of the window along the X-axis
			aviAspect = (float)desc.Height / (float)gHeight;
			desc.Width = (int)(desc.Width / aviAspect);

			offset = (gWidth - desc.Width) / 2;

			shaderVertices[0].x += offset;
			shaderVertices[2].x -= offset;
			shaderVertices[3].x -= offset;
			shaderVertices[1].x += offset;
		}
	}
	if (subtitleShow) { // decrease the surface size to display the subtitle text on the lower layer of surfaces
		int offset = (int)(15.0f * ((float)gHeight / (float)ResHeight));
		shaderVertices[1].y -= offset;
		shaderVertices[3].y -= offset;
	}

	void* vertexPointer;
	vertexMovie->Lock(0, 0, &vertexPointer, 0);
	CopyMemory(vertexPointer, shaderVertices, sizeof(shaderVertices));
	vertexMovie->Unlock();

	PlayAviMovie = true;
}

void Graphics::ShowMovieFrame(IDirect3DTexture9* tex) {
	if (!tex || DeviceLost || !movieTex) return;

	d3d9Device->UpdateTexture(tex, movieTex);
	d3d9Device->SetRenderTarget(0, backBuffer);

	d3d9Device->BeginScene();

	if (ScriptShaders::Count() && Graphics::GPUBlt) {
		d3d9Device->SetTexture(0, sTex2);
	} else {
		d3d9Device->SetTexture(0, mainTexD);
	}
	d3d9Device->SetStreamSource(0, vertexSfallRes, 0, sizeof(VertexFormat));

	// for showing subtitles
	if (Graphics::GPUBlt) {
		UINT passes;
		gpuBltEffect->Begin(&passes, 0);
		gpuBltEffect->BeginPass(0);
	}
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (Graphics::GPUBlt) {
		gpuBltEffect->EndPass();
		gpuBltEffect->End();
	}

	// for avi movie
	d3d9Device->SetTexture(0, movieTex);
	d3d9Device->SetStreamSource(0, vertexMovie, 0, sizeof(VertexFormat));
	d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	d3d9Device->EndScene();
	Present();
}

void Graphics::SetHighlightTexture(IDirect3DTexture9* htex, int xPos, int yPos) {
	gpuBltEffect->SetTexture(gpuBltHighlight, htex);

	float size[2];
	size[0] = 388.0f * rcpres[0];
	size[1] = 200.0f * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHighlightSize, size, 2);

	size[0] = (126.0f + xPos) * rcpres[0];
	size[1] = (16.0f + yPos) * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHighlightCorner, size, 2);
}

void Graphics::SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff, int showHighlight) {
	gpuBltEffect->SetInt(gpuBltShowHighlight, showHighlight);
	gpuBltEffect->SetTexture(gpuBltHead, tex);

	float size[2];
	size[0] = (float)width * rcpres[0];
	size[1] = (float)height * rcpres[1];
	gpuBltEffect->SetFloatArray(gpuBltHeadSize, size, 2);

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

static void SetGPUPalette() {
	D3DLOCKED_RECT lock;
	gpuPalette->LockRect(0, &lock, 0, D3DLOCK_DISCARD);
	std::memcpy(lock.pBits, palette, 256 * 4);
	gpuPalette->UnlockRect(0);
}

class FakeDirectDrawSurface : IDirectDrawSurface {
private:
	ULONG Refs;
	bool isPrimary;

	BYTE* lockTarget = nullptr;
	RECT* lockRect;

	// size for secondary (mve) surface
	DWORD m_width;
	DWORD m_height;

public:
	static bool IsPlayMovie;

	FakeDirectDrawSurface(bool primary, DDSURFACEDESC* desc) {
		Refs = 1;
		isPrimary = primary;
		if (primary) {
			if (Graphics::GPUBlt == 0) lockTarget = new BYTE[ResWidth * ResHeight]();
			// for enabled GPUBlt, use the mainTex texture as source buffer
		} else {
			m_width = desc->dwWidth;
			m_height = desc->dwHeight;
			lockTarget = new BYTE[m_width * m_height];
		}
	}

	/* IUnknown methods */

	HRESULT __stdcall QueryInterface(REFIID, LPVOID *) { return E_NOINTERFACE; }

	ULONG __stdcall AddRef() { return ++Refs; }

	ULONG __stdcall Release() {
		if (!--Refs) {
			delete[] lockTarget;
			delete this;
			return 0;
		} else return Refs;
	}

	/* IDirectDrawSurface methods */

	HRESULT __stdcall AddAttachedSurface(LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
	HRESULT __stdcall AddOverlayDirtyRect(LPRECT) { UNUSEDFUNCTION; }

	// called 0x4868DA movie_MVE_ShowFrame_ used for game movies (only for w/o HRP by Mash)
	HRESULT __stdcall Blt(LPRECT dst, LPDIRECTDRAWSURFACE b, LPRECT scr, DWORD d, LPDDBLTFX e) {
		if (DeviceLost && Restore() == DD_FALSE) return DDERR_SURFACELOST;

		mveDesc.dwHeight = scr->bottom;
		mveDesc.lPitch = scr->right;

		//dlog_f("\nBlt: [mveDesc: w:%d, h:%d]", DL_INIT, mveDesc.lPitch, mveDesc.dwHeight);

		IsPlayMovie = true;

		BYTE* mveSurface = ((FakeDirectDrawSurface*)b)->lockTarget;

		D3DLOCKED_RECT dRect;
		mainTex->LockRect(0, &dRect, dst, D3DLOCK_NO_DIRTY_UPDATE);
		mainTex->AddDirtyRect(dst);

		DWORD width = mveDesc.lPitch; // the current size of the width of the mve movie

		if (Graphics::GPUBlt) {
			if (d != 0) {
				HRP::Image::Scale(mveSurface, width, mveDesc.dwHeight, (BYTE*)dRect.pBits, dst->right - dst->left, dst->bottom - dst->top, dRect.Pitch);
			} else {
				fo::func::buf_to_buf(mveSurface, width, mveDesc.dwHeight, width, (BYTE*)dRect.pBits, dRect.Pitch);
			}
		} else {
			int sPitch = width;
			int height = mveDesc.dwHeight;

			if (d != 0) { // scale
				BYTE* mveScaleBuffer = (BYTE*)fo::var::getInt(FO_VAR_screen_buffer);

				width = dst->right - dst->left;
				height = dst->bottom - dst->top;

				HRP::Image::Scale(mveSurface, mveDesc.lPitch, mveDesc.dwHeight, mveScaleBuffer, width, height, ResWidth);

				mveSurface = mveScaleBuffer;
				sPitch = ResWidth;
			}

			int pitch = dRect.Pitch / 4;
			DWORD* pBits = (DWORD*)dRect.pBits;

			while (height--) {
				int x = width;
				while (x--) pBits[x] = palette[mveSurface[x]].xRGB;
				mveSurface += sPitch;
				pBits += pitch;
			}
		}
		mainTex->UnlockRect(0);
		d3d9Device->UpdateTexture(mainTex, mainTexD);

		//IDirect3DSurface9 *mSurf, *mSurfD;
		//mainTex->GetSurfaceLevel(0, &mSurf);
		//mainTexD->GetSurfaceLevel(0, &mSurfD);
		//d3d9Device->StretchRect(mSurf, 0, mSurfD, 0, D3DTEXF_LINEAR);

		//if (Graphics::PlayAviMovie) return DD_OK; // Blt method is not executed during avi playback because the sfShowFrame_ function is blocked

		Refresh();
		return DD_OK;
	}

	HRESULT __stdcall BltBatch(LPDDBLTBATCH, DWORD, DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall BltFast(DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall DeleteAttachedSurface(DWORD,LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumAttachedSurfaces(LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumOverlayZOrders(DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall Flip(LPDIRECTDRAWSURFACE, DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetBltStatus(DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetCaps(LPDDSCAPS) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetClipper(LPDIRECTDRAWCLIPPER *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetDC(HDC *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetFlipStatus(DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetOverlayPosition(LPLONG, LPLONG) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetPalette(LPDIRECTDRAWPALETTE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetPixelFormat(LPDDPIXELFORMAT) { UNUSEDFUNCTION; }

	HRESULT __stdcall GetSurfaceDesc(LPDDSURFACEDESC a) {
		*a = surfaceDesc;
		a->lpSurface = lockTarget;
		return DD_OK;
	}

	HRESULT __stdcall Initialize(LPDIRECTDRAW, LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT __stdcall IsLost() { UNUSEDFUNCTION; }

	/* Called from:
		0x4CB887 GNW95_ShowRect_      [c=1]
		0x48699D movieShowFrame_      [c=1]
		0x4CBBFA GNW95_zero_vid_mem_  [c=1] (clear surface)
		0x486861 movie_MVE_ShowFrame_ [c=1] (capture, never called)
		0x4F5E91/0x4F5EBB nf_mve_buf_lock [c=0] (from MVE_rmStepMovie_)
	*/
	HRESULT __stdcall Lock(LPRECT a, LPDDSURFACEDESC b, DWORD c, HANDLE d) {
		if (DeviceLost && Restore() == DD_FALSE) return DDERR_SURFACELOST; // DDERR_SURFACELOST (0x887601C2)
		if (isPrimary) {
			lockRect = a;
			if (Graphics::GPUBlt) {
				D3DLOCKED_RECT buf;
				if (SUCCEEDED(mainTex->LockRect(0, &buf, lockRect, D3DLOCK_NO_DIRTY_UPDATE))) {
					mainTex->AddDirtyRect(lockRect);
					b->lpSurface = buf.pBits;
					b->lPitch = buf.Pitch;
				}
			} else {
				*b = surfaceDesc;
				b->lpSurface = lockTarget;
			}
		} else {
			mveDesc.dwWidth = m_width;
			mveDesc.lPitch = m_width; //fo::var::getInt(FO_VAR_lastMovieW);
			mveDesc.dwHeight = m_height; //fo::var::getInt(FO_VAR_lastMovieH);
			*b = mveDesc;
			b->lpSurface = lockTarget;
			//dlog_f("\nLock: [mveDesc: w:%d, h:%d]", DL_INIT, mveDesc.lPitch, mveDesc.dwHeight);
		}
		return DD_OK;
	}

	HRESULT __stdcall ReleaseDC(HDC) { UNUSEDFUNCTION; }

	HRESULT __stdcall Restore() { // called 0x4CB907 GNW95_ShowRect_
		if (!d3d9Device) return DD_FALSE;
		if (d3d9Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
			ResetDevice(false);
			DeviceLost = false;
			// restore palette
			if (Graphics::GPUBlt) SetGPUPalette();
			#ifndef NDEBUG
			dlogr("\nD3D9 Device restored.", DL_MAIN);
			#endif
		}
		return (DeviceLost) ? DD_FALSE : DD_OK;
	}

	HRESULT __stdcall SetClipper(LPDIRECTDRAWCLIPPER) { UNUSEDFUNCTION; }
	HRESULT __stdcall SetColorKey(DWORD, LPDDCOLORKEY) { UNUSEDFUNCTION; }
	HRESULT __stdcall SetOverlayPosition(LONG, LONG) { UNUSEDFUNCTION; }

	HRESULT __stdcall SetPalette(LPDIRECTDRAWPALETTE a) {
		if (a) { // called 0x4CB198 GNW95_init_DirectDraw_
			primaryPalette = a;
			return DD_OK; // prevents executing the function when called from outside of sfall
		}

		D3DLOCKED_RECT dRect;
		mainTex->LockRect(0, &dRect, 0, 0);

		DWORD* pBits = (DWORD*)dRect.pBits;
		int pitch = (dRect.Pitch / 4) - ResWidth;
		BYTE* target = lockTarget;

		int height = ResHeight;
		while (height--) {
			int width = ResWidth;
			while (width--) {
				pBits[0] = palette[target[0]].xRGB; // takes the color index in the palette and copies the color value to the texture surface
				target++;
				pBits++;
			}
			pBits += pitch;
		}

		mainTex->UnlockRect(0);
		if (!DeviceLost) d3d9Device->UpdateTexture(mainTex, mainTexD);

		return DD_OK;
	}

	/* Called from:
		0x4CB8F0 GNW95_ShowRect_      (common game, primary)
		0x486A87 movieShowFrame_
		0x4CBC5A GNW95_zero_vid_mem_  (clear surface)
		0x4F5ECC nf_mve_buf_lock      (from MVE_rmStepMovie_)
		0x4868BA movie_MVE_ShowFrame_ (capture never call)
		0x4F5EFA/0x4F5F0B nf_mve_buf_unlock (from MVE_rmStepMovie_)
	*/
	HRESULT __stdcall Unlock(LPVOID lockSurface) {
		if (!isPrimary) return DD_OK;

		if (Graphics::GPUBlt == 0) {
			D3DLOCKED_RECT dRect;
			mainTex->LockRect(0, &dRect, lockRect, D3DLOCK_NO_DIRTY_UPDATE);
			mainTex->AddDirtyRect(lockRect);

			DWORD* pBits = (DWORD*)dRect.pBits;
			int pitch = dRect.Pitch / 4;

			if (lockRect) {
				BYTE* target = (BYTE*)lockSurface;
				int width = (lockRect->right - lockRect->left);
				int height = (lockRect->bottom - lockRect->top);

				while (height--) {
					int w = width;
					while (w--) pBits[w] = palette[target[w]].xRGB;
					pBits += pitch;
					target += ResWidth;
				}

				lockRect = 0;
			} else {
				pitch -= ResWidth;
				BYTE* target = lockTarget;
				int height = ResHeight;

				// slow copy method
				while (height--) {
					int width = ResWidth;
					while (width--) {
						pBits[0] = palette[target[0]].xRGB;
						target++;
						pBits++;
					}
					pBits += pitch;
				}
			}
		}

		mainTex->UnlockRect(0);
		d3d9Device->UpdateTexture(mainTex, mainTexD);

		if (!IsPlayMovie && !Graphics::PlayAviMovie) {
			Refresh();
		}
		IsPlayMovie = false;
		return DD_OK;
	}

	HRESULT __stdcall UpdateOverlay(LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) { UNUSEDFUNCTION; }
	HRESULT __stdcall UpdateOverlayDisplay(DWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall UpdateOverlayZOrder(DWORD, LPDIRECTDRAWSURFACE) { UNUSEDFUNCTION; }
};

bool FakeDirectDrawSurface::IsPlayMovie;

class FakeDirectDrawPalette : IDirectDrawPalette {
private:
	ULONG Refs;
public:
	FakeDirectDrawPalette() {
		Refs = 1;
	}

	/* IUnknown methods */

	HRESULT __stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }

	ULONG __stdcall AddRef() { return ++Refs; }

	ULONG __stdcall Release() {
		if (!--Refs) {
			delete this;
			return 0;
		} else return Refs;
	}

	/* IDirectDrawPalette methods */

	HRESULT __stdcall GetCaps(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetEntries(DWORD, DWORD, DWORD, LPPALETTEENTRY) { UNUSEDFUNCTION; }
	HRESULT __stdcall Initialize(LPDIRECTDRAW, DWORD, LPPALETTEENTRY) { UNUSEDFUNCTION; }

	/* Called from:
		0x4CB5C7 GNW95_SetPalette_
		0x4CB36B GNW95_SetPaletteEntries_
	*/
	HRESULT __stdcall SetEntries(DWORD a, DWORD b, DWORD c, LPPALETTEENTRY d) { // used to set palette for splash screen, fades, subtitles
		if (!paletteInit || (long)c <= 0) return DDERR_INVALIDPARAMS;

		fo::PALETTE* destPal = (fo::PALETTE*)d;

		if (Graphics::GPUBlt) {
			while (true) {
				palette[b].xRGB = *(DWORD*)destPal;
				if (--c == 0) break;
				destPal++;
				b++;
			}
			if (!DeviceLost) SetGPUPalette();
		} else {
			// copy and swap color B <> R
			while (true) {
				palette[b].R = destPal->R << 2;
				palette[b].G = destPal->G << 2;
				palette[b].B = destPal->B << 2;
				if (--c == 0) break;
				destPal++;
				b++;
			}

			primarySurface->SetPalette(0); // update texture
			if (FakeDirectDrawSurface::IsPlayMovie) return DD_OK; // prevents flickering at the beginning of playback (w/o HRP & GPUBlt=2)
		}
		if (!Graphics::PlayAviMovie) {
			Refresh();
		}
		return DD_OK;
	}
};

class FakeDirectDraw : IDirectDraw {
private:
	ULONG Refs;
public:
	FakeDirectDraw() {
		Refs = 1;
	}

	/* IUnknown methods */

	HRESULT __stdcall QueryInterface(REFIID, LPVOID*) { return E_NOINTERFACE; }

	ULONG __stdcall AddRef()  { return ++Refs; }

	ULONG __stdcall Release() { // called from GNW95_reset_mode_ (on game exit)
		if (!--Refs) {
			ScriptShaders::Release();

			SAFERELEASE(backBuffer);
			SAFERELEASE(sSurf1);
			SAFERELEASE(sSurf2);
			SAFERELEASE(mainTex);
			SAFERELEASE(mainTexD);
			SAFERELEASE(sTex1);
			SAFERELEASE(sTex2);
			SAFERELEASE(vertexOrigRes);
			SAFERELEASE(vertexSfallRes);
			SAFERELEASE(gpuPalette);
			SAFERELEASE(gpuBltEffect);
			SAFERELEASE(vertexMovie);
			SAFERELEASE(d3d9Device);
			SAFERELEASE(d3d9);

			delete[] palette;
			delete this;
			return 0;
		} else return Refs;
	}

	/* IDirectDraw methods */

	HRESULT __stdcall Compact() { UNUSEDFUNCTION; }
	HRESULT __stdcall CreateClipper(DWORD, LPDIRECTDRAWCLIPPER*, IUnknown*) { UNUSEDFUNCTION; }

	HRESULT __stdcall CreatePalette(DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE* c, IUnknown*) { // called 0x4CB182 GNW95_init_DirectDraw_
		*c = (IDirectDrawPalette*)new FakeDirectDrawPalette();
		return DD_OK;
	}

	/*
		0x4CB094 GNW95_init_DirectDraw_ (primary surface)
		0x4F5DD4/0x4F5DF9 nfConfig_     (mve surface)
	*/
	HRESULT __stdcall CreateSurface(LPDDSURFACEDESC desc, LPDIRECTDRAWSURFACE* b, IUnknown* c) {
		if (desc->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE && desc->dwFlags == DDSD_CAPS) {
			*b = primarySurface = (IDirectDrawSurface*)new FakeDirectDrawSurface(true, nullptr);
		} else {
			*b = (IDirectDrawSurface*)new FakeDirectDrawSurface(false, desc);
		}
		return DD_OK;
	}

	HRESULT __stdcall DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK) { UNUSEDFUNCTION; }
	HRESULT __stdcall FlipToGDISurface() { UNUSEDFUNCTION; }
	HRESULT __stdcall GetCaps(LPDDCAPS, LPDDCAPS b) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetDisplayMode(LPDDSURFACEDESC) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetFourCCCodes(LPDWORD,LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetGDISurface(LPDIRECTDRAWSURFACE *) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetMonitorFrequency(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetScanLine(LPDWORD) { UNUSEDFUNCTION; }
	HRESULT __stdcall GetVerticalBlankStatus(LPBOOL) { UNUSEDFUNCTION; }
	HRESULT __stdcall Initialize(GUID *) { UNUSEDFUNCTION; }

	HRESULT __stdcall RestoreDisplayMode() { // called from GNW95_reset_mode_
		#ifdef NDEBUG
		if (!Graphics::IsWindowedMode) ShowWindow(window, SW_HIDE);
		#endif
		return DD_OK;
	}

	HRESULT __stdcall SetCooperativeLevel(HWND a, DWORD b) { // called 0x4CB005 GNW95_init_DirectDraw_
		window = a;
		WinProc::SetHWND(window);
		WinProc::SetTitle(gWidth, gHeight, Graphics::mode);

		if (Graphics::mode >= 5) {
			long windowStyle = (Graphics::mode == 5) ? (WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU) : WS_OVERLAPPED;
			WinProc::SetStyle(windowStyle);
		}

		if (!d3d9Device) {
			CoInitialize(0);
			ResetDevice(true); // create
		}
		return (d3d9Device) ? DD_OK : DD_FALSE;
	}

	HRESULT __stdcall SetDisplayMode(DWORD, DWORD, DWORD) { return DD_OK; } // called 0x4CB01B GNW95_init_DirectDraw_
	HRESULT __stdcall WaitForVerticalBlank(DWORD, HANDLE) { UNUSEDFUNCTION; }
};

HRESULT __stdcall InitFakeDirectDrawCreate(void*, IDirectDraw** b, void*) {
	dlog("Initializing Direct3D...", DL_MAIN);

	// original resolution or HRP
	ResWidth  = HRP::Setting::ScreenWidth();  //*(DWORD*)0x4CAD6B; // 640
	ResHeight = HRP::Setting::ScreenHeight(); //*(DWORD*)0x4CAD66; // 480

	if (!d3d9) d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d9) return DD_FALSE;

	ZeroMemory(&surfaceDesc, sizeof(DDSURFACEDESC));

	surfaceDesc.dwSize = sizeof(DDSURFACEDESC);
	surfaceDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH;
	surfaceDesc.dwWidth = ResWidth;
	surfaceDesc.dwHeight = ResHeight;
	surfaceDesc.ddpfPixelFormat.dwRGBBitCount = 16; // R5G6B5
	surfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	surfaceDesc.ddpfPixelFormat.dwRBitMask = 0xF800;
	surfaceDesc.ddpfPixelFormat.dwGBitMask = 0x7E0;
	surfaceDesc.ddpfPixelFormat.dwBBitMask = 0x1F;
	surfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
	surfaceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	surfaceDesc.lPitch = ResWidth;

	// set params for .mve surface
	mveDesc = surfaceDesc;
	mveDesc.lPitch = 640;
	mveDesc.dwWidth = 640;
	mveDesc.dwHeight = 480;

	if (Graphics::mode == 6) {
		D3DDISPLAYMODE dispMode;
		GetDisplayMode(dispMode);
		gWidth  = dispMode.Width;
		gHeight = dispMode.Height;
	} else {
		gWidth  = IniReader::GetConfigInt("Graphics", "GraphicsWidth", 0);
		gHeight = IniReader::GetConfigInt("Graphics", "GraphicsHeight", 0);
		if (!gWidth || !gHeight) {
			gWidth  = ResWidth;
			gHeight = ResHeight;
		}
	}

	Graphics::GPUBlt = IniReader::GetConfigInt("Graphics", "GPUBlt", 0); // 0 - auto, 1 - GPU, 2 - CPU
	if (!Graphics::GPUBlt || Graphics::GPUBlt > 2)
		Graphics::GPUBlt = 2; // Swap them around to keep compatibility with old ddraw.ini
	else if (Graphics::GPUBlt == 2) Graphics::GPUBlt = 0; // Use CPU

	WinProc::SetSize(gWidth, gHeight, 0);
	if (Graphics::mode == 5) WinProc::LoadPosition();

	rcpres[0] = 1.0f / (float)gWidth;
	rcpres[1] = 1.0f / (float)gHeight;

	if (textureFilter) {
		float wScale = (float)gWidth / ResWidth;
		float hScale = (float)gHeight / ResHeight;
		if (wScale == 1.0f && hScale == 1.0f) textureFilter = 0; // disable texture filtering if the set resolutions are equal
		if (textureFilter == 1) {
			if (static_cast<int>(wScale) == wScale && static_cast<int>(hScale) == hScale) {
				textureFilter = 0; // disable for integer scales
			}
		}
	}

	*b = (IDirectDraw*)new FakeDirectDraw();

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

static __declspec(naked) void GNW95_SetPaletteEntries_replacement() {
	LPPALETTEENTRY palette;
	DWORD startIndex;
	DWORD count;
	__asm {
		push ebp;
		mov  ebp, esp;
		sub  esp, __LOCAL_SIZE;
		mov  count, ebx;
		mov  palette, eax;
		mov  startIndex, edx;
	}
	primaryPalette->SetEntries(0, startIndex, count, palette);

	__asm {
		mov esp, ebp; // epilog
		pop ebp;
		pop ecx;
		retn;
	}
}

static __declspec(naked) void GNW95_SetPalette_replacement() {
	LPPALETTEENTRY palette;
	__asm {
		push ecx;
		push edx;
		push ebp;
		mov  ebp, esp;
		sub  esp, __LOCAL_SIZE;
		mov  palette, eax;
	}
	primaryPalette->SetEntries(0, 0, 256, palette);

	__asm {
		mov esp, ebp; // epilog
		pop ebp;
		pop edx;
		pop ecx;
		retn;
	}
}

static DWORD forcingGraphicsRefresh = 0;

void Graphics::RefreshGraphics() {
	if (forcingGraphicsRefresh && !Graphics::PlayAviMovie) Refresh();
}

void __stdcall Graphics::ForceGraphicsRefresh(DWORD d) {
	if (!d3d9Device) return;
	forcingGraphicsRefresh = (d == 0) ? 0 : 1;
}

void Graphics::BackgroundClearColor(long indxColor) {
	if (Graphics::mode < 4) {
		DirectDraw::Clear(indxColor);
		return;
	}

	if (GPUBlt) {
		D3DLOCKED_RECT rectLock;
		mainTex->LockRect(0, &rectLock, 0, D3DLOCK_DISCARD);
		std::memset(rectLock.pBits, indxColor, rectLock.Pitch * ResHeight);
		mainTex->UnlockRect(0);
	} else {
		DDSURFACEDESC desc;
		primarySurface->GetSurfaceDesc(&desc);
		std::memset(desc.lpSurface, indxColor, ResWidth * ResHeight);
	}
}

long __stdcall SaveScreen(const char* file) {
	IDirect3DSurface9* surface;
	d3d9Device->CreateOffscreenPlainSurface(gWidth, gHeight, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, 0);
	d3d9Device->GetRenderTargetData(backBuffer, surface);

	LPD3DXBUFFER buffer;
	D3DXCreateBuffer(gWidth * gHeight * 2, &buffer);

	D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_PNG, surface, 0, 0);
	//D3DXSaveSurfaceToFileA(file, D3DXIFF_PNG, surface, 0, 0); // slow save

	HANDLE hFile = CreateFileA(file, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	bool resultOK = (hFile != INVALID_HANDLE_VALUE);
	if (resultOK) {
		DWORD dwWritten;
		WriteFile(hFile, buffer->GetBufferPointer(), buffer->GetBufferSize(), &dwWritten, 0);
		CloseHandle(hFile);
	}

	surface->Release();
	buffer->Release();

	return (resultOK) ? 0 : 1;
}

long __stdcall game_screendump_hook() {
	char fileName[16];

	for (int i = 0; i < 10000; i++) {
		std::sprintf(fileName, "scr%.5d.png", i); // scr#####.png

		HANDLE hFile = CreateFileA(fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE) {
			return SaveScreen(fileName);
		}
		CloseHandle(hFile);
	}
	return 1;
}

static __declspec(naked) void dump_screen_hack_replacement() {
	__asm {
		push ecx;
		push edx;
		call fo::funcoffs::game_screendump_; // call ds:[FO_VAR_screendump_func];
		pop  edx;
		pop  ecx;
		retn;
	}
}

void Graphics::init() {
	int gMode = (HRP::Setting::ExternalEnabled()) // avoid mode mismatch between ddraw.ini and another ini file
	          ? IniReader::GetIntDefaultConfig("Graphics", "Mode", 0)
	          : IniReader::GetConfigInt("Graphics", "Mode", 0);
	if (gMode >= 4) Graphics::mode = gMode;

	if (extWrapper || Graphics::mode < 0 || Graphics::mode > 6) {
		Graphics::mode = 0;
	}
	Graphics::IsWindowedMode = (Graphics::mode == 2 || Graphics::mode == 3 || Graphics::mode >= 5);

	if (Graphics::mode >= 4) {
		dlog("Applying DX9 graphics patch.", DL_INIT);
#define _DLL_NAME "d3dx9_43.dll"
		HMODULE h = LoadLibraryExA(_DLL_NAME, 0, LOAD_LIBRARY_AS_DATAFILE);
		if (!h) {
			dlogr(" Failed", DL_INIT);
			MessageBoxA(0, "You have selected DirectX graphics mode, but " _DLL_NAME " is missing.\n"
			               "Switch back to DirectDraw (Mode=0), or install an up-to-date version of DirectX 9.0c.", 0, MB_TASKMODAL | MB_ICONERROR);
#undef _DLL_NAME
			ExitProcess(-1);
		}
		FreeLibrary(h);

		SafeWrite8(0x50FB6B, '2'); // Set call DirectDrawCreat2
		HookCall(0x44260C, game_init_hook);

		MakeJump(fo::funcoffs::GNW95_SetPaletteEntries_ + 1, GNW95_SetPaletteEntries_replacement); // 0x4CB310
		MakeJump(fo::funcoffs::GNW95_SetPalette_, GNW95_SetPalette_replacement); // 0x4CB568

		// Replace the screenshot saving implementation for sfall DirectX 9
		HookCall(0x443EF3, game_screendump_hook);
		MakeJump(0x4C8F4C, dump_screen_hack_replacement);

		if (HRP::Setting::VersionIsValid) {
			// Patch HRP to show the mouse cursor over the window title
			if (Graphics::mode == 5) SafeWrite8(HRP::Setting::GetAddress(0x10027142), CodeType::JumpShort);

			// Patch HRP to fix the issue of displaying a palette color with index 255 for images (splash screens, ending slides)
			SafeWrite8(HRP::Setting::GetAddress(0x1000F8C7), CodeType::JumpShort);
		}

		textureFilter = IniReader::GetConfigInt("Graphics", "TextureFilter", 1);
		dlogr(" Done", DL_INIT);

		dShowMovies = Movies::DirectShowMovies();

		LoadGameHook::OnGameReset() += []() {
			ForceGraphicsRefresh(0); // disable refresh
		};
	} else if (HRP::Setting::IsEnabled()) {
		DirectDraw::init();
	}
	if (Graphics::mode == 5 || Graphics::mode == 2) WinProc::SetMoveKeys();

	if (HRP::Setting::IsEnabled()) {
		HRP::MoviesScreen::SetDrawMode(Graphics::mode < 4);

		// Reassign the WindowProc function to avoid an unnecessary jump from the engine code
		LoadGameHook::OnBeforeGameInit() += []() { WinProc::SetWindowProc(); };
	}

	// Set the maximum number of BMP screenshots to 10k (was 100k)
	SafeWriteBatch<DWORD>(10000, {0x4C908B, 0x4C9093}); // default_screendump_

	WindowRender::init();

	LoadGameHook::OnBeforeGameClose() += []() { WinProc::SavePosition(Graphics::mode); };
}

void Graphics::exit() {
	if (Graphics::mode >= 4) {
		CoUninitialize();
	} else {
		DirectDraw::exit();
	}
}

}

// This should be in global namespace
HRESULT __stdcall FakeDirectDrawCreate2(void* a, IDirectDraw** b, void* c) {
	return sfall::InitFakeDirectDrawCreate(a, b, c);
}
