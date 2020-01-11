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

static long heroIsFemale = -1;

// Searches the special character in the text and removes the text depending on the player's gender
// example: <MaleText^FemaleText>
static long __fastcall ReplaceGenderWord(fo::MessageNode* msgData, DWORD* msgFile) {
	if (!InDialog() || msgData->flags & MSG_GENDER_CHECK_FLG) return 1;
	if (heroIsFemale < 0) heroIsFemale = fo::HeroIsFemale();

	unsigned char* _pos = (u_char*)msgData->message;
	unsigned char* pos;
	while ((pos = (u_char*)std::strchr((char*)_pos, '^')) != 0) { // pos - pointer to the character position
		_pos = pos; // next find position
		for (u_char* n = pos - 1; ; n--) {
			if (n < (u_char*)msgData->message) {
				_pos++; // error, open char not found
				break;
			} else if (*n == '<') {
				if (heroIsFemale) { // remove left(male) side
					pos++;
					// shift all chars to the left
					do {
						*n++ = *pos++;
						if (*pos == '>') { // skip close char
							_pos = n; // set next find position
							do *n++ = *++pos; while (*pos); // continue shift (with '\0')
							break;
						}
					} while (*pos);
				} else { // remove right(female) side
					pos = n;
					pos++;
					// shift all chars to the left
					do {
						*n++ = *pos++;
						if (pos == _pos) { // skip '^' char
							while (*++pos && *pos != '>');  // skip female side
							do *n++ = *++pos; while (*pos); // continue shift (with '\0')
							break;
						}
					} while (*pos);
				}
				break; // exit for
			}
		}
		if (_pos > pos) break;
	}
	// set flag
	unsigned long outValue;
	fo::func::message_find(msgFile, msgData->number, &outValue);
	((fo::MessageNode*)(msgFile[1] + (outValue * 16)))->flags |= MSG_GENDER_CHECK_FLG;

	return 1;
}

static void __declspec(naked) scr_get_msg_str_speech_hook() {
	__asm {
		call fo::funcoffs::message_search_;
		cmp  eax, 1;
		jne  end;
		push ecx;
		lea  ecx, [esp + 8]; // message data
		mov  edx, [esp + 0x1C - 0x0C + 8]; // message file
		call ReplaceGenderWord;
		pop  ecx;
end:
		retn;
	}
}

// Loads the msg file from the 'english' folder if it does not exist in the current language directory
static void __declspec(naked) message_load_hook() {
	__asm {
		mov  ebx, edx; // keep mode
		mov  ecx, eax; // keep buf
		call fo::funcoffs::db_fopen_;
		test eax, eax;
		jz   noFile;
		retn;
noFile:
		push ebp;      // file
		push 0x500208; // "english"
		push 0x50B7D0; // "text"
		push 0x50B7D8; // "%s\%s\%s"
		push ecx;      // buf
		call fo::funcoffs::sprintf_;
		add  esp, 20;
		mov  edx, ebx;
		mov  eax, ecx;
		jmp  fo::funcoffs::db_fopen_;
	}
}

fo::MessageNode* GetMsgNode(fo::MessageList* msgList, int msgRef) {
	if (msgList != nullptr && msgList->numMsgs > 0) {
		fo::MessageNode *msgNode = msgList->nodes;
		long last = msgList->numMsgs - 1;
		long first = 0;
		long mid;

		// Use Binary Search to find msg
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

char* GetMsg(fo::MessageList* msgList, int msgRef, int msgNum) {
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
			if (fo::func::message_load(list, path.c_str()) == 1) {
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
	if (!fo::func::message_load(list, path.c_str())) {
		// change current language folder
		//path.insert(0, "..\\english\\");
		//if (!fo::func::message_load(list, path.c_str())) {
			delete list;
			return -2;
		//}
	}
	if (msgNumber == 0) msgNumber = msgNumCounter++;
	gExtraGameMsgLists.emplace(msgNumber, list);
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
	heroIsFemale = -1;
}

static void FallbackEnglishLoadMsgFiles() {
	char value[128];
	if (fo::func::get_game_config_string(value, "system", "language") && _stricmp(value, "english") != 0) {
		HookCall(0x484B18, message_load_hook);
	}
}

static void ClearReadExtraGameMsgFiles() {
	for (auto it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		fo::func::message_exit(it->second.get());
	}
}

void Message::init() {
	msgFileList = GetConfigList("Misc", "ExtraGameMsgFileList", "", 512);

	if (GetConfigInt("Misc", "DialogGenderWords", 0)) {
		dlog("Applying dialog gender words patch.", DL_INIT);
		HookCall(0x4A6CEE, scr_get_msg_str_speech_hook);
		SafeWrite16(0x484C62, 0x9090); // message_search_
		dlogr(" Done", DL_INIT);
	}

	LoadGameHook::OnGameInit() += FallbackEnglishLoadMsgFiles;
	LoadGameHook::OnBeforeGameStart() += ReadExtraGameMsgFiles;
	LoadGameHook::OnBeforeGameClose() += ClearReadExtraGameMsgFiles;
	LoadGameHook::OnGameReset() += ClearScriptAddedExtraGameMsg;
}

void Message::exit() {
	//gExtraGameMsgLists.clear();
}

}
