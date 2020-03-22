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

static char regAnimCombatCheck = 1;

void __stdcall RegAnimCombatCheck(DWORD newValue) {
	char oldValue = regAnimCombatCheck;
	regAnimCombatCheck = (newValue > 0);
	if (oldValue != regAnimCombatCheck) {
		SafeWrite8(0x459C97, regAnimCombatCheck); // reg_anim_func
		SafeWrite8(0x459D4B, regAnimCombatCheck); // reg_anim_animate
		SafeWrite8(0x459E3B, regAnimCombatCheck); // reg_anim_animate_reverse
		SafeWrite8(0x459EEB, regAnimCombatCheck); // reg_anim_obj_move_to_obj
		SafeWrite8(0x459F9F, regAnimCombatCheck); // reg_anim_obj_run_to_obj
		SafeWrite8(0x45A053, regAnimCombatCheck); // reg_anim_obj_move_to_tile
		SafeWrite8(0x45A10B, regAnimCombatCheck); // reg_anim_obj_run_to_tile
		SafeWrite8(0x45AE53, regAnimCombatCheck); // reg_anim_animate_forever
	}
}

static void __declspec(naked) op_reg_anim_combat_check() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ecx, edx) // new value
	_CHECK_ARG_INT(cx, end1)
	__asm {
		push edx
		call RegAnimCombatCheck
	}
end1:
	_OP_END
}

// new reg_anim functions (all using existing engine code)

// checks if combat mode is enabled (using R8 8-bit register) and jumps to GOTOFAIL if it is (does nothing if regAnimCombatCheck is 0)
#define _CHECK_COMBAT_MODE(R8, GOTOFAIL) __asm { \
	__asm mov R8, regAnimCombatCheck             \
	__asm test byte ptr ds:_combat_state, R8     \
	__asm jnz GOTOFAIL }

static void __declspec(naked) op_reg_anim_destroy() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, edx, esi) // object
	_CHECK_ARG_INT(dx, end1)
	_CHECK_COMBAT_MODE(al, end1)
	__asm {
		mov eax, esi // object
		call register_object_must_erase_;
	}
end1:
	_OP_END
}

static void __declspec(naked) op_reg_anim_animate_and_hide() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, esi) // delay
	_GET_ARG_R32(ebp, ecx, edi) // animID
	_GET_ARG_R32(ebp, edx, eax) // object
	// eax should not change until function call
	_CHECK_ARG_INT(bx, end)
	_CHECK_ARG_INT(cx, end)
	_CHECK_ARG_INT(dx, end)
	__asm {
		_CHECK_COMBAT_MODE(bl, end)
		mov ebx, esi // delay
		mov edx, edi // animID
		call register_object_animate_and_hide_;
	}
end:
	_OP_END
}

static void __declspec(naked) op_reg_anim_light() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, esi) // delay
	_GET_ARG_R32(ebp, ecx, edi) // light radius
	_GET_ARG_R32(ebp, edx, eax) // object
	// eax should not change until function call
	_CHECK_ARG_INT(bx, end)
	_CHECK_ARG_INT(cx, end)
	_CHECK_ARG_INT(dx, end)
	__asm {
		// check for valid radius
		cmp di, 0
		jge dontfixMin
		mov di, 0
		jmp dontfixMax
dontfixMin:
		cmp di, 8
		jle dontfixMax
		mov di, 8
dontfixMax:
		_CHECK_COMBAT_MODE(bl, end)
		mov ebx, esi // delay
		mov edx, edi // light radius
		call register_object_light_;
	}
end:
	_OP_END
}

static void __declspec(naked) op_reg_anim_change_fid() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, esi) // delay
	_GET_ARG_R32(ebp, ecx, edi) // FID
	_GET_ARG_R32(ebp, edx, eax) // object
	// eax should not change until function call
	_CHECK_ARG_INT(bx, end)
	_CHECK_ARG_INT(cx, end)
	_CHECK_ARG_INT(dx, end)
	__asm {
		_CHECK_COMBAT_MODE(bl, end)
		mov ebx, esi // delay
		mov edx, edi // FID
		call register_object_change_fid_
	}
end:
	_OP_END
}

static void __declspec(naked) op_reg_anim_take_out() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, esi) // delay - not used
	_GET_ARG_R32(ebp, ecx, edi) // weapon hold frame ID
	_GET_ARG_R32(ebp, edx, eax) // object
	// eax should not change until function call
	_CHECK_ARG_INT(bx, end)
	_CHECK_ARG_INT(cx, end)
	_CHECK_ARG_INT(dx, end)
	__asm {
		_CHECK_COMBAT_MODE(bl, end)
		//mov ebx, esi // delay - not used
		mov edx, edi // holdFrame
		call register_object_take_out_;
	}
end:
	_OP_END
}

static void __declspec(naked) op_reg_anim_turn_towards() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, esi) // delay - not used
	_GET_ARG_R32(ebp, ecx, edi) // tile
	_GET_ARG_R32(ebp, edx, eax) // object
	// eax should not change until function call
	_CHECK_ARG_INT(bx, end)
	_CHECK_ARG_INT(cx, end)
	_CHECK_ARG_INT(dx, end)
	__asm {
		_CHECK_COMBAT_MODE(bl, end)
		// mov ebx, esi // delay - not used
		mov edx, edi // tile
		call register_object_turn_towards_;
	}
end:
	_OP_END
}

static void __declspec(naked) ExecuteCallback() {
	__asm {
		call executeProcedure_;
		jmp  GetResetScriptReturnValue; // return callback result from scr_return script function: -1 - break registered sequence
	}
}

static void _stdcall op_reg_anim_callback2() {
	const ScriptValue &procArg = opHandler.arg(0);

	if (procArg.isInt()) {
		RegisterObjectCall(
			reinterpret_cast<long*>(opHandler.program()),
			reinterpret_cast<long*>(procArg.rawValue()), // callback procedure
			reinterpret_cast<void*>(ExecuteCallback),
			-1
		);
	} else {
		OpcodeInvalidArgs("reg_anim_callback");
	}
}

static void __declspec(naked) op_reg_anim_callback() {
	_WRAP_OPCODE(op_reg_anim_callback2, 1, 0)
}

static void _stdcall op_explosions_metarule2() {
	const ScriptValue &modeArg = opHandler.arg(0),
					  &arg1Arg = opHandler.arg(1),
					  &arg2Arg = opHandler.arg(2);

	if (modeArg.isInt() && arg1Arg.isInt() && arg2Arg.isInt()) {
		int mode = modeArg.rawValue(),
			result = ExplosionsMetaruleFunc(mode, arg1Arg.rawValue(), arg2Arg.rawValue());

		if (result == -1) {
			opHandler.printOpcodeError("metarule2_explosions() - mode (%d) is not supported for the function.", mode);
		}
		opHandler.setReturn(result);
	} else {
		OpcodeInvalidArgs("metarule2_explosions");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_explosions_metarule() {
	_WRAP_OPCODE(op_explosions_metarule2, 3, 1)
}

static void sf_art_cache_flush() {
	__asm call art_flush_;
}
