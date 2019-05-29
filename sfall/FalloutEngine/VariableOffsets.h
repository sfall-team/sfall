#pragma once

// Global variable constants

// To add another variable, first add FO_VAR_* constant with it's address, then add it in Varaibles_def.h

// PLEASE USE THOSE IN ASM BLOCKS!
#define FO_VAR_aDialogS_msg               0x50DBE8
#define FO_VAR_aiInfoList                 0x510948
#define FO_VAR_ambient_light              0x51923C
#define FO_VAR_anim_set                   0x54CC14
#define FO_VAR_art                        0x510738
#define FO_VAR_art_name                   0x56C9E4
#define FO_VAR_art_vault_guy_num          0x5108A4
#define FO_VAR_art_vault_person_nums      0x5108A8
#define FO_VAR_aTextSCuts                 0x501A8C
#define FO_VAR_aTextSCutsS                0x503530
#define FO_VAR_aTextSCutsSS               0x50B01C
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
#define FO_VAR_currentWindow              0x51DCB8
#define FO_VAR_cursor_line                0x664514
#define FO_VAR_dialogue_head              0x518850
#define FO_VAR_dialogue_state             0x518714
#define FO_VAR_dialogue_switch_mode       0x518718
#define FO_VAR_dialog_target              0x518848
#define FO_VAR_dialog_target_is_party     0x51884C
#define FO_VAR_displayMapList             0x41B560
#define FO_VAR_dropped_explosive          0x5190E0
#define FO_VAR_drugInfoList               0x5191CC
#define FO_VAR_editor_message_file        0x56FCA8
#define FO_VAR_edit_win                   0x57060C
#define FO_VAR_Educated                   0x57082C
#define FO_VAR_elevation                  0x631D2C
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
#define FO_VAR_frstc_draw1                0x5707D8
#define FO_VAR_game_global_vars           0x5186C0
#define FO_VAR_game_user_wants_to_quit    0x5186CC
#define FO_VAR_gcsd                       0x51094C
#define FO_VAR_gdBarterMod                0x51873C
#define FO_VAR_gDialogMusicVol            0x5187D8
#define FO_VAR_gdNumOptions               0x5186D8
#define FO_VAR_gIsSteal                   0x51D430
#define FO_VAR_glblmode                   0x5709D0
#define FO_VAR_gmouse_current_cursor      0x518C0C
#define FO_VAR_gmovie_played_list         0x596C78
#define FO_VAR_GNW_win_init_flag          0x51E3E0
#define FO_VAR_gsound_initialized         0x518E30
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
#define FO_VAR_mouse_hotx                 0x6AC7D0
#define FO_VAR_mouse_hoty                 0x6AC7CC
#define FO_VAR_mouse_is_hidden            0x6AC790
#define FO_VAR_mouse_x_                   0x6AC7A8
#define FO_VAR_mouse_y                    0x664450
#define FO_VAR_mouse_y_                   0x6AC7A4
#define FO_VAR_Mutate_                    0x5708B4
#define FO_VAR_name_color                 0x56D744
#define FO_VAR_name_font                  0x56D74C
#define FO_VAR_name_sort_list             0x56FCB0
#define FO_VAR_num_game_global_vars       0x5186C4
#define FO_VAR_num_map_global_vars        0x519574
#define FO_VAR_card_old_fid1              0x5709EC
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
#define FO_VAR_outlined_object            0x518D94
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
#define FO_VAR_proto_msg_files            0x6647AC
#define FO_VAR_proto_main_msg_file        0x6647FC
#define FO_VAR_ptable                     0x59E934
#define FO_VAR_pud                        0x59E960
#define FO_VAR_quest_count                0x51C12C
#define FO_VAR_queue                      0x6648C0
#define FO_VAR_quick_done                 0x5193BC
#define FO_VAR_read_callback              0x51DEEC
#define FO_VAR_retvals                    0x43EA7C
#define FO_VAR_rotation                   0x631D34
#define FO_VAR_sad                        0x530014
#define FO_VAR_scr_size                   0x6AC9F0
#define FO_VAR_script_engine_running      0x51C714
#define FO_VAR_script_path_base           0x51C710
#define FO_VAR_scriptListInfo             0x51C7C8
#define FO_VAR_skill_data                 0x51D118
#define FO_VAR_slot_cursor                0x5193B8
#define FO_VAR_sndfx_volume               0x518E90
#define FO_VAR_sneak_working              0x56D77C // DWORD var
#define FO_VAR_sound_music_path1          0x518E78
#define FO_VAR_sound_music_path2          0x518E7C
#define FO_VAR_square                     0x631E40
#define FO_VAR_squares                    0x66BE08
#define FO_VAR_stack                      0x59E86C
#define FO_VAR_stack_offset               0x59E844
#define FO_VAR_stat_data                  0x51D53C
#define FO_VAR_stat_flag                  0x66452A
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
#define FO_VAR_text_spacing               0x51E3CC
#define FO_VAR_text_to_buf                0x51E3B8
#define FO_VAR_text_width                 0x51E3C0
#define FO_VAR_tile                       0x631D30
#define FO_VAR_title_color                0x56D750
#define FO_VAR_title_font                 0x56D748
#define FO_VAR_trait_data                 0x51DB84
#define FO_VAR_view_page                  0x664520
#define FO_VAR_wd_obj                     0x59E98C
#define FO_VAR_wmAreaInfoList             0x51DDF8
#define FO_VAR_wmLastRndTime              0x51DEA0
#define FO_VAR_wmMaxMapNum                0x51DE10
#define FO_VAR_wmWorldOffsetX             0x51DE2C
#define FO_VAR_wmWorldOffsetY             0x51DE30
#define FO_VAR_wmYesNoStrs                0x51DD90
#define FO_VAR_world_xpos                 0x672E0C
#define FO_VAR_world_ypos                 0x672E10
#define FO_VAR_WorldMapCurrArea           0x672E08

// colors
#define FO_VAR_BlueColor                  0x6A38EF
#define FO_VAR_DarkGreenColor             0x6A3A90
#define FO_VAR_DullPinkColor              0x6AB718
#define FO_VAR_GoodColor                  0x6AB4EF
#define FO_VAR_GreenColor                 0x6A3CB0
#define FO_VAR_PeanutButter               0x6A82F3
#define FO_VAR_RedColor                   0x6AB4D0
#define FO_VAR_WhiteColor                 0x6AB8CF
#define FO_VAR_YellowColor                0x6AB8BB