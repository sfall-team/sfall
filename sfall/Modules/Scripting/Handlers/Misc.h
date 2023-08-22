/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

/*
 *	Misc operators
 */

namespace sfall
{
namespace script
{

class OpcodeContext;

//Stop game, the same effect as open charsscreen or inventory
void __declspec() op_stop_game();

//Resume the game when it is stopped
void __declspec() op_resume_game();

void op_set_dm_model(OpcodeContext&);

void op_set_df_model(OpcodeContext&);

void op_set_movie_path(OpcodeContext&);

void op_get_year(OpcodeContext&);

void __declspec() op_eax_available();

void __declspec() op_set_eax_environment();

void __declspec() op_get_uptime();

void __declspec() op_set_car_current_town();

void op_set_critical_table(OpcodeContext&);

void op_get_critical_table(OpcodeContext&);

void op_reset_critical_table(OpcodeContext&);

void op_set_palette(OpcodeContext&);

//numbers subgame functions
void __declspec() op_nb_create_char();

void __declspec() op_hero_select_win();

void __declspec() op_set_hero_style();

void __declspec() op_set_hero_race();

void __declspec() op_get_light_level();

void __declspec() op_refresh_pc_art();

void op_play_sfall_sound(OpcodeContext&);

void __declspec() op_stop_sfall_sound();

void op_get_tile_fid(OpcodeContext&);

void __declspec() op_mark_movie_played();

void __declspec() op_tile_under_cursor();

void __declspec() op_gdialog_get_barter_mod();

void op_sneak_success(OpcodeContext&);

void op_tile_light(OpcodeContext&);

void mf_exec_map_update_scripts(OpcodeContext&);

void mf_set_quest_failure_value(OpcodeContext&);

void mf_set_scr_name(OpcodeContext&);

}
}
