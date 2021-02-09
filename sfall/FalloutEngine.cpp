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

#include "main.h"

#include "FalloutEngine.h"
#include "Logging.h"

// Pointers to engine global variables

// defines pointer to an engine variable
#define PTR_(name, type)	\
	type* ptr_##name = reinterpret_cast<type*>(FO_VAR_##name);

// X-Macros pattern
#include "FalloutVars_def.h"


/**
	ENGINE FUNCTION OFFSETS
	const names should end with underscore
*/

#define FUNC(name, addr) const DWORD name = addr;

#include "FalloutFuncOffs_def.h"

#undef FUNC

// WRAPPERS

// Prints debug message to game debug.log file for develop build
#ifndef NDEBUG
void __declspec(naked) dev_printf(const char* fmt, ...) {
	__asm jmp debug_printf_;
}
#else
void dev_printf(...) {}
#endif

// Fallout2.exe was compiled using WATCOM compiler, which uses Watcom register calling convention.
// In this convention, up to 4 arguments are passed via registers in this order: EAX, EDX, EBX, ECX.

#define WRAP_WATCOM_CALL0(offs) \
	__asm call offs

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
void __declspec(naked) fo_debug_printf(const char* fmt, ...) {
	__asm jmp debug_printf_;
}

void __stdcall fo_interpretReturnValue(TProgram* scriptPtr, DWORD val, DWORD valType) {
	__asm {
		mov  esi, scriptPtr;
		mov  edx, val;
		cmp  valType, VAR_TYPE_STR;
		jne  isNotStr;
		mov  eax, esi;
		call interpretAddString_;
		mov  edx, eax;
isNotStr:
		mov  eax, esi;
		call interpretPushLong_;  // pushes value to Data stack (must be followed by InterpretPushShort)
		mov  edx, valType;
		mov  eax, esi;
		call interpretPushShort_; // pushes value type to Data stack (must be preceded by InterpretPushLong)
	}
}

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec(naked) fo_interpretError(const char* fmt, ...) {
	__asm jmp interpretError_;
}

long __fastcall fo_tile_num(long x, long y) {
	__asm push ebx; // don't delete (bug in tile_num_)
	WRAP_WATCOM_FCALL2(tile_num_, x, y)
	__asm pop  ebx;
}

TGameObj* __fastcall obj_blocking_at_wrapper(TGameObj* obj, DWORD tile, DWORD elevation, void* func) {
	__asm {
		mov  eax, ecx;
		mov  ebx, elevation;
		call func;
	}
}

long __stdcall fo_win_register_button(DWORD winRef, long xPos, long yPos, long width, long height, long hoverOn, long hoverOff, long buttonDown, long buttonUp, BYTE* pictureUp, BYTE* pictureDown, long arg12, long buttonType) {
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
		call win_register_button_;
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
		call dialog_out_;
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
		call dialog_out_; // edx - DisplayText (seconds lines)
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
		call win_line_;
	}
}

// draws an image to the buffer without scaling and with transparency display toggle
void __fastcall fo_windowDisplayBuf(long x, long width, long y, long height, void* data, long noTrans) {
	__asm {
		push height;
		push edx;       // from_width
		push y;
		mov  eax, data; // from
		mov  ebx, windowDisplayTransBuf_;
		cmp  noTrans, 0;
		cmovnz ebx, windowDisplayBuf_;
		call ebx; // *data<eax>, from_width<edx>, unused<ebx>, X<ecx>, Y, width, height
	}
}

// draws an image in the window and scales it to fit the window
void __fastcall fo_displayInWindow(long w_here, long width, long height, void* data) {
	__asm {
		mov  ebx, height;
		mov  eax, data;
		call displayInWindow_; // *data<eax>, width<edx>, height<ebx>, where<ecx>
	}
}

// draws an image to the buffer of the active script window
void __fastcall window_trans_cscale(long i_width, long i_height, long s_width, long s_height, long xy_shift, long w_width, void* data) {
	__asm {
		push w_width;
		push s_height;
		push s_width;
		call windowGetBuffer_;
		add  eax, xy_shift;
		mov  ebx, edx; // i_height
		mov  edx, ecx; // i_width
		push eax;      // to_buff
		mov  eax, data;
		call trans_cscale_; // *from_buff<eax>, i_width<edx>, i_height<ebx>, i_width2<ecx>, to_buff, width, height, to_width
	}
}

// buf_to_buf_ function with pure MMX implementation
void __cdecl fo_buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width) {
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
		movq mm0, [esi];      // movups xmm0, [esi]; // SSE implementation
		movq mm1, [esi + 8];
		movq mm2, [esi + 16]; // movups xmm1, [esi + 16];
		movq mm3, [esi + 24];
		movq mm4, [esi + 32]; // movups xmm2, [esi + 32];
		movq mm5, [esi + 40];
		movq mm6, [esi + 48]; // movups xmm3, [esi + 48];
		movq mm7, [esi + 56];
		movq [edi], mm0;      // movups [edi], xmm0;
		movq [edi + 8], mm1;
		movq [edi + 16], mm2; // movups [edi + 16], xmm1;
		movq [edi + 24], mm3;
		movq [edi + 32], mm4; // movups xmm2, [esi + 32];
		movq [edi + 40], mm5;
		movq [edi + 48], mm6; // movups xmm3, [esi + 48];
		movq [edi + 56], mm7;
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
		emms;
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
void __cdecl fo_trans_buf_to_buf(BYTE* src, long width, long height, long src_width, BYTE* dst, long dst_width) {
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

BYTE* __fastcall fo_loadPCX(const char* file, long* width, long* height) {
	__asm {
		mov  eax, ecx;
		mov  ebx, height;
		mov  ecx, FO_VAR_pal;
		call loadPCX_;
		push eax;
		mov  ebx, [width];
		mov  edx, FO_VAR_pal;
		mov  ecx, [height];
		mov  ebx, [ebx];
		mov  ecx, [ecx];
		call datafileConvertData_;
		pop  eax;
	}
}

long __fastcall fo_get_game_config_string(const char* outValue, const char* section, const char* param) {
	__asm {
		mov  ebx, param;
		mov  eax, FO_VAR_game_config;
		call config_get_string_; // section<edx>, outValue<ecx>
	}
}

////////////////////////////////////
// X-Macro for wrapper functions. //
////////////////////////////////////

#define WRAP_WATCOM_FUNC0(retType, name) \
	retType __stdcall fo_##name() { \
		WRAP_WATCOM_CALL0(name##_) \
	}

#define WRAP_WATCOM_FUNC1(retType, name, arg1t, arg1) \
	retType __stdcall fo_##name(arg1t arg1) { \
		WRAP_WATCOM_CALL1(name##_, arg1) \
	}

#define WRAP_WATCOM_FUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2) { \
		WRAP_WATCOM_CALL2(name##_, arg1, arg2) \
	}

#define WRAP_WATCOM_FUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3) { \
		WRAP_WATCOM_CALL3(name##_, arg1, arg2, arg3) \
	}

#define WRAP_WATCOM_FUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4) { \
		WRAP_WATCOM_CALL4(name##_, arg1, arg2, arg3, arg4) \
	}

#define WRAP_WATCOM_FUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5) { \
		WRAP_WATCOM_CALL5(name##_, arg1, arg2, arg3, arg4, arg5) \
	}

#define WRAP_WATCOM_FUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __stdcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6) { \
		WRAP_WATCOM_CALL6(name##_, arg1, arg2, arg3, arg4, arg5, arg6) \
	}


#define WRAP_WATCOM_FFUNC1(retType, name, arg1t, arg1) \
	retType __fastcall fo_##name(arg1t arg1) { \
		WRAP_WATCOM_FCALL1(name##_, arg1) \
	}

#define WRAP_WATCOM_FFUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2) { \
		WRAP_WATCOM_FCALL2(name##_, arg1, arg2) \
	}

#define WRAP_WATCOM_FFUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3) { \
		WRAP_WATCOM_FCALL3(name##_, arg1, arg2, arg3) \
	}

#define WRAP_WATCOM_FFUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4) { \
		WRAP_WATCOM_FCALL4(name##_, arg1, arg2, arg3, arg4) \
	}

#define WRAP_WATCOM_FFUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5) { \
		WRAP_WATCOM_FCALL5(name##_, arg1, arg2, arg3, arg4, arg5) \
	}

#define WRAP_WATCOM_FFUNC6(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6) { \
		WRAP_WATCOM_FCALL6(name##_, arg1, arg2, arg3, arg4, arg5, arg6) \
	}

#define WRAP_WATCOM_FFUNC7(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7) { \
		WRAP_WATCOM_FCALL7(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	}

#define WRAP_WATCOM_FFUNC8(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8) { \
		WRAP_WATCOM_FCALL8(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
	}

#define WRAP_WATCOM_FFUNC9(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5, arg6t, arg6, arg7t, arg7, arg8t, arg8, arg9t, arg9) \
	retType __fastcall fo_##name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t arg4, arg5t arg5, arg6t arg6, arg7t arg7, arg8t arg8, arg9t arg9) { \
		WRAP_WATCOM_FCALL9(name##_, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
	}

#include "FalloutFuncs_def.h"

///////////////////////////////// ENGINE UTILS /////////////////////////////////

static MSGNode messageBuf;

const char* GetMessageStr(const MSGList* fileAddr, long messageId) {
	return fo_getmsg(fileAddr, &messageBuf, messageId);
}

const char* MessageSearch(const MSGList* fileAddr, long messageId) {
	messageBuf.number = messageId;
	if (fo_message_search(fileAddr, &messageBuf) == 1) {
		return messageBuf.message;
	}
	return nullptr;
}

Queue* QueueFind(TGameObj* object, long type) {
	if (*ptr_queue) {
		Queue* queue = *ptr_queue;
		while (queue->object != object && queue->type != type) {
			queue = queue->next;
			if (!queue) break;
		}
		return queue;
	}
	return nullptr;
}

long AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		sProto* proto = nullptr;
		if (GetProto(weapon->protoId, proto) && proto->item.type == item_type_weapon) {
			return proto->item.weapon.animationCode;
		}
	}
	return 0;
}

bool GetProto(long pid, sProto* outProto) {
	return (fo_proto_ptr(pid, &outProto) != -1);
}

void SkillGetTags(long* result, long num) {
	if (num > 4) num = 4;
	fo_skill_get_tags(result, num);
}

void SkillSetTags(long* tags, long num) {
	if (num > 4) num = 4;
	fo_skill_set_tags(tags, num);
}

long __fastcall GetItemType(TGameObj* item) {
	return fo_item_get_type(item);
}

__declspec(noinline) TGameObj* __stdcall GetItemPtrSlot(TGameObj* critter, InvenType slot) {
	TGameObj* itemPtr = nullptr;
	switch (slot) {
		case INVEN_TYPE_LEFT_HAND:
			itemPtr = fo_inven_left_hand(critter);
			break;
		case INVEN_TYPE_RIGHT_HAND:
			itemPtr = fo_inven_right_hand(critter);
			break;
		case INVEN_TYPE_WORN:
			itemPtr = fo_inven_worn(critter);
			break;
	}
	return itemPtr;
}

long& GetActiveItemMode() {
	return ptr_itemButtonItems[*ptr_itemCurrentItem].mode;
}

TGameObj* GetActiveItem() {
	return ptr_itemButtonItems[*ptr_itemCurrentItem].item;
}

bool HeroIsFemale() {
	return (fo_stat_level(*ptr_obj_dude, STAT_gender) == GENDER_FEMALE);
}

// Checks whether the player is under the influence of negative effects of radiation
long __fastcall IsRadInfluence() {
	QueueRadiation* queue = (QueueRadiation*)fo_queue_find_first(*ptr_obj_dude, radiation_event);
	while (queue) {
		if (queue->init && queue->level >= 2) return 1;
		queue = (QueueRadiation*)fo_queue_find_next(*ptr_obj_dude, radiation_event);
	}
	return 0;
}

// Returns the number of local variables of the object script
long GetScriptLocalVars(long sid) {
	TScript* script = nullptr;
	fo_scr_ptr(sid, &script);
	return (script) ? script->numLocalVars : 0;
}

// Returns window by x/y coordinate (hidden windows are ignored)
WINinfo* __fastcall GetTopWindowAtPos(long xPos, long yPos, bool bypassTrans) {
	long num = *ptr_num_windows - 1;
	if (num) {
		int cflags = WinFlags::Hidden;
		if (bypassTrans) cflags |= WinFlags::Transparent;
		do {
			WINinfo* win = ptr_window[num];
			if (xPos >= win->wRect.left && xPos <= win->wRect.right && yPos >= win->wRect.top && yPos <= win->wRect.bottom) {
				if (!(win->flags & cflags)) {
					return win;
				}
			}
		} while (--num);
	}
	return ptr_window[0];
}

static long GetRangeTileNumbers(long sourceTile, long radius, long &outEnd) {
	long hexRadius = 200 * (radius + 1);

	outEnd = sourceTile + hexRadius;
	if (outEnd > 40000) outEnd = 40000;

	long startTile = sourceTile - hexRadius;
	return (startTile < 0) ? 0 : startTile;
}

// Returns an array of objects within the specified radius from the source tile
void GetObjectsTileRadius(std::vector<TGameObj*> &objs, long sourceTile, long radius, long elev, long type) {
	long endTile;
	for (long tile = GetRangeTileNumbers(sourceTile, radius, endTile); tile < endTile; tile++) {
		TGameObj* obj = fo_obj_find_first_at_tile(elev, tile);
		while (obj) {
			if (type == -1 || type == obj->Type()) {
				bool multiHex = (obj->flags & ObjectFlag::MultiHex) ? true : false;
				if (fo_tile_dist(sourceTile, obj->tile) <= (radius + multiHex)) {
					objs.push_back(obj);
				}
			}
			obj = fo_obj_find_next_at_tile();
		}
	}
}

// Checks the blocking tiles and returns the first blocking object
TGameObj* CheckAroundBlockingTiles(TGameObj* source, long dstTile) {
	long rotation = 5;
	do {
		long chkTile = fo_tile_num_in_direction(dstTile, rotation, 1);
		TGameObj* obj = fo_obj_blocking_at(source, chkTile, source->elevation);
		if (obj) return obj;
	} while (--rotation >= 0);

	return nullptr;
}

TGameObj* __fastcall MultiHexMoveIsBlocking(TGameObj* source, long dstTile) {
	if (fo_tile_dist(source->tile, dstTile) > 1) {
		return CheckAroundBlockingTiles(source, dstTile);
	}
	// Checks the blocking arc of adjacent tiles
	long dir = fo_tile_dir(source->tile, dstTile);

	long chkTile = fo_tile_num_in_direction(dstTile, dir, 1);
	TGameObj* obj = fo_obj_blocking_at(source, chkTile, source->elevation);
	if (obj) return obj;

	// +1 direction
	long rotation = (dir + 1) % 6;
	chkTile = fo_tile_num_in_direction(dstTile, rotation, 1);
	obj = fo_obj_blocking_at(source, chkTile, source->elevation);
	if (obj) return obj;

	// -1 direction
	rotation = (dir + 5) % 6;
	chkTile = fo_tile_num_in_direction(dstTile, rotation, 1);
	obj = fo_obj_blocking_at(source, chkTile, source->elevation);
	if (obj) return obj;

	return nullptr;
}

// Returns the type of the terrain sub tile at the the player's position on the world map
long wmGetCurrentTerrainType() {
	long* terrainId = *(long**)FO_VAR_world_subtile;
	if (terrainId == nullptr) {
		__asm {
			lea  ebx, terrainId;
			mov  edx, dword ptr ds:[FO_VAR_world_ypos];
			mov  eax, dword ptr ds:[FO_VAR_world_xpos];
			call wmFindCurSubTileFromPos_;
		}
	}
	return *terrainId;
}

//---------------------------------------------------------
// copy the area from the interface buffer to the data array
void SurfaceCopyToMem(long fromX, long fromY, long width, long height, long fromWidth, BYTE* fromSurface, BYTE* toMem) {
	fromSurface += fromY * fromWidth + fromX;
	for (long i = 0, h = 0; h < height; h++, i += width) {
		std::memcpy(&toMem[i], fromSurface, width);
		fromSurface += fromWidth;
	}
}

// safe copy data from memory to the area of the interface buffer
void DrawToSurface(long toX, long toY, long width, long height, long toWidth, long toHeight, BYTE* toSurface, BYTE* fromMem) {
	BYTE* _toSurface = toSurface + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurface;
	long i = 0;
	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurface + w > endToSurf) return;
			if (_toSurface >= toSurface) _toSurface[w] = fromMem[i++];
		}
		_toSurface += toWidth;
	}
}

// safe copy data from surface to surface with mask
void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf,
                   long toX, long toY, long toWidth, long toHeight, BYTE* toSurf, int maskRef)
{
	BYTE* _fromSurf = fromSurf + (fromY * fromWidth + fromX);
	BYTE* _toSurf =  toSurf + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurf;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurf + w > endToSurf) return;
			if (_toSurf >= toSurf && _fromSurf[w] != maskRef) _toSurf[w] = _fromSurf[w];
		}
		_fromSurf += fromWidth;
		_toSurf += toWidth;
	}
}

// safe copy data from surface to surface
void DrawToSurface(long width, long height, long fromX, long fromY, long fromWidth, BYTE* fromSurf,
                   long toX, long toY, long toWidth, long toHeight, BYTE* toSurf)
{
	BYTE* _fromSurf = fromSurf + (fromY * fromWidth + fromX);
	BYTE* _toSurf = toSurf + (toY * toWidth + toX);
	BYTE* endToSurf = (toWidth * toHeight) + toSurf;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (_toSurf + w > endToSurf) return;
			if (_toSurf >= toSurf) _toSurf[w] = _fromSurf[w];
		}
		_fromSurf += fromWidth;
		_toSurf += toWidth;
	}
}

//void TranslucentDarkFill(BYTE* surface, long x, long y, long width, long height, long surfWidth) {
//	BYTE* surf = surface + (y * surfWidth) + x;
//	fo_wmInterfaceDrawSubTileRectFogged(surf, width, height, surfWidth);
//}

// Fills the specified interface window with index color
void WinFillRect(long winID, long x, long y, long width, long height, BYTE indexColor) {
	WINinfo* win = fo_GNW_find(winID);
	BYTE* surf = win->surface + (win->width * y) + x;
	long pitch = win->width - width;
	while (height--) {
		long w = width;
		while (w--) *surf++ = indexColor;
		surf += pitch;
	};
}

// Fills the specified interface window with index color 0 (black color)
void ClearWindow(long winID, bool refresh) {
	WINinfo* win = fo_GNW_find(winID);
	std::memset(win->surface, 0, win->width * win->height);
	if (refresh) {
		fo_GNW_win_refresh(win, &win->rect, nullptr);
	}
}

//---------------------------------------------------------
void PrintFloatText(TGameObj* object, const char* text, long colorText, long colorOutline, long font) {
	BoundRect rect;
	if (!fo_text_object_create(object, text, font, colorText, colorOutline, &rect)) {
		fo_tile_refresh_rect(&rect, object->elevation);
	}
}

// print text to surface
void __stdcall PrintText(char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = yPos * toWidth + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call dword ptr ds:[FO_VAR_text_to_buf];
	}
}

void __stdcall PrintTextFM(const char* displayText, BYTE colorIndex, DWORD xPos, DWORD yPos, DWORD txtWidth, DWORD toWidth, BYTE* toSurface) {
	DWORD posOffset = yPos * toWidth + xPos;
	__asm {
		xor  eax, eax;
		mov  al, colorIndex;
		mov  edx, displayText;
		push eax;
		mov  ebx, txtWidth;
		mov  eax, toSurface;
		mov  ecx, toWidth;
		add  eax, posOffset;
		call FMtext_to_buf_;
	}
}

//---------------------------------------------------------
//gets the height of the currently selected font
DWORD __stdcall GetTextHeight() {
//	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[FO_VAR_text_height]; //get text height
//		mov  TxtHeight, eax;
	}
//	return TxtHeight;
}

//---------------------------------------------------------
//gets the length of a string using the currently selected font
DWORD __stdcall GetTextWidth(const char* TextMsg) {
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[FO_VAR_text_width]; //get text width
	}
}

DWORD __stdcall GetTextWidthFM(const char* TextMsg) {
	return fo_FMtext_width(TextMsg); //get text width
}

//---------------------------------------------------------
//get width of Char for current font
DWORD __stdcall GetCharWidth(char charVal) {
	__asm {
		mov  al, charVal;
		call dword ptr ds:[FO_VAR_text_char_width];
	}
}

DWORD __stdcall GetCharWidthFM(char charVal) {
	__asm {
		mov  al, charVal;
		call FMtext_char_width_;
	}
}

//---------------------------------------------------------
//get maximum string length for current font - if all characters were maximum width
DWORD __stdcall GetMaxTextWidth(const char* TextMsg) {
//	DWORD msgWidth;
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[FO_VAR_text_mono_width];
//		mov  msgWidth, eax;
	}
//	return msgWidth;
}

//---------------------------------------------------------
//get number of pixels between characters for current font
DWORD __stdcall GetCharGapWidth() {
//	DWORD gapWidth;
	__asm {
		call dword ptr ds:[FO_VAR_text_spacing];
//		mov  gapWidth, eax;
	}
//	return gapWidth;
}

//---------------------------------------------------------
//get maximum character width for current font
DWORD __stdcall GetMaxCharWidth() {
//	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[FO_VAR_text_max];
//		mov  charWidth, eax;
	}
//	return charWidth;
}

void RedrawObject(TGameObj* obj) {
	BoundRect rect;
	fo_obj_bound(obj, &rect);
	fo_tile_refresh_rect(&rect, obj->elevation);
}

// Redraws all windows
void RefreshGNW(bool skipOwner) {
	*(DWORD*)FO_VAR_doing_refresh_all = 1;
	for (size_t i = 0; i < *ptr_num_windows; i++) {
		if (skipOwner && ptr_window[i]->flags & WinFlags::OwnerFlag) continue;
		fo_GNW_win_refresh(ptr_window[i], ptr_scr_size, 0);
	}
	*(DWORD*)FO_VAR_doing_refresh_all = 0;
}

/////////////////////////////////////////////////////////////////UNLISTED FRM FUNCTIONS//////////////////////////////////////////////////////////////

static bool LoadFrmHeader(UNLSTDfrm *frmHeader, DbFile* frmStream) {
	if (fo_db_freadInt(frmStream, &frmHeader->version) == -1)
		return false;
	else if (fo_db_freadShort(frmStream, &frmHeader->FPS) == -1)
		return false;
	else if (fo_db_freadShort(frmStream, &frmHeader->actionFrame) == -1)
		return false;
	else if (fo_db_freadShort(frmStream, &frmHeader->numFrames) == -1)
		return false;
	else if (fo_db_freadShortCount(frmStream, frmHeader->xCentreShift, 6) == -1)
		return false;
	else if (fo_db_freadShortCount(frmStream, frmHeader->yCentreShift, 6) == -1)
		return false;
	else if (fo_db_freadIntCount(frmStream, frmHeader->oriOffset, 6) == -1)
		return false;
	else if (fo_db_freadInt(frmStream, &frmHeader->frameAreaSize) == -1)
		return false;

	return true;
}

static bool LoadFrmFrame(UNLSTDfrm::Frame *frame, DbFile* frmStream) {
	//FRMframe *frameHeader = (FRMframe*)frameMEM;
	//BYTE* frameBuff = frame + sizeof(FRMframe);

	if (fo_db_freadShort(frmStream, &frame->width) == -1)
		return false;
	else if (fo_db_freadShort(frmStream, &frame->height) == -1)
		return false;
	else if (fo_db_freadInt(frmStream, &frame->size) == -1)
		return false;
	else if (fo_db_freadShort(frmStream, &frame->x) == -1)
		return false;
	else if (fo_db_freadShort(frmStream, &frame->y) == -1)
		return false;

	frame->indexBuff = new BYTE[frame->size];
	if (fo_db_fread(frame->indexBuff, frame->size, 1, frmStream) != 1)
		return false;

	return true;
}

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef) {
	if (folderRef > OBJ_TYPE_SKILLDEX) return nullptr;

	char *artfolder = ptr_art[folderRef].path; // address of art type name
	char FrmPath[MAX_PATH];

	sprintf_s(FrmPath, MAX_PATH, "art\\%s\\%s", artfolder, frmName);

	UNLSTDfrm *frm = new UNLSTDfrm;

	DbFile* frmStream = fo_xfopen(FrmPath, "rb");

	if (frmStream != nullptr) {
		if (!LoadFrmHeader(frm, frmStream)) {
			fo_db_fclose(frmStream);
			delete frm;
			return nullptr;
		}

		DWORD oriOffset_1st = frm->oriOffset[0];
		DWORD oriOffset_new = 0;
		frm->frames = new UNLSTDfrm::Frame[6 * frm->numFrames];
		for (int ori = 0; ori < 6; ori++) {
			if (ori == 0 || frm->oriOffset[ori] != oriOffset_1st) {
				frm->oriOffset[ori] = oriOffset_new;
				for (int fNum = 0; fNum < frm->numFrames; fNum++) {
					if (!LoadFrmFrame(&frm->frames[oriOffset_new + fNum], frmStream)) {
						fo_db_fclose(frmStream);
						delete frm;
						return nullptr;
					}
				}
				oriOffset_new += frm->numFrames;
			} else {
				frm->oriOffset[ori] = 0;
			}
		}

		fo_db_fclose(frmStream);
	} else {
		delete frm;
		return nullptr;
	}
	return frm;
}
