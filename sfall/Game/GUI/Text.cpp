/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "..\..\Modules\Console.h"

#include "..\..\HRP\InterfaceBar.h"

#include "Text.h"

namespace game
{
namespace gui
{

namespace sf = sfall;

// Returns the position of the newline character, or the position of the character within the specified width (implementation from HRP)
static long GetPositionWidth(const char* text, long width, bool lineBreak) {
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
		if (lineBreak && c == '\\' && text[position + 1] == 'n') return position;

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
static void __fastcall DisplayPrint(const char* message, bool lineBreak) {
	if (*message == 0 || !fo::var::getInt(FO_VAR_disp_init)) return;

	sf::Console::PrintFile(message);

	const long max_lines = 100; // aka FO_VAR_max
	long max_disp_chars = 256;  // HRP value (vanilla 80)
	char* display_string_buf_addr = HRP::IFaceBar::display_string_buf; // array size: 100x80 (or 100x256 for sfall HRP)

	long width = (HRP::Setting::ExternalEnabled()) ? sf::GetIntHRPValue(HRP_VAR_disp_width) : HRP::IFaceBar::display_width;
	if (width == 0) {
		width = 167; // vanilla size
		max_disp_chars = 80;
	} else if (HRP::Setting::ExternalEnabled()) {
		display_string_buf_addr = (char*)HRP::Setting::GetAddress(HRP_VAR_display_string_buf); // array size 100x256, allocated by Mash's HRP
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

		long pos = GetPositionWidth(message, width, lineBreak);

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
		} else if (lineBreak && message[pos] == '\\' && message[pos + 1] == 'n') {
			pos += 2; // position after the 'n' character
		}
		message += pos;

		fo::var::setInt(FO_VAR_disp_start) = (fo::var::getInt(FO_VAR_disp_start) + 1) % max_lines;
	} while (*message);

	fo::var::setInt(FO_VAR_disp_curr) = fo::var::getInt(FO_VAR_disp_start);

	fo::func::text_font(font);
	__asm call fo::funcoffs::display_redraw_;
}

static __declspec(naked) void display_print_hack_replacemet() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, eax; // message
		xor  edx, edx;
		call DisplayPrint;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void display_print_line_break() {
	__asm {
		push ecx;
		mov  dl, 1;    // with line break
		mov  ecx, eax; // message
		call DisplayPrint;
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

static __declspec(naked) void inven_display_msg_line_break() {
	__asm {
		push ecx;
		push fo::funcoffs::inven_display_msg_;
		push eax; // message
		call SplitPrintMessage;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void display_print_line_break_extHRP() {
	__asm {
		push ecx;
		push fo::funcoffs::display_print_; // func replaced by Mash's HRP
		push eax; // message
		call SplitPrintMessage;
		pop  ecx;
		retn;
	}
}

void Text::init() {
	void* printFunc = display_print_line_break; // for vanilla and HRP 4.1.8

	if (!sf::versionCHI && HRP::Setting::IsEnabled()) {
		sf::MakeJump(fo::funcoffs::display_print_, display_print_hack_replacemet); // 0x43186C
	} else {
		if (sf::versionCHI || (HRP::Setting::ExternalEnabled() && !HRP::Setting::VersionIsValid)) {
			printFunc = display_print_line_break_extHRP;
		}
	}

	// Support for the newline control character '\n' in the object description in pro_*.msg files
	sf::SafeWriteBatch<DWORD>((DWORD)printFunc, {0x46ED87, 0x49AD7A}); // setup_inventory_, obj_examine_
	sf::SafeWrite32(0x472F9A, (DWORD)&inven_display_msg_line_break);   // inven_obj_examine_func_
}

}
}
