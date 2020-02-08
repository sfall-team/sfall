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

class OpcodeContext;

void sf_force_encounter(OpcodeContext&);

DWORD ForceEncounterRestore();

// world_map_functions
void __declspec() op_in_world_map();

void __declspec() op_get_game_mode();

void __declspec() op_get_world_map_x_pos();

void __declspec() op_get_world_map_y_pos();

void __declspec() op_set_world_map_pos();

void __declspec() op_set_map_time_multi();

void sf_set_car_intface_art(OpcodeContext&);

void sf_set_map_enter_position(OpcodeContext&);

void sf_get_map_enter_position(OpcodeContext&);

void sf_set_rest_heal_time(OpcodeContext&);

void sf_set_rest_mode(OpcodeContext&);

void sf_set_rest_on_map(OpcodeContext&);

void sf_get_rest_on_map(OpcodeContext&);

void sf_tile_by_position(OpcodeContext&);

void sf_set_terrain_name(OpcodeContext&);

void sf_set_town_title(OpcodeContext&);

}
}
