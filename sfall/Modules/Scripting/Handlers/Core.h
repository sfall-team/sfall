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

/* Opcodes for core sfall features. */

namespace sfall
{
namespace script
{

void op_typeof(OpcodeContext&);

void op_set_global_script_repeat();

void op_set_global_script_type();

void op_available_global_script_types();

void op_set_sfall_global(OpcodeContext&);

void op_get_sfall_global_int(OpcodeContext&);

void op_get_sfall_global_float(OpcodeContext&);

void op_get_sfall_arg(OpcodeContext&);

void mf_get_sfall_arg_at(OpcodeContext&);

void op_get_sfall_args(OpcodeContext&);

void op_set_sfall_arg(OpcodeContext&);

void op_set_sfall_return(OpcodeContext&);

void op_game_loaded();

void op_init_hook();

void op_set_self();

// used for both register_hook and register_hook_proc
void op_register_hook(OpcodeContext&);

void mf_add_g_timer_event(OpcodeContext&);

void mf_remove_timer_event(OpcodeContext&);

void op_sfall_ver_major();

void op_sfall_ver_minor();

void op_sfall_ver_build();

}
}
