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

std::tr1::unordered_map<int, MSGList*> gExtraGameMsgLists;

int LoadMsgList(MSGList *MsgList, char *MsgFilePath) {
	int retVal;
	__asm {
		mov edx, MsgFilePath
		mov eax, MsgList
		call message_load_
		mov retVal, eax
	}
	return retVal;
}

int DestroyMsgList(MSGList *MsgList) {
	int retVal;
	__asm {
		mov eax, MsgList
		call message_exit_
		mov retVal, eax
	}
	return retVal;
}

/*
bool GetMsg(MSGList *MsgList, MSGNode *MsgNode, DWORD msgRef) {
	bool retVal=FALSE;
	MsgNode->ref=msgRef;

	__asm {
		mov edx, MsgNode
		mov eax, MsgList
		call message_search_
		cmp eax, 1
		jne EndFunc
		mov retVal, 1
EndFunc:
	}
	return retVal;
}
*/

MSGNode *GetMsgNode(MSGList *MsgList, DWORD msgRef) {

	if (MsgList == NULL) return NULL;
	if (MsgList->numMsgs <= 0) return NULL;

	MSGNode *MsgNode = (MSGNode*)MsgList->MsgNodes;

	long last = MsgList->numMsgs - 1;
	long first = 0;
	long mid;

	//Use Binary Search to find msg
	while (first <= last) {
		mid = (first + last) / 2;
		if (msgRef > MsgNode[mid].ref)
			first = mid + 1;
		else if (msgRef < MsgNode[mid].ref)
			last = mid - 1;
		else
			return &MsgNode[mid];
	}

	return NULL;
}

char* GetMsg(MSGList *MsgList, DWORD msgRef, int msgNum) {
	MSGNode *MsgNode = GetMsgNode(MsgList, msgRef);
	if (MsgNode) {
		if (msgNum == 2)
			return MsgNode->msg2;
		else if (msgNum == 1)
			return MsgNode->msg1;
	}
	return NULL;
}

void ReadExtraGameMsgFiles() {
	int read;
	std::string names;

	names.resize(256);

	while ((read = GetPrivateProfileStringA("Misc", "ExtraGameMsgFileList", "",
		(LPSTR)names.data(), names.size(), ".\\ddraw.ini")) == names.size() - 1)
		names.resize(names.size() + 256);

	if (names.empty())
		return;

	names.resize(names.find_first_of('\0'));
	names.append(",");

	int begin = 0;
	int end;
	int length;

	while ((end = names.find_first_of(',', begin)) != std::string::npos) {
		length = end - begin;

		if (length > 0) {
			std::string path = "game\\" + names.substr(begin, length) + ".msg";
			MSGList* list = new MSGList;

			if (LoadMsgList(list, (char*)path.data()) == 1)
				gExtraGameMsgLists.insert(std::make_pair(0x2000 + gExtraGameMsgLists.size(), list));
			else
				delete list;
		}

		begin = end + 1;
	}
}

void ClearReadExtraGameMsgFiles() {
	std::tr1::unordered_map<int, MSGList*>::iterator it;

	for (it = gExtraGameMsgLists.begin(); it != gExtraGameMsgLists.end(); ++it) {
		DestroyMsgList(it->second);
		delete it->second;
	}

	gExtraGameMsgLists.clear();
}
