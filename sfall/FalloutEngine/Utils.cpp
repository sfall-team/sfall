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

#include "Wrappers.h"
#include "Structs.h"

#include "Utils.h"

static MessageNode message_buf;

const char* _stdcall GetMessageStr(const MessageList* fileAddr, int messageId) {
	return Wrapper::getmsg(fileAddr, &message_buf, messageId);
}

char AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != nullptr) {
		sProtoItem* proto = GetItemProto(weapon->pid);
		if (proto != nullptr && proto->type == item_type_weapon) {
			// TODO: find better way to cast into specific item type proto
			return static_cast<char>(reinterpret_cast<sProtoWeapon*>(proto)->animation_code);
		}
	}
	return 0;
}

sProtoItem* GetItemProto(int pid) {
	assert((pid >> 24) == OBJ_TYPE_ITEM);

	return reinterpret_cast<sProtoItem*>(GetProto(pid));
}

sProtoBase* GetProto(int pid) {
	sProtoBase* protoPtr;
	if (Wrapper::proto_ptr(pid, &protoPtr) != -1) {
		return protoPtr;
	}
	return nullptr;
}

void SkillGetTags(int* result, long num) {
	if (num > 4) {
		num = 4;
	}
	Wrapper::skill_get_tags(result, num);
}

void SkillSetTags(int* tags, long num) {
	if (num > 4) {
		num = 4;
	}
	Wrapper::skill_set_tags(tags, num);
}
