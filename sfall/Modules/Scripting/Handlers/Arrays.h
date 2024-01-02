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

namespace sfall
{
namespace script
{

void op_create_array(OpcodeContext&);

void op_set_array(OpcodeContext&);

/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
*/
void op_get_array(OpcodeContext&);

void op_free_array(OpcodeContext&);

void op_len_array(OpcodeContext&);

void op_resize_array(OpcodeContext&);

void op_temp_array(OpcodeContext&);

void op_fix_array(OpcodeContext&);

void op_scan_array(OpcodeContext&);

void op_save_array(OpcodeContext&);

void op_load_array(OpcodeContext&);

void op_get_array_key(OpcodeContext&);

void op_stack_array(OpcodeContext&);

void op_list_begin(OpcodeContext&);

void op_list_as_array(OpcodeContext&);

void op_list_next(OpcodeContext&);

void op_list_end(OpcodeContext&);

}
}
