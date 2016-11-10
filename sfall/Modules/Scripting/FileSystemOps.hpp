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

#include "..\..\main.h"

#include "..\FileSystem.h"
#include "..\ScriptExtender.h"

//file system functions
static void __declspec(naked) fs_create() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz fail;
next:
		cmp bx, 0xc001;
		jnz fail;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
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
		call FuncOffs::interpretPushLong_;
		mov eax, edi;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_copy() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0x9001;
		jz next;
		cmp di, 0x9801;
		jnz fail;
next:
		cmp si, 0x9001;
		jz next2;
		cmp si, 0x9801;
		jnz fail;
next2:
		mov ebx, eax;
		mov edx, esi;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
		mov esi, eax;
		mov ebx, ecx;
		mov edx, edi;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
		push eax;
		push esi;
		call FScopy;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_find() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0x9001;
		jz next;
		cmp di, 0x9801;
		jnz fail;
next:
		mov ebx, eax;
		mov edx, edi;
		mov eax, ebp;
		call FuncOffs::interpretGetString_;
		push eax;
		call FSfind;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_write_byte() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push ecx;
		push eax;
		call FSwrite_byte;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) fs_write_short() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push ecx;
		push eax;
		call FSwrite_short;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) fs_write_int() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jz next;
		cmp si, 0xa001;
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

static void __declspec(naked) fs_write_string() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov ebp, eax;
		cmp bx, 0x9001;
		jz next;
		cmp bx, 0x9801;
		jnz end;
next:
		cmp dx, 0xc001;
		jnz end;
		mov edx, ebx;
		mov ebx, esi;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		push ebp;
		call FSwrite_string;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) fs_write_bstring() {
	__asm {
		pushad;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov ebp, eax;
		cmp bx, 0x9001;
		jz next;
		cmp bx, 0x9801;
		jnz end;
next:
		cmp dx, 0xc001;
		jnz end;
		mov edx, ebx;
		mov ebx, esi;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		push ebp;
		call FSwrite_bstring;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) fs_read_byte() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_read_short() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_read_int() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_read_float() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xa001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_delete() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end
		push eax;
		call FSdelete;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) fs_size() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_pos() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) fs_seek() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push ecx;
		push eax;
		call FSseek;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) fs_resize() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push ecx;
		push eax;
		call FSresize;
end:
		popad;
		retn;
	}
}
