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

// Global variable pointers

namespace VarPtr
{

// defines const pointer to a variable (pointer is constant, but value can be changed)
#define VARDECL(type, name)	\
	extern type* const name;


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
VARDECL(DWORD, inventry_message_file)
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
VARDECL(DWORD, proto_main_msg_file)
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
