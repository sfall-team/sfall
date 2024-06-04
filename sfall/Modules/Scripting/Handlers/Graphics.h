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

// graphics_functions
void op_graphics_funcs_available();

void op_load_shader();

void op_free_shader();

void op_activate_shader();

void op_deactivate_shader();

void op_get_shader_texture();

void op_set_shader_int();

void op_set_shader_texture();

void op_set_shader_float();

void op_set_shader_vector();

void op_get_shader_version();

void op_set_shader_mode();

void op_force_graphics_refresh();

}
}
