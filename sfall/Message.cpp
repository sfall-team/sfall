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
#include "main.h"

#include "FalloutEngine.h"
#include "Message.h"

ExtraGameMessageListsMap gExtraGameMsgLists;

long __stdcall LoadMsgList(MSGList *msgList, const char *msgFilePath) {
	__asm {
		mov  edx, msgFilePath;
		mov  eax, msgList;
		call message_load_;
	}
}

long __stdcall DestroyMsgList(MSGList *msgList) {
	__asm {
		mov  eax, msgList;
		call message_exit_;
	}
}

MSGNode *GetMsgNode(MSGList *msgList, int msgRef) {
	if (msgList != nullptr && msgList->numMsgs > 0) {
		MSGNode *MsgNode = msgList->nodes;
		long last = msgList->numMsgs - 1;
		long first = 0;
		long mid;

		//Use Binary Search to find msg
		while (first <= last) {
			mid = (first + last) / 2;
			if (msgRef > MsgNode[mid].number)
				first = mid + 1;
			else if (msgRef < MsgNode[mid].number)
				last = mid - 1;
			else
				return &MsgNode[mid];
		}
	}
	return nullptr;
}

char* GetMsg(MSGList *msgList, int msgRef, int msgNum) {
	MSGNode *msgNode = GetMsgNode(msgList, msgRef);
	if (msgNode) {
		if (msgNum == 2) {
			return msgNode->message;
		} else if (msgNum == 1) {
			return msgNode->audio;
		}
	}
	return nullptr;
}

void ReadExtraGameMsgFiles() {
	int read;
	std::string names;

	names.resize(256);

	while ((read = GetConfigString("Misc", "ExtraGameMsgFileList", "",
		(LPSTR)names.data(), names.size())) == names.size() - 1)
		names.resize(names.size() + 256);

	if (names.empty()) return;

	names.resize(names.find_first_of('\0'));
	names.append(",");

	int begin = 0;
	int end;
	int length;
	int number = 0;

	while ((end = names.find_first_of(',', begin)) != std::string::npos) {
		length = end - begin;

		if (length > 0) {
			std::string path = "game\\" + names.substr(begin, length) + ".msg";
			MSGList* list = new MSGList;
			if (LoadMsgList(list, path.c_str()) == 1) {
				gExtraGameMsgLists.insert(std::make_pair(0x2000 + number, list));
			} else {
				delete list;
			}
		}
		if (++number == 4096) break;

		begin = end + 1;
	}
}

void ClearReadExtraGameMsgFiles() {
	for (ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		DestroyMsgList(it->second);
		delete it->second;
	}
	//gExtraGameMsgLists.clear();
}
