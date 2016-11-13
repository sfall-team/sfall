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

//file system functions

void __declspec() op_fs_create();

void __declspec() op_fs_copy();

void __declspec() op_fs_find();

void __declspec() op_fs_write_byte();

void __declspec() op_fs_write_short();

void __declspec() op_fs_write_int();

void __declspec() op_fs_write_string();

void __declspec() op_fs_write_bstring();

void __declspec() op_fs_read_byte();

void __declspec() op_fs_read_short();

void __declspec() op_fs_read_int();

void __declspec() op_fs_read_float();

void __declspec() op_fs_delete();

void __declspec() op_fs_size();

void __declspec() op_fs_pos();

void __declspec() op_fs_seek();

void __declspec() op_fs_resize();
