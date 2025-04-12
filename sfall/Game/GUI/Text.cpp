/*
 *    sfall
 *    Copyright (C) 2008-2025  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "Text.h"

namespace game
{
namespace gui
{

namespace sf = sfall;

static void __stdcall SplitPrintMessage(char* message, void* printFunc) {
	char* text = message;
	while (*text) {
		if (text[0] == '\\' && text[1] == 'n') {
			*text = '\0'; // set null terminator

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

static __declspec(naked) void sf_inven_display_msg() {
	__asm {
		push ecx;
		push fo::funcoffs::inven_display_msg_;
		push eax; // message
		call SplitPrintMessage;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void sf_display_print_alt() {
	__asm {
		push ecx;
		push fo::funcoffs::display_print_; // func replaced by HRP
		push eax; // message
		call SplitPrintMessage;
		pop  ecx;
		retn;
	}
}

static void __fastcall ObjDescStripNewlines(char* desc) {
	size_t i = 0, j = 0;
	while (desc[i]) {
		if (desc[i] == '\\' && desc[i + 1] == 'n') {
			desc[j++] = ' ';
			i += 2;
		} else {
			desc[j++] = desc[i++];
		}
	}
	desc[j] = '\0'; // null-terminate the modified string
}

static __declspec(naked) void obj_examine_func_hack() {
	__asm {
		cmp  dword ptr [esp + 0x1AC - 0x14 + 4], 0x445448; // gdialogDisplayMsg_
		jne  skip;
		push eax;
		push ecx;
		mov  ecx, eax;
		call ObjDescStripNewlines;
		pop  ecx;
		pop  eax;
skip:
		mov  edx, ds:[FO_VAR_proto_none_str]; // overwritten engine code
		retn;
	}
}

void Text::init() {
	// Support for the newline control character '\n' in the object description in pro_*.msg files
	const DWORD displayPrintAltAddr[] = {0x46ED87, 0x49AD7A}; // setup_inventory_, obj_examine_
	sf::SafeWriteBatch<DWORD>((DWORD)&sf_display_print_alt, displayPrintAltAddr);
	sf::SafeWrite32(0x472F9A, (DWORD)&sf_inven_display_msg); // inven_obj_examine_func_
	// Remove visible newline control characters when examining items in the barter screen
	sf::MakeCall(0x49AE33, obj_examine_func_hack, 1);
}

}
}
