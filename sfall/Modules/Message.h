/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <unordered_map>

#include "..\main.h"

#include "Module.h"

namespace sfall
{

typedef std::tr1::unordered_map<int, std::unique_ptr<fo::MessageList>> ExtraGameMessageListsMap;
extern ExtraGameMessageListsMap gExtraGameMsgLists;

class Message : public Module {
	const char* name() { return "Message"; }
	void init();
	void exit() override;
};

fo::MessageNode *GetMsgNode(fo::MessageList *msgList, int msgRef);
char* GetMsg(fo::MessageList *MsgList, int msgRef, int msgNum);

}
