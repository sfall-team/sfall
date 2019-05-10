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

#pragma once

namespace sfall
{
namespace script
{

// new reg_anim functions (all using existing engine code)

void RegAnimCombatCheck(DWORD newValue);

class OpcodeContext;

void sf_reg_anim_combat_check(OpcodeContext&);
void sf_reg_anim_destroy(OpcodeContext&);
void sf_reg_anim_animate_and_hide(OpcodeContext&);
void sf_reg_anim_light(OpcodeContext&);
void sf_reg_anim_change_fid(OpcodeContext&);
void sf_reg_anim_take_out(OpcodeContext&);
void sf_reg_anim_turn_towards(OpcodeContext&);

void sf_explosions_metarule(OpcodeContext&);

void sf_art_cache_flush(OpcodeContext&);

}
}
