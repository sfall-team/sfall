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
// Prefix all function handlers with sf_ and add them to sfall_metarule_table.
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
	int minArgs;
	// maximum number of arguments
	int maxArgs;
};

typedef std::tr1::unordered_map<std::string, const SfallMetarule*> MetaruleTableType;

static MetaruleTableType metaruleTable;

// currently executed metarule
static const SfallMetarule* currentMetarule;

static std::string sf_test_stringBuf;

// Example handler. Feel free to add handlers in other files.
static void sf_test() {
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

	sf_test_stringBuf = sstream.str();
	opHandler.setReturn(sf_test_stringBuf.c_str());
}

// returns current contents of metarule table
static void sf_get_metarule_table() {
	DWORD arr = TempArray(metaruleTable.size(), 0);
	int i = 0;
	for (MetaruleTableType::iterator it = metaruleTable.begin(); it != metaruleTable.end(); it++) {
		arrays[arr].val[i].set(it->first.c_str());
		i++;
	}
	opHandler.setReturn(arr, DATATYPE_INT);
}

/*
	Metarule array.

	Add your custom scripting functions here.

	Format is as follows:
	{ name, handler, minArgs, maxArgs }
		- name - name of function that will be used to call it from scripts,
		- handler - pointer to handler function (see examples below),
		- minArgs/maxArgs - minimum and maximum number of arguments allowed for this function
*/
static const SfallMetarule metaruleArray[] = {
	{"get_metarule_table", sf_get_metarule_table, 0, 0},
	{"validate_test", sf_test, 2, 5},
	{"spatial_radius", sf_spatial_radius, 1, 1},
	{"critter_inven_obj2", sf_critter_inven_obj2, 2, 2},
	{"intface_redraw", sf_intface_redraw, 0, 0},
	{"intface_show", sf_intface_show, 0, 0},
	{"intface_hide", sf_intface_hide, 0, 0},
	{"intface_is_hidden", sf_intface_is_hidden, 0, 0},
	{"exec_map_update_scripts", sf_exec_map_update_scripts, 0, 0},
	{"set_ini_setting", sf_set_ini_setting, 2, 2},
	{"floor2", sf_floor2, 1, 1},
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
			metaruleInfo->maxArgs);

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

static void _stdcall op_sfall_metarule_handler() {
	const ScriptValue &nameArg = opHandler.arg(0);
	if (nameArg.isString()) {
		const char* name = nameArg.asString();
		MetaruleTableType::iterator lookup = metaruleTable.find(name);
		if (lookup != metaruleTable.end()) {
			currentMetarule = lookup->second;
			// shift function name away, so argument #0 will correspond to actual first argument of function
			// this allows to use the same handlers for opcodes and metarule functions
			opHandler.setArgShift(1);
			if (ValidateMetaruleArguments(currentMetarule)) {
				currentMetarule->func();
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
