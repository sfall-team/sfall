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
// Use functions GetOpArgXXX(), IsOpArgXXX() inside handler function to read arguments, but argument 0 is always function name.
// Use SetOpReturn() to set return value.
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
	// type masks of arguments DATATYPE_MASK_* (use bitwise OR to combine, use 0 to disable type validation)
	int argTypeMasks[6]; 
};

#define DATATYPE_MASK_INT		(1 << DATATYPE_INT)
#define DATATYPE_MASK_FLOAT		(1 << DATATYPE_FLOAT)
#define DATATYPE_MASK_STR		(1 << DATATYPE_STR)
#define DATATYPE_MASK_NOT_NULL	(0x00010000)
#define DATATYPE_MASK_VALID_OBJ	(DATATYPE_MASK_INT | DATATYPE_MASK_NOT_NULL)

typedef std::tr1::unordered_map<std::string, const SfallMetarule*> MetaruleTableType;

static MetaruleTableType metaruleTable;

static std::string sf_test_stringBuf;

// Example handler. Feel free to add handlers in other files.
static void sf_test() {
	std::ostringstream sstream;
	sstream << "sfall_funcX(\"test\"";
	for (DWORD i = 1; i < opArgCount; i++) {
		sstream << ", ";
		switch (opArgs[i].type) {
			case DATATYPE_INT:
				sstream << GetOpArgInt(i);
				break;
			case DATATYPE_FLOAT:
				sstream << GetOpArgFloat(i);
				break;
			case DATATYPE_STR:
				sstream << '"' << GetOpArgStr(i) << '"';
				break;
			default:
				sstream << "???";
				break;
		}
	}
	sstream << ")";
	
	sf_test_stringBuf = sstream.str();
	SetOpReturn(sf_test_stringBuf.c_str());
}

// returns current contents of metarule table
static void sf_get_metarule_table() {
	DWORD arr = TempArray(metaruleTable.size(), 0);
	int i = 0;
	for (MetaruleTableType::iterator it = metaruleTable.begin(); it != metaruleTable.end(); it++) {
		arrays[arr].val[i].set(it->first.c_str());
		i++;
	}
	SetOpReturn(arr, DATATYPE_INT);
}

/*
	Metarule array.

	Add your custom scripting functions here.

	Format is as follows:
	{ name, handler, minArgs, maxArgs, {MASK1, MASK2, ...} }
		- name - name of function that will be used to call it from scripts,
		- handler - pointer to handler function (see examples above),
		- minArgs/maxArgs - minimum and maximum number of arguments allowed for this function,
		- MASK1, MASK2, ... - validation parameters for each argument as bit masks (see DATATYPE_MASK_* defines)
*/
static const SfallMetarule metaruleArray[] = {
	{"test", sf_test, 0, 6, {}},
	{"get_metarule_table", sf_get_metarule_table, 0, 0, {}},
	{"validate_test", sf_test, 2, 5, {DATATYPE_MASK_INT, DATATYPE_MASK_INT | DATATYPE_MASK_FLOAT, DATATYPE_MASK_STR, DATATYPE_NONE}},
	{"spatial_radius", sf_spatial_radius, 1, 1, {DATATYPE_MASK_VALID_OBJ}},
	{"critter_inven_obj2", sf_critter_inven_obj2, 2, 2, {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{"intface_redraw", sf_intface_redraw, 0, 0, {}},
	{"intface_show", sf_intface_show, 0, 0, {}},
	{"intface_hide", sf_intface_hide, 0, 0, {}},
	{"intface_is_hidden", sf_intface_is_hidden, 0, 0, {}},
	{"exec_map_update_scripts", sf_exec_map_update_scripts, 0, 0, {}},
};

static void InitMetaruleTable() {
	int length = sizeof(metaruleArray) / sizeof(SfallMetarule);
	for (int i = 0; i < length; ++i) {
		metaruleTable[metaruleArray[i].name] = &metaruleArray[i];
	}
}

// Validates arguments against metarule specification.
// On error prints to debug.log and returns false.
static bool ValidateOpcodeArguments(const SfallMetarule* metaruleInfo) {
	int argCount = GetOpArgCount() - 1; // don't count function name
	if (argCount < metaruleInfo->minArgs || argCount > metaruleInfo->maxArgs) {
		PrintOpcodeError(
			"sfall_funcX(\"%s\", ...) - invalid number of arguments (%d), must be from %d to %d.", 
			metaruleInfo->name, 
			argCount,
			metaruleInfo->minArgs,
			metaruleInfo->maxArgs);

		return false;
	} else {
		for (int i = 0; i < argCount; i++) {
			int typeMask = metaruleInfo->argTypeMasks[i];
			ScriptValue arg = opArgs[i + 1];
			if (typeMask != 0 && ((1 << arg.type) & typeMask) == 0) {
				PrintOpcodeError(
					"sfall_funcX(\"%s\", ...) - argument #%d has invalid type: %s.", 
					metaruleInfo->name, 
					i + 1,
					GetSfallTypeName(arg.type));

				return false;
			} else if ((typeMask & DATATYPE_MASK_NOT_NULL) && arg.val.dw == 0) {
				PrintOpcodeError(
					"sfall_funcX(\"%s\", ...) - argument #%d is null.", 
					metaruleInfo->name, 
					i + 1);

				return false;
			}
		}
	}
	return true;
}

static void _stdcall op_sfall_metarule_handler() {
	if (IsOpArgStr(0)) {
		const char* name = GetOpArgStr(0);
		MetaruleTableType::iterator lookup = metaruleTable.find(name);
		if (lookup != metaruleTable.end()) {
			const SfallMetarule* metaruleInfo = lookup->second;
			if (ValidateOpcodeArguments(metaruleInfo)) {
				metaruleInfo->func();
			}
		} else {
			PrintOpcodeError("sfall_funcX(name, ...) - name '%s' is unknown.", name);
		}
	} else {		
		PrintOpcodeError("sfall_funcX(name, ...) - name must be string.");
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
