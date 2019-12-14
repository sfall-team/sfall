/*
 *    sfall
 *    Copyright (C) 2010  The sfall team
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
#include <hash_map>

#include "main.h"

#include "FalloutEngine.h"
#include "Graphics.h"

#include "TalkingHeads.h"

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

typedef stdext::hash_map<__int64, TextureData> :: iterator tex_itr;
typedef stdext::hash_map<__int64, TextureData> :: const_iterator tex_citr;

static stdext::hash_map<__int64, TextureData> texMap;

static const char* headSuffix[] = { "gv", "gf", "gn", "ng", "nf", "nb", "bn", "bf", "bv", "gp", "np", "bp" };

static BYTE showHighlights;
static long reactionID;

bool Use32BitTalkingHeads = false;

/*             Head FID
 0-000-1000-00000000-0000-000000000000
   ID3 Type   ID2    ID1   .lst index
*/
static bool GetHeadFrmName(char* name) {
	int headFid = (*(DWORD*)_lips_draw_head)
				? *ptr_lipsFID
				: *ptr_fidgetFID;
	int index = headFid & 0xFFF;
	if (index >= ptr_art[8].total) return true; // OBJ_TYPE_HEAD
	int ID2 = (*(DWORD*)_fidgetFp) ? (headFid & 0xFF0000) >> 16 : reactionID;
	if (ID2 > 11) return true;
	int ID1 = (ID2 == 1 || ID2 == 4 || ID2 == 7) ? (headFid & 0xF000) >> 12 : -1;
	//if (ID1 > 3) ID1 = 3;
	const char* headLst = ptr_art[8].names; // OBJ_TYPE_HEAD
	char* fmt = (ID1 != -1) ? "%s%s%d" : "%s%s";
	_snprintf(name, 8, fmt, &headLst[13 * index], headSuffix[ID2], ID1);
	return false;
}

static void StrAppend(char* buf, const char* str, int pos) {
	int i = 0;
	while (pos < MAX_PATH && str[i]) buf[pos++] = str[i++];
	buf[pos] = str[i]; // copy '\0'
}

static bool LoadFrm(Frm* frm) { // backporting WIP
	if (!frm->key && GetHeadFrmName(frm->path)) {
		frm->broken = 1;
		return false;
	}
	tex_citr itr = texMap.find(frm->key);
	if (itr == texMap.end()) {
		// Loading head frames textures
		*(DWORD*)_bk_disabled = 1;
		char buf[MAX_PATH];
		int pathLen = sprintf_s(buf, "%s\\art\\heads\\%s\\", *ptr_patches, frm->path);
		if (pathLen > 250) return false;
		IDirect3DTexture9** textures = new IDirect3DTexture9*[frm->frames];
		for (int i = 0; i < frm->frames; i++) {
			sprintf(&buf[pathLen], "%d.png", i);
			if (FAILED(D3DXCreateTextureFromFileExA(d3d9Device, buf, 0, 0, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &textures[i]))) {
				for (int j = 0; j < i; j++) textures[j]->Release();
				delete[] textures;
				frm->broken = 1;
				*(DWORD*)_bk_disabled = 0;
				return false;
			}
			ProcessBk(); // eliminate lag when loading textures
		}
		if (frm->magic != 0xABCD) { // frm file not patched
			StrAppend(buf, "highlight.off", pathLen);
			if (GetFileAttributes(buf) != INVALID_FILE_ATTRIBUTES) frm->showHighlights = 1; // disable highlights
			StrAppend(buf, "background.off", pathLen);
			if (GetFileAttributes(buf) != INVALID_FILE_ATTRIBUTES) frm->bakedBackground = 1; // fills entire frame surface with a key-color
		}
		frm->textures = textures;
		texMap.insert(std::make_pair(frm->key, TextureData(textures, frm->showHighlights, frm->bakedBackground, frm->frames)));
		*(DWORD*)_bk_disabled = 0;
	} else {
		// Use preloaded textures
		frm->textures = itr->second.textures;
		frm->showHighlights = itr->second.showHighlights;
		frm->bakedBackground = itr->second.bakedBackground;
	}
	// make mask image
	for (int i = 0; i < frm->frames; i++) {
		FrmFrameData* frame = FramePtr((FrmHeaderData*)frm, i, 0);
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

static void __fastcall DrawHeadFrame(Frm* frm, int frameno) {
	if (frm && !frm->broken) {
		if (!frm->loaded && !LoadFrm(frm)) goto loadFail;
		FrmFrameData* frame = FramePtr((FrmHeaderData*)frm, frameno, 0);
		Gfx_SetHeadTex(frm->textures[frameno], frame->width, frame->height, frame->x + frm->xshift, frame->y + frm->yshift/*, (frm->showHighlights == 2)*/);
		showHighlights = frm->showHighlights;
		return;
	}
loadFail:
	showHighlights = 0; // show vanilla highlights
	Gfx_SetDefaultTechnique();
}

static const DWORD gdDisplayFrameRet = 0x44AD06;
static void __declspec(naked) gdDisplayFrame_hack() {
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

void __declspec(naked) gdDestroyHeadWindow_hack() {
	__asm {
		call Gfx_SetDefaultTechnique;
		mov  showHighlights, 0;
		pop  ebp;
		pop  edi;
		pop  edx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static void __declspec(naked) TransTalkHook() {
	__asm {
		test showHighlights, 0xFF;
		jnz  skip;
		jmp  talk_to_translucent_trans_buf_to_buf_;
skip:
		retn 0x18;
	}
}

static void __declspec(naked) gdPlayTransition_hook() {
	__asm {
		mov reactionID, ebx;
		jmp art_id_;
	}
}

static void __declspec(naked) gdialogInitFromScript_hook() {
	__asm {
		cmp dword ptr ds:[_dialogue_head], -1;
		jnz noScroll;
		jmp tile_scroll_to_;
noScroll:
		retn;
	}
}

void TalkingHeadsSetup() {
	if (!GPUBlt) return;

	HookCall(0x44AFB4, TransTalkHook);
	HookCall(0x44B00B, TransTalkHook);
	MakeJump(0x44AD01, gdDisplayFrame_hack); // Draw Frm
	MakeJump(0x4472F8, gdDestroyHeadWindow_hack);
	HookCall(0x44768B, gdPlayTransition_hook);
}

void TalkingHeadsInit() {
	// Disable centering the screen if NPC has talking head
	HookCall(0x445224, gdialogInitFromScript_hook);

	if (GraphicsMode && GetConfigInt("Graphics", "Use32BitHeadGraphics", 0)) {
		Use32BitTalkingHeads = true;
	}
}

void TalkingHeadsExit() {
	if (!texMap.empty()) {
		for (tex_citr it = texMap.begin(); it != texMap.end(); ++it) {
			for (int i = 0; i < it->second.frames; i++) {
				it->second.textures[i]->Release();
			}
			delete[] it->second.textures;
		}
	}
}
