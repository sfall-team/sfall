/*
 *    sfall
 *    Copyright (C) 2009, 2010  The sfall team
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

#include "QuestList.h"

namespace sfall
{

static DWORD calledflag = 0x0;
static DWORD called_quest_number = 0x0;
static DWORD total_quests = 0x0;
static DWORD curent_quest_page = 0x0;
static DWORD wait_flag = 0x0;

static void __declspec(naked) newhookpress() {
	__asm {
		push eax;
		mov calledflag, 1;
		mov called_quest_number, ebx;
		mov wait_flag, 0;
		mov total_quests, 0;
		pop eax;
		mov edx,ds:[FO_VAR_crnt_func];
		retn;
		}
}

static const DWORD back_function = 0x497828;

static void __declspec(naked) backhookfunct() {
	__asm {
		push eax;
		mov calledflag, 0;
		mov curent_quest_page, 0;
		pop eax;
		call back_function;
		retn;
		}
}

static void __declspec(naked) newhookpress1() {
	__asm {
		push eax;
		mov calledflag, 0;
		mov curent_quest_page, 0;
		pop eax;
		sub ebx, 0x1F4;
		retn;
		}
}
static void __declspec(naked) nexthookfunct() {
	__asm {
		push eax;
		mov calledflag, 0;
		mov curent_quest_page, 0;
		pop eax;
		mov eax, 0x401;
		retn;
		}
}

static const DWORD show_text_jump_addr = 0x498193;

static void __declspec(naked) newhooklooktext() {
	__asm {
		push eax;
		mov eax, curent_quest_page;
		imul eax, eax, 0x9;
		cmp wait_flag, eax;
jl smpfhj;
		add eax, 0x9;
		cmp wait_flag, eax;
		jg smpfhj;
		pop eax;
		call fo::funcoffs::_word_wrap_;
		inc wait_flag;
		add total_quests, 1;
		retn;
smpfhj:
		pop eax;
		add total_quests, 1;
		inc wait_flag;
		mov eax, 1;
		retn;
	}
}

static void __declspec(naked) newhookresetvalue() {
	__asm {
		push eax;
		mov wait_flag, 0;
		pop eax;
		call fo::funcoffs::pip_back_;
		retn;
		}
}

//static const DWORD call_newbutt1 = 0x49720C; // Unused
//static const DWORD call_newbutt2 = 0x497BD8; // Unused - PipStatus_
//static const DWORD call_newbutt3 = 0x4999C0; // Unused - NixHotLines_
//static const DWORD call_newbutt4 = 0x4D36D4; // Unused - buf_to_buf_

static void __declspec(naked) newactbpip() {
	__asm {
		cmp ebx, 0x300;
		je jmphr4;
		jmp jmphr3;
jmphr4:
		cmp calledflag, 0x1;
		je jmphr5;
		jmp jmphr3;
jmphr5:
		cmp curent_quest_page, 0x0;
		je jmphr1;
		push eax;
		sub curent_quest_page, 1;
		mov wait_flag, 0;
		pop eax;
		mov ebx, called_quest_number;
jmphr3:
		cmp ebx, 0x301;
		je jmphr;
		jmp jmphr1;
jmphr:
		cmp calledflag, 0x1;
		je jmphr2;
		jmp jmphr1;
jmphr2:
		push eax;
		mov eax, curent_quest_page;
		add eax, 1;
		imul eax, eax, 0x9;
		cmp eax, total_quests;
		jl rtjl;
		pop eax;
		jmp jmphr1;
rtjl:
		add curent_quest_page, 1;
		mov wait_flag, 0;
		pop eax;
		mov ebx, called_quest_number;
jmphr1:
		cmp ebx, 0x1F4;
		retn;
		}
}
static const DWORD n_newbutt2 = 0x4D87F8;

static const DWORD newbutt1 = 0x4D87F8;
//static const DWORD newbutt4 = 0x45E440; // Unused
//static const DWORD newbutt7 = 0x4D8260; // Unused - win_register_button_
//static const DWORD newbutt8 = 0x45E440; // Unused
//static const DWORD newbutt9 = 0x45E440; // Unused

static void __declspec(naked) newbuttonfct() {
	__asm {

		// recreate original 4 buttons
		mov edi, 0x155;
		mov ebp, 0x1F4;
		xor esi, esi;
smpj1:
		cmp esi, +0x1;
		jz smpj4;
		push +0x20;
		push +0x0;
		mov eax, ds:[0x664424];
		push eax;
		mov edx, ds:[0x664420];
		push edx;
		push ebp;
		push -0x1;
		push -0x1;
		mov ebx, ds:[0x6642E4];
		push -0x1;
		mov ecx, ds:[0x6642E0];
		mov eax, ds:[FO_VAR_pip_win];
		push ebx;
		mov edx, 0x35;
		mov ebx, edi;
		call fo::funcoffs::win_register_button_;
		cmp eax, -0x1;
		jz smpj3;
		mov ebx, 0x451978;
		mov edx, 0x451970;
		call n_newbutt2;
smpj3:
		inc ebp;
smpj4:
		inc esi;
		add edi, +0x1b;
		cmp esi, +0x5;
		jl smpj1;


		// load new texture for first (up) button. I used memory address for texture from buttons
		// at chracter screen. Everything fine, because this buttons can't use in one time, and
		// they everytime recreating
		mov ebx, 0x451990;
		mov edx, 0x451988;
		call newbutt1;
		xor ecx, ecx;
		push +0x0;
		mov edx, 0xB5; // number from intrface.lst for button-up
		mov eax, 0x6;
		xor ebx, ebx;
		call fo::funcoffs::art_id_;
		mov ecx, 0x518F28;
		xor ebx, ebx;
		xor edx, edx;
		call fo::funcoffs::art_ptr_lock_data_;
		mov ds:[0x59D3FC], eax; // first texture memory address
		push +0x0;
		mov edx, 0xB6; // number from intrface.lst for button-down
		mov eax, 0x6;
		xor ecx, ecx;
		xor ebx, ebx;
		call fo::funcoffs::art_id_;
		mov ecx, 0x518F2C;
		xor ebx, ebx;
		xor edx, edx;
		call fo::funcoffs::art_ptr_lock_data_;
		mov ds:[0x59D400], eax; // second texture memory address


		// load new texture for second (down) button
		mov ebx, 0x451990;
		mov edx, 0x451988;
		call newbutt1;
		xor ecx, ecx;
		push +0x0;
		mov edx, 0xC7; // number from intrface.lst for button-up
		mov eax, 0x6;
		xor ebx, ebx;
		call fo::funcoffs::art_id_;
		mov ecx, 0x518F28;
		xor ebx, ebx;
		xor edx, edx;
		call fo::funcoffs::art_ptr_lock_data_;
		mov ds:[0x570514], eax; // first texture memory address
		push +0x0;
		mov edx, 0xC8; // number from intrface.lst for button-down
		mov eax, 0x6;
		xor ecx, ecx;
		xor ebx, ebx;
		call fo::funcoffs::art_id_;
		mov ecx, 0x518F2C;
		xor ebx, ebx;
		xor edx, edx;
		call fo::funcoffs::art_ptr_lock_data_;
		mov ds:[0x570518], eax; // second texture memory address


		// creating new 2 buttons
		mov edi, 0x149; // y position
		mov ebp, 0x300; // this number will return when button pressed
		xor esi, esi;
		push +0x20;
		push +0x0;
		mov eax, ds:[0x570518];
		push eax;
		mov edx, ds:[0x570514];
		push edx;
		push ebp;
		push -0x1;
		push -0x1;
		mov ebx, 0xE;
		push -0x1;
		mov ecx, 0xB;
		mov eax, ds:[FO_VAR_pip_win];
		push ebx;
		mov edx, 0x8B;
		mov ebx, edi;
		call fo::funcoffs::win_register_button_;
		cmp eax, -0x1;
		jz smpj16;
		mov ebx, 0x451978;
		mov edx, 0x451970;
		call n_newbutt2;
smpj16:
		// second button
		inc ebp;
		inc esi;
		add edi, +0x9;
		push +0x20;
		push +0x0;
		mov eax, ds:[0x59D400];
		push eax;
		mov edx, ds:[0x59D3FC];
		push edx;
		push ebp;
		push -0x1;
		push -0x1;
		mov ebx, 0xE;
		push -0x1;
		mov ecx, 0xB;
		mov eax, ds:[FO_VAR_pip_win];
		push ebx;
		mov edx, 0x8B;
		mov ebx, edi;
		call fo::funcoffs::win_register_button_;
		cmp eax, -0x1;
		jz smpj6;
		mov ebx, 0x451978;
		mov edx, 0x451970;
		call n_newbutt2;
smpj6:
		inc ebp;
		inc esi;
		add edi, +0x9;


		retn;
	}
}

void QuestListPatch() {
//<comments removed because they couldn't display correctly in this encoding>
	SafeWrite8(0x004974DF, 0xE8);
	SafeWrite32(0x004974E0, ((DWORD)&newbuttonfct) - 0x004974E4);
	SafeWrite8(0x004974E4, 0xE9);
	SafeWrite32(0x004974e5, 0x0000005B);
//
	SafeWrite8(0x00497173, 0xE8);
	SafeWrite32(0x00497174, ((DWORD)&newactbpip) - 0x00497178);
	SafeWrite8(0x00497178, 0x90);
//
	SafeWrite8(0x004971B2, 0xE8);
	SafeWrite32(0x004971B3, ((DWORD)&newhookpress) - 0x004971B7);
	SafeWrite8(0x004971B7, 0x90);
//
	SafeWrite8(0x00497183, 0xE8);
	SafeWrite32(0x00497184, ((DWORD)&newhookpress1) - 0x00497188);
	SafeWrite8(0x00497188, 0x90);
//
	SafeWrite8(0x00498186, 0xE8);
	SafeWrite32(0x00498187, ((DWORD)&newhooklooktext) - 0x0049818B);
//
	SafeWrite8(0x004982B0, 0xE8);
	SafeWrite32(0x004982B1, ((DWORD)&newhookresetvalue) - 0x004982B5);
//
	SafeWrite8(0x004971D9, 0xE8);
	SafeWrite32(0x004971DA, ((DWORD)&nexthookfunct) - 0x004971DE);
//
	SafeWrite8(0x00497219, 0xE8);
	SafeWrite32(0x0049721A, ((DWORD)&backhookfunct) - 0x0049721E);
}

void QuestList::init() {
	if(GetConfigInt("Misc", "UseScrollingQuestsList", 0)) {
		dlog("Applying quests list patch ", DL_INIT);
		QuestListPatch();
		dlogr(" Done", DL_INIT);
	}
}

}
