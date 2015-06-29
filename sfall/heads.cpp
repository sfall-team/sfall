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

#include "main.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <hash_map>

void _stdcall SetHeadTex(IDirect3DTexture9* tex, int width, int height, int xoff, int yoff);

extern IDirect3DDevice9* d3d9Device;
static stdext::hash_map<__int64, IDirect3DTexture9**> texMap;
typedef stdext::hash_map<__int64, IDirect3DTexture9**> :: iterator tex_itr;
typedef stdext::hash_map<__int64, IDirect3DTexture9**> :: const_iterator tex_citr;
static BYTE overridden=0;

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
struct Frame {
	WORD width;
	WORD height;
	DWORD size;
	WORD xoffset;
	WORD yoffset;
	BYTE data[1];
};
#pragma pack(pop)

static const DWORD _frame_pointer=0x419880;
static Frame* FramePointer(const Frm* frm, int frameno) {
	Frame* result;
	__asm {
		mov eax, frm;
		mov edx, frameno;
		xor ebx, ebx;
		call _frame_pointer;
		mov result, eax;
	}
	return result;
}

static void LoadFrm(Frm* frm) {
	tex_citr itr=texMap.find(frm->key);
	if(itr==texMap.end()) {
		//Load textures
		char buf[MAX_PATH], buf2[MAX_PATH];
		strcpy(buf, "data\\art\\heads\\");
		strncat(buf, frm->path, 8);
		strcat(buf, "\\%d.png");
		IDirect3DTexture9** textures=new IDirect3DTexture9*[frm->frames];
		for(int i=0;i<frm->frames;i++) {
			sprintf(buf2, buf, i);
			if(FAILED(D3DXCreateTextureFromFileExA(d3d9Device, buf2, 0, 0, 1, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, 0, &textures[i]))) {
				for(int j=0;j<i;j++) textures[j]->Release();
				delete[] textures;
				frm->broken=1;
				return;
			}
		}
		frm->textures=textures;
		texMap[frm->key]=textures;
	} else {
		//Use preloaded textures
		frm->textures=itr->second;
	}
	//mask image
	for(int i=0;i<frm->frames;i++) {
		Frame* frame=FramePointer(frm, i);
		if(frm->bakedBackground) {
			memset(frame->data, 0x30, frame->size);
		} else {
			for(DWORD j=0;j<frame->size;j++) {
				if(frame->data[j]) frame->data[j]=0x30;
			}
		}
	}
	frm->loaded=1;
}

static void _stdcall DrawFrmHookInternal(Frm* frm, int frameno) {
	if(!frm) return;
	if(frm->magic==0xabcd&&!frm->broken) {
		if(!frm->loaded) LoadFrm(frm);
		if(frm->broken) return;
		Frame* frame=FramePointer(frm, frameno);
		SetHeadTex(frm->textures[frameno], frame->width, frame->height, frame->xoffset+frm->xshift, frame->yoffset+frm->yshift);
		overridden=!frm->showHighlights;
	} else overridden=0;
}

static const DWORD gdDisplayFrameRet=0x44AD06;
static void __declspec(naked) DrawFrmHook() {
	__asm {
		push edx;
		push eax;
		push edx;
		push eax;
		call DrawFrmHookInternal;
		pop eax;
		pop edx;
		sub esp, 0x38;
		mov esi, eax;
		jmp gdDisplayFrameRet;
	}
}

static const DWORD EndSpeechHookRet=0x447299;
void __declspec(naked) EndSpeechHook() {
	__asm {
		push label;
		push ebx;
		push ecx;
		push edx;
		push edi;
		push ebp;
		jmp EndSpeechHookRet;
label:
		xor eax, eax;
		push eax;
		push eax;
		push eax;
		push eax;
		push eax;
		call SetHeadTex;
		retn;
	}
}

static const DWORD _talk_to_translucent_trans_buf_to_buf=0x44AC68;
static void __declspec(naked) TransTalkHook() {
	__asm {
		cmp overridden, 0;
		jne skip;
		jmp _talk_to_translucent_trans_buf_to_buf;
skip:
		retn 0x18;
	}
}

void HeadsInit() {
	if(!GetPrivateProfileInt("Graphics", "Use32BitHeadGraphics", 0, ini)) return;
	HookCall(0x44AFB4, &TransTalkHook);
	HookCall(0x44B00B, &TransTalkHook);
	MakeCall(0x44AD01, &DrawFrmHook, true);
	MakeCall(0x447294, &EndSpeechHook, true);
}