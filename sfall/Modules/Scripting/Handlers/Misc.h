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

/*
 *	Misc operators
 */

namespace sfall
{
namespace script
{

class OpcodeContext;

void sf_set_dm_model(OpcodeContext&);

void sf_set_df_model(OpcodeContext&);

void sf_set_movie_path(OpcodeContext&);

void sf_get_year(OpcodeContext&);

void __declspec() op_game_loaded();

void __declspec() op_set_pipboy_available();

// Kill counters
void SetExtraKillCounter(bool value);

void __declspec() op_get_kill_counter();

void __declspec() op_mod_kill_counter();

void sf_set_object_knockback(OpcodeContext&);

void sf_remove_object_knockback(OpcodeContext&);

void __declspec() op_active_hand();

void __declspec() op_toggle_active_hand();

void __declspec() op_eax_available();

void sf_inc_npc_level(OpcodeContext&);

void sf_get_npc_level(OpcodeContext&);

void sf_get_ini_setting(OpcodeContext&);

void sf_get_ini_string(OpcodeContext&);

void __declspec() op_get_uptime();

void __declspec() op_set_car_current_town();

void __declspec() op_set_hp_per_level_mod();

void __declspec() op_get_bodypart_hit_modifier();

void __declspec() op_set_bodypart_hit_modifier();

void sf_set_critical_table(OpcodeContext&);

void sf_get_critical_table(OpcodeContext&);

void sf_reset_critical_table(OpcodeContext&);

void __declspec() op_set_unspent_ap_bonus();

void __declspec() op_get_unspent_ap_bonus();

void __declspec() op_set_unspent_ap_perk_bonus();

void __declspec() op_get_unspent_ap_perk_bonus();

void sf_set_palette(OpcodeContext&);

//numbers subgame functions
void __declspec() op_nb_create_char();

void __declspec() op_hero_select_win() ;

void __declspec() op_set_hero_style();

void __declspec() op_set_hero_race();

void __declspec() op_get_light_level();

void __declspec() op_refresh_pc_art();

void sf_get_attack_type(OpcodeContext&);

void sf_play_sfall_sound(OpcodeContext&);

void __declspec() op_stop_sfall_sound();

void __declspec() op_get_tile_fid();

void __declspec() op_modified_ini();

void __declspec() op_force_aimed_shots();

void __declspec() op_disable_aimed_shots();

void __declspec() op_mark_movie_played();

void __declspec() op_get_last_attacker();

void __declspec() op_get_last_target();

void __declspec() op_block_combat();

void __declspec() op_tile_under_cursor();

void __declspec() op_gdialog_get_barter_mod();

void __declspec() op_set_inven_ap_cost();

void sf_get_inven_ap_cost(OpcodeContext&);

void sf_attack_is_aimed(OpcodeContext&);

void sf_sneak_success(OpcodeContext&);

void sf_tile_light(OpcodeContext&);

void sf_exec_map_update_scripts(OpcodeContext&);

void sf_set_ini_setting(OpcodeContext&);

void sf_get_ini_sections(OpcodeContext&);

void sf_get_ini_section(OpcodeContext&);

void sf_npc_engine_level_up(OpcodeContext&);

}
}
