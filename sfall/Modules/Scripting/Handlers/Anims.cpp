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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\Explosions.h"
#include "..\..\ScriptExtender.h"

#include "Anims.h"

namespace sfall
{
namespace script
{

static char regAnimCombatCheck = 1;

void RegAnimCombatCheck(DWORD newValue) {
	char oldValue = regAnimCombatCheck;
	regAnimCombatCheck = (newValue > 0);
	if (oldValue != regAnimCombatCheck) {
		SafeWriteBatch<BYTE>(regAnimCombatCheck, {
			0x459C97, // reg_anim_func
			0x459D4B, // reg_anim_animate
			0x459E3B, // reg_anim_animate_reverse
			0x459EEB, // reg_anim_obj_move_to_obj
			0x459F9F, // reg_anim_obj_run_to_obj
			0x45A053, // reg_anim_obj_move_to_tile
			0x45A10B, // reg_anim_obj_run_to_tile
			0x45AE53  // reg_anim_animate_forever
		});
	}
}

// true if combat mode is active and combat check was not disabled
bool checkCombatMode() {
	return (regAnimCombatCheck & fo::var::combat_state) != 0;
}

void op_reg_anim_combat_check(OpcodeContext& ctx) {
	RegAnimCombatCheck(ctx.arg(0).rawValue());
}

void op_reg_anim_destroy(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		fo::func::register_object_must_erase(obj);
	}
}

void op_reg_anim_animate_and_hide(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		int animId = ctx.arg(1).rawValue(),
		    delay = ctx.arg(2).rawValue();

		fo::func::register_object_animate_and_hide(obj, animId, delay);
	}
}

void op_reg_anim_light(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		int radius = ctx.arg(1).rawValue(),
		    delay = ctx.arg(2).rawValue();

		if (radius < 0) {
			radius = 0;
		} else if (radius > 8) {
			radius = 8;
		}
		fo::func::register_object_light(obj, radius, delay);
	}
}

void op_reg_anim_change_fid(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		int fid = ctx.arg(1).rawValue(),
		    delay = ctx.arg(2).rawValue();

		fo::func::register_object_change_fid(obj, fid, delay);
	}
}

void op_reg_anim_take_out(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		int holdFrame = ctx.arg(1).rawValue(),
		    nothing = ctx.arg(2).rawValue(); // not used by engine

		fo::func::register_object_take_out(obj, holdFrame, nothing);
	}
}

void op_reg_anim_turn_towards(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		int tile = ctx.arg(1).rawValue(),
		    nothing = ctx.arg(2).rawValue();

		fo::func::register_object_turn_towards(obj, tile, nothing);
	}
}

static __declspec(naked) void ExecuteCallback() {
	__asm {
		call fo::funcoffs::executeProcedure_;
		jmp  ScriptExtender::GetResetScriptReturnValue; // return callback result from scr_return script function: -1 - break registered sequence
	}
}

void op_reg_anim_callback(OpcodeContext& ctx) {
	fo::func::register_object_call(
		reinterpret_cast<long*>(ctx.program()),
		reinterpret_cast<long*>(ctx.arg(0).rawValue()), // callback procedure
		reinterpret_cast<void*>(ExecuteCallback),
		-1
	);
}

void mf_reg_anim_animate_and_move(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).object();
		int tile = ctx.arg(1).rawValue(),
		    animId = ctx.arg(2).rawValue(),
		    delay = ctx.arg(3).rawValue();

		fo::func::register_object_animate_and_move_straight(obj, tile, obj->elevation, animId, delay);
	}
}

void op_explosions_metarule(OpcodeContext& ctx) {
	int mode = ctx.arg(0).rawValue(),
		result = ExplosionsMetaruleFunc(mode, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());

	if (result == -1) {
		ctx.printOpcodeError("%s() - mode (%d) is not supported for the function.", ctx.getOpcodeName(), mode);
	}
	ctx.setReturn(result);
}

void op_art_exists(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::art_exists(ctx.arg(0).rawValue()));
}

void mf_art_cache_flush(OpcodeContext& ctx) {
	__asm call fo::funcoffs::art_flush_;
}

}
}
