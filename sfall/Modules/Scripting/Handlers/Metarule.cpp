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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"
#include "Anims.h"
#include "Combat.h"
#include "Core.h"
#include "IniFiles.h"
#include "Interface.h"
#include "Inventory.h"
#include "Math.h"
#include "Misc.h"
#include "Objects.h"
#include "Perks.h"
#include "Stats.h"
#include "Utils.h"
#include "Worldmap.h"

#include "Metarule.h"

#ifndef NDEBUG
#include <sstream>
#endif

namespace sfall
{
namespace script
{

// Metarule is a universal opcode(s) for all kinds of new sfall scripting functions.
// Prefix all function handlers with mf_ and add them to sfall_metarule_table.
// DO NOT add arguments and/or return values to function handlers!
// Use ctx.arg(i), inside handler function to access arguments.
// Use ctx.setReturn(x) to set return value.
// If you want to call user-defined procedures in your handler, use RunScriptProc().

typedef std::unordered_map<std::string, const SfallMetarule*> MetaruleTableType;

static MetaruleTableType metaruleTable;

/*
	Metarule AKA sfall_funcX.

	Add your custom scripting functions here.

	Format is as follows:
	{ name, handler, minArgs, maxArgs, error, {arg1, arg2, ...} }
		- name - name of function that will be used in scripts,
		- handler - pointer to handler function (see examples below),
		- minArgs/maxArgs - minimum and maximum number of arguments allowed for this function (max 8)
		- returned error value for argument validation,
		- arg1, arg2, ... - argument types for automatic validation
*/
static const SfallMetarule metarules[] = {
	{"add_extra_msg_file",        mf_add_extra_msg_file,        1, 2, -1, {ARG_STRING, ARG_INT}},
	{"add_iface_tag",             mf_add_iface_tag,             0, 0},
	{"add_g_timer_event",         mf_add_g_timer_event,         2, 2, -1, {ARG_INT, ARG_INT}},
	{"add_trait",                 mf_add_trait,                 1, 1, -1, {ARG_INT}},
	{"art_cache_clear",           mf_art_cache_flush,           0, 0},
	{"art_frame_data",            mf_art_frame_data,            1, 3,  0, {ARG_INTSTR, ARG_INT, ARG_INT}},
	{"attack_is_aimed",           mf_attack_is_aimed,           0, 0},
	{"car_gas_amount",            mf_car_gas_amount,            0, 0},
	{"combat_data",               mf_combat_data,               0, 0},
	{"create_win",                mf_create_win,                5, 6, -1, {ARG_STRING, ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"critter_inven_obj2",        mf_critter_inven_obj2,        2, 2,  0, {ARG_OBJECT, ARG_INT}},
	{"dialog_message",            mf_dialog_message,            1, 1, -1, {ARG_STRING}},
	{"dialog_obj",                mf_get_dialog_object,         0, 0},
	{"display_stats",             mf_display_stats,             0, 0}, // refresh
	{"draw_image",                mf_draw_image,                1, 5, -1, {ARG_INTSTR, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"draw_image_scaled",         mf_draw_image_scaled,         1, 6, -1, {ARG_INTSTR, ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"exec_map_update_scripts",   mf_exec_map_update_scripts,   0, 0},
	{"floor2",                    mf_floor2,                    1, 1,  0, {ARG_NUMBER}},
	{"get_can_rest_on_map",       mf_get_rest_on_map,           2, 2, -1, {ARG_INT, ARG_INT}},
	{"get_combat_free_move",      mf_get_combat_free_move,      0, 0},
	{"get_current_inven_size",    mf_get_current_inven_size,    1, 1,  0, {ARG_OBJECT}},
	{"get_cursor_mode",           mf_get_cursor_mode,           0, 0},
	{"get_flags",                 mf_get_flags,                 1, 1,  0, {ARG_OBJECT}},
	{"get_ini_config",            mf_get_ini_config,            2, 2,  0, {ARG_STRING, ARG_INT}},
	{"get_ini_section",           mf_get_ini_section,           2, 2, -1, {ARG_STRING, ARG_STRING}},
	{"get_ini_sections",          mf_get_ini_sections,          1, 1, -1, {ARG_STRING}},
	{"get_inven_ap_cost",         mf_get_inven_ap_cost,         0, 0},
	{"get_map_enter_position",    mf_get_map_enter_position,    0, 0},
	{"get_metarule_table",        mf_get_metarule_table,        0, 0},
	{"get_object_ai_data",        mf_get_object_ai_data,        2, 2, -1, {ARG_OBJECT, ARG_INT}},
	{"get_object_data",           mf_get_object_data,           2, 2,  0, {ARG_OBJECT, ARG_INT}},
	{"get_outline",               mf_get_outline,               1, 1,  0, {ARG_OBJECT}},
	{"get_sfall_arg_at",          mf_get_sfall_arg_at,          1, 1,  0, {ARG_INT}},
	{"get_stat_max",              mf_get_stat_max,              1, 2,  0, {ARG_INT, ARG_INT}},
	{"get_stat_min",              mf_get_stat_min,              1, 2,  0, {ARG_INT, ARG_INT}},
	{"get_string_pointer",        mf_get_string_pointer,        1, 1,  0, {ARG_STRING}},
	{"get_terrain_name",          mf_get_terrain_name,          0, 2, -1, {ARG_INT, ARG_INT}},
	{"get_text_width",            mf_get_text_width,            1, 1,  0, {ARG_STRING}},
	{"get_window_attribute",      mf_get_window_attribute,      1, 2, -1, {ARG_INT, ARG_INT}},
	{"has_fake_perk_npc",         mf_has_fake_perk_npc,         2, 2,  0, {ARG_OBJECT, ARG_STRING}},
	{"has_fake_trait_npc",        mf_has_fake_trait_npc,        2, 2,  0, {ARG_OBJECT, ARG_STRING}},
	{"hide_window",               mf_hide_window,               0, 1, -1, {ARG_STRING}},
	{"interface_art_draw",        mf_interface_art_draw,        4, 6, -1, {ARG_INT, ARG_INTSTR, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"interface_overlay",         mf_interface_overlay,         2, 6, -1, {ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"interface_print",           mf_interface_print,           5, 6, -1, {ARG_STRING, ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"intface_hide",              mf_intface_hide,              0, 0},
	{"intface_is_hidden",         mf_intface_is_hidden,         0, 0},
	{"intface_redraw",            mf_intface_redraw,            0, 1},
	{"intface_show",              mf_intface_show,              0, 0},
	{"inventory_redraw",          mf_inventory_redraw,          0, 1, -1, {ARG_INT}},
	{"item_make_explosive",       mf_item_make_explosive,       3, 4, -1, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"item_weight",               mf_item_weight,               1, 1,  0, {ARG_OBJECT}},
	{"lock_is_jammed",            mf_lock_is_jammed,            1, 1,  0, {ARG_OBJECT}},
	{"loot_obj",                  mf_get_loot_object,           0, 0},
	{"message_box",               mf_message_box,               1, 4, -1, {ARG_STRING, ARG_INT, ARG_INT, ARG_INT}},
	{"metarule_exist",            mf_metarule_exist,            1, 1}, // no arg check
	{"npc_engine_level_up",       mf_npc_engine_level_up,       1, 1},
	{"obj_is_openable",           mf_obj_is_openable,           1, 1,  0, {ARG_OBJECT}},
	{"obj_under_cursor",          mf_obj_under_cursor,          2, 2,  0, {ARG_INT, ARG_INT}},
	{"objects_in_radius",         mf_objects_in_radius,         3, 4,  0, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"outlined_object",           mf_outlined_object,           0, 0},
	{"real_dude_obj",             mf_real_dude_obj,             0, 0},
	{"reg_anim_animate_and_move", mf_reg_anim_animate_and_move, 4, 4, -1, {ARG_OBJECT, ARG_INT, ARG_INT, ARG_INT}},
	{"remove_timer_event",        mf_remove_timer_event,        0, 1, -1, {ARG_INT}},
	{"set_spray_settings",        mf_set_spray_settings,        4, 4, -1, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"set_can_rest_on_map",       mf_set_rest_on_map,           3, 3, -1, {ARG_INT, ARG_INT, ARG_INT}},
	{"set_car_intface_art",       mf_set_car_intface_art,       1, 1, -1, {ARG_INT}},
	{"set_combat_free_move",      mf_set_combat_free_move,      1, 1, -1, {ARG_INT}},
	{"set_cursor_mode",           mf_set_cursor_mode,           1, 1, -1, {ARG_INT}},
	{"set_drugs_data",            mf_set_drugs_data,            3, 3, -1, {ARG_INT, ARG_INT, ARG_INT}},
	{"set_dude_obj",              mf_set_dude_obj,              1, 1, -1, {ARG_INT}},
	{"set_fake_perk_npc",         mf_set_fake_perk_npc,         5, 5, -1, {ARG_OBJECT, ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{"set_fake_trait_npc",        mf_set_fake_trait_npc,        5, 5, -1, {ARG_OBJECT, ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{"set_flags",                 mf_set_flags,                 2, 2, -1, {ARG_OBJECT, ARG_INT}},
	{"set_iface_tag_text",        mf_set_iface_tag_text,        3, 3, -1, {ARG_INT, ARG_STRING, ARG_INT}},
	{"set_ini_setting",           mf_set_ini_setting,           2, 2, -1, {ARG_STRING, ARG_INTSTR}},
	{"set_map_enter_position",    mf_set_map_enter_position,    3, 3, -1, {ARG_INT, ARG_INT, ARG_INT}},
	{"set_object_data",           mf_set_object_data,           3, 3, -1, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{"set_outline",               mf_set_outline,               2, 2, -1, {ARG_OBJECT, ARG_INT}},
	{"set_quest_failure_value",   mf_set_quest_failure_value,   2, 2, -1, {ARG_INT, ARG_INT}},
	{"set_rest_heal_time",        mf_set_rest_heal_time,        1, 1, -1, {ARG_INT}},
	{"set_worldmap_heal_time",    mf_set_worldmap_heal_time,    1, 1, -1, {ARG_INT}},
	{"set_rest_mode",             mf_set_rest_mode,             1, 1, -1, {ARG_INT}},
	{"set_scr_name",              mf_set_scr_name,              0, 1, -1, {ARG_STRING}},
	{"set_selectable_perk_npc",   mf_set_selectable_perk_npc,   5, 5, -1, {ARG_OBJECT, ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{"set_terrain_name",          mf_set_terrain_name,          3, 3, -1, {ARG_INT, ARG_INT, ARG_STRING}},
	{"set_town_title",            mf_set_town_title,            2, 2, -1, {ARG_INT, ARG_STRING}},
	{"set_unique_id",             mf_set_unique_id,             1, 2, -1, {ARG_OBJECT, ARG_INT}},
	{"set_unjam_locks_time",      mf_set_unjam_locks_time,      1, 1, -1, {ARG_INT}},
	{"set_window_flag",           mf_set_window_flag,           3, 3, -1, {ARG_INTSTR, ARG_INT, ARG_INT}},
	{"show_window",               mf_show_window,               0, 1, -1, {ARG_STRING}},
	{"signal_close_game",         mf_signal_close_game,         0, 0},
	{"spatial_radius",            mf_spatial_radius,            1, 1,  0, {ARG_OBJECT}},
	{"string_compare",            mf_string_compare,            2, 3,  0, {ARG_STRING, ARG_STRING, ARG_INT}},
	{"string_find",               mf_string_find,               2, 3, -1, {ARG_STRING, ARG_STRING, ARG_INT}},
	{"string_format",             mf_string_format,             2, 8,  0, {ARG_STRING, ARG_ANY, ARG_ANY, ARG_ANY, ARG_ANY, ARG_ANY, ARG_ANY, ARG_ANY}},
	{"string_to_case",            mf_string_to_case,            2, 2, -1, {ARG_STRING, ARG_INT}},
	{"tile_by_position",          mf_tile_by_position,          2, 2, -1, {ARG_INT, ARG_INT}},
	{"tile_refresh_display",      mf_tile_refresh_display,      0, 0},
	{"unjam_lock",                mf_unjam_lock,                1, 1, -1, {ARG_OBJECT}},
	{"unwield_slot",              mf_unwield_slot,              2, 2, -1, {ARG_OBJECT, ARG_INT}},
	{"win_fill_color",            mf_win_fill_color,            0, 5, -1, {ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	#ifndef NDEBUG
	{"validate_test",             mf_test,                      2, 5, -1, {ARG_INT, ARG_NUMBER, ARG_STRING, ARG_OBJECT, ARG_ANY}},
	#endif
};

// returns current contents of metarule table
static void mf_get_metarule_table(OpcodeContext& ctx) {
	DWORD arrId = CreateTempArray(metaruleTable.size(), 0);
	int i = 0;
	for (auto it = metaruleTable.begin(); it != metaruleTable.end(); ++it) {
		arrays[arrId].val[i].set(it->first.c_str());
		i++;
	}
	ctx.setReturn(arrId, DataType::INT);
}

static void mf_metarule_exist(OpcodeContext& ctx) {
	bool result = false;
	auto funcXName = ctx.arg(0).asString();
	if (funcXName[0] != '\0') {
		result = (metaruleTable.find(funcXName) != metaruleTable.cend());
	}
	ctx.setReturn(result);
}

void InitMetaruleTable() {
	for (auto metarule = std::begin(metarules); metarule != std::end(metarules); ++metarule) {
		metaruleTable[metarule->name] = metarule;
	}
}

// Validates arguments against metarule specification.
// On error prints to debug.log and returns false.
static bool ValidateMetaruleArguments(OpcodeContext& ctx) {
	const SfallMetarule* metaruleInfo = ctx.getMetarule();
	int argCount = ctx.numArgs();
	if (argCount < metaruleInfo->minArgs || argCount > metaruleInfo->maxArgs) {
		ctx.printOpcodeError(
			"%s(\"%s\", ...) - invalid number of arguments (%d), must be from %d to %d.",
			ctx.getOpcodeName(),
			metaruleInfo->name,
			argCount,
			metaruleInfo->minArgs,
			metaruleInfo->maxArgs
		);
		return false;
	} else {
		return ctx.validateArguments(metaruleInfo->argValidation, metaruleInfo->name);
	}
}

void HandleMetarule(OpcodeContext& ctx) {
	const ScriptValue &nameArg = ctx.arg(0);
	if (nameArg.isString()) {
		const char* name = nameArg.strValue();
		MetaruleTableType::iterator lookup = metaruleTable.find(name);
		if (lookup != metaruleTable.end()) {
			ctx.setMetarule(lookup->second);
			// shift function name away, so argument #0 will correspond to actual first argument of function
			// this allows to use the same handlers for opcodes and metarule functions
			ctx.setArgShift(1);
			if (ValidateMetaruleArguments(ctx)) {
				ctx.getMetarule()->func(ctx);
			} else if (ctx.hasReturn()) {
				ctx.setReturn(ctx.getMetarule()->errValue);
			}
		} else {
			ctx.printOpcodeError("%s(\"%s\", ...) - metarule function is unknown.", ctx.getOpcodeName(), name);
		}
	} else {
		ctx.printOpcodeError("%s(name, ...) - name must be string.", ctx.getOpcodeName());
	}
}

#ifndef NDEBUG
static std::string test_stringBuf;

void mf_test(OpcodeContext& ctx) {
	std::ostringstream sstream;
	sstream << "sfall_funcX(\"test\"";
	for (int i = 0; i < ctx.numArgs(); i++) {
		const ScriptValue &arg = ctx.arg(i);
		sstream << ", ";
		switch (arg.type()) {
		case DataType::INT:
			sstream << arg.asInt();
			break;
		case DataType::FLOAT:
			sstream << arg.asFloat();
			break;
		case DataType::STR:
			sstream << '"' << arg.asString() << '"';
			break;
		default:
			sstream << "???";
			break;
		}
	}
	sstream << ")";

	test_stringBuf = sstream.str();
	ctx.setReturn(test_stringBuf.c_str());
}
#endif

}
}
