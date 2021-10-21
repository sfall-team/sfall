#pragma once

#include "Graphics.h"

namespace sfall
{

void ScriptShaders_Init();
void ResetShaders();

size_t ScriptShaders_Count();

void ScriptShaders_Refresh(IDirect3DSurface9* sSurf1, IDirect3DSurface9* sSurf2, IDirect3DTexture9* sTex2);
void ScriptShaders_OnResetDevice();
void ScriptShaders_OnLostDevice();
void ScriptShaders_Release();

void ScriptShaders_LoadGlobalShader();

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

}
