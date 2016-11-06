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

int __stdcall ItemGetType(TGameObj* item) {
	__asm {
		mov eax, item
		call FuncOffs::item_get_type_
	}
}

int _stdcall IsPartyMember(TGameObj* obj) {
	__asm {
		mov eax, obj
		call FuncOffs::isPartyMember_
	}
}

int _stdcall PartyMemberGetCurrentLevel(TGameObj* obj) {
	__asm {
		mov eax, obj
		call FuncOffs::partyMemberGetCurLevel_
	}
}

TGameObj* __stdcall GetInvenWeaponLeft(TGameObj* obj) {
	__asm {
		mov eax, obj
		call FuncOffs::inven_left_hand_
	}
}

TGameObj* __stdcall GetInvenWeaponRight(TGameObj* obj) {
	__asm {
		mov eax, obj
		call FuncOffs::inven_right_hand_
	}
}


char* GetProtoPtr(DWORD pid) {
	char* proto;
	__asm {
		mov eax, pid
		lea edx, proto
		call FuncOffs::proto_ptr_
	}
	return proto;
}

// TODO: this is not wrapper, move it
char AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		char* proto = GetProtoPtr(weapon->pid);
		if (proto && *(int*)(proto + 32) == 3) {
			return (char)(*(int*)(proto + 36)); 
		}
	}
	return 0;
}

// Displays message in main UI console window
void DisplayConsoleMessage(const char* msg) {
	__asm {
		mov eax, msg
		call FuncOffs::display_print_
	}
}

static DWORD mesg_buf[4] = {0, 0, 0, 0};
const char* _stdcall GetMessageStr(DWORD fileAddr, DWORD messageId) {
	DWORD buf = (DWORD)mesg_buf;
	const char* result;
	__asm {
		mov eax, fileAddr
		mov ebx, messageId
		mov edx, buf
		call FuncOffs::getmsg_
		mov result, eax
	}
	return result;
}

// Change the name of playable character
void CritterPcSetName(const char* newName) {
	__asm {
		mov eax, newName
		call FuncOffs::critter_pc_set_name_
	}
}

// Returns the name of the critter
const char* __stdcall CritterName(TGameObj* critter) {
	__asm {
		mov eax, critter
		call FuncOffs::critter_name_
	}
}

void SkillGetTags(int* result, DWORD num) {
	if (num > 4) {
		num = 4;
	}
	__asm {
		mov eax, result
		mov edx, num
		call FuncOffs::skill_get_tags_
	}
}

void SkillSetTags(int* tags, DWORD num) {
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
int __stdcall ScrPtr(int scriptId, TScript** scriptPtr) {
	__asm {
		mov eax, scriptId;
		mov edx, scriptPtr;
		call FuncOffs::scr_ptr_;
	}
}

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void InterfaceRedraw() {
	__asm call FuncOffs::intface_redraw_
}

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall InterpretPopShort(TProgram* scriptPtr) {
	__asm {
		mov eax, scriptPtr
		call FuncOffs::interpretPopShort_
	}
}

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall InterpretPopLong(TProgram* scriptPtr) {
	__asm {
		mov eax, scriptPtr
		call FuncOffs::interpretPopLong_
	}
}

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall InterpretPushLong(TProgram* scriptPtr, DWORD val) {
	__asm {
		mov edx, val
		mov eax, scriptPtr
		call FuncOffs::interpretPushLong_
	}
}

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall InterpretPushShort(TProgram* scriptPtr, DWORD valType) {
	__asm {
		mov edx, valType
		mov eax, scriptPtr
		call FuncOffs::interpretPushShort_
	}
}

DWORD __stdcall InterpretAddString(TProgram* scriptPtr, const char* strval) {
	__asm {
		mov edx, strval
		mov eax, scriptPtr
		call FuncOffs::interpretAddString_
	}
}

const char* __stdcall InterpretGetString(TProgram* scriptPtr, DWORD strId, DWORD dataType) {
	__asm {
		mov edx, dataType
		mov ebx, strId
		mov eax, scriptPtr
		call FuncOffs::interpretGetString_
	}
}

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec(naked) InterpretError(const char* fmt, ...) {
	__asm jmp FuncOffs::interpretError_
}

// prints message to debug.log file
void __declspec(naked) DebugPrintf(const char* fmt, ...) {
	__asm jmp FuncOffs::debug_printf_
}

// returns the name of current procedure by program pointer
const char* __stdcall FindCurrentProc(TProgram* program) {
	__asm mov eax, program
	__asm call FuncOffs::findCurrentProc_
}

// critter worn item (armor)
TGameObj* __stdcall InvenWorn(TGameObj* critter) {
	__asm mov eax, critter
	__asm call FuncOffs::inven_worn_
}

// item in critter's left hand slot
TGameObj* __stdcall InvenLeftHand(TGameObj* critter) {
	__asm mov eax, critter
	__asm call FuncOffs::inven_left_hand_
}

// item in critter's right hand slot
TGameObj* __stdcall InvenRightHand(TGameObj* critter) {
	__asm mov eax, critter
	__asm call FuncOffs::inven_right_hand_
}
