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

typedef std::tr1::unordered_map<std::string, void(*)()> MetaruleTableType;

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

static void InitMetaruleTable() {
	metaruleTable["test"] = sf_test;
	metaruleTable["get_metarule_table"] = sf_get_metarule_table;
}

static void _stdcall op_sfall_metarule_handler() {
	if (IsOpArgStr(0)) {
		const char* name = GetOpArgStr(0);
		MetaruleTableType::iterator lookup = metaruleTable.find(name);
		if (lookup != metaruleTable.end()) {
			lookup->second();
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
