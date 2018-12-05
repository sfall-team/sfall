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

int BarBoxes::boxCount;      // total count box
static int actualBoxCount;
static int sizeBox;

#define sSize    (12)
struct sBox {
	DWORD msg;
	DWORD colour;
	void* mem;
};

#define tSize    (25)
struct tBox {
	bool  hasText;
	DWORD color;
	char  text[tSize - 5];
};

static sBox*  boxes;
static tBox*  boxText;
static bool*  boxesEnabled;
static DWORD* colorBakup;

static bool setCustomBoxText;

static const DWORD bboxMemAddr[] = {
	0x461266, 0x4612AC, 0x461374, 0x4613E8, 0x461479, 0x46148C, 0x4616BB,
};

static const DWORD DisplayBoxesRet1 = 0x4615A8;
static const DWORD DisplayBoxesRet2 = 0x4615BE;
static void __declspec(naked) DisplayBoxesHack() {
	__asm {
		mov  edx, [boxesEnabled];
		mov  ebx, 0;
start:
		mov  al, byte ptr [edx][ebx];
		test al, al;
		jz   next;
		lea  eax, [ebx + 5];
		call fo::funcoffs::add_bar_box_;
		add  esi, eax;
next:
		inc  ebx;
		cmp  ebx, actualBoxCount;
		jne  start;
		// engine code
		cmp  esi, 1;
		jle  fail;
		jmp  DisplayBoxesRet1;
fail:
		jmp  DisplayBoxesRet2;
	}
}

static void __declspec(naked) BarBoxesTextHack() {
	__asm {
		push ecx;                           // ecx = BoxIndex
		sub  ecx, 5;
		imul ecx, tSize;
		mov  esi, [boxText];                // boxText addr
		cmp  byte ptr [esi][ecx], 1;        // .hasText
		jnz  end;
		// get color
		mov  ebx, dword ptr [esi][ecx + 1]; // .color
		// set text
		lea  eax, [esi + ecx + 5];          // .text
		// set color
		pop  ecx;
		imul ecx, sSize;
		mov  esi, [boxes];                  // boxes addr
		cmp  dword ptr [esi][ecx + 4], 2;   // .colour
		jb   skip;
		mov  [esp + 0x440 - 0x20], ebx;     // Color
skip:
		retn;
end:
		add esp, 4;
		jmp fo::funcoffs::getmsg_;
	}
}

static const DWORD SetIndexBoxRet   = 0x4612E8;
static void __declspec(naked) BarBoxesIndexHack() {
	__asm {
		mov eax, ds:[0x4612E2]; // fontnum
		mov ecx, 5;             // start index
		mov edx, ecx;
		jmp SetIndexBoxRet;
	}
}

static const DWORD SizeLoopBoxRet = 0x461477;
static const DWORD ExitLoopBoxRet = 0x461498;
static void __declspec(naked) BarBoxesSizeHack() {
	__asm {
		cmp edx, sizeBox;
		jz  exitLoop;
		jmp SizeLoopBoxRet;
exitLoop:
		jmp ExitLoopBoxRet;
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
	for (int i = 0; i < actualBoxCount; i++) {
		boxesEnabled[i] = false;
		boxes[i + 5].colour = colorBakup[i];
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
	boxText[box].hasText = true;
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

	bool* enabled = new bool[actualBoxCount];
	for (int i = 0; i < actualBoxCount; i++) {
		enabled[i] = boxesEnabled[i];
		boxesEnabled[i] = false;
	}

	if (!setCustomBoxText) {
		MakeCall(0x461342, BarBoxesTextHack);   // construct_box_bar_win_
		MakeJump(0x461243, BarBoxesIndexHack);  // construct_box_bar_win_
		setCustomBoxText = true;
	}

	ReconstructBarBoxes();

	for (int i = 0; i < actualBoxCount; i++) {
		boxesEnabled[i] = enabled[i];
	}
	delete[] enabled;

	__asm call fo::funcoffs::refresh_box_bar_win_;
}

void BarBoxes::init() {

	boxCount = 5 + GetConfigInt("Misc", "BoxBarCount", 5);
	if (boxCount < 10)  boxCount = 10;
	if (boxCount > 100) boxCount = 100;

	actualBoxCount = boxCount - 5;
	sizeBox = sSize * boxCount;

	boxes        = new sBox[boxCount]();
	boxText      = new tBox[actualBoxCount]();
	boxesEnabled = new bool[actualBoxCount]();
	colorBakup   = new DWORD[actualBoxCount]();

	memcpy(boxes, (void*)0x518FE8, sSize * 5);

	for (int i = 5; i < boxCount; i++) {
		boxes[i].msg = 100 + i;
	}

	auto boxBarColors = GetConfigString("Misc", "BoxBarColours", "", actualBoxCount + 1);
	int size = boxBarColors.size();
	for (int i = 0; i < size; i++) {
		if (boxBarColors[i] == '1') {
			boxes[i + 5].colour = 1;
			colorBakup[i] = 1;
		}
	}

	SafeWriteBatch<DWORD>((DWORD)boxes + 8, bboxMemAddr); //.mem
	SafeWrite32(0x4612FE, (DWORD)boxes + 4);              //.colour
	SafeWrite32(0x46133C, (DWORD)boxes);                  //.msg

	SafeWrite8(0x46127C, boxCount);
	SafeWrite8(0x46140B, boxCount);

	if (boxCount > 10) {
		MakeJump(0x461493, BarBoxesSizeHack);
	} else {
		SafeWrite8(0x461495, sizeBox);
	}
	MakeJump(0x4615A3, DisplayBoxesHack);

	LoadGameHook::OnGameReset() += ResetBoxes;
}

bool BarBoxes::GetBox(int i) {
	if (i < 5 || i > BarBoxes::MaxBox()) return false;
	return boxesEnabled[i - 5];
}

void BarBoxes::AddBox(int i) {
	if (i < 5 || i > BarBoxes::MaxBox()) return;
	boxesEnabled[i - 5] = 1;
	_asm call fo::funcoffs::refresh_box_bar_win_;
}

void BarBoxes::RemoveBox(int i) {
	if (i < 5 || i > BarBoxes::MaxBox()) return;
	boxesEnabled[i - 5] = 0;
	_asm call fo::funcoffs::refresh_box_bar_win_;
}

void BarBoxes::exit() {
	delete[] boxes;
	delete[] boxText;
	delete[] boxesEnabled;
	delete[] colorBakup;
}

}
