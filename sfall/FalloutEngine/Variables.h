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

#pragma once

#include "Structs.h"

#define DWORD unsigned long

// Global variable offsets

#define _aiInfoList                 0x510948
#define _ambient_light              0x51923C
#define _art                        0x510738
#define _art_name                   0x56C9E4
#define _art_vault_guy_num          0x5108A4
#define _art_vault_person_nums      0x5108A8
#define _bckgnd                     0x5707A4
#define _black_palette              0x663FD0
#define _bottom_line                0x664524
#define _btable                     0x59E944
#define _btncnt                     0x43EA1C
#define _CarCurrArea                0x672E68
#define _cmap                       0x51DF34
#define _colorTable                 0x6A38D0
#define _combat_free_move           0x56D39C
#define _combat_list                0x56D390
#define _combat_state               0x510944
#define _combat_turn_running        0x51093C
#define _combatNumTurns             0x510940
#define _crit_succ_eff              0x510978
#define _critter_db_handle          0x58E94C
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
#define _frame_time                 0x5709C4
#define _free_perk                  0x570A29
#define _game_global_vars           0x5186C0
#define _game_user_wants_to_quit    0x5186CC
#define _gcsd                       0x51094C
#define _gdBarterMod                0x51873C
#define _gdNumOptions               0x5186D8
#define _gIsSteal                   0x51D430
#define _glblmode                   0x5709D0
#define _gmouse_current_cursor      0x518C0C
#define _gmovie_played_list         0x596C78
#define _GreenColor                 0x6A3CB0
#define _gsound_initialized         0x518E30
#define _hit_location_penalty       0x510954
#define _holo_flag                  0x664529
#define _holopages                  0x66445C
#define _hot_line_count             0x6644F8
#define _i_fid                      0x59E95C
#define _i_lhand                    0x59E958
#define _i_rhand                    0x59E968
#define _i_wid                      0x59E964
#define _i_worn                     0x59E954
#define _In_WorldMap                0x672E1C
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
#define _loadingGame                0x5194C4
#define _LSData                     0x613D30
#define _lsgwin                     0x6142C4
#define _main_ctd                   0x56D2B0
#define _main_window                0x5194F0
#define _map_elevation              0x519578
#define _map_global_vars            0x51956C
#define _master_db_handle           0x58E948
#define _max                        0x56FB50
#define _maxScriptNum               0x51C7CC
#define _Meet_Frank_Horrigan        0x672E04
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
#define _objectTable                0x639DA0
#define _objItemOutlineState        0x519798
#define _optionRect                 0x58ECC0
#define _outlined_object            0x518D94
#define _partyMemberAIOptions       0x519DB8
#define _partyMemberCount           0x519DAC
#define _partyMemberLevelUpInfoList 0x519DBC
#define _partyMemberList            0x519DA8 // each struct - 4 integers, first integer - objPtr
#define _partyMemberMaxCount        0x519D9C
#define _partyMemberPidList         0x519DA0
#define _patches                    0x5193CC
#define _paths                      0x6B24D0
#define _pc_crit_succ_eff           0x5179B0
#define _pc_kill_counts             0x56D780
#define _pc_name                    0x56D75C
#define _pc_proto                   0x51C370
#define _pc_trait                   0x66BE40
#define _pc_trait2                  0x66BE44
#define _perk_data                  0x519DCC
#define _perkLevelDataList          0x51C120
#define _pip_win                    0x6644C4
#define _pipboy_message_file        0x664348
#define _pipmesg                    0x664338
#define _preload_list_index         0x519640
#define _procTableStrs              0x51C758  // table of procId (from define.h) => procName map
#define _proto_main_msg_file        0x6647FC
#define _ptable                     0x59E934
#define _pud                        0x59E960
#define _queue                      0x6648C0
#define _quick_done                 0x5193BC
#define _read_callback              0x51DEEC
#define _RedColor                   0x6AB4D0
#define _retvals                    0x43EA7C
#define _scr_size                   0x6AC9F0
#define _scriptListInfo             0x51C7C8
#define _skill_data                 0x51D118
#define _slot_cursor                0x5193B8
#define _sneak_working              0x56D77C // DWORD var 
#define _square                     0x631E40
#define _squares                    0x66BE08
#define _stack                      0x59E86C
#define _stack_offset               0x59E844
#define _stat_data                  0x51D53C
#define _stat_flag                  0x66452A
#define _Tag_                       0x5708B0
#define _tag_skill                  0x668070
#define _target_curr_stack          0x59E948
#define _target_pud                 0x59E978
#define _target_stack               0x59E81C
#define _target_stack_offset        0x59E7EC
#define _target_str                 0x56D518
#define _target_xpos                0x672E20
#define _target_ypos                0x672E24
#define _text_char_width            0x51E3C4
#define _text_height                0x51E3BC
#define _text_max                   0x51E3D4
#define _text_mono_width            0x51E3C8
#define _text_spacing               0x51E3CC
#define _text_to_buf                0x51E3B8
#define _text_width                 0x51E3C0
#define _title_color                0x56D750
#define _title_font                 0x56D748
#define _trait_data                 0x51DB84
#define _view_page                  0x664520
#define _wd_obj                     0x59E98C
#define _wmAreaInfoList             0x51DDF8
#define _wmLastRndTime              0x51DEA0
#define _wmWorldOffsetX             0x51DE2C
#define _wmWorldOffsetY             0x51DE30
#define _world_xpos                 0x672E0C
#define _world_ypos                 0x672E10
#define _WorldMapCurrArea           0x672E08
#define _YellowColor                0x6AB8BB

// variables
// TODO: move to separate namespace

extern long* ptr_pc_traits; // 2 of them

extern DWORD* ptr_aiInfoList;
extern DWORD* ptr_ambient_light;
extern DWORD* ptr_art;
extern DWORD* ptr_art_name;
extern DWORD* ptr_art_vault_guy_num;
extern DWORD* ptr_art_vault_person_nums;
extern DWORD* ptr_bckgnd;
extern DWORD* ptr_black_palette;
extern DWORD* ptr_bottom_line;
extern DWORD* ptr_btable;
extern DWORD* ptr_btncnt;
extern DWORD* ptr_CarCurrArea;
extern DWORD* ptr_cmap;
extern DWORD* ptr_colorTable;
extern DWORD* ptr_combat_free_move;
extern DWORD* ptr_combat_list;
extern DWORD* ptr_combat_state;
extern DWORD* ptr_combat_turn_running;
extern DWORD* ptr_combatNumTurns;
extern DWORD* ptr_crit_succ_eff;
extern DWORD* ptr_critter_db_handle;
extern DWORD* ptr_critterClearObj;
extern DWORD* ptr_crnt_func;
extern DWORD* ptr_curr_font_num;
extern DWORD* ptr_curr_pc_stat;
extern DWORD* ptr_curr_stack;
extern DWORD* ptr_cursor_line;
extern DWORD* ptr_dialog_target;
extern DWORD* ptr_dialog_target_is_party;
extern DWORD* ptr_drugInfoList;
extern DWORD* ptr_edit_win;
extern DWORD* ptr_Educated;
extern DWORD* ptr_Experience_;
extern DWORD* ptr_fallout_game_time;
extern DWORD* ptr_flptr;
extern DWORD* ptr_folder_card_desc;
extern DWORD* ptr_folder_card_fid;
extern DWORD* ptr_folder_card_title;
extern DWORD* ptr_folder_card_title2;
extern DWORD* ptr_frame_time;
extern char*  ptr_free_perk;
extern DWORD* ptr_game_global_vars;
extern DWORD* ptr_game_user_wants_to_quit;
extern DWORD* ptr_gcsd;
extern DWORD* ptr_gdBarterMod;
extern DWORD* ptr_gdNumOptions;
extern DWORD* ptr_gIsSteal;
extern DWORD* ptr_glblmode;
extern DWORD* ptr_gmouse_current_cursor;
extern DWORD* ptr_gmovie_played_list;
extern DWORD* ptr_GreenColor;
extern DWORD* ptr_gsound_initialized;
extern DWORD* ptr_hit_location_penalty;
extern DWORD* ptr_holo_flag;
extern DWORD* ptr_holopages;
extern DWORD* ptr_hot_line_count;
extern DWORD* ptr_i_fid;
extern DWORD* ptr_i_lhand;
extern DWORD* ptr_i_rhand;
extern DWORD* ptr_i_wid;
extern DWORD* ptr_i_worn;
extern DWORD* ptr_In_WorldMap;
extern DWORD* ptr_info_line;
extern DWORD* ptr_interfaceWindow;
extern DWORD* ptr_intfaceEnabled;
extern DWORD* ptr_intotal;
extern TGameObj** ptr_inven_dude;
extern DWORD* ptr_inven_pid;
extern DWORD* ptr_inven_scroll_dn_bid;
extern DWORD* ptr_inven_scroll_up_bid;
extern DWORD* ptr_inventry_message_file;
extern DWORD* ptr_itemButtonItems;
extern DWORD* ptr_itemCurrentItem; // 0 - left, 1 - right
extern DWORD* ptr_kb_lock_flags;
extern DWORD* ptr_last_buttons;
extern DWORD* ptr_last_button_winID;
extern DWORD* ptr_last_level;
extern DWORD* ptr_Level_;
extern DWORD* ptr_Lifegiver;
extern DWORD* ptr_list_com;
extern DWORD* ptr_list_total;
extern DWORD* ptr_loadingGame;
extern DWORD* ptr_LSData;
extern DWORD* ptr_lsgwin;
extern DWORD* ptr_main_ctd;
extern DWORD* ptr_main_window;
extern DWORD* ptr_map_elevation;
extern DWORD* ptr_map_global_vars;
extern DWORD* ptr_master_db_handle;
extern DWORD* ptr_max;
extern DWORD* ptr_maxScriptNum;
extern DWORD* ptr_Meet_Frank_Horrigan;
extern DWORD* ptr_mouse_hotx;
extern DWORD* ptr_mouse_hoty;
extern DWORD* ptr_mouse_is_hidden;
extern DWORD* ptr_mouse_x_;
extern DWORD* ptr_mouse_y;
extern DWORD* ptr_mouse_y_;
extern DWORD* ptr_Mutate_;
extern DWORD* ptr_name_color;
extern DWORD* ptr_name_font;
extern DWORD* ptr_name_sort_list;
extern DWORD* ptr_num_game_global_vars;
extern DWORD* ptr_num_map_global_vars;
extern TGameObj** ptr_obj_dude;
extern DWORD* ptr_objectTable;
extern DWORD* ptr_objItemOutlineState;
extern DWORD* ptr_optionRect;
extern DWORD* ptr_outlined_object;
extern DWORD* ptr_partyMemberAIOptions;
extern DWORD* ptr_partyMemberCount;
extern DWORD* ptr_partyMemberLevelUpInfoList;
extern DWORD* ptr_partyMemberList; // each struct - 4 integers, first integer - objPtr
extern DWORD* ptr_partyMemberMaxCount;
extern DWORD* ptr_partyMemberPidList;
extern DWORD* ptr_patches;
extern DWORD* ptr_paths;
extern DWORD* ptr_pc_crit_succ_eff;
extern DWORD* ptr_pc_kill_counts;
extern char*  ptr_pc_name;
extern DWORD* ptr_pc_proto;
extern DWORD* ptr_perk_data;
extern int**  ptr_perkLevelDataList; // limited to PERK_Count
extern DWORD* ptr_pip_win;
extern DWORD* ptr_pipboy_message_file;
extern DWORD* ptr_pipmesg;
extern DWORD* ptr_preload_list_index;
extern DWORD* ptr_procTableStrs;  // table of procId (from define.h) => procName map
extern DWORD* ptr_proto_main_msg_file;
extern DWORD* ptr_ptable;
extern DWORD* ptr_pud;
extern DWORD* ptr_queue;
extern DWORD* ptr_quick_done;
extern DWORD* ptr_read_callback;
extern DWORD* ptr_RedColor;
extern DWORD* ptr_retvals;
extern DWORD* ptr_scr_size;
extern DWORD* ptr_scriptListInfo;
extern DWORD* ptr_skill_data;
extern DWORD* ptr_slot_cursor;
extern DWORD* ptr_sneak_working; // DWORD var 
extern DWORD* ptr_square;
extern DWORD* ptr_squares;
extern DWORD* ptr_stack;
extern DWORD* ptr_stack_offset;
extern DWORD* ptr_stat_data;
extern DWORD* ptr_stat_flag;
extern DWORD* ptr_Tag_;
extern DWORD* ptr_tag_skill;
extern DWORD* ptr_target_curr_stack;
extern DWORD* ptr_target_pud;
extern DWORD* ptr_target_stack;
extern DWORD* ptr_target_stack_offset;
extern DWORD* ptr_target_str;
extern DWORD* ptr_target_xpos;
extern DWORD* ptr_target_ypos;
extern DWORD* ptr_text_char_width;
extern DWORD* ptr_text_height;
extern DWORD* ptr_text_max;
extern DWORD* ptr_text_mono_width;
extern DWORD* ptr_text_spacing;
extern DWORD* ptr_text_to_buf;
extern DWORD* ptr_text_width;
extern DWORD* ptr_title_color;
extern DWORD* ptr_title_font;
extern DWORD* ptr_trait_data;
extern DWORD* ptr_view_page;
extern DWORD* ptr_wd_obj;
extern DWORD* ptr_wmAreaInfoList;
extern DWORD* ptr_wmLastRndTime;
extern DWORD* ptr_wmWorldOffsetX;
extern DWORD* ptr_wmWorldOffsetY;
extern DWORD* ptr_world_xpos;
extern DWORD* ptr_world_ypos;
extern DWORD* ptr_WorldMapCurrArea;
extern DWORD* ptr_YellowColor;