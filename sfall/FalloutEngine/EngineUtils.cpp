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

#include "EngineUtils.h"

namespace fo
{

static MessageNode message_buf;

const char* _stdcall GetMessageStr(const MessageList* fileAddr, int messageId) {
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

Proto* GetProto(int pid) {
	Proto* protoPtr;
	if (fo::func::proto_ptr(pid, &protoPtr) != -1) {
		return protoPtr;
	}
	return nullptr;
}

void SkillGetTags(int* result, long num) {
	if (num > 4) {
		num = 4;
	}
	fo::func::skill_get_tags(result, num);
}

void SkillSetTags(int* tags, long num) {
	if (num > 4) {
		num = 4;
	}
	fo::func::skill_set_tags(tags, num);
}

}
