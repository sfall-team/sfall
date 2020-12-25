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

extern DWORD GraphicsMode;
extern DWORD GPUBlt;

extern IDirect3D9* d3d9;
extern IDirect3DDevice9* d3d9Device;

extern bool Gfx_PlayAviMovie;
extern bool Gfx_AviMovieWidthFit;

void Graphics_Init();
void Graphics_Exit();
void Graphics_OnGameLoad();

HWND Gfx_GetFalloutWindowInfo(RECT* rect);
long Gfx_GetGameWidthRes();
long Gfx_GetGameHeightRes();

void Gfx_SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff);
void Gfx_SetHeadTechnique();
void Gfx_SetDefaultTechnique();

void Gfx_ShowMovieFrame(IDirect3DTexture9* movieTex);

void Gfx_SetMovieTexture(bool state);
HRESULT Gfx_CreateMovieTexture(D3DSURFACE_DESC &desc);
void Gfx_ReleaseMovieTexture();

void Gfx_RefreshGraphics();

void GameRender_CreateOverlaySurface(WINinfo* win, long winType);
void GameRender_DestroyOverlaySurface(WINinfo* win);
void GameRender_ClearOverlay(WINinfo* win);
void GameRender_ClearOverlay(WINinfo* win, sRectangle &rect);
BYTE* GameRender_GetOverlaySurface(WINinfo* win);

int __stdcall GetShaderVersion();
int __stdcall LoadShader(const char*);
void __stdcall ActivateShader(DWORD);
void __stdcall DeactivateShader(DWORD);
void __stdcall FreeShader(DWORD);
void __stdcall SetShaderMode(DWORD d, DWORD mode);

void __stdcall SetShaderInt(DWORD d, const char* param, int value);
void __stdcall SetShaderFloat(DWORD d, const char* param, float value);
void __stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4);

int __stdcall GetShaderTexture(DWORD d, DWORD id);
void __stdcall SetShaderTexture(DWORD d, const char* param, DWORD value);
