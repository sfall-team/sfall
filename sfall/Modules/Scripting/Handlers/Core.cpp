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
#include "..\..\..\SafeWrite.h"
#include "..\..\..\Version.h"
#include "..\..\KillCounter.h"
#include "..\..\HookScripts.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "Core.h"

void __declspec(naked) op_set_global_script_repeat() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		push ecx;
		call SetGlobalScriptRepeat;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_global_script_type() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		push ecx;
		call SetGlobalScriptType;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_available_global_script_types() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov edx, availableGlobalScriptTypes;
		mov ecx, eax;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_sfall_global() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
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
		jz next;
		cmp dx, 0xc001;
		jnz end;
		push esi;
		push eax;
		call SetGlobalVarInt;
		jmp end;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push esi;
		push eax;
		call SetGlobalVar;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_sfall_global_int() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		xor edx, edx;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp si, 0x9001;
		jz next;
		cmp si, 0x9801;
		jz next;
		cmp si, 0xc001;
		jnz end;
		push eax;
		call GetGlobalVarInt;
		mov edx, eax;
		jmp end;
next:
		mov edx, esi;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		call GetGlobalVar;
		mov edx, eax;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_sfall_global_float() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		xor edx, edx;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp si, 0x9001;
		jz next;
		cmp si, 0x9801;
		jz next;
		cmp si, 0xc001;
		jnz end;
		push eax;
		call GetGlobalVarInt;
		mov edx, eax;
		jmp end;
next:
		mov edx, esi;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		call GetGlobalVar;
		mov edx, eax;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xa001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_sfall_arg() {
	__asm {
		pushad;
		push eax;
		call GetHSArg;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static DWORD _stdcall GetSfallArgs() {
	DWORD argCount = GetHSArgCount();
	DWORD id = TempArray(argCount, 4);
	DWORD* args = GetHSArgs();
	for (DWORD i = 0; i < argCount; i++) {
		arrays[id].val[i].set(*(long*)&args[i]);
	}
	return id;
}

void __declspec(naked) op_get_sfall_args() {
	__asm {
		pushad;
		push eax;
		call GetSfallArgs;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_sfall_arg() {
	__asm {
		pushad;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push edx;
		push eax;
		call SetHSArg;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_set_sfall_return() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz end;
		push eax;
		call SetHSReturn;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_init_hook() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, InitingHookScripts;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_set_self() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		push eax;
		push ebp;
		call SetSelfObject;
end:
		popad;
		retn;
	}
}

// used for both register_hook and register_hook_proc
void sf_register_hook(OpcodeContext& ctx) {
	int id = ctx.arg(0).asInt();
	int proc = (ctx.numArgs() > 1)
		? ctx.arg(1).asInt()
		: -1;

	RegisterHook(ctx.program(), id, proc);
}

void sf_sfall_ver_major(OpcodeContext& ctx) {
	ctx.setReturn(VERSION_MAJOR);
}

void sf_sfall_ver_minor(OpcodeContext& ctx) {
	ctx.setReturn(VERSION_MINOR);
}

void sf_sfall_ver_build(OpcodeContext& ctx) {
	ctx.setReturn(VERSION_BUILD);
}
