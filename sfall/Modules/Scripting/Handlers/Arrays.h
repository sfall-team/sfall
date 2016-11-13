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

void __declspec() op_create_array();

void __declspec() op_set_array();

/*
	used in place of [] operator when compiling in sslc
	so it works as get_array if first argument is int and as substr(x, y, 1) if first argument is string
*/
void __declspec() op_get_array();

void __declspec() op_free_array();

void __declspec() op_len_array();

void __declspec() op_resize_array();

void __declspec() op_temp_array();

void __declspec() op_fix_array();

void __declspec() op_scan_array();

void __declspec() op_save_array();

void __declspec() op_load_array();

void __declspec() op_get_array_key();

void __declspec() op_stack_array();

void __declspec() op_list_begin();

void __declspec() op_list_as_array();

void __declspec() op_list_next();

void __declspec() op_list_end();
