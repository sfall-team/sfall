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

#include "..\..\..\FalloutEngine\AsmMacros.h"
#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\..\SafeWrite.h"

#include "Memory.h"

namespace sfall
{
namespace script
{

#define START_VALID_ADDR    0x410000
#define END_VALID_ADDR      0x6B403F

bool unsafeEnabled = false;
bool checkValidMemAddr = true;

__declspec(naked) void op_read_byte() {
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
		jmp  result;
	}
}

__declspec(naked) void op_read_short() {
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
		jmp  result;
	}
}

__declspec(naked) void op_read_int() {
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
		jmp  result;
	}
}

__declspec(naked) void op_read_string() {
	static const char* emptyStr = "";
	__asm {
		_GET_ARG_INT(error);
		test eax, eax;
		jz   error;
		mov  edx, eax;
result:
		mov  eax, ebx;
		call fo::funcoffs::interpretAddString_;
		mov  edx, eax;
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_STR);
//		retn;
error:
		mov  edx, emptyStr;
		jmp  result;
	}
}

__declspec(naked) void op_write_byte() {
	__asm {
		push ecx;
		_GET_ARG(esi, ecx); // write value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_INT;
		jnz  end;
		cmp  unsafeEnabled, 0;
		jz   end;
		// check valid addr
		cmp  checkValidMemAddr, 0;
		jz   noCheck;
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
noCheck:
		and  esi, 0xFF;
		push esi;
		push eax;
		call SafeWrite8;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_write_short() {
	__asm {
		push ecx;
		_GET_ARG(esi, ecx); // write value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_INT;
		jnz  end;
		cmp  unsafeEnabled, 0;
		jz   end;
		// check valid addr
		cmp  checkValidMemAddr, 0;
		jz   noCheck;
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
noCheck:
		and  esi, 0xFFFF;
		push esi;
		push eax;
		call SafeWrite16;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_write_int() {
	__asm {
		push ecx;
		_GET_ARG(esi, ecx); // write value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_INT;
		jnz  end;
		cmp  unsafeEnabled, 0;
		jz   end;
		// check valid addr
		cmp  checkValidMemAddr, 0;
		jz   noCheck;
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
noCheck:
		push esi;
		push eax;
		call SafeWrite32;
end:
		pop  ecx;
		retn;
	}
}

static void __fastcall WriteStringInternal(char* addr, long type, long strID, fo::Program* script) {
	const char* str = fo::func::interpretGetString(script, type, strID);
	while (*str) {
		if (!addr[0] && addr[1]) break; // addr[1] as *(addr + 1)
		*addr++ = *str++;
	}
	*addr = 0;
}

__declspec(naked) void op_write_string() {
	__asm {
		push ecx;
		_GET_ARG(esi, ecx); // str value
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  cx, VAR_TYPE_STR2;
		je   next;
		cmp  cx, VAR_TYPE_STR;
		jnz  end;
next:
		cmp  unsafeEnabled, 0;
		jz   end;
		// ecx - type, esi - value
		// edx - type, eax - addr
		// check valid address
		cmp  checkValidMemAddr, 0;
		jz   noCheck;
		cmp  eax, START_VALID_ADDR;
		jb   end;
		cmp  eax, END_VALID_ADDR;
		ja   end;
noCheck:
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

static void __fastcall CallOffsetInternal(fo::Program* script, DWORD func) {
	func = (func >> 2) - 0x1d2;
	DWORD args[5];
	long illegalArg = 0;
	int argCount = func % 5;

	for (int i = argCount; i >= 0; i--) {
		if ((short)fo::func::interpretPopShort(script) != (short)VAR_TYPE_INT) illegalArg++;
		args[i] = fo::func::interpretPopLong(script);
	}
	if (illegalArg || !unsafeEnabled || (checkValidMemAddr && (args[0] < 0x410010 || args[0] > 0x4FCE34))) {
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
			_RET_VAL_INT;
		}
	}
}

__declspec(naked) void op_call_offset() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		call CallOffsetInternal; // edx - func
		mov  ecx, esi;
		retn;
	}
}

}
}