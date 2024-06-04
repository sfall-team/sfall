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

#include <stdio.h>
#include <unordered_map>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "LoadGameHook.h"

#include "TalkingHeads.h"

namespace sfall
{

#pragma pack(push, 1)
struct Frm {
	DWORD version;
	WORD fps;
	WORD actionFrame;
	WORD frames;
	union {
		WORD xshifts[6];
		struct {
			WORD xshift;
			WORD magic;
			union {
			char path[8];
			__int64 key;
			};
		};
	};
	union {
		WORD yshifts[6];
		struct {
			WORD yshift;
			BYTE loaded;
			BYTE broken;
			IDirect3DTexture9** textures;
			BYTE bakedBackground;
			BYTE showHighlights;
		};
	};
};
#pragma pack(pop)

struct TextureData {
	IDirect3DTexture9** textures;
	BYTE showHighlights;
	BYTE bakedBackground;
	int frames;

	TextureData(IDirect3DTexture9** tex, BYTE show, BYTE baked, int frames)
		: textures(tex), showHighlights(show), bakedBackground(baked), frames(frames) {}
};

typedef std::unordered_map<__int64, TextureData>::iterator tex_itr;
typedef std::unordered_map<__int64, TextureData>::const_iterator tex_citr;

static std::unordered_map<__int64, TextureData> texMap;
static IDirect3DTexture9* texHighlight = nullptr;

static const char* headSuffix[] = { "gv", "gf", "gn", "ng", "nf", "nb", "bn", "bf", "bv", "gp", "np", "bp" };

static BYTE showHighlights;
static long reactionID;

/*             Head FID
 0-000-1000-00000000-0000-000000000000
   ID3 Type   ID2    ID1   .lst index
*/

static bool GetHeadFrmName(char* name) {
	int headFid = (fo::var::getInt(FO_VAR_lips_draw_head))
	            ? fo::var::lipsFID
	            : fo::var::fidgetFID;
	int index = headFid & 0xFFF;
	if (index >= fo::var::art[fo::OBJ_TYPE_HEAD].total) return true;
	int ID2 = (fo::var::getInt(FO_VAR_fidgetFp)) ? (headFid & 0xFF0000) >> 16 : reactionID;
	if (ID2 > 11) return true;
	int ID1 = (ID2 == 1 || ID2 == 4 || ID2 == 7) ? (headFid & 0xF000) >> 12 : -1;
	//if (ID1 > 3) ID1 = 3;
	const char* headLst = fo::var::art[fo::OBJ_TYPE_HEAD].names;
	char* fmt = (ID1 != -1) ? "%s%s%d" : "%s%s";
	_snprintf(name, 8, fmt, &headLst[13 * index], headSuffix[ID2], ID1);
	return false;
}

static void StrAppend(char* buf, const char* str, int pos) {
	int i = 0;
	while (pos < MAX_PATH && str[i]) buf[pos++] = str[i++];
	buf[pos] = str[i]; // copy '\0'
}

static bool LoadFrm(Frm* frm) {
	if (!frm->key && GetHeadFrmName(frm->path)) {
		frm->broken = 1;
		return false;
	}
	tex_citr itr = texMap.find(frm->key);
	if (itr == texMap.end()) {
		// Loading head frames textures
		fo::var::setInt(FO_VAR_bk_disabled) = 1;
		char buf[MAX_PATH];
		int pathLen = sprintf_s(buf, "%s\\art\\heads\\%s\\", fo::var::patches, frm->path);
		if (pathLen > 250) return false;

		if (!(GetFileAttributesA(frm->path) & FILE_ATTRIBUTE_DIRECTORY)) {
			frm->broken = 1;
			return false;
		}
		IDirect3DTexture9** textures = new IDirect3DTexture9*[frm->frames];
		for (int i = 0; i < frm->frames; i++) {
			sprintf(&buf[pathLen], "%d.png", i);
			if (FAILED(D3DXCreateTextureFromFileExA(d3d9Device, buf, 0, 0, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &textures[i]))) {
				for (int j = 0; j < i; j++) textures[j]->Release();
				delete[] textures;
				frm->broken = 1;
				fo::var::setInt(FO_VAR_bk_disabled) = 0;
				return false;
			}
			fo::func::process_bk(); // eliminate lag when loading textures
		}
		if (frm->magic != 0xABCD) { // frm file not patched
			StrAppend(buf, "highlight.off", pathLen);
			if (GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES) frm->showHighlights = 1; // disable all highlights
			if (!frm->showHighlights && texHighlight) {
				StrAppend(buf, "highlight.on", pathLen);
				if (GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES) frm->showHighlights = 2; // show textured highlights
			}
			StrAppend(buf, "background.off", pathLen);
			if (GetFileAttributesA(buf) != INVALID_FILE_ATTRIBUTES) frm->bakedBackground = 1; // fills entire frame surface with a key-color
		}
		frm->textures = textures;
		texMap.emplace(std::piecewise_construct, std::forward_as_tuple(frm->key),
		               std::forward_as_tuple(textures, frm->showHighlights, frm->bakedBackground, frm->frames));
		fo::var::setInt(FO_VAR_bk_disabled) = 0;
	} else {
		// Use preloaded textures
		frm->textures = itr->second.textures;
		frm->showHighlights = itr->second.showHighlights;
		frm->bakedBackground = itr->second.bakedBackground;
	}
	// make mask image
	for (int i = 0; i < frm->frames; i++) {
		fo::FrmFrameData* frame = fo::func::frame_ptr((fo::FrmHeaderData*)frm, i, 0);
		if (frm->bakedBackground) {
			memset(frame->data, 255, frame->size);
		} else {
			for (DWORD j = 0; j < frame->size; j++) {
				if (frame->data[j]) frame->data[j] = 255; // set index color
			}
		}
	}
	frm->loaded = 1;
	return true;
}

static struct DialogWinPos {
	long x = -1;
	long y;
} dialogWinPos;

static void __fastcall DrawHeadFrame(Frm* frm, int frameno) {
	if (frm && !frm->broken) {
		if (!frm->loaded && !LoadFrm(frm)) goto loadFail;
		fo::FrmFrameData* frame = fo::func::frame_ptr((fo::FrmHeaderData*)frm, frameno, 0);

		if (dialogWinPos.x == -1) {
			fo::Window* dialogWin = fo::func::GNW_find(fo::var::dialogueBackWindow);
			if (texHighlight) Graphics::SetHighlightTexture(texHighlight, dialogWin->rect.x, dialogWin->rect.y);
			dialogWinPos.x = dialogWin->rect.x;
			dialogWinPos.y = dialogWin->rect.y;
		}
		Graphics::SetHeadTex(frm->textures[frameno],
		                     frame->width,
		                     frame->height,
		                     frame->x + frm->xshift + dialogWinPos.x,
		                     frame->y + frm->yshift + dialogWinPos.y,
		                     (frm->showHighlights == 2)
		);
		showHighlights = frm->showHighlights;
		return;
	}
loadFail:
	showHighlights = 0; // show vanilla highlights
	Graphics::SetDefaultTechnique();
}

static __declspec(naked) void gdDisplayFrame_hack() {
	static const DWORD gdDisplayFrameRet = 0x44AD06;
	__asm {
		push edx;
		push eax;
		mov  ecx, eax;      // frm file
		call DrawHeadFrame; // edx - frameno
		pop  eax;
		pop  edx;
		sub  esp, 0x38;
		mov  esi, eax;
		jmp  gdDisplayFrameRet;
	}
}

__declspec(naked) void gdDestroyHeadWindow_hack() {
	__asm {
		call Graphics::SetDefaultTechnique;
		mov  showHighlights, 0;
		//mov  dword ptr ds:[dialogWinPos], -1; // uncomment if the dialog window position is supposed to change
		pop  ebp;
		pop  edi;
		pop  edx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static __declspec(naked) void TransTalkHook() {
	__asm {
		test showHighlights, 0xFF;
		jnz  skip;
		jmp  fo::funcoffs::talk_to_translucent_trans_buf_to_buf_;
skip:
		retn 0x18;
	}
}

static __declspec(naked) void gdPlayTransition_hook() {
	__asm {
		mov reactionID, ebx;
		jmp fo::funcoffs::art_id_;
	}
}

static __declspec(naked) void gdialogInitFromScript_hook() {
	__asm {
		cmp dword ptr ds:[FO_VAR_dialogue_head], -1;
		jnz noScroll;
		jmp fo::funcoffs::tile_scroll_to_;
noScroll:
		retn;
	}
}

static void TalkingHeadsInit() {
	if (!Graphics::GPUBlt) return;

	fo::var::setInt(FO_VAR_lips_draw_head) = 0; // fix for non-speaking heads
	HookCalls(TransTalkHook, {0x44AFB4, 0x44B00B});
	MakeJump(0x44AD01, gdDisplayFrame_hack); // Draw Frm
	MakeJump(0x4472F8, gdDestroyHeadWindow_hack);
	HookCall(0x44768B, gdPlayTransition_hook);

	// Load highlights texture
	char buf[MAX_PATH];
	sprintf_s(buf, "%s\\art\\stex\\highlight.png", fo::var::patches);
	if (!FAILED(D3DXCreateTextureFromFileExA(d3d9Device, buf, 0, 0, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &texHighlight))) {
		LoadGameHook::OnGameModeChange() += [](DWORD state) {
			static bool setHeadTech = false;
			if (showHighlights == 2) {
				if (GetLoopFlags() & DIALOGVIEW) {
					Graphics::SetDefaultTechnique();
					setHeadTech = true;
				} else if (setHeadTech) {
					Graphics::SetHeadTechnique();
					setHeadTech = false;
				}
			}
		};
	}
}

void TalkingHeads::init() {
	// Disable centering the screen if NPC has talking head
	HookCall(0x445224, gdialogInitFromScript_hook);

	if (Graphics::mode >= 4 && IniReader::GetConfigInt("Graphics", "Use32BitHeadGraphics", 0)) {
		LoadGameHook::OnAfterGameInit() += TalkingHeadsInit;
	}
}

void TalkingHeads::exit() {
	if (!texMap.empty()) {
		for (auto& frm : texMap) {
			for (int i = 0; i < frm.second.frames; i++) {
				frm.second.textures[i]->Release();
			}
			delete[] frm.second.textures;
		}
	}
	//if (texHighlight) texHighlight->Release(); // it seems that it is released in Graphics.cpp at code line: SAFERELEASE(gpuBltEffect)
}

}
