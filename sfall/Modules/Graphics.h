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

#include "Module.h"

namespace sfall
{

class Graphics : public Module {
public:
	const char* name() { return "Graphics"; }
	void init();

	static DWORD mode;
};

int _stdcall GetShaderVersion();
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

void RefreshGraphics();
void GetFalloutWindowInfo(DWORD* width, DWORD* height, HWND* window);

void SetMovieTexture(void* tex);
void _stdcall PlayMovieFrame();

}
