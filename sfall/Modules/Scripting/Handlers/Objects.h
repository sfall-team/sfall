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

#if (_MSC_VER < 1600)
#include "..\..\win9x\Cpp11_emu.h"
#endif

#include "..\..\..\main.h"
#include "..\..\Inventory.h"
#include "..\..\ScriptExtender.h"

//script control functions

void __declspec() op_remove_script();

void __declspec() op_set_script();

void __declspec() op_create_spatial();

void sf_spatial_radius(OpcodeHandler& opHandler);

void __declspec() op_get_script();

void __declspec() op_set_critter_burst_disable();

void __declspec() op_get_weapon_ammo_pid();

void __declspec() op_set_weapon_ammo_pid();

void __declspec() op_get_weapon_ammo_count();

void __declspec() op_set_weapon_ammo_count();

void __declspec() op_make_straight_path();

void __declspec() op_make_path();

void __declspec() op_obj_blocking_at();

void __declspec() op_tile_get_objects();

void __declspec() op_get_party_members();

void __declspec() op_art_exists();

void __declspec() op_obj_is_carrying_obj();

void sf_critter_inven_obj2(OpcodeHandler& opHandler);
