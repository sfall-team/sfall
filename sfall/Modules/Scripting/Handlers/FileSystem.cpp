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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\ScriptExtender.h"
#include "..\..\FileSystem.h"

#include "FileSystem.h"

namespace sfall
{
namespace script
{

void __declspec(naked) op_fs_create() {
	__asm {
		pushad;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz fail;
next:
		cmp bx, VAR_TYPE_INT;
		jnz fail;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push esi;
		push eax;
		call FScreate;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_copy() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_STR2;
		jz next;
		cmp di, VAR_TYPE_STR;
		jnz fail;
next:
		cmp si, VAR_TYPE_STR2;
		jz next2;
		cmp si, VAR_TYPE_STR;
		jnz fail;
next2:
		mov ebx, eax;
		mov edx, esi;
		mov eax, ebp;
		call fo::funcoffs::interpretGetString_;
		mov esi, eax;
		mov ebx, ecx;
		mov edx, edi;
		mov eax, ebp;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push esi;
		call FScopy;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_find() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_STR2;
		jz next;
		cmp di, VAR_TYPE_STR;
		jnz fail;
next:
		mov ebx, eax;
		mov edx, edi;
		mov eax, ebp;
		call fo::funcoffs::interpretGetString_;
		push eax;
		call FSfind;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_write_byte() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push ecx;
		push eax;
		call FSwrite_byte;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_write_short() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push ecx;
		push eax;
		call FSwrite_short;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_write_int() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jz next;
		cmp si, VAR_TYPE_FLOAT;
		jnz end;
next:
		push ecx;
		push eax;
		call FSwrite_int;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_write_string() {
	__asm {
		pushad;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ebp, eax;
		cmp bx, VAR_TYPE_STR2;
		jz next;
		cmp bx, VAR_TYPE_STR;
		jnz end;
next:
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov edx, ebx;
		mov ebx, esi;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push ebp;
		call FSwrite_string;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_write_bstring() {
	__asm {
		pushad;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ebp, eax;
		cmp bx, VAR_TYPE_STR2;
		jz next;
		cmp bx, VAR_TYPE_STR;
		jnz end;
next:
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov edx, ebx;
		mov ebx, esi;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push ebp;
		call FSwrite_bstring;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_read_byte() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call FSread_byte;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_read_short() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call FSread_short;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_read_int() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call FSread_int;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_read_float() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call FSread_int;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_FLOAT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_delete() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end
			push eax;
		call FSdelete;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_size() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call FSsize;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_pos() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call FSpos;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_seek() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push ecx;
		push eax;
		call FSseek;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_fs_resize() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push ecx;
		push eax;
		call FSresize;
end:
		popad;
		retn;
	}
}

}
}
