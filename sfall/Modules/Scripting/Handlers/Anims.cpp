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
#include "..\..\Explosions.h"
#include "..\OpcodeContext.h"

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
bool checkCombatMode() {
	return (regAnimCombatCheck & fo::var::combat_state) != 0;
}

void sf_reg_anim_combat_check(OpcodeContext& ctx) {
	RegAnimCombatCheck(ctx.arg(0).asInt());
}

void sf_reg_anim_destroy(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).asObject();
		fo::func::register_object_must_erase(obj);
	}
}

void sf_reg_anim_animate_and_hide(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).asObject();
		int animId = ctx.arg(1).asInt(),
			delay = ctx.arg(2).asInt();

		fo::func::register_object_animate_and_hide(obj, animId, delay);
	}
}

void sf_reg_anim_light(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).asObject();
		int radius = ctx.arg(1).asInt(),
			delay = ctx.arg(2).asInt();

		if (radius < 0) {
			radius = 0;
		} else if (radius > 8) {
			radius = 8;
		}
		fo::func::register_object_light(obj, radius, delay);
	}
}

void sf_reg_anim_change_fid(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).asObject();
		int fid = ctx.arg(1).asInt(),
			delay = ctx.arg(2).asInt();

		fo::func::register_object_change_fid(obj, fid, delay);
	}
}

void sf_reg_anim_take_out(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).asObject();
		int holdFrame = ctx.arg(1).asInt(),
			nothing = ctx.arg(2).asInt(); // not used by engine

		fo::func::register_object_take_out(obj, holdFrame, nothing);
	}
}

void sf_reg_anim_turn_towards(OpcodeContext& ctx) {
	if (!checkCombatMode()) {
		auto obj = ctx.arg(0).asObject();
		int tile = ctx.arg(1).asInt(),
			nothing = ctx.arg(2).asInt();

		fo::func::register_object_turn_towards(obj, tile, nothing);
	}
}

void sf_explosions_metarule(OpcodeContext& ctx) {
	int mode = ctx.arg(0).asInt(),
		result = ExplosionsMetaruleFunc(mode, ctx.arg(1).asInt(), ctx.arg(2).asInt());

	if (result == -1) {
		ctx.printOpcodeError("%s() - mode (%d) is not supported for the function.", ctx.getOpcodeName(), mode);
	}
	ctx.setReturn(result);
}

void sf_art_cache_flush(OpcodeContext& ctx) {
	__asm call fo::funcoffs::art_flush_;
}

}
}