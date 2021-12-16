/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "..\..\Modules\Console.h"

#include "Text.h"

namespace game
{
namespace gui
{

namespace sf = sfall;

// Returns the position of the newline character, or the position of the character within the specified width (implementation from HRP)
static long GetPositionWidth(const char* text, long width) {
	long gapWidth;
	__asm {
		call dword ptr ds:[FO_VAR_text_spacing];
		mov  gapWidth, eax;
	}

	long wordCharCount = 0;
	long position = 0;
	long w = 0;

	char c = text[position];
	while (c) {
		if (c == '\\' && text[position + 1] == 'n') return position;

		if (c != ' ') wordCharCount++; else wordCharCount = 0;

		w += gapWidth + fo::util::GetCharWidth(c);
		if (w > width) {
			// set the position to the beginning of the current word
			if (wordCharCount > 1 && ((wordCharCount - 1) != position)) {
				position -= wordCharCount; // position at the space char
			}
			break;
		}
		c = text[++position];
	}
	return position;
}

// Replaces the implementation of display_print_ function from HRP with support for the newline control character '\n'
// Works with vanilla and HRP 4.1.8
static void __fastcall DisplayPrintLineBreak(const char* message) {
	if (*message == 0 || !fo::var::getInt(FO_VAR_disp_init)) return;

	sf::Console::PrintFile(message);

	const long max_lines = 100; // aka FO_VAR_max
	long max_disp_chars = 256;  // HRP value (vanilla 80)
	char* display_string_buf_addr;

	long width = (sf::hrpIsEnabled) ? sf::GetIntHRPValue(HRP_VAR_disp_width) : 0;
	if (width == 0) {
		width = 167; // vanilla size
		max_disp_chars = 80;
		display_string_buf_addr = (char*)FO_VAR_display_string_buf; // array size 100x80
	} else {
		display_string_buf_addr = (char*)sf::HRPAddress(HRP_VAR_display_string_buf); // array size 100x256, allocated by HRP
	}

	if (!(fo::var::combat_state & fo::CombatStateFlag::InCombat)) {
		long time = fo::var::getInt(FO_VAR_bk_process_time);
		if ((time - fo::var::getInt(FO_VAR_lastTime)) >= 500) {
			fo::var::setInt(FO_VAR_lastTime) = time;
			fo::func::gsound_play_sfx_file((const char*)0x50163C); // "monitor"
		}
	}

	long font = fo::var::curr_font_num;
	fo::func::text_font(101);

	unsigned char bulletChar = 149;
	long wChar = fo::util::GetCharWidth(bulletChar);
	width -= (wChar + fo::var::getInt(FO_VAR_max_disp));

	do {
		char* display_string_buf = &display_string_buf_addr[max_disp_chars * fo::var::getInt(FO_VAR_disp_start)];

		long pos = GetPositionWidth(message, width);

		if (bulletChar) {
			*display_string_buf = bulletChar;
			display_string_buf++;
			bulletChar = 0;
			width += wChar;
		}

		std::strncpy(display_string_buf, message, pos);
		display_string_buf[pos] = 0;

		if (message[pos] == ' ') {
			pos++;
		} else if (message[pos] == '\\' && message[pos + 1] == 'n') {
			pos += 2; // position after the 'n' character
		}
		message += pos;

		fo::var::setInt(FO_VAR_disp_start) = (fo::var::getInt(FO_VAR_disp_start) + 1) % max_lines;
	} while (*message);

	fo::var::setInt(FO_VAR_disp_curr) = fo::var::getInt(FO_VAR_disp_start);

	fo::func::text_font(font);
	__asm call fo::funcoffs::display_redraw_;
}

static void __declspec(naked) sf_display_print() {
	__asm {
		push ecx;
		mov  ecx, eax; // message
		call DisplayPrintLineBreak;
		pop  ecx;
		retn;
	}
}

static void __stdcall SplitPrintMessage(char* message, void* printFunc) {
	char* text = message;
	while (*text) {
		if (text[0] == '\\' && text[1] == 'n') {
			*text = 0; // set null terminator

			__asm mov  eax, message;
			__asm call printFunc;

			*text = '\\';
			text += 2; // position after the 'n' character
			message = text;
		} else {
			text++;
		}
	}
	// print the last line or all the text if there is no line break
	if (message != text) {
		__asm mov  eax, message;
		__asm call printFunc;
	}
}

static void __declspec(naked) sf_display_print_alt() {
	__asm {
		push ecx;
		push fo::funcoffs::display_print_;
		push eax; // message
		call SplitPrintMessage;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) sf_inven_display_msg() {
	__asm {
		push ecx;
		push fo::funcoffs::inven_display_msg_;
		push eax; // message
		call SplitPrintMessage;
		pop  ecx;
		retn;
	}
}

void Text::init() {
	// Support for the newline control character '\n' in the object description in pro_*.msg files
	void* printFunc = sf_display_print; // for vanilla and HRP 4.1.8
	if (sf::versionCHI || (sf::hrpIsEnabled && !sf::hrpVersionValid)) {
		printFunc = sf_display_print_alt;
	}
	sf::SafeWriteBatch<DWORD>((DWORD)printFunc, {0x46ED87, 0x49AD7A}); // setup_inventory_, obj_examine_
	sf::SafeWrite32(0x472F9A, (DWORD)&sf_inven_display_msg); // inven_obj_examine_func_
}

}
}