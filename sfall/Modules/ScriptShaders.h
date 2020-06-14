#pragma once

#include "Module.h"
#include "Graphics.h"

namespace sfall
{

class ScriptShaders : public Module {
public:
	const char* name() { return "ScriptShaders"; }
	void init();
	//void exit() override;

	static size_t Count();

	static void Refresh(IDirect3DSurface9* sSurf1, IDirect3DSurface9* sSurf2, IDirect3DTexture9* sTex2);
	static void OnResetDevice();
	static void OnLostDevice();
	static void Release();

	static void LoadGlobalShader();
};

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
