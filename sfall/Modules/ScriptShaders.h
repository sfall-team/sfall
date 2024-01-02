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

#pragma once

#include "Module.h"
#include "Graphics.h"

namespace sfall
{

class ScriptShaders : public Module {
public:
	const char* name() { return "ScriptShaders"; }
	void init();

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
