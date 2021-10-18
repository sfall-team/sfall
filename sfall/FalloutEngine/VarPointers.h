/*
 *    sfall
 *    Copyright (C) 2008-2017  The sfall team
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

#include "VariableOffsets.h"
#include "Structs.h"

//
// Global variable pointers.
//
// In normal CPP code use: *ptr_name to read/write value or ptr_name to use as pointer.
//

// defines pointer to an engine variable
#define PTR_(name, type) \
	extern type* ptr_##name;

#define PTRC(name, type) \
	extern const type* ptr_##name;

// X-Macros pattern
#include "VarPointers_def.h"


//namespace var // 4.x
//{

__inline long var_getInt(DWORD addr) {
	return *reinterpret_cast<DWORD*>(addr);
}

__inline BYTE var_getByte(DWORD addr) {
	return *reinterpret_cast<BYTE*>(addr);
}

__inline long& var_setInt(DWORD addr) {
	return *reinterpret_cast<long*>(addr);
}

__inline BYTE& var_setByte(DWORD addr) {
	return *reinterpret_cast<BYTE*>(addr);
}
