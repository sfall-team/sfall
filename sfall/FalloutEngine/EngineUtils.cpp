/*
* sfall
* Copyright (C) 2008-2016 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>

#include "Functions.h"
#include "Structs.h"
#include "VariableOffsets.h"

#include "EngineUtils.h"

// TODO: split these functions into several files
namespace fo
{

static MessageNode message_buf;

const char* _stdcall GetMessageStr(const MessageList* fileAddr, long messageId) {
	return fo::func::getmsg(fileAddr, &message_buf, messageId);
}

long AnimCodeByWeapon(GameObject* weapon) {
	if (weapon != nullptr) {
		Proto* proto = GetProto(weapon->pid);
		if (proto != nullptr && proto->item.type == item_type_weapon) {
			return proto->item.weapon.animationCode;
		}
	}
	return 0;
}

Proto* GetProto(long pid) {
	Proto* protoPtr;
	if (fo::func::proto_ptr(pid, &protoPtr) != -1) {
		return protoPtr;
	}
	return nullptr;
}

void SkillGetTags(long* result, long num) {
	if (num > 4) {
		num = 4;
	}
	fo::func::skill_get_tags(result, num);
}

void SkillSetTags(long* tags, long num) {
	if (num > 4) {
		num = 4;
	}
	fo::func::skill_set_tags(tags, num);
}



//---------------------------------------------------------
//print text to surface
void PrintText(char *DisplayText, BYTE ColourIndex, DWORD Xpos, DWORD Ypos, DWORD TxtWidth, DWORD ToWidth, BYTE *ToSurface) {
	DWORD posOffset = Ypos*ToWidth + Xpos;
	__asm {
		xor eax, eax
		MOV AL, ColourIndex
		push eax
		mov edx, DisplayText
		mov ebx, TxtWidth
		mov ecx, ToWidth
		mov eax, ToSurface
		add eax, posOffset
		call dword ptr ds:[FO_VAR_text_to_buf]
	}
}

//---------------------------------------------------------
//gets the height of the currently selected font
DWORD GetTextHeight() {
	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[FO_VAR_text_height] //get text height
		mov TxtHeight, eax
	}
	return TxtHeight;
}

//---------------------------------------------------------
//gets the length of a string using the currently selected font
DWORD GetTextWidth(char *TextMsg) {
	DWORD TxtWidth;
	__asm {
		mov eax, TextMsg
		call dword ptr ds:[FO_VAR_text_width] //get text width
		mov TxtWidth, eax
	}
	return TxtWidth;
}

//---------------------------------------------------------
//get width of Char for current font
DWORD GetCharWidth(char CharVal) {
	DWORD charWidth;
	__asm {
		mov al, CharVal
		call dword ptr ds:[FO_VAR_text_char_width]
		mov charWidth, eax
	}
	return charWidth;
}

//---------------------------------------------------------
//get maximum string length for current font - if all characters were maximum width
DWORD GetMaxTextWidth(char *TextMsg) {
	DWORD msgWidth;
	__asm {
		mov eax, TextMsg
		call dword ptr ds:[FO_VAR_text_mono_width]
		mov msgWidth, eax
	}
	return msgWidth;
}

//---------------------------------------------------------
//get number of pixels between characters for current font
DWORD GetCharGapWidth() {
	DWORD gapWidth;
	__asm {
		call dword ptr ds:[FO_VAR_text_spacing]
		mov gapWidth, eax
	}
	return gapWidth;
}

//---------------------------------------------------------
//get maximum character width for current font
DWORD GetMaxCharWidth() {
	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[FO_VAR_text_max]
		mov charWidth, eax
	}
	return charWidth;
}

}
