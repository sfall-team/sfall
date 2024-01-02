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

void op_div(OpcodeContext&);

void op_sqrt(OpcodeContext&);

void op_abs(OpcodeContext&);

void op_sin(OpcodeContext&);

void op_cos(OpcodeContext&);

void op_tan(OpcodeContext&);

void op_arctan(OpcodeContext&);

void op_power(OpcodeContext&);

void op_log(OpcodeContext&);

void op_exponent(OpcodeContext&);

void op_ceil(OpcodeContext&);

void op_round(OpcodeContext&);

void mf_floor2(OpcodeContext&);

}
}
