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

#define CASTMSG(adr) reinterpret_cast<MSGList*>(adr)
const MSGList* gameMsgFiles[] = {
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

// Loads the msg file from the 'english' folder if it does not exist in the current language directory
static void __declspec(naked) message_load_hook() {
	__asm {
		mov  ebx, edx; // keep mode
		mov  ecx, eax; // keep buf
		call db_fopen_;
		test eax, eax;
		jz   noFile;
		retn;
noFile:
		push ebp;      // file
		push 0x500208; // "english"
		push 0x50B7D0; // "text"
		push 0x50B7D8; // "%s\%s\%s"
		push ecx;      // buf
		call sprintf_;
		add  esp, 20;
		mov  edx, ebx;
		mov  eax, ecx;
		jmp  db_fopen_;
	}
}

MSGNode* GetMsgNode(MSGList* msgList, int msgRef) {
	if (msgList != nullptr && msgList->numMsgs > 0) {
		MSGNode *MsgNode = msgList->nodes;
		long last = msgList->numMsgs - 1;
		long first = 0;
		long mid;

		// Use Binary Search to find msg
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

char* GetMsg(MSGList* msgList, int msgRef, int msgNum) {
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

void FallbackEnglishLoadMsgFiles() {
	char value[128];
	if (GetGameConfigString(value, "system", "language") && _stricmp(value, "english") != 0) {
		HookCall(0x484B18, message_load_hook);
	}
}

void ClearReadExtraGameMsgFiles() {
	for (ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		DestroyMsgList(it->second);
		delete it->second;
	}
}
