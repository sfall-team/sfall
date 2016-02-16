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

#include "Logging.h"
#include "FalloutEngine.h"

// global variables
TGameObj** obj_dude_ptr = (TGameObj**)(0x6610B8);
TGameObj** inven_dude_ptr = (TGameObj**)(0x519058);
DWORD* activeUIHand_ptr = (DWORD*)(0x518F78); // 0 - left, 1 - right
DWORD* dude_traits = (DWORD*)(0x66BE40); // 2 of them
DWORD* itemCurrentItem = (DWORD*)(0x518F78); 
DWORD* itemButtonItems = (DWORD*)(0x5970F8);

/**
	ENGINE FUNCTIONS OFFSETS
	const names should end with underscore
*/


// AI FUNCTIONS
const DWORD ai_can_use_weapon_ = 0x4298EC;  // (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0

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
const DWORD ai_search_inven_armor_ = 0x429A6C;
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
const DWORD automap_ = 0x41B8BC;
const DWORD barter_compute_value_ = 0x474B2C;
const DWORD buf_to_buf_ = 0x4D36D4;
const DWORD check_death_ = 0x410814;
const DWORD Check4Keys_ = 0x43F73C;
const DWORD combat_ = 0x422D2C;
const DWORD combat_ai_ = 0x42B130;
const DWORD combat_attack_ = 0x422F3C;
const DWORD combat_input_ = 0x4227F4;
const DWORD combat_should_end_ = 0x422C60;
const DWORD combat_turn_ = 0x42299C;
const DWORD compute_damage_ = 0x4247B8;
const DWORD config_set_value_ = 0x42C160;
const DWORD correctFidForRemovedItem_ = 0x45409C;
const DWORD credits_ = 0x42C860;
const DWORD credits_get_next_line_ = 0x42CE6C;
const DWORD critter_body_type_ = 0x42DDC4;
const DWORD critter_can_obj_dude_rest_ = 0x42E564;
const DWORD critter_compute_ap_from_distance_ = 0x42E62C;
const DWORD critter_get_hits_ = 0x42D18C;
const DWORD critter_is_dead_ = 0x42DD18;
const DWORD critter_kill_ = 0x42DA64;
const DWORD critter_kill_count_type_ = 0x42D920;
const DWORD critter_name_ = 0x42D0A8;
const DWORD critter_pc_set_name_ = 0x42D138;
const DWORD critterClearObjDrugs_ = 0x42DA54;
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
const DWORD db_fseek_ = 0x4C60C0;
const DWORD db_fwriteByte_ = 0x4C61AC;
const DWORD db_fwriteByteCount_ = 0x4C6464;
const DWORD db_fwriteInt_ = 0x4C6214;
const DWORD db_read_to_buf_ = 0x4C5DD4;
const DWORD dbase_close_ = 0x4E5270;
const DWORD dbase_open_ = 0x4E4F58;
const DWORD debug_register_env_ = 0x4C6D90;
const DWORD determine_to_hit_func_ = 0x4243A8;
const DWORD dialog_out_ = 0x41CF20;
const DWORD display_inventory_ = 0x46FDF4;
const DWORD display_print_ = 0x43186C;
const DWORD display_target_inventory_ = 0x47036C;
const DWORD do_options_ = 0x48FC48;
const DWORD do_optionsFunc_ = 0x48FC50;
const DWORD do_prefscreen_ = 0x490798;
const DWORD DOSCmdLineDestroy_ = 0x4E3D3C;
const DWORD DrawCard_ = 0x43AAEC;
const DWORD DrawFolder_ = 0x43410C;
const DWORD DrawInfoWin_ = 0x4365AC;
const DWORD dude_stand_ = 0x418378;
const DWORD editor_design_ = 0x431DF8;
const DWORD elapsed_time_ = 0x4C93E0;
const DWORD elevator_end_ = 0x43F6D0;
const DWORD elevator_start_ = 0x43F324;
const DWORD endgame_slideshow_ = 0x43F788;
const DWORD exec_script_proc_ = 0x4A4810;
const DWORD executeProcedure_ = 0x46DD2C;
const DWORD fadeSystemPalette_ = 0x4C7320;
const DWORD findVar_ = 0x4410AC;
const DWORD folder_print_line_ = 0x43E3D8;
const DWORD frame_ptr_ = 0x419880;
const DWORD game_get_global_var_ = 0x443C68;
const DWORD game_help_ = 0x443F74;
const DWORD game_set_global_var_ = 0x443C98;
const DWORD game_time_date_ = 0x4A3338;
const DWORD gdialog_barter_cleanup_tables_ = 0x448660;
const DWORD gdProcess_ = 0x4465C0;
const DWORD get_input_ = 0x4C8B78;
const DWORD get_time_ = 0x4C9370;
const DWORD getmsg_ = 0x48504C; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory
const DWORD gmouse_is_scrolling_ = 0x44B54C;
const DWORD gmouse_set_cursor_ = 0x44C840;
const DWORD GNW_find_ = 0x4D7888;
const DWORD GNW95_process_message_ = 0x4C9CF0;
const DWORD gsnd_build_weapon_sfx_name_ = 0x451760;
const DWORD gsound_play_sfx_file_ = 0x4519A8;
const DWORD handle_inventory_ = 0x46E7B0;
const DWORD inc_game_time_ = 0x4A34CC;
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
const DWORD intface_redraw_ = 0x45EB98;
const DWORD intface_toggle_item_state_ = 0x45F4E0;
const DWORD intface_toggle_items_ = 0x45F404;
const DWORD intface_update_ac_ = 0x45EDA8;
const DWORD intface_update_hit_points_ = 0x45EBD8;
const DWORD intface_update_items_ = 0x45EFEC;
const DWORD intface_update_move_points_ = 0x45EE0C;
const DWORD intface_use_item_ = 0x45F5EC;
const DWORD invenUnwieldFunc_ = 0x472A64;
const DWORD invenWieldFunc_ = 0x472768;
const DWORD inven_display_msg_ = 0x472D24;
const DWORD inven_left_hand_ = 0x471BBC;
const DWORD inven_pid_is_carried_ptr_ = 0x471CA0;
const DWORD inven_right_hand_ = 0x471B70;
const DWORD inven_unwield_ = 0x472A54;
const DWORD inven_wield_ = 0x472758;
const DWORD inven_worn_ = 0x471C08;
const DWORD is_within_perception_ = 0x42BA04;
const DWORD isPartyMember_ = 0x494FC4;
const DWORD item_add_force_ = 0x4772B8;
const DWORD item_c_curr_size_ = 0x479A20;
const DWORD item_c_max_size_ = 0x479A00;
const DWORD item_caps_total_ = 0x47A6A8;
const DWORD item_d_check_addict_ = 0x47A640;
const DWORD item_d_take_drug_ = 0x479F60;
const DWORD item_get_type_ = 0x477AFC;
const DWORD item_m_dec_charges_ = 0x4795A4;
const DWORD item_m_turn_off_ = 0x479898;
const DWORD item_move_all_ = 0x4776AC;
const DWORD item_mp_cost_ = 0x478040;
const DWORD item_remove_mult_ = 0x477490;
const DWORD item_size_ = 0x477B68;
const DWORD item_total_cost_ = 0x477DAC;
const DWORD item_total_weight_ = 0x477E98;
const DWORD item_w_anim_code_ = 0x478DA8;
const DWORD item_w_anim_weap_ = 0x47860C;
const DWORD item_w_can_reload_ = 0x478874;
const DWORD item_w_compute_ammo_cost_ = 0x4790AC;
const DWORD item_w_cur_ammo_ = 0x4786A0;
const DWORD item_w_dam_div_ = 0x479294;
const DWORD item_w_dam_mult_ = 0x479230;
const DWORD item_w_damage_ = 0x478448;
const DWORD item_w_damage_type_ = 0x478570;
const DWORD item_w_dr_adjust_ = 0x4791E0;
const DWORD item_w_max_ammo_ = 0x478674;
const DWORD item_w_mp_cost_ = 0x478B24;
const DWORD item_w_range_ = 0x478A1C;
const DWORD item_w_try_reload_ = 0x478768;
const DWORD item_weight_ = 0x477B88;
const DWORD light_get_tile_ = 0x47A980;
const DWORD ListDrvdStats_ = 0x43527C;
const DWORD ListSkills_ = 0x436154;
const DWORD ListTraits_ = 0x43B8A8;
const DWORD loadColorTable_ = 0x4C78E4;
const DWORD LoadGame_ = 0x47C640;
const DWORD loadProgram_ = 0x4A3B74;
const DWORD LoadSlot_ = 0x47DC68;
const DWORD main_game_loop_ = 0x480E48;
const DWORD main_menu_hide_ = 0x481A00;
const DWORD main_menu_loop_ = 0x481AEC;
const DWORD make_path_func_ = 0x415EFC;
const DWORD make_straight_path_func_ = 0x4163C8;
const DWORD map_disable_bk_processes_ = 0x482104;
const DWORD map_enable_bk_processes_ = 0x4820C0;
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
const DWORD mouse_get_position_ = 0x4CA9DC;
const DWORD mouse_hide_ = 0x4CA534;
const DWORD mouse_show_ = 0x4CA34C;
const DWORD move_inventory_ = 0x474708;
const DWORD NixHotLines_ = 0x4999C0;
const DWORD obj_ai_blocking_at_ = 0x48BA20;
const DWORD obj_blocking_at_ = 0x48B848; // (EAX *obj, EDX hexNum, EBX level)
const DWORD obj_bound_ = 0x48B66C;
const DWORD obj_change_fid_ = 0x48AA3C;
const DWORD obj_connect_ = 0x489EC4;
const DWORD obj_destroy_ = 0x49B9A0;
const DWORD obj_dist_ = 0x48BBD4;
const DWORD obj_erase_object_ = 0x48B0FC;
const DWORD obj_find_first_at_ = 0x48B48C;
const DWORD obj_find_first_at_tile_ = 0x48B5A8;
const DWORD obj_find_next_at_ = 0x48B510;
const DWORD obj_find_next_at_tile_ = 0x48B608;
const DWORD obj_new_sid_inst_ = 0x49AAC0;
const DWORD obj_outline_object_ = 0x48C2B4;
const DWORD obj_pid_new_ = 0x489C9C;
const DWORD obj_remove_outline_ = 0x48C2F0;
const DWORD obj_save_dude_ = 0x48D59C;
const DWORD obj_scroll_blocking_at_ = 0x48BB44;
const DWORD obj_set_light_ = 0x48AC90; // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
const DWORD obj_shoot_blocking_at_ = 0x48B930; // (EAX *obj, EDX hexNum, EBX level)
const DWORD obj_sight_blocking_at_ = 0x48BB88;
const DWORD obj_use_book_ = 0x49B9F0;
const DWORD obj_use_power_on_car_ = 0x49BDE8;
const DWORD OptionWindow_ = 0x437C08;
const DWORD palette_set_to_ = 0x493B48;
const DWORD partyMemberCopyLevelInfo_ = 0x495EA8;
const DWORD partyMemberGetAIOptions_ = 0x4941F0;
const DWORD partyMemberGetCurLevel_ = 0x495FF0;
const DWORD partyMemberIncLevels_ = 0x495B60;
const DWORD partyMemberRemove_ = 0x4944DC;
const DWORD pc_flag_off_ = 0x42E220;
const DWORD pc_flag_on_ = 0x42E26C;
const DWORD pc_flag_toggle_ = 0x42E2B0;
const DWORD perform_withdrawal_end_ = 0x47A558;
const DWORD perk_add_ = 0x496A5C;
const DWORD perk_add_effect_ = 0x496BFC;
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
const DWORD proto_ptr_ = 0x4A2108; // eax - PID, edx - int** - pointer to a pointer to a proto struct
const DWORD pushLongStack_ = 0x46736C; // sometimes used instead of "SetResult"
const DWORD qsort_ = 0x4F05B6;
const DWORD queue_add_ = 0x4A258C;
const DWORD queue_clear_type_ = 0x4A2790;
const DWORD queue_find_ = 0x4A26A8;
const DWORD queue_find_first_ = 0x4A295C;
const DWORD queue_find_next_ = 0x4A2994;
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
const DWORD scr_new_ = 0x4A5F28;
const DWORD scr_ptr_ = 0x4A5E34;
const DWORD scr_remove_ = 0x4A61D4;
const DWORD scr_set_ext_param_ = 0x4A3B34;
const DWORD scr_set_objs_ = 0x4A3B0C;
const DWORD scr_write_ScriptNode_ = 0x4A5704;
const DWORD set_game_time_ = 0x4A347C;
const DWORD SexWindow_ = 0x437664;
const DWORD skill_check_stealing_ = 0x4ABBE4;
const DWORD skill_dec_point_ = 0x4AA8C4;
const DWORD skill_get_tags_ = 0x4AA508;
const DWORD skill_inc_point_ = 0x4AA6BC;
const DWORD skill_level_ = 0x4AA558;
const DWORD skill_points_ = 0x4AA680;
const DWORD skill_set_tags_ = 0x4AA4E4;
const DWORD skill_use_ = 0x4AAD08;
const DWORD skilldex_select_ = 0x4ABFD0;
const DWORD sprintf_ = 0x4F0041;
const DWORD square_num_ = 0x4B1F04;
const DWORD stat_get_base_direct_ = 0x4AF408;
const DWORD stat_get_bonus_ = 0x4AF474;
const DWORD stat_level_ = 0x4AEF48; // &GetCurrentStat(void* critter, int statID)
const DWORD stat_pc_add_experience_ = 0x4AFAA8;
const DWORD stat_pc_get_ = 0x4AF8FC;
const DWORD stat_pc_set_ = 0x4AF910;
const DWORD strncpy_ = 0x4F014F;
const DWORD switch_hand_ = 0x4714E0;
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
const DWORD win_show_ = 0x4D6DAC;
const DWORD wmInterfaceScrollTabsStart_ = 0x4C219C;
const DWORD wmPartyWalkingStep_ = 0x4C1F90;
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
const DWORD xrewind_ = 0x4DF6E4;
const DWORD xungetc_ = 0x4DF3F4;
const DWORD xvfprintf_ = 0x4DF1AC;


// WRAPPERS
// please, use CamelCase for those


int __stdcall ItemGetType(TGameObj* item) {
	__asm {
		mov eax, item;
		call item_get_type_;
	}
}

int _stdcall IsPartyMember(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call isPartyMember_;
	}
}

TGameObj* GetInvenWeaponLeft(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call inven_left_hand_;
	}
}

TGameObj* GetInvenWeaponRight(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call inven_right_hand_;
	}
}


char* GetProtoPtr(DWORD pid) {
	char* proto;
	__asm {
		mov eax, pid;
		lea edx, proto;
		call proto_ptr_;
	}
	return proto;
}

char AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != NULL) {
		char* proto = GetProtoPtr(weapon->pid);
		if (proto && *(int*)(proto + 32) == 3) {
			return (char)(*(int*)(proto + 36)); 
		}
	}
	return 0;
}


void DisplayConsoleMessage(const char* msg) {
	__asm {
		mov eax, msg;
		call display_print_;
	}
}

static DWORD mesg_buf[4] = {0, 0, 0, 0};
const char* _stdcall GetMessageStr(DWORD fileAddr, DWORD messageId)
{
	DWORD buf = (DWORD)mesg_buf;
	const char* result;
	__asm {
		mov eax, fileAddr
		mov ebx, messageId
		mov edx, buf
		call getmsg_
		mov result, eax
	}
	return result;
}
