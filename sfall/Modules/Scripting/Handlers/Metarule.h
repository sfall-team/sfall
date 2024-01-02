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

#pragma once

#include <string>
#include <unordered_map>

#include "..\..\..\main.h"
#include "..\..\ScriptExtender.h"

namespace sfall
{
namespace script
{

// Example handler. Feel free to add handlers in other files.
#ifndef NDEBUG
void mf_test(OpcodeContext&);
#endif

// returns current contents of metarule table
void mf_get_metarule_table(OpcodeContext&);

void mf_metarule_exist(OpcodeContext&);

void InitMetaruleTable();

void HandleMetarule(OpcodeContext& ctx);

}
}
