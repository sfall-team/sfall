/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2011, 2012  The sfall team
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

#include <cmath>

#include "main.h"

#include "ScriptExtender.h"
#include "ScriptArrays.hpp"
#include "FileSystem.h"
#include "Arrays.h"
#include "Message.h"

static void __declspec(naked) funcSqrt() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fsqrt;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void funcAbs2() {
	const ScriptValue &value = opHandler.arg(0);
	if (value.isInt()) {
		opHandler.setReturn(abs((int)value.rawValue()));
	} else {
		opHandler.setReturn(abs(value.asFloat()));
	}
}

static void __declspec(naked) funcAbs() {
	_WRAP_OPCODE(funcAbs2, 1, 1)
}

static void __declspec(naked) funcSin() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fsin;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}
static void __declspec(naked) funcCos() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fcos;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}
static void __declspec(naked) funcTan() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp calc;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
calc:
		fptan;
		fstp [esp];
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}
static void __declspec(naked) funcATan() {
	__asm {
		pushad;
		sub esp, 4;
		mov ecx, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_INT;
		jnz arg1l2;
		mov [esp], eax;
		fild [esp];
		jmp arg2l1;
arg1l2:
		cmp bx, VAR_TYPE_FLOAT;
		jnz fail;
		mov [esp], eax;
		fld [esp];
arg2l1:
		cmp dx, VAR_TYPE_INT;
		jnz arg2l2;
		mov [esp], edi;
		fild [esp];
		jmp calc;
arg2l2:
		cmp dx, VAR_TYPE_FLOAT;
		jnz fail2;
		mov [esp], edi;
		fld [esp];
calc:
		fpatan;
		fstp [esp];
		mov edx, [esp];
		jmp end;
fail2:
		fstp [esp];
fail:
		fldz;
		fstp [esp];
		mov edx, [esp];
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ecx;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static int _stdcall StringSplit(const char* str, const char* split) {
	int id;
	size_t count, splitLen = strlen(split);
	if (!splitLen) {
		count = strlen(str);
		id = TempArray(count, 0);
		for (DWORD i = 0; i < count; i++) {
			arrays[id].val[i].set(&str[i], 1);
		}
	} else {
		count = 1;
		const char *ptr = str, *newptr;
		while (true) {
			newptr = strstr(ptr, split);
			if (!newptr) break;
			count++;
			ptr = newptr + splitLen;
		}
		id = TempArray(count, 0);
		ptr = str;
		count = 0;
		while (true) {
			newptr = strstr(ptr, split);
			int len = (newptr) ? newptr - ptr : strlen(ptr);
			arrays[id].val[count++].set(ptr, len);
			if (!newptr) break;
			ptr = newptr + splitLen;
		}
	}
	return id;
}
static void __declspec(naked) string_split() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ebp;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopShort_;
		mov ecx, eax;
		mov eax, ebp;
		call interpretPopLong_;
		mov esi, eax;
		cmp bx, VAR_TYPE_STR2;
		jz str1;
		cmp bx, VAR_TYPE_STR;
		jnz fail;
str1:
		cmp cx, VAR_TYPE_STR2;
		jz str2;
		cmp cx, VAR_TYPE_STR;
		jnz fail;
str2:
		mov eax, ebp;
		mov edx, ebx;
		mov ebx, edi;
		call interpretGetString_;
		mov edi, eax;
		mov eax, ebp;
		mov edx, ecx;
		mov ebx, esi;
		call interpretGetString_;
		push edi;
		push eax;
		call StringSplit;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ebp;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static int _stdcall str_to_int_internal(const char* str) {
	return static_cast<int>(strtol(str, (char**)nullptr, 0)); // auto-determine radix
}
static DWORD _stdcall str_to_flt_internal(const char* str) {
	float f = static_cast<float>(atof(str));
	return *(DWORD*)&f;
}
static void __declspec(naked) str_to_int() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_STR2;
		jz str1;
		cmp bx, VAR_TYPE_STR;
		jnz fail;
str1:
		mov edx, ebx;
		mov ebx, eax;
		mov eax, ebp;
		call interpretGetString_;
		push eax;
		call str_to_int_internal;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ebp;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) str_to_flt() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp bx, VAR_TYPE_STR2;
		jz str1;
		cmp bx, VAR_TYPE_STR;
		jnz fail;
str1:
		mov edx, ebx;
		mov ebx, eax;
		mov eax, ebp;
		call interpretGetString_;
		push eax;
		call str_to_flt_internal;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov edx, VAR_TYPE_FLOAT;
		mov eax, ebp;
		call interpretPushShort_;
		popad;
		retn;
	}
}
char* _stdcall mysubstr(char* str, int pos, int length) {
	char* newstr;
	int srclen;
	srclen = strlen(str);
	if (pos < 0)
		pos = srclen + pos;
	if (length < 0)
		length = srclen - pos + length;
	if (pos >= srclen)
		length = 0;
	else if (length + pos > srclen)
		length = srclen - pos;
	newstr = new char[length + 1];
	if (length > 0)
		memcpy(newstr, &str[pos], length);
	newstr[length] = '\0';
	return newstr;
}

static DWORD _stdcall mystrlen(char* str) {
	return strlen(str);
}
static char* sprintfbuf = nullptr;
static char* _stdcall mysprintf(char* format, DWORD value, DWORD valueType) {
	valueType = valueType & 0xFFFF; // use lower 2 bytes
	int fmtlen = strlen(format);
	int buflen = fmtlen + 1;
	for (int i = 0; i < fmtlen; i++) {
		if (format[i] == '%')
			buflen++; // will possibly be escaped, need space for that
	}
	// parse format to make it safe
	char* newfmt = new char[buflen];
	byte mode = 0;
	int j = 0;
	char c, specifier;
	bool hasDigits = false;
	for (int i = 0; i < fmtlen; i++) {
		c = format[i];
		switch (mode) {
		case 0: // prefix
			if (c == '%') {
				mode = 1;
			}
			break;
		case 1: // definition
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				if (c == 'h' || c == 'l' || c == 'j' || c == 'z' || c == 't' || c == 'L') { // ignore sub-specifiers
					continue;
				}
				if (c == 's' && valueType != VAR_TYPE_STR2 && valueType != VAR_TYPE_STR) { // don't allow to treat non-string values as string pointers
					c = 'd';
				} else if (c == 'n') { // don't allow "n" specifier
					c = 'd';
				}
				specifier = c;
				mode = 2;
			} else if (c == '%') {
				mode = 0;
				hasDigits = false;
			} else if (c >= '0' && c <= '9') {
				hasDigits = true;
			}
			break;
		case 2: // postfix
		default:
			if (c == '%') { // don't allow more than one specifier
				newfmt[j++] = '%'; // escape it
				if (format[i + 1] == '%') {
					i++; // skip already escaped
				}
			}
			break;
		}
		newfmt[j++] = c;
	}
	newfmt[j] = '\0';
	// calculate required memory
	if (hasDigits) {
		buflen = 254;
	} else if (specifier == 'c') {
		buflen = j;
	} else if (specifier == 's') {
		buflen = j + strlen((char*)value);
	} else {
		buflen = j + 30; // numbers
	}
	if (sprintfbuf) {
		delete[] sprintfbuf;
	}
	sprintfbuf = new char[buflen + 1];
	if (valueType == VAR_TYPE_FLOAT) {
		_snprintf(sprintfbuf, buflen, newfmt, *(float*)(&value));
	} else {
		_snprintf(sprintfbuf, buflen, newfmt, value);
	}
	sprintfbuf[buflen] = '\0'; // just in case
	delete[] newfmt;
	return sprintfbuf;
}

static void __declspec(naked) op_substr() {
	__asm {
		pushad;
		mov edi, eax;
		call interpretPopShort_;
		push eax;
		mov eax, edi;
		call interpretPopLong_; // length
		push eax;
		mov eax, edi;
		call interpretPopShort_;
		push eax;
		mov eax, edi;
		call interpretPopLong_; // position
		push eax;
		mov eax, edi;
		call interpretPopShort_;
		push eax;
		mov eax, edi;
		call interpretPopLong_; // string
		push eax;

		movzx eax, word ptr [esp+12];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr [esp+20];
		cmp eax, VAR_TYPE_INT;
		jne fail;
		movzx eax, word ptr [esp+4];
		cmp eax, VAR_TYPE_STR2;
		je next1;
		cmp eax, VAR_TYPE_STR;
		jne fail;
next1:
		mov eax, edi;
		mov edx, [esp+4];
		mov ebx, [esp];
		call interpretGetString_;
		mov ebx, [esp+16];
		mov edx, [esp+8];
		push ebx;
		push edx;
		push eax;
		call mysubstr;
		mov edx, eax;
		mov eax, edi;
		call interpretAddString_;
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_STR;
		mov eax, edi;
		call interpretPushShort_;
		jmp end;
fail:
		xor edx, edx;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
end:
		add esp, 24;
		popad;
		retn;
	}
}

static void __declspec(naked) op_strlen() {
	__asm {
		pushad;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_; // string
		cmp dx, VAR_TYPE_STR2;
		je next;
		cmp dx, VAR_TYPE_STR;
		jne fail;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		call mystrlen;
		jmp end;
fail:
		xor eax, eax; // return 0
end:
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_sprintf() {
	__asm {
		pushad;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_; // any value
		mov ebx, eax;
		mov eax, edi;

		call interpretPopShort_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopLong_; // format string
		mov esi, eax;
		// check types
		cmp cx, VAR_TYPE_STR2;
		je nextarg;
		cmp cx, VAR_TYPE_STR;
		jne fail;
nextarg:
		cmp dx, VAR_TYPE_STR2;
		je next2;
		cmp dx, VAR_TYPE_STR;
		jne notstring;
next2:
		mov eax, edi;
		call interpretGetString_; // value string ptr
		mov ebx, eax;
notstring:
		push edx; // arg 3 - valueType
		mov eax, esi;
		mov esi, ebx;
		mov edx, ecx;
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_; // format string ptr
		push esi; // arg 2 - value
		push eax; // arg 1 - format str
		call mysprintf;
		mov edx, eax;
		mov eax, edi;
		call interpretAddString_;
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_STR;
		mov eax, edi;
		call interpretPushShort_;
fail:
		popad;
		retn;
	}
}

static void __declspec(naked) op_ord() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, esi)
	_PARSE_STR_ARG(1, ebp, bx, esi, notstring)
	__asm {
		mov eax, 0;
		mov al, [esi]; // first character
		jmp done;
notstring:
		mov eax, 0;
	done:
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static DWORD _stdcall GetValueType(DWORD datatype) {
	datatype &= 0xFFFF;
	switch (datatype) {
	case VAR_TYPE_STR:
	case VAR_TYPE_STR2:
		return DATATYPE_STR;
	case VAR_TYPE_INT:
		return DATATYPE_INT;
	case VAR_TYPE_FLOAT:
		return DATATYPE_FLOAT;
	default:
		return DATATYPE_NONE; // just in case
	}
}

static void __declspec(naked) op_typeof() {
	__asm {
		pushad;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_; // call just in case (not used)
		push edx;
		call GetValueType;
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		popad;
		retn;
	}
}

static void funcPow2() {
	const ScriptValue &base = opHandler.arg(0),
					  &power = opHandler.arg(1);
	float result = 0.0;
	if (!base.isString() && !power.isString()) {
		if (power.isFloat())
			result = pow(base.asFloat(), power.floatValue());
		else
			result = pow(base.asFloat(), power.asInt());

		if (base.isInt() && power.isInt()) {
			opHandler.setReturn(static_cast<int>(result));
		} else {
			opHandler.setReturn(result);
		}
	} else {
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) funcPow() {
	_WRAP_OPCODE(funcPow2, 2, 1)
}

static void funcLog2() {
	opHandler.setReturn(log(opHandler.arg(0).asFloat()));
}

static void __declspec(naked) funcLog() {
	_WRAP_OPCODE(funcLog2, 1, 1)
}

static void funcExp2() {
	opHandler.setReturn(exp(opHandler.arg(0).asFloat()));
}

static void __declspec(naked) funcExp() {
	_WRAP_OPCODE(funcExp2, 1, 1)
}

static void funcCeil2() {
	opHandler.setReturn(static_cast<int>(ceil(opHandler.arg(0).asFloat())));
}

static void __declspec(naked) funcCeil() {
	_WRAP_OPCODE(funcCeil2, 1, 1)
}

static void funcRound2() {
	float arg = opHandler.arg(0).asFloat();
	int argI = static_cast<int>(arg);
	float mod = arg - static_cast<float>(argI);
	if (abs(mod) >= 0.5) {
		argI += (mod > 0 ? 1 : -1);
	}
	opHandler.setReturn(argI);
}

static void __declspec(naked) funcRound() {
	_WRAP_OPCODE(funcRound2, 1, 1)
}

/*

*/

// TODO: move to FalloutEngine module
static const DWORD game_msg_files[] = {
	0x56D368, // COMBAT
	0x56D510, // AI
	0x56D754, // SCRNAME
	0x58E940, // MISC
	0x58EA98, // CUSTOM
	0x59E814, // INVENTRY
	0x59E980, // ITEM
	0x613D28, // LSGAME
	0x631D48, // MAP
	0x6637E8, // OPTIONS
	0x6642D4, // PERK
	0x664348, // PIPBOY
	0x664410, // QUESTS
	0x6647FC, // PROTO
	0x667724, // SCRIPT
	0x668080, // SKILL
	0x6680F8, // SKILLDEX
	0x66817C, // STAT
	0x66BE38, // TRAIT
	0x672FB0, // WORLDMAP
};

// TODO: move to FalloutEngine
static const DWORD* proto_msg_files = (DWORD*)0x006647AC;

static void _stdcall op_message_str_game2() {
	const char* msg = 0;
	const ScriptValue &fileIdArg = opHandler.arg(0),
					  &msgIdArg = opHandler.arg(1);
	if (fileIdArg.isInt() && msgIdArg.isInt()) {
		int fileId = fileIdArg.asInt();
		int msgId = msgIdArg.asInt();
		if (fileId < 20) { // main msg files
			msg = GetMessageStr(game_msg_files[fileId], msgId);
		} else if (fileId >= 0x1000 && fileId <= 0x1005) { // proto msg files
			msg = GetMessageStr((DWORD)&proto_msg_files[2*(fileId - 0x1000)], msgId);
		} else if (fileId >= 0x2000) { // Extra game message files.
			ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.find(fileId);
			if (it != gExtraGameMsgLists.end()) {
				msg = GetMsg(it->second, msgId, 2);
			}
		}
	}
	if (msg == 0) {
		msg = "Error";
	}
	opHandler.setReturn(msg);
}

static void __declspec(naked) op_message_str_game() {
	_WRAP_OPCODE(op_message_str_game2, 2, 1)
}

static void sf_floor2() {
	opHandler.setReturn(static_cast<int>(floor(opHandler.arg(0).asFloat())));
}
