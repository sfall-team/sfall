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

#define MSG_FILE_COMBAT		(0x56D368)
#define MSG_FILE_AI			(0x56D510)
#define MSG_FILE_SCRNAME	(0x56D754)
#define MSG_FILE_MISC		(0x58E940)
#define MSG_FILE_CUSTOM		(0x58EA98)
#define MSG_FILE_INVENTRY	(0x59E814)
#define MSG_FILE_ITEM		(0x59E980)
#define MSG_FILE_LSGAME		(0x613D28)
#define MSG_FILE_MAP		(0x631D48)
#define MSG_FILE_OPTIONS	(0x6637E8)
#define MSG_FILE_PERK		(0x6642D4)
#define MSG_FILE_PIPBOY		(0x664348)
#define MSG_FILE_QUESTS		(0x664410)
#define MSG_FILE_PROTO		(0x6647FC)
#define MSG_FILE_SCRIPT		(0x667724)
#define MSG_FILE_SKILL		(0x668080)
#define MSG_FILE_SKILLDEX	(0x6680F8)
#define MSG_FILE_STAT		(0x66817C)
#define MSG_FILE_TRAIT		(0x66BE38)
#define MSG_FILE_WORLDMAP	(0x672FB0)

typedef std::unordered_map<int, std::unique_ptr<fo::MessageList>> ExtraGameMessageListsMap;
extern ExtraGameMessageListsMap gExtraGameMsgLists;
extern const fo::MessageList* gameMsgFiles[];

class Message : public Module {
public:
	const char* name() { return "Message"; }
	void init();
	void exit() override;

	static long AddExtraMsgFile(const char* msgName, long msgNumber);
};

fo::MessageNode* GetMsgNode(fo::MessageList* msgList, int msgRef);
char* GetMsg(fo::MessageList* msgList, int msgRef, int msgNum);

}
