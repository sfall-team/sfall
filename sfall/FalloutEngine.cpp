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

#include "main.h"

#include "Define.h"
#include "FalloutEngine.h"
#include "Logging.h"

// global variables
long* ptr_pc_traits                   = reinterpret_cast<long*>(_pc_trait); // 2 of them

DWORD* ptr_aiInfoList                 = reinterpret_cast<DWORD*>(_aiInfoList);
DWORD* ptr_ambient_light              = reinterpret_cast<DWORD*>(_ambient_light);
Art*   ptr_art                        = reinterpret_cast<Art*>(_art);
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
DWORD* ptr_elevation                  = reinterpret_cast<DWORD*>(_elevation);
DWORD* ptr_Experience_                = reinterpret_cast<DWORD*>(_Experience_);
DWORD* ptr_fallout_game_time          = reinterpret_cast<DWORD*>(_fallout_game_time);
DWORD* ptr_fidgetFID                  = reinterpret_cast<DWORD*>(_fidgetFID);
DWORD* ptr_flptr                      = reinterpret_cast<DWORD*>(_flptr);
DWORD* ptr_folder_card_desc           = reinterpret_cast<DWORD*>(_folder_card_desc);
DWORD* ptr_folder_card_fid            = reinterpret_cast<DWORD*>(_folder_card_fid);
DWORD* ptr_folder_card_title          = reinterpret_cast<DWORD*>(_folder_card_title);
DWORD* ptr_folder_card_title2         = reinterpret_cast<DWORD*>(_folder_card_title2);
DWORD* ptr_frame_time                 = reinterpret_cast<DWORD*>(_frame_time);
char*  ptr_free_perk                  = reinterpret_cast<char*>(_free_perk);
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
DWORD* ptr_lipsFID                    = reinterpret_cast<DWORD*>(_lipsFID);
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
DWORD* ptr_optionsButtonDown          = reinterpret_cast<DWORD*>(_optionsButtonDown);
DWORD* ptr_optionsButtonDown1         = reinterpret_cast<DWORD*>(_optionsButtonDown1);
DWORD* ptr_optionsButtonDownKey       = reinterpret_cast<DWORD*>(_optionsButtonDownKey);
DWORD* ptr_optionsButtonUp            = reinterpret_cast<DWORD*>(_optionsButtonUp);
DWORD* ptr_optionsButtonUp1           = reinterpret_cast<DWORD*>(_optionsButtonUp1);
DWORD* ptr_optionsButtonUpKey         = reinterpret_cast<DWORD*>(_optionsButtonUpKey);
DWORD* ptr_outlined_object            = reinterpret_cast<DWORD*>(_outlined_object);
DWORD* ptr_partyMemberAIOptions       = reinterpret_cast<DWORD*>(_partyMemberAIOptions);
DWORD* ptr_partyMemberCount           = reinterpret_cast<DWORD*>(_partyMemberCount);
DWORD** ptr_partyMemberLevelUpInfoList = reinterpret_cast<DWORD**>(_partyMemberLevelUpInfoList);
DWORD** ptr_partyMemberList           = reinterpret_cast<DWORD**>(_partyMemberList); // each struct - 4 integers, first integer - objPtr
DWORD* ptr_partyMemberMaxCount        = reinterpret_cast<DWORD*>(_partyMemberMaxCount);
DWORD** ptr_partyMemberPidList        = reinterpret_cast<DWORD**>(_partyMemberPidList);
DWORD* ptr_patches                    = reinterpret_cast<DWORD*>(_patches);
DWORD* ptr_paths                      = reinterpret_cast<DWORD*>(_paths);
DWORD* ptr_pc_crit_succ_eff           = reinterpret_cast<DWORD*>(_pc_crit_succ_eff);
DWORD* ptr_pc_kill_counts             = reinterpret_cast<DWORD*>(_pc_kill_counts);
char*  ptr_pc_name                    = reinterpret_cast<char*>(_pc_name);
DWORD* ptr_pc_proto                   = reinterpret_cast<DWORD*>(_pc_proto);
DWORD* ptr_perk_data                  = reinterpret_cast<DWORD*>(_perk_data);
int**  ptr_perkLevelDataList          = reinterpret_cast<int**>(_perkLevelDataList);
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
DWORD* ptr_rotation                   = reinterpret_cast<DWORD*>(_rotation);
BoundRect* ptr_scr_size               = reinterpret_cast<BoundRect*>(_scr_size);
DWORD* ptr_scriptListInfo             = reinterpret_cast<DWORD*>(_scriptListInfo);
DWORD* ptr_skill_data                 = reinterpret_cast<DWORD*>(_skill_data);
DWORD* ptr_slot_cursor                = reinterpret_cast<DWORD*>(_slot_cursor);
DWORD* ptr_sneak_working              = reinterpret_cast<DWORD*>(_sneak_working); // DWORD var
char** ptr_sound_music_path1          = reinterpret_cast<char**>(_sound_music_path1);
char** ptr_sound_music_path2          = reinterpret_cast<char**>(_sound_music_path2);
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
DWORD* ptr_tile                       = reinterpret_cast<DWORD*>(_tile);
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

/**
	ENGINE FUNCTIONS OFFSETS
	const names should end with underscore
*/


// AI FUNCTIONS
const DWORD ai_can_use_weapon_ = 0x4298EC;  // (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0
const DWORD ai_check_drugs_ = 0x428480;
const DWORD ai_run_away_ = 0x428868;
const DWORD ai_search_inven_armor_ = 0x429A6C;
const DWORD ai_try_attack_ = 0x42A7D8;

// UI FUNCTIONS
const DWORD interface_disable_ = 0x45EAFC;
const DWORD interface_enable_ = 0x45EA64;
const DWORD intface_item_reload_ = 0x460B20;

// OBJECTS manipulation
const DWORD obj_new_ = 0x489A84;  // int aObj*<eax>, int aPid<ebx>
const DWORD obj_turn_off_ = 0x48AE68;  // int aObj<eax>, int ???<edx>
const DWORD obj_move_to_tile_ = 0x48A568;  // int aObj<eax>, int aTile<edx>, int aElev<ebx>


// misc functions in alphabetical order
const DWORD action_get_an_object_ = 0x412134;
const DWORD action_loot_container_ = 0x4123E8;
const DWORD action_use_an_item_on_object_ = 0x411F2C;
const DWORD add_bar_box_ = 0x4616F0;
const DWORD AddHotLines_ = 0x4998C0;
const DWORD adjust_ac_ = 0x4715F8;
const DWORD adjust_fid_ = 0x4716E8;
const DWORD art_alias_num_ = 0x419998;
const DWORD art_exists_ = 0x4198C8; // eax - frameID, used for critter FIDs
const DWORD art_flush_ = 0x41927C;
const DWORD art_frame_data_ = 0x419870;
const DWORD art_frame_length_ = 0x4197B8;
const DWORD art_frame_width_ = 0x4197A0;
const DWORD art_get_code_ = 0x419314;
const DWORD art_id_ = 0x419C88;
const DWORD art_init_ = 0x418840;
const DWORD art_lock_ = 0x4191CC;
const DWORD art_ptr_lock_ = 0x419160;
const DWORD art_ptr_lock_data_ = 0x419188;
const DWORD art_ptr_unlock_ = 0x419260;
const DWORD attack_crit_success_ = 0x423EB4;
const DWORD automap_ = 0x41B8BC;
const DWORD barter_compute_value_ = 0x474B2C;
const DWORD barter_inventory_ = 0x4757F0;
const DWORD buf_to_buf_ = 0x4D36D4;
const DWORD check_death_ = 0x410814;
const DWORD Check4Keys_ = 0x43F73C;
const DWORD combat_ = 0x422D2C;
const DWORD combat_ai_ = 0x42B130;
const DWORD combat_anim_finished_ = 0x425E80;
const DWORD combat_attack_ = 0x422F3C;
const DWORD combat_delete_critter_ = 0x426DDC;
const DWORD combat_input_ = 0x4227F4;
const DWORD combat_should_end_ = 0x422C60;
const DWORD combat_turn_ = 0x42299C;
const DWORD combat_turn_run_ = 0x4227DC;
const DWORD compute_damage_ = 0x4247B8;
const DWORD config_get_string_ = 0x42BF48;
const DWORD config_set_value_ = 0x42C160;
const DWORD container_exit_ = 0x476394;
const DWORD correctFidForRemovedItem_ = 0x45409C;
const DWORD createWindow_ = 0x4B7F3C;
const DWORD credits_ = 0x42C860;
const DWORD credits_get_next_line_ = 0x42CE6C;
const DWORD critter_body_type_ = 0x42DDC4;
const DWORD critter_can_obj_dude_rest_ = 0x42E564;
const DWORD critter_compute_ap_from_distance_ = 0x42E62C;
const DWORD critter_flag_check_ = 0x42E6AC;
const DWORD critter_get_hits_ = 0x42D18C;
const DWORD critter_is_dead_ = 0x42DD18;
const DWORD critter_kill_ = 0x42DA64;
const DWORD critter_kill_count_type_ = 0x42D920;
const DWORD critter_name_ = 0x42D0A8;
const DWORD critter_pc_set_name_ = 0x42D138;
const DWORD critterClearObjDrugs_ = 0x42DA54;
const DWORD critterIsOverloaded_ = 0x42E66C;
const DWORD db_access_ = 0x4390B4;
const DWORD db_dir_entry_ = 0x4C5D68;
const DWORD db_fclose_ = 0x4C5EB4;
const DWORD db_fgetc_ = 0x4C5F24;
const DWORD db_fgets_ = 0x4C5F70;
const DWORD db_fopen_ = 0x4C5EC8;
const DWORD db_fread_ = 0x4C5FFC;
const DWORD db_freadByte_ = 0x4C60E0;
const DWORD db_freadByteCount_ = 0x4C62FC;
const DWORD db_freadInt_ = 0x4C614C;
const DWORD db_freadIntCount_ = 0x4C63BC;
const DWORD db_freadShort_ = 0x4C60F4;
const DWORD db_freadShortCount_ = 0x4C6330;
const DWORD db_free_file_list_ = 0x4C6868;
const DWORD db_fseek_ = 0x4C60C0;
const DWORD db_fwriteByte_ = 0x4C61AC;
const DWORD db_fwriteByteCount_ = 0x4C6464;
const DWORD db_fwriteInt_ = 0x4C6214;
const DWORD db_get_file_list_ = 0x4C6628;
const DWORD db_read_to_buf_ = 0x4C5DD4;
const DWORD dbase_close_ = 0x4E5270;
const DWORD dbase_open_ = 0x4E4F58;
const DWORD debug_log_ = 0x4C7028;
const DWORD debug_printf_ = 0x4C6F48;
const DWORD debug_register_env_ = 0x4C6D90;
const DWORD determine_to_hit_func_ = 0x4243A8;
const DWORD dialog_out_ = 0x41CF20;
const DWORD display_inventory_ = 0x46FDF4;
const DWORD display_print_ = 0x43186C;
const DWORD display_scroll_down_ = 0x431B9C;
const DWORD display_scroll_up_ = 0x431B70;
const DWORD display_stats_ = 0x471D5C;
const DWORD display_table_inventories_ = 0x475334;
const DWORD display_target_inventory_ = 0x47036C;
const DWORD do_options_ = 0x48FC48;
const DWORD do_optionsFunc_ = 0x48FC50;
const DWORD do_prefscreen_ = 0x490798;
const DWORD DOSCmdLineDestroy_ = 0x4E3D3C;
const DWORD DrawCard_ = 0x43AAEC;
const DWORD DrawFolder_ = 0x43410C;
const DWORD DrawInfoWin_ = 0x4365AC;
const DWORD drop_into_container_ = 0x476464;
const DWORD dude_stand_ = 0x418378;
const DWORD editor_design_ = 0x431DF8;
const DWORD elapsed_time_ = 0x4C93E0;
const DWORD elevator_end_ = 0x43F6D0;
const DWORD elevator_start_ = 0x43F324;
const DWORD endgame_slideshow_ = 0x43F788;
const DWORD EndLoad_ = 0x47F4C8;
const DWORD EndPipboy_ = 0x497828;
const DWORD exec_script_proc_ = 0x4A4810;
const DWORD executeProcedure_ = 0x46DD2C;
const DWORD findCurrentProc_ = 0x467160;
const DWORD fadeSystemPalette_ = 0x4C7320;
const DWORD findVar_ = 0x4410AC;
const DWORD folder_print_line_ = 0x43E3D8;
const DWORD frame_ptr_ = 0x419880;
const DWORD game_exit_ = 0x442C34;
const DWORD game_get_global_var_ = 0x443C68;
const DWORD game_help_ = 0x443F74;
const DWORD game_set_global_var_ = 0x443C98;
const DWORD game_time_ = 0x4A3330;
const DWORD game_time_date_ = 0x4A3338;
const DWORD gdialog_barter_cleanup_tables_ = 0x448660;
const DWORD gdialog_barter_pressed_ = 0x44A52C;
const DWORD gdialogActive_ = 0x444D2C;
const DWORD gdialogDisplayMsg_ = 0x445448;
const DWORD gdProcess_ = 0x4465C0;
const DWORD GetSlotList_ = 0x47E5D0;
const DWORD get_input_ = 0x4C8B78;
const DWORD get_time_ = 0x4C9370;
const DWORD getmsg_ = 0x48504C; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory
const DWORD gmouse_3d_get_mode_ = 0x44CB6C;
const DWORD gmouse_3d_set_mode_ = 0x44CA18;
const DWORD gmouse_is_scrolling_ = 0x44B54C;
const DWORD gmouse_set_cursor_ = 0x44C840;
const DWORD GNW_find_ = 0x4D7888;
const DWORD GNW95_process_message_ = 0x4C9CF0;
const DWORD gsnd_build_weapon_sfx_name_ = 0x451760;
const DWORD gsound_background_pause_ = 0x450B50;
const DWORD gsound_background_stop_ = 0x450AB4;
const DWORD gsound_background_unpause_ = 0x450B64;
const DWORD gsound_play_sfx_file_ = 0x4519A8;
const DWORD gsound_red_butt_press_ = 0x451970;
const DWORD gsound_red_butt_release_ = 0x451978;
const DWORD handle_inventory_ = 0x46E7B0;
const DWORD inc_game_time_ = 0x4A34CC;
const DWORD inc_stat_ = 0x4AF5D4;
const DWORD insert_withdrawal_ = 0x47A290;
const DWORD interpret_ = 0x46CCA4;
const DWORD interpretAddString_ = 0x467A80; // edx = ptr to string, eax = script
const DWORD interpretFindProcedure_ = 0x46DCD0;
const DWORD interpretFreeProgram_ = 0x467614;
const DWORD interpretGetString_ = 0x4678E0; // eax = script ptr, edx = var type, ebx = var
const DWORD interpretPopLong_ = 0x467500;
const DWORD interpretPopShort_ = 0x4674F0;
const DWORD interpretPushLong_ = 0x4674DC;
const DWORD interpretPushShort_ = 0x46748C;
const DWORD interpretError_ = 0x4671F0;
const DWORD intface_redraw_ = 0x45EB98;
const DWORD intface_toggle_item_state_ = 0x45F4E0;
const DWORD intface_toggle_items_ = 0x45F404;
const DWORD intface_update_ac_ = 0x45EDA8;
const DWORD intface_update_hit_points_ = 0x45EBD8;
const DWORD intface_update_items_ = 0x45EFEC;
const DWORD intface_update_move_points_ = 0x45EE0C;
const DWORD intface_use_item_ = 0x45F5EC;
const DWORD intface_show_ = 0x45EA10;
const DWORD intface_hide_ = 0x45E9E0;
const DWORD intface_is_hidden_ = 0x45EA5C;
const DWORD intface_get_attack_ = 0x45EF6C;
const DWORD invenUnwieldFunc_ = 0x472A64;
const DWORD invenWieldFunc_ = 0x472768;
const DWORD inven_display_msg_ = 0x472D24;
const DWORD inven_find_id_ = 0x4726EC;
const DWORD inven_left_hand_ = 0x471BBC;
const DWORD inven_pid_is_carried_ptr_ = 0x471CA0;
const DWORD inven_right_hand_ = 0x471B70;
const DWORD inven_unwield_ = 0x472A54;
const DWORD inven_wield_ = 0x472758;
const DWORD inven_worn_ = 0x471C08;
const DWORD is_pc_sneak_working_ = 0x42E3F4;
const DWORD is_within_perception_ = 0x42BA04;
const DWORD isPartyMember_ = 0x494FC4;
const DWORD item_add_force_ = 0x4772B8;
const DWORD item_add_mult_ = 0x477158;
const DWORD item_c_curr_size_ = 0x479A20;
const DWORD item_c_max_size_ = 0x479A00;
const DWORD item_caps_total_ = 0x47A6A8;
const DWORD item_d_check_addict_ = 0x47A640;
const DWORD item_d_take_drug_ = 0x479F60;
const DWORD item_drop_all_ = 0x477804;
const DWORD item_get_type_ = 0x477AFC;
const DWORD item_m_cell_pid_ = 0x479454;
const DWORD item_m_dec_charges_ = 0x4795A4;
const DWORD item_m_turn_off_ = 0x479898;
const DWORD item_move_all_ = 0x4776AC;
const DWORD item_move_all_hidden_ = 0x4776E0;
const DWORD item_move_force_ = 0x4776A4;
const DWORD item_mp_cost_ = 0x478040;
const DWORD item_remove_mult_ = 0x477490;
const DWORD item_size_ = 0x477B68;
const DWORD item_total_cost_ = 0x477DAC;
const DWORD item_total_weight_ = 0x477E98;
const DWORD item_w_anim_code_ = 0x478DA8;
const DWORD item_w_anim_weap_ = 0x47860C;
const DWORD item_w_can_reload_ = 0x478874;
const DWORD item_w_compute_ammo_cost_ = 0x4790AC;
const DWORD item_w_curr_ammo_ = 0x4786A0;
const DWORD item_w_dam_div_ = 0x479294;
const DWORD item_w_dam_mult_ = 0x479230;
const DWORD item_w_damage_ = 0x478448;
const DWORD item_w_damage_type_ = 0x478570;
const DWORD item_w_dr_adjust_ = 0x4791E0;
const DWORD item_w_max_ammo_ = 0x478674;
const DWORD item_w_mp_cost_ = 0x478B24;
const DWORD item_w_perk_ = 0x478D58;
const DWORD item_w_range_ = 0x478A1C;
const DWORD item_w_rounds_ = 0x478D80;
const DWORD item_w_subtype_ = 0x478280;
const DWORD item_w_try_reload_ = 0x478768;
const DWORD item_w_unload_ = 0x478F80;
const DWORD item_weight_ = 0x477B88;
const DWORD light_get_tile_ = 0x47A980;
const DWORD ListDrvdStats_ = 0x43527C;
const DWORD ListHoloDiskTitles_ = 0x498C40;
const DWORD ListSkills_ = 0x436154;
const DWORD ListTraits_ = 0x43B8A8;
const DWORD loadColorTable_ = 0x4C78E4;
const DWORD LoadGame_ = 0x47C640;
const DWORD loadProgram_ = 0x4A3B74;
const DWORD LoadSlot_ = 0x47DC68;
const DWORD loot_container_ = 0x473904;
const DWORD main_game_loop_ = 0x480E48;
const DWORD main_init_system_ = 0x480CC0;
const DWORD main_menu_hide_ = 0x481A00;
const DWORD main_menu_loop_ = 0x481AEC;
const DWORD make_path_func_ = 0x415EFC;
const DWORD make_straight_path_func_ = 0x4163C8;
const DWORD map_disable_bk_processes_ = 0x482104;
const DWORD map_enable_bk_processes_ = 0x4820C0;
const DWORD map_load_idx_ = 0x482B34;
const DWORD MapDirErase_ = 0x480040;
const DWORD mem_free_ = 0x4C5C24;
const DWORD mem_malloc_ = 0x4C5AD0;
const DWORD mem_realloc_ = 0x4C5B50;
const DWORD message_add_ = 0x484D68;
const DWORD message_exit_ = 0x484964;
const DWORD message_filter_ = 0x485078;
const DWORD message_find_ = 0x484D10;
const DWORD message_init_ = 0x48494C;
const DWORD message_load_ = 0x484AA4;
const DWORD message_make_path_ = 0x484CB8;
const DWORD message_search_ = 0x484C30;
const DWORD mouse_click_in_ = 0x4CA934;
const DWORD mouse_get_position_ = 0x4CA9DC;
const DWORD mouse_hide_ = 0x4CA534;
const DWORD mouse_in_ = 0x4CA8C8;
const DWORD mouse_show_ = 0x4CA34C;
const DWORD move_inventory_ = 0x474708;
const DWORD new_obj_id_ = 0x4A386C;
const DWORD NixHotLines_ = 0x4999C0;
const DWORD nrealloc_ = 0x4F1669;
const DWORD obj_ai_blocking_at_ = 0x48BA20;
const DWORD obj_blocking_at_ = 0x48B848; // (EAX *obj, EDX hexNum, EBX level)
const DWORD obj_bound_ = 0x48B66C;
const DWORD obj_change_fid_ = 0x48AA3C;
const DWORD obj_connect_ = 0x489EC4;
const DWORD obj_destroy_ = 0x49B9A0;
const DWORD obj_dist_ = 0x48BBD4;
const DWORD obj_dist_with_tile_ = 0x48BC08;
const DWORD obj_drop_ = 0x49B8B0;
const DWORD obj_erase_object_ = 0x48B0FC;
const DWORD obj_find_first_ = 0x48B3A8;
const DWORD obj_find_first_at_ = 0x48B48C;
const DWORD obj_find_first_at_tile_ = 0x48B5A8;
const DWORD obj_find_next_ = 0x48B41C;
const DWORD obj_find_next_at_ = 0x48B510;
const DWORD obj_find_next_at_tile_ = 0x48B608;
const DWORD obj_lock_is_jammed_ = 0x49D410;
const DWORD obj_new_sid_inst_ = 0x49AAC0;
const DWORD obj_outline_object_ = 0x48C2B4;
const DWORD obj_pid_new_ = 0x489C9C;
const DWORD obj_remove_outline_ = 0x48C2F0;
const DWORD obj_save_dude_ = 0x48D59C;
const DWORD obj_scroll_blocking_at_ = 0x48BB44;
const DWORD obj_set_light_ = 0x48AC90; // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
const DWORD obj_shoot_blocking_at_ = 0x48B930; // (EAX *obj, EDX hexNum, EBX level)
const DWORD obj_sight_blocking_at_ = 0x48BB88;
const DWORD obj_top_environment_ = 0x48B304;
const DWORD obj_unjam_lock_ = 0x49D480;
const DWORD obj_use_book_ = 0x49B9F0;
const DWORD obj_use_power_on_car_ = 0x49BDE8;
const DWORD object_under_mouse_ = 0x44CEC4;
const DWORD OptionWindow_ = 0x437C08;
const DWORD palette_init_ = 0x493A00;
const DWORD palette_set_to_ = 0x493B48;
const DWORD partyMemberCopyLevelInfo_ = 0x495EA8;
const DWORD partyMemberGetAIOptions_ = 0x4941F0;
const DWORD partyMemberGetCurLevel_ = 0x495FF0;
const DWORD partyMemberIncLevels_ = 0x495B60;
const DWORD partyMemberRemove_ = 0x4944DC;
const DWORD partyMemberSaveProtos_ = 0x495870;
const DWORD pc_flag_off_ = 0x42E220;
const DWORD pc_flag_on_ = 0x42E26C;
const DWORD pc_flag_toggle_ = 0x42E2B0;
const DWORD perform_withdrawal_end_ = 0x47A558;
const DWORD perk_add_ = 0x496A5C;
const DWORD perk_add_effect_ = 0x496BFC;
const DWORD perk_can_add_ = 0x49680C;
const DWORD perk_description_ = 0x496BB4;
const DWORD perk_init_ = 0x4965A0;
const DWORD perk_level_ = 0x496B78;
const DWORD perk_make_list_ = 0x496B44;
const DWORD perk_name_ = 0x496B90;
const DWORD perk_skilldex_fid_ = 0x496BD8;
const DWORD perkGetLevelData_ = 0x49678C;
const DWORD perks_dialog_ = 0x43C4F0;
const DWORD pick_death_ = 0x41060C;
const DWORD pip_back_ = 0x497B64;
const DWORD pip_print_ = 0x497A40;
const DWORD pipboy_ = 0x497004;
const DWORD PipStatus_ = 0x497BD8;
const DWORD PrintBasicStat_ = 0x434B38;
const DWORD PrintLevelWin_ = 0x434920;
const DWORD process_bk_ = 0x4C8BDC;
const DWORD protinst_use_item_ = 0x49BF38;
const DWORD protinst_use_item_on_ = 0x49C3CC;
const DWORD proto_dude_update_gender_ = 0x49F984;
const DWORD proto_list_str_ = 0x49E758;
const DWORD proto_ptr_ = 0x4A2108; // eax - PID, edx - int** - pointer to a pointer to a proto struct
const DWORD pushLongStack_ = 0x46736C; // sometimes used instead of "SetResult"
const DWORD qsort_ = 0x4F05B6;
const DWORD queue_add_ = 0x4A258C;
const DWORD queue_clear_type_ = 0x4A2790;
const DWORD queue_find_ = 0x4A26A8;
const DWORD queue_find_first_ = 0x4A295C;
const DWORD queue_find_next_ = 0x4A2994;
const DWORD queue_leaving_map_ = 0x4A2920;
const DWORD queue_remove_this_ = 0x4A264C;
const DWORD refresh_box_bar_win_ = 0x4614CC;
const DWORD register_begin_ = 0x413AF4;
const DWORD register_clear_ = 0x413C4C;
const DWORD register_end_ = 0x413CCC;
const DWORD register_object_animate_ = 0x4149D0;
const DWORD register_object_animate_and_hide_ = 0x414B7C;
const DWORD register_object_change_fid_ = 0x41518C;
const DWORD register_object_funset_ = 0x4150A8;
const DWORD register_object_light_ = 0x415334;
const DWORD register_object_must_erase_ = 0x414E20;
const DWORD register_object_take_out_ = 0x415238;
const DWORD register_object_turn_towards_ = 0x414C50;
const DWORD report_explosion_ = 0x413144;
const DWORD RestorePlayer_ = 0x43A8BC;
const DWORD roll_random_ = 0x4A30C0;
const DWORD runProgram_ = 0x46E154;
const DWORD SaveGame_ = 0x47B88C;
const DWORD SavePlayer_ = 0x43A7DC;
const DWORD scr_exec_map_update_scripts_ = 0x4A67E4;
const DWORD scr_find_first_at_ = 0x4A6524;
const DWORD scr_find_next_at_ = 0x4A6564;
const DWORD scr_find_obj_from_program_ = 0x4A39AC;
const DWORD scr_find_sid_from_program_ = 0x4A390C;
const DWORD scr_get_local_var_ = 0x4A6D64;
const DWORD scr_new_ = 0x4A5F28;
const DWORD scr_ptr_ = 0x4A5E34;
const DWORD scr_remove_ = 0x4A61D4;
const DWORD scr_set_ext_param_ = 0x4A3B34;
const DWORD scr_set_local_var_ = 0x4A6E58;
const DWORD scr_set_objs_ = 0x4A3B0C;
const DWORD scr_write_ScriptNode_ = 0x4A5704;
const DWORD set_game_time_ = 0x4A347C;
const DWORD setup_move_timer_win_ = 0x476AB8;
const DWORD SexWindow_ = 0x437664;
const DWORD skill_check_stealing_ = 0x4ABBE4;
const DWORD skill_dec_point_ = 0x4AA8C4;
const DWORD skill_get_tags_ = 0x4AA508;
const DWORD skill_inc_point_ = 0x4AA6BC;
const DWORD skill_is_tagged_ = 0x4AA52C;
const DWORD skill_level_ = 0x4AA558;
const DWORD skill_points_ = 0x4AA680;
const DWORD skill_set_tags_ = 0x4AA4E4;
const DWORD skill_use_ = 0x4AAD08;
const DWORD skilldex_select_ = 0x4ABFD0;
const DWORD soundDelete_ = 0x4AD8DC;
const DWORD sprintf_ = 0x4F0041;
const DWORD square_num_ = 0x4B1F04;
const DWORD stat_get_base_direct_ = 0x4AF408;
const DWORD stat_get_bonus_ = 0x4AF474;
const DWORD stat_level_ = 0x4AEF48; // &GetCurrentStat(void* critter, int statID)
const DWORD stat_pc_add_experience_ = 0x4AFAA8;
const DWORD stat_pc_get_ = 0x4AF8FC;
const DWORD stat_pc_set_ = 0x4AF910;
const DWORD stat_set_bonus_ = 0x4AF63C;
const DWORD stat_set_defaults_ = 0x4AF6CC;
const DWORD stricmp_ = 0x4DECE6;
const DWORD strncpy_ = 0x4F014F;
const DWORD strParseStrFromList_ = 0x4AFE08;
const DWORD switch_hand_ = 0x4714E0;
const DWORD talk_to_critter_reacts_ = 0x447CA0;
const DWORD talk_to_translucent_trans_buf_to_buf_ = 0x44AC68;
const DWORD text_font_ = 0x4D58DC;
const DWORD text_object_create_ = 0x4B036C;
const DWORD tile_coord_ = 0x4B1674;
const DWORD tile_num_ = 0x4B1754;
const DWORD tile_refresh_display_ = 0x4B12D8;
const DWORD tile_refresh_rect_ = 0x4B12C0;
const DWORD tile_scroll_to_ = 0x4B3924;
const DWORD trait_get_ = 0x4B3B54;
const DWORD trait_init_ = 0x4B39F0;
const DWORD trait_level_ = 0x4B3BC8;
const DWORD trait_set_ = 0x4B3B48;
const DWORD use_inventory_on_ = 0x4717E4;
const DWORD _word_wrap_ = 0x4BC6F0;
const DWORD win_add_ = 0x4D6238;
const DWORD win_delete_ = 0x4D6468;
const DWORD win_disable_button_ = 0x4D94D0;
const DWORD win_draw_ = 0x4D6F5C;
const DWORD win_draw_rect_ = 0x4D6F80;
const DWORD win_enable_button_ = 0x4D9474;
const DWORD win_get_buf_ = 0x4D78B0;
const DWORD win_hide_ = 0x4D6E64;
const DWORD win_line_ = 0x4D6B24;
const DWORD win_print_ = 0x4D684C;
const DWORD win_register_button_ = 0x4D8260;
const DWORD win_register_button_disable_ = 0x4D8674;
const DWORD win_register_button_sound_func_ = 0x4D87F8;
const DWORD win_show_ = 0x4D6DAC;
const DWORD wmFindCurSubTileFromPos_ = 0x4C0C00;
const DWORD wmInterfaceScrollTabsStart_ = 0x4C219C;
const DWORD wmMapIsSaveable_ = 0x4BFA64;
const DWORD wmMarkSubTileRadiusVisited_ = 0x4C3550;
const DWORD wmPartyInitWalking_ = 0x4C1E54;
const DWORD wmPartyWalkingStep_ = 0x4C1F90;
const DWORD wmSubTileMarkRadiusVisited_ = 0x4C35A8;
const DWORD wmWorldMapFunc_ = 0x4BFE10;
const DWORD wmWorldMapLoadTempData_ = 0x4BD6B4;
const DWORD xfclose_ = 0x4DED6C;
const DWORD xfeof_ = 0x4DF780;
const DWORD xfgetc_ = 0x4DF22C;
const DWORD xfgets_ = 0x4DF280;
const DWORD xfilelength_ = 0x4DF828;
const DWORD xfopen_ = 0x4DEE2C;
const DWORD xfputc_ = 0x4DF320;
const DWORD xfputs_ = 0x4DF380;
const DWORD xfread_ = 0x4DF44C;
const DWORD xfseek_ = 0x4DF5D8;
const DWORD xftell_ = 0x4DF690;
const DWORD xfwrite_ = 0x4DF4E8;
const DWORD xremovepath_ = 0x4DFAB4;
const DWORD xrewind_ = 0x4DF6E4;
const DWORD xungetc_ = 0x4DF3F4;
const DWORD xvfprintf_ = 0x4DF1AC;


// WRAPPERS
// please, use CamelCase for those


long __stdcall ItemGetType(TGameObj* item) {
	__asm {
		mov  eax, item;
		call item_get_type_;
	}
}

long __stdcall ItemSize(TGameObj* item) {
	__asm {
		mov  eax, item;
		call item_size_;
	}
}

long __stdcall IsPartyMember(TGameObj* obj) {
	__asm {
		mov  eax, obj;
		call isPartyMember_;
	}
}

long __stdcall PartyMemberGetCurrentLevel(TGameObj* obj) {
	__asm {
		mov  eax, obj;
		call partyMemberGetCurLevel_;
	}
}

char* GetProtoPtr(long pid) {
	char* proto;
	long result;
	__asm {
		mov  eax, pid;
		lea  edx, proto;
		call proto_ptr_;
		mov  result, eax;
	}
	if (result != -1) {
		return proto;
	}
	return nullptr;
}

char AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		char* proto = GetProtoPtr(weapon->pid);
		if (proto != nullptr && *(int*)(proto + 32) == item_type_weapon) {
			return (char)(*(int*)(proto + 36));
		}
	}
	return 0;
}

void DisplayConsoleMessage(const char* msg) {
	__asm {
		mov  eax, msg;
		call display_print_;
	}
}

static DWORD mesg_buf[4] = {0, 0, 0, 0};
const char* __stdcall GetMessageStr(DWORD fileAddr, DWORD messageId) {
	DWORD buf = (DWORD)mesg_buf;
	const char* result;
	__asm {
		mov  eax, fileAddr;
		mov  ebx, messageId;
		mov  edx, buf;
		call getmsg_;
		mov result, eax;
	}
	return result;
}

// Change the name of playable character
void CritterPcSetName(const char* newName) {
	__asm {
		mov  eax, newName;
		call critter_pc_set_name_;
	}
}

// Returns the name of the critter
const char* __stdcall CritterName(TGameObj* critter) {
	__asm {
		mov  eax, critter;
		call critter_name_;
	}
}

void SkillGetTags(int* result, DWORD num) {
	if (num > 4) {
		num = 4;
	}
	__asm {
		mov  eax, result;
		mov  edx, num;
		call skill_get_tags_;
	}
}

void SkillSetTags(int* tags, DWORD num) {
	if (num > 4) {
		num = 4;
	}
	__asm {
		mov  eax, tags;
		mov  edx, num;
		call skill_set_tags_;
	}
}

// Saves pointer to script object into scriptPtr using scriptID.
// Returns 0 on success, -1 on failure.
long __stdcall ScrPtr(long scriptId, TScript** scriptPtr) {
	__asm {
		mov  eax, scriptId;
		mov  edx, scriptPtr;
		call scr_ptr_;
	}
}

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid) {
	TScript* script = nullptr;
	ScrPtr(sid, &script);
	return (script) ? script->num_local_vars : 0;
}

long __fastcall ScrGetLocalVar(long sid, long varId, long* value) {
	__asm {
		mov  ebx, value;
		mov  eax, ecx;
		call scr_get_local_var_;
	}
}

long __fastcall ScrSetLocalVar(long sid, long varId, long value) {
	__asm {
		mov  ebx, value;
		mov  eax, ecx;
		call scr_set_local_var_;
	}
}

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void InterfaceRedraw() {
	__asm call intface_redraw_;
}

void __stdcall ProcessBk() {
	__asm call process_bk_;
}

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall InterpretPopShort(TProgram* scriptPtr) {
	__asm {
		mov  eax, scriptPtr;
		call interpretPopShort_;
	}
}

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall InterpretPopLong(TProgram* scriptPtr) {
	__asm {
		mov  eax, scriptPtr;
		call interpretPopLong_;
	}
}

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall InterpretPushLong(TProgram* scriptPtr, DWORD val) {
	__asm {
		mov  edx, val;
		mov  eax, scriptPtr;
		call interpretPushLong_;
	}
}

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall InterpretPushShort(TProgram* scriptPtr, DWORD valType) {
	__asm {
		mov  edx, valType;
		mov  eax, scriptPtr;
		call interpretPushShort_;
	}
}

DWORD __stdcall InterpretAddString(TProgram* scriptPtr, const char* strval) {
	__asm {
		mov  edx, strval;
		mov  eax, scriptPtr;
		call interpretAddString_;
	}
}

const char* __stdcall InterpretGetString(TProgram* scriptPtr, DWORD strId, DWORD dataType) {
	__asm {
		mov  edx, dataType;
		mov  ebx, strId;
		mov  eax, scriptPtr;
		call interpretGetString_;
	}
}

void __declspec(naked) InterpretError(const char* fmt, ...) {
	__asm {
		jmp interpretError_;
	}
}

void __declspec(naked) DebugPrintf(const char* fmt, ...) {
	__asm {
		jmp debug_printf_;
	}
}

const char* __stdcall FindCurrentProc(TProgram* program) {
	__asm {
		mov  eax, program;
		call findCurrentProc_;
	}
}

TGameObj* __stdcall InvenWorn(TGameObj* critter) {
	__asm {
		mov  eax, critter;
		call inven_worn_;
	}
}

__declspec(noinline) TGameObj* __stdcall GetItemPtrSlot(TGameObj* critter, long slot) {
	TGameObj* itemPtr = nullptr;
	switch (slot) {
		case INVEN_TYPE_LEFT_HAND:
			itemPtr = InvenLeftHand(critter);
			break;
		case INVEN_TYPE_RIGHT_HAND:
			itemPtr = InvenRightHand(critter);
			break;
		case INVEN_TYPE_WORN:
			itemPtr = InvenWorn(critter);
			break;
	}
	return itemPtr;
}

TGameObj* __stdcall InvenLeftHand(TGameObj* critter) {
	__asm {
		mov  eax, critter;
		call inven_left_hand_;
	}
}

TGameObj* __stdcall InvenRightHand(TGameObj* critter) {
	__asm {
		mov  eax, critter;
		call inven_right_hand_;
	}
}

long __fastcall CreateWindowFunc(const char* winName, long x, long y, long width, long height, long bgColorIndex, long flags) {
	__asm {
		push flags;
		push bgColorIndex;
		push height;
		mov  eax, ecx;
		mov  ebx, y;
		mov  ecx, width;
		call createWindow_;
	}
}

long __stdcall WinRegisterButton(DWORD winRef, long xPos, long yPos, long width, long height, long hoverOn, long hoverOff, long buttonDown, long buttonUp, BYTE* pictureUp, BYTE* pictureDown, long arg12, long buttonType) {
	__asm {
		push buttonType;
		push arg12;
		push pictureDown;
		push pictureUp;
		push buttonUp;
		push buttonDown;
		push hoverOff;
		push hoverOn;
		push height;
		mov  ecx, width;
		mov  ebx, yPos;
		mov  edx, xPos;
		mov  eax, winRef;
		call win_register_button_;
	}
}

void __stdcall DialogOut(const char* text) {
	__asm {
		push 1;          // flag
		xor  eax, eax;
		push eax;        // ColorMsg
		push eax;        // DisplayMsg
		mov  al, byte ptr ds:[0x6AB718];
		push eax;        // ColorIndex
		push 116;        // y
		mov  ecx, 192;   // x
		mov  eax, text;  // DisplayText
		xor  ebx, ebx;   // ?
		xor  edx, edx;   // ?
		call dialog_out_;
	}
}

long __fastcall WordWrap(const char* text, int maxWidth, DWORD* buf, BYTE* count) {
	__asm {
		mov  eax, ecx;
		mov  ebx, buf;
		mov  ecx, count;
		call _word_wrap_;
	}
}

void __stdcall RedrawWin(DWORD winRef) {
	__asm {
		mov  eax, winRef;
		call win_draw_;
	}
}

void __fastcall DisplayInventory(long inventoryOffset, long visibleOffset, long mode) {
	__asm {
		mov  ebx, mode;
		mov  eax, ecx;
		call display_inventory_;
	}
}

void __fastcall DisplayTargetInventory(long inventoryOffset, long visibleOffset, DWORD* targetInventory, long mode) {
	__asm {
		mov  eax, ecx;
		mov  ebx, targetInventory;
		mov  ecx, mode;
		call display_target_inventory_;
	}
}

long __stdcall StatLevel(TGameObj* critter, long statId) {
	__asm {
		mov  edx, statId;
		mov  eax, critter;
		call stat_level_;
	}
}

long __stdcall PerkLevel(TGameObj* critter, long perkId) {
	__asm {
		mov  edx, perkId;
		mov  eax, critter;
		call perk_level_;
	}
}

long __stdcall TraitLevel(long traitID) {
	__asm {
		mov  eax, traitID;
		call trait_level_;
	}
}

long __stdcall QueueFindFirst(TGameObj* object, long qType) {
	__asm {
		mov  edx, qType;
		mov  eax, object;
		call queue_find_first_;
	}
}

TGameObj* __stdcall ObjFindFirst() {
	__asm call obj_find_first_;
}

TGameObj* __stdcall ObjFindNext() {
	__asm call obj_find_next_;
}

long __stdcall NewObjId() {
	__asm call new_obj_id_;
}

FrmFrameData* __fastcall FramePtr(FrmHeaderData* frm, long frame, long direction) {
	__asm {
		mov  ebx, direction;
		mov  eax, ecx;
		call frame_ptr_;
	}
}

void __stdcall MapDirErase(const char* folder, const char* ext) {
	__asm {
		mov  edx, ext;
		mov  eax, folder;
		call MapDirErase_;
	}
}

long __stdcall ItemWAnimWeap(TGameObj* item, DWORD hitMode) {
	__asm {
		mov  edx, hitMode;
		mov  eax, item;
		call item_w_anim_weap_;
	}
}

long __stdcall ItemWComputeAmmoCost(TGameObj* item, DWORD* rounds) {
	__asm {
		mov  edx, rounds;
		mov  eax, item;
		call item_w_compute_ammo_cost_;
	}
}

long __stdcall ItemWCurrAmmo(TGameObj* item) {
	__asm {
		mov  eax, item;
		call item_w_curr_ammo_;
	}
}

long __stdcall ItemWRounds(TGameObj* item) {
	__asm {
		mov  eax, item;
		call item_w_rounds_;
	}
}

long __stdcall BarterComputeValue(TGameObj* source, TGameObj* target) {
	__asm {
		mov  edx, target;
		mov  eax, source;
		call barter_compute_value_;
	}
}

long __stdcall ItemCapsTotal(TGameObj* object) {
	__asm {
		mov  eax, object;
		call item_caps_total_;
	}
}

long __stdcall ItemTotalCost(TGameObj* object) {
	__asm {
		mov  eax, object;
		call item_total_cost_;
	}
}
