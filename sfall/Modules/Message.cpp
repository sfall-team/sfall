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

#define CASTMSG(adr) reinterpret_cast<fo::MessageList*>(adr)
const fo::MessageList* gameMsgFiles[] = {
	CASTMSG(MSG_FILE_COMBAT),
	CASTMSG(MSG_FILE_AI),
	CASTMSG(MSG_FILE_SCRNAME),
	CASTMSG(MSG_FILE_MISC),
	CASTMSG(MSG_FILE_CUSTOM),
	CASTMSG(MSG_FILE_INVENTRY),
	CASTMSG(MSG_FILE_ITEM),
	CASTMSG(MSG_FILE_LSGAME),
	CASTMSG(MSG_FILE_MAP),
	CASTMSG(MSG_FILE_OPTIONS),
	CASTMSG(MSG_FILE_PERK),
	CASTMSG(MSG_FILE_PIPBOY),
	CASTMSG(MSG_FILE_QUESTS),
	CASTMSG(MSG_FILE_PROTO),
	CASTMSG(MSG_FILE_SCRIPT),
	CASTMSG(MSG_FILE_SKILL),
	CASTMSG(MSG_FILE_SKILLDEX),
	CASTMSG(MSG_FILE_STAT),
	CASTMSG(MSG_FILE_TRAIT),
	CASTMSG(MSG_FILE_WORLDMAP)
};
#undef CASTMSG

ExtraGameMessageListsMap gExtraGameMsgLists;
static std::vector<std::string> msgFileList;

static long msgNumCounter = 0x3000;

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
	return nullptr;
}

static void ReadExtraGameMsgFiles() {
	if (!msgFileList.empty()) {
		int number = 0;
		for (auto& msgName : msgFileList) {
			std::string path("game\\");
			auto n = msgName.find(':');
			if (n == std::string::npos) {
				path += msgName;
			} else {
				path += msgName.substr(0, n);
				number = std::stoi(msgName.substr(n + 1), nullptr, 0);
			}
			path += ".msg";
			fo::MessageList* list = new fo::MessageList();
			if (fo::func::message_load(list, (char*)path.data()) == 1) {
				gExtraGameMsgLists.insert(std::make_pair(0x2000 + number, list));
			} else {
				delete list;
			}
			if (++number == 4096) break;
		}
		msgFileList.clear();
	}
}

long Message::AddExtraMsgFile(const char* msgName, long msgNumber) {
	if (msgNumber) {
		if (msgNumber < 0x2000 || msgNumber > 0x2FFF) return -1;
		if (gExtraGameMsgLists.count(msgNumber)) return 0; // file has already been added
	} else if (msgNumCounter > 0x3FFF) return -3;

	std::string path("game\\");
	path += msgName;
	fo::MessageList* list = new fo::MessageList();
	if (fo::func::message_load(list, path.c_str())) {
		if (msgNumber == 0) msgNumber = msgNumCounter++;
		gExtraGameMsgLists.emplace(msgNumber, list);
	} else {
		delete list;
		msgNumber = -2;
	}
	return msgNumber;
}

static void ClearScriptAddedExtraGameMsg() { // C++11
	for (auto it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end();) {
		if (it->first >= 0x3000 && it->first <= 0x3FFF) {
			fo::func::message_exit(it->second.get());
			it = gExtraGameMsgLists.erase(it);
		} else {
			++it;
		}
	}
	msgNumCounter = 0x3000;
}

static void ClearReadExtraGameMsgFiles() {
	for (auto it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		fo::func::message_exit(it->second.get());
	}
	//gExtraGameMsgLists.clear();
}

void Message::init() {
	msgFileList = GetConfigList("Misc", "ExtraGameMsgFileList", "", 512);
	LoadGameHook::OnBeforeGameStart() += ReadExtraGameMsgFiles;
	LoadGameHook::OnGameReset() += ClearScriptAddedExtraGameMsg;
}

void Message::exit() {
	ClearReadExtraGameMsgFiles();
}

}
