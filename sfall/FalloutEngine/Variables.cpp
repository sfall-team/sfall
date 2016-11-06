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

// Pointers to engine global variables
namespace VarPtr
{

// defines const pointer to a variable (pointer is constant, but value can be changed)
#define VARDEFN(type, name, addr)	\
	type* const name = reinterpret_cast<type*>(addr);


VARDEFN(long, pc_trait, _pc_trait) // 2 of them
VARDEFN(DWORD, aiInfoList, _aiInfoList)
VARDEFN(DWORD, ambient_light, _ambient_light)
VARDEFN(sArt, art, _art)
VARDEFN(DWORD, art_name, _art_name)
VARDEFN(DWORD, art_vault_guy_num, _art_vault_guy_num)
VARDEFN(DWORD, art_vault_person_nums, _art_vault_person_nums)
VARDEFN(DWORD, bckgnd, _bckgnd)
VARDEFN(DWORD, black_palette, _black_palette)
VARDEFN(DWORD, bottom_line, _bottom_line)
VARDEFN(DWORD, btable, _btable)
VARDEFN(DWORD, btncnt, _btncnt)
VARDEFN(DWORD, CarCurrArea, _CarCurrArea)
VARDEFN(DWORD, cmap, _cmap)
VARDEFN(DWORD, colorTable, _colorTable)
VARDEFN(DWORD, combat_free_move, _combat_free_move)
VARDEFN(DWORD, combat_list, _combat_list)
VARDEFN(DWORD, combat_state, _combat_state)
VARDEFN(DWORD, combat_turn_running, _combat_turn_running)
VARDEFN(DWORD, combatNumTurns, _combatNumTurns)
VARDEFN(CritStruct, crit_succ_eff, _crit_succ_eff)  // static array
VARDEFN(DWORD, critter_db_handle, _critter_db_handle)
VARDEFN(DWORD, critterClearObj, _critterClearObj)
VARDEFN(DWORD, crnt_func, _crnt_func)
VARDEFN(DWORD, curr_font_num, _curr_font_num)
VARDEFN(DWORD, curr_pc_stat, _curr_pc_stat)
VARDEFN(DWORD, curr_stack, _curr_stack)
VARDEFN(DWORD, cursor_line, _cursor_line)
VARDEFN(DWORD, dialog_target, _dialog_target)
VARDEFN(DWORD, dialog_target_is_party, _dialog_target_is_party)
VARDEFN(DWORD, drugInfoList, _drugInfoList)
VARDEFN(DWORD, edit_win, _edit_win)
VARDEFN(DWORD, Educated, _Educated)
VARDEFN(DWORD, Experience_, _Experience_)
VARDEFN(DWORD, fallout_game_time, _fallout_game_time)
VARDEFN(DWORD, flptr, _flptr)
VARDEFN(DWORD, folder_card_desc, _folder_card_desc)
VARDEFN(DWORD, folder_card_fid, _folder_card_fid)
VARDEFN(DWORD, folder_card_title, _folder_card_title)
VARDEFN(DWORD, folder_card_title2, _folder_card_title2)
VARDEFN(DWORD, frame_time, _frame_time)
VARDEFN(char, free_perk, _free_perk)
VARDEFN(int*, game_global_vars, _game_global_vars)
VARDEFN(DWORD, game_user_wants_to_quit, _game_user_wants_to_quit)
VARDEFN(DWORD, gcsd, _gcsd)
VARDEFN(DWORD, gdBarterMod, _gdBarterMod)
VARDEFN(DWORD, gdNumOptions, _gdNumOptions)
VARDEFN(DWORD, gIsSteal, _gIsSteal)
VARDEFN(DWORD, glblmode, _glblmode)
VARDEFN(DWORD, gmouse_current_cursor, _gmouse_current_cursor)
VARDEFN(DWORD, gmovie_played_list, _gmovie_played_list)
VARDEFN(BYTE, GreenColor, _GreenColor)
VARDEFN(DWORD, gsound_initialized, _gsound_initialized)
VARDEFN(DWORD, hit_location_penalty, _hit_location_penalty)
VARDEFN(DWORD, holo_flag, _holo_flag)
VARDEFN(DWORD, holopages, _holopages)
VARDEFN(DWORD, hot_line_count, _hot_line_count)
VARDEFN(DWORD, i_fid, _i_fid)
VARDEFN(DWORD, i_lhand, _i_lhand)
VARDEFN(DWORD, i_rhand, _i_rhand)
VARDEFN(DWORD, i_wid, _i_wid)
VARDEFN(DWORD, i_worn, _i_worn)
VARDEFN(DWORD, In_WorldMap, _In_WorldMap)
VARDEFN(DWORD, info_line, _info_line)
VARDEFN(DWORD, interfaceWindow, _interfaceWindow)
VARDEFN(DWORD, intfaceEnabled, _intfaceEnabled)
VARDEFN(DWORD, intotal, _intotal)
VARDEFN(TGameObj*, inven_dude, _inven_dude)
VARDEFN(DWORD, inven_pid, _inven_pid)
VARDEFN(DWORD, inven_scroll_dn_bid, _inven_scroll_dn_bid)
VARDEFN(DWORD, inven_scroll_up_bid, _inven_scroll_up_bid)
VARDEFN(DWORD, inventry_message_file, _inventry_message_file)
VARDEFN(DWORD, itemButtonItems, _itemButtonItems)
VARDEFN(DWORD, itemCurrentItem, _itemCurrentItem) // 0 - left, 1 - right
VARDEFN(DWORD, kb_lock_flags, _kb_lock_flags)
VARDEFN(DWORD, last_buttons, _last_buttons)
VARDEFN(DWORD, last_button_winID, _last_button_winID)
VARDEFN(DWORD, last_level, _last_level)
VARDEFN(DWORD, Level_, _Level_)
VARDEFN(DWORD, Lifegiver, _Lifegiver)
VARDEFN(DWORD, list_com, _list_com)
VARDEFN(DWORD, list_total, _list_total)
VARDEFN(DWORD, loadingGame, _loadingGame)
VARDEFN(DWORD, LSData, _LSData)
VARDEFN(DWORD, lsgwin, _lsgwin)
VARDEFN(DWORD, main_ctd, _main_ctd)
VARDEFN(DWORD, main_window, _main_window)
VARDEFN(DWORD, map_elevation, _map_elevation)
VARDEFN(int*,  map_global_vars, _map_global_vars)
VARDEFN(DWORD, master_db_handle, _master_db_handle)
VARDEFN(DWORD, max, _max)
VARDEFN(DWORD, maxScriptNum, _maxScriptNum)
VARDEFN(DWORD, Meet_Frank_Horrigan, _Meet_Frank_Horrigan)
VARDEFN(DWORD, mouse_hotx, _mouse_hotx)
VARDEFN(DWORD, mouse_hoty, _mouse_hoty)
VARDEFN(DWORD, mouse_is_hidden, _mouse_is_hidden)
VARDEFN(DWORD, mouse_x_, _mouse_x_)
VARDEFN(DWORD, mouse_y, _mouse_y)
VARDEFN(DWORD, mouse_y_, _mouse_y_)
VARDEFN(DWORD, Mutate_, _Mutate_)
VARDEFN(DWORD, name_color, _name_color)
VARDEFN(DWORD, name_font, _name_font)
VARDEFN(DWORD, name_sort_list, _name_sort_list)
VARDEFN(int, num_game_global_vars, _num_game_global_vars)
VARDEFN(int, num_map_global_vars, _num_map_global_vars)
VARDEFN(TGameObj*, obj_dude, _obj_dude)
VARDEFN(DWORD, objectTable, _objectTable)
VARDEFN(DWORD, objItemOutlineState, _objItemOutlineState)
VARDEFN(DWORD, optionRect, _optionRect)
VARDEFN(DWORD, outlined_object, _outlined_object)
VARDEFN(DWORD, partyMemberAIOptions, _partyMemberAIOptions)
VARDEFN(DWORD, partyMemberCount, _partyMemberCount)
VARDEFN(DWORD, partyMemberLevelUpInfoList, _partyMemberLevelUpInfoList)
VARDEFN(DWORD*, partyMemberList, _partyMemberList) // each struct - 4 integers, first integer - objPtr
VARDEFN(DWORD, partyMemberMaxCount, _partyMemberMaxCount)
VARDEFN(DWORD, partyMemberPidList, _partyMemberPidList)
VARDEFN(char*, patches, _patches)
VARDEFN(sPath*, paths, _paths)  // array
VARDEFN(CritStruct, pc_crit_succ_eff, _pc_crit_succ_eff)  // 
VARDEFN(DWORD, pc_kill_counts, _pc_kill_counts)
VARDEFN(char, pc_name, _pc_name)
VARDEFN(DWORD, pc_proto, _pc_proto)
VARDEFN(DWORD, perk_data, _perk_data)
VARDEFN(int*, perkLevelDataList, _perkLevelDataList)
VARDEFN(DWORD, pip_win, _pip_win)
VARDEFN(DWORD, pipboy_message_file, _pipboy_message_file)
VARDEFN(DWORD, pipmesg, _pipmesg)
VARDEFN(DWORD, preload_list_index, _preload_list_index)
VARDEFN(const char*, procTableStrs, _procTableStrs)  // table of procId (from define.h) => procName map
VARDEFN(DWORD, proto_main_msg_file, _proto_main_msg_file)
VARDEFN(DWORD, ptable, _ptable)
VARDEFN(DWORD, pud, _pud)
VARDEFN(DWORD, queue, _queue)
VARDEFN(DWORD, quick_done, _quick_done)
VARDEFN(DWORD, read_callback, _read_callback)
VARDEFN(BYTE, RedColor, _RedColor)
VARDEFN(DWORD, retvals, _retvals)
VARDEFN(DWORD, scr_size, _scr_size)
VARDEFN(DWORD, scriptListInfo, _scriptListInfo)
VARDEFN(SkillInfo, skill_data, _skill_data)
VARDEFN(DWORD, slot_cursor, _slot_cursor)
VARDEFN(DWORD, sneak_working, _sneak_working) // DWORD var 
VARDEFN(DWORD, square, _square)
VARDEFN(DWORD*, squares, _squares)
VARDEFN(DWORD, stack, _stack)
VARDEFN(DWORD, stack_offset, _stack_offset)
VARDEFN(DWORD, stat_data, _stat_data)
VARDEFN(DWORD, stat_flag, _stat_flag)
VARDEFN(DWORD, Tag_, _Tag_)
VARDEFN(DWORD, tag_skill, _tag_skill)
VARDEFN(DWORD, target_curr_stack, _target_curr_stack)
VARDEFN(DWORD, target_pud, _target_pud)
VARDEFN(DWORD, target_stack, _target_stack)
VARDEFN(DWORD, target_stack_offset, _target_stack_offset)
VARDEFN(DWORD, target_str, _target_str)
VARDEFN(DWORD, target_xpos, _target_xpos)
VARDEFN(DWORD, target_ypos, _target_ypos)
VARDEFN(DWORD, text_char_width, _text_char_width)
VARDEFN(DWORD, text_height, _text_height)
VARDEFN(DWORD, text_max, _text_max)
VARDEFN(DWORD, text_mono_width, _text_mono_width)
VARDEFN(DWORD, text_spacing, _text_spacing)
VARDEFN(DWORD, text_to_buf, _text_to_buf)
VARDEFN(DWORD, text_width, _text_width)
VARDEFN(DWORD, title_color, _title_color)
VARDEFN(DWORD, title_font, _title_font)
VARDEFN(DWORD, trait_data, _trait_data)
VARDEFN(DWORD, view_page, _view_page)
VARDEFN(DWORD, wd_obj, _wd_obj)
VARDEFN(DWORD, wmAreaInfoList, _wmAreaInfoList)
VARDEFN(DWORD, wmLastRndTime, _wmLastRndTime)
VARDEFN(DWORD, wmWorldOffsetX, _wmWorldOffsetX)
VARDEFN(DWORD, wmWorldOffsetY, _wmWorldOffsetY)
VARDEFN(DWORD, world_xpos, _world_xpos)
VARDEFN(DWORD, world_ypos, _world_ypos)
VARDEFN(DWORD, WorldMapCurrArea, _WorldMapCurrArea)
VARDEFN(BYTE, YellowColor, _YellowColor)

}
