/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include <string>
#include <sstream>
#include <unordered_map>

#include "main.h"
#include "ScriptExtender.h"

// Metarule is a universal opcode(s) for all kinds of new sfall scripting functions.
// Prefix all function handlers with mf_ and add them to sfall_metarule_table.
// DO NOT add arguments and/or return values to function handlers!
// Use opHandler.arg(i), inside handler function to access arguments.
// Use opHandler.setReturn(x) to set return value.
// If you want to call user-defined procedures in your handler, use RunScriptProc().

struct SfallMetarule {
	// function name
	const char* name;
	// pointer to handler function
	void(*func)();
	// mininum number of arguments
	short minArgs;
	// maximum number of arguments
	short maxArgs;
};

typedef std::tr1::unordered_map<std::string, const SfallMetarule*> MetaruleTableType;

static MetaruleTableType metaruleTable;

// currently executed metarule
static const SfallMetarule* currentMetarule;

// Example handler. Feel free to add handlers in other files.
#ifndef NDEBUG
static std::string test_stringBuf;

static void mf_test() {
	std::ostringstream sstream;
	sstream << "sfall_funcX(\"test\"";
	for (int i = 0; i < opHandler.numArgs(); i++) {
		const ScriptValue &arg = opHandler.arg(i);
		sstream << ", ";
		switch (arg.type()) {
			case DATATYPE_INT:
				sstream << arg.asInt();
				break;
			case DATATYPE_FLOAT:
				sstream << arg.asFloat();
				break;
			case DATATYPE_STR:
				sstream << '"' << arg.asString() << '"';
				break;
			default:
				sstream << "???";
				break;
		}
	}
	sstream << ")";

	test_stringBuf = sstream.str();
	opHandler.setReturn(test_stringBuf.c_str());
}
#endif

// returns current contents of metarule table
static void mf_get_metarule_table() {
	DWORD arrId = CreateTempArray(metaruleTable.size(), 0);
	int i = 0;
	for (MetaruleTableType::iterator it = metaruleTable.begin(); it != metaruleTable.end(); ++it) {
		arrays[arrId].val[i].set(it->first.c_str());
		i++;
	}
	opHandler.setReturn(arrId, DATATYPE_INT);
}

static void mf_metarule_exist() {
	bool result = false;
	const char* funcXName = opHandler.arg(0).asString();
	if (funcXName[0] != '\0') {
		const MetaruleTableType::iterator &it = metaruleTable.find(funcXName);
		if (it != metaruleTable.cend()) result = true;
	}
	opHandler.setReturn(result);
}

/*
	Metarule array.

	Add your custom scripting functions here.

	Format is as follows:
	{ name, handler, minArgs, maxArgs }
		- name - name of function that will be used to call it from scripts,
		- handler - pointer to handler function (see examples below),
		- minArgs/maxArgs - minimum and maximum number of arguments allowed for this function (max 6)
*/
static const SfallMetarule metaruleArray[] = {
	{"add_extra_msg_file",      mf_add_extra_msg_file,      1, 2},
	{"add_iface_tag",           mf_add_iface_tag,           0, 0},
	{"add_g_timer_event",       mf_add_g_timer_event,       2, 2},
	{"add_trait",               mf_add_trait,               1, 1},
	{"art_cache_clear",         mf_art_cache_flush,         0, 0},
	{"attack_is_aimed",         mf_attack_is_aimed,         0, 0},
	{"car_gas_amount",          mf_car_gas_amount,          0, 0},
	{"combat_data",             mf_combat_data,             0, 0},
	{"create_win",              mf_create_win,              5, 6},
	{"critter_inven_obj2",      mf_critter_inven_obj2,      2, 2},
	{"dialog_obj",              mf_get_dialog_object,       0, 0},
	{"display_stats",           mf_display_stats,           0, 0}, // refresh
	{"draw_image",              mf_draw_image,              1, 5},
	{"draw_image_scaled",       mf_draw_image_scaled,       1, 6},
	{"exec_map_update_scripts", mf_exec_map_update_scripts, 0, 0},
	{"floor2",                  mf_floor2,                  1, 1},
	{"get_current_inven_size",  mf_get_current_inven_size,  1, 1},
	{"get_cursor_mode",         mf_get_cursor_mode,         0, 0},
	{"get_flags",               mf_get_flags,               1, 1},
	{"get_inven_ap_cost",       mf_get_inven_ap_cost,       0, 0},
	{"get_map_enter_position",  mf_get_map_enter_position,  0, 0},
	{"get_metarule_table",      mf_get_metarule_table,      0, 0},
	{"get_object_data",         mf_get_object_data,         2, 2},
	{"get_outline",             mf_get_outline,             1, 1},
	{"get_sfall_arg_at",        mf_get_sfall_arg_at,        1, 1},
	{"get_stat_max",            mf_get_stat_max,            1, 2},
	{"get_stat_min",            mf_get_stat_min,            1, 2},
	{"get_text_width",          mf_get_text_width,          1, 1},
	{"get_window_attribute",    mf_get_window_attribute,    1, 2},
	{"hide_window",             mf_hide_window,             0, 1},
	{"interface_art_draw",      mf_interface_art_draw,      4, 6},
	{"interface_print",         mf_interface_print,         5, 6},
	{"intface_hide",            mf_intface_hide,            0, 0},
	{"intface_is_hidden",       mf_intface_is_hidden,       0, 0},
	{"intface_redraw",          mf_intface_redraw,          0, 1},
	{"intface_show",            mf_intface_show,            0, 0},
	{"inventory_redraw",        mf_inventory_redraw,        0, 1},
	{"item_weight",             mf_item_weight,             1, 1},
	{"lock_is_jammed",          mf_lock_is_jammed,          1, 1},
	{"loot_obj",                mf_get_loot_object,         0, 0},
	{"message_box",             mf_message_box,             1, 4},
	{"metarule_exist",          mf_metarule_exist,          1, 1},
	{"npc_engine_level_up",     mf_npc_engine_level_up,     1, 1},
	{"obj_under_cursor",        mf_obj_under_cursor,        2, 2},
	{"objects_in_radius",       mf_objects_in_radius,       3, 4},
	{"outlined_object",         mf_outlined_object,         0, 0},
	{"real_dude_obj",           mf_real_dude_obj,           0, 0},
	{"remove_timer_event",      mf_remove_timer_event,      0, 1},
	{"set_car_intface_art",     mf_set_car_intface_art,     1, 1},
	{"set_cursor_mode",         mf_set_cursor_mode,         1, 1},
	{"set_flags",               mf_set_flags,               2, 2},
	{"set_iface_tag_text",      mf_set_iface_tag_text,      3, 3},
	{"set_ini_setting",         mf_set_ini_setting,         2, 2},
	{"set_map_enter_position",  mf_set_map_enter_position,  3, 3},
	{"set_object_data",         mf_set_object_data,         3, 3},
	{"set_outline",             mf_set_outline,             2, 2},
	{"set_terrain_name",        mf_set_terrain_name,        3, 3},
	{"set_town_title",          mf_set_town_title,          2, 2},
	{"set_unique_id",           mf_set_unique_id,           1, 2},
	{"set_unjam_locks_time",    mf_set_unjam_locks_time,    1, 1},
	{"set_window_flag",         mf_set_window_flag,         3, 3},
	{"show_window",             mf_show_window,             0, 1},
	{"spatial_radius",          mf_spatial_radius,          1, 1},
	{"string_compare",          mf_string_compare,          2, 3},
	{"string_format",           mf_string_format,           2, 5},
	{"string_to_case",          mf_string_to_case,          2, 2},
	{"tile_by_position",        mf_tile_by_position,        2, 2},
	{"tile_refresh_display",    mf_tile_refresh_display,    0, 0},
	{"unjam_lock",              mf_unjam_lock,              1, 1},
	{"unwield_slot",            mf_unwield_slot,            2, 2},
	{"win_fill_color",          mf_win_fill_color,          0, 5},
	#ifndef NDEBUG
	{"validate_test",           mf_test,                    2, 5},
	#endif
};

static void InitMetaruleTable() {
	int length = sizeof(metaruleArray) / sizeof(SfallMetarule);
	for (int i = 0; i < length; ++i) {
		metaruleTable[metaruleArray[i].name] = &metaruleArray[i];
	}
}

// Validates arguments against metarule specification.
// On error prints to debug.log and returns false.
static bool ValidateMetaruleArguments(const SfallMetarule* metaruleInfo) {
	int argCount = opHandler.numArgs();
	if (argCount < metaruleInfo->minArgs || argCount > metaruleInfo->maxArgs) {
		opHandler.printOpcodeError(
			"sfall_funcX(\"%s\", ...) - invalid number of arguments (%d), must be from %d to %d.",
			metaruleInfo->name,
			argCount,
			metaruleInfo->minArgs,
			metaruleInfo->maxArgs
		);
		return false;
	} else {
		// check if metadata is available for this handler
		OpcodeMetaTableType::iterator it = opcodeMetaTable.find(metaruleInfo->func);
		if (it != opcodeMetaTable.end()) {
			const SfallOpcodeMetadata* meta = it->second;

			// automatically validate argument types
			return opHandler.validateArguments(meta->argTypeMasks, argCount, metaruleInfo->name);
		}
	}
	return true;
}

static void __stdcall op_sfall_metarule_handler() {
	const ScriptValue &nameArg = opHandler.arg(0);
	if (nameArg.isString()) {
		const char* name = nameArg.strValue();
		MetaruleTableType::iterator lookup = metaruleTable.find(name);
		if (lookup != metaruleTable.end()) {
			currentMetarule = lookup->second;
			// shift function name away, so argument #0 will correspond to actual first argument of function
			// this allows to use the same handlers for opcodes and metarule functions
			opHandler.setArgShift(1);
			if (ValidateMetaruleArguments(currentMetarule)) {
				currentMetarule->func();
			} else {
				opHandler.setReturn(-1);
			}
		} else {
			opHandler.printOpcodeError("sfall_funcX(name, ...) - name '%s' is unknown.", name);
		}
	} else {
		opHandler.printOpcodeError("sfall_funcX(name, ...) - name must be string.");
	}
}

#define metaruleOpcode(numArg, numArgPlusOne) \
	static void __declspec(naked) op_sfall_metarule##numArg() {\
		_WRAP_OPCODE(op_sfall_metarule_handler, numArgPlusOne, 1)\
	}

metaruleOpcode(0, 1)
metaruleOpcode(1, 2)
metaruleOpcode(2, 3)
metaruleOpcode(3, 4)
metaruleOpcode(4, 5)
metaruleOpcode(5, 6)
metaruleOpcode(6, 7)

#undef metaruleOpcode
