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

// true if combat mode is active and combat check was not disabled
static bool checkCombatMode() {
	return (regAnimCombatCheck & *ptr_combat_state) != 0;
}

static void __stdcall op_reg_anim_combat_check2() {
	const ScriptValue &newValArg = opHandler.arg(0);

	if (newValArg.isInt()) {
		RegAnimCombatCheck(newValArg.rawValue());
	} else {
		OpcodeInvalidArgs("reg_anim_combat_check");
	}
}

static void __declspec(naked) op_reg_anim_combat_check() {
	_WRAP_OPCODE(op_reg_anim_combat_check2, 1, 0)
}

static void __stdcall op_reg_anim_destroy2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		if (!checkCombatMode()) {
			fo_register_object_must_erase(obj);
		}
	} else {
		OpcodeInvalidArgs("reg_anim_destroy");
	}
}

static void __declspec(naked) op_reg_anim_destroy() {
	_WRAP_OPCODE(op_reg_anim_destroy2, 1, 0)
}

static void __stdcall op_reg_anim_animate_and_hide2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &animIdArg = opHandler.arg(1),
	                  &delayArg = opHandler.arg(2);

	if (obj && animIdArg.isInt() && delayArg.isInt()) {
		if (!checkCombatMode()) {
			int animId = animIdArg.rawValue(),
				delay = delayArg.rawValue();

			fo_register_object_animate_and_hide(obj, animId, delay);
		}
	} else {
		OpcodeInvalidArgs("reg_anim_animate_and_hide");
	}
}

static void __declspec(naked) op_reg_anim_animate_and_hide() {
	_WRAP_OPCODE(op_reg_anim_animate_and_hide2, 3, 0)
}

static void __stdcall op_reg_anim_light2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &radiusArg = opHandler.arg(1),
	                  &delayArg = opHandler.arg(2);

	if (obj && radiusArg.isInt() && delayArg.isInt()) {
		if (!checkCombatMode()) {
			int radius = radiusArg.rawValue(),
				delay = delayArg.rawValue();

			if (radius < 0) {
				radius = 0;
			} else if (radius > 8) {
				radius = 8;
			}
			fo_register_object_light(obj, radius, delay);
		}
	} else {
		OpcodeInvalidArgs("reg_anim_light");
	}
}

static void __declspec(naked) op_reg_anim_light() {
	_WRAP_OPCODE(op_reg_anim_light2, 3, 0)
}

static void __stdcall op_reg_anim_change_fid2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &fidArg = opHandler.arg(1),
	                  &delayArg = opHandler.arg(2);

	if (obj && fidArg.isInt() && delayArg.isInt()) {
		if (!checkCombatMode()) {
			int fid = fidArg.rawValue(),
				delay = delayArg.rawValue();

			fo_register_object_change_fid(obj, fid, delay);
		}
	} else {
		OpcodeInvalidArgs("reg_anim_change_fid");
	}
}

static void __declspec(naked) op_reg_anim_change_fid() {
	_WRAP_OPCODE(op_reg_anim_change_fid2, 3, 0)
}

static void __stdcall op_reg_anim_take_out2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &holdFrameArg = opHandler.arg(1),
	                  &nothingArg = opHandler.arg(2);

	if (obj && holdFrameArg.isInt() && nothingArg.isInt()) {
		if (!checkCombatMode()) {
			int holdFrame = holdFrameArg.rawValue(),
				nothing = nothingArg.rawValue(); // not used by engine

			fo_register_object_take_out(obj, holdFrame, nothing);
		}
	} else {
		OpcodeInvalidArgs("reg_anim_take_out");
	}
}

static void __declspec(naked) op_reg_anim_take_out() {
	_WRAP_OPCODE(op_reg_anim_take_out2, 3, 0)
}

static void __stdcall op_reg_anim_turn_towards2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &tileArg = opHandler.arg(1),
	                  &nothingArg = opHandler.arg(2);

	if (obj && tileArg.isInt() && nothingArg.isInt()) {
		if (!checkCombatMode()) {
			int tile = tileArg.rawValue(),
				nothing = nothingArg.rawValue(); // not used by engine

			fo_register_object_turn_towards(obj, tile, nothing);
		}
	} else {
		OpcodeInvalidArgs("reg_anim_turn_towards");
	}
}

static void __declspec(naked) op_reg_anim_turn_towards() {
	_WRAP_OPCODE(op_reg_anim_turn_towards2, 3, 0)
}

static void __declspec(naked) ExecuteCallback() {
	__asm {
		call executeProcedure_;
		jmp  GetResetScriptReturnValue; // return callback result from scr_return script function: -1 - break registered sequence
	}
}

static void __stdcall op_reg_anim_callback2() {
	const ScriptValue &procArg = opHandler.arg(0);

	if (procArg.isInt()) {
		fo_register_object_call(
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

static void __stdcall op_explosions_metarule2() {
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

static void __declspec(naked) op_art_exists() {
	__asm {
		_GET_ARG_INT(fail);
		call art_exists_;
		mov  edx, eax;
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void mf_art_cache_flush() {
	__asm call art_flush_;
}
