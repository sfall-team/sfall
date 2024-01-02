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

#include "Graphics.h"
#include "LoadGameHook.h"

#include "ScriptShaders.h"

namespace sfall
{

#define SAFERELEASE(a) { if (a) { a->Release(); a = nullptr; } }

static size_t shadersSize;
static bool globalShadersActive = false;

struct GlobalShader {
	std::string gShaderFile;
	bool badShader = false;

	GlobalShader(std::string file) {
		gShaderFile = std::move(file);
	}
};

struct sShader {
	ID3DXEffect* Effect;
	D3DXHANDLE ehTicks;
	DWORD mode;
	DWORD mode2;
	bool Active;

	sShader() : Effect(0), ehTicks(0), mode(0), mode2(0), Active(false) {}
};

static std::vector<GlobalShader> gShaderFiles;
static std::vector<sShader> shaders;
static std::vector<IDirect3DTexture9*> shaderTextures;

size_t ScriptShaders::Count() {
	return shadersSize;
}

void __stdcall SetShaderMode(DWORD d, DWORD mode) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	if (mode & 0x80000000) {
		shaders[d].mode2 = mode ^ 0x80000000;
	} else {
		shaders[d].mode = mode;
	}
}

int __stdcall LoadShader(const char* file) {
	if (Graphics::mode < 4 || strstr(file, "..") || strstr(file, ":")) return -1;
	char buf[MAX_PATH];
	sprintf_s(buf, "%s\\shaders\\%s", fo::var::master_db_handle->path, file); // fo::var::patches
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

	shader.Effect->SetFloatArray("rcpres", Graphics::rcpresGet(), 2);

	for (int i = 1; i < 128; i++) {
		const char* name;
		IDirect3DTexture9* tex;

		sprintf(buf, "texname%d", i);
		if (FAILED(shader.Effect->GetString(buf, &name))) break;
		sprintf_s(buf, "%s\\art\\stex\\%s", fo::var::master_db_handle->path, name); // fo::var::patches
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

void ScriptShaders::LoadGlobalShader() {
	if (!globalShadersActive) return;
	for (auto &shader : gShaderFiles) {
		if (shader.badShader) continue;
		long index = LoadShader(shader.gShaderFile.c_str());
		if (index != -1) {
			shaders[index].Effect->SetInt("w", Graphics::GetGameWidthRes());
			shaders[index].Effect->SetInt("h", Graphics::GetGameHeightRes());
			shaders[index].Active = true;
			dlog_f("Global shader file %s is loaded.\n", DL_INIT, shader.gShaderFile.c_str());
		} else {
			shader.badShader = true;
		}
	}
}

void __stdcall ActivateShader(DWORD d) {
	if (d < shadersSize && shaders[d].Effect) shaders[d].Active = true;
}

void __stdcall DeactivateShader(DWORD d) {
	if (d < shadersSize) shaders[d].Active = false;
}

int __stdcall GetShaderTexture(DWORD d, DWORD id) {
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

void __stdcall FreeShader(DWORD d) {
	if (d < shadersSize) {
		SAFERELEASE(shaders[d].Effect);
		shaders[d].Active = false;
	}
}

void __stdcall SetShaderInt(DWORD d, const char* param, int value) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetInt(param, value);
}

void __stdcall SetShaderFloat(DWORD d, const char* param, float value) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetFloat(param, value);
}

void __stdcall SetShaderVector(DWORD d, const char* param, float f1, float f2, float f3, float f4) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	shaders[d].Effect->SetFloatArray(param, &f1, 4);
}

void __stdcall SetShaderTexture(DWORD d, const char* param, DWORD value) {
	if (d >= shadersSize || !shaders[d].Effect || value >= shaderTextures.size()) return;
	shaders[d].Effect->SetTexture(param, shaderTextures[value]);
}

static void ResetShaders() {
	for (DWORD d = 0; d < shadersSize; d++) SAFERELEASE(shaders[d].Effect);
	shaders.clear();
	shadersSize = 0;
	ScriptShaders::LoadGlobalShader();
}

void ScriptShaders::Refresh(IDirect3DSurface9* sSurf1, IDirect3DSurface9* sSurf2, IDirect3DTexture9* sTex2) {
	for (int i = shadersSize - 1; i >= 0; i--) {
		if (!shaders[i].Effect || !shaders[i].Active) continue;
		if (shaders[i].mode2 && !(shaders[i].mode2 & GetLoopFlags())) continue;
		if (shaders[i].mode & GetLoopFlags()) continue;

		if (shaders[i].ehTicks) shaders[i].Effect->SetInt(shaders[i].ehTicks, GetTickCount());

		UINT passes;
		shaders[i].Effect->Begin(&passes, 0);
		for (DWORD pass = 0; pass < passes; pass++) {
			shaders[i].Effect->BeginPass(pass);
			d3d9Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			shaders[i].Effect->EndPass();

			d3d9Device->StretchRect(sSurf1, 0, sSurf2, 0, D3DTEXF_NONE); // copy sSurf1 to sSurf2
			d3d9Device->SetTexture(0, sTex2);
		}
		shaders[i].Effect->End();
		d3d9Device->SetTexture(0, sTex2);
	}
}

void ScriptShaders::OnResetDevice() {
	for (DWORD d = 0; d < shadersSize; d++) {
		if (shaders[d].Effect) shaders[d].Effect->OnResetDevice();
	}
}

void ScriptShaders::OnLostDevice() {
	for (DWORD d = 0; d < shadersSize; d++) {
		if (shaders[d].Effect) shaders[d].Effect->OnLostDevice();
	}
}

void ScriptShaders::Release() {
	globalShadersActive = false;
	ResetShaders();
	for (DWORD d = 0; d < shaderTextures.size(); d++) shaderTextures[d]->Release();
	shaderTextures.clear();
}

void ScriptShaders::init() {
	if (Graphics::mode >= 4) {
		for each (const auto& shaderFile in IniReader::GetConfigList("Graphics", "GlobalShaderFile", "")) {
			if (shaderFile.length() > 3) gShaderFiles.push_back(GlobalShader(shaderFile));
		}
		globalShadersActive = !gShaderFiles.empty();

		LoadGameHook::OnGameReset() += ResetShaders;
	}
}

}
