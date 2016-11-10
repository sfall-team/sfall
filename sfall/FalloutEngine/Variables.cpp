/*
* sfall
* Copyright (C) 2008-2016 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Variables.h"

// Pointers to engine global variables

namespace VarPtr
{

// defines reference to a variable (pointer is constant, but value can be changed)
#define _VAR_(name, type)	\
	type &name = *reinterpret_cast<type*>(VARPTR_##name);

// defines reference to static array
#define _VARA(name, type, size)	\
	ArrayWrapper<type, size> &name = *reinterpret_cast<ArrayWrapper<type, size>*>(VARPTR_##name);

// defines reference to static 2-dimensional array
#define _VAR2(name, type, size1, size2)	\
	ArrayWrapper<ArrayWrapper<type, size2>, size1> &name = *reinterpret_cast<ArrayWrapper<ArrayWrapper<type, size2>, size1>*>(VARPTR_##name);

// defines reference to static 3-dimensional array
#define _VAR3(name, type, size1, size2, size3)	\
	ArrayWrapper<ArrayWrapper<ArrayWrapper<type, size3>, size2>, size1> &name = *reinterpret_cast<ArrayWrapper<ArrayWrapper<ArrayWrapper<type, size3>, size2>, size1>*>(VARPTR_##name);

// defines const pointer to variable (useful for static arrays, when exact size is unknown)
#define _VARP(name, type)	\
	type* const name = reinterpret_cast<type*>(VARPTR_##name);

// X-Macros pattern
#include "Variables_def.h"

}
