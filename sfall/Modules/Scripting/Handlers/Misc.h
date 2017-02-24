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

class OpcodeContext;

void __declspec() op_set_dm_model();

void __declspec() op_set_df_model();

void __declspec() op_set_movie_path();

void __declspec() op_get_year();

void __declspec() op_game_loaded();

void __declspec() op_set_map_time_multi();

void __declspec() op_set_pipboy_available();

// Kill counters
void __declspec() op_get_kill_counter();

void __declspec() op_mod_kill_counter();

void __declspec() op_set_weapon_knockback();

void __declspec() op_set_target_knockback();

void __declspec() op_set_attacker_knockback();

void __declspec() op_remove_weapon_knockback();

void __declspec() op_remove_target_knockback();

void __declspec() op_remove_attacker_knockback();

void __declspec() op_get_kill_counter2();

void __declspec() op_mod_kill_counter2();

void __declspec() op_active_hand();

void __declspec() op_toggle_active_hand();

void __declspec() op_eax_available();

void __declspec() op_inc_npc_level();

void __declspec() op_get_npc_level();

void __declspec() op_get_ini_setting();

void __declspec() op_get_ini_string();

void __declspec() op_get_uptime();

void __declspec() op_set_car_current_town();

void __declspec() op_set_hp_per_level_mod();

void __declspec() op_get_bodypart_hit_modifier();

void __declspec() op_set_bodypart_hit_modifier();

void __declspec() op_set_critical_table();

void __declspec() op_get_critical_table();

void __declspec() op_reset_critical_table();

void __declspec() op_set_unspent_ap_bonus();

void __declspec() op_get_unspent_ap_bonus();

void __declspec() op_set_unspent_ap_perk_bonus();

void __declspec() op_get_unspent_ap_perk_bonus();

void __declspec() op_set_palette();

//numbers subgame functions
void __declspec() op_nb_create_char();

void __declspec() op_get_proto_data();

void __declspec() op_set_proto_data();

void __declspec() op_hero_select_win() ;

void __declspec() op_set_hero_style();

void __declspec() op_set_hero_race();

void __declspec() op_get_light_level();

void __declspec() op_refresh_pc_art();

void __declspec() op_get_attack_type();

void __declspec() op_play_sfall_sound();

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

void sf_sneak_success(OpcodeContext& ctx);

void sf_tile_light(OpcodeContext& ctx);

void sf_exec_map_update_scripts(OpcodeContext&);
