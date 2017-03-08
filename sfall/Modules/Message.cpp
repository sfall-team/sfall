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

#include <string>
#include <memory>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Message.h"

namespace sfall
{

ExtraGameMessageListsMap gExtraGameMsgLists;

fo::MessageNode *GetMsgNode(fo::MessageList *msgList, int msgRef) {
	if (msgList != nullptr && msgList->numMsgs > 0) {
		fo::MessageNode *msgNode = msgList->nodes;
		long last = msgList->numMsgs - 1;
		long first = 0;
		long mid;

		//Use Binary Search to find msg
		while (first <= last) {
			mid = (first + last) / 2;
			if (msgRef > msgNode[mid].number)
				first = mid + 1;
			else if (msgRef < msgNode[mid].number)
				last = mid - 1;
			else
				return &msgNode[mid];
		}
	}
	return nullptr;
}

char* GetMsg(fo::MessageList *msgList, int msgRef, int msgNum) {
	fo::MessageNode *msgNode = GetMsgNode(msgList, msgRef);
	if (msgNode) {
		if (msgNum == 2) {
			return msgNode->message;
		} else if (msgNum == 1) {
			return msgNode->audio;
		}
	}
	return NULL;
}

void ReadExtraGameMsgFiles() {
	auto msgFileList = GetConfigList("Misc", "ExtraGameMsgFileList", "", 512);
	if (msgFileList.size() > 0) {
		for (auto& msgName : msgFileList) {
			std::string path = "game\\" + msgName + ".msg";
			fo::MessageList* list = new fo::MessageList();
			if (fo::func::message_load(list, (char*)path.data()) == 1) {
				gExtraGameMsgLists.insert(std::make_pair(0x2000 + gExtraGameMsgLists.size(), list));
			} else {
				delete list;
			}
		}
	}
}

void ClearReadExtraGameMsgFiles() {
	ExtraGameMessageListsMap::iterator it;

	for (it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		fo::func::message_exit(it->second.get());
	}

	gExtraGameMsgLists.clear();
}

void Message::init() {
	LoadGameHook::onGameInit += ReadExtraGameMsgFiles;
}

void Message::exit() {
	ClearReadExtraGameMsgFiles();
}

}
