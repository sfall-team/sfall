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
#define BYTE unsigned char

// Global variable constants

// PLEASE USE THOSE IN ASM BLOCKS!
#define VARPTR_aiInfoList                 0x510948
#define VARPTR_ambient_light              0x51923C
#define VARPTR_art                        0x510738
#define VARPTR_art_name                   0x56C9E4
#define VARPTR_art_vault_guy_num          0x5108A4
#define VARPTR_art_vault_person_nums      0x5108A8
#define VARPTR_bckgnd                     0x5707A4
#define VARPTR_black_palette              0x663FD0
#define VARPTR_bottom_line                0x664524
#define VARPTR_btable                     0x59E944
#define VARPTR_btncnt                     0x43EA1C
#define VARPTR_CarCurrArea                0x672E68
#define VARPTR_cmap                       0x51DF34
#define VARPTR_colorTable                 0x6A38D0
#define VARPTR_combat_free_move           0x56D39C
#define VARPTR_combat_list                0x56D390
#define VARPTR_combat_state               0x510944
#define VARPTR_combat_turn_running        0x51093C
#define VARPTR_combatNumTurns             0x510940
#define VARPTR_crit_succ_eff              0x510978
#define VARPTR_critter_db_handle          0x58E94C
#define VARPTR_critterClearObj            0x518438
#define VARPTR_crnt_func                  0x664508
#define VARPTR_curr_font_num              0x51E3B0
#define VARPTR_curr_pc_stat               0x6681AC
#define VARPTR_curr_stack                 0x59E96C
#define VARPTR_cursor_line                0x664514
#define VARPTR_dialog_target              0x518848
#define VARPTR_dialog_target_is_party     0x51884C
#define VARPTR_drugInfoList               0x5191CC
#define VARPTR_edit_win                   0x57060C
#define VARPTR_Educated                   0x57082C
#define VARPTR_Experience_                0x6681B4
#define VARPTR_fallout_game_time          0x51C720
#define VARPTR_flptr                      0x614808
#define VARPTR_folder_card_desc           0x5705CC
#define VARPTR_folder_card_fid            0x5705B0
#define VARPTR_folder_card_title          0x5705B8
#define VARPTR_folder_card_title2         0x5705BC
#define VARPTR_frame_time                 0x5709C4
#define VARPTR_free_perk                  0x570A29
#define VARPTR_game_global_vars           0x5186C0
#define VARPTR_game_user_wants_to_quit    0x5186CC
#define VARPTR_gcsd                       0x51094C
#define VARPTR_gdBarterMod                0x51873C
#define VARPTR_gdNumOptions               0x5186D8
#define VARPTR_gIsSteal                   0x51D430
#define VARPTR_glblmode                   0x5709D0
#define VARPTR_gmouse_current_cursor      0x518C0C
#define VARPTR_gmovie_played_list         0x596C78
#define VARPTR_GreenColor                 0x6A3CB0
#define VARPTR_gsound_initialized         0x518E30
#define VARPTR_hit_location_penalty       0x510954
#define VARPTR_holo_flag                  0x664529
#define VARPTR_holopages                  0x66445C
#define VARPTR_hot_line_count             0x6644F8
#define VARPTR_i_fid                      0x59E95C
#define VARPTR_i_lhand                    0x59E958
#define VARPTR_i_rhand                    0x59E968
#define VARPTR_i_wid                      0x59E964
#define VARPTR_i_worn                     0x59E954
#define VARPTR_In_WorldMap                0x672E1C
#define VARPTR_info_line                  0x5707D0
#define VARPTR_interfaceWindow            0x519024
#define VARPTR_intfaceEnabled             0x518F10
#define VARPTR_intotal                    0x43E95C
#define VARPTR_inven_dude                 0x519058
#define VARPTR_inven_pid                  0x51905C
#define VARPTR_inven_scroll_dn_bid        0x5190E8
#define VARPTR_inven_scroll_up_bid        0x5190E4
#define VARPTR_inventry_message_file      0x59E814
#define VARPTR_itemButtonItems            0x5970F8
#define VARPTR_itemCurrentItem            0x518F78
#define VARPTR_kb_lock_flags              0x51E2EA
#define VARPTR_last_buttons               0x51E2AC
#define VARPTR_last_button_winID          0x51E404
#define VARPTR_last_level                 0x5707B4
#define VARPTR_Level_                     0x6681B0
#define VARPTR_Lifegiver                  0x570854
#define VARPTR_list_com                   0x56D394
#define VARPTR_list_total                 0x56D37C
#define VARPTR_loadingGame                0x5194C4
#define VARPTR_LSData                     0x613D30
#define VARPTR_lsgwin                     0x6142C4
#define VARPTR_main_ctd                   0x56D2B0
#define VARPTR_main_window                0x5194F0
#define VARPTR_map_elevation              0x519578
#define VARPTR_map_global_vars            0x51956C
#define VARPTR_master_db_handle           0x58E948
#define VARPTR_max                        0x56FB50
#define VARPTR_maxScriptNum               0x51C7CC
#define VARPTR_Meet_Frank_Horrigan        0x672E04
#define VARPTR_mouse_hotx                 0x6AC7D0
#define VARPTR_mouse_hoty                 0x6AC7CC
#define VARPTR_mouse_is_hidden            0x6AC790
#define VARPTR_mouse_x_                   0x6AC7A8
#define VARPTR_mouse_y                    0x664450
#define VARPTR_mouse_y_                   0x6AC7A4
#define VARPTR_Mutate_                    0x5708B4
#define VARPTR_name_color                 0x56D744
#define VARPTR_name_font                  0x56D74C
#define VARPTR_name_sort_list             0x56FCB0
#define VARPTR_num_game_global_vars       0x5186C4
#define VARPTR_num_map_global_vars        0x519574
#define VARPTR_obj_dude                   0x6610B8
#define VARPTR_objectTable                0x639DA0
#define VARPTR_objItemOutlineState        0x519798
#define VARPTR_optionRect                 0x58ECC0
#define VARPTR_outlined_object            0x518D94
#define VARPTR_partyMemberAIOptions       0x519DB8
#define VARPTR_partyMemberCount           0x519DAC
#define VARPTR_partyMemberLevelUpInfoList 0x519DBC
#define VARPTR_partyMemberList            0x519DA8 // each struct - 4 integers, first integer - objPtr
#define VARPTR_partyMemberMaxCount        0x519D9C
#define VARPTR_partyMemberPidList         0x519DA0
#define VARPTR_patches                    0x5193CC
#define VARPTR_paths                      0x6B24D0
#define VARPTR_pc_crit_succ_eff           0x5179B0
#define VARPTR_pc_kill_counts             0x56D780
#define VARPTR_pc_name                    0x56D75C
#define VARPTR_pc_proto                   0x51C370
#define VARPTR_pc_trait                   0x66BE40
#define VARPTR_pc_trait2                  0x66BE44
#define VARPTR_perk_data                  0x519DCC
#define VARPTR_perkLevelDataList          0x51C120
#define VARPTR_pip_win                    0x6644C4
#define VARPTR_pipboy_message_file        0x664348
#define VARPTR_pipmesg                    0x664338
#define VARPTR_preload_list_index         0x519640
#define VARPTR_procTableStrs              0x51C758  // table of procId (from define.h) => procName map
#define VARPTR_proto_msg_files            0x6647AC
#define VARPTR_proto_main_msg_file        0x6647FC
#define VARPTR_ptable                     0x59E934
#define VARPTR_pud                        0x59E960
#define VARPTR_queue                      0x6648C0
#define VARPTR_quick_done                 0x5193BC
#define VARPTR_read_callback              0x51DEEC
#define VARPTR_RedColor                   0x6AB4D0
#define VARPTR_retvals                    0x43EA7C
#define VARPTR_scr_size                   0x6AC9F0
#define VARPTR_scriptListInfo             0x51C7C8
#define VARPTR_skill_data                 0x51D118
#define VARPTR_slot_cursor                0x5193B8
#define VARPTR_sneak_working              0x56D77C // DWORD var 
#define VARPTR_square                     0x631E40
#define VARPTR_squares                    0x66BE08
#define VARPTR_stack                      0x59E86C
#define VARPTR_stack_offset               0x59E844
#define VARPTR_stat_data                  0x51D53C
#define VARPTR_stat_flag                  0x66452A
#define VARPTR_Tag_                       0x5708B0
#define VARPTR_tag_skill                  0x668070
#define VARPTR_target_curr_stack          0x59E948
#define VARPTR_target_pud                 0x59E978
#define VARPTR_target_stack               0x59E81C
#define VARPTR_target_stack_offset        0x59E7EC
#define VARPTR_target_str                 0x56D518
#define VARPTR_target_xpos                0x672E20
#define VARPTR_target_ypos                0x672E24
#define VARPTR_text_char_width            0x51E3C4
#define VARPTR_text_height                0x51E3BC
#define VARPTR_text_max                   0x51E3D4
#define VARPTR_text_mono_width            0x51E3C8
#define VARPTR_text_spacing               0x51E3CC
#define VARPTR_text_to_buf                0x51E3B8
#define VARPTR_text_width                 0x51E3C0
#define VARPTR_title_color                0x56D750
#define VARPTR_title_font                 0x56D748
#define VARPTR_trait_data                 0x51DB84
#define VARPTR_view_page                  0x664520
#define VARPTR_wd_obj                     0x59E98C
#define VARPTR_wmAreaInfoList             0x51DDF8
#define VARPTR_wmLastRndTime              0x51DEA0
#define VARPTR_wmWorldOffsetX             0x51DE2C
#define VARPTR_wmWorldOffsetY             0x51DE30
#define VARPTR_world_xpos                 0x672E0C
#define VARPTR_world_ypos                 0x672E10
#define VARPTR_WorldMapCurrArea           0x672E08
#define VARPTR_YellowColor                0x6AB8BB


//
// Global variable pointers.
// 
// Use those in normal C++ code.
//
namespace VarPtr
{

// defines const pointer to a variable (pointer is constant, but value can be changed)
#define VARDECL(type, name)	\
	extern type* const name;

// TODO: assign appropriate types (arrays, structs, strings, etc.) for all variables

VARDECL(long, pc_trait) // 2 of them
VARDECL(DWORD, aiInfoList)
VARDECL(DWORD, ambient_light)
VARDECL(sArt, art)	// array of structs
VARDECL(DWORD, art_name)
VARDECL(DWORD, art_vault_guy_num)
VARDECL(DWORD, art_vault_person_nums)
VARDECL(DWORD, bckgnd)
VARDECL(DWORD, black_palette)
VARDECL(DWORD, bottom_line)
VARDECL(DWORD, btable)
VARDECL(DWORD, btncnt)
VARDECL(DWORD, CarCurrArea)
VARDECL(DWORD, cmap)
VARDECL(DWORD, colorTable)
VARDECL(DWORD, combat_free_move)
VARDECL(DWORD, combat_list)
VARDECL(DWORD, combat_state)
VARDECL(DWORD, combat_turn_running)
VARDECL(DWORD, combatNumTurns)
VARDECL(CritStruct, crit_succ_eff) // static array
VARDECL(DWORD, critter_db_handle)
VARDECL(DWORD, critterClearObj)
VARDECL(DWORD, crnt_func)
VARDECL(DWORD, curr_font_num)
VARDECL(DWORD, curr_pc_stat)
VARDECL(DWORD, curr_stack)
VARDECL(DWORD, cursor_line)
VARDECL(DWORD, dialog_target)
VARDECL(DWORD, dialog_target_is_party)
VARDECL(DWORD, drugInfoList)
VARDECL(DWORD, edit_win)
VARDECL(DWORD, Educated)
VARDECL(DWORD, Experience_)
VARDECL(DWORD, fallout_game_time)
VARDECL(DWORD, flptr)
VARDECL(DWORD, folder_card_desc)
VARDECL(DWORD, folder_card_fid)
VARDECL(DWORD, folder_card_title)
VARDECL(DWORD, folder_card_title2)
VARDECL(DWORD, frame_time)
VARDECL(char, free_perk)
VARDECL(int*, game_global_vars) // array
VARDECL(DWORD, game_user_wants_to_quit)
VARDECL(DWORD, gcsd)
VARDECL(DWORD, gdBarterMod)
VARDECL(DWORD, gdNumOptions)
VARDECL(DWORD, gIsSteal)
VARDECL(DWORD, glblmode)
VARDECL(DWORD, gmouse_current_cursor)
VARDECL(DWORD, gmovie_played_list)
VARDECL(BYTE, GreenColor)
VARDECL(DWORD, gsound_initialized)
VARDECL(DWORD, hit_location_penalty)
VARDECL(DWORD, holo_flag)
VARDECL(DWORD, holopages)
VARDECL(DWORD, hot_line_count)
VARDECL(DWORD, i_fid)
VARDECL(DWORD, i_lhand)
VARDECL(DWORD, i_rhand)
VARDECL(DWORD, i_wid)
VARDECL(DWORD, i_worn)
VARDECL(DWORD, In_WorldMap)
VARDECL(DWORD, info_line)
VARDECL(DWORD, interfaceWindow)
VARDECL(DWORD, intfaceEnabled)
VARDECL(DWORD, intotal)
VARDECL(TGameObj*, inven_dude)
VARDECL(DWORD, inven_pid)
VARDECL(DWORD, inven_scroll_dn_bid)
VARDECL(DWORD, inven_scroll_up_bid)
VARDECL(MessageList, inventry_message_file)
VARDECL(DWORD, itemButtonItems)
VARDECL(DWORD, itemCurrentItem) // 0 - left, 1 - right
VARDECL(DWORD, kb_lock_flags)
VARDECL(DWORD, last_buttons)
VARDECL(DWORD, last_button_winID)
VARDECL(DWORD, last_level)
VARDECL(DWORD, Level_)
VARDECL(DWORD, Lifegiver)
VARDECL(DWORD, list_com)
VARDECL(DWORD, list_total)
VARDECL(DWORD, loadingGame)
VARDECL(DWORD, LSData)
VARDECL(DWORD, lsgwin)
VARDECL(DWORD, main_ctd)
VARDECL(DWORD, main_window)
VARDECL(DWORD, map_elevation)
VARDECL(int*, map_global_vars)
VARDECL(DWORD, master_db_handle)
VARDECL(DWORD, max)
VARDECL(DWORD, maxScriptNum)
VARDECL(DWORD, Meet_Frank_Horrigan)
VARDECL(DWORD, mouse_hotx)
VARDECL(DWORD, mouse_hoty)
VARDECL(DWORD, mouse_is_hidden)
VARDECL(DWORD, mouse_x_)
VARDECL(DWORD, mouse_y)
VARDECL(DWORD, mouse_y_)
VARDECL(DWORD, Mutate_)
VARDECL(DWORD, name_color)
VARDECL(DWORD, name_font)
VARDECL(DWORD, name_sort_list)
VARDECL(int, num_game_global_vars)
VARDECL(int, num_map_global_vars)
VARDECL(TGameObj*, obj_dude)
VARDECL(DWORD, objectTable)
VARDECL(DWORD, objItemOutlineState)
VARDECL(DWORD, optionRect)
VARDECL(DWORD, outlined_object)
VARDECL(DWORD, partyMemberAIOptions)
VARDECL(DWORD, partyMemberCount)
VARDECL(DWORD, partyMemberLevelUpInfoList)
VARDECL(DWORD*, partyMemberList) // each struct - 4 integers, first integer - objPtr
VARDECL(DWORD, partyMemberMaxCount)
VARDECL(DWORD, partyMemberPidList)
VARDECL(char*, patches)
VARDECL(sPath*, paths)  // array
VARDECL(CritStruct, pc_crit_succ_eff)  // static array
VARDECL(DWORD, pc_kill_counts)
VARDECL(char, pc_name)
VARDECL(DWORD, pc_proto)
VARDECL(DWORD, perk_data)
VARDECL(int*, perkLevelDataList) // limited to PERK_Count
VARDECL(DWORD, pip_win)
VARDECL(DWORD, pipboy_message_file)
VARDECL(DWORD, pipmesg)
VARDECL(DWORD, preload_list_index)
VARDECL(const char*, procTableStrs)  // table of procId (from define.h) => procName map
VARDECL(MessageList, proto_main_msg_file)
VARDECL(MessageList, proto_msg_files) // array of 6 elements
VARDECL(DWORD, ptable)
VARDECL(DWORD, pud)
VARDECL(DWORD, queue)
VARDECL(DWORD, quick_done)
VARDECL(DWORD, read_callback)
VARDECL(BYTE, RedColor)
VARDECL(DWORD, retvals)
VARDECL(DWORD, scr_size)
VARDECL(DWORD, scriptListInfo)
VARDECL(SkillInfo, skill_data)
VARDECL(DWORD, slot_cursor)
VARDECL(DWORD, sneak_working) // DWORD var 
VARDECL(DWORD, square)
VARDECL(DWORD*, squares)
VARDECL(DWORD, stack)
VARDECL(DWORD, stack_offset)
VARDECL(DWORD, stat_data)
VARDECL(DWORD, stat_flag)
VARDECL(DWORD, Tag_)
VARDECL(DWORD, tag_skill)
VARDECL(DWORD, target_curr_stack)
VARDECL(DWORD, target_pud)
VARDECL(DWORD, target_stack)
VARDECL(DWORD, target_stack_offset)
VARDECL(DWORD, target_str)
VARDECL(DWORD, target_xpos)
VARDECL(DWORD, target_ypos)
VARDECL(DWORD, text_char_width)
VARDECL(DWORD, text_height)
VARDECL(DWORD, text_max)
VARDECL(DWORD, text_mono_width)
VARDECL(DWORD, text_spacing)
VARDECL(DWORD, text_to_buf)
VARDECL(DWORD, text_width)
VARDECL(DWORD, title_color)
VARDECL(DWORD, title_font)
VARDECL(DWORD, trait_data)
VARDECL(DWORD, view_page)
VARDECL(DWORD, wd_obj)
VARDECL(DWORD, wmAreaInfoList)
VARDECL(DWORD, wmLastRndTime)
VARDECL(DWORD, wmWorldOffsetX)
VARDECL(DWORD, wmWorldOffsetY)
VARDECL(DWORD, world_xpos)
VARDECL(DWORD, world_ypos)
VARDECL(DWORD, WorldMapCurrArea)
VARDECL(BYTE, YellowColor)

#undef VARDECL

}
