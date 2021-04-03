/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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

#include "version.h"

static void __stdcall op_typeof2() {
	opHandler.setReturn(static_cast<int>(opHandler.arg(0).type()));
}

static void __declspec(naked) op_typeof() {
	_WRAP_OPCODE(op_typeof2, 1, 1)
}

static void __declspec(naked) op_set_global_script_repeat() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		_GET_ARG_INT(end);
		mov  edx, eax;              // frames
		call SetGlobalScriptRepeat; // ecx - script
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_global_script_type() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		_GET_ARG_INT(end);
		mov  edx, eax;            // type
		call SetGlobalScriptType; // ecx - script
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_available_global_script_types() {
	__asm {
		mov  edx, availableGlobalScriptTypes;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __stdcall op_set_sfall_global2() {
	const ScriptValue &varArg = opHandler.arg(0),
	                  &valArg = opHandler.arg(1);

	if (!varArg.isFloat() && !valArg.isString()) {
		if (varArg.isString()) {
			if (SetGlobalVar(varArg.strValue(), valArg.rawValue())) {
				opHandler.printOpcodeError("set_sfall_global() - the name of the global variable must consist of 8 characters.");
			}
		} else {
			SetGlobalVarInt(varArg.rawValue(), valArg.rawValue());
		}
	} else {
		OpcodeInvalidArgs("set_sfall_global");
	}
}

static void __declspec(naked) op_set_sfall_global() {
	_WRAP_OPCODE(op_set_sfall_global2, 2, 0)
}

static long __stdcall GetGlobalVarNameString(OpcodeHandler& opHandler, const char* opcodeName) {
	const char* var = opHandler.arg(0).strValue();
	if (strlen(var) != 8) {
		opHandler.printOpcodeError("%s() - the name of the global variable must consist of 8 characters.", opcodeName);
		return 0;
	}
	return GetGlobalVarInternal(*(__int64*)var);
}

static void __stdcall GetGlobalVarFunc(OpcodeHandler& opHandler, SfallDataType type, const char* opcodeName) {
	long result;
	if (opHandler.arg(0).isString()) {
		result = GetGlobalVarNameString(opHandler, opcodeName);
	} else {
		result = GetGlobalVarInt(opHandler.arg(0).rawValue());
	}
	opHandler.setReturn(result, type);
}

static void __stdcall op_get_sfall_global_int2() {
	const ScriptValue &varArg = opHandler.arg(0);

	if (!varArg.isFloat()) {
		GetGlobalVarFunc(opHandler, DATATYPE_INT, "get_sfall_global_int");
	} else {
		OpcodeInvalidArgs("get_sfall_global_int");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_sfall_global_int() {
	_WRAP_OPCODE(op_get_sfall_global_int2, 1, 1)
}

static void __stdcall op_get_sfall_global_float2() {
	const ScriptValue &varArg = opHandler.arg(0);

	if (!varArg.isFloat()) {
		GetGlobalVarFunc(opHandler, DATATYPE_FLOAT, "get_sfall_global_float");
	} else {
		OpcodeInvalidArgs("get_sfall_global_float");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_sfall_global_float() {
	_WRAP_OPCODE(op_get_sfall_global_float2, 1, 1)
}

static void __declspec(naked) op_get_sfall_arg() {
	__asm {
		mov  esi, ecx;
		call GetHSArg;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void mf_get_sfall_arg_at() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		long argVal = 0;
		long id = idArg.rawValue();
		if (id >= static_cast<long>(GetHSArgCount()) || id < 0) {
			opHandler.printOpcodeError("get_sfall_arg_at() - invalid value for argument.");
		} else {
			argVal = GetHSArgAt(id);
		}
		opHandler.setReturn(argVal);
	} else {
		OpcodeInvalidArgs("get_sfall_arg_at");
		opHandler.setReturn(0);
	}
}

static DWORD __stdcall GetSfallArgs() {
	DWORD argCount = GetHSArgCount();
	DWORD id = CreateTempArray(argCount, 0);
	DWORD* args = GetHSArgs();
	for (DWORD i = 0; i < argCount; i++) {
		arrays[id].val[i].set(*(long*)&args[i]);
	}
	return id;
}

static void __declspec(naked) op_get_sfall_args() {
	__asm {
		mov  esi, ecx;
		call GetSfallArgs;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __stdcall op_set_sfall_arg2() {
	const ScriptValue &argNumArg = opHandler.arg(0),
	                  &valArg = opHandler.arg(1);

	if (argNumArg.isInt() && valArg.isInt()) {
		SetHSArg(argNumArg.rawValue(), valArg.rawValue());
	} else {
		OpcodeInvalidArgs("set_sfall_arg");
	}
}

static void __declspec(naked) op_set_sfall_arg() {
	_WRAP_OPCODE(op_set_sfall_arg2, 2, 0)
}

static void __declspec(naked) op_set_sfall_return() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call SetHSReturn;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_game_loaded() {
	__asm {
		mov  esi, ecx;
		push eax; // script
		call ScriptHasLoaded;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_init_hook() {
	__asm {
		mov  edx, initingHookScripts;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_set_self() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		_GET_ARG_INT(end);
		mov  edx, eax;      // object
		call SetSelfObject; // ecx - script
end:
		mov  ecx, esi;
		retn;
	}
}

static void __stdcall op_register_hook2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		RegisterHook(opHandler.program(), idArg.rawValue(), -1, false);
	} else {
		OpcodeInvalidArgs("register_hook");
	}
}

static void __declspec(naked) op_register_hook() {
	_WRAP_OPCODE(op_register_hook2, 1, 0)
}

static void __stdcall op_register_hook_proc2() {
	const ScriptValue &idArg = opHandler.arg(0),
	                  &procArg = opHandler.arg(1);

	if (idArg.isInt() && procArg.isInt()) {
		RegisterHook(opHandler.program(), idArg.rawValue(), procArg.rawValue(), false);
	} else {
		OpcodeInvalidArgs("register_hook_proc");
	}
}

static void __declspec(naked) op_register_hook_proc() {
	_WRAP_OPCODE(op_register_hook_proc2, 2, 0)
}

static void __stdcall op_register_hook_proc_spec2() {
	const ScriptValue &idArg = opHandler.arg(0),
	                  &procArg = opHandler.arg(1);

	if (idArg.isInt() && procArg.isInt()) {
		RegisterHook(opHandler.program(), idArg.rawValue(), procArg.rawValue(), true);
	} else {
		OpcodeInvalidArgs("register_hook_proc_spec");
	}
}

static void __declspec(naked) op_register_hook_proc_spec() {
	_WRAP_OPCODE(op_register_hook_proc_spec2, 2, 0)
}

static void mf_add_g_timer_event() {
	AddTimerEventScripts(opHandler.program(), opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue());
}

static void mf_remove_timer_event() {
	if (opHandler.numArgs() > 0) {
		RemoveTimerEventScripts(opHandler.program(), opHandler.arg(0).rawValue());
	} else {
		RemoveTimerEventScripts(opHandler.program()); // remove all
	}
}

static void __declspec(naked) op_sfall_ver_major() {
	__asm {
		mov  edx, VERSION_MAJOR;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_sfall_ver_minor() {
	__asm {
		mov  edx, VERSION_MINOR;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_sfall_ver_build() {
	__asm {
		mov  edx, VERSION_BUILD;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}
