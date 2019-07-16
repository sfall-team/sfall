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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"
#include "Anims.h"
#include "Interface.h"
#include "Misc.h"
#include "Objects.h"
#include "Perks.h"
#include "Utils.h"
#include "Worldmap.h"

#include "Metarule.h"

namespace sfall
{
namespace script
{

// Metarule is a universal opcode(s) for all kinds of new sfall scripting functions.
// Prefix all function handlers with sf_ and add them to sfall_metarule_table.
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
	{ name, handler, minArgs, maxArgs, {arg1, arg2, ...} }
		- name - name of function that will be used in scripts,
		- handler - pointer to handler function (see examples below),
		- minArgs/maxArgs - minimum and maximum number of arguments allowed for this function (max 6)
		- arg1, arg2, ... - argument types for automatic validation
*/
static const SfallMetarule metarules[] = {
	{"add_extra_msg_file",      sf_add_extra_msg_file,      1, 2, {ARG_STRING, ARG_INT}},
	{"add_iface_tag",           sf_add_iface_tag,           0, 0},
	{"art_cache_clear",         sf_art_cache_flush,         0, 0},
	{"attack_is_aimed",         sf_attack_is_aimed,         0, 0},
	{"car_gas_amount",          sf_car_gas_amount,          0, 0},
	{"create_win",              sf_create_win,              5, 6, {ARG_STRING, ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"critter_inven_obj2",      sf_critter_inven_obj2,      2, 2, {ARG_OBJECT, ARG_INT}},
	{"dialog_message",          sf_dialog_message,          1, 1, {ARG_STRING}},
	{"dialog_obj",              sf_get_dialog_object,       0, 0},
	{"display_stats",           sf_display_stats,           0, 0}, // refresh
	{"draw_image",              sf_draw_image,              1, 5, {ARG_INTSTR, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"draw_image_scaled",       sf_draw_image_scaled,       1, 6, {ARG_INTSTR, ARG_INT, ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"exec_map_update_scripts", sf_exec_map_update_scripts, 0, 0},
	{"floor2",                  sf_floor2,                  1, 1, {ARG_NUMBER}},
	{"get_can_rest_on_map",     sf_get_rest_on_map,         2, 2, {ARG_INT, ARG_INT}},
	{"get_current_inven_size",  sf_get_current_inven_size,  1, 1, {ARG_OBJECT}},
	{"get_cursor_mode",         sf_get_cursor_mode,         0, 0},
	{"get_flags",               sf_get_flags,               1, 1, {ARG_OBJECT}},
	{"get_ini_section",         sf_get_ini_section,         2, 2, {ARG_STRING, ARG_STRING}},
	{"get_ini_sections",        sf_get_ini_sections,        1, 1, {ARG_STRING}},
	{"get_map_enter_position",  sf_get_map_enter_position,  0, 0},
	{"get_metarule_table",      sf_get_metarule_table,      0, 0},
	{"get_object_ai_data",      sf_get_object_ai_data,      2, 2, {ARG_OBJECT, ARG_INT}},
	{"get_object_data",         sf_get_object_data,         2, 2, {ARG_OBJECT, ARG_INT}},
	{"get_outline",             sf_get_outline,             1, 1, {ARG_OBJECT}},
	{"get_string_pointer",      sf_get_string_pointer,      1, 1, {ARG_STRING}},
	{"has_fake_perk_npc",       sf_has_fake_perk_npc,       2, 2, {ARG_OBJECT, ARG_STRING}},
	{"has_fake_trait_npc",      sf_has_fake_trait_npc,      2, 2, {ARG_OBJECT, ARG_STRING}},
	{"intface_hide",            sf_intface_hide,            0, 0},
	{"intface_is_hidden",       sf_intface_is_hidden,       0, 0},
	{"intface_redraw",          sf_intface_redraw,          0, 0},
	{"intface_show",            sf_intface_show,            0, 0},
	{"inventory_redraw",        sf_inventory_redraw,        1, 1, {ARG_INT}},
	{"item_make_explosive",     sf_item_make_explosive,     3, 4, {ARG_INT, ARG_INT, ARG_INT, ARG_INT}},
	{"item_weight",             sf_item_weight,             1, 1, {ARG_OBJECT}},
	{"lock_is_jammed",          sf_lock_is_jammed,          1, 1, {ARG_OBJECT}},
	{"loot_obj",                sf_get_loot_object,         0, 0},
	{"metarule_exist",          sf_metarule_exist,          1, 1}, // no arg check
	{"npc_engine_level_up",     sf_npc_engine_level_up,     1, 1, {ARG_ANY}},
	{"obj_under_cursor",        sf_get_obj_under_cursor,    2, 2, {ARG_INT, ARG_INT}},
	{"outlined_object",         sf_outlined_object,         0, 0},
	{"real_dude_obj",           sf_real_dude_obj,           0, 0},
	{"set_can_rest_on_map",     sf_set_rest_on_map,         3, 3, {ARG_INT, ARG_INT, ARG_INT}},
	{"set_car_intface_art",     sf_set_car_intface_art,     1, 1, {ARG_INT}},
	{"set_cursor_mode",         sf_set_cursor_mode,         1, 1, {ARG_INT}},
	{"set_drugs_data",          sf_set_drugs_data,          3, 3, {ARG_INT, ARG_INT, ARG_INT}},
	{"set_dude_obj",            sf_set_dude_obj,            1, 1, {ARG_INT}},
	{"set_fake_perk_npc",       sf_set_fake_perk_npc,       5, 5, {ARG_OBJECT, ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{"set_fake_trait_npc",      sf_set_fake_trait_npc,      5, 5, {ARG_OBJECT, ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{"set_flags",               sf_set_flags,               2, 2, {ARG_OBJECT, ARG_INT}},
	{"set_iface_tag_text",      sf_set_iface_tag_text,      3, 3, {ARG_INT, ARG_STRING, ARG_INT}},
	{"set_ini_setting",         sf_set_ini_setting,         2, 2, {ARG_STRING, ARG_INTSTR}},
	{"set_map_enter_position",  sf_set_map_enter_position,  3, 3, {ARG_INT, ARG_INT, ARG_INT}},
	{"set_object_data",         sf_set_object_data,         3, 3, {ARG_OBJECT, ARG_INT, ARG_INT}},
	{"set_outline",             sf_set_outline,             2, 2, {ARG_OBJECT, ARG_INT}},
	{"set_rest_heal_time",      sf_set_rest_heal_time,      1, 1, {ARG_INT}},
	{"set_rest_mode",           sf_set_rest_mode,           1, 1, {ARG_INT}},
	{"set_selectable_perk_npc", sf_set_selectable_perk_npc, 5, 5, {ARG_OBJECT, ARG_STRING, ARG_INT, ARG_INT, ARG_STRING}},
	{"set_unique_id",           sf_set_unique_id,           1, 2, {ARG_OBJECT, ARG_INT}},
	{"set_unjam_locks_time",    sf_set_unjam_locks_time,    1, 1, {ARG_INT}},
	{"spatial_radius",          sf_spatial_radius,          1, 1, {ARG_OBJECT}},
	{"tile_refresh_display",    sf_tile_refresh_display,    0, 0},
	{"unjam_lock",              sf_unjam_lock,              1, 1, {ARG_OBJECT}},
	#ifndef NDEBUG
	{"validate_test",           sf_test,                    2, 5, {ARG_INT, ARG_NUMBER, ARG_STRING, ARG_OBJECT, ARG_ANY}},
	#endif
};

// returns current contents of metarule table
static void sf_get_metarule_table(OpcodeContext& ctx) {
	DWORD arrId = TempArray(metaruleTable.size(), 0);
	int i = 0;
	for (auto it = metaruleTable.begin(); it != metaruleTable.end(); it++) {
		arrays[arrId].val[i].set(it->first.c_str());
		i++;
	}
	ctx.setReturn(arrId, DataType::INT);
}

static void sf_metarule_exist(OpcodeContext& ctx) {
	bool result = false;
	auto funcXName = ctx.arg(0).asString();
	if (funcXName[0] != '\0') {
		for (auto it = metaruleTable.begin(); it != metaruleTable.end(); it++) {
			if (it->first == funcXName) {
				result = true;
				break;
			}
		}
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
	const SfallMetarule* metaruleInfo = ctx.metarule;
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
			ctx.metarule = lookup->second;
			// shift function name away, so argument #0 will correspond to actual first argument of function
			// this allows to use the same handlers for opcodes and metarule functions
			ctx.setArgShift(1);
			if (ValidateMetaruleArguments(ctx)) {
				ctx.metarule->func(ctx);
			} else if (ctx.hasReturn()) {
				ctx.setReturn(-1);
			}
		} else {
			ctx.printOpcodeError("%s(\"%s\", ...) - metarule function is unknown.", ctx.getOpcodeName(), name);
		}
	} else {
		ctx.printOpcodeError("%s(name, ...) - name must be string.", ctx.getOpcodeName());
	}
}

#ifndef NDEBUG
static std::string sf_test_stringBuf;
void sf_test(OpcodeContext& ctx) {
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

	sf_test_stringBuf = sstream.str();
	ctx.setReturn(sf_test_stringBuf.c_str());
}
#endif

}
}
