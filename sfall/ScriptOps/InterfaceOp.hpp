/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#pragma once

#include "main.h"
#include "input.h"
#include "ScriptExtender.h"


// input_functions
static void __declspec(naked) InputFuncsAvailable() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, 1; //They're always available from 2.9 on
		mov ebx, 0x4674DC;
		call ebx;
		mov edx, 0xc001;
		mov eax, ecx;
		mov ebx, 0x46748C;
		call ebx;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) KeyPressed() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov ebx, 0x4674F0;
		call ebx;
		mov edx, eax;
		mov eax, ecx;
		mov ebx, 0x467500;
		call ebx;
		cmp dx, 0xC001;
		jnz fail;
		push ecx;
		push eax;
		call KeyDown;
		mov edx, eax;
		pop ecx;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx;
		mov ebx, 0x4674DC;
		call ebx;
		mov edx, 0xc001;
		mov eax, ecx;
		mov ebx, 0x46748C;
		call ebx;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) funcTapKey() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov ebx, 0x4674F0;
		call ebx;
		mov edx, eax;
		mov eax, ecx;
		mov ebx, 0x467500;
		call ebx;
		cmp dx, 0xC001;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 255;
		jge end;
		push eax;
		call TapKey;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

//// *** From helios *** ////
static void __declspec(naked) get_mouse_x() {
   __asm {
	  push ecx;
	  push edx;
	  mov ecx, eax;
	  mov edx, ds:[0x6AC7A8];
	  add edx, ds:[0x6AC7D0];
	  call SetResult;
	  mov eax, ecx;
	  mov edx, 0xc001;
	  call SetResultType
	  pop edx;
	  pop ecx;
	  retn;
   }
}

//Return mouse y position
static void __declspec(naked) get_mouse_y() {
   __asm {
	  push ecx;
	  push edx;
	  mov ecx, eax;
	  mov edx, ds:[0x6AC7A4];
	  add edx, ds:[0x6AC7CC];
	  call SetResult;
	  mov eax, ecx;
	  mov edx, 0xc001;
	  call SetResultType
	  pop edx;
	  pop ecx;
	  retn;
   }
}

//Return pressed mouse button (1=left, 2=right, 3=left+right)
static void __declspec(naked) get_mouse_buttons() {
   __asm {
	  push ecx;
	  push edx;
	  mov ecx, eax;
	  mov edx, ds:[0x0051E2AC];
	  call SetResult;
	  mov eax, ecx;
	  mov edx, 0xc001;
	  call SetResultType
	  pop edx;
	  pop ecx;
	  retn;
   }
}

//Return the window number under the mous
static void __declspec(naked) get_window_under_mouse() {
   __asm {
	  push ecx;
	  push edx;
	  mov ecx, eax;
	  mov edx, ds:[0x0051E404];
	  call SetResult;
	  mov eax, ecx;
	  mov edx, 0xc001;
	  call SetResultType
	  pop edx;
	  pop ecx;
	  retn;
   }
}
//Return screen width
static void __declspec(naked) get_screen_width() {
   __asm {
	  push ecx;
	  push edx;
	  mov ecx, eax;
	  mov edx, ds:[0x4CAD6B];
	  call SetResult;
	  mov eax, ecx;
	  mov edx, 0xc001;
	  call SetResultType
	  pop edx;
	  pop ecx;
	  retn;
   }
}
//Return screen height
static void __declspec(naked) get_screen_height() {
   __asm {
	  push ecx;
	  push edx;
	  mov ecx, eax;
	  mov edx, ds:[0x4CAD66];
	  call SetResult;
	  mov eax, ecx;
	  mov edx, 0xc001;
	  call SetResultType
	  pop edx;
	  pop ecx;
	  retn;
   }
}
//Stop game, the same effect as open charsscreen or inventory
static void __declspec(naked) stop_game() {
   __asm {
	  push ebx;
	  mov ebx, 0x00482104;
	  call ebx;
	  pop ebx;
	  retn;
   }
}

//Resume the game when it is stopped
static void __declspec(naked) resume_game() {
   __asm {
	  push ebx;
	  mov ebx, 0x004820C0;
	  call ebx;
	  pop ebx;
	  retn;
   }
}

//Create a message window with given string
static const DWORD createWindow=0x41CF20;
static void __declspec(naked) create_message_window() {
   __asm {
	  pushad
	  mov ebx, dword ptr ds:[0x51E3B0];
	  cmp ebx, 0x65;
	  je end;

	  mov ecx, eax;
	  call GetArgType;
	  mov edx, eax;
	  mov eax, ecx;
	  call GetArg;
	  cmp dx, 0x9001;
	  jz next;
	  cmp dx, 0x9801;
	  jnz end;
next:
	  mov ebx, eax;
	  mov eax, ecx;
	  call GetStringVar;
	  mov esi, eax

	  mov ecx, eax;
	  mov eax, 3;
	  push 1;
	  mov al, ds:0x006AB718;
	  push eax;
	  push 0;
	  push eax;
	  push 0x74;
	  mov ecx, 0xC0;
	  mov eax, esi;
	  xor ebx, ebx;
	  xor edx, edx;
	  call createWindow;
	  //xor eax, eax;
end:
	  popad
	  ret;
   }
}

static void __declspec(naked) GetViewportX() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[0x51DE2C];
		mov ebx, 0x4674DC;
		call ebx;
		mov edx, 0xc001;
		mov eax, ecx;
		mov ebx, 0x46748C;
		call ebx;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetViewportY() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[0x51DE30];
		mov ebx, 0x4674DC;
		call ebx;
		mov edx, 0xc001;
		mov eax, ecx;
		mov ebx, 0x46748C;
		call ebx;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetViewportX() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov ebx, 0x4674F0;
		call ebx;
		mov edx, eax;
		mov eax, ecx;
		mov ebx, 0x467500;
		call ebx;
		cmp dx, 0xC001;
		jnz end;
		mov ds:[0x51DE2C], eax
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetViewportY() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov ebx, 0x4674F0;
		call ebx;
		mov edx, eax;
		mov eax, ecx;
		mov ebx, 0x467500;
		call ebx;
		cmp dx, 0xC001;
		jnz end;
		mov ds:[0x51DE30], eax
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static const DWORD _refresh_box_bar_win=0x4614CC;
static const DWORD _pc_flag_on=0x42E26C;
static const DWORD _pc_flag_off=0x42E220;
static void __declspec(naked) ShowIfaceTag() {
	__asm {
		pushad;
		mov ecx, eax;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
		cmp dx, 0xC001;
		jnz end;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call AddBox;
		call _refresh_box_bar_win;
		jmp end;
falloutfunc:
		call _pc_flag_on;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) HideIfaceTag() {
	__asm {
		pushad;
		mov ecx, eax;
		call GetArgType;
		mov edx, eax;
		mov eax, ecx;
		call GetArg;
		cmp dx, 0xC001;
		jnz end;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call RemoveBox;
		call _refresh_box_bar_win;
		jmp end;
falloutfunc:
		call _pc_flag_off;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) IsIfaceTagActive() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebx, eax;
		call GetArgType;
		mov edx, eax;
		mov eax, ebx;
		call GetArg;
		cmp dx, 0xC001;
		jnz fail;
		cmp eax, 3;
		je falloutfunc;
		cmp eax, 4;
		je falloutfunc;
		push eax;
		call GetBox;
		mov edx, eax;
		jmp end;
falloutfunc:
		mov ecx, eax;
		mov eax, dword ptr ds:[0x6610B8];
		mov edx, esp;
		mov eax, [eax+0x64];
		mov edi, 0x4A2108;
		call edi;
		mov edx, 1;
		shl edx, cl;
		mov ecx, [esp];
		mov eax, [ecx+0x20];
		and eax, edx;
		jz fail;
		xor edx, edx;
		inc edx;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebx;
		call SetResult;
		mov eax, ebx;
		mov edx, 0xc001;
		call SetResultType;
		add esp, 4;
		popad;
		retn;
	}
}