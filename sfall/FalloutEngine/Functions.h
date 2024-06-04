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

#include "Structs.h"

//
// WRAPPERS for FO engine functions.
// Use those as you would if there were source code for the engine...
//
namespace fo
{
namespace func
{

#ifndef NDEBUG
// Prints debug message to game debug.log file for develop build
void dev_printf(const char* fmt, ...);
#else
void dev_printf(...);
#endif

/*
	Add functions here if they have non-trivial wrapper implementation (like vararg functions or too many arguments, etc.)
	Otherwise use Functions_def.h file (much easier).
*/

// prints message to debug.log file
void debug_printf(const char* fmt, ...);

void interpretReturnValue(Program* scriptPtr, DWORD val, DWORD valType);

DWORD __fastcall interpretGetValue(Program* scriptPtr, DWORD &outType);

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void interpretError(const char* fmt, ...);

long __fastcall db_init(const char* path_dat, const char* path_patches);

long __fastcall tile_num(long x, long y);

void __fastcall square_xy(long x, long y, long* outSX, long* outSY);

GameObject* __fastcall obj_blocking_at_wrapper(GameObject* obj, DWORD tile, DWORD elevation, void* func);

// Creates a button on a given window
// buttonType: 0x10 = move window pos, 0x20 or 0x0 = regular click, 0x23 = toggle click
// pictureUp/pictureDown - pointers to a surface
long __stdcall win_register_button(DWORD winRef, long xPos, long yPos, long width, long height, long hoverOn, long hoverOff, long buttonDown, long buttonUp, BYTE* pictureUp, BYTE* pictureDown, long arg12, long buttonType);

void __stdcall DialogOut(const char* text);

long __fastcall DialogOutEx(const char* text, const char** textEx, long lines, long flags, long colors = 0);

// draws an image to the buffer without scaling and with transparency display toggle
void __fastcall windowDisplayBuf(long x, long width, long y, long height, void* data, long noTrans);

// draws an image in the window and scales it to fit the window
void __fastcall displayInWindow(long w_here, long width, long height, void* data);

// draws an image to the buffer of the active script window
void __fastcall window_trans_cscale(long i_width, long i_height, long s_width, long s_height, long xy_shift, long w_width, void* data);

// buf_to_buf_ function in pure SSE implementation
void __cdecl buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width);

// trans_buf_to_buf_ function implementation
void __cdecl trans_buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width);

long __fastcall get_game_config_string(const char** outValue, const char* section, const char* param);

void __stdcall freePtr_invoke(void *ptr);

// X-Macro for wrapper functions.
#define WRAP_WATCOM_FUNC0(retType, name) \
	retType __stdcall name();

#define WRAP_WATCOM_FUNC1(retType, name, arg1t, arg1) \
	retType __stdcall name(arg1t arg1);

#define WRAP_WATCOM_FUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __stdcall name(arg1t arg1, arg2t arg2);

#define WRAP_WATCOM_FUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3);

#define WRAP_WATCOM_FUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4);

#define WRAP_WATCOM_FUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5);

#define WRAP_WATCOM_FUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6);

#define WRAP_WATCOM_FUNC7(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7);

#define WRAP_WATCOM_FUNC8(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8);

#define WRAP_WATCOM_FUNC9(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8, arg9t, arg9) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8, arg9t arg9);


#define WRAP_WATCOM_FFUNC1(retType, name, arg1t, arg1) \
	retType __fastcall name(arg1t arg1);

#define WRAP_WATCOM_FFUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __fastcall name(arg1t arg1, arg2t arg2);

#define WRAP_WATCOM_FFUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3);

#define WRAP_WATCOM_FFUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4);

#define WRAP_WATCOM_FFUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5);

#define WRAP_WATCOM_FFUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6);

#define WRAP_WATCOM_FFUNC7(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7);

#define WRAP_WATCOM_FFUNC8(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8);

#define WRAP_WATCOM_FFUNC9(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8, arg9t, arg9) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8, arg9t arg9);

#include "Functions_def.h"

#undef WRAP_WATCOM_FUNC0
#undef WRAP_WATCOM_FUNC1
#undef WRAP_WATCOM_FUNC2
#undef WRAP_WATCOM_FUNC3
#undef WRAP_WATCOM_FUNC4
#undef WRAP_WATCOM_FUNC5
#undef WRAP_WATCOM_FUNC6
#undef WRAP_WATCOM_FUNC7
#undef WRAP_WATCOM_FUNC8
#undef WRAP_WATCOM_FUNC9

#undef WRAP_WATCOM_FFUNC1
#undef WRAP_WATCOM_FFUNC2
#undef WRAP_WATCOM_FFUNC3
#undef WRAP_WATCOM_FFUNC4
#undef WRAP_WATCOM_FFUNC5
#undef WRAP_WATCOM_FFUNC6
#undef WRAP_WATCOM_FFUNC7
#undef WRAP_WATCOM_FFUNC8
#undef WRAP_WATCOM_FFUNC9

}
}
