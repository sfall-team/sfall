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

#pragma once

#include "Structs.h"

#define DWORD unsigned long

// 
// WRAPPERS for FO engine functions. 
// Use those as you would if there were source code for the engine...
//
namespace Wrapper
{

int _stdcall isPartyMember(TGameObj* obj);

int _stdcall partyMemberGetCurLevel(TGameObj* obj);

char* proto_ptr(DWORD pid);

// Displays message in main UI console window
void display_print(const char* msg);

int __stdcall item_get_type(TGameObj* item);

// Change the name of playable character
void critter_pc_set_name(const char* newName);

// Returns the name of the critter
const char* __stdcall critter_name(TGameObj* critter);

// searches for message ID in given message file and places result in @result
const char* _stdcall getmsg(DWORD fileAddr, int messageId, sMessage* result);

// Saves pointer to script object into scriptPtr using scriptID. 
// Returns 0 on success, -1 on failure.
int __stdcall scr_ptr(int scriptId, TScript** scriptPtr);

void skill_get_tags(int* result, DWORD num);
void skill_set_tags(int* tags, DWORD num);

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void intface_redraw();

// critter worn item (armor)
TGameObj* __stdcall inven_worn(TGameObj* critter);

// item in critter's left hand slot
TGameObj* __stdcall inven_left_hand(TGameObj* critter);

// item in critter's right hand slot
TGameObj* __stdcall inven_right_hand(TGameObj* critter);

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall interpretPopShort(TProgram* scriptPtr);

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall interpretPopLong(TProgram* scriptPtr);

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall interpretPushLong(TProgram* scriptPtr, DWORD val);

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall interpretPushShort(TProgram* scriptPtr, DWORD valType);

const char* __stdcall interpretGetString(TProgram* scriptPtr, DWORD strId, DWORD dataType);

DWORD __stdcall interpretAddString(TProgram* scriptPtr, const char* str);

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec() interpretError(const char* fmt, ...);

// prints message to debug.log file
void __declspec() debug_printf(const char* fmt, ...);

// returns the name of current procedure by program pointer
const char* __stdcall findCurrentProc(TProgram* program);
}
