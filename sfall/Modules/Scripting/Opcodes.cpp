/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\..\FalloutEngine\Fallout2.h"
#include "..\KillCounter.h"
#include "..\LoadGameHook.h"

#include "Handlers\Anims.h"
#include "Handlers\Arrays.h"
#include "Handlers\Combat.h"
#include "Handlers\Core.h"
#include "Handlers\FileSystem.h"
#include "Handlers\Graphics.h"
#include "Handlers\IniFiles.h"
#include "Handlers\Interface.h"
#include "Handlers\Inventory.h"
#include "Handlers\Math.h"
#include "Handlers\Memory.h"
#include "Handlers\Misc.h"
#include "Handlers\Objects.h"
#include "Handlers\Perks.h"
#include "Handlers\Stats.h"
#include "Handlers\Utils.h"
#include "Handlers\Worldmap.h"
#include "Handlers\Metarule.h"
#include "OpcodeContext.h"

#include "Opcodes.h"

namespace sfall
{
namespace script
{

static const short sfallOpcodeStart = 0x156;
static const short opcodeCount = 0x300;

// "Raw" opcode table. Each value points to a "raw" handler function that is called directly by engine.
// First half is filled with vanilla opcodes by engine itself.
// Other half is filled by sfall here.
static void* opcodes[opcodeCount];

// Opcode Table. Add additional (sfall) opcodes here.
// Format: {
//    opcode number,
//    function name,
//    function handler,
//    number of arguments (max 7),
//    has return value,
//    returned error value for argument validation,
//    { argument 1 type, argument 2 type, ...}
// }
static SfallOpcodeInfo opcodeInfoArray[] = {
	{0x15a, "set_pc_base_stat",           op_set_pc_base_stat,          2, false,  0, {ARG_INT, ARG_INT}},
	{0x15b, "set_pc_extra_stat",          op_set_pc_extra_stat,         2, false,  0, {ARG_INT, ARG_INT}},
	{0x15c, "get_pc_base_stat",           op_get_pc_base_stat,          1, true,   0, {ARG_INT}},
	{0x15d, "get_pc_extra_stat",          op_get_pc_extra_stat,         1, true,   0, {ARG_INT}},
	{0x15e, "set_critter_base_stat",      op_set_critter_base_stat,     3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x15f, "set_critter_extra_stat",     op_set_critter_extra_stat,    3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x160, "get_critter_base_stat",      op_get_critter_base_stat,     2, true,   0, {ARG_OBJECT, ARG_INT}},
	{0x161, "get_critter_extra_stat",     op_get_critter_extra_stat,    2, true,   0, {ARG_OBJECT, ARG_INT}},
	{0x163, "get_year",                   op_get_year,                  0, true},
	{0x16c, "key_pressed",                op_key_pressed,               1, true,   0, {ARG_INT}},
	{0x171, "force_encounter",            op_force_encounter,           1, false,  0, {ARG_INT}},
	{0x175, "set_dm_model",               op_set_dm_model,              1, false,  0, {ARG_STRING}},
	{0x176, "set_df_model",               op_set_df_model,              1, false,  0, {ARG_STRING}},
	{0x177, "set_movie_path",             op_set_movie_path,            2, false,  0, {ARG_STRING, ARG_INT}},

	{0x189, "set_perk_name",              op_set_perk_name,             2, false,  0, {ARG_INT, ARG_STRING}},
	{0x18a, "set_perk_desc",              op_set_perk_desc,             2, false,  0, {ARG_INT, ARG_STRING}},
	{0x18b, "set_pipboy_available",       op_set_pipboy_available,      1, false,  0, {ARG_INT}},

	{0x190, "get_perk_available",         op_get_perk_available,        1, true,   0, {ARG_INT}},
	{0x192, "set_critter_current_ap",     op_set_critter_current_ap,    2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x195, "set_weapon_knockback",       op_set_object_knockback,      3, false,  0, {ARG_OBJECT, ARG_INT, ARG_NUMBER}},
	{0x196, "set_target_knockback",       op_set_object_knockback,      3, false,  0, {ARG_OBJECT, ARG_INT, ARG_NUMBER}},
	{0x197, "set_attacker_knockback",     op_set_object_knockback,      3, false,  0, {ARG_OBJECT, ARG_INT, ARG_NUMBER}},
	{0x198, "remove_weapon_knockback",    op_remove_object_knockback,   1, false,  0, {ARG_OBJECT}},
	{0x199, "remove_target_knockback",    op_remove_object_knockback,   1, false,  0, {ARG_OBJECT}},
	{0x19a, "remove_attacker_knockback",  op_remove_object_knockback,   1, false,  0, {ARG_OBJECT}},
	{0x19d, "set_sfall_global",           op_set_sfall_global,          2, false,  0, {ARG_INTSTR, ARG_NUMBER}},
	{0x19e, "get_sfall_global_int",       op_get_sfall_global_int,      1, true,   0, {ARG_INTSTR}},
	{0x19f, "get_sfall_global_float",     op_get_sfall_global_float,    1, true,   0, {ARG_INTSTR}},
	{0x1a5, "inc_npc_level",              op_inc_npc_level,             1, false,  0, {ARG_INTSTR}},
	{0x1aa, "set_xp_mod",                 op_set_xp_mod,                1, false,  0, {ARG_INT}},
	{0x1ac, "get_ini_setting",            op_get_ini_setting,           1, true,  -1, {ARG_STRING}},

	{0x1bb, "set_fake_perk",              op_set_fake_perk,             4, false,  0, {ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{0x1bc, "set_fake_trait",             op_set_fake_trait,            4, false,  0, {ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{0x1bd, "set_selectable_perk",        op_set_selectable_perk,       4, false,  0, {ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{0x1c1, "has_fake_perk",              op_has_fake_perk,             1, true,   0, {ARG_INTSTR}},
	{0x1c2, "has_fake_trait",             op_has_fake_trait,            1, true,   0, {ARG_STRING}},
	{0x1c5, "set_critter_hit_chance_mod", op_set_critter_hit_chance_mod,3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x1c7, "set_critter_skill_mod",      op_set_critter_skill_mod,     2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x1c9, "set_critter_pickpocket_mod", op_set_critter_pickpocket_mod,3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},

	{0x1dc, "show_iface_tag",             op_show_iface_tag,            1, false,  0, {ARG_INT}},
	{0x1dd, "hide_iface_tag",             op_hide_iface_tag,            1, false,  0, {ARG_INT}},
	{0x1de, "is_iface_tag_active",        op_is_iface_tag_active,       1, true,   0, {ARG_INT}},
	{0x1e1, "set_critical_table",         op_set_critical_table,        5, false,  0, {ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{0x1e2, "get_critical_table",         op_get_critical_table,        4, true,   0, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{0x1e3, "reset_critical_table",       op_reset_critical_table,      4, false,  0, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{0x1e4, "get_sfall_arg",              op_get_sfall_arg,             0, true},
	{0x1e5, "set_sfall_return",           op_set_sfall_return,          1, false,  0, {ARG_ANY}}, // hook script system will validate type
	{0x1eb, "get_ini_string",             op_get_ini_string,            1, true,  -1, {ARG_STRING}},
	{0x1ec, "sqrt",                       op_sqrt,                      1, true,   0, {ARG_NUMBER}},
	{0x1ed, "abs",                        op_abs,                       1, true,   0, {ARG_NUMBER}},
	{0x1ee, "sin",                        op_sin,                       1, true,   0, {ARG_NUMBER}},
	{0x1ef, "cos",                        op_cos,                       1, true,   0, {ARG_NUMBER}},
	{0x1f0, "tan",                        op_tan,                       1, true,   0, {ARG_NUMBER}},
	{0x1f1, "arctan",                     op_arctan,                    2, true,   0, {ARG_NUMBER, ARG_NUMBER}},
	{0x1f2, "set_palette",                op_set_palette,               1, false,  0, {ARG_STRING}},
	{0x1f3, "remove_script",              op_remove_script,             1, false,  0, {ARG_OBJECT}},
	{0x1f4, "set_script",                 op_set_script,                2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x1f5, "get_script",                 op_get_script,                1, true,  -1, {ARG_OBJECT}},

	{0x1f7, "fs_create",                  op_fs_create,                 2, true,  -1, {ARG_STRING, ARG_INT}},
	{0x1f8, "fs_copy",                    op_fs_copy,                   2, true,  -1, {ARG_STRING, ARG_STRING}},
	{0x1f9, "fs_find",                    op_fs_find,                   1, true,  -1, {ARG_STRING}},
	{0x1fa, "fs_write_byte",              op_fs_write_byte,             2, false,  0, {ARG_INT, ARG_INT}},
	{0x1fb, "fs_write_short",             op_fs_write_short,            2, false,  0, {ARG_INT, ARG_INT}},
	{0x1fc, "fs_write_int",               op_fs_write_int,              2, false,  0, {ARG_INT, ARG_INT}},
	{0x1fd, "fs_write_int",               op_fs_write_int,              2, false,  0, {ARG_INT, ARG_INT}},
	{0x1fe, "fs_write_string",            op_fs_write_string,           2, false,  0, {ARG_INT, ARG_STRING}},
	{0x1ff, "fs_delete",                  op_fs_delete,                 1, false,  0, {ARG_INT}},
	{0x200, "fs_size",                    op_fs_size,                   1, true,   0, {ARG_INT}},
	{0x201, "fs_pos",                     op_fs_pos,                    1, true,  -1, {ARG_INT}},
	{0x202, "fs_seek",                    op_fs_seek,                   2, false,  0, {ARG_INT, ARG_INT}},
	{0x203, "fs_resize",                  op_fs_resize,                 2, false,  0, {ARG_INT, ARG_INT}},
	{0x204, "get_proto_data",             op_get_proto_data,            2, true,  -1, {ARG_INT, ARG_INT}},
	{0x205, "set_proto_data",             op_set_proto_data,            3, false,  0, {ARG_INT, ARG_INT, ARG_INT}},
	{0x207, "register_hook",              op_register_hook,             1, false,  0, {ARG_INT}},
	{0x208, "fs_write_bstring",           op_fs_write_bstring,          2, false,  0, {ARG_INT, ARG_STRING}},
	{0x209, "fs_read_byte",               op_fs_read_byte,              1, true,   0, {ARG_INT}},
	{0x20a, "fs_read_short",              op_fs_read_short,             1, true,   0, {ARG_INT}},
	{0x20b, "fs_read_int",                op_fs_read_int,               1, true,   0, {ARG_INT}},
	{0x20c, "fs_read_float",              op_fs_read_float,             1, true,   0, {ARG_INT}},
	{0x20d, "list_begin",                 op_list_begin,                1, true,   0, {ARG_INT}},
	{0x20e, "list_next",                  op_list_next,                 1, true,   0, {ARG_INT}},
	{0x20f, "list_end",                   op_list_end,                  1, false,  0, {ARG_INT}},
	{0x216, "set_critter_burst_disable",  op_set_critter_burst_disable, 2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x217, "get_weapon_ammo_pid",        op_get_weapon_ammo_pid,       1, true,  -1, {ARG_OBJECT}},
	{0x218, "set_weapon_ammo_pid",        op_set_weapon_ammo_pid,       2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x219, "get_weapon_ammo_count",      op_get_weapon_ammo_count,     1, true,   0, {ARG_OBJECT}},
	{0x21a, "set_weapon_ammo_count",      op_set_weapon_ammo_count,     2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x21e, "get_mouse_buttons",          op_get_mouse_buttons,         0, true},

	{0x224, "create_message_window",      op_create_message_window,     1, false,  0, {ARG_STRING}},
	{0x228, "get_attack_type",            op_get_attack_type,           0, true},
	{0x229, "force_encounter_with_flags", op_force_encounter,           2, false,  0, {ARG_INT, ARG_INT}},
	{0x22a, "set_map_time_multi",         op_set_map_time_multi,        1, false,  0, {ARG_NUMBER}},
	{0x22b, "play_sfall_sound",           op_play_sfall_sound,          2, true,   0, {ARG_STRING, ARG_INT}},
	{0x22d, "create_array",               op_create_array,              2, true,  -1, {ARG_INT, ARG_INT}},
	{0x22e, "set_array",                  op_set_array,                 3, false,  0, {ARG_OBJECT, ARG_ANY, ARG_ANY}},
	{0x22f, "get_array",                  op_get_array,                 2, true},  // can also be used on strings
	{0x230, "free_array",                 op_free_array,                1, false,  0, {ARG_OBJECT}},
	{0x231, "len_array",                  op_len_array,                 1, true,   0, {ARG_INT}},
	{0x232, "resize_array",               op_resize_array,              2, false,  0, {ARG_OBJECT, ARG_INT}},
	{0x233, "temp_array",                 op_temp_array,                2, true,  -1, {ARG_INT, ARG_INT}},
	{0x234, "fix_array",                  op_fix_array,                 1, false,  0, {ARG_OBJECT}},
	{0x235, "string_split",               op_string_split,              2, true,  -1, {ARG_STRING, ARG_STRING}},
	{0x236, "list_as_array",              op_list_as_array,             1, true,  -1, {ARG_INT}},
	{0x237, "atoi",                       op_atoi,                      1, true,   0, {ARG_STRING}},
	{0x238, "atof",                       op_atof,                      1, true,   0, {ARG_STRING}},
	{0x239, "scan_array",                 op_scan_array,                2, true,  -1, {ARG_OBJECT, ARG_ANY}},
	{0x23a, "get_tile_fid",               op_get_tile_fid,              1, true,   0, {ARG_INT}},
	{0x23b, "modified_ini",               op_modified_ini,              0, true},
	{0x23c, "get_sfall_args",             op_get_sfall_args,            0, true},
	{0x23d, "set_sfall_arg",              op_set_sfall_arg,             2, false,  0, {ARG_INT, ARG_ANY}}, // hook script system will validate type
	{0x241, "get_npc_level",              op_get_npc_level,             1, true,  -1, {ARG_INTSTR}},
	{0x242, "set_critter_skill_points",   op_set_critter_skill_points,  3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x243, "get_critter_skill_points",   op_get_critter_skill_points,  2, true,   0, {ARG_OBJECT, ARG_INT}},

	{0x24e, "substr",                     op_substr,                    3, true,  -1, {ARG_STRING, ARG_INT, ARG_INT}},
	{0x24f, "strlen",                     op_strlen,                    1, true,   0, {ARG_STRING}},
	{0x250, "sprintf",                    op_sprintf,                   2, true,   0, {ARG_STRING, ARG_ANY}},
	{0x251, "charcode",                   op_ord,                       1, true,   0, {ARG_STRING}},
	// 0x252 // RESERVED
	{0x253, "typeof",                     op_typeof,                    1, true},
	{0x254, "save_array",                 op_save_array,                2, false,  0, {ARG_ANY, ARG_OBJECT}},
	{0x255, "load_array",                 op_load_array,                1, true,  -1, {ARG_INTSTR}},
	{0x256, "array_key",                  op_get_array_key,             2, true,   0, {ARG_INT, ARG_INT}},
	{0x257, "arrayexpr",                  op_stack_array,               2, true},
	// 0x258 // RESERVED for arrays
	// 0x259 // RESERVED for arrays
	{0x25a, "reg_anim_destroy",           op_reg_anim_destroy,          1, false,  0, {ARG_OBJECT}},
	{0x25b, "reg_anim_animate_and_hide",  op_reg_anim_animate_and_hide, 3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x25c, "reg_anim_combat_check",      op_reg_anim_combat_check,     1, false,  0, {ARG_INT}},
	{0x25d, "reg_anim_light",             op_reg_anim_light,            3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x25e, "reg_anim_change_fid",        op_reg_anim_change_fid,       3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x25f, "reg_anim_take_out",          op_reg_anim_take_out,         3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x260, "reg_anim_turn_towards",      op_reg_anim_turn_towards,     3, false,  0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x261, "metarule2_explosions",       op_explosions_metarule,       3, true,  -1, {ARG_INT, ARG_INT, ARG_INT}},
	{0x262, "register_hook_proc",         op_register_hook,             2, false,  0, {ARG_INT, ARG_INT}},
	{0x263, "power",                      op_power,                     2, true,   0, {ARG_NUMBER, ARG_NUMBER}}, // '^' operator
	{0x264, "log",                        op_log,                       1, true,   0, {ARG_NUMBER}},
	{0x265, "exponent",                   op_exponent,                  1, true,   0, {ARG_NUMBER}},
	{0x266, "ceil",                       op_ceil,                      1, true,   0, {ARG_NUMBER}},
	{0x267, "round",                      op_round,                     1, true,   0, {ARG_NUMBER}},
	// 0x268 RESERVED
	// 0x269 RESERVED
	// 0x26a RESERVED
	{0x26b, "message_str_game",           op_message_str_game,          2, true,   0, {ARG_INT, ARG_INT}},
	{0x26c, "sneak_success",              op_sneak_success,             0, true},
	{0x26d, "tile_light",                 op_tile_light,                2, true,  -1, {ARG_INT, ARG_INT}},
	{0x26e, "obj_blocking_line",          op_make_straight_path,        3, true,   0, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x26f, "obj_blocking_tile",          op_obj_blocking_at,           3, true,   0, {ARG_INT, ARG_INT, ARG_INT}},
	{0x270, "tile_get_objs",              op_tile_get_objects,          2, true,  -1, {ARG_INT, ARG_INT}},
	{0x271, "party_member_list",          op_get_party_members,         1, true,  -1, {ARG_INT}},
	{0x272, "path_find_to",               op_make_path,                 3, true,  -1, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{0x273, "create_spatial",             op_create_spatial,            4, true,   0, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{0x274, "art_exists",                 op_art_exists,                1, true,   0, {ARG_INT}},
	{0x275, "obj_is_carrying_obj",        op_obj_is_carrying_obj,       2, true,   0, {ARG_OBJECT, ARG_OBJECT}},

	// universal opcodes:
	{0x276, "sfall_func0", HandleMetarule, 1, true},
	{0x277, "sfall_func1", HandleMetarule, 2, true},
	{0x278, "sfall_func2", HandleMetarule, 3, true},
	{0x279, "sfall_func3", HandleMetarule, 4, true},
	{0x27a, "sfall_func4", HandleMetarule, 5, true},
	{0x27b, "sfall_func5", HandleMetarule, 6, true},
	{0x27c, "sfall_func6", HandleMetarule, 7, true},

	{0x27d, "register_hook_proc_spec",    op_register_hook,             2, false,  0, {ARG_INT, ARG_INT}},
	{0x27e, "reg_anim_callback",          op_reg_anim_callback,         1, false,  0, {ARG_INT}},
	{0x27f, "div",                        op_div,                       2, true,   0, {ARG_NUMBER, ARG_NUMBER}}, // div operator

	{0x280, "sfall_func7", HandleMetarule, 8, true},
	{0x281, "sfall_func8", HandleMetarule, 9, true}, // if you need more arguments - use arrays
};

// An array for opcode info, indexed by opcode.
// Initialized at run time from the array above.
static std::array<const SfallOpcodeInfo*, opcodeCount - sfallOpcodeStart> opcodeInfoTable;

// Initializes the opcode info table.
static void InitOpcodeInfoTable() {
	int length = sizeof(opcodeInfoArray) / sizeof(opcodeInfoArray[0]);
	for (int i = 0; i < length; ++i) {
		// index: opcode, value: reference to opcode element in the opcodeInfoArray array
		opcodeInfoTable[opcodeInfoArray[i].opcode - sfallOpcodeStart] = &opcodeInfoArray[i];
	}
}

// Default handler for Sfall Opcodes.
// Searches current opcode in Opcode Info table and executes the appropriate handler.
static void __fastcall defaultOpcodeHandler(fo::Program* program, DWORD opcodeOffset) { // eax/ebx - program, edx - opcodeOffset
	__asm push ecx;
	__asm mov  program, ebx;

	int opcode = opcodeOffset / 4;
	auto opcodeInfo = opcodeInfoTable[opcode - sfallOpcodeStart];
	if (opcodeInfo != nullptr) {
		OpcodeContext ctx(program, opcodeInfo);
		ctx.handleOpcode(opcodeInfo->handler, opcodeInfo->argValidation);
	} else {
		fo::func::interpretError("Unknown opcode: %d", opcode);
	}
	__asm pop ecx;
}

void Opcodes::InitNew() {
	dlog("Adding sfall opcodes.", DL_SCRIPT);

	SafeWrite32(0x46E370, opcodeCount);    // Maximum number of allowed opcodes
	SafeWrite32(0x46CE34, (DWORD)opcodes); // cmp check to make sure opcode exists
	SafeWrite32(0x46CE6C, (DWORD)opcodes); // call that actually jumps to the opcode
	SafeWrite32(0x46E390, (DWORD)opcodes); // mov that writes to the opcode

	// see opcodeMetaArray above for additional scripting functions via "metarule"

	InitOpcodeInfoTable();
	InitMetaruleTable();

	SetExtraKillCounter(KillCounter::UsingExtraKillTypes());

	LoadGameHook::OnGameReset() += []() {
		PipboyAvailableRestore();
		ForceEncounterRestore(); // restore if the encounter did not happen
		ResetIniCache();
	};

	if (int unsafe = IniReader::GetIntDefaultConfig("Debugging", "AllowUnsafeScripting", 0)) {
		unsafeEnabled = true;
		if (unsafe == 2) checkValidMemAddr = false;
		dlogr(" Unsafe opcodes enabled.", DL_SCRIPT);
	} else {
		dlogr(" Unsafe opcodes disabled.", DL_SCRIPT);
	}
	opcodes[0x1cf] = op_write_byte;
	opcodes[0x1d0] = op_write_short;
	opcodes[0x1d1] = op_write_int;
	opcodes[0x21b] = op_write_string;
	for (int i = 0x1d2; i < 0x1dc; i++) {
		opcodes[i] = op_call_offset;
	}
	opcodes[0x156] = op_read_byte;
	opcodes[0x157] = op_read_short;
	opcodes[0x158] = op_read_int;
	opcodes[0x159] = op_read_string;

	opcodes[0x162] = op_tap_key;
	opcodes[0x164] = op_game_loaded;
	opcodes[0x165] = op_graphics_funcs_available;
	opcodes[0x166] = op_load_shader;
	opcodes[0x167] = op_free_shader;
	opcodes[0x168] = op_activate_shader;
	opcodes[0x169] = op_deactivate_shader;
	opcodes[0x16a] = op_set_global_script_repeat;
	opcodes[0x16b] = op_input_funcs_available;

	opcodes[0x16d] = op_set_shader_int;
	opcodes[0x16e] = op_set_shader_float;
	opcodes[0x16f] = op_set_shader_vector;
	opcodes[0x170] = op_in_world_map;
	opcodes[0x172] = op_set_world_map_pos;
	opcodes[0x173] = op_get_world_map_x_pos;
	opcodes[0x174] = op_get_world_map_y_pos;

	for (int i = 0x178; i < 0x189; i++) {
		opcodes[i] = op_set_perk_value;
	}
	opcodes[0x18c] = op_get_kill_counter;
	opcodes[0x18d] = op_mod_kill_counter;
	opcodes[0x18e] = op_get_perk_owed;
	opcodes[0x18f] = op_set_perk_owed;
	opcodes[0x191] = op_get_critter_current_ap;
	opcodes[0x193] = op_active_hand;
	opcodes[0x194] = op_toggle_active_hand;
	opcodes[0x19b] = op_set_global_script_type;
	opcodes[0x19c] = op_available_global_script_types;
	opcodes[0x1a0] = op_set_pickpocket_max;
	opcodes[0x1a1] = op_set_hit_chance_max;
	opcodes[0x1a2] = op_set_skill_max;
	opcodes[0x1a3] = op_eax_available;
	opcodes[0x1a4] = op_set_eax_environment;
	opcodes[0x1a6] = op_get_viewport_x;
	opcodes[0x1a7] = op_get_viewport_y;
	opcodes[0x1a8] = op_set_viewport_x;
	opcodes[0x1a9] = op_set_viewport_y;
	opcodes[0x1ab] = op_set_perk_level_mod;
	opcodes[0x1ad] = op_get_shader_version;
	opcodes[0x1ae] = op_set_shader_mode;
	opcodes[0x1af] = op_get_game_mode;
	opcodes[0x1b0] = op_force_graphics_refresh;
	opcodes[0x1b1] = op_get_shader_texture;
	opcodes[0x1b2] = op_set_shader_texture;
	opcodes[0x1b3] = op_get_uptime;
	opcodes[0x1b4] = op_set_stat_max;
	opcodes[0x1b5] = op_set_stat_min;
	opcodes[0x1b6] = op_set_car_current_town;
	opcodes[0x1b7] = op_set_pc_stat_max;
	opcodes[0x1b8] = op_set_pc_stat_min;
	opcodes[0x1b9] = op_set_npc_stat_max;
	opcodes[0x1ba] = op_set_npc_stat_min;
	opcodes[0x1be] = op_set_perkbox_title;
	opcodes[0x1bf] = op_hide_real_perks;
	opcodes[0x1c0] = op_show_real_perks;
	opcodes[0x1c3] = op_perk_add_mode;
	opcodes[0x1c4] = op_clear_selectable_perks;

	opcodes[0x1c6] = op_set_base_hit_chance_mod;
	opcodes[0x1c8] = op_set_base_skill_mod;

	opcodes[0x1ca] = op_set_base_pickpocket_mod;
	opcodes[0x1cb] = op_set_pyromaniac_mod;
	opcodes[0x1cc] = op_apply_heaveho_fix;
	opcodes[0x1cd] = op_set_swiftlearner_mod;
	opcodes[0x1ce] = op_set_hp_per_level_mod;

	opcodes[0x1df] = op_get_bodypart_hit_modifier;
	opcodes[0x1e0] = op_set_bodypart_hit_modifier;
	opcodes[0x1e6] = op_set_unspent_ap_bonus;
	opcodes[0x1e7] = op_get_unspent_ap_bonus;
	opcodes[0x1e8] = op_set_unspent_ap_perk_bonus;
	opcodes[0x1e9] = op_get_unspent_ap_perk_bonus;
	opcodes[0x1ea] = op_init_hook;

	opcodes[0x1f6] = op_nb_create_char;
	opcodes[0x206] = op_set_self;
	opcodes[0x210] = op_sfall_ver_major;
	opcodes[0x211] = op_sfall_ver_minor;
	opcodes[0x212] = op_sfall_ver_build;
	opcodes[0x213] = op_hero_select_win;
	opcodes[0x214] = op_set_hero_race;
	opcodes[0x215] = op_set_hero_style;

	opcodes[0x21c] = op_get_mouse_x;
	opcodes[0x21d] = op_get_mouse_y;
	opcodes[0x21f] = op_get_window_under_mouse;
	opcodes[0x220] = op_get_screen_width;
	opcodes[0x221] = op_get_screen_height;
	opcodes[0x222] = op_stop_game;
	opcodes[0x223] = op_resume_game;
	opcodes[0x225] = op_remove_trait;
	opcodes[0x226] = op_get_light_level;
	opcodes[0x227] = op_refresh_pc_art;
	opcodes[0x22c] = op_stop_sfall_sound;

	opcodes[0x23e] = op_force_aimed_shots;
	opcodes[0x23f] = op_disable_aimed_shots;
	opcodes[0x240] = op_mark_movie_played;
	opcodes[0x244] = op_set_available_skill_points;
	opcodes[0x245] = op_get_available_skill_points;
	opcodes[0x246] = op_mod_skill_points_per_level;
	opcodes[0x247] = op_set_perk_freq;
	opcodes[0x248] = op_get_last_attacker;
	opcodes[0x249] = op_get_last_target;
	opcodes[0x24a] = op_block_combat;
	opcodes[0x24b] = op_tile_under_cursor;
	opcodes[0x24c] = op_gdialog_get_barter_mod;
	opcodes[0x24d] = op_set_inven_ap_cost;

	// configure default opcode handler
	for (int i = sfallOpcodeStart; i < opcodeCount; i++) {
		if (opcodes[i] == nullptr) {
			opcodes[i] = defaultOpcodeHandler;
		}
	}
}

}
}
