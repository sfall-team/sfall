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

#include "..\OpcodeContext.h"

namespace sfall
{
namespace script
{

void op_get_ini_setting(OpcodeContext&);

void op_get_ini_string(OpcodeContext&);

void op_modified_ini(OpcodeContext&);

void mf_set_ini_setting(OpcodeContext&);

void mf_get_ini_sections(OpcodeContext&);

void mf_get_ini_section(OpcodeContext&);

void mf_get_ini_config(OpcodeContext&);

void ResetIniCache();

}
}
