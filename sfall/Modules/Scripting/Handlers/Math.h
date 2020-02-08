/*
 *    sfall
 *    Copyright (C) 2008-2020  The sfall team
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

void sf_div(OpcodeContext&);

void sf_sqrt(OpcodeContext&);

void sf_abs(OpcodeContext&);

void sf_sin(OpcodeContext&);

void sf_cos(OpcodeContext&);

void sf_tan(OpcodeContext&);

void sf_arctan(OpcodeContext&);

void sf_power(OpcodeContext&);

void sf_log(OpcodeContext&);

void sf_exponent(OpcodeContext&);

void sf_ceil(OpcodeContext&);

void sf_round(OpcodeContext&);

void sf_floor2(OpcodeContext&);

}
}
