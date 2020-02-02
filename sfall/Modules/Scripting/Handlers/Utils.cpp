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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\..\FalloutEngine\EngineUtils.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Message.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "Utils.h"

namespace sfall
{
namespace script
{

// compares strings case-insensitive with specifics for Fallout
static bool FalloutStringCompare(const char* str1, const char* str2, long codePage) {
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

void sf_strlen(OpcodeContext& ctx) {
	ctx.setReturn(
		static_cast<int>(strlen(ctx.arg(0).strValue()))
	);
}

void sf_atoi(OpcodeContext& ctx) {
	auto str = ctx.arg(0).strValue();
	ctx.setReturn(
		static_cast<int>(strtol(str, (char**)nullptr, 0)) // auto-determine radix
	);
}

void sf_atof(OpcodeContext& ctx) {
	auto str = ctx.arg(0).strValue();
	ctx.setReturn(
		static_cast<float>(atof(str))
	);
}

void sf_ord(OpcodeContext& ctx) {
	unsigned char firstChar = ctx.arg(0).strValue()[0];
	ctx.setReturn(static_cast<unsigned long>(firstChar));
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

void sf_string_split(OpcodeContext& ctx) {
	ctx.setReturn(StringSplit(ctx.arg(0).strValue(), ctx.arg(1).strValue()));
}

static char* SubString(const char* str, int startPos, int length) {
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

	const int bufMax = ScriptExtender::TextBufferSize() - 1;
	if (length > bufMax) length = bufMax;

	memcpy(ScriptExtender::gTextBuffer, &str[startPos], length);
	ScriptExtender::gTextBuffer[length] = '\0';
	return ScriptExtender::gTextBuffer;
}

void sf_substr(OpcodeContext& ctx) {
	const char* str = ctx.arg(0).strValue();
	if (*str != '\0') str = SubString(str, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());
	ctx.setReturn(str);
}

void sf_string_compare(OpcodeContext& ctx) {
	if (ctx.numArgs() < 3) {
		ctx.setReturn(
			(_stricmp(ctx.arg(0).strValue(), ctx.arg(1).strValue()) ? 0 : 1)
		);
	} else {
		ctx.setReturn(FalloutStringCompare(ctx.arg(0).strValue(), ctx.arg(1).strValue(), ctx.arg(2).rawValue()));
	}
}

// A safer version of sprintf for using in user scripts.
static char* sprintf_lite(const char* format, ScriptValue value) {
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

	// calculate required length
	if (hasDigits) {
		buflen = 254;
	} else if (specifier == 'c') {
		buflen = j;
	} else if (specifier == 's') {
		buflen = j + strlen(value.strValue());
	} else {
		buflen = j + 30; // numbers
	}

	const long bufMaxLen = ScriptExtender::TextBufferSize() - 1;
	if (buflen > bufMaxLen - 1) buflen = bufMaxLen - 1;
	ScriptExtender::gTextBuffer[bufMaxLen] = '\0';

	if (value.isFloat()) {
		_snprintf(ScriptExtender::gTextBuffer, buflen, newfmt, value.floatValue());
	} else {
		_snprintf(ScriptExtender::gTextBuffer, buflen, newfmt, value.rawValue());
	}
	delete[] newfmt;
	return ScriptExtender::gTextBuffer;
}

void sf_sprintf(OpcodeContext& ctx) {
	ctx.setReturn(
		sprintf_lite(ctx.arg(0).strValue(), ctx.arg(1))
	);
}

void sf_string_format(OpcodeContext& ctx) {
	const char* format = ctx.arg(0).strValue();

	int fmtLen = strlen(format);
	if (fmtLen == 0) {
		ctx.setReturn(format);
		return;
	}
	if (fmtLen > 1024) {
		ctx.printOpcodeError("%s() - the format string exceeds maximum length of 1024 characters.", ctx.getMetaruleName());
		ctx.setReturn("Error");
	} else {
		char* newFmt = new char[fmtLen + 1];
		newFmt[fmtLen] = '\0';
		// parse format to make it safe
		int i = 0, arg = 0, totalArg = ctx.numArgs(); // total passed args
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
					else if ((cf == 's' && (arg > 0 && !ctx.arg(arg).isString())) || (cf != 's' && cf != 'd')) {
						newFmt[i++] = c;
						c = 'd'; // replace with %d
					}
				} else {
					newFmt[i++] = cf; // skip %%
				}
			}
			newFmt[i] = c;
		} while (++i < fmtLen);

		const long bufMaxLen = ScriptExtender::TextBufferSize() - 1;

		switch (totalArg) {
		case 2 :
			_snprintf(ScriptExtender::gTextBuffer, bufMaxLen, newFmt, ctx.arg(1).rawValue());
			break;
		case 3 :
			_snprintf(ScriptExtender::gTextBuffer, bufMaxLen, newFmt, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());
			break;
		case 4 :
			_snprintf(ScriptExtender::gTextBuffer, bufMaxLen, newFmt, ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), ctx.arg(3).rawValue());
			break;
		case 5 :
			_snprintf(ScriptExtender::gTextBuffer, bufMaxLen, newFmt, ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), ctx.arg(3).rawValue(), ctx.arg(4).rawValue());
		}
		ScriptExtender::gTextBuffer[bufMaxLen] = '\0'; // just in case

		delete[] newFmt;
		ctx.setReturn(ScriptExtender::gTextBuffer);
	}
}

void sf_message_str_game(OpcodeContext& ctx) {
	const char* msg = nullptr;

	int fileId = ctx.arg(0).rawValue();
	if (fileId >= 0) {
		int msgId = ctx.arg(1).rawValue();
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
	}
	if (msg == nullptr) msg = "Error";
	ctx.setReturn(msg);
}

void sf_add_extra_msg_file(OpcodeContext& ctx) {
	long result = Message::AddExtraMsgFile(ctx.arg(0).strValue(), (ctx.numArgs() == 2) ? ctx.arg(1).rawValue() : 0);
	switch (result) {
	case -1 :
		ctx.printOpcodeError("%s() - cannot add message file with the specified number.", ctx.getMetaruleName());
		break;
	case -2 :
		ctx.printOpcodeError("%s() - error loading message file.", ctx.getMetaruleName());
		break;
	case -3 :
		ctx.printOpcodeError("%s() - the limit of adding message files has been exceeded.", ctx.getMetaruleName());
	}
	ctx.setReturn(result);
}

void sf_get_string_pointer(OpcodeContext& ctx) {
	ctx.setReturn(reinterpret_cast<long>(ctx.arg(0).strValue()), DataType::INT);
}

void sf_get_text_width(OpcodeContext& ctx) {
	ctx.setReturn(fo::GetTextWidth(ctx.arg(0).strValue()));
}

static std::string strToCase;

void sf_string_to_case(OpcodeContext& ctx) {
	strToCase = ctx.arg(0).strValue();
	std::transform(strToCase.begin(), strToCase.end(), strToCase.begin(), ctx.arg(1).rawValue() ? ::toupper : ::tolower);

	ctx.setReturn(strToCase.c_str());
}

}
}
