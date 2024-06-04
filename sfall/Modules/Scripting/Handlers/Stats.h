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

namespace sfall
{
namespace script
{

void op_set_hp_per_level_mod();

// stat_funcs
void op_set_pc_base_stat(OpcodeContext&);

void op_set_pc_extra_stat(OpcodeContext&);

void op_get_pc_base_stat(OpcodeContext&);

void op_get_pc_extra_stat(OpcodeContext&);

void op_set_critter_base_stat(OpcodeContext&);

void op_set_critter_extra_stat(OpcodeContext&);

void op_get_critter_base_stat(OpcodeContext&);

void op_get_critter_extra_stat(OpcodeContext&);

void op_set_critter_skill_points(OpcodeContext&);

void op_get_critter_skill_points(OpcodeContext&);

void op_set_available_skill_points();

void op_get_available_skill_points();

void op_mod_skill_points_per_level();

void op_set_unspent_ap_bonus();

void op_get_unspent_ap_bonus();

void op_set_unspent_ap_perk_bonus();

void op_get_unspent_ap_perk_bonus();

void op_get_critter_current_ap();

void op_set_critter_current_ap(OpcodeContext&);

void op_set_pickpocket_max();

void op_set_hit_chance_max();

void op_set_critter_hit_chance_mod(OpcodeContext&);

void op_set_base_hit_chance_mod();

void op_set_critter_pickpocket_mod(OpcodeContext&);

void op_set_base_pickpocket_mod();

void op_set_critter_skill_mod(OpcodeContext&);

void op_set_base_skill_mod();

void op_set_skill_max();

void op_set_stat_max();

void op_set_stat_min();

void op_set_pc_stat_max();

void op_set_pc_stat_min();

void op_set_npc_stat_max();

void op_set_npc_stat_min();

void mf_get_stat_max(OpcodeContext&);

void mf_get_stat_min(OpcodeContext&);

void op_set_xp_mod(OpcodeContext&);

}
}
