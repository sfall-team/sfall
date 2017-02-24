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

void sf_create_array(OpcodeContext&);

void sf_set_array(OpcodeContext&);

/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
*/
void sf_get_array(OpcodeContext&);

void sf_free_array(OpcodeContext&);

void sf_len_array(OpcodeContext&);

void sf_resize_array(OpcodeContext&);

void sf_temp_array(OpcodeContext&);

void sf_fix_array(OpcodeContext&);

void sf_scan_array(OpcodeContext&);

void sf_save_array(OpcodeContext&);

void sf_load_array(OpcodeContext&);

void sf_get_array_key(OpcodeContext&);

void sf_stack_array(OpcodeContext&);

void sf_list_begin(OpcodeContext&);

void sf_list_as_array(OpcodeContext&);

void sf_list_next(OpcodeContext&);

void sf_list_end(OpcodeContext&);
