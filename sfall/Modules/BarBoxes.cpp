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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "BarBoxes.h"

namespace sfall
{

static const DWORD DisplayBoxesRet1 = 0x4615A8;
static const DWORD DisplayBoxesRet2 = 0x4615BE;
static const DWORD SetIndexBoxRet   = 0x4612E8;

#define sSize    (12)
struct sBox {
	DWORD msg;
	DWORD colour;
	void* mem;
};
static sBox boxes[10];
static DWORD boxesEnabled[5];

#define tSize    (28)
struct tBox {
	DWORD hasText;
	DWORD color;
	char text[tSize - 8];
};
static tBox boxText[5];

static DWORD clrBakup[5];
static bool setCustomBoxText;

static const DWORD bboxMemAddr[] = {
	0x461266, 0x4612AC, 0x461374, 0x4613E8, 0x461479, 0x46148C, 0x4616BB,
};

static void __declspec(naked) DisplayBoxesHook() {
	__asm {
		mov ebx, 0;
start:
		mov eax, boxesEnabled[ebx * 4];
		test eax, eax;
		jz next;
		lea eax, [ebx + 5];
		call fo::funcoffs::add_bar_box_;
		add esi, eax;
next:
		inc ebx;
		cmp ebx, 5;
		jne start;
		cmp esi, 1;
		jle fail;
		jmp DisplayBoxesRet1;
fail:
		jmp DisplayBoxesRet2;
	}
}

static void __declspec(naked) BarBoxesTextHack() {
	__asm {
		//mov ecx, [esp + 0x440 - 0x1C];
		push ecx;                      // ecx = BoxIndex
		sub ecx, 5;
		imul ecx, tSize;
		cmp boxText[ecx], 1;           // .hasText
		jnz end;
		// get color
		mov ebx, boxText[ecx + 4];     // .color
		// set text
		lea eax, [boxText + ecx + 8];  // .text
		// set color
		pop ecx;
		imul ecx, sSize;
		cmp boxes[ecx + 4], 2;         // .colour
		jb skip;
		mov [esp + 0x440 - 0x20], ebx; // Color
skip:
		retn;
end:
		add esp, 4;
		jmp fo::funcoffs::getmsg_;
	}
}

static void __declspec(naked) BarBoxesIndexHack() {
	__asm {
		mov eax, ds:[0x4612E2]; // fontnum
		mov ecx, 5;             // start index
		mov edx, ecx;
		jmp SetIndexBoxRet;
	}
}

static void ReconstructBarBoxes() {
	__asm {
		call fo::funcoffs::refresh_box_bar_win_;
		call fo::funcoffs::reset_box_bar_win_;
		call fo::funcoffs::construct_box_bar_win_;
	}
}

static void ResetBoxes() {
	for (int i = 0; i < 5; i++) {
		boxesEnabled[i] = 0;
		boxes[i + 5].colour = clrBakup[i];
	}

	if (!setCustomBoxText) return;

	//Restore boxes
	SafeWrite32(0x461343, 0x00023D05); // call getmsg_
	ReconstructBarBoxes();
	SafeWrite8(0x461243, 0x31);
	SafeWrite32(0x461244, 0x249489D2);

	setCustomBoxText = false;
}

void BarBoxes::SetText(int box, const char* text, DWORD color) {
	boxes[box].colour = color;
	box -= 5;
	boxText[box].hasText = 1;
	strncpy_s(boxText[box].text, text, _TRUNCATE);

	DWORD clr;
	switch (color) {
	case 2:
		clr = fo::var::WhiteColor;
		break;
	case 3:
		clr = fo::var::YellowColor;
		break;
	case 4:
		clr = fo::var::PeanutButter;
		break;
	case 5:
		clr = fo::var::BlueColor;
		break;
	case 6:
		clr = fo::var::GoodColor;
		break;
	default:
		clr = fo::var::GreenColor;
	}
	boxText[box].color = clr;

	int enabled[5];
	for (int i = 0; i < 5; i++) {
		enabled[i] = boxesEnabled[i];
		boxesEnabled[i] = 0;
	}

	if (!setCustomBoxText) {
		MakeCall(0x461342, BarBoxesTextHack);
		MakeJump(0x461243, BarBoxesIndexHack);
		setCustomBoxText = true;
	}

	ReconstructBarBoxes();

	for (int i = 0; i < 5; i++) {
		boxesEnabled[i] = enabled[i];
	}
	__asm call fo::funcoffs::refresh_box_bar_win_;
}

void BarBoxes::init() {
	SafeWriteBatch<DWORD>((DWORD)boxes + 8, bboxMemAddr); //.mem
	SafeWrite32(0x4612FE, (DWORD)boxes + 4); //.colour
	SafeWrite32(0x46133C, (DWORD)boxes);     //.msg

	int size = sSize * 10;
	memset(boxes, 0, size);
	memset(boxesEnabled, 0, 5 * 4);
	memcpy(boxes, (void*)0x518FE8, sSize * 5);

	for (int i = 5; i < 10; i++) {
		boxes[i].msg = 100 + i;
	}

	SafeWrite8(0x46127C, 10);
	SafeWrite8(0x46140B, 10);
	SafeWrite8(0x461495, size);

	MakeJump(0x4615A3, DisplayBoxesHook);
	auto boxBarColors = GetConfigString("Misc", "BoxBarColours", "", 6);
	if (boxBarColors.size() >= 5) {
		for (int i = 0; i < 5; i++) {
			if (boxBarColors[i] == '1') {
				boxes[i + 5].colour = 1;
				clrBakup[i] = 1;
			} else {
				clrBakup[i] = 0;
			}
		}
	}

	LoadGameHook::OnGameReset() += ResetBoxes;
}

int _stdcall GetBox(int i) {
	if (i < 5 || i > 9) return 0;
	return boxesEnabled[i - 5];
}

void _stdcall AddBox(int i) {
	if (i < 5 || i > 9) return;
	boxesEnabled[i - 5] = 1;
}

void _stdcall RemoveBox(int i) {
	if (i < 5 || i > 9) return;
	boxesEnabled[i - 5] = 0;
}

}
