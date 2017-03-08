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

#include <cmath>

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\ScriptExtender.h"
#include "..\..\FileSystem.h"
#include "..\..\Message.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"
#include "..\ScriptValue.h"

#include "Utils.h"

namespace sfall
{
namespace script
{

void sf_sqrt(OpcodeContext& ctx) {
	ctx.setReturn(sqrt(ctx.arg(0).asFloat()));
}

void sf_abs(OpcodeContext& ctx) {
	ctx.setReturn(abs(ctx.arg(0).asFloat()));
}

void sf_sin(OpcodeContext& ctx) {
	ctx.setReturn(sin(ctx.arg(0).asFloat()));
}

void sf_cos(OpcodeContext& ctx) {
	ctx.setReturn(cos(ctx.arg(0).asFloat()));
}

void sf_tan(OpcodeContext& ctx) {	
	ctx.setReturn(tan(ctx.arg(0).asFloat()));
}

void sf_arctan(OpcodeContext& ctx) {	
	ctx.setReturn(atan2(ctx.arg(0).asFloat(), ctx.arg(1).asFloat()));
}

void sf_strlen(OpcodeContext& ctx) {
	ctx.setReturn(
		static_cast<int>(strlen(ctx.arg(0).asString()))
	);
}

void sf_atoi(OpcodeContext& ctx) {
	auto str = ctx.arg(0).asString();
	ctx.setReturn(
		static_cast<int>(strtol(str, (char**)NULL, 0)) // auto-determine radix
	);
}

void sf_atof(OpcodeContext& ctx) {	
	auto str = ctx.arg(0).asString();
	ctx.setReturn(
		static_cast<float>(atof(str))
	);
}

void sf_ord(OpcodeContext& ctx) {
	char firstChar = ctx.arg(0).asString()[0];
	ctx.setReturn(static_cast<int>(firstChar));
}

void sf_typeof(OpcodeContext& ctx) {
	ctx.setReturn(static_cast<int>(ctx.arg(0).type()));
}

static int _stdcall StringSplit(const char* str, const char* split) {
	int id;
	if (strlen(split) == 0) {
		id = TempArray(strlen(str), 4);
		for (DWORD i = 0; i < strlen(str); i++) {
			arrays[id].val[i].set(&str[i], 1);
		}
	} else {
		int count = 1;
		const char *ptr = str, *newptr;
		while (true) {
			newptr = strstr(ptr, split);
			if (!newptr) break;
			count++;
			ptr = newptr + strlen(split);
		}
		id = TempArray(count, 0);
		ptr = str;
		count = 0;
		while (true) {
			newptr = strstr(ptr, split);
			int len = newptr ? newptr - ptr : strlen(ptr);
			arrays[id].val[count++].set(ptr, len);
			if (!newptr) break;
			ptr = newptr + strlen(split);
		}
	}
	return id;
}

void sf_string_split(OpcodeContext& ctx) {
	ctx.setReturn(StringSplit(ctx.arg(0).asString(), ctx.arg(1).asString()));
}

char* _stdcall Substring(const char* str, int pos, int length) {
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

void sf_substr(OpcodeContext& ctx) {
	ctx.setReturn(
		Substring(ctx.arg(0).asString(), ctx.arg(1).asInt(), ctx.arg(2).asInt())
	);
}

static char* sprintfbuf = NULL;
// A safer version of sprintf for using in user scripts.
static char* _stdcall sprintf_lite(const char* format, ScriptValue value) {
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
				if (c == 's' && !value.isString()) { // don't allow to treat non-string values as string pointers
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
		buflen = j + strlen(value.asString());
	} else {
		buflen = j + 30; // numbers
	}
	if (sprintfbuf) {
		delete[] sprintfbuf;
	}
	sprintfbuf = new char[buflen + 1];
	if (value.isFloat()) {
		_snprintf(sprintfbuf, buflen, newfmt, value.asFloat());
	} else {
		_snprintf(sprintfbuf, buflen, newfmt, value.rawValue());
	}
	sprintfbuf[buflen] = '\0'; // just in case
	delete[] newfmt;
	return sprintfbuf;
}

void sf_sprintf(OpcodeContext& ctx) {
	ctx.setReturn(
		sprintf_lite(ctx.arg(0).asString(), ctx.arg(1))
	);
}

// fix for vanilla negate operator not working on floats
static const DWORD NegateFixHook_Back = 0x46AB79;
void __declspec(naked) NegateFixHook() {
	__asm {
		mov     ebx, edi;
		lea     edx, [ecx + 36];
		mov     eax, [ecx + 28];
		cmp     si, VAR_TYPE_FLOAT;
		jne     notfloat;
		push    ebx;
		fld[esp];
		fchs;
		fstp[esp];
		pop     ebx;
		call    FuncOffs::pushLongStack_;
		mov     edx, VAR_TYPE_FLOAT;
		jmp     end;
notfloat:
		neg     ebx
		call    FuncOffs::pushLongStack_;
		mov     edx, VAR_TYPE_INT;
end:
		mov     eax, ecx;
		jmp     NegateFixHook_Back;
	}
}

void sf_power(OpcodeContext& ctx) {
	const ScriptValue &base = ctx.arg(0),
					  &power = ctx.arg(1);
	float result = 0.0;
	if (power.isFloat())
		result = pow(base.asFloat(), power.asFloat());
	else
		result = pow(base.asFloat(), power.asInt());

	if (base.isInt() && power.isInt()) {
		ctx.setReturn(static_cast<int>(result));
	} else {
		ctx.setReturn(result);
	}
}

void sf_log(OpcodeContext& ctx) {
	ctx.setReturn(log(ctx.arg(0).asFloat()));
}

void sf_exponent(OpcodeContext& ctx) {
	ctx.setReturn(exp(ctx.arg(0).asFloat()));
}

void sf_ceil(OpcodeContext& ctx) {
	ctx.setReturn(static_cast<int>(ceil(ctx.arg(0).asFloat())));
}

void sf_round(OpcodeContext& ctx) {
	float arg = ctx.arg(0).asFloat();
	int argI = static_cast<int>(arg);
	float mod = arg - static_cast<float>(argI);
	if (abs(mod) >= 0.5) {
		argI += (mod > 0 ? 1 : -1);
	}
	ctx.setReturn(argI);
}

// TODO: move to FalloutEngine module
#define CASTMSG(adr) reinterpret_cast<fo::MessageList*>(adr)
static const fo::MessageList* gameMsgFiles[] =
{ CASTMSG(0x56D368)     // COMBAT
, CASTMSG(0x56D510)     // AI
, CASTMSG(0x56D754)     // SCRNAME
, CASTMSG(0x58E940)     // MISC
, CASTMSG(0x58EA98)     // CUSTOM
, CASTMSG(0x59E814)     // INVENTRY
, CASTMSG(0x59E980)     // ITEM
, CASTMSG(0x613D28)     // LSGAME
, CASTMSG(0x631D48)     // MAP
, CASTMSG(0x6637E8)     // OPTIONS
, CASTMSG(0x6642D4)     // PERK
, CASTMSG(0x664348)     // PIPBOY
, CASTMSG(0x664410)     // QUESTS
, CASTMSG(0x6647FC)     // PROTO
, CASTMSG(0x667724)     // SCRIPT
, CASTMSG(0x668080)     // SKILL
, CASTMSG(0x6680F8)     // SKILLDEX
, CASTMSG(0x66817C)     // STAT
, CASTMSG(0x66BE38)     // TRAIT
, CASTMSG(0x672FB0) };  // WORLDMAP
#undef CASTMSG

void sf_message_str_game(OpcodeContext& ctx) {
	const char* msg = nullptr;
	const ScriptValue &fileIdArg = ctx.arg(0),
		&msgIdArg = ctx.arg(1);

	int fileId = fileIdArg.asInt();
	int msgId = msgIdArg.asInt();
	if (fileId < 20) { // main msg files
		msg = fo::GetMessageStr(gameMsgFiles[fileId], msgId);
	} else if (fileId >= 0x1000 && fileId <= 0x1005) { // proto msg files
		msg = fo::GetMessageStr(&fo::var::proto_msg_files[fileId - 0x1000], msgId);
	} else if (fileId >= 0x2000) { // Extra game message files.
		ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.find(fileId);

		if (it != gExtraGameMsgLists.end()) {
			msg = GetMsg(it->second.get(), msgId, 2);
		}
	}

	if (msg == nullptr) {
		msg = "Error";
	}
	ctx.setReturn(msg);
}

}
}
