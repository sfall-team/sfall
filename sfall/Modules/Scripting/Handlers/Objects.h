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

#include "..\..\..\main.h"
#include "..\..\Inventory.h"
#include "..\..\ScriptExtender.h"

namespace sfall
{
namespace script
{

//script control functions

class OpcodeContext;

void __declspec() op_remove_script();

void __declspec() op_set_script();

void sf_create_spatial(OpcodeContext&);

void sf_spatial_radius(OpcodeContext&);

void sf_get_script(OpcodeContext&);

void sf_set_critter_burst_disable(OpcodeContext&);

void sf_get_weapon_ammo_pid(OpcodeContext&);

void sf_set_weapon_ammo_pid(OpcodeContext&);

void sf_get_weapon_ammo_count(OpcodeContext&);

void sf_set_weapon_ammo_count(OpcodeContext&);

void sf_make_straight_path(OpcodeContext&);

void sf_make_path(OpcodeContext&);

void sf_obj_blocking_at(OpcodeContext&);

void sf_tile_get_objects(OpcodeContext&);

void sf_get_party_members(OpcodeContext&);

void sf_art_exists(OpcodeContext&);

void sf_obj_is_carrying_obj(OpcodeContext&);

void sf_critter_inven_obj2(OpcodeContext&);

void sf_set_outline(OpcodeContext&);

void sf_get_outline(OpcodeContext&);

void sf_set_flags(OpcodeContext&);

void sf_get_flags(OpcodeContext&);

void sf_outlined_object(OpcodeContext&);

void sf_item_weight(OpcodeContext&);

void sf_set_dude_obj(OpcodeContext&);

void sf_real_dude_obj(OpcodeContext&);

void sf_car_gas_amount(OpcodeContext&);

void sf_lock_is_jammed(OpcodeContext&);

void sf_unjam_lock(OpcodeContext&);

void sf_set_unjam_locks_time(OpcodeContext&);

void sf_item_make_explosive(OpcodeContext&);

}
}
