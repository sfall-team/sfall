/*
* sfall
* Copyright (C) 2008-2016 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Variables.h"

// global variables
long* ptr_pc_traits					  = reinterpret_cast<long*>(_pc_trait); // 2 of them

DWORD* ptr_aiInfoList                 = reinterpret_cast<DWORD*>(_aiInfoList);
DWORD* ptr_ambient_light              = reinterpret_cast<DWORD*>(_ambient_light);
DWORD* ptr_art                        = reinterpret_cast<DWORD*>(_art);
DWORD* ptr_art_name                   = reinterpret_cast<DWORD*>(_art_name);
DWORD* ptr_art_vault_guy_num          = reinterpret_cast<DWORD*>(_art_vault_guy_num);
DWORD* ptr_art_vault_person_nums      = reinterpret_cast<DWORD*>(_art_vault_person_nums);
DWORD* ptr_bckgnd                     = reinterpret_cast<DWORD*>(_bckgnd);
DWORD* ptr_black_palette              = reinterpret_cast<DWORD*>(_black_palette);
DWORD* ptr_bottom_line                = reinterpret_cast<DWORD*>(_bottom_line);
DWORD* ptr_btable                     = reinterpret_cast<DWORD*>(_btable);
DWORD* ptr_btncnt                     = reinterpret_cast<DWORD*>(_btncnt);
DWORD* ptr_CarCurrArea                = reinterpret_cast<DWORD*>(_CarCurrArea);
DWORD* ptr_cmap                       = reinterpret_cast<DWORD*>(_cmap);
DWORD* ptr_colorTable                 = reinterpret_cast<DWORD*>(_colorTable);
DWORD* ptr_combat_free_move           = reinterpret_cast<DWORD*>(_combat_free_move);
DWORD* ptr_combat_list                = reinterpret_cast<DWORD*>(_combat_list);
DWORD* ptr_combat_state               = reinterpret_cast<DWORD*>(_combat_state);
DWORD* ptr_combat_turn_running        = reinterpret_cast<DWORD*>(_combat_turn_running);
DWORD* ptr_combatNumTurns             = reinterpret_cast<DWORD*>(_combatNumTurns);
DWORD* ptr_crit_succ_eff              = reinterpret_cast<DWORD*>(_crit_succ_eff);
DWORD* ptr_critter_db_handle          = reinterpret_cast<DWORD*>(_critter_db_handle);
DWORD* ptr_critterClearObj            = reinterpret_cast<DWORD*>(_critterClearObj);
DWORD* ptr_crnt_func                  = reinterpret_cast<DWORD*>(_crnt_func);
DWORD* ptr_curr_font_num              = reinterpret_cast<DWORD*>(_curr_font_num);
DWORD* ptr_curr_pc_stat               = reinterpret_cast<DWORD*>(_curr_pc_stat);
DWORD* ptr_curr_stack                 = reinterpret_cast<DWORD*>(_curr_stack);
DWORD* ptr_cursor_line                = reinterpret_cast<DWORD*>(_cursor_line);
DWORD* ptr_dialog_target              = reinterpret_cast<DWORD*>(_dialog_target);
DWORD* ptr_dialog_target_is_party     = reinterpret_cast<DWORD*>(_dialog_target_is_party);
DWORD* ptr_drugInfoList               = reinterpret_cast<DWORD*>(_drugInfoList);
DWORD* ptr_edit_win                   = reinterpret_cast<DWORD*>(_edit_win);
DWORD* ptr_Educated                   = reinterpret_cast<DWORD*>(_Educated);
DWORD* ptr_Experience_                = reinterpret_cast<DWORD*>(_Experience_);
DWORD* ptr_fallout_game_time          = reinterpret_cast<DWORD*>(_fallout_game_time);
DWORD* ptr_flptr                      = reinterpret_cast<DWORD*>(_flptr);
DWORD* ptr_folder_card_desc           = reinterpret_cast<DWORD*>(_folder_card_desc);
DWORD* ptr_folder_card_fid            = reinterpret_cast<DWORD*>(_folder_card_fid);
DWORD* ptr_folder_card_title          = reinterpret_cast<DWORD*>(_folder_card_title);
DWORD* ptr_folder_card_title2         = reinterpret_cast<DWORD*>(_folder_card_title2);
DWORD* ptr_frame_time                 = reinterpret_cast<DWORD*>(_frame_time);
char* ptr_free_perk                   = reinterpret_cast<char*>(_free_perk);
DWORD* ptr_game_global_vars           = reinterpret_cast<DWORD*>(_game_global_vars);
DWORD* ptr_game_user_wants_to_quit    = reinterpret_cast<DWORD*>(_game_user_wants_to_quit);
DWORD* ptr_gcsd                       = reinterpret_cast<DWORD*>(_gcsd);
DWORD* ptr_gdBarterMod                = reinterpret_cast<DWORD*>(_gdBarterMod);
DWORD* ptr_gdNumOptions               = reinterpret_cast<DWORD*>(_gdNumOptions);
DWORD* ptr_gIsSteal                   = reinterpret_cast<DWORD*>(_gIsSteal);
DWORD* ptr_glblmode                   = reinterpret_cast<DWORD*>(_glblmode);
DWORD* ptr_gmouse_current_cursor      = reinterpret_cast<DWORD*>(_gmouse_current_cursor);
DWORD* ptr_gmovie_played_list         = reinterpret_cast<DWORD*>(_gmovie_played_list);
DWORD* ptr_GreenColor                 = reinterpret_cast<DWORD*>(_GreenColor);
DWORD* ptr_gsound_initialized         = reinterpret_cast<DWORD*>(_gsound_initialized);
DWORD* ptr_hit_location_penalty       = reinterpret_cast<DWORD*>(_hit_location_penalty);
DWORD* ptr_holo_flag                  = reinterpret_cast<DWORD*>(_holo_flag);
DWORD* ptr_holopages                  = reinterpret_cast<DWORD*>(_holopages);
DWORD* ptr_hot_line_count             = reinterpret_cast<DWORD*>(_hot_line_count);
DWORD* ptr_i_fid                      = reinterpret_cast<DWORD*>(_i_fid);
DWORD* ptr_i_lhand                    = reinterpret_cast<DWORD*>(_i_lhand);
DWORD* ptr_i_rhand                    = reinterpret_cast<DWORD*>(_i_rhand);
DWORD* ptr_i_wid                      = reinterpret_cast<DWORD*>(_i_wid);
DWORD* ptr_i_worn                     = reinterpret_cast<DWORD*>(_i_worn);
DWORD* ptr_In_WorldMap                = reinterpret_cast<DWORD*>(_In_WorldMap);
DWORD* ptr_info_line                  = reinterpret_cast<DWORD*>(_info_line);
DWORD* ptr_interfaceWindow            = reinterpret_cast<DWORD*>(_interfaceWindow);
DWORD* ptr_intfaceEnabled             = reinterpret_cast<DWORD*>(_intfaceEnabled);
DWORD* ptr_intotal                    = reinterpret_cast<DWORD*>(_intotal);
TGameObj** ptr_inven_dude             = reinterpret_cast<TGameObj**>(_inven_dude);
DWORD* ptr_inven_pid                  = reinterpret_cast<DWORD*>(_inven_pid);
DWORD* ptr_inven_scroll_dn_bid        = reinterpret_cast<DWORD*>(_inven_scroll_dn_bid);
DWORD* ptr_inven_scroll_up_bid        = reinterpret_cast<DWORD*>(_inven_scroll_up_bid);
DWORD* ptr_inventry_message_file      = reinterpret_cast<DWORD*>(_inventry_message_file);
DWORD* ptr_itemButtonItems            = reinterpret_cast<DWORD*>(_itemButtonItems);
DWORD* ptr_itemCurrentItem            = reinterpret_cast<DWORD*>(_itemCurrentItem); // 0 - left, 1 - right
DWORD* ptr_kb_lock_flags              = reinterpret_cast<DWORD*>(_kb_lock_flags);
DWORD* ptr_last_buttons               = reinterpret_cast<DWORD*>(_last_buttons);
DWORD* ptr_last_button_winID          = reinterpret_cast<DWORD*>(_last_button_winID);
DWORD* ptr_last_level                 = reinterpret_cast<DWORD*>(_last_level);
DWORD* ptr_Level_                     = reinterpret_cast<DWORD*>(_Level_);
DWORD* ptr_Lifegiver                  = reinterpret_cast<DWORD*>(_Lifegiver);
DWORD* ptr_list_com                   = reinterpret_cast<DWORD*>(_list_com);
DWORD* ptr_list_total                 = reinterpret_cast<DWORD*>(_list_total);
DWORD* ptr_loadingGame                = reinterpret_cast<DWORD*>(_loadingGame);
DWORD* ptr_LSData                     = reinterpret_cast<DWORD*>(_LSData);
DWORD* ptr_lsgwin                     = reinterpret_cast<DWORD*>(_lsgwin);
DWORD* ptr_main_ctd                   = reinterpret_cast<DWORD*>(_main_ctd);
DWORD* ptr_main_window                = reinterpret_cast<DWORD*>(_main_window);
DWORD* ptr_map_elevation              = reinterpret_cast<DWORD*>(_map_elevation);
DWORD* ptr_map_global_vars            = reinterpret_cast<DWORD*>(_map_global_vars);
DWORD* ptr_master_db_handle           = reinterpret_cast<DWORD*>(_master_db_handle);
DWORD* ptr_max                        = reinterpret_cast<DWORD*>(_max);
DWORD* ptr_maxScriptNum               = reinterpret_cast<DWORD*>(_maxScriptNum);
DWORD* ptr_Meet_Frank_Horrigan        = reinterpret_cast<DWORD*>(_Meet_Frank_Horrigan);
DWORD* ptr_mouse_hotx                 = reinterpret_cast<DWORD*>(_mouse_hotx);
DWORD* ptr_mouse_hoty                 = reinterpret_cast<DWORD*>(_mouse_hoty);
DWORD* ptr_mouse_is_hidden            = reinterpret_cast<DWORD*>(_mouse_is_hidden);
DWORD* ptr_mouse_x_                   = reinterpret_cast<DWORD*>(_mouse_x_);
DWORD* ptr_mouse_y                    = reinterpret_cast<DWORD*>(_mouse_y);
DWORD* ptr_mouse_y_                   = reinterpret_cast<DWORD*>(_mouse_y_);
DWORD* ptr_Mutate_                    = reinterpret_cast<DWORD*>(_Mutate_);
DWORD* ptr_name_color                 = reinterpret_cast<DWORD*>(_name_color);
DWORD* ptr_name_font                  = reinterpret_cast<DWORD*>(_name_font);
DWORD* ptr_name_sort_list             = reinterpret_cast<DWORD*>(_name_sort_list);
DWORD* ptr_num_game_global_vars       = reinterpret_cast<DWORD*>(_num_game_global_vars);
DWORD* ptr_num_map_global_vars        = reinterpret_cast<DWORD*>(_num_map_global_vars);
TGameObj** ptr_obj_dude               = reinterpret_cast<TGameObj**>(_obj_dude);
DWORD* ptr_objectTable                = reinterpret_cast<DWORD*>(_objectTable);
DWORD* ptr_objItemOutlineState        = reinterpret_cast<DWORD*>(_objItemOutlineState);
DWORD* ptr_optionRect                 = reinterpret_cast<DWORD*>(_optionRect);
DWORD* ptr_outlined_object            = reinterpret_cast<DWORD*>(_outlined_object);
DWORD* ptr_partyMemberAIOptions       = reinterpret_cast<DWORD*>(_partyMemberAIOptions);
DWORD* ptr_partyMemberCount           = reinterpret_cast<DWORD*>(_partyMemberCount);
DWORD* ptr_partyMemberLevelUpInfoList = reinterpret_cast<DWORD*>(_partyMemberLevelUpInfoList);
DWORD* ptr_partyMemberList            = reinterpret_cast<DWORD*>(_partyMemberList); // each struct - 4 integers, first integer - objPtr
DWORD* ptr_partyMemberMaxCount        = reinterpret_cast<DWORD*>(_partyMemberMaxCount);
DWORD* ptr_partyMemberPidList         = reinterpret_cast<DWORD*>(_partyMemberPidList);
DWORD* ptr_patches                    = reinterpret_cast<DWORD*>(_patches);
DWORD* ptr_paths                      = reinterpret_cast<DWORD*>(_paths);
DWORD* ptr_pc_crit_succ_eff           = reinterpret_cast<DWORD*>(_pc_crit_succ_eff);
DWORD* ptr_pc_kill_counts             = reinterpret_cast<DWORD*>(_pc_kill_counts);
char* ptr_pc_name                     = reinterpret_cast<char*>(_pc_name);
DWORD* ptr_pc_proto                   = reinterpret_cast<DWORD*>(_pc_proto);
DWORD* ptr_perk_data                  = reinterpret_cast<DWORD*>(_perk_data);
int** ptr_perkLevelDataList           = reinterpret_cast<int**>(_perkLevelDataList);
DWORD* ptr_pip_win                    = reinterpret_cast<DWORD*>(_pip_win);
DWORD* ptr_pipboy_message_file        = reinterpret_cast<DWORD*>(_pipboy_message_file);
DWORD* ptr_pipmesg                    = reinterpret_cast<DWORD*>(_pipmesg);
DWORD* ptr_preload_list_index         = reinterpret_cast<DWORD*>(_preload_list_index);
DWORD* ptr_procTableStrs              = reinterpret_cast<DWORD*>(_procTableStrs);  // table of procId (from define.h) => procName map
DWORD* ptr_proto_main_msg_file        = reinterpret_cast<DWORD*>(_proto_main_msg_file);
DWORD* ptr_ptable                     = reinterpret_cast<DWORD*>(_ptable);
DWORD* ptr_pud                        = reinterpret_cast<DWORD*>(_pud);
DWORD* ptr_queue                      = reinterpret_cast<DWORD*>(_queue);
DWORD* ptr_quick_done                 = reinterpret_cast<DWORD*>(_quick_done);
DWORD* ptr_read_callback              = reinterpret_cast<DWORD*>(_read_callback);
DWORD* ptr_RedColor                   = reinterpret_cast<DWORD*>(_RedColor);
DWORD* ptr_retvals                    = reinterpret_cast<DWORD*>(_retvals);
DWORD* ptr_scr_size                   = reinterpret_cast<DWORD*>(_scr_size);
DWORD* ptr_scriptListInfo             = reinterpret_cast<DWORD*>(_scriptListInfo);
DWORD* ptr_skill_data                 = reinterpret_cast<DWORD*>(_skill_data);
DWORD* ptr_slot_cursor                = reinterpret_cast<DWORD*>(_slot_cursor);
DWORD* ptr_sneak_working              = reinterpret_cast<DWORD*>(_sneak_working); // DWORD var 
DWORD* ptr_square                     = reinterpret_cast<DWORD*>(_square);
DWORD* ptr_squares                    = reinterpret_cast<DWORD*>(_squares);
DWORD* ptr_stack                      = reinterpret_cast<DWORD*>(_stack);
DWORD* ptr_stack_offset               = reinterpret_cast<DWORD*>(_stack_offset);
DWORD* ptr_stat_data                  = reinterpret_cast<DWORD*>(_stat_data);
DWORD* ptr_stat_flag                  = reinterpret_cast<DWORD*>(_stat_flag);
DWORD* ptr_Tag_                       = reinterpret_cast<DWORD*>(_Tag_);
DWORD* ptr_tag_skill                  = reinterpret_cast<DWORD*>(_tag_skill);
DWORD* ptr_target_curr_stack          = reinterpret_cast<DWORD*>(_target_curr_stack);
DWORD* ptr_target_pud                 = reinterpret_cast<DWORD*>(_target_pud);
DWORD* ptr_target_stack               = reinterpret_cast<DWORD*>(_target_stack);
DWORD* ptr_target_stack_offset        = reinterpret_cast<DWORD*>(_target_stack_offset);
DWORD* ptr_target_str                 = reinterpret_cast<DWORD*>(_target_str);
DWORD* ptr_target_xpos                = reinterpret_cast<DWORD*>(_target_xpos);
DWORD* ptr_target_ypos                = reinterpret_cast<DWORD*>(_target_ypos);
DWORD* ptr_text_char_width            = reinterpret_cast<DWORD*>(_text_char_width);
DWORD* ptr_text_height                = reinterpret_cast<DWORD*>(_text_height);
DWORD* ptr_text_max                   = reinterpret_cast<DWORD*>(_text_max);
DWORD* ptr_text_mono_width            = reinterpret_cast<DWORD*>(_text_mono_width);
DWORD* ptr_text_spacing               = reinterpret_cast<DWORD*>(_text_spacing);
DWORD* ptr_text_to_buf                = reinterpret_cast<DWORD*>(_text_to_buf);
DWORD* ptr_text_width                 = reinterpret_cast<DWORD*>(_text_width);
DWORD* ptr_title_color                = reinterpret_cast<DWORD*>(_title_color);
DWORD* ptr_title_font                 = reinterpret_cast<DWORD*>(_title_font);
DWORD* ptr_trait_data                 = reinterpret_cast<DWORD*>(_trait_data);
DWORD* ptr_view_page                  = reinterpret_cast<DWORD*>(_view_page);
DWORD* ptr_wd_obj                     = reinterpret_cast<DWORD*>(_wd_obj);
DWORD* ptr_wmAreaInfoList             = reinterpret_cast<DWORD*>(_wmAreaInfoList);
DWORD* ptr_wmLastRndTime              = reinterpret_cast<DWORD*>(_wmLastRndTime);
DWORD* ptr_wmWorldOffsetX             = reinterpret_cast<DWORD*>(_wmWorldOffsetX);
DWORD* ptr_wmWorldOffsetY             = reinterpret_cast<DWORD*>(_wmWorldOffsetY);
DWORD* ptr_world_xpos                 = reinterpret_cast<DWORD*>(_world_xpos);
DWORD* ptr_world_ypos                 = reinterpret_cast<DWORD*>(_world_ypos);
DWORD* ptr_WorldMapCurrArea           = reinterpret_cast<DWORD*>(_WorldMapCurrArea);
DWORD* ptr_YellowColor                = reinterpret_cast<DWORD*>(_YellowColor);

