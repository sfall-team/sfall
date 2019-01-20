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

namespace sfall
{
namespace script
{

void __declspec(naked) op_set_global_script_repeat() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
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
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
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
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void sf_set_sfall_global(OpcodeContext& ctx) {
	if (ctx.arg(0).isString()) {
		if (SetGlobalVar(ctx.arg(0).String(), ctx.arg(1).rawValue())) {
			ctx.printOpcodeError("set_sfall_global() - The name of the global variable must consist of 8 letters.");
		}
	} else {
		SetGlobalVarInt(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
	}
}

static long GetGlobalVarNameString(OpcodeContext& ctx) {
	const char* var = ctx.arg(0).String();
	if (strlen(var) != 8) {
		ctx.printOpcodeError("get_sfall_global() - The name of the global variable must consist of 8 letters.");
		return 0;
	}
	return GetGlobalVarInternal(*(__int64*)var);
}

static void GetGlobalVar(OpcodeContext& ctx, DataType type) {
	long result; 
	if (ctx.arg(0).isString()) {
		result = GetGlobalVarNameString(ctx);
	} else {
		result = GetGlobalVarInt(ctx.arg(0).rawValue());
	}
	ctx.setReturn(result, type);
}

void sf_get_sfall_global_int(OpcodeContext& ctx) {
	GetGlobalVar(ctx, DataType::INT);
}

void sf_get_sfall_global_float(OpcodeContext& ctx) {
	GetGlobalVar(ctx, DataType::FLOAT);
}

void __declspec(naked) op_get_sfall_arg() {
	__asm {
		pushad;
		push eax;
		call GetHSArg;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
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
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_sfall_arg() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
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
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
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
		mov edx, initingHookScripts;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_set_self() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
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

}
}
