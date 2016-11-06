/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include <vector>

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "DebugEditor.h"
#include "ScriptExtender.h"
#include "Scripting\Arrays.h"

#define CODE_EXIT (254)
#define CODE_SET_GLOBAL  (0)
#define CODE_SET_MAPVAR  (1)
#define CODE_GET_CRITTER (2)
#define CODE_SET_CRITTER (3)
#define CODE_SET_SGLOBAL (4)
#define CODE_GET_PROTO   (5)
#define CODE_SET_PROTO   (6)
#define CODE_GET_PLAYER  (7)
#define CODE_SET_PLAYER  (8)
#define CODE_GET_ARRAY   (9)
#define CODE_SET_ARRAY   (10)

static bool SetBlocking(SOCKET s, bool block) {
	DWORD d=!block;
	ioctlsocket(s, FIONBIO, &d);
}
static bool InternalSend(SOCKET s, const void* _data, int size) {
	const char* data=(const char*) _data;
	int upto=0;
	int tmp;
	DWORD d;
	while(upto<size) {
		tmp=send(s, &data[upto], size-upto, 0);
		if(tmp>0) upto+=tmp;
		else {
			d=WSAGetLastError();
			if(d!=WSAEWOULDBLOCK && d!=WSAENOBUFS) return true;
		}
	}
	return false;
}
static bool InternalRecv(SOCKET s, void* _data, int size) {
	char* data=(char*)_data;
	int upto=0;
	int tmp;
	DWORD d;
	while(upto<size) {
		tmp=recv(s, &data[upto], size-upto, 0);
		if(tmp>0) upto+=tmp;
		else {
			d=WSAGetLastError();
			if(d!=WSAEWOULDBLOCK && d!=WSAENOBUFS) return true;
		}
	}
	return false;
}
static void RunEditorInternal(SOCKET &s) {
	std::vector<DWORD*> vec = std::vector<DWORD*>();
	for(int elv=0;elv<3;elv++) {
		for(int tile=0;tile<40000;tile++) {
			DWORD* obj;
			__asm {
				mov edx, tile;
				mov eax, elv;
				call FuncOffs::obj_find_first_at_tile_;
				mov obj, eax;
			}
			while(obj) {
				DWORD otype = obj[25];
				otype = (otype&0xff000000) >> 24;
				if(otype==1) vec.push_back(obj);
				__asm {
					call FuncOffs::obj_find_next_at_tile_;
					mov obj, eax;
				}
			}
		}
	}

	int numCritters=vec.size();

	int numGlobals = *VarPtr::num_game_global_vars;
	int numMapVars = *VarPtr::num_map_global_vars;
	int numSGlobals = GetNumGlobals();
	int numArrays = GetNumArrays();
	InternalSend(s, &numGlobals, 4);
	InternalSend(s, &numMapVars, 4);
	InternalSend(s, &numSGlobals, 4);
	InternalSend(s, &numArrays, 4);
	InternalSend(s, &numCritters, 4);

	sGlobalVar* sglobals=new sGlobalVar[numSGlobals];
	GetGlobals(sglobals);
	int* arrays=new int[numArrays*3];
	GetArrays(arrays);

	InternalSend(s, *reinterpret_cast<void**>(VarPtr::game_global_vars), 4*numGlobals);
	InternalSend(s, *reinterpret_cast<void**>(VarPtr::map_global_vars), 4*numMapVars);
	InternalSend(s, sglobals, sizeof(sGlobalVar)*numSGlobals);
	InternalSend(s, arrays, numArrays*3*4);
	for(int i=0;i<numCritters;i++) InternalSend(s, &vec[i][25], 4);

	while(true) {
		BYTE code;
		InternalRecv(s, &code, 1);
		if(code==CODE_EXIT) break;
		int id, val;
		switch(code) {
			case 0:
				InternalRecv(s, &id, 4);
				InternalRecv(s, &val, 4);
				(*VarPtr::game_global_vars)[id] = val;
				break;
			case 1:
				InternalRecv(s, &id, 4);
				InternalRecv(s, &val, 4);
				(*VarPtr::map_global_vars)[id] = val;
				break;
			case 2:
				InternalRecv(s, &id, 4);
				InternalSend(s, vec[id], 0x74);
				break;
			case 3:
				InternalRecv(s, &id, 4);
				InternalRecv(s, vec[id], 0x74);
				break;
			case 4:
				InternalRecv(s, &id, 4);
				InternalRecv(s, &val, 4);
				sglobals[id].val=val;
				break;
			case 9:
				{
				InternalRecv(s, &id, 4);
				DWORD *types=new DWORD[arrays[id*3+1]];
				char *data=new char[arrays[id*3+1]*arrays[id*3+2]];
				DEGetArray(arrays[id*3], types, data);
				InternalSend(s, types, arrays[id*3+1]*4);
				InternalSend(s, data, arrays[id*3+1]*arrays[id*3+2]);
				delete[] data;
				delete[] types;
				}
				break;
			case 10:
				{
				InternalRecv(s, &id, 4);
				char *data=new char[arrays[id*3+1]*arrays[id*3+2]];
				InternalRecv(s, data, arrays[id*3+1]*arrays[id*3+2]);
				DESetArray(arrays[id*3], 0, data);
				delete[] data;
				}
				break;
		}
	}

	SetGlobals(sglobals);
	delete[] sglobals;
	delete[] arrays;
}

void RunDebugEditor() {
	WSADATA wsaData;
	SOCKET sock, client;

	if(WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR) return;
	//create the socket
	sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock == INVALID_SOCKET) {
		WSACleanup();
		return;
	}
	//bind the socket
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(4245);

	if(bind(sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return;
	}
	if(listen(sock, 4) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return;
	}

	//Start up the editor
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	si.cb=sizeof(si);

	if(!CreateProcessA("FalloutClient.exe", "FalloutClient.exe -debugedit", 0, 0, false, 0, 0, 0, &si, &pi)) {
		closesocket(sock);
		WSACleanup();
		return;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	//Connect to the editor
	client=accept(sock, 0, 0);
	if(client == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return;
	}

	RunEditorInternal(client);

	closesocket(client);
	closesocket(sock);
	WSACleanup();
}