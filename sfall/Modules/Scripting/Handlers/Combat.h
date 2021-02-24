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

#include "..\OpcodeContext.h"

namespace sfall
{
namespace script
{

// Kill counters
void SetExtraKillCounter(bool value);

void __declspec() op_get_kill_counter();

void __declspec() op_mod_kill_counter();

void op_set_object_knockback(OpcodeContext&);

void op_remove_object_knockback(OpcodeContext&);

void __declspec() op_get_bodypart_hit_modifier();

void __declspec() op_set_bodypart_hit_modifier();

void op_get_attack_type(OpcodeContext&);

void __declspec() op_force_aimed_shots();

void __declspec() op_disable_aimed_shots();

void __declspec() op_get_last_attacker();

void __declspec() op_get_last_target();

void __declspec() op_block_combat();

void mf_attack_is_aimed(OpcodeContext&);

void mf_combat_data(OpcodeContext&);

}
}
