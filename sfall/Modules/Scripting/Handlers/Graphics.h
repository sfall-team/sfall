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

// graphics_functions
void __declspec() op_graphics_funcs_available();

void __declspec() op_load_shader();

void __declspec() op_free_shader();

void __declspec() op_activate_shader();

void __declspec() op_deactivate_shader();

void __declspec() op_get_shader_texture();

void __declspec() op_set_shader_int();

void __declspec() op_set_shader_texture();

void __declspec() op_set_shader_float();

void __declspec() op_set_shader_vector();

void __declspec() op_get_shader_version();

void __declspec() op_set_shader_mode();

void __declspec() op_force_graphics_refresh();
