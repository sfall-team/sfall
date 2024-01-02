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

#pragma once

#include "..\OpcodeContext.h"

namespace sfall
{
namespace script
{

// new reg_anim functions (all using existing engine code)

void RegAnimCombatCheck(DWORD newValue);

class OpcodeContext;

void op_reg_anim_combat_check(OpcodeContext&);
void op_reg_anim_destroy(OpcodeContext&);
void op_reg_anim_animate_and_hide(OpcodeContext&);
void op_reg_anim_light(OpcodeContext&);
void op_reg_anim_change_fid(OpcodeContext&);
void op_reg_anim_take_out(OpcodeContext&);
void op_reg_anim_turn_towards(OpcodeContext&);
void op_reg_anim_callback(OpcodeContext&);

void mf_reg_anim_animate_and_move(OpcodeContext&);

void op_explosions_metarule(OpcodeContext&);

void op_art_exists(OpcodeContext&);

void mf_art_cache_flush(OpcodeContext&);

}
}
