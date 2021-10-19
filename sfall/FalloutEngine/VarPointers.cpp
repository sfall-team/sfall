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

#include "VarPointers.h"

// Pointers to engine global variables

namespace fo
{
namespace ptr
{

// defines pointer to a variable (pointer is constant, but value can be changed)
#define PTR_(name, type) \
	type* name = reinterpret_cast<type*>(FO_VAR_##name);

// defines pointer to a constant variable (value can't be changed from sfall)
#define PTRC(name, type) \
	const type* name = reinterpret_cast<type*>(FO_VAR_##name);

// X-Macros pattern
#include "VarPointers_def.h"

}
}
