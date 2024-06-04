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

#include "..\..\..\Version.h"
#include "..\..\HookScripts\Common.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"

#include "Core.h"

namespace sfall
{
namespace script
{

void op_typeof(OpcodeContext& ctx) {
	ctx.setReturn(static_cast<int>(ctx.arg(0).type()));
}

__declspec(naked) void op_set_global_script_repeat() {
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

__declspec(naked) void op_set_global_script_type() {
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

__declspec(naked) void op_available_global_script_types() {
	__asm {
		mov  edx, availableGlobalScriptTypes;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

void op_set_sfall_global(OpcodeContext& ctx) {
	if (ctx.arg(0).isString()) {
		if (SetGlobalVar(ctx.arg(0).strValue(), ctx.arg(1).rawValue())) {
			ctx.printOpcodeError("%s() - the name of the global variable must consist of 8 characters.", ctx.getOpcodeName());
		}
	} else {
		SetGlobalVarInt(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
	}
}

static long GetGlobalVarNameString(OpcodeContext& ctx) {
	const char* var = ctx.arg(0).strValue();
	if (strlen(var) != 8) {
		ctx.printOpcodeError("%s() - the name of the global variable must consist of 8 characters.", ctx.getOpcodeName());
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

void op_get_sfall_global_int(OpcodeContext& ctx) {
	GetGlobalVar(ctx, DataType::INT);
}

void op_get_sfall_global_float(OpcodeContext& ctx) {
	GetGlobalVar(ctx, DataType::FLOAT);
}

void op_get_sfall_arg(OpcodeContext& ctx) {
	ctx.setReturn(HookCommon::GetHSArg());
}

void mf_get_sfall_arg_at(OpcodeContext& ctx) {
	long id = ctx.arg(0).rawValue();
	if (id >= static_cast<long>(HookCommon::GetHSArgCount()) || id < 0) {
		ctx.printOpcodeError("%s() - invalid value for argument.", ctx.getMetaruleName());
		ctx.setReturn(0);
		return;
	}
	ctx.setReturn(HookCommon::GetHSArgAt(id));
}

void op_get_sfall_args(OpcodeContext& ctx) {
	DWORD argCount = HookCommon::GetHSArgCount();
	DWORD id = CreateTempArray(argCount, 0);
	for (DWORD i = 0; i < argCount; i++) {
		arrays[id].val[i].set(HookCommon::GetHSArgAt(i));
	}
	ctx.setReturn(id);
}

void op_set_sfall_arg(OpcodeContext& ctx) {
	HookCommon::SetHSArg(ctx.arg(0).rawValue(), ctx.arg(1));
}

void op_set_sfall_return(OpcodeContext& ctx) {
	HookCommon::SetHSReturn(ctx.arg(0));
}

__declspec(naked) void op_game_loaded() {
	__asm {
		mov  esi, ecx;
		push eax; // script
		call ScriptExtender::ScriptHasLoaded;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_init_hook() {
	__asm {
		mov  edx, HookScripts::initingHookScripts;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_set_self() {
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

// used for both register_hook and register_hook_proc
void op_register_hook(OpcodeContext& ctx) {
	bool specReg = false;
	int proc;
	switch (ctx.opcode()) {
	case 0x27d:
		specReg = true;
	case 0x262:
		proc = ctx.arg(1).rawValue();
		if (proc < 0 || (specReg && proc == 0)) return;
		break;
	default:
		proc = -1;
	}
	HookScripts::RegisterHook(ctx.program(), ctx.arg(0).rawValue(), proc, specReg);
}

void mf_add_g_timer_event(OpcodeContext& ctx) {
	ScriptExtender::AddTimerEventScripts(ctx.program(), ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void mf_remove_timer_event(OpcodeContext& ctx) {
	if (ctx.numArgs() > 0) {
		ScriptExtender::RemoveTimerEventScripts(ctx.program(), ctx.arg(0).rawValue());
	} else {
		ScriptExtender::RemoveTimerEventScripts(ctx.program()); // remove all
	}
}

__declspec(naked) void op_sfall_ver_major() {
	__asm {
		mov  edx, VERSION_MAJOR;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_sfall_ver_minor() {
	__asm {
		mov  edx, VERSION_MINOR;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_sfall_ver_build() {
	__asm {
		mov  edx, VERSION_BUILD;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

}
}
