
#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Graphics.h"
#include "LoadGameHook.h"

#include "ScriptShaders.h"

namespace sfall
{

static size_t shadersSize;

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

size_t ScriptShaders::Count() {
	return shadersSize;
}

void _stdcall SetShaderMode(DWORD d, DWORD mode) {
	if (d >= shadersSize || !shaders[d].Effect) return;
	if (mode & 0x80000000) {
		shaders[d].mode2 = mode ^ 0x80000000;
	} else {
		shaders[d].mode = mode;
	}
}

int _stdcall LoadShader(const char* path) {
	if (!Graphics::mode || strstr(path, "..") || strstr(path, ":")) return -1;
	char buf[MAX_PATH];
	sprintf_s(buf, "%s\\shaders\\%s", fo::var::patches, path);
	for (DWORD d = 0; d < shadersSize; d++) {
		if (!shaders[d].Effect) {
			if (FAILED(D3DXCreateEffectFromFile(d3d9Device, buf, 0, 0, 0, 0, &shaders[d].Effect, 0))) return -1;
			else return d;
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
		sprintf_s(buf, "%s\\art\\stex\\%s", fo::var::patches, name);
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

void ResetShaders() {
	for (DWORD d = 0; d < shadersSize; d++) SAFERELEASE(shaders[d].Effect);
	shaders.clear();
	shadersSize = 0;
}

void ScriptShaders::Refresh(IDirect3DSurface9* sSurf1, IDirect3DSurface9* sSurf2, IDirect3DTexture9* sTex2) {
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
	ResetShaders();
	for (DWORD d = 0; d < shaderTextures.size(); d++) shaderTextures[d]->Release();
	shaderTextures.clear();
}

void ScriptShaders::init() {
	if (Graphics::mode) {
		LoadGameHook::OnGameReset() += ResetShaders;
	}
}

}