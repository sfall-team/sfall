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

/*
	MACROS for operators assembly code

	Notes:
	- DO NOT add any comments within macros
	- every macro should contain __asm {} block
	- every assembly line should start with __asm and should NOT have semicolon in the end!
	- use this macros outside of other __asm {} blocks (obviously)
*/

/*
	Gets argument from stack to eax and puts its type to edx register
	eax register must contain the script_ptr
	jlabel - name of the jump label in case the value type is not INT
	return: eax - arg value
*/
#define _GET_ARG_INT(jlabel) __asm {				\
	__asm mov  edx, eax								\
	__asm call fo::funcoffs::interpretPopShort_		\
	__asm xchg eax, edx								\
	__asm call fo::funcoffs::interpretPopLong_		\
	__asm cmp  dx, VAR_TYPE_INT						\
	__asm jnz  jlabel								\
}

#define _GET_ARG(outVal, outType) __asm {			\
	__asm call fo::funcoffs::interpretPopShort_		\
	__asm mov  outType, eax							\
	__asm mov  eax, ebx								\
	__asm call fo::funcoffs::interpretPopLong_		\
	__asm mov  outVal, eax							\
}

/*
	Returns the value to the script
	eax and ebx register must contain the script_ptr
	edx register must contain the returned value
*/
#define _RET_VAL_INT __asm {						\
	__asm call fo::funcoffs::interpretPushLong_		\
	__asm mov  edx, VAR_TYPE_INT					\
	__asm mov  eax, ebx								\
	__asm call fo::funcoffs::interpretPushShort_	\
}

#define _J_RET_VAL_TYPE(type) __asm {				\
	__asm call fo::funcoffs::interpretPushLong_		\
	__asm mov  edx, type							\
	__asm mov  eax, ebx								\
	__asm jmp  fo::funcoffs::interpretPushShort_	\
}
