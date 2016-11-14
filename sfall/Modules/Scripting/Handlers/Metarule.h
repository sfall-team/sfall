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

#pragma once

#include <string>
#include <sstream>
#include <unordered_map>

#include "..\..\..\main.h"
#include "..\..\ScriptExtender.h"


// Example handler. Feel free to add handlers in other files.
void sf_test(OpcodeHandler&);

// returns current contents of metarule table
void sf_get_metarule_table(OpcodeHandler&);

void InitMetaruleTable();

#define metaruleOpcode(numArg, numArgPlusOne) \
	void __declspec() op_sfall_metarule##numArg();

metaruleOpcode(0, 1)
metaruleOpcode(1, 2)
metaruleOpcode(2, 3)
metaruleOpcode(3, 4)
metaruleOpcode(4, 5)
metaruleOpcode(5, 6)
metaruleOpcode(6, 7)

#undef metaruleOpcode
