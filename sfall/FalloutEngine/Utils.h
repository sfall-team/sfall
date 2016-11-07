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

//
// Various utility functions, based on FO engine functions
//

#define DWORD unsigned long

// returns weapon animation code
char AnimCodeByWeapon(TGameObj* weapon);

// returns message string from given file
const char* _stdcall GetMessageStr(DWORD fileAddr, int messageId);

// returns pointer to prototype by PID, or nullptr on failure
sProtoBase* GetProto(int pid);

sProtoItem* GetItemProto(int pid);

const char* MsgSearch(int msgno, DWORD* file);

// wrapper for skill_get_tags with bounds checking
void SkillGetTags(int* result, long num);

// wrapper for skill_set_tags with bounds checking
void SkillSetTags(int* tags, long num);