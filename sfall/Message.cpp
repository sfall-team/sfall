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

static char gameLanguage[32]; // (max length of language string is 40)

const char* Message_GameLanguage() {
	return &gameLanguage[0];
}

ExtraGameMessageListsMap gExtraGameMsgLists;
static std::vector<std::string> msgFileList;

static long msgNumCounter = 0x3000;

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

void ReadExtraGameMsgFiles() {
	if (!msgFileList.empty()) {
		int number = 0;
		for (std::vector<std::string>::const_iterator it = msgFileList.begin(); it != msgFileList.end(); ++it) {
			std::string path("game\\");
			size_t n = it->find(':');
			if (n == std::string::npos) {
				path += *it;
			} else {
				path += it->substr(0, n);
				number = std::stoi(it->substr(n + 1), nullptr, 0);
			}
			path += ".msg";
			MSGList* list = new MSGList();
			if (fo_message_load(list, path.c_str()) == 1) {
				gExtraGameMsgLists.insert(std::make_pair(0x2000 + number, list));
			} else {
				delete list;
			}
			if (++number == 4096) break;
		}
		msgFileList.clear();
	}
}

long __stdcall Message_AddExtraMsgFile(const char* msgName, long msgNumber) {
	if (msgNumber) {
		if (msgNumber < 0x2000 || msgNumber > 0x2FFF) return -1;
		if (gExtraGameMsgLists.count(msgNumber)) return 0; // file has already been added
	} else if (msgNumCounter > 0x3FFF) return -3;

	std::string path("game\\");
	path += msgName;
	MSGList* list = new MSGList();
	if (!fo_message_load(list, path.c_str())) {
		// change current language folder
		//path.insert(0, "..\\english\\");
		//if (!fo_message_load(list, path.c_str())) {
			delete list;
			return -2;
		//}
	}
	if (msgNumber == 0) msgNumber = msgNumCounter++;
	gExtraGameMsgLists.insert(std::make_pair(msgNumber, list));
	return msgNumber;
}

void ClearScriptAddedExtraGameMsg() {
	for (ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end();) {
		if (it->first >= 0x3000 && it->first <= 0x3FFF) {
			fo_message_exit(it->second);
			delete it->second;
			it = gExtraGameMsgLists.erase(it);
		} else {
			++it;
		}
	}
	msgNumCounter = 0x3000;
}

void FallbackEnglishLoadMsgFiles() {
	const char* lang;
	if (fo_get_game_config_string(&lang, "system", "language")) {
		strncpy_s(gameLanguage, lang, _TRUNCATE);
		if (_stricmp(lang, "english") != 0) HookCall(0x484B18, message_load_hook);
	}
}

void ClearReadExtraGameMsgFiles() {
	for (ExtraGameMessageListsMap::iterator it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		fo_message_exit(it->second);
		delete it->second;
	}
}

void Message_Init() {
	msgFileList = GetConfigList("Misc", "ExtraGameMsgFileList", "", 512);
}

void Message_Exit() {
	//gExtraGameMsgLists.clear();
}
