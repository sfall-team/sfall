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

#include "Wrappers.h"
#include "Structs.h"

#include "Utils.h"

static sMessage message_buf;

const char* _stdcall GetMessageStr(DWORD fileAddr, int messageId) {
	return Wrapper::getmsg(fileAddr, messageId, &message_buf);
}

char AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		char* proto = Wrapper::proto_ptr(weapon->pid);
		if (proto && *(int*)(proto + 32) == 3) {
			return (char)(*(int*)(proto + 36)); 
		}
	}
	return 0;
}

const char* MsgSearch(int msgno, DWORD* file) {
	if(!file) return 0;
	sMessage msg = { msgno, 0, 0, 0 };
	Wrapper::message_search(file, &msg);
	return msg.message;
}

