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

#include "FunctionOffsets.h"
#include "VariableOffsets.h"

#include "Functions.h"

namespace fo
{
namespace func
{

// Prints debug message to game debug.log file for develop build
#ifndef NDEBUG
__declspec(naked) void dev_printf(const char* fmt, ...) {
	__asm jmp fo::funcoffs::debug_printf_;
}
#else
void dev_printf(...) {}
#endif

// Fallout2.exe was compiled using WATCOM compiler, which uses Watcom register calling convention.
// In this convention, up to 4 arguments are passed via registers in this order: EAX, EDX, EBX, ECX.

#define WRAP_WATCOM_CALL0(offs) \
	__asm call fo::funcoffs::offs

#define WRAP_WATCOM_CALL1(offs, arg1) \
	__asm mov eax, arg1               \
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_CALL2(offs, arg1, arg2) \
	__asm mov edx, arg2                     \
	WRAP_WATCOM_CALL1(offs, arg1)

#define WRAP_WATCOM_CALL3(offs, arg1, arg2, arg3) \
	__asm mov ebx, arg3                           \
	WRAP_WATCOM_CALL2(offs, arg1, arg2)

#define WRAP_WATCOM_CALL4(offs, arg1, arg2, arg3, arg4) \
	__asm mov ecx, arg4                                 \
	WRAP_WATCOM_CALL3(offs, arg1, arg2, arg3)

#define WRAP_WATCOM_CALL5(offs, arg1, arg2, arg3, arg4, arg5) \
	__asm push arg5                                           \
	WRAP_WATCOM_CALL4(offs, arg1, arg2, arg3, arg4)

#define WRAP_WATCOM_CALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6) \
	__asm push arg6                                                 \
	WRAP_WATCOM_CALL5(offs, arg1, arg2, arg3, arg4, arg5)

#define WRAP_WATCOM_CALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	__asm push arg7                                                       \
	WRAP_WATCOM_CALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6)

#define WRAP_WATCOM_CALL8(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	__asm push arg8                                                             \
	WRAP_WATCOM_CALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define WRAP_WATCOM_CALL9(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	__asm push arg9                                                                   \
	WRAP_WATCOM_CALL8(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)

// defines wrappers for __fastcall
#define WRAP_WATCOM_FCALL1(offs, arg1) \
	__asm mov eax, ecx                 \
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_FCALL2(offs, arg1, arg2) \
	WRAP_WATCOM_FCALL1(offs, arg1)

#define WRAP_WATCOM_FCALL3(offs, arg1, arg2, arg3) \
	__asm mov ebx, arg3                            \
	WRAP_WATCOM_FCALL1(offs, arg1)

#define WRAP_WATCOM_FCALL4(offs, arg1, arg2, arg3, arg4) \
	__asm mov eax, ecx     \
	__asm mov ebx, arg3    \
	__asm mov ecx, arg4    \
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_FCALL5(offs, arg1, arg2, arg3, arg4, arg5) \
	__asm push arg5                                            \
	WRAP_WATCOM_FCALL4(offs, arg1, arg2, arg3, arg4)

#define WRAP_WATCOM_FCALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6) \
	__asm push arg6                                                  \
	WRAP_WATCOM_FCALL5(offs, arg1, arg2, arg3, arg4, arg5)

#define WRAP_WATCOM_FCALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	__asm push arg7                                                        \
	WRAP_WATCOM_FCALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6)

#define WRAP_WATCOM_FCALL8(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	__asm push arg8                                                              \
	WRAP_WATCOM_FCALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7)

#define WRAP_WATCOM_FCALL9(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	__asm push arg9                                                                    \
	WRAP_WATCOM_FCALL8(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)


// prints message to debug.log file
__declspec(naked) void debug_printf(const char* fmt, ...) {
	__asm jmp fo::funcoffs::debug_printf_;
}

void interpretReturnValue(Program* scriptPtr, DWORD val, DWORD valType) {
	__asm {
		mov  esi, scriptPtr;
		mov  edx, val;
		cmp  valType, VAR_TYPE_STR;
		jne  isNotStr;
		mov  eax, esi;
		call fo::funcoffs::interpretAddString_;
		mov  edx, eax;
isNotStr:
		mov  eax, esi;
		call fo::funcoffs::interpretPushLong_;  // pushes value to Data stack (must be followed by InterpretPushShort)
		mov  edx, valType;
		mov  eax, esi;
		call fo::funcoffs::interpretPushShort_; // pushes value type to Data stack (must be preceded by InterpretPushLong)
	}
}

DWORD __fastcall interpretGetValue(Program* scriptPtr, DWORD &outType) {
	__asm {
		mov  eax, ecx;
		call fo::funcoffs::interpretPopShort_; // pops value type from Data stack (must be followed by InterpretPopLong)
		mov  [edx], eax; // out type
		mov  edx, eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_; // pops value from Data stack (must be preceded by InterpretPopShort)
		cmp  dx, VAR_TYPE_STR;
		ja   isNotStr;
		mov  ebx, eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretGetString_; // retrieve string argument
isNotStr:
	}
}

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
__declspec(naked) void interpretError(const char* fmt, ...) {
	__asm jmp fo::funcoffs::interpretError_;
}

long __fastcall db_init(const char* path_dat, const char* path_patches) {
	__asm mov ebx, edx; // don't delete
	WRAP_WATCOM_FCALL2(db_init_, path_dat, path_patches);
}

long __fastcall tile_num(long x, long y) {
	__asm xor ebx, ebx; // don't delete (bug in tile_num_)
	WRAP_WATCOM_FCALL2(tile_num_, x, y);
}

void __fastcall square_xy(long x, long y, long* outSX, long* outSY) {
	__asm {
		xor  ebx, ebx; // don't delete (bug in square_xy_)
		mov  eax, ecx;
		push outSY;
		mov  ecx, outSX;
		call fo::funcoffs::square_xy_;
	}
}

GameObject* __fastcall obj_blocking_at_wrapper(GameObject* obj, DWORD tile, DWORD elevation, void* func) {
	__asm {
		mov  eax, ecx;
		mov  ebx, elevation;
		call func;
	}
}

long __stdcall win_register_button(DWORD winRef, long xPos, long yPos, long width, long height, long hoverOn, long hoverOff, long buttonDown, long buttonUp, BYTE* pictureUp, BYTE* pictureDown, long arg12, long buttonType) {
	__asm {
		push buttonType;
		push arg12;
		push pictureDown;
		push pictureUp;
		push buttonUp;
		push buttonDown;
		push hoverOff;
		push hoverOn;
		push height;
		mov  ecx, width;
		mov  ebx, yPos;
		mov  edx, xPos;
		mov  eax, winRef;
		call fo::funcoffs::win_register_button_;
	}
}

void __stdcall DialogOut(const char* text) {
	__asm {
		push 1;          // DIALOGOUT_NORMAL flag
		xor  edx, edx;
		push edx;
		push edx;
		mov  dl, byte ptr ds:[0x6AB718];
		push edx;        // ColorMsg
		mov  ecx, 192;   // x
		push 116;        // y
		mov  eax, text;  // DisplayText
		xor  ebx, ebx;
		call fo::funcoffs::dialog_out_;
	}
}

long __fastcall DialogOutEx(const char* text, const char** textEx, long lines, long flags, long colors) {
	__asm {
		mov  ebx, colors; // Color index
		xor  eax, eax;
		push flags;
		test ebx, ebx;
		jnz  cColor;
		mov  al, byte ptr ds:[0x6AB718];
		mov  bl, al;
		jmp  skip;
cColor:
		mov  al, bh;
		and  ebx, 0xFF
skip:
		push eax;        // ColorMsg2
		push 0;          // DisplayMsg (unknown)
		mov  eax, ecx;   // DisplayText (first line)
		push ebx;        // ColorMsg1
		mov  ecx, 192;   // x
		push 116;        // y
		mov  ebx, lines; // count second lines
		call fo::funcoffs::dialog_out_; // edx - DisplayText (seconds lines)
	}
}

void __fastcall DrawWinLine(int winRef, DWORD startXPos, DWORD endXPos, DWORD startYPos, DWORD endYPos, BYTE colour) {
	__asm {
		xor  eax, eax;
		mov  al, colour;
		push eax;
		push endYPos;
		mov  eax, ecx; // winRef
		mov  ecx, endXPos;
		mov  ebx, startYPos;
		//mov  edx, xStartPos;
		call fo::funcoffs::win_line_;
	}
}

// draws an image to the buffer without scaling and with transparency display toggle
void __fastcall windowDisplayBuf(long x, long width, long y, long height, void* data, long noTrans) {
	__asm {
		push height;
		push edx;       // from_width
		push y;
		mov  eax, data; // from
		mov  ebx, fo::funcoffs::windowDisplayTransBuf_;
		cmp  noTrans, 0;
		cmovnz ebx, fo::funcoffs::windowDisplayBuf_;
		call ebx; // *data<eax>, from_width<edx>, unused<ebx>, X<ecx>, Y, width, height
	}
}

// draws an image in the window and scales it to fit the window
void __fastcall displayInWindow(long w_here, long width, long height, void* data) {
	__asm {
		mov  ebx, height;
		mov  eax, data;
		call fo::funcoffs::displayInWindow_; // *data<eax>, width<edx>, height<ebx>, where<ecx>
	}
}

// draws an image to the buffer of the active script window
void __fastcall window_trans_cscale(long i_width, long i_height, long s_width, long s_height, long xy_shift, long w_width, void* data) {
	__asm {
		push w_width;
		push s_height;
		push s_width;
		call fo::funcoffs::windowGetBuffer_;
		add  eax, xy_shift;
		mov  ebx, edx; // i_height
		mov  edx, ecx; // i_width
		push eax;      // to_buff
		mov  eax, data;
		call fo::funcoffs::trans_cscale_; // *from_buff<eax>, i_width<edx>, i_height<ebx>, i_width2<ecx>, to_buff, width, height, to_width
	}
}

// buf_to_buf_ function in pure SSE implementation
void __cdecl buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width) {
	if (height <= 0 || width <= 0) return;

	size_t blockCount = width / 64; // 64 bytes
	size_t remainder = width % 64;
	size_t sizeD = remainder >> 2;
	size_t sizeB = remainder & 3;
	size_t s_pitch = src_width - width;
	size_t d_pitch = dst_width - width;

	__asm {
		mov  ebx, s_pitch;
		mov  edx, d_pitch;
		mov  esi, src;
		mov  edi, dst;
		mov  eax, height;
startLoop:
		mov  ecx, blockCount;
		test ecx, ecx;
		jz   copySmall;
copyBlock: // copies block of 64 bytes
		movups xmm0, [esi];
		movups xmm1, [esi + 16];
		movups xmm2, [esi + 32];
		movups xmm3, [esi + 48];
		movups [edi], xmm0;
		movups [edi + 16], xmm1;
		movups [edi + 32], xmm2;
		movups [edi + 48], xmm3;
		add  esi, 64;
		lea  edi, [edi + 64];
		dec  ecx; // blockCount
		jnz  copyBlock;
		// copies the remaining bytes
		mov  ecx, sizeD;
		rep  movsd;
		mov  ecx, sizeB;
		rep  movsb;
		add  esi, ebx; // s_pitch
		add  edi, edx; // d_pitch
		dec  eax;      // height
		jnz  startLoop;
		jmp  end;
copySmall: // copies the small size data
		mov  ecx, sizeD;
		rep  movsd;
		mov  ecx, sizeB;
		rep  movsb;
		add  esi, ebx; // s_pitch
		add  edi, edx; // d_pitch
		dec  eax;      // height
		jnz  copySmall;
end:
	}
}

// trans_buf_to_buf_ function implementation
void __cdecl trans_buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width) {
	if (height <= 0 || width <= 0) return;

	size_t blockCount = width >> 3;
	size_t lastBytes  = width & 7;
	size_t s_pitch = src_width - width;
	size_t d_pitch = dst_width - width;

	__asm mov esi, src;
	__asm mov edi, dst;
	do {
		size_t count = blockCount;
		while (count--) {
			__asm {
				pxor mm2, mm2;
				movq mm0, qword ptr [esi]; // 8 bytes
				movq mm1, qword ptr [edi];
				pcmpeqb mm2, mm0;          // mm2 = (src == 0) ? 1 : 0;
				lea  esi, [esi + 8];
				pand mm2, mm1;
				por  mm0, mm2;
				movq qword ptr [edi], mm0; // src or dst
				lea  edi, [edi + 8];
			}
		}
		size_t bytes = lastBytes;
		while (bytes--) {
			__asm {
				mov  al, [esi];
				lea  esi, [esi + 1];
				test al, al;
				jz   skip;
				mov  [edi], al;
			skip:
				lea  edi, [edi + 1];
			}
		}
		__asm add esi, s_pitch;
		__asm add edi, d_pitch;
	} while (--height);
	__asm emms;
}

long __fastcall get_game_config_string(const char** outValue, const char* section, const char* param) {
	__asm {
		mov  ebx, param;
		mov  eax, FO_VAR_game_config;
		call fo::funcoffs::config_get_string_; // section<edx>, outValue<ecx>
	}
}

void __stdcall freePtr_invoke(void *p) {
	__asm mov  eax, p;
	__asm call ds:[FO_VAR_freePtr];
}

////////////////////////////////////
// X-Macro for wrapper functions. //
////////////////////////////////////

#define WRAP_WATCOM_FUNC0(retType, name) \
	retType __stdcall name() { \
		WRAP_WATCOM_CALL0(name##_) \
	}

#define WRAP_WATCOM_FUNC1(retType, name, arg1t, arg1) \
	retType __stdcall name(arg1t arg1) { \
		WRAP_WATCOM_CALL1(name##_, arg1) \
	}

#define WRAP_WATCOM_FUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __stdcall name(arg1t arg1, arg2t arg2) { \
		WRAP_WATCOM_CALL2(name##_, arg1, arg2) \
	}

#define WRAP_WATCOM_FUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3) { \
		WRAP_WATCOM_CALL3(name##_, arg1, arg2, arg3) \
	}

#define WRAP_WATCOM_FUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4) { \
		WRAP_WATCOM_CALL4(name##_, arg1, arg2, arg3, arg4) \
	}

#define WRAP_WATCOM_FUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5) { \
		WRAP_WATCOM_CALL5(name##_, arg1, arg2, arg3, arg4, arg5) \
	}

#define WRAP_WATCOM_FUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6) { \
		WRAP_WATCOM_CALL6(name##_, arg1, arg2, arg3, arg4, arg5, arg6) \
	}

#define WRAP_WATCOM_FUNC7(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7) { \
		WRAP_WATCOM_CALL7(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	}

#define WRAP_WATCOM_FUNC8(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8) { \
		WRAP_WATCOM_CALL8(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	}

#define WRAP_WATCOM_FUNC9(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8, arg9t, arg9) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8, arg9t arg9) { \
		WRAP_WATCOM_CALL9(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	}


#define WRAP_WATCOM_FFUNC1(retType, name, arg1t, arg1) \
	retType __fastcall name(arg1t arg1) { \
		WRAP_WATCOM_FCALL1(name##_, arg1) \
	}

#define WRAP_WATCOM_FFUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __fastcall name(arg1t arg1, arg2t arg2) { \
		WRAP_WATCOM_FCALL2(name##_, arg1, arg2) \
	}

#define WRAP_WATCOM_FFUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3) { \
		WRAP_WATCOM_FCALL3(name##_, arg1, arg2, arg3) \
	}

#define WRAP_WATCOM_FFUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4) { \
		WRAP_WATCOM_FCALL4(name##_, arg1, arg2, arg3, arg4) \
	}

#define WRAP_WATCOM_FFUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5) { \
		WRAP_WATCOM_FCALL5(name##_, arg1, arg2, arg3, arg4, arg5) \
	}

#define WRAP_WATCOM_FFUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6) { \
		WRAP_WATCOM_FCALL6(name##_, arg1, arg2, arg3, arg4, arg5, arg6) \
	}

#define WRAP_WATCOM_FFUNC7(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7) { \
		WRAP_WATCOM_FCALL7(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	}

#define WRAP_WATCOM_FFUNC8(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8) { \
		WRAP_WATCOM_FCALL8(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	}

#define WRAP_WATCOM_FFUNC9(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8, arg9t, arg9) \
	retType __fastcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8, arg9t arg9) { \
		WRAP_WATCOM_FCALL9(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	}

#include "Functions_def.h"

}
}
