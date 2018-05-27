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

#include "BarBoxes.h"
#include "FalloutEngine.h"

static const DWORD DisplayBoxesRet1 = 0x4615A8;
static const DWORD DisplayBoxesRet2 = 0x4615BE;
struct sBox {
	DWORD msg;
	DWORD colour;
	void* mem;
};
static sBox boxes[10];
static DWORD boxesEnabled[5];

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
		call add_bar_box_;
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

void BarBoxesInit() {
	for (int i = 0; i < sizeof(bboxMemAddr) / 4; i++) {
		SafeWrite32(bboxMemAddr[i], (DWORD)boxes + 8); //.mem
	}
	SafeWrite32(0x4612FE, (DWORD)boxes + 4); //.colour
	SafeWrite32(0x46133C, (DWORD)boxes + 0); //.msg

	memset(boxes, 0, 12 * 10);
	memset(boxesEnabled, 0, 5 * 4);
	memcpy(boxes, (void*)0x518FE8, 12 * 5);

	for (int i = 5; i < 10; i++) {
		boxes[i].msg = 0x69 + i - 5;
	}

	SafeWrite8(0x46127C, 10);
	SafeWrite8(0x46140B, 10);
	SafeWrite8(0x461495, 0x78);

	MakeJump(0x4615A3, DisplayBoxesHook);
	char buf[6];
	GetPrivateProfileString("Misc", "BoxBarColours", "", buf, 6, ini);
	if (strlen(buf) == 5) {
		for (int i = 0; i < 5; i++) {
			if (buf[i] == '1') {
				boxes[i + 5].colour = 1;
			}
		}
	}
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
