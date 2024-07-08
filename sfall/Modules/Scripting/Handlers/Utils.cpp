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

#include "Utils.h"

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\..\Utils.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Message.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

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
			// replace Russian 'x' with English (Fallout specific)
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

void op_strlen(OpcodeContext& ctx) {
	ctx.setReturn(
		static_cast<int>(strlen(ctx.arg(0).strValue()))
	);
}

void op_atoi(OpcodeContext& ctx) {
	auto str = ctx.arg(0).strValue();
	ctx.setReturn(
		static_cast<int>(StrToLong(str, 0))
	);
}

void op_atof(OpcodeContext& ctx) {
	auto str = ctx.arg(0).strValue();
	ctx.setReturn(
		static_cast<float>(atof(str))
	);
}

void op_ord(OpcodeContext& ctx) {
	unsigned char firstChar = ctx.arg(0).strValue()[0];
	ctx.setReturn(static_cast<unsigned long>(firstChar));
}

static int __stdcall StringSplit(const char* str, const char* split) {
	int id;
	size_t count, splitLen = strlen(split);
	if (!splitLen) {
		count = strlen(str);
		id = CreateTempArray(count, 0);
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
		id = CreateTempArray(count, 0);
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

void op_string_split(OpcodeContext& ctx) {
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

void op_substr(OpcodeContext& ctx) {
	const char* str = ctx.arg(0).strValue();
	if (*str != '\0') str = SubString(str, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());
	ctx.setReturn(str);
}

void mf_string_compare(OpcodeContext& ctx) {
	if (ctx.numArgs() < 3) {
		ctx.setReturn(
			(_stricmp(ctx.arg(0).strValue(), ctx.arg(1).strValue()) ? 0 : 1)
		);
	} else {
		ctx.setReturn(FalloutStringCompare(ctx.arg(0).strValue(), ctx.arg(1).strValue(), ctx.arg(2).rawValue()));
	}
}

void mf_string_find(OpcodeContext& ctx) {
	const char* const haystack = ctx.arg(0).strValue();
	int pos = 0;
	if (ctx.numArgs() > 2) {
		int len = strlen(haystack);
		pos = ctx.arg(2).intValue();
		if (pos >= len) {
			ctx.setReturn(-1);
			return;
		} else if (pos < 0) {
			pos += len;
		}
	}
	const char* needle = strstr(haystack + pos, ctx.arg(1).strValue());
	ctx.setReturn(
		needle != nullptr ? (int)(needle - haystack) : -1
	);
}

// A safer version of sprintf for using in user scripts.
static const char* sprintf_lite(OpcodeContext& ctx, const char* opcodeName) {
	const char* format = ctx.arg(0).strValue();
	int fmtLen = strlen(format);
	if (fmtLen == 0) {
		return format;
	}
	if (fmtLen > 1024) {
		ctx.printOpcodeError("%s() - format string exceeds maximum length of 1024 characters.", opcodeName);
		return "Error";
	}
	int newFmtLen = fmtLen;

	for (int i = 0; i < fmtLen; i++) {
		if (format[i] == '%') newFmtLen++; // will possibly be escaped, need space for that
	}

	// parse format to make it safe
	char* newFmt = new char[newFmtLen + 1];
	bool conversion = false;
	int j = 0;
	int valIdx = 0;
	char* outBuf = ScriptExtender::gTextBuffer;
	long bufCount = ScriptExtender::TextBufferSize() - 1;
	int numArgs = ctx.numArgs();

	for (int i = 0; i < fmtLen; i++) {
		char c = format[i];
		if (!conversion) {
			// Start conversion.
			if (c == '%') conversion = true;
		} else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '%') {
			int partLen;
			if (c == '%') {
				// escaped % sign, just copy newFmt up to (and including) the leading % sign
				newFmt[j] = '\0';
				strncpy_s(outBuf, bufCount, newFmt, j);
				partLen = j;
			} else {
				// ignore size prefixes
				if (c == 'h' || c == 'l' || c == 'j' || c == 'z' || c == 't' || c == 'w' || c == 'L' || c == 'I') continue;
				// Type specifier, perform conversion.
				if (++valIdx == numArgs) {
					ctx.printOpcodeError("%s() - format string contains more conversions than passed arguments (%d): %s", opcodeName, numArgs - 1, format);
				}
				const auto& arg = ctx.arg(valIdx < numArgs ? valIdx : numArgs - 1);
				if (c == 'S' || c == 'Z') {
					c = 's'; // don't allow wide strings
				}
				if (c == 's' && !arg.isString() || // don't allow treating non-string values as string pointers
				    c == 'n') // don't allow "n" specifier
				{
					c = 'd';
				}
				newFmt[j++] = c;
				newFmt[j] = '\0';
				partLen = arg.isFloat()
				        ? _snprintf(outBuf, bufCount, newFmt, arg.floatValue())
				        : _snprintf(outBuf, bufCount, newFmt, arg.rawValue());
			}
			outBuf += partLen;
			bufCount -= partLen;
			conversion = false;
			j = 0;
			if (bufCount <= 0) {
				break;
			}
			continue;
		}
		newFmt[j++] = c;
	}
	// Copy the remainder of the string.
	if (bufCount > 0) {
		newFmt[j] = '\0';
		strcpy_s(outBuf, bufCount, newFmt);
	}

	delete[] newFmt;
	return ScriptExtender::gTextBuffer;
}

void op_sprintf(OpcodeContext& ctx) {
	ctx.setReturn(
		sprintf_lite(ctx, ctx.getOpcodeName())
	);
}

void mf_string_format(OpcodeContext& ctx) {
	ctx.setReturn(
		sprintf_lite(ctx, ctx.getMetaruleName())
	);
}

void op_message_str_game(OpcodeContext& ctx) {
	const char* msg = nullptr;

	int fileId = ctx.arg(0).rawValue();
	if (fileId >= 0) {
		int msgId = ctx.arg(1).rawValue();
		if (fileId <= 20) { // main msg files
			msg = fo::util::GetMessageStr(Message::gameMsgFiles[fileId], msgId);
		} else if (fileId >= 0x1000 && fileId <= 0x1005) { // proto msg files
			msg = fo::util::GetMessageStr(&fo::var::proto_msg_files[fileId - 0x1000], msgId);
		} else if (fileId >= 0x2000) { // Extra game message files.
			ExtraGameMessageListsMap::iterator it = Message::gExtraGameMsgLists.find(fileId);
			if (it != Message::gExtraGameMsgLists.end()) {
				msg = fo::util::GetMessageStr(it->second.get(), msgId);
			}
		}
	}
	if (msg == nullptr) msg = "Error";
	ctx.setReturn(msg);
}

void mf_add_extra_msg_file(OpcodeContext& ctx) {
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

void mf_get_string_pointer(OpcodeContext& ctx) {
	ctx.setReturn(reinterpret_cast<long>(ctx.arg(0).strValue()), DataType::INT);
}

void mf_get_text_width(OpcodeContext& ctx) {
	ctx.setReturn(fo::util::GetTextWidth(ctx.arg(0).strValue()));
}

static std::string strToCase;

void mf_string_to_case(OpcodeContext& ctx) {
	strToCase = ctx.arg(0).strValue();
	std::transform(strToCase.begin(), strToCase.end(), strToCase.begin(), ctx.arg(1).rawValue() ? ::toupper : ::tolower);

	ctx.setReturn(strToCase.c_str());
}

}
}
