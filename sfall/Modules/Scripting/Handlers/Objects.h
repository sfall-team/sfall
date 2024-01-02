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

void op_inc_npc_level(OpcodeContext&);

void op_get_npc_level(OpcodeContext&);

void op_remove_script(OpcodeContext&);

void op_set_script(OpcodeContext&);

void op_create_spatial(OpcodeContext&);

void mf_spatial_radius(OpcodeContext&);

void op_get_script(OpcodeContext&);

void op_set_critter_burst_disable(OpcodeContext&);

void op_get_weapon_ammo_pid(OpcodeContext&);

void op_set_weapon_ammo_pid(OpcodeContext&);

void op_get_weapon_ammo_count(OpcodeContext&);

void op_set_weapon_ammo_count(OpcodeContext&);

void op_make_straight_path(OpcodeContext&);

void op_make_path(OpcodeContext&);

void op_obj_blocking_at(OpcodeContext&);

void op_tile_get_objects(OpcodeContext&);

void op_get_party_members(OpcodeContext&);

void mf_set_outline(OpcodeContext&);

void mf_get_outline(OpcodeContext&);

void mf_set_flags(OpcodeContext&);

void mf_get_flags(OpcodeContext&);

void mf_outlined_object(OpcodeContext&);

void mf_set_dude_obj(OpcodeContext&);

void mf_real_dude_obj(OpcodeContext&);

void mf_car_gas_amount(OpcodeContext&);

void mf_lock_is_jammed(OpcodeContext&);

void mf_unjam_lock(OpcodeContext&);

void mf_set_unjam_locks_time(OpcodeContext&);

void mf_item_make_explosive(OpcodeContext&);

void mf_get_dialog_object(OpcodeContext&);

void mf_obj_under_cursor(OpcodeContext&);

void mf_get_loot_object(OpcodeContext&);

void op_get_proto_data(OpcodeContext&);

void op_set_proto_data(OpcodeContext&);

void mf_get_object_data(OpcodeContext&);

void mf_set_object_data(OpcodeContext&);

void mf_get_object_ai_data(OpcodeContext&);

void mf_set_drugs_data(OpcodeContext&);

void mf_set_unique_id(OpcodeContext&);

void mf_objects_in_radius(OpcodeContext&);

void mf_npc_engine_level_up(OpcodeContext&);

void mf_obj_is_openable(OpcodeContext&);

}
}
