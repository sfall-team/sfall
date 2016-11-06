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

#include "Functions.h"
#include "Wrappers.h"

namespace Wrapper
{

int __stdcall item_get_type(TGameObj* item) {
	__asm {
		mov eax, item
		call FuncOffs::item_get_type_
	}
}

int _stdcall isPartyMember(TGameObj* obj) {
	__asm {
		mov eax, obj
		call FuncOffs::isPartyMember_
	}
}

int _stdcall partyMemberGetCurLevel(TGameObj* obj) {
	__asm {
		mov eax, obj
		call FuncOffs::partyMemberGetCurLevel_
	}
}

char* proto_ptr(DWORD pid) {
	char* proto;
	__asm {
		mov eax, pid
		lea edx, proto
		call FuncOffs::proto_ptr_
	}
	return proto;
}

// Displays message in main UI console window
void display_print(const char* msg) {
	__asm {
		mov eax, msg
		call FuncOffs::display_print_
	}
}

const char* _stdcall getmsg(DWORD fileAddr, int messageId, sMessage* result) {
	__asm {
		mov eax, fileAddr
		mov ebx, messageId
		mov edx, result
		call FuncOffs::getmsg_
	}
}

// Change the name of playable character
void critter_pc_set_name(const char* newName) {
	__asm {
		mov eax, newName
		call FuncOffs::critter_pc_set_name_
	}
}

// Returns the name of the critter
const char* __stdcall critter_name(TGameObj* critter) {
	__asm {
		mov eax, critter
		call FuncOffs::critter_name_
	}
}

void skill_get_tags(int* result, DWORD num) {
	if (num > 4) {
		num = 4;
	}
	__asm {
		mov eax, result
		mov edx, num
		call FuncOffs::skill_get_tags_
	}
}

void skill_set_tags(int* tags, DWORD num) {
	if (num > 4) {
		num = 4;
	}
	__asm {
		mov eax, tags
		mov edx, num
		call FuncOffs::skill_set_tags_
	}
}

// Saves pointer to script object into scriptPtr using scriptID. 
// Returns 0 on success, -1 on failure.
int __stdcall scr_ptr(int scriptId, TScript** scriptPtr) {
	__asm {
		mov eax, scriptId;
		mov edx, scriptPtr;
		call FuncOffs::scr_ptr_;
	}
}

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void intface_redraw() {
	__asm call FuncOffs::intface_redraw_
}

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall interpretPopShort(TProgram* scriptPtr) {
	__asm {
		mov eax, scriptPtr
		call FuncOffs::interpretPopShort_
	}
}

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall interpretPopLong(TProgram* scriptPtr) {
	__asm {
		mov eax, scriptPtr
		call FuncOffs::interpretPopLong_
	}
}

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall interpretPushLong(TProgram* scriptPtr, DWORD val) {
	__asm {
		mov edx, val
		mov eax, scriptPtr
		call FuncOffs::interpretPushLong_
	}
}

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall interpretPushShort(TProgram* scriptPtr, DWORD valType) {
	__asm {
		mov edx, valType
		mov eax, scriptPtr
		call FuncOffs::interpretPushShort_
	}
}

DWORD __stdcall interpretAddString(TProgram* scriptPtr, const char* strval) {
	__asm {
		mov edx, strval
		mov eax, scriptPtr
		call FuncOffs::interpretAddString_
	}
}

const char* __stdcall interpretGetString(TProgram* scriptPtr, DWORD strId, DWORD dataType) {
	__asm {
		mov edx, dataType
		mov ebx, strId
		mov eax, scriptPtr
		call FuncOffs::interpretGetString_
	}
}

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec(naked) interpretError(const char* fmt, ...) {
	__asm jmp FuncOffs::interpretError_
}

// prints message to debug.log file
void __declspec(naked) debug_printf(const char* fmt, ...) {
	__asm jmp FuncOffs::debug_printf_
}

// returns the name of current procedure by program pointer
const char* __stdcall findCurrentProc(TProgram* program) {
	__asm mov eax, program
	__asm call FuncOffs::findCurrentProc_
}

// critter worn item (armor)
TGameObj* __stdcall inven_worn(TGameObj* critter) {
	__asm mov eax, critter
	__asm call FuncOffs::inven_worn_
}

// item in critter's left hand slot
TGameObj* __stdcall inven_left_hand(TGameObj* critter) {
	__asm mov eax, critter
	__asm call FuncOffs::inven_left_hand_
}

// item in critter's right hand slot
TGameObj* __stdcall inven_right_hand(TGameObj* critter) {
	__asm mov eax, critter
	__asm call FuncOffs::inven_right_hand_
}

int __stdcall message_search(DWORD* file, sMessage* msg) {
	__asm {
		mov edx, msg;
		mov eax, file;
		call FuncOffs::message_search_;
	}
}

}
