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

#include <unordered_map>
#include "main.h"

//for holding a message
typedef struct MSGNode {
	long number;
	long flags;
	char *audio; //unused
	char *message;

	MSGNode() {
		number = 0;
		flags = 0;
		audio = nullptr;
		message = nullptr;
	}
} MSGNode;

//for holding msg array
typedef struct MSGList {
	long numMsgs;
	MSGNode *nodes;

	MSGList() {
		nodes = nullptr;
		numMsgs = 0;
	}
} MSGList;

typedef std::tr1::unordered_map<int, MSGList*> ExtraGameMessageListsMap;
extern ExtraGameMessageListsMap gExtraGameMsgLists;

int LoadMsgList(MSGList *msgList, const char *msgFilePath);
int DestroyMsgList(MSGList *msgList);
MSGNode *GetMsgNode(MSGList *msgList, int msgRef);
char* GetMsg(MSGList *msgList, int msgRef, int msgNum);
void ReadExtraGameMsgFiles();
void ClearReadExtraGameMsgFiles();
