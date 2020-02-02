/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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
//#include "ScriptExtender.h"

#define START_VALID_ADDR    0x410000
#define END_VALID_ADDR      0x6B403F

// memory_reading_funcs
static void __declspec(naked) ReadByte() {
	__asm {
		_GET_ARG_INT(error);
		test eax, eax;
		jz   error;
		movzx edx, byte ptr ds:[eax]; // read memory
result:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
error:
		xor  edx, edx;
		jmp result;
	}
}

static void __declspec(naked) ReadShort() {
	__asm {
		_GET_ARG_INT(error);
		test eax, eax;
		jz   error;
		movzx edx, word ptr ds:[eax]; // read memory
result:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
error:
		xor  edx, edx;
		jmp result;
	}
}

static void __declspec(naked) ReadInt() {
	__asm {
		_GET_ARG_INT(error);
		test eax, eax;
		jz   error;
		mov  edx, dword ptr ds:[eax]; // read memory
result:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
error:
		xor  edx, edx;
		jmp result;
	}
}

static void __declspec(naked) ReadString() {
	__asm {
		_GET_ARG_INT(error);
		test eax, eax;
		jz   error;
		mov  edx, eax;
result:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_STR);
//		retn;
error:
		xor  edx, edx;
		jmp  result;
	}
}

static void __declspec(naked) WriteByte() {
	__asm {
		push ecx;
		call interpretPopShort_;
		mov  ecx, eax; // type
		mov  eax, ebx;
		call interpretPopLong_;
		mov  esi, eax; // write value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_INT;
		jnz  end;
		// check valid addr
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
		and  esi, 0xFF;
		push esi;
		push eax;
		call SafeWrite8;
end:
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) WriteShort() {
	__asm {
		push ecx;
		call interpretPopShort_;
		mov  ecx, eax; // type
		mov  eax, ebx;
		call interpretPopLong_;
		mov  esi, eax; // write value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_INT;
		jnz  end;
		// check valid addr
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
		and  esi, 0xFFFF;
		push esi;
		push eax;
		call SafeWrite16;
end:
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) WriteInt() {
	__asm {
		push ecx;
		call interpretPopShort_;
		mov  ecx, eax; // type
		mov  eax, ebx;
		call interpretPopLong_;
		mov  esi, eax; // write value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_INT;
		jnz  end;
		// check valid addr
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
		push esi;
		push eax;
		call SafeWrite32;
end:
		pop  ecx;
		retn;
	}
}

static void __fastcall WriteStringInternal(char* addr, long type, long strID, TProgram* script) {
	const char* str = InterpretGetString(script, type, strID);
	while (*str) {
		if (!addr[0] && addr[1]) break; // addr[1] as *(addr + 1)
		*addr++ = *str++;
	}
	*addr = 0;
}

static void __declspec(naked) WriteString() {
	__asm {
		push ecx;
		call interpretPopShort_;
		mov  ecx, eax; // type
		mov  eax, ebx;
		call interpretPopLong_;
		mov  esi, eax; // str value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_STR2;
		je   next;
		cmp  cx, VAR_TYPE_STR;
		jnz  end;
next:
		// ecx - type, esi - value
		// edx - type, eax - addr
		// check valid address
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
		push ebx; // script
		push esi; // str value
		mov  edx, ecx; // type
		mov  ecx, eax; // addr
		call WriteStringInternal;
end:
		pop  ecx;
		retn;
	}
}

static void __fastcall CallOffsetInternal(TProgram* script, DWORD func) {
	func = (func >> 2) - 0x1d2;
	DWORD args[5];
	DWORD illegalArg = 0;
	int argCount = func % 5;

	for (int i = argCount; i >= 0; i--) {
		if ((short)InterpretPopShort(script) != (short)VAR_TYPE_INT) illegalArg++;
		args[i] = InterpretPopLong(script);
	}
	if (illegalArg || args[0] < 0x410010 || args[0] > 0x4FCE34) {
		args[0] = 0;
	} else {
		__asm {
			mov  eax, args[4];
			mov  edx, args[8];
			mov  ebx, args[12];
			mov  ecx, args[16];
			call args[0];
			mov  args[0], eax;
		}
	}
	if (func >= 5) { // has return
		__asm {
			mov eax, script;
			mov edx, args[0];
			mov ebx, eax;
			_RET_VAL_INT2;
		}
	}
}

static void __declspec(naked) CallOffset() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		call CallOffsetInternal; // edx - func
		mov  ecx, esi;
		retn;
	}
}
