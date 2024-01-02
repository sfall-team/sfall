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

class OpcodeContext;

void op_string_split(OpcodeContext&);

void op_atoi(OpcodeContext&);

void op_atof(OpcodeContext&);

void op_substr(OpcodeContext&);

void op_strlen(OpcodeContext&);

void mf_string_compare(OpcodeContext&);

void mf_string_find(OpcodeContext&);

void op_sprintf(OpcodeContext&);

void mf_string_format(OpcodeContext&);

void op_ord(OpcodeContext&);

void op_message_str_game(OpcodeContext&);

void mf_add_extra_msg_file(OpcodeContext&);

void mf_get_string_pointer(OpcodeContext&);

void mf_get_text_width(OpcodeContext&);

void mf_string_to_case(OpcodeContext&);

}
}
