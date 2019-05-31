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
};

int _stdcall LoadShader(const char*);
void _stdcall ActivateShader(DWORD);
void _stdcall DeactivateShader(DWORD);
void _stdcall FreeShader(DWORD);
void _stdcall SetShaderMode(DWORD d, DWORD mode);

void _stdcall SetShaderInt(DWORD d, const char* param, int value);
void _stdcall SetShaderFloat(DWORD d, const char* param, float value);
void _stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4);

int _stdcall GetShaderTexture(DWORD d, DWORD id);
void _stdcall SetShaderTexture(DWORD d, const char* param, DWORD value);

}
