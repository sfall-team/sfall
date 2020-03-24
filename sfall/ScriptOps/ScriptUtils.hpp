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

#include "main.h"

#include "ScriptExtender.h"
#include "Arrays.h"
#include "Message.h"

// compares strings case-insensitive with specifics for Fallout
static bool _stdcall FalloutStringCompare(const char* str1, const char* str2, long codePage) {
	while (true) {
		unsigned char c1 = *str1;
		unsigned char c2 = *str2;

		if (c1 == 0 && c2 == 0) return true;  // end - strings are equal
		if (c1 == 0 || c2 == 0) return false; // strings are not equal
		str1++;
		str2++;
		if (c1 == c2) continue;

		if (codePage == 866) {
			// replace Russian 'x' to English (Fallout specific)
			if (c1 == 229) c1 -= 229 - 'x';
			if (c2 == 229) c2 -= 229 - 'x';
		}

		// 0 - 127 (standard ASCII)
		// upper to lower case
		if (c1 >= 'A' && c1 <= 'Z') c1 |= 32;
		if (c2 >= 'A' && c2 <= 'Z') c2 |= 32;
		if (c1 == c2) continue;
		if (c1 < 128 || c2 < 128) return false;

		// 128 - 255 (international/extended)
		switch (codePage) {
		case 866:
			if (c1 != 149 && c2 != 149) { // code used for the 'bullet' character in Fallout font (the Russian letter 'X' uses Latin letter)
				// upper to lower case
				if (c1 >= 128 && c1 <= 159) {
					c1 |= 32;
				} else if (c1 >= 224 && c1 <= 239) {
					c1 -= 48; // shift lower range
				} else if (c1 == 240) {
					c1++;
				}
				if (c2 >= 128 && c2 <= 159) {
					c2 |= 32;
				} else if (c2 >= 224 && c2 <= 239) {
					c2 -= 48; // shift lower range
				} else if (c2 == 240) {
					c2++;
				}
			}
			break;
		case 1251:
			// upper to lower case
			if (c1 >= 0xC0 && c1 <= 0xDF) c1 |= 32;
			if (c2 >= 0xC0 && c2 <= 0xDF) c2 |= 32;
			if (c1 == 0xA8) c1 += 16;
			if (c2 == 0xA8) c2 += 16;
			break;
		case 1250:
		case 1252:
			if (c1 != 0xD7 && c1 != 0xF7 && c2 != 0xD7 && c2 != 0xF7) {
				if (c1 >= 0xC0 && c1 <= 0xDE) c1 |= 32;
				if (c2 >= 0xC0 && c2 <= 0xDE) c2 |= 32;
			}
			break;
		}
		if (c1 != c2) return false; // strings are not equal
	}
}

static void _stdcall op_strlen2() {
	const ScriptValue &strArg = opHandler.arg(0);

	if (strArg.isString()) {
		opHandler.setReturn(
			static_cast<int>(strlen(strArg.strValue()))
		);
	} else {
		OpcodeInvalidArgs("strlen");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_strlen() {
	_WRAP_OPCODE(op_strlen2, 1, 1)
}

static void _stdcall str_to_int2() {
	const ScriptValue &strArg = opHandler.arg(0);

	if (strArg.isString()) {
		const char* str = strArg.strValue();
		opHandler.setReturn(
			static_cast<int>(strtol(str, (char**)nullptr, 0)) // auto-determine radix
		);
	} else {
		OpcodeInvalidArgs("atoi");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) str_to_int() {
	_WRAP_OPCODE(str_to_int2, 1, 1)
}

static void _stdcall str_to_flt2() {
	const ScriptValue &strArg = opHandler.arg(0);

	if (strArg.isString()) {
		const char* str = strArg.strValue();
		opHandler.setReturn(
			static_cast<float>(atof(str))
		);
	} else {
		OpcodeInvalidArgs("atof");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) str_to_flt() {
	_WRAP_OPCODE(str_to_flt2, 1, 1)
}

static void _stdcall op_ord2() {
	const ScriptValue &strArg = opHandler.arg(0);

	if (strArg.isString()) {
		unsigned char firstChar = strArg.strValue()[0];
		opHandler.setReturn(static_cast<unsigned long>(firstChar));
	} else {
		OpcodeInvalidArgs("charcode");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_ord() {
	_WRAP_OPCODE(op_ord2, 1, 1)
}

static void _stdcall op_typeof2() {
	opHandler.setReturn(static_cast<int>(opHandler.arg(0).type()));
}

static void __declspec(naked) op_typeof() {
	_WRAP_OPCODE(op_typeof2, 1, 1)
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

static void _stdcall string_split2() {
	const ScriptValue &strArg = opHandler.arg(0),
					  &splitArg = opHandler.arg(1);

	if (strArg.isString() && splitArg.isString()) {
		opHandler.setReturn(StringSplit(strArg.strValue(), splitArg.strValue()));
	} else {
		OpcodeInvalidArgs("string_split");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) string_split() {
	_WRAP_OPCODE(string_split2, 2, 1)
}

static char* _stdcall SubString(const char* str, int startPos, int length) {
	int len = strlen(str);

	if (startPos < 0) {
		startPos += len; // start from end
		if (startPos < 0) startPos = 0;
	}
	if (length < 0) {
		length += len - startPos; // cutoff at end
		if (length == 0) return "";
		length = abs(length); // length can't be negative
	}
	// check position
	if (startPos >= len) return ""; // start position is out of string length, return empty string
	if (length == 0 || length + startPos > len) {
		length = len - startPos; // set the correct length, the length of characters goes beyond the end of the string
	}

	const int bufMax = GlblTextBufferSize() - 1;
	if (length > bufMax) length = bufMax;

	memcpy(gTextBuffer, &str[startPos], length);
	gTextBuffer[length] = '\0';
	return gTextBuffer;
}

static void _stdcall op_substr2() {
	const ScriptValue &strArg = opHandler.arg(0),
					  &startArg = opHandler.arg(1),
					  &lenArg = opHandler.arg(2);

	if (strArg.isString() && startArg.isInt() && lenArg.isInt()) {
		const char* str = strArg.strValue();
		if (*str != '\0') str = SubString(str, startArg.rawValue(), lenArg.rawValue());
		opHandler.setReturn(str);
	} else {
		OpcodeInvalidArgs("substr");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_substr() {
	_WRAP_OPCODE(op_substr2, 3, 1)
}

static void sf_string_compare() {
	const ScriptValue &str1Arg = opHandler.arg(0),
					  &str2Arg = opHandler.arg(1);

	if (str1Arg.isString() && str2Arg.isString()) {
		if (opHandler.numArgs() < 3) {
			opHandler.setReturn(
				(_stricmp(str1Arg.strValue(), str2Arg.strValue()) ? 0 : 1)
			);
		} else {
			const ScriptValue &codePageArg = opHandler.arg(2);
			if (!codePageArg.isInt()) goto invalidArgs;
			opHandler.setReturn(FalloutStringCompare(str1Arg.strValue(), str2Arg.strValue(), codePageArg.rawValue()));
		}
	} else {
invalidArgs:
		OpcodeInvalidArgs("string_compare");
		opHandler.setReturn(0);
	}
}

// A safer version of sprintf for using in user scripts.
static char* _stdcall mysprintf(const char* format, DWORD value, DWORD valueType) {
	valueType = valueType & 0xFFFF; // use lower 2 bytes
	int fmtlen = strlen(format);
	int buflen = fmtlen + 1;
	for (int i = 0; i < fmtlen; i++) {
		if (format[i] == '%') buflen++; // will possibly be escaped, need space for that
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

	// calculate required length
	if (hasDigits) {
		buflen = 254;
	} else if (specifier == 'c') {
		buflen = j;
	} else if (specifier == 's') {
		buflen = j + strlen((char*)value);
	} else {
		buflen = j + 30; // numbers
	}

	const long bufMaxLen = GlblTextBufferSize() - 1;
	if (buflen > bufMaxLen - 1) buflen = bufMaxLen - 1;
	gTextBuffer[bufMaxLen] = '\0';

	if (valueType == VAR_TYPE_FLOAT) {
		_snprintf(gTextBuffer, buflen, newfmt, *(float*)(&value));
	} else {
		_snprintf(gTextBuffer, buflen, newfmt, value);
	}
	delete[] newfmt;
	return gTextBuffer;
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

static void sf_string_format() {
	const ScriptValue &fmtArg = opHandler.arg(0);

	if (!fmtArg.isString()) {
		OpcodeInvalidArgs("string_format");
		opHandler.setReturn(0);
		return;
	}

	const char* format = fmtArg.strValue();

	int fmtLen = strlen(format);
	if (fmtLen == 0) {
		opHandler.setReturn(format);
		return;
	}
	if (fmtLen > 1024) {
		opHandler.printOpcodeError("string_format() - the format string exceeds maximum length of 1024 characters.");
		opHandler.setReturn("Error");
	} else {
		char* newFmt = new char[fmtLen + 1];
		newFmt[fmtLen] = '\0';
		// parse format to make it safe
		int i = 0, arg = 0, totalArg = opHandler.numArgs(); // total passed args
		do {
			char c = format[i];
			if (c == '%') {
				char cf = format[i + 1];
				if (cf != '%') {
					if (arg >= 0) {
						arg++;
						if (arg == totalArg) arg = -1; // format '%' prefixes in the format string exceed the number of passed value args
					}
					if (arg < 0) { // have % more than passed value args
						c = '^';   // delete %
					}
					// check string is valid or replace unsupported format
					else if ((cf == 's' && (arg > 0 && !opHandler.arg(arg).isString())) || (cf != 's' && cf != 'd')) {
						newFmt[i++] = c;
						c = 'd'; // replace with %d
					}
				} else {
					newFmt[i++] = cf; // skip %%
				}
			}
			newFmt[i] = c;
		} while (++i < fmtLen);

		const long bufMaxLen = GlblTextBufferSize() - 1;

		switch (totalArg) {
		case 2 :
			_snprintf(gTextBuffer, bufMaxLen, newFmt, opHandler.arg(1).rawValue());
			break;
		case 3 :
			_snprintf(gTextBuffer, bufMaxLen, newFmt, opHandler.arg(1).rawValue(), opHandler.arg(2).rawValue());
			break;
		case 4 :
			_snprintf(gTextBuffer, bufMaxLen, newFmt, opHandler.arg(1).rawValue(), opHandler.arg(2).rawValue(), opHandler.arg(3).rawValue());
			break;
		case 5 :
			_snprintf(gTextBuffer, bufMaxLen, newFmt, opHandler.arg(1).rawValue(), opHandler.arg(2).rawValue(), opHandler.arg(3).rawValue(), opHandler.arg(4).rawValue());
		}
		gTextBuffer[bufMaxLen] = '\0'; // just in case

		delete[] newFmt;
		opHandler.setReturn(gTextBuffer);
	}
}

static void _stdcall op_message_str_game2() {
	const char* msg = nullptr;
	const ScriptValue &fileIdArg = opHandler.arg(0),
					  &msgIdArg = opHandler.arg(1);

	if (fileIdArg.isInt() && msgIdArg.isInt()) {
		int fileId = fileIdArg.rawValue();
		if (fileId >= 0) {
			int msgId = msgIdArg.rawValue();
			if (fileId < 20) { // main msg files
				msg = GetMessageStr(gameMsgFiles[fileId], msgId);
			} else if (fileId >= 0x1000 && fileId <= 0x1005) { // proto msg files
				msg = GetMessageStr((MSGList*)&ptr_proto_msg_files[2 * (fileId - 0x1000)], msgId);
			} else if (fileId >= 0x2000) { // Extra game message files.
				ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.find(fileId);
				if (it != gExtraGameMsgLists.end()) {
					msg = GetMsg(it->second, msgId, 2);
				}
			}
		}
		if (msg == nullptr) msg = "Error";
		opHandler.setReturn(msg);
	} else {
		OpcodeInvalidArgs("message_str_game");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_message_str_game() {
	_WRAP_OPCODE(op_message_str_game2, 2, 1)
}

static void sf_get_text_width() {
	const ScriptValue &textArg = opHandler.arg(0);

	if (textArg.isString()) {
		opHandler.setReturn(GetTextWidth(textArg.strValue()));
	} else {
		OpcodeInvalidArgs("get_text_width");
		opHandler.setReturn(0);
	}
}

static std::string strToCase;

static void sf_string_to_case() {
	strToCase = opHandler.arg(0).strValue();
	std::transform(strToCase.begin(), strToCase.end(), strToCase.begin(), opHandler.arg(1).rawValue() ? ::toupper : ::tolower);

	opHandler.setReturn(strToCase.c_str());
}
