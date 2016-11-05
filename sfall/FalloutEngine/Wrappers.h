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

// WRAPPERS:
// TODO: move these to different namespace
int _stdcall IsPartyMember(TGameObj* obj);
int _stdcall PartyMemberGetCurrentLevel(TGameObj* obj);
TGameObj* __stdcall GetInvenWeaponLeft(TGameObj* obj);
TGameObj* __stdcall GetInvenWeaponRight(TGameObj* obj);
char* GetProtoPtr(DWORD pid);
char AnimCodeByWeapon(TGameObj* weapon);
// Displays message in main UI console window
void DisplayConsoleMessage(const char* msg);
const char* _stdcall GetMessageStr(DWORD fileAddr, DWORD messageId);
int __stdcall ItemGetType(TGameObj* item);

// Change the name of playable character
void CritterPcSetName(const char* newName);

// Returns the name of the critter
const char* __stdcall CritterName(TGameObj* critter);

// Saves pointer to script object into scriptPtr using scriptID. 
// Returns 0 on success, -1 on failure.
int __stdcall ScrPtr(int scriptId, TScript** scriptPtr);

void SkillGetTags(int* result, DWORD num);
void SkillSetTags(int* tags, DWORD num);

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void InterfaceRedraw();

// critter worn item (armor)
TGameObj* __stdcall InvenWorn(TGameObj* critter);

// item in critter's left hand slot
TGameObj* __stdcall InvenLeftHand(TGameObj* critter);

// item in critter's right hand slot
TGameObj* __stdcall InvenRightHand(TGameObj* critter);

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall InterpretPopShort(TProgram* scriptPtr);

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall InterpretPopLong(TProgram* scriptPtr);

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall InterpretPushLong(TProgram* scriptPtr, DWORD val);

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall InterpretPushShort(TProgram* scriptPtr, DWORD valType);

const char* __stdcall InterpretGetString(TProgram* scriptPtr, DWORD strId, DWORD dataType);

DWORD __stdcall InterpretAddString(TProgram* scriptPtr, const char* str);

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec() InterpretError(const char* fmt, ...);

// prints message to debug.log file
void __declspec() DebugPrintf(const char* fmt, ...);

// returns the name of current procedure by program pointer
const char* __stdcall FindCurrentProc(TProgram* program);