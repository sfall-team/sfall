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
#include "Interface.h"
#include "Misc.h"
#include "Objects.h"

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

struct SfallMetarule {
	// function name
	const char* name;

	// pointer to handler function
	ScriptingFunctionHandler func;

	// minimum number of arguments
	int minArgs;

	// maximum number of arguments
	int maxArgs;
	
	// argument validation settings
	OpcodeArgumentType argValidation[OP_MAX_ARGUMENTS];
};

typedef std::tr1::unordered_map<std::string, const SfallMetarule*> MetaruleTableType;

static MetaruleTableType metaruleTable;

// currently executed metarule
static const SfallMetarule* currentMetarule;

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

// returns current contents of metarule table
void sf_get_metarule_table(OpcodeContext& ctx) {
	DWORD arr = TempArray(metaruleTable.size(), 0);
	int i = 0;
	for (auto it = metaruleTable.begin(); it != metaruleTable.end(); it++) {
		arrays[arr].val[i].set(it->first.c_str());
		i++;
	}
	ctx.setReturn(arr, DataType::INT);
}

/*
	Metarule array.

	Add your custom scripting functions here.

	Format is as follows:
	{ name, handler, minArgs, maxArgs }
		- name - name of function that will be used to call it from scripts,
		- handler - pointer to handler function (see examples below),
		- minArgs/maxArgs - minimum and maximum number of arguments allowed for this function
		- argument types for validation
*/
static const SfallMetarule metaruleArray[] = {
	{"get_metarule_table", sf_get_metarule_table, 0, 0},
	{"validate_test", sf_test, 2, 5, {ARG_INT, ARG_NUMBER, ARG_STRING, ARG_OBJECT, ARG_ANY}},
	{"spatial_radius", sf_spatial_radius, 1, 1, {ARG_OBJECT}},
	{"critter_inven_obj2", sf_critter_inven_obj2, 2, 2, {ARG_OBJECT, ARG_INT}},
	{"intface_redraw", sf_intface_redraw, 0, 0},
	{"intface_show", sf_intface_show, 0, 0},
	{"intface_hide", sf_intface_hide, 0, 0},
	{"intface_is_hidden", sf_intface_is_hidden, 0, 0},
	{"item_weight", sf_item_weight, 1, 1, {ARG_OBJECT}},
	{"exec_map_update_scripts", sf_exec_map_update_scripts, 0, 0},
	{"set_outline", sf_set_outline, 2, 2, {ARG_OBJECT, ARG_INT}},
	{"get_outline", sf_get_outline, 1, 1, {ARG_OBJECT}},
	{"set_flags", sf_set_flags, 2, 2, {ARG_OBJECT, ARG_INT}},
	{"get_flags", sf_get_flags, 1, 1, {ARG_OBJECT}},
	{"tile_refresh_display", sf_tile_refresh_display, 0, 0},
	{"outlined_object", sf_outlined_object, 0, 0},
	{"set_dude_obj", sf_set_dude_obj, 1, 1, {ARG_OBJECT}},
};

void InitMetaruleTable() {
	int length = sizeof(metaruleArray) / sizeof(SfallMetarule);
	for (int i = 0; i < length; ++i) {
		metaruleTable[metaruleArray[i].name] = &metaruleArray[i];
	}
}

// Validates arguments against metarule specification.
// On error prints to debug.log and returns false.
static bool ValidateMetaruleArguments(OpcodeContext& ctx, const SfallMetarule* metaruleInfo) {
	int argCount = ctx.numArgs();
	if (argCount < metaruleInfo->minArgs || argCount > metaruleInfo->maxArgs) {
		ctx.printOpcodeError(
			"sfall_funcX(\"%s\", ...) - invalid number of arguments (%d), must be from %d to %d.", 
			metaruleInfo->name, 
			argCount,
			metaruleInfo->minArgs,
			metaruleInfo->maxArgs);

		return false;
	} else {
		return ctx.validateArguments(metaruleInfo->argValidation, metaruleInfo->name);
	}
}

void HandleMetarule(OpcodeContext& ctx) {
	const ScriptValue &nameArg = ctx.arg(0);
	if (nameArg.isString()) {
		const char* name = nameArg.asString();
		MetaruleTableType::iterator lookup = metaruleTable.find(name);
		if (lookup != metaruleTable.end()) {
			currentMetarule = lookup->second;
			// shift function name away, so argument #0 will correspond to actual first argument of function
			// this allows to use the same handlers for opcodes and metarule functions
			ctx.setArgShift(1);
			if (ValidateMetaruleArguments(ctx, currentMetarule)) {
				currentMetarule->func(ctx);
			} else if (ctx.hasReturn()) {
				ctx.setReturn(-1);
			}
		} else {
			ctx.printOpcodeError("sfall_funcX(name, ...) - name '%s' is unknown.", name);
		}
	} else {
		ctx.printOpcodeError("sfall_funcX(name, ...) - name must be string.");
	}
}

}
}
