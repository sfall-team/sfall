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

#include "FalloutEngine.h"
#include "Logging.h"

// global variables
long* ptr_pc_traits                     = reinterpret_cast<long*>(_pc_trait); // 2 of them

DWORD* ptr_aiInfoList                   = reinterpret_cast<DWORD*>(_aiInfoList);
DWORD* ptr_ambient_light                = reinterpret_cast<DWORD*>(_ambient_light);
sArt*  ptr_art                          = reinterpret_cast<sArt*>(_art); // array of 11 sArt
DWORD* ptr_art_name                     = reinterpret_cast<DWORD*>(_art_name);
DWORD* ptr_art_vault_guy_num            = reinterpret_cast<DWORD*>(_art_vault_guy_num);
DWORD* ptr_art_vault_person_nums        = reinterpret_cast<DWORD*>(_art_vault_person_nums);
DWORD* ptr_background_volume            = reinterpret_cast<DWORD*>(_background_volume);
BYTE** ptr_bckgnd                       = reinterpret_cast<BYTE**>(_bckgnd);
DWORD* ptr_black_palette                = reinterpret_cast<DWORD*>(_black_palette);
BYTE*  ptr_BlueColor                    = reinterpret_cast<BYTE*>(_BlueColor);
DWORD* ptr_bottom_line                  = reinterpret_cast<DWORD*>(_bottom_line);
DWORD* ptr_btable                       = reinterpret_cast<DWORD*>(_btable);
DWORD* ptr_btncnt                       = reinterpret_cast<DWORD*>(_btncnt);
DWORD* ptr_CarCurrArea                  = reinterpret_cast<DWORD*>(_CarCurrArea);
DWORD* ptr_cmap                         = reinterpret_cast<DWORD*>(_cmap);
DWORD* ptr_colorTable                   = reinterpret_cast<DWORD*>(_colorTable);
DWORD* ptr_combat_free_move             = reinterpret_cast<DWORD*>(_combat_free_move);
DWORD* ptr_combat_list                  = reinterpret_cast<DWORD*>(_combat_list);
DWORD* ptr_combat_state                 = reinterpret_cast<DWORD*>(_combat_state);
DWORD* ptr_combat_turn_running          = reinterpret_cast<DWORD*>(_combat_turn_running);
DWORD* ptr_combatNumTurns               = reinterpret_cast<DWORD*>(_combatNumTurns);
DWORD* ptr_crit_succ_eff                = reinterpret_cast<DWORD*>(_crit_succ_eff);
PathNode** ptr_critter_db_handle        = reinterpret_cast<PathNode**>(_critter_db_handle);
DWORD* ptr_critterClearObj              = reinterpret_cast<DWORD*>(_critterClearObj);
DWORD* ptr_crnt_func                    = reinterpret_cast<DWORD*>(_crnt_func);
DWORD* ptr_curr_font_num                = reinterpret_cast<DWORD*>(_curr_font_num);
DWORD* ptr_curr_pc_stat                 = reinterpret_cast<DWORD*>(_curr_pc_stat);
DWORD* ptr_curr_stack                   = reinterpret_cast<DWORD*>(_curr_stack);
TProgram** ptr_currentProgram           = reinterpret_cast<TProgram**>(_currentProgram);
DWORD* ptr_cursor_line                  = reinterpret_cast<DWORD*>(_cursor_line);
BYTE*  ptr_DARK_GREY_Color              = reinterpret_cast<BYTE*>(_DARK_GREY_Color);
BYTE*  ptr_DarkGreenColor               = reinterpret_cast<BYTE*>(_DarkGreenColor);
BYTE*  ptr_DarkGreenGreyColor           = reinterpret_cast<BYTE*>(_DarkGreenGreyColor);
TGameObj** ptr_dialog_target            = reinterpret_cast<TGameObj**>(_dialog_target);
DWORD* ptr_dialog_target_is_party       = reinterpret_cast<DWORD*>(_dialog_target_is_party);
const DWORD* ptr_dialogueBackWindow     = reinterpret_cast<DWORD*>(_dialogueBackWindow);
DWORD* ptr_drugInfoList                 = reinterpret_cast<DWORD*>(_drugInfoList);
BYTE*  ptr_DullPinkColor                = reinterpret_cast<BYTE*>(_DullPinkColor);
const DWORD* ptr_edit_win               = reinterpret_cast<DWORD*>(_edit_win);
DWORD* ptr_Educated                     = reinterpret_cast<DWORD*>(_Educated);
DWORD* ptr_elevation                    = reinterpret_cast<DWORD*>(_elevation);
DWORD* ptr_endgame_subtitle_done        = reinterpret_cast<DWORD*>(_endgame_subtitle_done);
DWORD* ptr_Experience_                  = reinterpret_cast<DWORD*>(_Experience_);
DWORD* ptr_fallout_game_time            = reinterpret_cast<DWORD*>(_fallout_game_time);
DWORD* ptr_fidgetFID                    = reinterpret_cast<DWORD*>(_fidgetFID);
DWORD* ptr_flptr                        = reinterpret_cast<DWORD*>(_flptr);
DWORD* ptr_folder_card_desc             = reinterpret_cast<DWORD*>(_folder_card_desc);
DWORD* ptr_folder_card_fid              = reinterpret_cast<DWORD*>(_folder_card_fid);
DWORD* ptr_folder_card_title            = reinterpret_cast<DWORD*>(_folder_card_title);
DWORD* ptr_folder_card_title2           = reinterpret_cast<DWORD*>(_folder_card_title2);
DWORD* ptr_frame_time                   = reinterpret_cast<DWORD*>(_frame_time);
char*  ptr_free_perk                    = reinterpret_cast<char*>(_free_perk);
long** ptr_game_global_vars             = reinterpret_cast<long**>(_game_global_vars); // dynamic array of size == num_game_global_vars
DWORD* ptr_game_user_wants_to_quit      = reinterpret_cast<DWORD*>(_game_user_wants_to_quit);
CombatGcsd** ptr_gcsd                   = reinterpret_cast<CombatGcsd**>(_gcsd);
DWORD* ptr_gdBarterMod                  = reinterpret_cast<DWORD*>(_gdBarterMod);
DWORD* ptr_gdNumOptions                 = reinterpret_cast<DWORD*>(_gdNumOptions);
DWORD* ptr_gIsSteal                     = reinterpret_cast<DWORD*>(_gIsSteal);
DWORD* ptr_glblmode                     = reinterpret_cast<DWORD*>(_glblmode);
long*  ptr_gmouse_current_cursor        = reinterpret_cast<long*>(_gmouse_current_cursor);
DWORD* ptr_gmovie_played_list           = reinterpret_cast<DWORD*>(_gmovie_played_list);
BYTE*  ptr_GoodColor                    = reinterpret_cast<BYTE*>(_GoodColor);
BYTE*  ptr_GreenColor                   = reinterpret_cast<BYTE*>(_GreenColor);
DWORD* ptr_gsound_initialized           = reinterpret_cast<DWORD*>(_gsound_initialized);
long*  ptr_hit_location_penalty         = reinterpret_cast<long*>(_hit_location_penalty);
DWORD* ptr_holo_flag                    = reinterpret_cast<DWORD*>(_holo_flag);
DWORD* ptr_holopages                    = reinterpret_cast<DWORD*>(_holopages);
DWORD* ptr_hot_line_count               = reinterpret_cast<DWORD*>(_hot_line_count);
DWORD* ptr_i_fid                        = reinterpret_cast<DWORD*>(_i_fid);
TGameObj** ptr_i_lhand                  = reinterpret_cast<TGameObj**>(_i_lhand);
TGameObj** ptr_i_rhand                  = reinterpret_cast<TGameObj**>(_i_rhand);
const DWORD* ptr_i_wid                  = reinterpret_cast<DWORD*>(_i_wid);
TGameObj** ptr_i_worn                   = reinterpret_cast<TGameObj**>(_i_worn);
void** ptr_idle_func                    = reinterpret_cast<void**>(_idle_func);
DWORD* ptr_In_WorldMap                  = reinterpret_cast<DWORD*>(_In_WorldMap); // moving in WorldMap
DWORD* ptr_info_line                    = reinterpret_cast<DWORD*>(_info_line);
const DWORD* ptr_interfaceWindow        = reinterpret_cast<DWORD*>(_interfaceWindow);
DWORD* ptr_intfaceEnabled               = reinterpret_cast<DWORD*>(_intfaceEnabled);
DWORD* ptr_intotal                      = reinterpret_cast<DWORD*>(_intotal);
TGameObj** ptr_inven_dude               = reinterpret_cast<TGameObj**>(_inven_dude);
DWORD* ptr_inven_pid                    = reinterpret_cast<DWORD*>(_inven_pid);
DWORD* ptr_inven_scroll_dn_bid          = reinterpret_cast<DWORD*>(_inven_scroll_dn_bid);
DWORD* ptr_inven_scroll_up_bid          = reinterpret_cast<DWORD*>(_inven_scroll_up_bid);
MSGList* ptr_inventry_message_file      = reinterpret_cast<MSGList*>(_inventry_message_file);
ItemButtonItem* ptr_itemButtonItems     = reinterpret_cast<ItemButtonItem*>(_itemButtonItems); // array of 2 ItemButtonItem
long*  ptr_itemCurrentItem              = reinterpret_cast<long*>(_itemCurrentItem); // 0 - left, 1 - right
DWORD* ptr_kb_lock_flags                = reinterpret_cast<DWORD*>(_kb_lock_flags);
DWORD* ptr_last_buttons                 = reinterpret_cast<DWORD*>(_last_buttons);
DWORD* ptr_last_button_winID            = reinterpret_cast<DWORD*>(_last_button_winID);
DWORD* ptr_last_level                   = reinterpret_cast<DWORD*>(_last_level);
DWORD* ptr_Level_                       = reinterpret_cast<DWORD*>(_Level_);
DWORD* ptr_Lifegiver                    = reinterpret_cast<DWORD*>(_Lifegiver);
BYTE*  ptr_LIGHT_GREY_Color             = reinterpret_cast<BYTE*>(_LIGHT_GREY_Color);
DWORD* ptr_lipsFID                      = reinterpret_cast<DWORD*>(_lipsFID);
DWORD* ptr_list_com                     = reinterpret_cast<DWORD*>(_list_com);
DWORD* ptr_list_total                   = reinterpret_cast<DWORD*>(_list_total);
DWORD* ptr_loadingGame                  = reinterpret_cast<DWORD*>(_loadingGame);
DWORD* ptr_LSData                       = reinterpret_cast<DWORD*>(_LSData);
DWORD* ptr_lsgwin                       = reinterpret_cast<DWORD*>(_lsgwin);
TComputeAttack* ptr_main_ctd            = reinterpret_cast<TComputeAttack*>(_main_ctd);
DWORD* ptr_main_death_voiceover_done    = reinterpret_cast<DWORD*>(_main_death_voiceover_done);
DWORD* ptr_main_window                  = reinterpret_cast<DWORD*>(_main_window);
DWORD* ptr_map_elevation                = reinterpret_cast<DWORD*>(_map_elevation);
long** ptr_map_global_vars              = reinterpret_cast<long**>(_map_global_vars); // array
DWORD* ptr_map_number                   = reinterpret_cast<DWORD*>(_map_number);
PathNode** ptr_master_db_handle         = reinterpret_cast<PathNode**>(_master_db_handle);
DWORD* ptr_master_volume                = reinterpret_cast<DWORD*>(_master_volume);
DWORD* ptr_max                          = reinterpret_cast<DWORD*>(_max);
long*  ptr_maxScriptNum                 = reinterpret_cast<long*>(_maxScriptNum);
bool*  ptr_Meet_Frank_Horrigan          = reinterpret_cast<bool*>(_Meet_Frank_Horrigan);
const char** ptr_movie_list             = reinterpret_cast<const char**>(_movie_list); // array of 17 char*
DWORD* ptr_mouse_hotx                   = reinterpret_cast<DWORD*>(_mouse_hotx);
DWORD* ptr_mouse_hoty                   = reinterpret_cast<DWORD*>(_mouse_hoty);
DWORD* ptr_mouse_is_hidden              = reinterpret_cast<DWORD*>(_mouse_is_hidden);
DWORD* ptr_mouse_x_                     = reinterpret_cast<DWORD*>(_mouse_x_);
DWORD* ptr_mouse_y                      = reinterpret_cast<DWORD*>(_mouse_y);
DWORD* ptr_mouse_y_                     = reinterpret_cast<DWORD*>(_mouse_y_);
DWORD* ptr_Mutate_                      = reinterpret_cast<DWORD*>(_Mutate_);
DWORD* ptr_name_color                   = reinterpret_cast<DWORD*>(_name_color);
DWORD* ptr_name_font                    = reinterpret_cast<DWORD*>(_name_font);
DWORD* ptr_name_sort_list               = reinterpret_cast<DWORD*>(_name_sort_list);
DWORD* ptr_num_game_global_vars         = reinterpret_cast<DWORD*>(_num_game_global_vars);
DWORD* ptr_num_map_global_vars          = reinterpret_cast<DWORD*>(_num_map_global_vars);
DWORD* ptr_num_windows                  = reinterpret_cast<DWORD*>(_num_windows);
TGameObj** ptr_obj_dude                 = reinterpret_cast<TGameObj**>(_obj_dude);
DWORD* ptr_objectTable                  = reinterpret_cast<DWORD*>(_objectTable);
DWORD* ptr_objItemOutlineState          = reinterpret_cast<DWORD*>(_objItemOutlineState);
DWORD* ptr_optionRect                   = reinterpret_cast<DWORD*>(_optionRect);
DWORD* ptr_optionsButtonDown            = reinterpret_cast<DWORD*>(_optionsButtonDown);
DWORD* ptr_optionsButtonDown1           = reinterpret_cast<DWORD*>(_optionsButtonDown1);
DWORD* ptr_optionsButtonDownKey         = reinterpret_cast<DWORD*>(_optionsButtonDownKey);
DWORD* ptr_optionsButtonUp              = reinterpret_cast<DWORD*>(_optionsButtonUp);
DWORD* ptr_optionsButtonUp1             = reinterpret_cast<DWORD*>(_optionsButtonUp1);
DWORD* ptr_optionsButtonUpKey           = reinterpret_cast<DWORD*>(_optionsButtonUpKey);
const DWORD* ptr_optnwin                = reinterpret_cast<DWORD*>(_optnwin);
DWORD* ptr_outlined_object              = reinterpret_cast<DWORD*>(_outlined_object);
DWORD* ptr_partyMemberAIOptions         = reinterpret_cast<DWORD*>(_partyMemberAIOptions);
DWORD* ptr_partyMemberCount             = reinterpret_cast<DWORD*>(_partyMemberCount);
DWORD** ptr_partyMemberLevelUpInfoList  = reinterpret_cast<DWORD**>(_partyMemberLevelUpInfoList);
DWORD** ptr_partyMemberList             = reinterpret_cast<DWORD**>(_partyMemberList); // each struct - 4 integers, first integer - objPtr
DWORD* ptr_partyMemberMaxCount          = reinterpret_cast<DWORD*>(_partyMemberMaxCount);
DWORD** ptr_partyMemberPidList          = reinterpret_cast<DWORD**>(_partyMemberPidList);
char** ptr_patches                      = reinterpret_cast<char**>(_patches);
PathNode** ptr_paths                    = reinterpret_cast<PathNode**>(_paths); // array
DWORD* ptr_pc_crit_succ_eff             = reinterpret_cast<DWORD*>(_pc_crit_succ_eff);
DWORD* ptr_pc_kill_counts               = reinterpret_cast<DWORD*>(_pc_kill_counts);
char*  ptr_pc_name                      = reinterpret_cast<char*>(_pc_name);
DWORD* ptr_pc_proto                     = reinterpret_cast<DWORD*>(_pc_proto);
BYTE*  ptr_PeanutButter                 = reinterpret_cast<BYTE*>(_PeanutButter);
DWORD* ptr_perk_data                    = reinterpret_cast<DWORD*>(_perk_data);
int**  ptr_perkLevelDataList            = reinterpret_cast<int**>(_perkLevelDataList);
const DWORD* ptr_pip_win                = reinterpret_cast<DWORD*>(_pip_win);
DWORD* ptr_pipboy_message_file          = reinterpret_cast<DWORD*>(_pipboy_message_file);
DWORD* ptr_pipmesg                      = reinterpret_cast<DWORD*>(_pipmesg);
DWORD* ptr_preload_list_index           = reinterpret_cast<DWORD*>(_preload_list_index);
const char** ptr_procTableStrs          = reinterpret_cast<const char**>(_procTableStrs);  // table of procId (from define.h) => procName map
MSGList* ptr_proto_main_msg_file        = reinterpret_cast<MSGList*>(_proto_main_msg_file);
MSGList* ptr_proto_msg_files            = reinterpret_cast<MSGList*>(_proto_msg_files); // array of 6 elements
DWORD* ptr_ptable                       = reinterpret_cast<DWORD*>(_ptable);
DWORD* ptr_pud                          = reinterpret_cast<DWORD*>(_pud);
DWORD* ptr_queue                        = reinterpret_cast<DWORD*>(_queue);
DWORD* ptr_quick_done                   = reinterpret_cast<DWORD*>(_quick_done);
DWORD* ptr_read_callback                = reinterpret_cast<DWORD*>(_read_callback);
BYTE*  ptr_RedColor                     = reinterpret_cast<BYTE*>(_RedColor);
DWORD* ptr_retvals                      = reinterpret_cast<DWORD*>(_retvals);
DWORD* ptr_rotation                     = reinterpret_cast<DWORD*>(_rotation);
DWORD* ptr_sampleRate                   = reinterpret_cast<DWORD*>(_sampleRate);
BoundRect* ptr_scr_size                 = reinterpret_cast<BoundRect*>(_scr_size);
ScriptListInfoItem** ptr_scriptListInfo = reinterpret_cast<ScriptListInfoItem**>(_scriptListInfo); // dynamic array
DWORD* ptr_skill_data                   = reinterpret_cast<DWORD*>(_skill_data);
const DWORD* ptr_skldxwin               = reinterpret_cast<DWORD*>(_skldxwin);
DWORD* ptr_slot_cursor                  = reinterpret_cast<DWORD*>(_slot_cursor);
DWORD* ptr_sndfx_volume                 = reinterpret_cast<DWORD*>(_sndfx_volume);
DWORD* ptr_sneak_working                = reinterpret_cast<DWORD*>(_sneak_working); // DWORD var
char** ptr_sound_music_path1            = reinterpret_cast<char**>(_sound_music_path1);
char** ptr_sound_music_path2            = reinterpret_cast<char**>(_sound_music_path2);
DWORD* ptr_speech_volume                = reinterpret_cast<DWORD*>(_speech_volume);
DWORD* ptr_square                       = reinterpret_cast<DWORD*>(_square);
DWORD* ptr_squares                      = reinterpret_cast<DWORD*>(_squares);
DWORD* ptr_stack                        = reinterpret_cast<DWORD*>(_stack);
DWORD* ptr_stack_offset                 = reinterpret_cast<DWORD*>(_stack_offset);
DWORD* ptr_stat_data                    = reinterpret_cast<DWORD*>(_stat_data);
DWORD* ptr_stat_flag                    = reinterpret_cast<DWORD*>(_stat_flag);
SubTitleList** ptr_subtitleList         = reinterpret_cast<SubTitleList**>(_subtitleList);
DWORD* ptr_sWindows                     = reinterpret_cast<DWORD*>(_sWindows); // total 16 sWindow struct
DWORD* ptr_Tag_                         = reinterpret_cast<DWORD*>(_Tag_);
DWORD* ptr_tag_skill                    = reinterpret_cast<DWORD*>(_tag_skill);
DWORD* ptr_target_curr_stack            = reinterpret_cast<DWORD*>(_target_curr_stack);
DWORD** ptr_target_pud                  = reinterpret_cast<DWORD**>(_target_pud);
DWORD* ptr_target_stack                 = reinterpret_cast<DWORD*>(_target_stack); // array of 10 DWORD
DWORD* ptr_target_stack_offset          = reinterpret_cast<DWORD*>(_target_stack_offset); // array of 10 DWORD
DWORD* ptr_target_str                   = reinterpret_cast<DWORD*>(_target_str);
DWORD* ptr_target_xpos                  = reinterpret_cast<DWORD*>(_target_xpos);
DWORD* ptr_target_ypos                  = reinterpret_cast<DWORD*>(_target_ypos);
DWORD* ptr_text_char_width              = reinterpret_cast<DWORD*>(_text_char_width);
DWORD* ptr_text_height                  = reinterpret_cast<DWORD*>(_text_height);
DWORD* ptr_text_max                     = reinterpret_cast<DWORD*>(_text_max);
DWORD* ptr_text_mono_width              = reinterpret_cast<DWORD*>(_text_mono_width);
DWORD* ptr_text_object_index            = reinterpret_cast<DWORD*>(_text_object_index);
FloatText** ptr_text_object_list        = reinterpret_cast<FloatText**>(_text_object_list); // array of 20 FloatText*
DWORD* ptr_text_spacing                 = reinterpret_cast<DWORD*>(_text_spacing);
DWORD* ptr_text_to_buf                  = reinterpret_cast<DWORD*>(_text_to_buf);
DWORD* ptr_text_width                   = reinterpret_cast<DWORD*>(_text_width);
DWORD* ptr_tile                         = reinterpret_cast<DWORD*>(_tile);
DWORD* ptr_title_color                  = reinterpret_cast<DWORD*>(_title_color);
DWORD* ptr_title_font                   = reinterpret_cast<DWORD*>(_title_font);
DWORD* ptr_trait_data                   = reinterpret_cast<DWORD*>(_trait_data);
DWORD* ptr_view_page                    = reinterpret_cast<DWORD*>(_view_page);
DWORD* ptr_wd_obj                       = reinterpret_cast<DWORD*>(_wd_obj);
WINinfo** ptr_window                    = reinterpret_cast<WINinfo**>(_window); // array of 50 WINinfo*
BYTE*  ptr_WhiteColor                   = reinterpret_cast<BYTE*>(_WhiteColor);
DWORD* ptr_wmAreaInfoList               = reinterpret_cast<DWORD*>(_wmAreaInfoList);
const DWORD* ptr_wmBkWin                = reinterpret_cast<DWORD*>(_wmBkWin);
BYTE** ptr_wmBkWinBuf                   = reinterpret_cast<BYTE**>(_wmBkWinBuf);
DWORD* ptr_wmLastRndTime                = reinterpret_cast<DWORD*>(_wmLastRndTime);
MSGList* ptr_wmMsgFile                  = reinterpret_cast<MSGList*>(_wmMsgFile);
DWORD* ptr_wmNumHorizontalTiles         = reinterpret_cast<DWORD*>(_wmNumHorizontalTiles);
long*  ptr_wmWorldOffsetX               = reinterpret_cast<long*>(_wmWorldOffsetX);
long*  ptr_wmWorldOffsetY               = reinterpret_cast<long*>(_wmWorldOffsetY);
DWORD* ptr_world_xpos                   = reinterpret_cast<DWORD*>(_world_xpos);
DWORD* ptr_world_ypos                   = reinterpret_cast<DWORD*>(_world_ypos);
DWORD* ptr_WorldMapCurrArea             = reinterpret_cast<DWORD*>(_WorldMapCurrArea);
BYTE*  ptr_YellowColor                  = reinterpret_cast<BYTE*>(_YellowColor);


/**
	ENGINE FUNCTIONS OFFSETS
	const names should end with underscore
*/

// AI FUNCTIONS
const DWORD ai_can_use_weapon_ = 0x4298EC;  // (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0
const DWORD ai_cap_ = 0x4280B4;
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
const DWORD anim_can_use_door_ = 0x415E24;
const DWORD art_alias_fid_ = 0x4199D4;
const DWORD art_alias_num_ = 0x419998;
const DWORD art_exists_ = 0x4198C8; // eax - frameID, used for critter FIDs
const DWORD art_flush_ = 0x41927C;
const DWORD art_frame_data_ = 0x419870;
const DWORD art_frame_length_ = 0x4197B8;
const DWORD art_frame_width_ = 0x4197A0;
const DWORD art_get_code_ = 0x419314;
const DWORD art_get_name_ = 0x419428;
const DWORD art_id_ = 0x419C88;
const DWORD art_init_ = 0x418840;
const DWORD art_lock_ = 0x4191CC;
const DWORD art_ptr_lock_ = 0x419160;
const DWORD art_ptr_lock_data_ = 0x419188;
const DWORD art_ptr_unlock_ = 0x419260;
const DWORD attack_crit_success_ = 0x423EB4;
const DWORD audioCloseFile_ = 0x41A50C;
const DWORD audioFileSize_ = 0x41A78C;
const DWORD audioOpen_ = 0x41A2EC;
const DWORD audioRead_ = 0x41A574;
const DWORD audioSeek_ = 0x41A5E0;
const DWORD automap_ = 0x41B8BC;
const DWORD barter_compute_value_ = 0x474B2C;
const DWORD barter_inventory_ = 0x4757F0;
const DWORD block_for_tocks_ = 0x4C93B8;
const DWORD buf_to_buf_ = 0x4D36D4;
const DWORD cai_attempt_w_reload_ = 0x42AECC;
const DWORD caiHasWeapPrefType_ = 0x42938C;
const DWORD can_see_ = 0x412BEC;
const DWORD check_death_ = 0x410814;
const DWORD check_for_death_ = 0x424EE8;
const DWORD Check4Keys_ = 0x43F73C;
const DWORD combat_ = 0x422D2C;
const DWORD combat_ai_ = 0x42B130;
const DWORD combat_anim_finished_ = 0x425E80;
const DWORD combat_attack_ = 0x422F3C;
const DWORD combat_check_bad_shot_ = 0x426614;
const DWORD combat_ctd_init_ = 0x422EC4;
const DWORD combat_delete_critter_ = 0x426DDC;
const DWORD combat_input_ = 0x4227F4;
const DWORD combat_should_end_ = 0x422C60;
const DWORD combat_turn_ = 0x42299C;
const DWORD combat_turn_run_ = 0x4227DC;
const DWORD combatai_rating_ = 0x42B90C;
const DWORD compute_damage_ = 0x4247B8;
const DWORD compute_spray_ = 0x423488;
const DWORD config_get_string_ = 0x42BF48;
const DWORD config_get_value_ = 0x42C05C;
const DWORD config_set_value_ = 0x42C160;
const DWORD construct_box_bar_win_ = 0x461134;
const DWORD container_exit_ = 0x476394;
const DWORD correctFidForRemovedItem_ = 0x45409C;
const DWORD createWindow_ = 0x4B7F3C;
const DWORD credits_ = 0x42C860;
const DWORD credits_get_next_line_ = 0x42CE6C;
const DWORD critter_adjust_hits_ = 0x42D1A4;
const DWORD critter_body_type_ = 0x42DDC4;
const DWORD critter_can_obj_dude_rest_ = 0x42E564;
const DWORD critter_compute_ap_from_distance_ = 0x42E62C;
const DWORD critter_flag_check_ = 0x42E6AC;
const DWORD critter_get_hits_ = 0x42D18C;
const DWORD critter_get_rads_ = 0x42D38C;
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
const DWORD db_init_ = 0x4C5D30;
const DWORD db_read_to_buf_ = 0x4C5DD4;
const DWORD dbase_close_ = 0x4E5270;
const DWORD dbase_open_ = 0x4E4F58;
const DWORD debug_log_ = 0x4C7028;
const DWORD debug_printf_ = 0x4C6F48;
const DWORD debug_register_env_ = 0x4C6D90;
const DWORD determine_to_hit_ = 0x42436C;
const DWORD determine_to_hit_from_tile_ = 0x424394;
const DWORD determine_to_hit_func_ = 0x4243A8;
const DWORD determine_to_hit_no_range_ = 0x424380;
const DWORD dialog_out_ = 0x41CF20;
const DWORD displayInWindow_ = 0x4B8B10;
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
const DWORD dude_standup_ = 0x418574;
const DWORD editor_design_ = 0x431DF8;
const DWORD elapsed_time_ = 0x4C93E0;
const DWORD elevator_end_ = 0x43F6D0;
const DWORD elevator_start_ = 0x43F324;
const DWORD endgame_slideshow_ = 0x43F788;
const DWORD EndLoad_ = 0x47F4C8;
const DWORD EndPipboy_ = 0x497828;
const DWORD exec_script_proc_ = 0x4A4810;
const DWORD executeProcedure_ = 0x46DD2C;
const DWORD exit_inventory_ = 0x46FBD8;
const DWORD fadeSystemPalette_ = 0x4C7320;
const DWORD findCurrentProc_ = 0x467160;
const DWORD findVar_ = 0x4410AC;
const DWORD FMtext_char_width_ = 0x4421DC;
const DWORD FMtext_to_buf_ = 0x4422B4;
const DWORD FMtext_width_ = 0x442188;
const DWORD folder_print_line_ = 0x43E3D8;
const DWORD fprintf_ = 0x4F0D56;
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
const DWORD gdialogFreeSpeech_ = 0x4450C4;
const DWORD gdProcess_ = 0x4465C0;
const DWORD gdReviewExit_ = 0x445C18;
const DWORD gdReviewInit_ = 0x445938;
const DWORD GetSlotList_ = 0x47E5D0;
const DWORD get_input_ = 0x4C8B78;
const DWORD get_input_str2_ = 0x47F084;
const DWORD get_time_ = 0x4C9370;
const DWORD getmsg_ = 0x48504C; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory
const DWORD gmouse_3d_get_mode_ = 0x44CB6C;
const DWORD gmouse_3d_set_mode_ = 0x44CA18;
const DWORD gmouse_is_scrolling_ = 0x44B54C;
const DWORD gmouse_set_cursor_ = 0x44C840;
const DWORD gmovie_play_ = 0x44E690;
const DWORD gmovieIsPlaying_ = 0x44EB14;
const DWORD GNW_do_bk_process_ = 0x4C8D1C;
const DWORD GNW_find_ = 0x4D7888;
const DWORD GNW_win_refresh_ = 0x4D6FD8;
const DWORD GNW95_lost_focus_ = 0x4C9EEC;
const DWORD GNW95_process_message_ = 0x4C9CF0;
const DWORD gsnd_build_weapon_sfx_name_ = 0x451760;
const DWORD gsound_background_pause_ = 0x450B50;
const DWORD gsound_background_restart_last_ = 0x450B0C;
const DWORD gsound_background_stop_ = 0x450AB4;
const DWORD gsound_background_unpause_ = 0x450B64;
const DWORD gsound_background_volume_get_set_ = 0x450620;
const DWORD gsound_play_sfx_file_ = 0x4519A8;
const DWORD gsound_red_butt_press_ = 0x451970;
const DWORD gsound_red_butt_release_ = 0x451978;
const DWORD gsound_speech_length_get_ = 0x450C94;
const DWORD gsound_speech_play_ = 0x450CA0;
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
const DWORD inven_find_type_ = 0x472698;
const DWORD inven_left_hand_ = 0x471BBC;
const DWORD inven_pid_is_carried_ptr_ = 0x471CA0;
const DWORD inven_right_hand_ = 0x471B70;
const DWORD inven_set_mouse_ = 0x470BCC;
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
const DWORD item_hit_with_ = 0x477FF8;
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
const DWORD item_w_reload_ = 0x478918;
const DWORD item_w_rounds_ = 0x478D80;
const DWORD item_w_subtype_ = 0x478280;
const DWORD item_w_try_reload_ = 0x478768;
const DWORD item_w_unload_ = 0x478F80;
const DWORD item_weight_ = 0x477B88;
const DWORD kb_clear_ = 0x4CBDA8;
const DWORD light_get_tile_ = 0x47A980;
const DWORD ListDPerks_ = 0x43D0BC;
const DWORD ListDrvdStats_ = 0x43527C;
const DWORD ListHoloDiskTitles_ = 0x498C40;
const DWORD ListSkills_ = 0x436154;
const DWORD ListTraits_ = 0x43B8A8;
const DWORD loadColorTable_ = 0x4C78E4;
const DWORD LoadGame_ = 0x47C640;
const DWORD loadProgram_ = 0x4A3B74;
const DWORD LoadSlot_ = 0x47DC68;
const DWORD load_frame_ = 0x419EC0;
const DWORD loot_container_ = 0x473904;
const DWORD main_game_loop_ = 0x480E48;
const DWORD main_init_system_ = 0x480CC0;
const DWORD main_menu_hide_ = 0x481A00;
const DWORD main_menu_loop_ = 0x481AEC;
const DWORD make_path_func_ = 0x415EFC;
const DWORD make_straight_path_ = 0x4163AC;
const DWORD make_straight_path_func_ = 0x4163C8;
const DWORD map_disable_bk_processes_ = 0x482104;
const DWORD map_enable_bk_processes_ = 0x4820C0;
const DWORD map_get_short_name_ = 0x48261C;
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
const DWORD movieRun_ = 0x487AC8;
const DWORD movieStop_ = 0x487150;
const DWORD movieUpdate_ = 0x487BEC;
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
const DWORD obj_examine_ = 0x49AD78;
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
const DWORD obj_remove_from_inven_ = 0x49B73C;
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
const DWORD palette_fade_to_ = 0x493AD4;
const DWORD palette_init_ = 0x493A00;
const DWORD palette_set_to_ = 0x493B48;
const DWORD partyMemberCopyLevelInfo_ = 0x495EA8;
const DWORD partyMemberGetAIOptions_ = 0x4941F0;
const DWORD partyMemberGetCurLevel_ = 0x495FF0;
const DWORD partyMemberIncLevels_ = 0x495B60;
const DWORD partyMemberPrepItemSaveAll_ = 0x495140;
const DWORD partyMemberPrepLoad_ =  0x4947AC;
const DWORD partyMemberRemove_ = 0x4944DC;
const DWORD partyMemberSaveProtos_ = 0x495870;
const DWORD pause_for_tocks_ = 0x4C937C;
const DWORD pc_flag_off_ = 0x42E220;
const DWORD pc_flag_on_ = 0x42E26C;
const DWORD pc_flag_toggle_ = 0x42E2B0;
const DWORD perform_withdrawal_end_ = 0x47A558;
const DWORD perk_add_ = 0x496A5C;
const DWORD perk_add_effect_ = 0x496BFC;
const DWORD perk_add_force_ = 0x496A9C;
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
const DWORD queue_explode_exit_ = 0x4A2830;
const DWORD queue_find_ = 0x4A26A8;
const DWORD queue_find_first_ = 0x4A295C;
const DWORD queue_find_next_ = 0x4A2994;
const DWORD queue_leaving_map_ = 0x4A2920;
const DWORD queue_next_time_ = 0x4A2808;
const DWORD queue_remove_this_ = 0x4A264C;
const DWORD refresh_box_bar_win_ = 0x4614CC;
const DWORD register_begin_ = 0x413AF4;
const DWORD register_clear_ = 0x413C4C;
const DWORD register_end_ = 0x413CCC;
const DWORD register_object_animate_ = 0x4149D0;
const DWORD register_object_animate_and_hide_ = 0x414B7C;
const DWORD register_object_call_ = 0x414E98;
const DWORD register_object_change_fid_ = 0x41518C;
const DWORD register_object_funset_ = 0x4150A8;
const DWORD register_object_light_ = 0x415334;
const DWORD register_object_must_erase_ = 0x414E20;
const DWORD register_object_take_out_ = 0x415238;
const DWORD register_object_turn_towards_ = 0x414C50;
const DWORD remove_bk_process_ = 0x4C8DC4;
const DWORD report_explosion_ = 0x413144;
const DWORD reset_box_bar_win_ = 0x4614A0;
const DWORD RestorePlayer_ = 0x43A8BC;
const DWORD roll_random_ = 0x4A30C0;
const DWORD runProgram_ = 0x46E154;
const DWORD SaveGame_ = 0x47B88C;
const DWORD SavePlayer_ = 0x43A7DC;
const DWORD scr_exec_map_exit_scripts_ = 0x4A69A0;
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
const DWORD set_focus_func_ = 0x4C9438;
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
const DWORD soundGetPosition_ = 0x4AE634;
const DWORD soundPlay_ = 0x4AD73C;
const DWORD soundPlaying_ = 0x4ADA84;
const DWORD soundSetCallback_ = 0x4ADFF0;
const DWORD soundSetFileIO_ = 0x4AE2FC;
const DWORD soundVolume_ = 0x4ADE0C;
const DWORD sprintf_ = 0x4F0041;
const DWORD square_num_ = 0x4B1F04;
const DWORD stat_get_base_ = 0x4AF3E0;
const DWORD stat_get_base_direct_ = 0x4AF408;
const DWORD stat_get_bonus_ = 0x4AF474;
const DWORD stat_level_ = 0x4AEF48; // &GetCurrentStat(void* critter, int statID)
const DWORD stat_pc_add_experience_ = 0x4AFAA8;
const DWORD stat_pc_get_ = 0x4AF8FC;
const DWORD stat_pc_set_ = 0x4AF910;
const DWORD stat_recalc_derived_ = 0x4AF6FC;
const DWORD stat_set_bonus_ = 0x4AF63C;
const DWORD stat_set_defaults_ = 0x4AF6CC;
const DWORD stricmp_ = 0x4DECE6;
const DWORD strncpy_ = 0x4F014F;
const DWORD strParseStrFromList_ = 0x4AFE08;
const DWORD switch_hand_ = 0x4714E0;
const DWORD talk_to_critter_reacts_ = 0x447CA0;
const DWORD talk_to_translucent_trans_buf_to_buf_ = 0x44AC68;
const DWORD text_curr_ = 0x4D58D4;
const DWORD text_font_ = 0x4D58DC;
const DWORD text_object_create_ = 0x4B036C;
const DWORD tile_coord_ = 0x4B1674;
const DWORD tile_dir_ = 0x4B1ABC;
const DWORD tile_dist_ = 0x4B185C;
const DWORD tile_num_ = 0x4B1754;
const DWORD tile_num_in_direction_ = 0x4B1A6C;
const DWORD tile_refresh_display_ = 0x4B12D8;
const DWORD tile_refresh_rect_ = 0x4B12C0;
const DWORD tile_scroll_to_ = 0x4B3924;
const DWORD tile_set_center_ = 0x4B12F8;
const DWORD trait_get_ = 0x4B3B54;
const DWORD trait_init_ = 0x4B39F0;
const DWORD trait_level_ = 0x4B3BC8;
const DWORD trait_set_ = 0x4B3B48;
const DWORD trans_cscale_ = 0x4D3560;
const DWORD use_inventory_on_ = 0x4717E4;
const DWORD _word_wrap_ = 0x4BC6F0;
const DWORD win_add_ = 0x4D6238;
const DWORD win_delete_ = 0x4D6468;
const DWORD win_disable_button_ = 0x4D94D0;
const DWORD win_draw_ = 0x4D6F5C;
const DWORD win_draw_rect_ = 0x4D6F80;
const DWORD win_enable_button_ = 0x4D9474;
const DWORD win_fill_ = 0x4D6CC8;
const DWORD win_get_buf_ = 0x4D78B0;
const DWORD win_get_top_win_ = 0x4D78CC;
const DWORD win_height_ = 0x4D7934;
const DWORD win_hide_ = 0x4D6E64;
const DWORD win_line_ = 0x4D6B24;
const DWORD win_print_ = 0x4D684C;
const DWORD win_register_button_ = 0x4D8260;
const DWORD win_register_button_disable_ = 0x4D8674;
const DWORD win_register_button_sound_func_ = 0x4D87F8;
const DWORD win_show_ = 0x4D6DAC;
const DWORD win_width_ = 0x4D7918;
const DWORD windowDisplayBuf_ = 0x4B8EF0;
const DWORD windowDisplayTransBuf_ = 0x4B8F64;
const DWORD windowGetBuffer_ = 0x4B82DC;
const DWORD windowHide_ = 0x4B7610;
const DWORD windowShow_ = 0x4B7648;
const DWORD windowWidth_ = 0x4B7734;
const DWORD wmDrawCursorStopped_ = 0x4C41EC;
const DWORD wmFindCurSubTileFromPos_ = 0x4C0C00;
const DWORD wmInterfaceInit_ = 0x4C2324;
const DWORD wmInterfaceRefresh_ = 0x4C3830;
const DWORD wmInterfaceScrollTabsStart_ = 0x4C219C;
const DWORD wmMapIsSaveable_ = 0x4BFA64;
const DWORD wmMarkSubTileOffsetVisitedFunc_ = 0x4C3434;
const DWORD wmMarkSubTileRadiusVisited_ = 0x4C3550;
const DWORD wmMatchAreaContainingMapIdx_ = 0x4C59A4;
const DWORD wmPartyInitWalking_ = 0x4C1E54;
const DWORD wmPartyWalkingStep_ = 0x4C1F90;
const DWORD wmRefreshInterfaceOverlay_ = 0x4C50F4;
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

// Prints debug message to game debug.log file for develop build
#ifndef NDEBUG
void __declspec(naked) DevPrintf(const char* fmt, ...) {
	__asm jmp debug_printf_;
}
#else
void DevPrintf(...) {}
#endif

// Fallout2.exe was compiled using WATCOM compiler, which uses Watcom register calling convention.
// In this convention, up to 4 arguments are passed via registers in this order: EAX, EDX, EBX, ECX.

#define WRAP_WATCOM_CALL0(offs) \
	__asm call offs

#define WRAP_WATCOM_CALL1(offs, arg1) \
	__asm mov eax, arg1				\
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_CALL2(offs, arg1, arg2) \
	__asm mov edx, arg2				\
	WRAP_WATCOM_CALL1(offs, arg1)

#define WRAP_WATCOM_CALL3(offs, arg1, arg2, arg3) \
	__asm mov ebx, arg3				\
	WRAP_WATCOM_CALL2(offs, arg1, arg2)

#define WRAP_WATCOM_CALL4(offs, arg1, arg2, arg3, arg4) \
	__asm mov ecx, arg4				\
	WRAP_WATCOM_CALL3(offs, arg1, arg2, arg3)

#define WRAP_WATCOM_CALL5(offs, arg1, arg2, arg3, arg4, arg5) \
	__asm push arg5				\
	WRAP_WATCOM_CALL4(offs, arg1, arg2, arg3, arg4)

#define WRAP_WATCOM_CALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6) \
	__asm push arg6				\
	WRAP_WATCOM_CALL5(offs, arg1, arg2, arg3, arg4, arg5)

#define WRAP_WATCOM_CALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	__asm push arg7				\
	WRAP_WATCOM_CALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6)

// defines wrappers for __fastcall
#define WRAP_WATCOM_FCALL1(offs, arg1) \
	__asm mov eax, ecx				\
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_FCALL2(offs, arg1, arg2) \
	WRAP_WATCOM_FCALL1(offs, arg1)

#define WRAP_WATCOM_FCALL3(offs, arg1, arg2, arg3) \
	__asm mov ebx, arg3				\
	WRAP_WATCOM_FCALL1(offs, arg1)

#define WRAP_WATCOM_FCALL4(offs, arg1, arg2, arg3, arg4) \
	__asm mov eax, ecx				\
	__asm mov ebx, arg3				\
	__asm mov ecx, arg4				\
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_FCALL5(offs, arg1, arg2, arg3, arg4, arg5) \
	__asm push arg5				\
	WRAP_WATCOM_FCALL4(offs, arg1, arg2, arg3, arg4)

#define WRAP_WATCOM_FCALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6) \
	__asm push arg6				\
	WRAP_WATCOM_FCALL5(offs, arg1, arg2, arg3, arg4, arg5)

#define WRAP_WATCOM_FCALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	__asm push arg7				\
	WRAP_WATCOM_FCALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6)


// prints message to debug.log file
void __declspec(naked) DebugPrintf(const char* fmt, ...) {
	__asm jmp debug_printf_;
}

void __stdcall InterpretReturnValue(TProgram* scriptPtr, DWORD val, DWORD valType) {
	__asm {
		mov  esi, scriptPtr;
		mov  edx, val;
		cmp  valType, VAR_TYPE_STR;
		jne  isNotStr;
		mov  eax, esi;
		call interpretAddString_;
		mov  edx, eax;
isNotStr:
		mov  eax, esi;
		call interpretPushLong_;  // pushes value to Data stack (must be followed by InterpretPushShort)
		mov  edx, valType;
		mov  eax, esi;
		call interpretPushShort_; // pushes value type to Data stack (must be preceded by InterpretPushLong)
	}
}

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec(naked) InterpretError(const char* fmt, ...) {
	__asm jmp interpretError_;
}

long __fastcall TileNum(long x, long y) {
	__asm push ebx; // don't delete (bug in tile_num_)
	WRAP_WATCOM_FCALL2(tile_num_, x, y)
	__asm pop  ebx;
}

TGameObj* __fastcall obj_blocking_at_wrapper(TGameObj* obj, DWORD tile, DWORD elevation, void* func) {
	__asm {
		mov  eax, ecx;
		mov  ebx, elevation;
		call func;
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
		push 1;          // DIALOGOUT_NORMAL flag
		xor  edx, edx;
		push edx;
		push edx;
		mov  dl, byte ptr ds:[0x6AB718];
		push edx;        // ColorMsg
		mov  ecx, 192;   // x
		push 116;        // y
		mov  eax, text;  // DisplayText
		xor  ebx, ebx;
		call dialog_out_;
	}
}

long __fastcall DialogOutEx(const char* text, const char** textEx, long lines, long flags, long colors) {
	__asm {
		mov  ebx, colors; // Color index
		xor  eax, eax;
		push flags;
		test ebx, ebx;
		jnz  cColor;
		mov  al, byte ptr ds:[0x6AB718];
		mov  bl, al;
		jmp  skip;
cColor:
		mov  al, bh;
		and  ebx, 0xFF
skip:
		push eax;        // ColorMsg2
		push 0;          // DisplayMsg (unknown)
		mov  eax, ecx;   // DisplayText (first line)
		push ebx;        // ColorMsg1
		mov  ecx, 192;   // x
		push 116;        // y
		mov  ebx, lines; // count second lines
		call dialog_out_; // edx - DisplayText (seconds lines)
	}
}

void __fastcall DrawWinLine(int winRef, DWORD startXPos, DWORD endXPos, DWORD startYPos, DWORD endYPos, BYTE colour) {
	__asm {
		xor  eax, eax;
		mov  al, colour;
		push eax;
		push endYPos;
		mov  eax, ecx; // winRef
		mov  ecx, endXPos;
		mov  ebx, startYPos;
		//mov  edx, xStartPos;
		call win_line_;
	}
}

// draws an image to the buffer without scaling and with transparency display toggle
void __fastcall WindowDisplayBuf(long x, long width, long y, long height, void* data, long noTrans) {
	__asm {
		push height;
		push edx;       // from_width
		push y;
		mov  eax, data; // from
		mov  ebx, windowDisplayTransBuf_;
		cmp  noTrans, 0;
		cmovnz ebx, windowDisplayBuf_;
		call ebx; // *data<eax>, from_width<edx>, unused<ebx>, X<ecx>, Y, width, height
	}
}

// draws an image in the window and scales it to fit the window
void __fastcall DisplayInWindow(long w_here, long width, long height, void* data) {
	__asm {
		mov  ebx, height;
		mov  eax, data;
		call displayInWindow_; // *data<eax>, width<edx>, height<ebx>, where<ecx>
	}
}

void __fastcall TransCscale(long i_width, long i_height, long s_width, long s_height, long xy_shift, long w_width, void* data) {
	__asm {
		push w_width;
		push s_height;
		push s_width;
		mov  ebx, edx; // i_height
		mov  edx, ecx; // i_width
		call windowGetBuffer_;
		add  eax, xy_shift;
		push eax;      // to_buff
		mov  eax, data;
		call trans_cscale_; // *from_buff<eax>, i_width<edx>, i_height<ebx>, i_width2<ecx>, to_buff, width, height, to_width
	}
}

// buf_to_buf_ function with pure MMX implementation
void __cdecl BufToBuf(void* src, long width, long height, long src_width, void* dst, long dst_width) {
	if (height <= 0 || width <= 0) return;

	size_t blockCount = width / 64; // 64 bytes
	size_t remainder = width % 64;
	size_t remainderD = remainder / 4;
	size_t remainderB = remainder % 4;
	size_t s_pitch = src_width - width;
	size_t d_pitch = dst_width - width;

	__asm {
		mov  ebx, s_pitch;
		mov  edx, d_pitch;
		mov  esi, src;
		mov  edi, dst;
		mov  eax, height;
	startLoop:
		mov  ecx, blockCount;
		test ecx, ecx;
		jz   copySmall;
	copyBlock: // copies block of 64 bytes
		movq mm0, [esi];      // movups xmm0, [esi]; // SSE implementation
		movq mm1, [esi + 8];
		movq mm2, [esi + 16]; // movups xmm1, [esi + 16];
		movq mm3, [esi + 24];
		movq mm4, [esi + 32]; // movups xmm2, [esi + 32];
		movq mm5, [esi + 40];
		movq mm6, [esi + 48]; // movups xmm3, [esi + 48];
		movq mm7, [esi + 56];
		movq [edi], mm0;      // movups [edi], xmm0;
		movq [edi + 8], mm1;
		movq [edi + 16], mm2; // movups [edi + 16], xmm1;
		movq [edi + 24], mm3;
		movq [edi + 32], mm4; // movups xmm2, [esi + 32];
		movq [edi + 40], mm5;
		movq [edi + 48], mm6; // movups xmm3, [esi + 48];
		movq [edi + 56], mm7;
		add  esi, 64;
		lea  edi, [edi + 64];
		dec  ecx; // blockCount
		jnz  copyBlock;
		// copies the remaining bytes
		mov  ecx, remainderD;
		rep  movsd;
		mov  ecx, remainderB;
		rep  movsb;
		add  esi, ebx; // s_pitch
		add  edi, edx; // d_pitch
		dec  eax;      // height
		jnz  startLoop;
		emms;
		jmp  end;
	copySmall: // copies the small size data
		mov  ecx, remainderD;
		rep  movsd;
		mov  ecx, remainderB;
		rep  movsb;
		add  esi, ebx; // s_pitch
		add  edi, edx; // d_pitch
		dec  eax;      // height
		jnz  copySmall;
end:
	}
}

long __fastcall GetGameConfigString(const char* outValue, const char* section, const char* param) {
	__asm {
		mov  ebx, param;
		mov  eax, _game_config;
		call config_get_string_; // section<edx>, outValue<ecx>
	}
}

/* stdcall */
bool __stdcall ArtExists(long artFid) {
	WRAP_WATCOM_CALL1(art_exists_, artFid)
}

void __stdcall ArtFlush() {
	WRAP_WATCOM_CALL0(art_flush_)
}

const char* __stdcall ArtGetName(long artFID) {
	WRAP_WATCOM_CALL1(art_get_name_, artFID)
}

long __stdcall ArtId(long artType, long lstIndex, long animCode, long weaponCode, long directionCode) {
	WRAP_WATCOM_CALL5(art_id_, artType, lstIndex, animCode, weaponCode, directionCode)
}

BYTE* __stdcall ArtFrameData(FrmHeaderData* frm, long frameNum, long rotation) {
	WRAP_WATCOM_CALL3(art_frame_data_, frm, frameNum, rotation)
}

long __stdcall ArtFrameWidth(FrmHeaderData* frm, long frameNum, long rotation) {
	WRAP_WATCOM_CALL3(art_frame_width_, frm, frameNum, rotation)
}

long __stdcall ArtFrameLength(FrmHeaderData* frm, long frameNum, long rotation) {
	WRAP_WATCOM_CALL3(art_frame_length_, frm, frameNum, rotation)
}

FrmHeaderData* __stdcall ArtPtrLock(long frmId, DWORD* lockPtr) {
	WRAP_WATCOM_CALL2(art_ptr_lock_, frmId, lockPtr)
}

BYTE* __stdcall ArtPtrLockData(long frmId, long frameNum, long rotation, DWORD* lockPtr) {
	WRAP_WATCOM_CALL4(art_ptr_lock_data_, frmId, frameNum, rotation, lockPtr)
}

BYTE* __stdcall ArtLock(long frmId, DWORD* lockPtr, long* widthOut, long* heightOut) {
	WRAP_WATCOM_CALL4(art_lock_, frmId, lockPtr, widthOut, heightOut)
}

long __stdcall ArtPtrUnlock(DWORD lockId) {
	WRAP_WATCOM_CALL1(art_ptr_unlock_, lockId)
}

long __stdcall BarterComputeValue(TGameObj* source, TGameObj* target) {
	WRAP_WATCOM_CALL2(barter_compute_value_, source, target)
}

long __stdcall BlockForTocks(long ticks) {
	WRAP_WATCOM_CALL1(block_for_tocks_, ticks)
}

// Returns the name of the critter
const char* __stdcall CritterName(TGameObj* critter) {
	WRAP_WATCOM_CALL1(critter_name_, critter)
}

// Change the name of playable character
void __stdcall CritterPcSetName(const char* newName) {
	WRAP_WATCOM_CALL1(critter_pc_set_name_, newName)
}

// Checks if given file exists in DB
bool __stdcall DbAccess(const char* fileName) {
	WRAP_WATCOM_CALL1(db_access_, fileName)
}

long __stdcall DbFClose(DbFile* file) {
	WRAP_WATCOM_CALL1(db_fclose_, file)
}

DbFile* __stdcall DbFOpen(const char* path, const char* mode) {
	WRAP_WATCOM_CALL2(db_fopen_, path, mode)
}

long __stdcall DbFGetc(DbFile* file) {
	WRAP_WATCOM_CALL1(db_fgetc_, file)
}

char* __stdcall DbFGets(char* buf, long max_count, DbFile* file) {
	WRAP_WATCOM_CALL3(db_fgets_, buf, max_count, file)
}

long __stdcall DbFRead(void* buf, long elsize, long count, DbFile* file) {
	WRAP_WATCOM_CALL4(db_fread_, buf, elsize, count, file)
}

long __stdcall DbFSeek(DbFile* file, long pos, long origin) {
	WRAP_WATCOM_CALL3(db_fseek_, file, pos, origin)
}

// Destroys filelist array created by DbGetFileList
void __stdcall DbFreeFileList(char*** fileList, DWORD arg2) {
	WRAP_WATCOM_CALL2(db_free_file_list_, fileList, arg2)
}

long __stdcall DbFReadByte(DbFile* file, BYTE* rout) {
	WRAP_WATCOM_CALL2(db_freadByte_, file, rout)
}

long __stdcall DbFReadShort(DbFile* file, WORD* rout) {
	WRAP_WATCOM_CALL2(db_freadShort_, file, rout)
}

long __stdcall DbFReadInt(DbFile* file, DWORD* rout) {
	WRAP_WATCOM_CALL2(db_freadInt_, file, rout)
}

long __stdcall DbFReadByteCount(DbFile* file, BYTE* cptr, long count) {
	WRAP_WATCOM_CALL3(db_freadByteCount_, file, cptr, count)
}

long __stdcall DbFReadShortCount(DbFile* file, WORD* dest, long count) {
	WRAP_WATCOM_CALL3(db_freadShortCount_, file, dest, count)
}

long __stdcall DbFReadIntCount(DbFile* file, DWORD* dest, long count) {
	WRAP_WATCOM_CALL3(db_freadIntCount_, file, dest, count)
}

long __stdcall DbFWriteByte(DbFile* file, long value) {
	WRAP_WATCOM_CALL2(db_fwriteByte_, file, value)
}

long __stdcall DbFWriteInt(DbFile* file, long value) {
	WRAP_WATCOM_CALL2(db_fwriteInt_, file, value)
}

long __stdcall DbFWriteByteCount(DbFile* file, const BYTE* cptr, long count) {
	WRAP_WATCOM_CALL3(db_fwriteByteCount_, file, cptr, count)
}

// Check fallout file and get file size (result 0 - file exists)
long __stdcall DbDirEntry(const char *fileName, DWORD *sizeOut) {
	WRAP_WATCOM_CALL2(db_dir_entry_, fileName, sizeOut)
}

// Searches files in DB by given path/filename mask and stores result in fileList
// fileList is a pointer to a variable, that will be assigned with an address of an array of char* strings
// Returns number of elements in *fileList
long __stdcall DbGetFileList(const char* searchMask, char* * *fileList) {
	WRAP_WATCOM_CALL2(db_get_file_list_, searchMask, fileList)
}

long __stdcall DbInit(const char* path_dat, const char* path_patches) {
	WRAP_WATCOM_CALL2(db_init_, path_dat, path_patches)
}

void* __stdcall DbaseOpen(const char* fileName) {
	WRAP_WATCOM_CALL1(dbase_open_, fileName)
}

void __stdcall DbaseClose(void* dbPtr) {
	WRAP_WATCOM_CALL1(dbase_close_, dbPtr)
}

// Displays message in main UI console window
void __stdcall DisplayPrint(const char* msg) {
	WRAP_WATCOM_CALL1(display_print_, msg)
}

void __stdcall DisplayStats() {
	WRAP_WATCOM_CALL0(display_stats_)
}

long __stdcall CritterIsDead(TGameObj* critter) {
	WRAP_WATCOM_CALL1(critter_is_dead_, critter)
}

// Execute script proc by internal proc number (from script's proc table, basically a sequential number of a procedure as defined in code, starting from 1)
void __stdcall ExecuteProcedure(TProgram* sptr, long procNum) {
	WRAP_WATCOM_CALL2(executeProcedure_, sptr, procNum)
}

// Returns the name of current procedure by program pointer
const char* __stdcall FindCurrentProc(TProgram* program) {
	WRAP_WATCOM_CALL1(findCurrentProc_, program)
}

long __stdcall FMtextWidth(const char* text) {
	WRAP_WATCOM_CALL1(FMtext_width_, text)
}

long __stdcall GetInputBtn() {
	WRAP_WATCOM_CALL0(get_input_)
}

// Searches for message ID in given message file and places result in result argument
const char* __stdcall Getmsg(const MSGList* fileAddr, MSGNode* result, long messageId) {
	WRAP_WATCOM_CALL3(getmsg_, fileAddr, result, messageId)
}

long __stdcall Gmouse3dGetMode() {
	WRAP_WATCOM_CALL0(gmouse_3d_get_mode_)
}

void __stdcall Gmouse3dSetMode(long mode) {
	WRAP_WATCOM_CALL1(gmouse_3d_set_mode_, mode)
}

long __stdcall GmouseSetCursor(long picNum) {
	WRAP_WATCOM_CALL1(gmouse_set_cursor_, picNum)
}

long __stdcall GsoundBackgroundVolumeGetSet(long setVolume) {
	WRAP_WATCOM_CALL1(gsound_background_volume_get_set_, setVolume)
}

// Plays SFX sound with given name
void __stdcall GsoundPlaySfxFile(const char* name) {
	WRAP_WATCOM_CALL1(gsound_play_sfx_file_, name)
}

WINinfo* __stdcall GNWFind(long winRef) {
	WRAP_WATCOM_CALL1(GNW_find_, winRef)
}

long __stdcall Interpret(TProgram* program, long arg2) {
	WRAP_WATCOM_CALL2(interpret_, program, arg2)
}

long __stdcall InterpretFindProcedure(TProgram* scriptPtr, const char* procName) {
	WRAP_WATCOM_CALL2(interpretFindProcedure_, scriptPtr, procName)
}

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall InterpretPopShort(TProgram* scriptPtr) {
	WRAP_WATCOM_CALL1(interpretPopShort_, scriptPtr)
}

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall InterpretPopLong(TProgram* scriptPtr) {
	WRAP_WATCOM_CALL1(interpretPopLong_, scriptPtr)
}

long __stdcall IntfaceGetAttack(DWORD* hitMode, DWORD* isSecondary) {
	WRAP_WATCOM_CALL2(intface_get_attack_, hitMode, isSecondary)
}

long __stdcall IntfaceIsHidden() {
	WRAP_WATCOM_CALL0(intface_is_hidden_)
}

// Redraws the main game interface windows (useful after changing some data like active hand, etc.)
void __stdcall IntfaceRedraw() {
	WRAP_WATCOM_CALL0(intface_redraw_)
}

void __stdcall IntfaceToggleItemState() {
	WRAP_WATCOM_CALL0(intface_toggle_item_state_)
}

void __stdcall IntfaceUpdateAc(long animate) {
	WRAP_WATCOM_CALL1(intface_update_ac_, animate)
}

void __stdcall IntfaceUpdateMovePoints(long ap, long freeAP) {
	WRAP_WATCOM_CALL2(intface_update_move_points_, ap, freeAP)
}

void __stdcall IntfaceUseItem() {
	WRAP_WATCOM_CALL0(intface_use_item_)
}

// Item in critter's left hand slot
TGameObj* __stdcall InvenLeftHand(TGameObj* critter) {
	WRAP_WATCOM_CALL1(inven_left_hand_, critter)
}

// Item in critter's right hand slot
TGameObj* __stdcall InvenRightHand(TGameObj* critter) {
	WRAP_WATCOM_CALL1(inven_right_hand_, critter)
}

TGameObj* __stdcall InvenPidIsCarriedPtr(TGameObj* invenObj, long pid) {
	WRAP_WATCOM_CALL2(inven_pid_is_carried_ptr_, invenObj, pid)
}

long __stdcall InvenUnwield(TGameObj* critter, long slot) {
	WRAP_WATCOM_CALL2(inven_unwield_, critter, slot)
}

// Critter worn item (armor)
TGameObj* __stdcall InvenWorn(TGameObj* critter) {
	WRAP_WATCOM_CALL1(inven_worn_, critter)
}

long __stdcall IsPartyMember(TGameObj* obj) {
	WRAP_WATCOM_CALL1(isPartyMember_, obj)
}

long __stdcall IsWithinPerception(TGameObj* source, TGameObj* target) {
	WRAP_WATCOM_CALL2(is_within_perception_, source, target)
}

long __stdcall ItemCCurrSize(TGameObj* critter) {
	WRAP_WATCOM_CALL1(item_c_curr_size_, critter)
}

long __stdcall ItemCapsTotal(TGameObj* object) {
	WRAP_WATCOM_CALL1(item_caps_total_, object)
}

long __stdcall ItemGetType(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_get_type_, item)
}

// Returns 0 on success, -1 if the item has no charges
long __stdcall ItemMDecCharges(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_m_dec_charges_, item)
}

long __stdcall ItemSize(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_size_, item)
}

long __stdcall ItemTotalCost(TGameObj* object) {
	WRAP_WATCOM_CALL1(item_total_cost_, object)
}

long __stdcall ItemTotalWeight(TGameObj* object) {
	WRAP_WATCOM_CALL1(item_total_weight_, object)
}

long __stdcall ItemWAnimWeap(TGameObj* item, DWORD hitMode) {
	WRAP_WATCOM_CALL2(item_w_anim_weap_, item, hitMode)
}

long __stdcall ItemWComputeAmmoCost(TGameObj* item, DWORD* rounds) {
	WRAP_WATCOM_CALL2(item_w_compute_ammo_cost_, item, rounds)
}

long __stdcall ItemWCurrAmmo(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_w_curr_ammo_, item)
}

long __stdcall ItemWMaxAmmo(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_w_max_ammo_, item)
}

long __stdcall ItemWRange(TGameObj* critter, long hitMode) {
	WRAP_WATCOM_CALL2(item_w_range_, critter, hitMode)
}

long __stdcall ItemWReload(TGameObj* weapon, TGameObj* ammo) {
	WRAP_WATCOM_CALL2(item_w_reload_, weapon, ammo)
}

long __stdcall ItemWRounds(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_w_rounds_, item)
}

long __stdcall ItemWeight(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_weight_, item)
}

// Returns light level at given tile
long __stdcall LightGetTile(long elevation, long tileNum) {
	WRAP_WATCOM_CALL2(light_get_tile_, elevation, tileNum)
}

long __stdcall LoadFrame(const char* filename, FrmFile** frmPtr) {
	WRAP_WATCOM_CALL2(load_frame_, filename, frmPtr)
}

TProgram* __stdcall LoadProgram(const char* fileName) {
	WRAP_WATCOM_CALL1(loadProgram_, fileName)
}

const char* __stdcall MapGetShortName(long mapID) {
	WRAP_WATCOM_CALL1(map_get_short_name_, mapID)
}

void __stdcall MapDirErase(const char* folder, const char* ext) {
	WRAP_WATCOM_CALL2(MapDirErase_, folder, ext)
}

void __stdcall MemFree(void* mem) {
	WRAP_WATCOM_CALL1(mem_free_, mem)
}

void* __stdcall MemRealloc(void* lpmem, DWORD msize) {
	WRAP_WATCOM_CALL2(mem_realloc_, lpmem, msize)
}

// Destroys message list
long __stdcall MessageExit(MSGList* msgList) {
	WRAP_WATCOM_CALL1(message_exit_, msgList)
}

// Loads MSG file into given MessageList
long __stdcall MessageLoad(MSGList* msgList, const char* msgFilePath) {
	WRAP_WATCOM_CALL2(message_load_, msgList, msgFilePath)
}

long __stdcall MessageSearch(const MSGList* file, MSGNode* msg) {
	WRAP_WATCOM_CALL2(message_search_, file, msg)
}

void __stdcall MouseGetPosition(long* outX, long* outY) {
	WRAP_WATCOM_CALL2(mouse_get_position_, outX, outY)
}

void __stdcall MouseShow() {
	WRAP_WATCOM_CALL0(mouse_show_)
}

void __stdcall MouseHide() {
	WRAP_WATCOM_CALL0(mouse_hide_)
}

// Calculates path and returns it's length
long __stdcall MakePathFunc(TGameObj* objectFrom, long tileFrom, long tileTo, char* pathDataBuffer, long arg5, void* blockingFunc) {
	WRAP_WATCOM_CALL6(make_path_func_, objectFrom, tileFrom, tileTo, pathDataBuffer, arg5, blockingFunc)
}

long __stdcall NewObjId() {
	WRAP_WATCOM_CALL0(new_obj_id_)
}

// Calculates bounding box (rectangle) for a given object
void __stdcall ObjBound(TGameObj* object, BoundRect* boundRect) {
	WRAP_WATCOM_CALL2(obj_bound_, object, boundRect)
}

long __stdcall ObjDestroy(TGameObj* object) {
	WRAP_WATCOM_CALL1(obj_destroy_, object)
}

long __stdcall ObjDist(TGameObj* obj_src, TGameObj* obj_trg) {
	WRAP_WATCOM_CALL2(obj_dist_, obj_src, obj_trg)
}

long __stdcall ObjEraseObject(TGameObj* object, BoundRect* boundRect) {
	WRAP_WATCOM_CALL2(obj_erase_object_, object, boundRect)
}

TGameObj* __stdcall ObjFindFirst() {
	WRAP_WATCOM_CALL0(obj_find_first_)
}

TGameObj* __stdcall ObjFindNext() {
	WRAP_WATCOM_CALL0(obj_find_next_)
}

TGameObj* __stdcall ObjFindFirstAtTile(long elevation, long tileNum) {
	WRAP_WATCOM_CALL2(obj_find_first_at_tile_, elevation, tileNum)
}

TGameObj* __stdcall ObjFindNextAtTile() {
	WRAP_WATCOM_CALL0(obj_find_next_at_tile_)
}

long __stdcall ObjPidNew(TGameObj* object, long pid) {
	WRAP_WATCOM_CALL2(obj_pid_new_, object, pid)
}

// Checks/unjams jammed locks
long __stdcall ObjLockIsJammed(TGameObj* object) {
	WRAP_WATCOM_CALL1(obj_lock_is_jammed_, object)
}

void __stdcall ObjUnjamLock(TGameObj* object) {
	WRAP_WATCOM_CALL1(obj_unjam_lock_, object)
}

long __stdcall PartyMemberGetCurrentLevel(TGameObj* obj) {
	WRAP_WATCOM_CALL1(partyMemberGetCurLevel_, obj)
}

long __stdcall PerkCanAdd(TGameObj* critter, long perkId) {
	WRAP_WATCOM_CALL2(perk_can_add_, critter, perkId)
}

long __stdcall PerkLevel(TGameObj* critter, long perkId) {
	WRAP_WATCOM_CALL2(perk_level_, critter, perkId)
}

long __stdcall PickDeath(TGameObj* attacker, TGameObj* target, TGameObj* weapon, long amount, long anim, long hitFromBack) {
	WRAP_WATCOM_CALL6(pick_death_, attacker, target, weapon, amount, anim, hitFromBack)
}

void __stdcall ProcessBk() {
	WRAP_WATCOM_CALL0(process_bk_)
}

void __stdcall ProtoDudeUpdateGender() {
	WRAP_WATCOM_CALL0(proto_dude_update_gender_)
}

long* __stdcall QueueFindFirst(TGameObj* object, long qType) {
	WRAP_WATCOM_CALL2(queue_find_first_, object, qType)
}

long* __stdcall QueueFindNext(TGameObj* object, long qType) {
	WRAP_WATCOM_CALL2(queue_find_next_, object, qType)
}

long __stdcall RegisterObjectAnimateAndHide(TGameObj* object, long anim, long delay) {
	WRAP_WATCOM_CALL3(register_object_animate_and_hide_, object, anim, delay)
}

long __stdcall RegisterObjectChangeFid(TGameObj* object, long artFid, long delay) {
	WRAP_WATCOM_CALL3(register_object_change_fid_, object, artFid, delay)
}

long __stdcall RegisterObjectLight(TGameObj* object, long lightRadius, long delay) {
	WRAP_WATCOM_CALL3(register_object_light_, object, lightRadius, delay)
}

long __stdcall RegisterObjectMustErase(TGameObj* object) {
	WRAP_WATCOM_CALL1(register_object_must_erase_, object)
}

long __stdcall RegisterObjectTakeOut(TGameObj* object, long holdFrameId, long nothing) {
	WRAP_WATCOM_CALL3(register_object_take_out_, object, holdFrameId, nothing)
}

long __stdcall RegisterObjectTurnTowards(TGameObj* object, long tileNum, long nothing) {
	WRAP_WATCOM_CALL3(register_object_turn_towards_, object, tileNum, nothing)
}

long __stdcall RollRandom(long minValue, long maxValue) {
	WRAP_WATCOM_CALL2(roll_random_, minValue, maxValue)
}

long* __stdcall RunProgram(TProgram* progPtr) {
	WRAP_WATCOM_CALL1(runProgram_, progPtr)
}

TScript* __stdcall ScrFindFirstAt(long elevation) {
	WRAP_WATCOM_CALL1(scr_find_first_at_, elevation)
}

TScript* __stdcall ScrFindNextAt() {
	WRAP_WATCOM_CALL0(scr_find_next_at_)
}

TGameObj* __stdcall ScrFindObjFromProgram(TProgram* program) {
	WRAP_WATCOM_CALL1(scr_find_obj_from_program_, program)
}

long __stdcall ScrNew(long* scriptID, long sType) {
	WRAP_WATCOM_CALL2(scr_new_, scriptID, sType)
}

// Saves pointer to script object into scriptPtr using scriptID
// Returns 0 on success, -1 on failure
long __stdcall ScrPtr(long scriptId, TScript** scriptPtr) {
	WRAP_WATCOM_CALL2(scr_ptr_, scriptId, scriptPtr)
}

long __stdcall ScrRemove(long scriptID) {
	WRAP_WATCOM_CALL1(scr_remove_, scriptID)
}

void __stdcall SetFocusFunc(void* func) {
	WRAP_WATCOM_CALL1(set_focus_func_, func)
}

long __stdcall SkillIsTagged(long skill) {
	WRAP_WATCOM_CALL1(skill_is_tagged_, skill)
}

long __stdcall StatGetBaseDirect(TGameObj* critter, long statID) {
	WRAP_WATCOM_CALL2(stat_get_base_direct_, critter, statID)
}

long __stdcall StatLevel(TGameObj* critter, long statId) {
	WRAP_WATCOM_CALL2(stat_level_, critter, statId)
}

long __stdcall TextFont(long fontNum) {
	WRAP_WATCOM_CALL1(text_font_, fontNum)
}

long __stdcall TileDist(long scrTile, long dstTile) {
	WRAP_WATCOM_CALL2(tile_dist_, scrTile, dstTile)
}

long __stdcall TileDir(long scrTile, long dstTile) {
	WRAP_WATCOM_CALL2(tile_dir_, scrTile, dstTile)
}

// Redraws the whole screen
void __stdcall TileRefreshDisplay() {
	WRAP_WATCOM_CALL0(tile_refresh_display_)
}

// Redraws the given rectangle on screen
void __stdcall TileRefreshRect(BoundRect* boundRect, long elevation) {
	WRAP_WATCOM_CALL2(tile_refresh_rect_, boundRect, elevation)
}

long __stdcall TraitLevel(long traitID) {
	WRAP_WATCOM_CALL1(trait_level_, traitID)
}

long __stdcall WinAdd(long x, long y, long width, long height, long bgColorIndex, long flags) {
	WRAP_WATCOM_CALL6(win_add_, x, y, width, height, bgColorIndex, flags)
}

void __stdcall WinShow(DWORD winRef) {
	WRAP_WATCOM_CALL1(win_show_, winRef)
}

void __stdcall WinHide(DWORD winRef) {
	WRAP_WATCOM_CALL1(win_hide_, winRef)
}

BYTE* __stdcall WinGetBuf(DWORD winRef) {
	WRAP_WATCOM_CALL1(win_get_buf_, winRef)
}

void __stdcall WinDraw(DWORD winRef) {
	WRAP_WATCOM_CALL1(win_draw_, winRef)
}

void __stdcall WinDrawRect(DWORD winRef, RECT* rect) {
	WRAP_WATCOM_CALL2(win_draw_rect_, winRef, rect)
}

void __stdcall WinDelete(DWORD winRef) {
	WRAP_WATCOM_CALL1(win_delete_, winRef)
}

long __stdcall WindowWidth() {
	WRAP_WATCOM_CALL0(windowWidth_)
}

void __stdcall WmRefreshInterfaceOverlay(long isRedraw) {
	WRAP_WATCOM_CALL1(wmRefreshInterfaceOverlay_, isRedraw)
}

DbFile* __stdcall XFOpen(const char* fileName, const char* flags) {
	WRAP_WATCOM_CALL2(xfopen_, fileName, flags)
}

long __stdcall XFSeek(DbFile* file, long fOffset, long origin) {
	WRAP_WATCOM_CALL3(xfseek_, file, fOffset, origin)
}

/* fastcall */
long __fastcall WordWrap(const char* text, int maxWidth, DWORD* buf, BYTE* count) {
	WRAP_WATCOM_FCALL4(_word_wrap_, text, maxWidth, buf, count)
}

void __fastcall CheckForDeath(TGameObj* critter, long amountDamage, long* flags) {
	WRAP_WATCOM_FCALL3(check_for_death_, critter, amountDamage, flags)
}

void __fastcall CorrectFidForRemovedItemFunc(TGameObj* critter, TGameObj* item, long slotFlag) {
	WRAP_WATCOM_FCALL3(correctFidForRemovedItem_, critter, item, slotFlag)
}

long __fastcall CreateWindowFunc(const char* winName, DWORD x, DWORD y, DWORD width, DWORD height, long color, long flags) {
	WRAP_WATCOM_FCALL7(createWindow_, winName, x, y, width, height, color, flags)
}

long __fastcall DetermineToHit(TGameObj* source, TGameObj* target, long bodyPart, long hitMode) {
	WRAP_WATCOM_FCALL4(determine_to_hit_, source, target, bodyPart, hitMode)
}

void __fastcall DisplayInventory(long inventoryOffset, long visibleOffset, long mode) {
	WRAP_WATCOM_FCALL3(display_inventory_, inventoryOffset, visibleOffset, mode)
}

void __fastcall DisplayTargetInventory(long inventoryOffset, long visibleOffset, DWORD* targetInventory, long mode) {
	WRAP_WATCOM_FCALL4(display_target_inventory_, inventoryOffset, visibleOffset, targetInventory, mode)
}

FrmFrameData* __fastcall FramePtr(FrmHeaderData* frm, long frame, long direction) {
	WRAP_WATCOM_FCALL3(frame_ptr_, frm, frame, direction)
}

void __fastcall GNWWinRefresh(WINinfo* win, BoundRect* rect, long* buffer) {
	WRAP_WATCOM_FCALL3(GNW_win_refresh_, win, rect, buffer)
}

void __fastcall IntfaceUpdateItems(long animate, long modeLeft, long modeRight) {
	WRAP_WATCOM_FCALL3(intface_update_items_, animate, modeLeft, modeRight)
}

TGameObj* __fastcall InvenFindType(TGameObj* critter, long itemType, DWORD* buf) {
	WRAP_WATCOM_FCALL3(inven_find_type_, critter, itemType, buf)
}

long __fastcall ItemAddForce(TGameObj* critter, TGameObj* item, long count) {
	WRAP_WATCOM_FCALL3(item_add_force_, critter, item, count)
}

long __fastcall ItemWMpCost(TGameObj* source, long hitMode, long isCalled) {
	WRAP_WATCOM_FCALL3(item_w_mp_cost_, source, hitMode, isCalled)
}

void __fastcall MakeStraightPathFunc(TGameObj* objFrom, DWORD tileFrom, DWORD tileTo, void* rotationPtr, DWORD* result, long flags, void* func) {
	WRAP_WATCOM_FCALL7(make_straight_path_func_, objFrom, tileFrom, tileTo, rotationPtr, result, flags, func)
}

long __fastcall MessageFind(DWORD* msgFile, long msgNumber, DWORD* outBuf) {
	WRAP_WATCOM_FCALL3(message_find_, msgFile, msgNumber, outBuf)
}

long __fastcall MouseClickIn(long x, long y, long x_end, long y_end) {
	WRAP_WATCOM_FCALL4(mouse_click_in_, x, y, x_end, y_end)
}

TGameObj* __fastcall ObjBlockingAt(TGameObj* object, long tile, long elevation) {
	WRAP_WATCOM_FCALL3(obj_blocking_at_, object, tile, elevation)
}

long __fastcall ObjNewSidInst(TGameObj* object, long sType, long scriptIndex) {
	WRAP_WATCOM_FCALL3(obj_new_sid_inst_, object, sType, scriptIndex)
}

long __fastcall ObjectUnderMouse(long crSwitch, long inclDude, long elevation) {
	WRAP_WATCOM_FCALL3(object_under_mouse_, crSwitch, inclDude, elevation)
}

void __fastcall RegisterObjectCall(long* target, long* source, void* func, long delay) {
	WRAP_WATCOM_FCALL4(register_object_call_, target, source, func, delay)
}

long __fastcall ScrGetLocalVar(long sid, long varId, long* value) {
	WRAP_WATCOM_FCALL3(scr_get_local_var_, sid, varId, value)
}

long __fastcall ScrSetLocalVar(long sid, long varId, long value) {
	WRAP_WATCOM_FCALL3(scr_set_local_var_, sid, varId, value)
}

long __fastcall TileNumInDirection(long tile, long rotation, long distance) {
	WRAP_WATCOM_FCALL3(tile_num_in_direction_, tile, rotation, distance)
}

const char* __fastcall InterpretGetString(TProgram* scriptPtr, DWORD dataType, DWORD strId) {
	WRAP_WATCOM_FCALL3(interpretGetString_, scriptPtr, dataType, strId)
}

///////////////////////////////// ENGINE UTILS /////////////////////////////////

static MSGNode messageBuf;

const char* GetMessageStr(const MSGList* fileAddr, long messageId) {
	return Getmsg(fileAddr, &messageBuf, messageId);
}

const char* MsgSearch(const MSGList* fileAddr, long messageId) {
	messageBuf.number = messageId;
	if (MessageSearch(fileAddr, &messageBuf) == 1) {
		return messageBuf.message;
	}
	return nullptr;
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

long AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		char* proto = GetProtoPtr(weapon->protoId);
		if (proto != nullptr && *(int*)(proto + 32) == item_type_weapon) {
			return *(int*)(proto + 36);
		}
	}
	return 0;
}

void SkillGetTags(long* result, long num) {
	if (num > 4) num = 4;
	WRAP_WATCOM_CALL2(skill_get_tags_, result, num)
}

void SkillSetTags(long* tags, long num) {
	if (num > 4) num = 4;
	WRAP_WATCOM_CALL2(skill_set_tags_, tags, num)
}

long __fastcall GetItemType(TGameObj* item) {
	return ItemGetType(item);
}

__declspec(noinline) TGameObj* __stdcall GetItemPtrSlot(TGameObj* critter, InvenType slot) {
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

long& GetActiveItemMode() {
	return ptr_itemButtonItems[*ptr_itemCurrentItem].mode;
}

TGameObj* GetActiveItem() {
	return ptr_itemButtonItems[*ptr_itemCurrentItem].item;
}

bool HeroIsFemale() {
	return (StatLevel(*ptr_obj_dude, STAT_gender) == GENDER_FEMALE);
}

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence() {
	QueueRadiation* queue = (QueueRadiation*)QueueFindFirst(*ptr_obj_dude, radiation_event);
	while (queue) {
		if (queue->init && queue->level >= 2) return 1;
		queue = (QueueRadiation*)QueueFindNext(*ptr_obj_dude, radiation_event);
	}
	return 0;
}

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid) {
	TScript* script = nullptr;
	ScrPtr(sid, &script);
	return (script) ? script->numLocalVars : 0;
}

// Returns window ID by x/y coordinate (hidden windows are ignored)
long __fastcall GetTopWindowID(long xPos, long yPos) {
	WINinfo* win = nullptr;
	long countWin = *ptr_num_windows - 1;
	for (int n = countWin; n >= 0; n--) {
		win = ptr_window[n];
		if (xPos >= win->wRect.left && xPos <= win->wRect.right && yPos >= win->wRect.top && yPos <= win->wRect.bottom) {
			if (!(win->flags & WinFlags::Hidden)) {
				break;
			}
		}
	}
	return win->wID;
}

enum WinNameType {
	WINTYPE_Inventory = 0, // any inventory window (player/loot/use/barter)
	WINTYPE_Dialog    = 1,
	WINTYPE_PipBoy    = 2,
	WINTYPE_WorldMap  = 3,
	WINTYPE_IfaceBar  = 4, // the interface bar
	WINTYPE_Character = 5,
	WINTYPE_Skilldex  = 6,
	WINTYPE_EscMenu   = 7, // escape menu
	//WINTYPE_Automap   = 8  // for this window there is no global variable
};

WINinfo* GetUIWindow(long winType) {
	long winID = 0;
	switch (winType) {
	case WINTYPE_Inventory:
		winID = *ptr_i_wid;
		break;
	case WINTYPE_Dialog:
		winID = *ptr_dialogueBackWindow;
		break;
	case WINTYPE_PipBoy:
		winID = *ptr_pip_win;
		break;
	case WINTYPE_WorldMap:
		winID = *ptr_wmBkWin;
		break;
	case WINTYPE_IfaceBar:
		winID = *ptr_interfaceWindow;
		break;
	case WINTYPE_Character:
		winID = *ptr_edit_win;
		break;
	case WINTYPE_Skilldex:
		winID = *ptr_skldxwin;
		break;
	case WINTYPE_EscMenu:
		winID = *ptr_optnwin;
		break;
	default:
		return (WINinfo*)(-1);
	}
	return (winID > 0) ? GNWFind(winID) : nullptr;
}

static long GetRangeTileNumbers(long sourceTile, long radius, long &outEnd) {
	long hexRadius = 200 * (radius + 1);

	outEnd = sourceTile + hexRadius;
	if (outEnd > 40000) outEnd = 40000;

	long startTile = sourceTile - hexRadius;
	return (startTile < 0) ? 0 : startTile;
}

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<TGameObj*> &objs, long sourceTile, long radius, long elev, long type) {
	long endTile;
	for (long tile = GetRangeTileNumbers(sourceTile, radius, endTile); tile < endTile; tile++) {
		TGameObj* obj = ObjFindFirstAtTile(elev, tile);
		while (obj) {
			if (type == -1 || type == obj->Type()) {
				bool multiHex = (obj->flags & ObjectFlag::MultiHex) ? true : false;
				if (TileDist(sourceTile, obj->tile) <= (radius + multiHex)) {
					objs.push_back(obj);
				}
			}
			obj = ObjFindNextAtTile();
		}
	}
}

// Checks the blocking tiles and returns the first blocking object
TGameObj* CheckAroundBlockingTiles(TGameObj* source, long dstTile) {
	long rotation = 5;
	do {
		long chkTile = TileNumInDirection(dstTile, rotation, 1);
		TGameObj* obj = ObjBlockingAt(source, chkTile, source->elevation);
		if (obj) return obj;
	} while (--rotation >= 0);

	return nullptr;
}

TGameObj* __fastcall MultiHexMoveIsBlocking(TGameObj* source, long dstTile) {
	if (TileDist(source->tile, dstTile) > 1) {
		return CheckAroundBlockingTiles(source, dstTile);
	}
	// Checks the blocking arc of adjacent tiles
	long dir = TileDir(source->tile, dstTile);

	long chkTile = TileNumInDirection(dstTile, dir, 1);
	TGameObj* obj = ObjBlockingAt(source, chkTile, source->elevation);
	if (obj) return obj;

	// +1 direction
	long rotation = (dir + 1) % 6;
	chkTile = TileNumInDirection(dstTile, rotation, 1);
	obj = ObjBlockingAt(source, chkTile, source->elevation);
	if (obj) return obj;

	// -1 direction
	rotation = (dir + 5) % 6;
	chkTile = TileNumInDirection(dstTile, rotation, 1);
	obj = ObjBlockingAt(source, chkTile, source->elevation);
	if (obj) return obj;

	return nullptr;
}

// Returns the type of the terrain sub tile at the the player's position on the world map
long wmGetCurrentTerrainType() {
	long* terrainId = *(long**)_world_subtile;
	if (terrainId == nullptr) {
		__asm {
			lea  ebx, terrainId;
			mov  edx, dword ptr ds:[_world_ypos];
			mov  eax, dword ptr ds:[_world_xpos];
			call wmFindCurSubTileFromPos_;
		}
	}
	return *terrainId;
}

//---------------------------------------------------------
// copy the area from the interface buffer to the data array
void SurfaceCopyToMem(long fromX, long fromY, long width, long height, long fromWidth, BYTE* fromSurface, BYTE* toMem) {
	fromSurface += fromY * fromWidth + fromX;
	long i = 0;
	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			toMem[i++] = fromSurface[w];
		}
		fromSurface += fromWidth;
	}
}

// safe copy data from memory to the area of the interface buffer
void DrawToSurface(long toX, long toY, long width, long height, long toWidth, long toHeight, BYTE* toSurface, BYTE* fromMem) {
	BYTE* _toSurface = toSurface + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurface;
	long i = 0;
	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurface + w > endToSurf) return;
			if (_toSurface >= toSurface) _toSurface[w] = fromMem[i++];
		}
		_toSurface += toWidth;
	}
}

// safe copy data from surface to surface with mask
void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf,
                   long toX, long toY, long toWidth, long toHeight, BYTE* toSurf, int maskRef)
{
	BYTE* _fromSurf = fromSurf + (fromY * fromWidth + fromX);
	BYTE* _toSurf =  toSurf + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurf;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurf + w > endToSurf) return;
			if (_toSurf >= toSurf && _fromSurf[w] != maskRef) _toSurf[w] = _fromSurf[w];
		}
		_fromSurf += fromWidth;
		_toSurf += toWidth;
	}
}

// safe copy data from surface to surface
void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf,
                   long toX, long toY, long toWidth, long toHeight, BYTE* toSurf)
{
	BYTE* _fromSurf = fromSurf + (fromY * fromWidth + fromX);
	BYTE* _toSurf = toSurf + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurf;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurf + w > endToSurf) return;
			if (_toSurf >= toSurf) _toSurf[w] = _fromSurf[w];
		}
		_fromSurf += fromWidth;
		_toSurf += toWidth;
	}
}

// Fills the specified non-scripted interface window with black color
void ClearWindow(DWORD winID, bool refresh) {
	__asm {
		xor  ebx, ebx;
		push ebx;
		mov  eax, winID;
		call win_height_;
		push eax;
		mov  eax, winID;
		call win_width_;
		mov  ecx, eax;
		mov  edx, ebx;
		mov  eax, winID;
		call win_fill_;
	}
	if (refresh) WinDraw(winID);
}

//---------------------------------------------------------
// print text to surface
void __stdcall PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = yPos * toWidth + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call dword ptr ds:[_text_to_buf];
	}
}

void __stdcall PrintTextFM(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = yPos * toWidth + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call FMtext_to_buf_;
	}
}

//---------------------------------------------------------
//gets the height of the currently selected font
DWORD __stdcall GetTextHeight() {
//	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[_text_height]; //get text height
//		mov  TxtHeight, eax;
	}
//	return TxtHeight;
}

//---------------------------------------------------------
//gets the length of a string using the currently selected font
DWORD __stdcall GetTextWidth(const char* TextMsg) {
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[_text_width]; //get text width
	}
}

DWORD __stdcall GetTextWidthFM(const char* TextMsg) {
	return FMtextWidth(TextMsg); //get text width
}

//---------------------------------------------------------
//get width of Char for current font
DWORD __stdcall GetCharWidth(char charVal) {
	__asm {
		mov  al, charVal;
		call dword ptr ds:[_text_char_width];
	}
}

DWORD __stdcall GetCharWidthFM(char charVal) {
	__asm {
		mov  al, charVal;
		call FMtext_char_width_;
	}
}

//---------------------------------------------------------
//get maximum string length for current font - if all characters were maximum width
DWORD __stdcall GetMaxTextWidth(const char* TextMsg) {
//	DWORD msgWidth;
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[_text_mono_width];
//		mov  msgWidth, eax;
	}
//	return msgWidth;
}

//---------------------------------------------------------
//get number of pixels between characters for current font
DWORD __stdcall GetCharGapWidth() {
//	DWORD gapWidth;
	__asm {
		call dword ptr ds:[_text_spacing];
//		mov  gapWidth, eax;
	}
//	return gapWidth;
}

//---------------------------------------------------------
//get maximum character width for current font
DWORD __stdcall GetMaxCharWidth() {
//	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[_text_max];
//		mov  charWidth, eax;
	}
//	return charWidth;
}

void RedrawObject(TGameObj* obj) {
	BoundRect rect;
	ObjBound(obj, &rect);
	TileRefreshRect(&rect, obj->elevation);
}

// Redraws all interface windows
void RefreshGNW() {
	*(DWORD*)_doing_refresh_all = 1;
	for (size_t i = 0; i < *ptr_num_windows; i++) {
		GNWWinRefresh(ptr_window[i], ptr_scr_size, 0);
	}
	*(DWORD*)_doing_refresh_all = 0;
}

/////////////////////////////////////////////////////////////////UNLISTED FRM FUNCTIONS//////////////////////////////////////////////////////////////

static bool LoadFrmHeader(UNLSTDfrm *frmHeader, DbFile* frmStream) {
	if (DbFReadInt(frmStream, &frmHeader->version) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frmHeader->FPS) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frmHeader->actionFrame) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frmHeader->numFrames) == -1)
		return false;
	else if (DbFReadShortCount(frmStream, frmHeader->xCentreShift, 6) == -1)
		return false;
	else if (DbFReadShortCount(frmStream, frmHeader->yCentreShift, 6) == -1)
		return false;
	else if (DbFReadIntCount(frmStream, frmHeader->oriOffset, 6) == -1)
		return false;
	else if (DbFReadInt(frmStream, &frmHeader->frameAreaSize) == -1)
		return false;

	return true;
}

static bool LoadFrmFrame(UNLSTDfrm::Frame *frame, DbFile* frmStream) {
	//FRMframe *frameHeader = (FRMframe*)frameMEM;
	//BYTE* frameBuff = frame + sizeof(FRMframe);

	if (DbFReadShort(frmStream, &frame->width) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frame->height) == -1)
		return false;
	else if (DbFReadInt(frmStream, &frame->size) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frame->x) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frame->y) == -1)
		return false;

	frame->indexBuff = new BYTE[frame->size];
	if (DbFRead(frame->indexBuff, frame->size, 1, frmStream) != 1)
		return false;

	return true;
}

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef) {
	if (folderRef > OBJ_TYPE_SKILLDEX) return nullptr;

	char *artfolder = ptr_art[folderRef].path; // address of art type name
	char FrmPath[MAX_PATH];

	sprintf_s(FrmPath, MAX_PATH, "art\\%s\\%s", artfolder, frmName);

	UNLSTDfrm *frm = new UNLSTDfrm;

	DbFile* frmStream = XFOpen(FrmPath, "rb");

	if (frmStream != nullptr) {
		if (!LoadFrmHeader(frm, frmStream)) {
			DbFClose(frmStream);
			delete frm;
			return nullptr;
		}

		DWORD oriOffset_1st = frm->oriOffset[0];
		DWORD oriOffset_new = 0;
		frm->frames = new UNLSTDfrm::Frame[6 * frm->numFrames];
		for (int ori = 0; ori < 6; ori++) {
			if (ori == 0 || frm->oriOffset[ori] != oriOffset_1st) {
				frm->oriOffset[ori] = oriOffset_new;
				for (int fNum = 0; fNum < frm->numFrames; fNum++) {
					if (!LoadFrmFrame(&frm->frames[oriOffset_new + fNum], frmStream)) {
						DbFClose(frmStream);
						delete frm;
						return nullptr;
					}
				}
				oriOffset_new += frm->numFrames;
			} else {
				frm->oriOffset[ori] = 0;
			}
		}

		DbFClose(frmStream);
	} else {
		delete frm;
		return nullptr;
	}
	return frm;
}
