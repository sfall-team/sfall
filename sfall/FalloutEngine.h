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
 * FALLOUT2.EXE structs, function offsets and wrappers should be placed here
 *
 * only place functions and variables here which are likely to be used in more than one module
 *
 */

#include <stdint.h>

#include "Define.h"
#include "FalloutStructs.h"

// Global variable offsets
// TODO: probably need to hide these by moving inside implementation file

#define FO_VAR_aiInfoList                 0x510948
#define FO_VAR_ambient_light              0x51923C
#define FO_VAR_anim_set                   0x54CC14
#define FO_VAR_anon_alias                 0x56CAEC
#define FO_VAR_art                        0x510738
#define FO_VAR_art_name                   0x56C9E4
#define FO_VAR_art_vault_guy_num          0x5108A4
#define FO_VAR_art_vault_person_nums      0x5108A8
#define FO_VAR_background_volume          0x518E88
#define FO_VAR_bboxslot                   0x5970E0
#define FO_VAR_bckgnd                     0x5707A4
#define FO_VAR_bk_disabled                0x6AC780
#define FO_VAR_black_palette              0x663FD0
#define FO_VAR_bottom_line                0x664524
#define FO_VAR_btable                     0x59E944
#define FO_VAR_btncnt                     0x43EA1C
#define FO_VAR_carCurrentArea             0x672E68
#define FO_VAR_carGasAmount               0x672E6C
#define FO_VAR_card_old_fid1              0x5709EC
#define FO_VAR_character_points           0x518538
#define FO_VAR_cmap                       0x51DF34
#define FO_VAR_colorTable                 0x6A38D0
#define FO_VAR_combat_end_due_to_load     0x517F98
#define FO_VAR_combat_free_move           0x56D39C
#define FO_VAR_combat_list                0x56D390
#define FO_VAR_combat_state               0x510944
#define FO_VAR_combat_turn_running        0x51093C
#define FO_VAR_combatNumTurns             0x510940
#define FO_VAR_crit_succ_eff              0x510978
#define FO_VAR_critter_db_handle          0x58E94C
#define FO_VAR_critterClearObj            0x518438
#define FO_VAR_crnt_func                  0x664508
#define FO_VAR_cur_id                     0x51C7D4
#define FO_VAR_curr_font_num              0x51E3B0
#define FO_VAR_curr_pc_stat               0x6681AC
#define FO_VAR_curr_stack                 0x59E96C
#define FO_VAR_currentProgram             0x59E78C
#define FO_VAR_currentWindow              0x51DCB8
#define FO_VAR_cursor_line                0x664514
#define FO_VAR_debug_func                 0x51DF04
#define FO_VAR_dialog_red_button_down_buf 0x58F4A4
#define FO_VAR_dialog_red_button_down_key 0x58F4BC
#define FO_VAR_dialog_red_button_up_buf   0x58F4AC
#define FO_VAR_dialog_red_button_up_key   0x58F46C
#define FO_VAR_dialog_target              0x518848
#define FO_VAR_dialog_target_is_party     0x51884C
#define FO_VAR_dialogue_head              0x518850
#define FO_VAR_dialogue_state             0x518714
#define FO_VAR_dialogue_switch_mode       0x518718
#define FO_VAR_dialogueBackWindow         0x518740
#define FO_VAR_dialogueWindow             0x518744
#define FO_VAR_display_win                0x631E4C
#define FO_VAR_displayMapList             0x41B560
#define FO_VAR_doing_refresh_all          0x6ADF38
#define FO_VAR_dropped_explosive          0x5190E0
#define FO_VAR_drugInfoList               0x5191CC
#define FO_VAR_edit_win                   0x57060C
#define FO_VAR_Educated                   0x57082C
#define FO_VAR_elevation                  0x631D2C
#define FO_VAR_endgame_subtitle_done      0x570BD0
#define FO_VAR_endgame_subtitle_characters 0x51866C
#define FO_VAR_endgame_voiceover_loaded   0x570AB8
#define FO_VAR_Experience_                0x6681B4
#define FO_VAR_fallout_game_time          0x51C720
#define FO_VAR_fidgetFID                  0x5186F4
#define FO_VAR_fidgetFp                   0x5186FC
#define FO_VAR_flptr                      0x614808
#define FO_VAR_folder_card_desc           0x5705CC
#define FO_VAR_folder_card_fid            0x5705B0
#define FO_VAR_folder_card_title          0x5705B8
#define FO_VAR_folder_card_title2         0x5705BC
#define FO_VAR_frame_time                 0x5709C4
#define FO_VAR_free_perk                  0x570A29
#define FO_VAR_freePtr                    0x519594
#define FO_VAR_frstc_draw1                0x5707D8
#define FO_VAR_game_config                0x58E950
#define FO_VAR_game_global_vars           0x5186C0
#define FO_VAR_game_ui_disabled           0x5186B4
#define FO_VAR_game_user_wants_to_quit    0x5186CC
#define FO_VAR_gconfig_file_name          0x58E978
#define FO_VAR_gcsd                       0x51094C
#define FO_VAR_gdBarterMod                0x51873C
#define FO_VAR_gdialog_speech_playing     0x518710
#define FO_VAR_gDialogMusicVol            0x5187D8
#define FO_VAR_gdNumOptions               0x5186D8
#define FO_VAR_gIsSteal                   0x51D430
#define FO_VAR_glblmode                   0x5709D0
#define FO_VAR_gmouse_3d_current_mode     0x518D38
#define FO_VAR_gmouse_current_cursor      0x518C0C
#define FO_VAR_gmovie_played_list         0x596C78
#define FO_VAR_GNW_win_init_flag          0x51E3E0
#define FO_VAR_GNW95_isActive             0x51E444
#define FO_VAR_GNWWin                     0x5195B8
#define FO_VAR_gsound_initialized         0x518E30
#define FO_VAR_gsound_speech_tag          0x518E54
#define FO_VAR_hit_location_penalty       0x510954
#define FO_VAR_holo_flag                  0x664529
#define FO_VAR_holodisk                   0x6644F4
#define FO_VAR_holopages                  0x66445C
#define FO_VAR_hot_line_count             0x6644F8
#define FO_VAR_i_fid                      0x59E95C
#define FO_VAR_i_lhand                    0x59E958
#define FO_VAR_i_rhand                    0x59E968
#define FO_VAR_i_wid                      0x59E964
#define FO_VAR_i_worn                     0x59E954
#define FO_VAR_idle_func                  0x51E234
#define FO_VAR_In_WorldMap                0x672E1C
#define FO_VAR_info_line                  0x5707D0
#define FO_VAR_interfaceWindow            0x519024
#define FO_VAR_intfaceEnabled             0x518F10
#define FO_VAR_intotal                    0x43E95C
#define FO_VAR_inven_dude                 0x519058
#define FO_VAR_inven_pid                  0x51905C
#define FO_VAR_inven_scroll_dn_bid        0x5190E8
#define FO_VAR_inven_scroll_up_bid        0x5190E4
#define FO_VAR_inventry_message_file      0x59E814
#define FO_VAR_itemButtonItems            0x5970F8
#define FO_VAR_itemCurrentItem            0x518F78
#define FO_VAR_kb_lock_flags              0x51E2EA
#define FO_VAR_last_buttons               0x51E2AC
#define FO_VAR_last_button_winID          0x51E404
#define FO_VAR_last_level                 0x5707B4
#define FO_VAR_lastMovieH                 0x638E64
#define FO_VAR_lastMovieW                 0x638E68
#define FO_VAR_Level_                     0x6681B0
#define FO_VAR_Lifegiver                  0x570854
#define FO_VAR_lips_draw_head             0x519248
#define FO_VAR_lipsFID                    0x518704
#define FO_VAR_list_com                   0x56D394
#define FO_VAR_list_total                 0x56D37C
#define FO_VAR_loadingGame                0x5194C4
#define FO_VAR_LSData                     0x613D30
#define FO_VAR_lsgwin                     0x6142C4
#define FO_VAR_main_ctd                   0x56D2B0
#define FO_VAR_main_death_voiceover_done  0x614838
#define FO_VAR_main_window                0x5194F0
#define FO_VAR_map_elevation              0x519578
#define FO_VAR_map_global_vars            0x51956C
#define FO_VAR_map_number                 0x631D88
#define FO_VAR_map_state                  0x631D28
#define FO_VAR_mapEntranceElevation       0x519558
#define FO_VAR_mapEntranceTileNum         0x51955C
#define FO_VAR_master_db_handle           0x58E948
#define FO_VAR_master_volume              0x518E84
#define FO_VAR_max                        0x56FB50
#define FO_VAR_maxScriptNum               0x51C7CC
#define FO_VAR_Meet_Frank_Horrigan        0x672E04
#define FO_VAR_Move_on_Car                0x672E64
#define FO_VAR_mouse_buttons              0x6AC7B0
#define FO_VAR_mouse_hotx                 0x6AC7D0
#define FO_VAR_mouse_hoty                 0x6AC7CC
#define FO_VAR_mouse_is_hidden            0x6AC790
#define FO_VAR_mouse_x_                   0x6AC7A8
#define FO_VAR_mouse_y                    0x664450
#define FO_VAR_mouse_y_                   0x6AC7A4
#define FO_VAR_movie_list                 0x518DA0
#define FO_VAR_Mutate_                    0x5708B4
#define FO_VAR_name_color                 0x56D744
#define FO_VAR_name_font                  0x56D74C
#define FO_VAR_name_sort_list             0x56FCB0
#define FO_VAR_num_game_global_vars       0x5186C4
#define FO_VAR_num_map_global_vars        0x519574
#define FO_VAR_num_windows                0x6ADF24
#define FO_VAR_obj_dude                   0x6610B8
#define FO_VAR_obj_seen                   0x662445
#define FO_VAR_objectTable                0x639DA0
#define FO_VAR_objItemOutlineState        0x519798
#define FO_VAR_optionRect                 0x58ECC0
#define FO_VAR_optionsButtonDown          0x59D400
#define FO_VAR_optionsButtonDown1         0x570518
#define FO_VAR_optionsButtonDownKey       0x518F2C
#define FO_VAR_optionsButtonUp            0x59D3FC
#define FO_VAR_optionsButtonUp1           0x570514
#define FO_VAR_optionsButtonUpKey         0x518F28
#define FO_VAR_optnwin                    0x663900
#define FO_VAR_outlined_object            0x518D94
#define FO_VAR_pal                        0x56D7E0
#define FO_VAR_partyMemberAIOptions       0x519DB8
#define FO_VAR_partyMemberCount           0x519DAC
#define FO_VAR_partyMemberLevelUpInfoList 0x519DBC
#define FO_VAR_partyMemberList            0x519DA8 // each struct - 4 integers, first integer - objPtr
#define FO_VAR_partyMemberMaxCount        0x519D9C
#define FO_VAR_partyMemberPidList         0x519DA0
#define FO_VAR_patches                    0x5193CC
#define FO_VAR_paths                      0x6B24D0
#define FO_VAR_pc_crit_succ_eff           0x5179B0
#define FO_VAR_pc_kill_counts             0x56D780
#define FO_VAR_pc_name                    0x56D75C
#define FO_VAR_pc_proto                   0x51C370
#define FO_VAR_pc_trait                   0x66BE40
#define FO_VAR_pc_trait2                  0x66BE44
#define FO_VAR_perk_data                  0x519DCC
#define FO_VAR_perkLevelDataList          0x51C120
#define FO_VAR_pip_win                    0x6644C4
#define FO_VAR_pipboy_message_file        0x664348
#define FO_VAR_pipmesg                    0x664338
#define FO_VAR_preload_list_index         0x519640
#define FO_VAR_procTableStrs              0x51C758  // table of procId (from define.h) => procName map
#define FO_VAR_proto_main_msg_file        0x6647FC
#define FO_VAR_proto_msg_files            0x6647AC
#define FO_VAR_ptable                     0x59E934
#define FO_VAR_pud                        0x59E960
#define FO_VAR_quest_count                0x51C12C
#define FO_VAR_queue                      0x6648C0
#define FO_VAR_quick_done                 0x5193BC
#define FO_VAR_read_callback              0x51DEEC
#define FO_VAR_rectList                   0x51DEF4
#define FO_VAR_retvals                    0x43EA7C
#define FO_VAR_rm_FrameCount              0x6B36A8
#define FO_VAR_rotation                   0x631D34
#define FO_VAR_sad                        0x530014
#define FO_VAR_sampleRate                 0x66815C
#define FO_VAR_scr_blit                   0x6ACA18
#define FO_VAR_scr_size                   0x6AC9F0
#define FO_VAR_screen_buffer              0x51E3FC
#define FO_VAR_script_engine_running      0x51C714
#define FO_VAR_scriptListInfo             0x51C7C8
#define FO_VAR_skill_data                 0x51D118
#define FO_VAR_skldxwin                   0x668140
#define FO_VAR_slot_cursor                0x5193B8
#define FO_VAR_sndfx_volume               0x518E90
#define FO_VAR_sneak_working              0x56D77C // DWORD var
#define FO_VAR_sound_music_path1          0x518E78
#define FO_VAR_sound_music_path2          0x518E7C
#define FO_VAR_speech_volume              0x518E8C
#define FO_VAR_square                     0x631E40
#define FO_VAR_squares                    0x66BE08
#define FO_VAR_stack                      0x59E86C
#define FO_VAR_stack_offset               0x59E844
#define FO_VAR_stat_data                  0x51D53C
#define FO_VAR_stat_flag                  0x66452A
#define FO_VAR_subtitleList               0x638E74
#define FO_VAR_subtitles                  0x663974
#define FO_VAR_sWindows                   0x6727B0
#define FO_VAR_Tag_                       0x5708B0
#define FO_VAR_tag_skill                  0x668070
#define FO_VAR_target_curr_stack          0x59E948
#define FO_VAR_target_pud                 0x59E978
#define FO_VAR_target_stack               0x59E81C
#define FO_VAR_target_stack_offset        0x59E7EC
#define FO_VAR_target_str                 0x56D518
#define FO_VAR_target_xpos                0x672E20
#define FO_VAR_target_ypos                0x672E24
#define FO_VAR_text_char_width            0x51E3C4
#define FO_VAR_text_height                0x51E3BC
#define FO_VAR_text_max                   0x51E3D4
#define FO_VAR_text_mono_width            0x51E3C8
#define FO_VAR_text_object_index          0x51D944
#define FO_VAR_text_object_list           0x6681C0
#define FO_VAR_text_spacing               0x51E3CC
#define FO_VAR_text_to_buf                0x51E3B8
#define FO_VAR_text_width                 0x51E3C0
#define FO_VAR_tile                       0x631D30
#define FO_VAR_title_color                0x56D750
#define FO_VAR_title_font                 0x56D748
#define FO_VAR_trait_data                 0x51DB84
#define FO_VAR_view_page                  0x664520
#define FO_VAR_wd_obj                     0x59E98C
#define FO_VAR_window                     0x6ADE58
#define FO_VAR_window_index               0x6ADD90
#define FO_VAR_wmAreaInfoList             0x51DDF8
#define FO_VAR_wmBkWin                    0x51DE14
#define FO_VAR_wmBkWinBuf                 0x51DE24
#define FO_VAR_wmEncounterIconShow        0x672E48
#define FO_VAR_wmLastRndTime              0x51DEA0
#define FO_VAR_wmMaxMapNum                0x51DE10
#define FO_VAR_wmMsgFile                  0x672FB0
#define FO_VAR_wmNumHorizontalTiles       0x51DDF4
#define FO_VAR_wmRndCursorFid             0x672E58
#define FO_VAR_wmWorldOffsetX             0x51DE2C
#define FO_VAR_wmWorldOffsetY             0x51DE30
#define FO_VAR_wmYesNoStrs                0x51DD90
#define FO_VAR_world_subtile              0x672E14
#define FO_VAR_world_xpos                 0x672E0C
#define FO_VAR_world_ypos                 0x672E10
#define FO_VAR_WorldMapCurrArea           0x672E08

// colors
#define FO_VAR_BlueColor                  0x6A38EF
#define FO_VAR_DARK_GREY_Color            0x6A59D8
#define FO_VAR_DarkGreenColor             0x6A3A90
#define FO_VAR_DarkGreenGreyColor         0x6A3DF1
#define FO_VAR_DarkRedColor               0x6AA8D0
#define FO_VAR_DarkYellowColor            0x6AB472
#define FO_VAR_DullPinkColor              0x6AB718
#define FO_VAR_GoodColor                  0x6AB4EF
#define FO_VAR_GreenColor                 0x6A3CB0
#define FO_VAR_LIGHT_GREY_Color           0x6A76BF
#define FO_VAR_LIGHT_RED_Color            0x6AB61A
#define FO_VAR_PeanutButter               0x6A82F3
#define FO_VAR_RedColor                   0x6AB4D0
#define FO_VAR_WhiteColor                 0x6AB8CF
#define FO_VAR_YellowColor                0x6AB8BB // Light

// Global variable pointers
// TODO: move to separate namespace

// defines pointer to an engine variable
#define PTR_(name, type)	\
	extern type* ptr_##name;

// X-Macros pattern
#include "FalloutVars_def.h"

// engine function offsets
// TODO: move to separate namespace
extern const DWORD AddHotLines_;
extern const DWORD Check4Keys_;
extern const DWORD Create_AudioDecoder_;
extern const DWORD DOSCmdLineDestroy_;
extern const DWORD DrawCard_;
extern const DWORD DrawFolder_;
extern const DWORD DrawInfoWin_;
extern const DWORD EndLoad_;
extern const DWORD EndPipboy_;
extern const DWORD FMtext_char_width_;
extern const DWORD FMtext_to_buf_;
extern const DWORD FMtext_width_;
extern const DWORD GNW95_lost_focus_;
extern const DWORD GNW95_process_message_;
extern const DWORD GNW_button_refresh_;
extern const DWORD GNW_do_bk_process_;
extern const DWORD GNW_find_;
extern const DWORD GNW_win_refresh_;
extern const DWORD GetSlotList_;
extern const DWORD ListDPerks_;
extern const DWORD ListDrvdStats_;
extern const DWORD ListHoloDiskTitles_;
extern const DWORD ListSkills_;
extern const DWORD ListTraits_;
extern const DWORD LoadGame_;
extern const DWORD LoadSlot_;
extern const DWORD MapDirErase_;
extern const DWORD NixHotLines_;
extern const DWORD OptionWindow_;
extern const DWORD PipStatus_;
extern const DWORD PrintBasicStat_;
extern const DWORD PrintLevelWin_;
extern const DWORD RestorePlayer_;
extern const DWORD SaveGame_;
extern const DWORD SavePlayer_;
extern const DWORD SexWindow_;
extern const DWORD _word_wrap_;
extern const DWORD action_get_an_object_;
extern const DWORD action_loot_container_;
extern const DWORD action_use_an_item_on_object_;
extern const DWORD add_bar_box_;
extern const DWORD adjust_ac_;
extern const DWORD adjust_fid_;
extern const DWORD ai_can_use_weapon_; // (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0
extern const DWORD ai_cap_;
extern const DWORD ai_check_drugs_;
extern const DWORD ai_pick_hit_mode_;
extern const DWORD ai_run_away_;
extern const DWORD ai_search_inven_armor_;
extern const DWORD ai_search_inven_weap_;
extern const DWORD ai_switch_weapons_;
extern const DWORD ai_try_attack_;
extern const DWORD anim_can_use_door_;
extern const DWORD art_alias_fid_;
extern const DWORD art_alias_num_;
extern const DWORD art_exists_; // eax - frameID, used for critter FIDs
extern const DWORD art_flush_;
extern const DWORD art_frame_data_;
extern const DWORD art_frame_length_;
extern const DWORD art_frame_width_;
extern const DWORD art_get_code_;
extern const DWORD art_get_name_;
extern const DWORD art_id_;
extern const DWORD art_init_;
extern const DWORD art_lock_;
extern const DWORD art_ptr_lock_;
extern const DWORD art_ptr_lock_data_;
extern const DWORD art_ptr_unlock_;
extern const DWORD attack_crit_success_;
extern const DWORD audioCloseFile_;
extern const DWORD audioFileSize_;
extern const DWORD audioOpen_;
extern const DWORD audioRead_;
extern const DWORD audioSeek_;
extern const DWORD automap_;
extern const DWORD barter_compute_value_;
extern const DWORD barter_inventory_;
extern const DWORD block_for_tocks_;
extern const DWORD buf_to_buf_;
extern const DWORD caiHasWeapPrefType_;
extern const DWORD cai_attempt_w_reload_;
extern const DWORD can_see_;
extern const DWORD check_death_;
extern const DWORD check_for_death_;
extern const DWORD combat_;
extern const DWORD combat_ai_;
extern const DWORD combat_anim_finished_;
extern const DWORD combat_attack_;
extern const DWORD combat_check_bad_shot_;
extern const DWORD combat_ctd_init_;
extern const DWORD combat_delete_critter_;
extern const DWORD combat_input_;
extern const DWORD combat_should_end_;
extern const DWORD combat_turn_;
extern const DWORD combat_turn_run_;
extern const DWORD combatai_rating_;
extern const DWORD compute_damage_;
extern const DWORD compute_spray_;
extern const DWORD config_get_string_;
extern const DWORD config_get_value_;
extern const DWORD config_set_value_;
extern const DWORD construct_box_bar_win_;
extern const DWORD container_exit_;
extern const DWORD correctFidForRemovedItem_; // (int critter@<eax>, int oldArmor@<edx>, int removeSlotsFlags@<ebx>)
extern const DWORD createWindow_;
extern const DWORD credits_;
extern const DWORD credits_get_next_line_;
extern const DWORD critterClearObjDrugs_;
extern const DWORD critterIsOverloaded_;
extern const DWORD critter_adjust_hits_;
extern const DWORD critter_body_type_;
extern const DWORD critter_can_obj_dude_rest_;
extern const DWORD critter_compute_ap_from_distance_;
extern const DWORD critter_flag_check_;
extern const DWORD critter_get_hits_;
extern const DWORD critter_get_rads_;
extern const DWORD critter_is_dead_;
extern const DWORD critter_kill_;
extern const DWORD critter_kill_count_type_;
extern const DWORD critter_name_;
extern const DWORD critter_pc_set_name_;
extern const DWORD datafileConvertData_;
extern const DWORD db_access_;
extern const DWORD db_dir_entry_;
extern const DWORD db_fclose_;
extern const DWORD db_fgetc_;
extern const DWORD db_fgets_;
extern const DWORD db_fopen_;
extern const DWORD db_freadByteCount_;
extern const DWORD db_freadByte_;
extern const DWORD db_freadIntCount_;
extern const DWORD db_freadInt_;
extern const DWORD db_freadShortCount_;
extern const DWORD db_freadShort_;
extern const DWORD db_fread_;
extern const DWORD db_free_file_list_;
extern const DWORD db_fseek_;
extern const DWORD db_fwriteByteCount_;
extern const DWORD db_fwriteByte_;
extern const DWORD db_fwriteInt_;
extern const DWORD db_get_file_list_;
extern const DWORD db_init_;
extern const DWORD db_read_to_buf_;
extern const DWORD dbase_close_;
extern const DWORD dbase_open_;
extern const DWORD debug_log_;
extern const DWORD debug_printf_;
extern const DWORD debug_register_env_;
extern const DWORD determine_to_hit_;
extern const DWORD determine_to_hit_from_tile_;
extern const DWORD determine_to_hit_func_;
extern const DWORD determine_to_hit_no_range_;
extern const DWORD dialog_out_;
extern const DWORD displayInWindow_;
extern const DWORD display_inventory_;
extern const DWORD display_print_; // eax - char* to display
extern const DWORD display_scroll_down_;
extern const DWORD display_scroll_up_;
extern const DWORD display_stats_;
extern const DWORD display_table_inventories_;
extern const DWORD display_target_inventory_;
extern const DWORD do_optionsFunc_;
extern const DWORD do_options_;
extern const DWORD do_prefscreen_;
extern const DWORD drop_into_container_;
extern const DWORD dude_stand_;
extern const DWORD dude_standup_;
extern const DWORD editor_design_;
extern const DWORD elapsed_time_;
extern const DWORD elevator_end_;
extern const DWORD elevator_start_;
extern const DWORD endgame_slideshow_;
extern const DWORD exec_script_proc_; // unsigned int aScriptID<eax>, int aProcId<edx>
extern const DWORD executeProcedure_; // <eax> - programPtr, <edx> - procNumber
extern const DWORD exit_inventory_;
extern const DWORD fadeSystemPalette_;
extern const DWORD findCurrentProc_;
extern const DWORD findVar_;
extern const DWORD folder_print_line_;
extern const DWORD fprintf_;
extern const DWORD frame_ptr_;
extern const DWORD game_exit_;
extern const DWORD game_get_global_var_;
extern const DWORD game_help_;
extern const DWORD game_set_global_var_;
extern const DWORD game_time_;
extern const DWORD game_time_date_;
extern const DWORD gdDestroyHeadWindow_;
extern const DWORD gdProcess_;
extern const DWORD gdReviewExit_;
extern const DWORD gdReviewInit_;
extern const DWORD gdialogActive_;
extern const DWORD gdialogDisplayMsg_;
extern const DWORD gdialogFreeSpeech_;
extern const DWORD gdialog_barter_cleanup_tables_;
extern const DWORD gdialog_barter_pressed_;
extern const DWORD gdialog_window_create_;
extern const DWORD gdialog_window_destroy_;
extern const DWORD get_input_;
extern const DWORD get_input_str2_;
extern const DWORD get_time_;
extern const DWORD getmsg_; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory
//extern const DWORD gmouse_3d_get_mode_;
extern const DWORD gmouse_3d_set_mode_;
extern const DWORD gmouse_is_scrolling_;
extern const DWORD gmouse_set_cursor_;
extern const DWORD gmovieIsPlaying_;
extern const DWORD gmovie_play_;
extern const DWORD gsnd_build_weapon_sfx_name_;
extern const DWORD gsound_background_pause_;
extern const DWORD gsound_background_restart_last_;
extern const DWORD gsound_background_stop_;
extern const DWORD gsound_background_unpause_;
extern const DWORD gsound_background_volume_get_set_;
extern const DWORD gsound_play_sfx_file_;
extern const DWORD gsound_red_butt_press_;
extern const DWORD gsound_red_butt_release_;
extern const DWORD gsound_speech_length_get_;
extern const DWORD gsound_speech_play_;
extern const DWORD handle_inventory_;
extern const DWORD inc_game_time_;
extern const DWORD inc_stat_;
extern const DWORD insert_withdrawal_;
extern const DWORD interpretAddString_; // edx = ptr to string, eax = script
extern const DWORD interpretError_;
extern const DWORD interpretFindProcedure_; // get proc number (different for each script) by name: *<eax> - scriptPtr, char* <edx> - proc name
extern const DWORD interpretFreeProgram_; // <eax> - program ptr, frees it from memory and from scripting engine
extern const DWORD interpretGetString_; // eax = script ptr, edx = var type, ebx = var
extern const DWORD interpretPopLong_;
extern const DWORD interpretPopShort_;
extern const DWORD interpretPushLong_;
extern const DWORD interpretPushShort_;
extern const DWORD interpret_;
extern const DWORD intface_disable_;
extern const DWORD intface_enable_;
extern const DWORD intface_get_attack_;
extern const DWORD intface_hide_;
extern const DWORD intface_is_hidden_;
extern const DWORD intface_is_item_right_hand_;
extern const DWORD intface_item_reload_;
extern const DWORD intface_redraw_;
extern const DWORD intface_show_;
extern const DWORD intface_toggle_item_state_;
extern const DWORD intface_toggle_items_;
extern const DWORD intface_update_ac_;
extern const DWORD intface_update_hit_points_;
extern const DWORD intface_update_items_;
extern const DWORD intface_update_move_points_;
extern const DWORD intface_use_item_;
extern const DWORD invenUnwieldFunc_; // (int critter@<eax>, int slot@<edx>, int a3@<ebx>) - int result (-1 on error, 0 on success)
extern const DWORD invenWieldFunc_; // (int who@<eax>, int item@<edx>, int a3@<ecx>, int slot@<ebx>) - int result (-1 on error, 0 on success)
extern const DWORD inven_display_msg_;
extern const DWORD inven_find_id_;
extern const DWORD inven_find_type_;
extern const DWORD inven_left_hand_;
extern const DWORD inven_pid_is_carried_ptr_;
extern const DWORD inven_right_hand_;
extern const DWORD inven_set_mouse_;
extern const DWORD inven_unwield_;
extern const DWORD inven_wield_;
extern const DWORD inven_worn_;
extern const DWORD isPartyMember_; // (<eax> - object) - bool result
extern const DWORD is_pc_sneak_working_;
extern const DWORD is_within_perception_;
extern const DWORD item_add_force_;
extern const DWORD item_add_mult_;
extern const DWORD item_c_curr_size_;
extern const DWORD item_c_max_size_;
extern const DWORD item_caps_total_;
extern const DWORD item_count_;
extern const DWORD item_d_check_addict_;
extern const DWORD item_d_take_drug_;
extern const DWORD item_drop_all_;
extern const DWORD item_get_type_;
extern const DWORD item_hit_with_;
extern const DWORD item_m_cell_pid_;
extern const DWORD item_m_dec_charges_;
extern const DWORD item_m_turn_off_;
extern const DWORD item_move_all_;
extern const DWORD item_move_all_hidden_;
extern const DWORD item_move_force_;
extern const DWORD item_mp_cost_;
extern const DWORD item_remove_mult_;
extern const DWORD item_size_;
extern const DWORD item_total_cost_;
extern const DWORD item_total_weight_;
extern const DWORD item_w_anim_code_;
extern const DWORD item_w_anim_weap_;
extern const DWORD item_w_can_reload_;
extern const DWORD item_w_compute_ammo_cost_; // signed int aWeapon<eax>, int *aRoundsSpent<edx>
extern const DWORD item_w_curr_ammo_;
extern const DWORD item_w_dam_div_;
extern const DWORD item_w_dam_mult_;
extern const DWORD item_w_damage_;
extern const DWORD item_w_damage_type_;
extern const DWORD item_w_dr_adjust_;
extern const DWORD item_w_max_ammo_;
extern const DWORD item_w_mp_cost_;
extern const DWORD item_w_perk_;
extern const DWORD item_w_range_;
extern const DWORD item_w_reload_;
extern const DWORD item_w_rounds_;
extern const DWORD item_w_subtype_;
extern const DWORD item_w_try_reload_;
extern const DWORD item_w_unload_;
extern const DWORD item_weight_;
extern const DWORD kb_clear_;
extern const DWORD light_get_tile_; // aElev<eax>, aTilenum<edx>
extern const DWORD loadColorTable_;
extern const DWORD loadPCX_;
extern const DWORD loadProgram_; // loads script from scripts/ folder by file name and returns pointer to it: char* <eax> - file name (w/o extension)
extern const DWORD load_frame_;
extern const DWORD loot_container_;
extern const DWORD main_game_loop_;
extern const DWORD main_init_system_;
extern const DWORD main_menu_hide_;
extern const DWORD main_menu_loop_;
// (int aObjFrom<eax>, int aTileFrom<edx>, char* aPathPtr<ecx>, int aTileTo<ebx>, int a5, int (__fastcall *a6)(_DWORD, _DWORD))
// - path is saved in ecx as a sequence of tile directions (0..5) to move on each step,
// - returns path length
extern const DWORD make_path_func_;
extern const DWORD make_straight_path_;
extern const DWORD make_straight_path_func_; // (TGameObj *aObj<eax>, int aTileFrom<edx>, int a3<ecx>, signed int aTileTo<ebx>, TGameObj **aObjResult, int a5, int (*a6)(void))
extern const DWORD map_disable_bk_processes_;
extern const DWORD map_enable_bk_processes_;
extern const DWORD map_exit_;
extern const DWORD map_get_short_name_;
extern const DWORD map_load_idx_;
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
extern const DWORD mouse_click_in_;
extern const DWORD mouse_get_position_;
extern const DWORD mouse_get_rect_;
extern const DWORD mouse_hide_;
extern const DWORD mouse_in_;
extern const DWORD mouse_show_;
extern const DWORD move_inventory_;
extern const DWORD movieRun_;
extern const DWORD movieStop_;
extern const DWORD movieUpdate_;
extern const DWORD my_free_;
extern const DWORD new_obj_id_;
extern const DWORD nrealloc_;
extern const DWORD obj_ai_blocking_at_;
extern const DWORD obj_blocking_at_; // <eax>(int aExcludeObject<eax> /* can be 0 */, signed int aTile<edx>, int aElevation<ebx>)
extern const DWORD obj_bound_;
extern const DWORD obj_change_fid_;
extern const DWORD obj_connect_;
extern const DWORD obj_destroy_;
extern const DWORD obj_dist_;
extern const DWORD obj_dist_with_tile_;
extern const DWORD obj_drop_;
extern const DWORD obj_erase_object_;
extern const DWORD obj_examine_;
extern const DWORD obj_find_first_;
extern const DWORD obj_find_first_at_;
extern const DWORD obj_find_first_at_tile_; // <eax>(int elevation<eax>, int tile<edx>)
extern const DWORD obj_find_next_;
extern const DWORD obj_find_next_at_;
extern const DWORD obj_find_next_at_tile_;
extern const DWORD obj_is_a_portal_;
extern const DWORD obj_lock_is_jammed_;
extern const DWORD obj_move_to_tile_;  // int aObj<eax>, int aTile<edx>, int aElev<ebx>
extern const DWORD obj_new_;  // int aObj*<eax>, int aPid<ebx>
extern const DWORD obj_new_sid_inst_;
extern const DWORD obj_outline_object_;
extern const DWORD obj_pid_new_;
extern const DWORD obj_remove_from_inven_;
extern const DWORD obj_remove_outline_;
extern const DWORD obj_save_dude_;
extern const DWORD obj_scroll_blocking_at_;
extern const DWORD obj_set_light_; // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
extern const DWORD obj_shoot_blocking_at_;
extern const DWORD obj_sight_blocking_at_;
extern const DWORD obj_top_environment_;
extern const DWORD obj_turn_off_;  // int aObj<eax>, int ???<edx>
extern const DWORD obj_unjam_lock_;
extern const DWORD obj_use_book_;
extern const DWORD obj_use_power_on_car_;
extern const DWORD object_under_mouse_;
extern const DWORD palette_fade_to_;
extern const DWORD palette_init_;
extern const DWORD palette_set_to_;
extern const DWORD partyMemberCopyLevelInfo_;
extern const DWORD partyMemberGetAIOptions_;
extern const DWORD partyMemberGetCurLevel_;
extern const DWORD partyMemberIncLevels_;
extern const DWORD partyMemberPrepItemSaveAll_;
extern const DWORD partyMemberPrepLoad_;
extern const DWORD partyMemberRemove_;
extern const DWORD partyMemberSaveProtos_;
extern const DWORD pause_for_tocks_;
extern const DWORD pc_flag_off_;
extern const DWORD pc_flag_on_;
extern const DWORD pc_flag_toggle_;
extern const DWORD perform_withdrawal_end_;
extern const DWORD perkGetLevelData_;
extern const DWORD perk_add_;
extern const DWORD perk_add_effect_;
extern const DWORD perk_add_force_;
extern const DWORD perk_can_add_;
extern const DWORD perk_description_;
extern const DWORD perk_init_;
extern const DWORD perk_level_;
extern const DWORD perk_make_list_;
extern const DWORD perk_name_;
extern const DWORD perk_skilldex_fid_;
extern const DWORD perks_dialog_;
extern const DWORD pick_death_;
extern const DWORD pip_back_;
extern const DWORD pip_print_;
extern const DWORD pipboy_;
extern const DWORD process_bk_;
extern const DWORD protinst_use_item_;
extern const DWORD protinst_use_item_on_;
extern const DWORD proto_dude_update_gender_;
extern const DWORD proto_list_str_;
extern const DWORD proto_ptr_; // eax - PID, edx - int** - pointer to a pointer to a proto struct
extern const DWORD pushLongStack_;
extern const DWORD qsort_;
extern const DWORD queue_add_;
extern const DWORD queue_clear_type_;
extern const DWORD queue_explode_exit_;
extern const DWORD queue_find_;
extern const DWORD queue_find_first_;
extern const DWORD queue_find_next_;
extern const DWORD queue_leaving_map_;
extern const DWORD queue_next_time_;
extern const DWORD queue_remove_this_;
extern const DWORD rect_clip_;
extern const DWORD rect_malloc_;
extern const DWORD refresh_all_;
extern const DWORD refresh_box_bar_win_;
extern const DWORD register_begin_;
extern const DWORD register_clear_;
extern const DWORD register_end_;
extern const DWORD register_object_animate_; // int aObj<eax>, int aAnim<edx>, int delay<ebx>
extern const DWORD register_object_animate_and_hide_; // int aObj<eax>, int aAnim<edx>, int delay<ebx>
extern const DWORD register_object_call_;
extern const DWORD register_object_change_fid_; // int aObj<eax>, int aFid<edx>, int aDelay<ebx>
extern const DWORD register_object_funset_; // int aObj<eax>, int ???<edx>, int aDelay<ebx> - not really sure what this does
extern const DWORD register_object_light_; // <eax>(int aObj<eax>, int aRadius<edx>, int aDelay<ebx>)
extern const DWORD register_object_must_erase_; // int aObj<eax>
extern const DWORD register_object_take_out_; // int aObj<eax>, int aHoldFrame<edx> - hold frame ID (1 - spear, 2 - club, etc.)
extern const DWORD register_object_turn_towards_; // int aObj<eax>, int aTile<edx>
extern const DWORD remove_bk_process_;
extern const DWORD report_explosion_;
extern const DWORD reset_box_bar_win_;
extern const DWORD roll_random_;
extern const DWORD runProgram_; // eax - programPtr, called once for each program after first loaded - hooks program to game and UI events
extern const DWORD scr_exec_map_exit_scripts_;
extern const DWORD scr_exec_map_update_scripts_;
extern const DWORD scr_find_first_at_; // eax - elevation, returns spatial scriptID
extern const DWORD scr_find_next_at_; // no args, returns spatial scriptID
extern const DWORD scr_find_obj_from_program_; // eax - *program - finds self_obj by program pointer (has nice additional effect - creates fake object for a spatial script)
extern const DWORD scr_find_sid_from_program_;
extern const DWORD scr_get_local_var_;
extern const DWORD scr_new_; // eax - script index from scripts lst, edx - type (0 - system, 1 - spatials, 2 - time, 3 - items, 4 - critters)
extern const DWORD scr_ptr_; // eax - scriptId, edx - **TScript (where to store script pointer)
extern const DWORD scr_remove_;
extern const DWORD scr_set_ext_param_;
extern const DWORD scr_set_local_var_;
extern const DWORD scr_set_objs_;
extern const DWORD scr_write_ScriptNode_;
extern const DWORD selectWindowID_;
extern const DWORD set_focus_func_;
extern const DWORD set_game_time_;
extern const DWORD setup_move_timer_win_;
extern const DWORD skill_check_stealing_;
extern const DWORD skill_dec_point_;
extern const DWORD skill_get_tags_; // eax - pointer to array DWORD, edx - number of elements to read
extern const DWORD skill_inc_point_;
extern const DWORD skill_is_tagged_;
extern const DWORD skill_level_;
extern const DWORD skill_points_;
extern const DWORD skill_set_tags_; // eax - pointer to array DWORD, edx - number of elements to write
extern const DWORD skill_use_;
extern const DWORD skilldex_select_;
extern const DWORD soundDelete_;
extern const DWORD soundGetPosition_;
extern const DWORD soundPlay_;
extern const DWORD soundPlaying_;
extern const DWORD soundSetCallback_;
extern const DWORD soundSetFileIO_;
extern const DWORD soundVolume_;
extern const DWORD sprintf_;
extern const DWORD square_num_;
extern const DWORD stat_get_base_;
extern const DWORD stat_get_base_direct_;
extern const DWORD stat_get_bonus_;
extern const DWORD stat_level_; // &GetCurrentStat(void* critter, int statID)
extern const DWORD stat_pc_add_experience_;
extern const DWORD stat_pc_get_;
extern const DWORD stat_pc_set_;
extern const DWORD stat_recalc_derived_;
extern const DWORD stat_set_bonus_;
extern const DWORD stat_set_defaults_;
extern const DWORD strParseStrFromList_;
extern const DWORD stricmp_;
extern const DWORD strncpy_;
extern const DWORD switch_hand_;
extern const DWORD talk_to_critter_reacts_;
extern const DWORD talk_to_translucent_trans_buf_to_buf_;
extern const DWORD text_curr_;
extern const DWORD text_font_;
extern const DWORD text_object_create_;
extern const DWORD tile_coord_; // eax - tilenum, edx (int*) - x, ebx (int*) - y
extern const DWORD tile_dir_;
extern const DWORD tile_dist_;
extern const DWORD tile_num_;
extern const DWORD tile_num_in_direction_;
extern const DWORD tile_refresh_display_;
extern const DWORD tile_refresh_rect_; // (int elevation<edx>, unkown<ecx>)
extern const DWORD tile_scroll_to_;
extern const DWORD tile_set_center_;
extern const DWORD trait_adjust_skill_;
extern const DWORD trait_adjust_stat_;
extern const DWORD trait_get_;
extern const DWORD trait_init_;
extern const DWORD trait_level_;
extern const DWORD trait_set_;
extern const DWORD trans_cscale_;
extern const DWORD use_inventory_on_;
extern const DWORD win_add_;
extern const DWORD win_clip_;
extern const DWORD win_delete_;
extern const DWORD win_disable_button_;
extern const DWORD win_draw_;
extern const DWORD win_draw_rect_;
extern const DWORD win_enable_button_;
extern const DWORD win_fill_;
extern const DWORD win_get_buf_;
extern const DWORD win_get_rect_;
extern const DWORD win_get_top_win_;
extern const DWORD win_height_;
extern const DWORD win_hide_;
extern const DWORD win_line_;
extern const DWORD win_print_;
extern const DWORD win_register_button_;
extern const DWORD win_register_button_disable_;
extern const DWORD win_register_button_sound_func_;
extern const DWORD win_show_;
extern const DWORD win_width_;
extern const DWORD windowDisplayBuf_;
extern const DWORD windowDisplayTransBuf_;
extern const DWORD windowGetBuffer_;
extern const DWORD windowGetTextColor_;
extern const DWORD windowHide_;
extern const DWORD windowOutput_;
extern const DWORD windowShow_;
extern const DWORD windowWidth_;
extern const DWORD windowWrapLineWithSpacing_;
extern const DWORD wmDrawCursorStopped_;
extern const DWORD wmFindCurSubTileFromPos_;
extern const DWORD wmInterfaceDrawSubTileRectFogged_;
extern const DWORD wmInterfaceInit_;
extern const DWORD wmInterfaceRefresh_;
extern const DWORD wmInterfaceScrollTabsStart_;
extern const DWORD wmMapIsSaveable_;
extern const DWORD wmMarkSubTileOffsetVisitedFunc_;
extern const DWORD wmMarkSubTileRadiusVisited_;
extern const DWORD wmMatchAreaContainingMapIdx_;
extern const DWORD wmPartyInitWalking_;
extern const DWORD wmPartyWalkingStep_;
extern const DWORD wmRefreshInterfaceOverlay_;
extern const DWORD wmSubTileMarkRadiusVisited_;
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
extern const DWORD xremovepath_;
extern const DWORD xrewind_;
extern const DWORD xungetc_;
extern const DWORD xvfprintf_;


/*
* HOW TO USE ENGINE FUNCTIONS:
*
* in ASM code, call offsets directly, don't call wrappers as they might not be __stdcall
* in C++ code, use wrappers (add new ones if the don't exist yet)
*
* Note: USE C++!
* 1) Place thin __declspec(naked) hooks, only use minimum ASM to pass values to/from C++
* 2) Call __stdcall functions from (1), write those entirely in C++ (with little ASM blocks only to call engine functions, when you are too lazy to add wrapper)
*/

// WRAPPERS:
// TODO: move these to different namespace

#ifndef NDEBUG
// Prints debug message to game debug.log file for develop build
void dev_printf(const char* fmt, ...);
#else
void dev_printf(...);
#endif

/*
	Add functions here if they have non-trivial wrapper implementation (like vararg functions or too many arguments, etc.)
	Otherwise use FalloutFuncs_def.h file (much easier).
*/

// prints message to debug.log file
void __declspec() fo_debug_printf(const char* fmt, ...);

void __stdcall fo_interpretReturnValue(TProgram* scriptPtr, DWORD val, DWORD valType);

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec() fo_interpretError(const char* fmt, ...);

long __fastcall fo_tile_num(long x, long y);

TGameObj* __fastcall obj_blocking_at_wrapper(TGameObj* obj, DWORD tile, DWORD elevation, void* func);

// Creates a button on a given window
// buttonType: 0x10 = move window pos, 0x20 or 0x0 = regular click, 0x23 = toggle click
// pictureUp/pictureDown - pointers to a surface
long __stdcall fo_win_register_button(DWORD winRef, long xPos, long yPos, long width, long height, long hoverOn, long hoverOff, long buttonDown, long buttonUp, BYTE* pictureUp, BYTE* pictureDown, long arg12, long buttonType);

void __stdcall DialogOut(const char* text);

long __fastcall DialogOutEx(const char* text, const char** textEx, long lines, long flags, long colors = 0);

// draws an image to the buffer without scaling and with transparency display toggle
void __fastcall fo_windowDisplayBuf(long x, long width, long y, long height, void* data, long noTrans);

// draws an image in the window and scales it to fit the window
void __fastcall fo_displayInWindow(long w_here, long width, long height, void* data);

// draws an image to the buffer of the active script window
void __fastcall window_trans_cscale(long i_width, long i_height, long s_width, long s_height, long xy_shift, long w_width, void* data);

// buf_to_buf_ function with pure MMX implementation
void __cdecl fo_buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width);

// trans_buf_to_buf_ function implementation
void __cdecl fo_trans_buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width);

BYTE* __fastcall fo_loadPCX(const char* file, long* width, long* height);

long __fastcall fo_get_game_config_string(const char* outValue, const char* section, const char* param);

// X-Macro for wrapper functions.
#define WRAP_WATCOM_FUNC0(retType, name) \
	retType __stdcall fo_##name();

#define WRAP_WATCOM_FUNC1(retType, name, arg1t, arg1) \
	retType __stdcall fo_##name(arg1t arg1);

#define WRAP_WATCOM_FUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2);

#define WRAP_WATCOM_FUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3);

#define WRAP_WATCOM_FUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4);

#define WRAP_WATCOM_FUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5);

#define WRAP_WATCOM_FUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6);


#define WRAP_WATCOM_FFUNC1(retType, name, arg1t, arg1) \
	retType __fastcall fo_##name(arg1t arg1);

#define WRAP_WATCOM_FFUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2);

#define WRAP_WATCOM_FFUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3);

#define WRAP_WATCOM_FFUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4);

#define WRAP_WATCOM_FFUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5);

#define WRAP_WATCOM_FFUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6);

#define WRAP_WATCOM_FFUNC7(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7);

#define WRAP_WATCOM_FFUNC8(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8);

#define WRAP_WATCOM_FFUNC9(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8, arg9t, arg9) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8, arg9t arg9);

#include "FalloutFuncs_def.h"

#undef WRAP_WATCOM_FUNC0
#undef WRAP_WATCOM_FUNC1
#undef WRAP_WATCOM_FUNC2
#undef WRAP_WATCOM_FUNC3
#undef WRAP_WATCOM_FUNC4
#undef WRAP_WATCOM_FUNC5
#undef WRAP_WATCOM_FUNC6
//#undef WRAP_WATCOM_FUNC7

#undef WRAP_WATCOM_FFUNC1
#undef WRAP_WATCOM_FFUNC2
#undef WRAP_WATCOM_FFUNC3
#undef WRAP_WATCOM_FFUNC4
#undef WRAP_WATCOM_FFUNC5
#undef WRAP_WATCOM_FFUNC6
#undef WRAP_WATCOM_FFUNC7
#undef WRAP_WATCOM_FFUNC8
#undef WRAP_WATCOM_FFUNC9

///////////////////////////////// ENGINE UTILS /////////////////////////////////

// rect_free_ function for inline implementation
__forceinline void sf_rect_free(RectList* rect) {
	RectList* front = *ptr_rectList;
	*ptr_rectList = rect;
	rect->nextRect = front;
}

// returns message string from given file or "Error" when not found
const char* GetMessageStr(const MSGList* fileAddr, long messageId);

// similar to GetMessageStr, but returns nullptr when no message is found
const char* MsgSearch(const MSGList* fileAddr, long messageId);

Queue* QueueFindUtil(TGameObj* object, long type);

// returns weapon animation code
long AnimCodeByWeapon(TGameObj* weapon);

// returns pointer to prototype by PID, or nullptr on failure
sProto* GetProto(long pid);

// wrapper for skill_get_tags with bounds checking
void SkillGetTags(long* result, long num);

// wrapper for skill_set_tags with bounds checking
void SkillSetTags(long* tags, long num);

long __fastcall GetItemType(TGameObj* item);

__declspec(noinline) TGameObj* __stdcall GetItemPtrSlot(TGameObj* critter, InvenType slot);

long& GetActiveItemMode();

TGameObj* GetActiveItem();

bool HeroIsFemale();

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence();

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid);

// Returns window by x/y coordinate (hidden windows are ignored)
WINinfo* __fastcall GetTopWindowAtPos(long xPos, long yPos, bool bypassTrans = false);

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<TGameObj*> &objs, long sourceTile, long radius, long elev, long type = -1);

// Checks the blocking tiles and returns the first blocking object
TGameObj* CheckAroundBlockingTiles(TGameObj* source, long dstTile);

TGameObj* __fastcall MultiHexMoveIsBlocking(TGameObj* source, long dstTile);

long wmGetCurrentTerrainType();

void SurfaceCopyToMem(long fromX, long fromY, long width, long height, long fromWidth, BYTE* fromSurface, BYTE* toMem);

void DrawToSurface(long toX, long toY, long width, long height, long toWidth, long toHeight, BYTE* toSurface, BYTE* fromMem);

void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf, long toX, long toY, long toWidth, long toHeight, BYTE* toSurf, int maskRef);

void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf, long toX, long toY, long toWidth, long toHeight, BYTE* toSurf);

// Fills the specified interface window with index color
void WinFillRect(long winID, long x, long y, long width, long height, BYTE indexColor);

// Fills the specified interface window with index color 0 (black color)
void ClearWindow(long winID, bool refresh = true);

void PrintFloatText(TGameObj* object, const char* text, long colorText, long colorOutline = 207, long font = 101);

// Print text to surface
void __stdcall PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface);
void __stdcall PrintTextFM(const char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface);

// gets the height of the currently selected font
DWORD __stdcall GetTextHeight();

// gets the length of a string using the currently selected font
DWORD __stdcall GetTextWidth(const char* textMsg);
DWORD __stdcall GetTextWidthFM(const char* textMsg);

// get width of Char for current font
DWORD __stdcall GetCharWidth(char charVal);
DWORD __stdcall GetCharWidthFM(char charVal);

// get maximum string length for current font - if all characters were maximum width
DWORD __stdcall GetMaxTextWidth(const char* textMsg);

// get number of pixels between characters for current font
DWORD __stdcall GetCharGapWidth();

// get maximum character width for current font
DWORD __stdcall GetMaxCharWidth();

// Redraw the given object on screen (does not always redraws the whole object)
void RedrawObject(TGameObj* obj);

// Redraws all windows
void RefreshGNW(bool skipOwner = false);

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef);
