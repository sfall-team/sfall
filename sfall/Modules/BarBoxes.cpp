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

int BarBoxes::boxCount;                   // current box count
static int totalBoxCount, actualBoxCount; // total boxes and w/o vanilla
static int initCount = 5;                 // init config counter (5 - vanilla box)
static int sizeBox, setBoxIndex = 5;

#define sSize    (12)
static struct sBox {
	DWORD msg;
	DWORD colour;
	void* mem;
} *boxes;

#define tSize    (24)
static struct tBox {
	bool hasText;
	char color;
	char cfgColor;
	bool isActive;
	char text[tSize - 4];
} *boxText;

static bool setCustomBoxText;

static const DWORD bboxMemAddr[] = {
	0x461266, 0x4612AC, 0x461374, 0x4613E8, 0x461479, 0x46148C, 0x4616BB,
};

static const DWORD DisplayBoxesRet1 = 0x4615A8;
static const DWORD DisplayBoxesRet2 = 0x4615BE;
static void __declspec(naked) DisplayBoxesHack() {
	__asm {
		mov  edx, [boxText];
		xor  ebx, ebx;
start:
		imul eax, ebx, tSize;
		mov  al, byte ptr [edx][eax + 3];   // .isActive
		test al, al;
		jz   next;
		lea  eax, [ebx + 5];                // index box
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
		sub  ecx, 5;                        // subtract vanilla boxes
		imul ecx, tSize;
		mov  esi, [boxText];                // boxText addr
		cmp  byte ptr [esi][ecx], 1;        // .hasText
		je   customText;
		add  esp, 4;
		jmp  fo::funcoffs::getmsg_;
customText:
		// get color
		movzx ebx, byte ptr [esi][ecx + 1]; // .color
		// set text
		lea  eax, [esi + ecx + 4];          // .text
		// set color
		pop  ecx;                           // restore BoxIndex
		imul ecx, sSize;
		mov  esi, [boxes];                  // boxes addr
		cmp  dword ptr [esi][ecx + 4], 2;   // .colour
		jb   skip;
		mov  [esp + 0x440 - 0x20], ebx;     // set to Color
skip:
		retn;
	}
}

static const DWORD SetIndexBoxRet   = 0x4612E8;
static void __declspec(naked) BarBoxesIndexHack() {
	__asm {
		mov eax, ds:[0x4612E2]; // fontnum
		mov ecx, setBoxIndex;   // start index
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

static void ReconstructBarBoxes(int count) {
	SafeWrite8(0x46140B, count);
	__asm {
		//call fo::funcoffs::refresh_box_bar_win_;
		call fo::funcoffs::reset_box_bar_win_;
		call fo::funcoffs::construct_box_bar_win_;
	}
}

static void ResetBoxes() {
	for (int i = 0; i < actualBoxCount; i++) {
		boxText[i].isActive = false;
		boxText[i].hasText = false;
		boxes[i + 5].colour = boxText[i].cfgColor;
	}
	if (setBoxIndex == 5) return;

	// Restore boxes
	setBoxIndex = 5; // set start
	//SafeWrite32(0x461343, 0x00023D05); // call getmsg_
	ReconstructBarBoxes(totalBoxCount);
	//SafeWrite8(0x461243, 0x31);
	//SafeWrite32(0x461244, 0x249489D2);
}

void BarBoxes::SetText(int box, const char* text, DWORD color) {
	setBoxIndex = box;
	boxes[box].colour = color;
	box -= 5;
	boxText[box].hasText = true;
	strncpy_s(boxText[box].text, text, _TRUNCATE);

	char clr;
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
	case 7:
		clr = *(BYTE*)FO_VAR_DullPinkColor;
		break;
	default:
		clr = fo::var::GreenColor;
	}
	boxText[box].color = clr;

	if (!setCustomBoxText) {
		setCustomBoxText = true;
		MakeCall(0x461342, BarBoxesTextHack);   // construct_box_bar_win_
		MakeJump(0x461243, BarBoxesIndexHack);  // construct_box_bar_win_
	}
	ReconstructBarBoxes(setBoxIndex + 1);
}

static void SetEngine(int count) {
	SafeWriteBatch<DWORD>((DWORD)boxes + 8, bboxMemAddr); //.mem
	SafeWrite32(0x4612FE, (DWORD)boxes + 4);              //.colour
	SafeWrite32(0x46133C, (DWORD)boxes);                  //.msg

	sizeBox = sizeof(sBox) * count;

	static bool onlyOnce = false;
	if (onlyOnce) return;

	if (count > 10) {
		MakeJump(0x461493, BarBoxesSizeHack); // deconstruct_box_bar_win_
		onlyOnce = true;
	} else {
		SafeWrite8(0x461495, sizeBox);
	}
}

void BarBoxes::init() {

	initCount += GetConfigInt("Misc", "BoxBarCount", 5);
	if (initCount < 5)  initCount = 5; // exclude the influence of negative values from the config
	if (initCount > 100) initCount = 100;

	actualBoxCount = initCount - 5;

	boxes   = new sBox[initCount]();
	boxText = new tBox[actualBoxCount]();

	memcpy(boxes, (void*)0x518FE8, sSize * 5);
	for (int i = 5; i < initCount; i++) {
		boxes[i].msg = 100 + i;
	}

	auto boxBarColors = GetConfigString("Misc", "BoxBarColours", "", actualBoxCount + 1);
	for (size_t i = 0; i < boxBarColors.size(); i++) {
		if (boxBarColors[i] == '1') {
			boxes[i + 5].colour = 1; // red color
			boxText[i].cfgColor = 1;
		}
	}

	SetEngine(initCount);
	SafeWrite8(0x46127C, initCount); // for only init
	SafeWrite8(0x46140B, initCount);

	MakeJump(0x4615A3, DisplayBoxesHack);

	totalBoxCount = boxCount = initCount;

	LoadGameHook::OnGameReset() += []() {
		ResetBoxes();
		if (initCount != totalBoxCount) {
			boxCount = initCount;
			actualBoxCount = initCount - 5;
		}
	};
}

long BarBoxes::AddExtraBox() {
	if (boxCount < totalBoxCount) {
		actualBoxCount++;
		boxCount++;
		return MaxBox(); // just return the number of the previously added box
	}
	if (totalBoxCount > 126) return -1; // limit is exceeded

	void* data;
	__asm {
		mov  eax, 2730;
		call fo::funcoffs::mem_malloc_;
		mov  data, eax;
	}
	if (data == nullptr) return -1;   // error on memory allocation
	memcpy(data, boxes[1].mem, 2730); // copy image box to a new allocated memory

	sBox* boxes_new = new sBox[totalBoxCount + 1];
	memcpy(boxes_new, boxes, sizeof(sBox) * totalBoxCount);
	delete[] boxes;
	boxes = boxes_new;

	boxes[totalBoxCount].mem = data;
	boxes[totalBoxCount].msg = 100; // set default text for added box
	boxes[totalBoxCount].colour = 0;
	boxCount = ++totalBoxCount;

	tBox* boxText_new = new tBox[actualBoxCount + 1];
	memcpy(boxText_new, boxText, sizeof(tBox) * actualBoxCount);
	delete[] boxText;
	boxText = boxText_new;

	memset((void*)&boxText[actualBoxCount], 0, sizeof(tBox));
	actualBoxCount++;

	SetEngine(totalBoxCount);
	return MaxBox(); // current number of added box
}

bool BarBoxes::GetBox(int i) {
	if (i < 5 || i > BarBoxes::MaxBox()) return false;
	return boxText[i - 5].isActive;
}

void BarBoxes::AddBox(int i) {
	if (i < 5 || i > BarBoxes::MaxBox()) return;
	boxText[i - 5].isActive = true;
	__asm call fo::funcoffs::refresh_box_bar_win_;
}

void BarBoxes::RemoveBox(int i) {
	if (i < 5 || i > BarBoxes::MaxBox()) return;
	boxText[i - 5].isActive = false;
	__asm call fo::funcoffs::refresh_box_bar_win_;
}

void BarBoxes::exit() {
	delete[] boxes;
	delete[] boxText;
}

}
