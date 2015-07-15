/*
* sfall
* Copyright (C) 2008-2015 The sfall team
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
#pragma once

/* 
* FALLOUT2.EXE structs, function offsets and wrappers should be placed here  
* 
* only place functions and variables here which are likely to be used in more than one module
*
*/

#include "FalloutStructs.h"

#define _ambient_light              0x51923C
#define _art                        0x510738
#define _art_name                   0x56C9E4
#define _art_vault_guy_num          0x5108A4
#define _art_vault_person_nums      0x5108A8
#define _black_palette              0x663FD0
#define _bottom_line                0x664524
#define _btable                     0x59E944
#define _btncnt                     0x43EA1C
#define _CarCurrArea                0x672E68
#define _cmap                       0x51DF34
#define _combat_free_move           0x56D39C
#define _combat_list                0x56D390
#define _combat_state               0x510944
#define _combat_turn_running        0x51093C
#define _combatNumTurns             0x510940
#define _crit_succ_eff              0x510978
#define _critterClearObj            0x518438
#define _crnt_func                  0x664508
#define _curr_font_num              0x51E3B0
#define _curr_pc_stat               0x6681AC
#define _curr_stack                 0x59E96C
#define _cursor_line                0x664514
#define _dialog_target              0x518848
#define _dialog_target_is_party     0x51884C
#define _drugInfoList               0x5191CC
#define _edit_win                   0x57060C
#define _Educated                   0x57082C
#define _Experience_                0x6681B4
#define _fallout_game_time          0x51C720
#define _flptr                      0x614808
#define _folder_card_desc           0x5705CC
#define _folder_card_fid            0x5705B0
#define _folder_card_title          0x5705B8
#define _folder_card_title2         0x5705BC
#define _free_perk                  0x570A29
#define _game_global_vars           0x5186C0
#define _game_user_wants_to_quit    0x5186CC
#define _gdBarterMod                0x51873C
#define _gIsSteal                   0x51D430
#define _gmovie_played_list         0x596C78
#define _GreenColor                 0x6A3CB0
#define _hit_location_penalty       0x510954
#define _holo_flag                  0x664529
#define _holopages                  0x66445C
#define _hot_line_count             0x6644F8
#define _i_fid                      0x59E95C
#define _i_lhand                    0x59E958
#define _i_rhand                    0x59E968
#define _i_wid                      0x59E964
#define _i_worn                     0x59E954
#define _info_line                  0x5707D0
#define _interfaceWindow            0x519024
#define _intfaceEnabled             0x518F10
#define _intotal                    0x43E95C
#define _inven_dude                 0x519058
#define _inven_pid                  0x51905C
#define _inven_scroll_dn_bid        0x5190E8
#define _inven_scroll_up_bid        0x5190E4
#define _inventry_message_file      0x59E814
#define _itemButtonItems            0x5970F8
#define _itemCurrentItem            0x518F78
#define _kb_lock_flags              0x51E2EA
#define _last_buttons               0x51E2AC
#define _last_button_winID          0x51E404
#define _last_level                 0x5707B4
#define _Level_                     0x6681B0
#define _Lifegiver                  0x570854
#define _list_com                   0x56D394
#define _list_total                 0x56D37C
#define _LSData                     0x613D30
#define _lsgwin                     0x6142C4
#define _main_ctd                   0x56D2B0
#define _main_window                0x5194F0
#define _map_elevation              0x519578
#define _map_global_vars            0x51956C
#define _mouse_hotx                 0x6AC7D0
#define _mouse_hoty                 0x6AC7CC
#define _mouse_is_hidden            0x6AC790
#define _mouse_x_                   0x6AC7A8
#define _mouse_y                    0x664450
#define _mouse_y_                   0x6AC7A4
#define _Mutate_                    0x5708B4
#define _name_color                 0x56D744
#define _name_font                  0x56D74C
#define _name_sort_list             0x56FCB0
#define _num_game_global_vars       0x5186C4
#define _num_map_global_vars        0x519574
#define _obj_dude                   0x6610B8
#define _outlined_object            0x518D94
#define _partyMemberAIOptions       0x519DB8
#define _partyMemberCount           0x519DAC
#define _partyMemberLevelUpInfoList 0x519DBC
#define _partyMemberList            0x519DA8
#define _partyMemberMaxCount        0x519D9C
#define _partyMemberPidList         0x519DA0
#define _patches                    0x5193CC
#define _paths                      0x6B24D0
#define _pc_crit_succ_eff           0x5179B0
#define _pc_kill_counts             0x56D780
#define _pc_name                    0x56D75C
#define _pc_trait                   0x66BE40
#define _pc_trait2                  0x66BE44
#define _perkLevelDataList          0x51C120
#define _pip_win                    0x6644C4
#define _pipboy_message_file        0x664348
#define _pipmesg                    0x664338
#define _procTableStrs              0x51C758
#define _proto_main_msg_file        0x6647FC
#define _ptable                     0x59E934
#define _pud                        0x59E960
#define _quick_done                 0x5193BC
#define _read_callback              0x51DEEC
#define _RedColor                   0x6AB4D0
#define _retvals                    0x43EA7C
#define _skill_data                 0x51D118
#define _slot_cursor                0x5193B8
#define _sneak_working              0x56D77C
#define _square                     0x631E40
#define _squares                    0x66BE08
#define _stack                      0x59E86C
#define _stack_offset               0x59E844
#define _stat_flag                  0x66452A
#define _Tag_                       0x5708B0
#define _tag_skill                  0x668070
#define _target_curr_stack          0x59E948
#define _target_pud                 0x59E978
#define _target_stack               0x59E81C
#define _target_stack_offset        0x59E7EC
#define _target_str                 0x56D518
#define _text_char_width            0x51E3C4
#define _text_height                0x51E3BC
#define _text_max                   0x51E3D4
#define _text_mono_width            0x51E3C8
#define _text_spacing               0x51E3CC
#define _text_to_buf                0x51E3B8
#define _text_width                 0x51E3C0
#define _title_color                0x56D750
#define _title_font                 0x56D748
#define _view_page                  0x664520
#define _wd_obj                     0x59E98C
#define _wmLastRndTime              0x51DEA0
#define _wmWorldOffsetX             0x51DE2C
#define _wmWorldOffsetY             0x51DE30
#define _world_xpos                 0x672E0C
#define _world_ypos                 0x672E10
#define _YellowColor                0x6AB8BB

// variables
extern TGameObj** obj_dude_ptr;
extern TGameObj** inven_dude_ptr;
extern DWORD* activeUIHand_ptr; // 0 - left, 1 - right
extern DWORD* dude_traits; // 2 of them
extern DWORD* itemCurrentItem;
extern DWORD* itemButtonItems;


// misc. offsets from engine
extern const DWORD action_get_an_object_;
extern const DWORD action_loot_container_;
extern const DWORD action_use_an_item_on_object_;
extern const DWORD add_bar_box_;
extern const DWORD AddHotLines_;
extern const DWORD adjust_ac_;
extern const DWORD adjust_fid_;
extern const DWORD ai_search_inven_armor_;
extern const DWORD art_exists_;
extern const DWORD art_flush_;
extern const DWORD art_frame_data_;
extern const DWORD art_frame_length_;
extern const DWORD art_frame_width_;
extern const DWORD art_get_code_;
extern const DWORD art_id_;
extern const DWORD art_init_;
extern const DWORD art_lock_;
extern const DWORD art_ptr_lock_;
extern const DWORD art_ptr_lock_data_;
extern const DWORD art_ptr_unlock_;
extern const DWORD automap_;
extern const DWORD barter_compute_value_;
extern const DWORD buf_to_buf_;
extern const DWORD check_death_;
extern const DWORD Check4Keys_;
extern const DWORD combat_;
extern const DWORD combat_ai_;
extern const DWORD combat_attack_;
extern const DWORD combat_input_;
extern const DWORD combat_should_end_;
extern const DWORD combat_turn_;
extern const DWORD compute_damage_;
extern const DWORD credits_;
extern const DWORD credits_get_next_line_;
extern const DWORD critter_body_type_;
extern const DWORD critter_can_obj_dude_rest_;
extern const DWORD critter_compute_ap_from_distance_;
extern const DWORD critter_is_dead_;
extern const DWORD critter_kill_;
extern const DWORD critter_kill_count_type_;
extern const DWORD critter_name_;
extern const DWORD critter_pc_set_name_;
extern const DWORD critterClearObjDrugs_;
extern const DWORD db_dir_entry_;
extern const DWORD db_fclose_;
extern const DWORD db_fgetc_;
extern const DWORD db_fgets_;
extern const DWORD db_fopen_;
extern const DWORD db_fread_;
extern const DWORD db_freadByte_;
extern const DWORD db_freadByteCount_;
extern const DWORD db_freadInt_;
extern const DWORD db_freadIntCount_;
extern const DWORD db_freadShort_;
extern const DWORD db_freadShortCount_;
extern const DWORD db_fseek_;
extern const DWORD db_fwriteByte_;
extern const DWORD db_fwriteByteCount_;
extern const DWORD db_fwriteInt_;
extern const DWORD db_read_to_buf_;
extern const DWORD dbase_close_;
extern const DWORD dbase_open_;
extern const DWORD determine_to_hit_func_;
extern const DWORD dialog_out_;
extern const DWORD display_inventory_;
extern const DWORD display_print_;
extern const DWORD display_target_inventory_;
extern const DWORD do_options_;
extern const DWORD do_optionsFunc_;
extern const DWORD do_prefscreen_;
extern const DWORD DOSCmdLineDestroy_;
extern const DWORD DrawCard_;
extern const DWORD DrawFolder_;
extern const DWORD DrawInfoWin_;
extern const DWORD editor_design_;
extern const DWORD elapsed_time_;
extern const DWORD elevator_end_;
extern const DWORD elevator_start_;
extern const DWORD endgame_slideshow_;
extern const DWORD exec_script_proc_;
extern const DWORD executeProcedure_;
extern const DWORD fadeSystemPalette_;
extern const DWORD findVar_;
extern const DWORD folder_print_line_;
extern const DWORD frame_ptr_;
extern const DWORD game_get_global_var_;
extern const DWORD game_help_;
extern const DWORD game_set_global_var_;
extern const DWORD game_time_date_;
extern const DWORD gdialog_barter_cleanup_tables_;
extern const DWORD gdProcess_;
extern const DWORD get_input_;
extern const DWORD get_time_;
extern const DWORD getmsg_;
extern const DWORD gmouse_is_scrolling_;
extern const DWORD gmouse_set_cursor_;
extern const DWORD GNW_find_;
extern const DWORD GNW95_process_message_;
extern const DWORD gsnd_build_weapon_sfx_name_;
extern const DWORD gsound_play_sfx_file_;
extern const DWORD handle_inventory_;
extern const DWORD inc_game_time_;
extern const DWORD insert_withdrawal_;
extern const DWORD interpret_;
extern const DWORD interpretAddString_;
extern const DWORD interpretFindProcedure_;
extern const DWORD interpretFreeProgram_;
extern const DWORD interpretGetString_;
extern const DWORD interpretPopLong_;
extern const DWORD interpretPopShort_;
extern const DWORD interpretPushLong_;
extern const DWORD interpretPushShort_;
extern const DWORD intface_redraw_;
extern const DWORD intface_toggle_item_state_;
extern const DWORD intface_toggle_items_;
extern const DWORD intface_update_ac_;
extern const DWORD intface_update_hit_points_;
extern const DWORD intface_update_items_;
extern const DWORD intface_update_move_points_;
extern const DWORD intface_use_item_;
extern const DWORD inven_display_msg_;
extern const DWORD inven_left_hand_;
extern const DWORD inven_pid_is_carried_ptr_;
extern const DWORD inven_right_hand_;
extern const DWORD inven_unwield_;
extern const DWORD inven_wield_;
extern const DWORD inven_worn_;
extern const DWORD is_within_perception_;
extern const DWORD isPartyMember_;
extern const DWORD item_add_force_;
extern const DWORD item_c_curr_size_;
extern const DWORD item_c_max_size_;
extern const DWORD item_caps_total_;
extern const DWORD item_d_check_addict_;
extern const DWORD item_d_take_drug_;
extern const DWORD item_get_type_;
extern const DWORD item_m_dec_charges_;
extern const DWORD item_m_turn_off_;
extern const DWORD item_move_all_;
extern const DWORD item_mp_cost_;
extern const DWORD item_remove_mult_;
extern const DWORD item_size_;
extern const DWORD item_total_cost_;
extern const DWORD item_total_weight_;
extern const DWORD item_w_anim_code_;
extern const DWORD item_w_anim_weap_;
extern const DWORD item_w_compute_ammo_cost_;
extern const DWORD item_w_cur_ammo_;
extern const DWORD item_w_dam_div_;
extern const DWORD item_w_dam_mult_;
extern const DWORD item_w_damage_;
extern const DWORD item_w_damage_type_;
extern const DWORD item_w_dr_adjust_;
extern const DWORD item_w_max_ammo_;
extern const DWORD item_w_mp_cost_;
extern const DWORD item_w_range_;
extern const DWORD item_w_try_reload_;
extern const DWORD item_weight_;
extern const DWORD light_get_tile_;
extern const DWORD ListDrvdStats_;
extern const DWORD ListSkills_;
extern const DWORD ListTraits_;
extern const DWORD loadColorTable_;
extern const DWORD LoadGame_;
extern const DWORD loadProgram_;
extern const DWORD LoadSlot_;
extern const DWORD main_game_loop_;
extern const DWORD main_menu_hide_;
extern const DWORD main_menu_loop_;
extern const DWORD make_path_func_;
extern const DWORD make_straight_path_func_;
extern const DWORD map_disable_bk_processes_;
extern const DWORD map_enable_bk_processes_;
extern const DWORD mem_free_;
extern const DWORD mem_malloc_;
extern const DWORD mem_realloc_;
extern const DWORD message_add_;
extern const DWORD message_exit_;
extern const DWORD message_filter_;
extern const DWORD message_find_;
extern const DWORD message_init_;
extern const DWORD message_load_;
extern const DWORD message_make_path_;
extern const DWORD message_search_;
extern const DWORD mouse_get_position_;
extern const DWORD mouse_hide_;
extern const DWORD mouse_show_;
extern const DWORD move_inventory_;
extern const DWORD NixHotLines_;
extern const DWORD obj_ai_blocking_at_;
extern const DWORD obj_blocking_at_;
extern const DWORD obj_bound_;
extern const DWORD obj_change_fid_;
extern const DWORD obj_connect_;
extern const DWORD obj_destroy_;
extern const DWORD obj_dist_;
extern const DWORD obj_erase_object_;
extern const DWORD obj_find_first_at_;
extern const DWORD obj_find_first_at_tile_;
extern const DWORD obj_find_next_at_;
extern const DWORD obj_find_next_at_tile_;
extern const DWORD obj_new_sid_inst_;
extern const DWORD obj_outline_object_;
extern const DWORD obj_pid_new_;
extern const DWORD obj_remove_outline_;
extern const DWORD obj_save_dude_;
extern const DWORD obj_scroll_blocking_at_;
extern const DWORD obj_set_light_;
extern const DWORD obj_shoot_blocking_at_;
extern const DWORD obj_sight_blocking_at_;
extern const DWORD obj_use_book_;
extern const DWORD obj_use_power_on_car_;
extern const DWORD OptionWindow_;
extern const DWORD palette_set_to_;
extern const DWORD partyMemberCopyLevelInfo_;
extern const DWORD partyMemberGetAIOptions_;
extern const DWORD partyMemberGetCurLevel_;
extern const DWORD partyMemberIncLevels_;
extern const DWORD partyMemberRemove_;
extern const DWORD pc_flag_off_;
extern const DWORD pc_flag_on_;
extern const DWORD pc_flag_toggle_;
extern const DWORD perform_withdrawal_end_;
extern const DWORD perk_add_;
extern const DWORD perk_add_effect_;
extern const DWORD perk_description_;
extern const DWORD perk_init_;
extern const DWORD perk_level_;
extern const DWORD perk_make_list_;
extern const DWORD perk_name_;
extern const DWORD perk_skilldex_fid_;
extern const DWORD perkGetLevelData_;
extern const DWORD perks_dialog_;
extern const DWORD pick_death_;
extern const DWORD pip_back_;
extern const DWORD pip_print_;
extern const DWORD pipboy_;
extern const DWORD PipStatus_;
extern const DWORD PrintBasicStat_;
extern const DWORD PrintLevelWin_;
extern const DWORD process_bk_;
extern const DWORD protinst_use_item_;
extern const DWORD protinst_use_item_on_;
extern const DWORD proto_dude_update_gender_;
extern const DWORD proto_ptr_;
extern const DWORD pushLongStack_;
extern const DWORD qsort_;
extern const DWORD queue_clear_type_;
extern const DWORD queue_find_first_;
extern const DWORD queue_find_next_;
extern const DWORD queue_remove_this_;
extern const DWORD refresh_box_bar_win_;
extern const DWORD register_begin_;
extern const DWORD register_clear_;
extern const DWORD register_end_;
extern const DWORD register_object_animate_;
extern const DWORD register_object_animate_and_hide_;
extern const DWORD register_object_change_fid_;
extern const DWORD register_object_funset_;
extern const DWORD register_object_light_;
extern const DWORD register_object_must_erase_;
extern const DWORD register_object_take_out_;
extern const DWORD register_object_turn_towards_;
extern const DWORD report_explosion_;
extern const DWORD RestorePlayer_;
extern const DWORD roll_random_;
extern const DWORD runProgram_;
extern const DWORD SaveGame_;
extern const DWORD SavePlayer_;
extern const DWORD scr_exec_map_update_scripts_;
extern const DWORD scr_find_first_at_;
extern const DWORD scr_find_next_at_;
extern const DWORD scr_find_obj_from_program_;
extern const DWORD scr_find_sid_from_program_;
extern const DWORD scr_new_;
extern const DWORD scr_ptr_;
extern const DWORD scr_remove_;
extern const DWORD scr_set_ext_param_;
extern const DWORD scr_set_objs_;
extern const DWORD scr_write_ScriptNode_;
extern const DWORD SexWindow_;
extern const DWORD skill_check_stealing_;
extern const DWORD skill_dec_point_;
extern const DWORD skill_get_tags_;
extern const DWORD skill_inc_point_;
extern const DWORD skill_level_;
extern const DWORD skill_points_;
extern const DWORD skill_set_tags_;
extern const DWORD skill_use_;
extern const DWORD skilldex_select_;
extern const DWORD sprintf_;
extern const DWORD square_num_;
extern const DWORD stat_get_base_direct_;
extern const DWORD stat_get_bonus_;
extern const DWORD stat_level_;
extern const DWORD stat_pc_get_;
extern const DWORD stat_pc_set_;
extern const DWORD strncpy_;
extern const DWORD switch_hand_;
extern const DWORD talk_to_translucent_trans_buf_to_buf_;
extern const DWORD text_font_;
extern const DWORD text_object_create_;
extern const DWORD tile_coord_;
extern const DWORD tile_num_;
extern const DWORD tile_refresh_display_;
extern const DWORD tile_refresh_rect_;
extern const DWORD tile_scroll_to_;
extern const DWORD trait_get_;
extern const DWORD trait_init_;
extern const DWORD trait_level_;
extern const DWORD trait_set_;
extern const DWORD _word_wrap_;
extern const DWORD win_add_;
extern const DWORD win_delete_;
extern const DWORD win_disable_button_;
extern const DWORD win_draw_;
extern const DWORD win_draw_rect_;
extern const DWORD win_enable_button_;
extern const DWORD win_get_buf_;
extern const DWORD win_hide_;
extern const DWORD win_line_;
extern const DWORD win_print_;
extern const DWORD win_register_button_;
extern const DWORD win_register_button_disable_;
extern const DWORD win_show_;
extern const DWORD wmInterfaceScrollTabsStart_;
extern const DWORD wmPartyWalkingStep_;
extern const DWORD wmWorldMapFunc_;
extern const DWORD wmWorldMapLoadTempData_;
extern const DWORD xfclose_;
extern const DWORD xfeof_;
extern const DWORD xfgetc_;
extern const DWORD xfgets_;
extern const DWORD xfilelength_;
extern const DWORD xfopen_;
extern const DWORD xfputc_;
extern const DWORD xfputs_;
extern const DWORD xfread_;
extern const DWORD xfseek_;
extern const DWORD xftell_;
extern const DWORD xfwrite_;
extern const DWORD xrewind_;
extern const DWORD xungetc_;
extern const DWORD xvfprintf_;


/*
* HOW TO USE ENGINE FUNCTIONS:
*
* in ASM code, call offsets directly, don't call wrappers as they might not be _stdcall
* in C++ code, use wrappers (add new ones if the don't exist yet)
*
* Note: USE C++! 
* 1) Place thin __declspec(naked) hooks, only use minimum ASM to pass values to/from C++
* 2) Call _stdcall functions from (1), write those entirely in C++ (with little ASM blocks only to call engine functions, when you are too lazy to add wrapper)
*/

extern const DWORD item_get_type_;
extern const DWORD inven_left_hand_; // eax - object
extern const DWORD inven_right_hand_; // eax - object
extern const DWORD proto_ptr_; // eax - PID, edx - int** - pointer to a pointer to a proto struct
extern const DWORD ai_can_use_weapon_; //  (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0
extern const DWORD item_w_max_ammo_; // eax - object
extern const DWORD item_w_cur_ammo_; // eax - object


// Interface
extern const DWORD intface_redraw_; // no args
extern const DWORD interface_disable_;
extern const DWORD interface_enable_;
extern const DWORD intface_toggle_items_;
extern const unsigned int display_print_; // eax - char* to display
extern const DWORD intface_item_reload_; // no args
extern const DWORD intface_toggle_item_state_; // no args
extern const DWORD intface_use_item_; // no args

// objects
extern const DWORD isPartyMember_; // (<eax> - object) - bool result
extern const DWORD obj_set_light_; // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
extern const DWORD obj_new_;  // int aObj*<eax>, int aPid<ebx>
extern const DWORD obj_turn_off_;  // int aObj<eax>, int ???<edx>
extern const DWORD obj_move_to_tile_;  // int aObj<eax>, int aTile<edx>, int aElev<ebx>
extern const DWORD obj_find_first_at_tile_; //  <eax>(int elevation<eax>, int tile<edx>)
extern const DWORD obj_find_next_at_tile_; // no args
extern const DWORD critter_is_dead_; // eax - critter
extern const DWORD tile_coord_; // eax - tilenum, edx (int*) - x, ebx (int*) - y

// Animation
extern const DWORD tile_refresh_rect_; // (int elevation<edx>, unkown<ecx>)
extern const DWORD register_object_animate_;  // int aObj<eax>, int aAnim<edx>, int delay<ebx>
extern const DWORD register_object_animate_and_hide_;  // int aObj<eax>, int aAnim<edx>, int delay<ebx>
extern const DWORD register_object_must_erase_;  // int aObj<eax>
extern const DWORD register_object_change_fid_;  // int aObj<eax>, int aFid<edx>, int aDelay<ebx>
extern const DWORD register_object_light_; // <eax>(int aObj<eax>, int aRadius<edx>, int aDelay<ebx>)
extern const DWORD register_object_funset_; // int aObj<eax>, int ???<edx>, int aDelay<ebx>
extern const DWORD register_object_take_out_; // int aObj<eax>, int aHoldFrame<edx> - hold frame ID (1 - spear, 2 - club, etc.)
extern const DWORD register_object_turn_towards_; // int aObj<eax>, int aTile<edx>

extern const DWORD art_exists_; // eax - frameID, used for critter FIDs

extern const DWORD getmsg_; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory

#define MSG_FILE_COMBAT		(0x56D368)
#define MSG_FILE_AI			(0x56D510)
#define MSG_FILE_SCRNAME	(0x56D754)
#define MSG_FILE_MISC		(0x58E940)
#define MSG_FILE_CUSTOM		(0x58EA98)
#define MSG_FILE_INVENTRY	(0x59E814)
#define MSG_FILE_ITEM		(0x59E980)
#define MSG_FILE_LSGAME		(0x613D28)
#define MSG_FILE_MAP		(0x631D48)
#define MSG_FILE_OPTIONS	(0x6637E8)
#define MSG_FILE_PERK		(0x6642D4)
#define MSG_FILE_PIPBOY		(0x664348)
#define MSG_FILE_QUESTS		(0x664410)
#define MSG_FILE_PROTO		(0x6647FC)
#define MSG_FILE_SCRIPT		(0x667724)
#define MSG_FILE_SKILL		(0x668080)
#define MSG_FILE_SKILLDEX	(0x6680F8)
#define MSG_FILE_STAT		(0x66817C)
#define MSG_FILE_TRAIT		(0x66BE38)
#define MSG_FILE_WORLDMAP	(0x672FB0)

// WRAPPERS:
int _stdcall IsPartyMember(TGameObj* obj);
TGameObj* GetInvenWeaponLeft(TGameObj* obj);
TGameObj* GetInvenWeaponRight(TGameObj* obj);
char* GetProtoPtr(DWORD pid);
char AnimCodeByWeapon(TGameObj* weapon);
void DisplayConsoleMessage(const char* msg);
const char* _stdcall GetMessageStr(DWORD fileAddr, DWORD messageId);
int __stdcall ItemGetType(TGameObj* item);