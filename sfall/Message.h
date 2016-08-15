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
	DWORD ref;
	DWORD a;
	char *msg1; //unused
	char *msg2;

	MSGNode() {
		ref = 0;
		a = 0;
		msg1 = NULL;
		msg2 = NULL;
	}
} MSGNode;

//for holding msg array
typedef struct MSGList {
	long numMsgs;
	void *MsgNodes;

	MSGList() {
		MsgNodes = NULL;
		numMsgs = 0;
	}
} MSGList;

extern std::tr1::unordered_map<int, MSGList*> gExtraGameMsgLists;

int LoadMsgList(MSGList *MsgList, char *MsgFilePath);
int DestroyMsgList(MSGList *MsgList);
//bool GetMsg(MSGList *MsgList, MSGNode *MsgNode, DWORD msgRef);
MSGNode *GetMsgNode(MSGList *MsgList, DWORD msgRef);
char* GetMsg(MSGList *MsgList, DWORD msgRef, int msgNum);
void ReadExtraGameMsgFiles();
void ClearReadExtraGameMsgFiles();
