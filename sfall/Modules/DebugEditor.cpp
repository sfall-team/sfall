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
#include "..\InputFuncs.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"
#include "Scripting\Arrays.h"

#include "DebugEditor.h"

namespace sfall
{

enum DECode {
	CODE_SET_GLOBAL  = 0,
	CODE_SET_MAPVAR  = 1,
	CODE_GET_CRITTER = 2,
	CODE_SET_CRITTER = 3,
	CODE_SET_SGLOBAL = 4,
	CODE_GET_PROTO   = 5,
	CODE_SET_PROTO   = 6,
	CODE_GET_PLAYER  = 7,
	CODE_SET_PLAYER  = 8,
	CODE_GET_ARRAY   = 9,
	CODE_SET_ARRAY   = 10,
	CODE_EXIT        = 254
};

static const char* debugLog = "LOG";
static const char* debugGnw = "GNW";

static DWORD debugEditorKey = 0;

struct sArray {
	DWORD id;
	long  isMap;
	long  size;
	long  flag;
};

static void DEGameWinRedraw() {
	fo::func::process_bk();
}

static bool SetBlocking(SOCKET s, bool block) {
	DWORD d = !block;
	ioctlsocket(s, FIONBIO, &d);
}

static bool InternalSend(SOCKET s, const void* _data, int size) {
	const char* data = (const char*)_data;
	int upto = 0;
	while (upto < size) {
		int tmp = send(s, &data[upto], size - upto, 0);
		if (tmp > 0) {
			upto += tmp;
		} else {
			DWORD d = WSAGetLastError();
			if (d != WSAEWOULDBLOCK && d != WSAENOBUFS) return true;
		}
	}
	return false;
}

static bool InternalRecv(SOCKET s, void* _data, int size) {
	char* data = (char*)_data;
	int upto = 0;
	while (upto < size) {
		int tmp = recv(s, &data[upto], size - upto, 0);
		if (tmp > 0) {
			upto += tmp;
		} else {
			DWORD d = WSAGetLastError();
			if (d != WSAEWOULDBLOCK && d != WSAENOBUFS) return true;
		}
	}
	return false;
}

static void RunEditorInternal(SOCKET &s) {
	*(DWORD*)FO_VAR_script_engine_running = 0;

	std::vector<DWORD*> vec = std::vector<DWORD*>();
	for (int elv = 0; elv < 3; elv++) {
		for (int tile = 0; tile < 40000; tile++) {
			fo::GameObject* obj = fo::func::obj_find_first_at_tile(elv, tile);
			while (obj) {
				if ((obj->Type()) == fo::OBJ_TYPE_CRITTER) {
					vec.push_back(reinterpret_cast<DWORD*>(obj));
				}
				obj = fo::func::obj_find_next_at_tile();
			}
		}
	}
	int numCritters = vec.size();
	int numGlobals = fo::var::num_game_global_vars;
	int numMapVars = fo::var::num_map_global_vars;
	int numSGlobals = GetNumGlobals();
	int numArrays = script::GetNumArrays();

	InternalSend(s, &numGlobals, 4);
	InternalSend(s, &numMapVars, 4);
	InternalSend(s, &numSGlobals, 4);
	InternalSend(s, &numArrays, 4);
	InternalSend(s, &numCritters, 4);

	GlobalVar* sglobals = new GlobalVar[numSGlobals];
	GetGlobals(sglobals);

	sArray* arrays = new sArray[numArrays];
	script::GetArrays((int*)arrays);

	InternalSend(s, reinterpret_cast<void*>(fo::var::game_global_vars), 4 * numGlobals);
	InternalSend(s, reinterpret_cast<void*>(fo::var::map_global_vars), 4 * numMapVars);
	InternalSend(s, sglobals, sizeof(GlobalVar)*numSGlobals);
	InternalSend(s, arrays, numArrays * sizeof(sArray));
	for (int i = 0; i < numCritters; i++) {
		InternalSend(s, &vec[i][25], 4);
		InternalSend(s, &vec[i], 4);
	}

	while (true) {
		BYTE code;
		InternalRecv(s, &code, 1);
		if (code == CODE_EXIT) break;
		int id, val;
		switch (code) {
		case CODE_SET_GLOBAL:
			InternalRecv(s, &id, 4);
			InternalRecv(s, &val, 4);
			fo::var::game_global_vars[id] = val;
			break;
		case CODE_SET_MAPVAR:
			InternalRecv(s, &id, 4);
			InternalRecv(s, &val, 4);
			fo::var::map_global_vars[id] = val;
			break;
		case CODE_GET_CRITTER:
			InternalRecv(s, &id, 4);
			InternalSend(s, vec[id], 0x84);
			break;
		case CODE_SET_CRITTER:
			InternalRecv(s, &id, 4);
			InternalRecv(s, vec[id], 0x84);
			break;
		case CODE_SET_SGLOBAL:
			InternalRecv(s, &id, 4);
			InternalRecv(s, &val, 4);
			sglobals[id].val = val;
			break;
		case CODE_GET_ARRAY: // get array values
			{
				InternalRecv(s, &id, 4);
				DWORD *types = new DWORD[arrays[id].size * 2]; // type, len
				script::DEGetArray(arrays[id].id, types, nullptr);
				int dataLen = 0;
				for (long i = 0; i < arrays[id].size; i++) {
					dataLen += types[i * 2 + 1];
				}
				char *data = new char[dataLen];
				script::DEGetArray(arrays[id].id, nullptr, data);
				InternalSend(s, types, arrays[id].size * 8);
				InternalSend(s, data, dataLen);
				delete[] data;
				delete[] types;
			}
			break;
		case CODE_SET_ARRAY: // set array values
			{
				InternalRecv(s, &id, 4);
				InternalRecv(s, &val, 4); // len data
				char *data = new char[val];
				InternalRecv(s, data, val);
				script::DESetArray(arrays[id].id, nullptr, data);
				delete[] data;
			}
			break;
		}
		DEGameWinRedraw();
	}

	SetGlobals(sglobals);
	delete[] sglobals;
	delete[] arrays;

	FlushInputBuffer();
	*(DWORD*)FO_VAR_script_engine_running = 1;
}

void RunDebugEditor() {
	WSADATA wsaData;
	SOCKET sock, client;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) return;
	// create the socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		WSACleanup();
		return;
	}
	// bind the socket
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr("127.0.0.1");
	service.sin_port = htons(4245);

	if (bind(sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return;
	}
	if (listen(sock, 4) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return;
	}

	// Start up the editor
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	si.cb = sizeof(si);

	if (!CreateProcessA("FalloutClient.exe", "FalloutClient.exe -debugedit", 0, 0, false, 0, 0, 0, &si, &pi)) {
		closesocket(sock);
		WSACleanup();
		return;
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	// Connect to the editor
	client = accept(sock, 0, 0);
	if (client == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return;
	}

	RunEditorInternal(client);

	closesocket(client);
	closesocket(sock);
	WSACleanup();
}

static void __declspec(naked) dbg_error_hack() {
	__asm {
		cmp  ebx, 1;
		je   hide;
		sub  esp, 0x104;
		retn;
hide:
		add  esp, 4; // destroy this return addr
		pop  esi;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) win_debug_hook() {
	__asm {
		call fo::funcoffs::debug_log_;
		xor  eax, eax;
		cmp  ds:[FO_VAR_GNW_win_init_flag], eax;
		retn;
	}
}

void DebugModePatch() {
	DWORD dbgMode = GetPrivateProfileIntA("Debugging", "DebugMode", 0, ::sfall::ddrawIni);
	if (dbgMode) {
		dlog("Applying debugmode patch.", DL_INIT);
		// If the player is using an exe with the debug patch already applied, just skip this block without erroring
		if (*((DWORD*)0x444A64) != 0x082327E8) {
			SafeWrite32(0x444A64, 0x082327E8); // call debug_register_env_
			SafeWrite32(0x444A68, 0x0120E900); // jmp  0x444B8E
			SafeWrite8(0x444A6D, 0);
			SafeWrite32(0x444A6E, 0x90909090);
		}
		SafeWrite8(0x4C6D9B, 0xB8);            // mov  eax, GNW/LOG
		if (dbgMode & 2) {
			SafeWrite32(0x4C6D9C, (DWORD)debugLog);
			if (dbgMode & 1) {
				SafeWrite16(0x4C6E75, 0x66EB); // jmps 0x4C6EDD
				SafeWrite8(0x4C6EF2, 0xEB);
				SafeWrite8(0x4C7034, 0x0);
				MakeCall(0x4DC319, win_debug_hook, 2);
			}
		} else {
			SafeWrite32(0x4C6D9C, (DWORD)debugGnw);
		}
		dlogr(" Done", DL_INIT);
	}
}

void DontDeleteProtosPatch() {
	if (GetPrivateProfileIntA("Debugging", "DontDeleteProtos", 0, ::sfall::ddrawIni)) {
		dlog("Applying permanent protos patch.", DL_INIT);
		SafeWrite8(0x48007E, 0xEB);
		dlogr(" Done", DL_INIT);
	}
}

void DebugEditor::init() {
	if (!isDebug) return;
	DebugModePatch();
	DontDeleteProtosPatch();

	debugEditorKey = GetConfigInt("Input", "DebugEditorKey", 0);
	if (debugEditorKey != 0) {
		OnKeyPressed() += [](DWORD scanCode, bool pressed) {
			if (scanCode == debugEditorKey && pressed && IsMapLoaded()) {
				RunDebugEditor();
			}
		};
	}

	if (GetPrivateProfileIntA("Debugging", "HideObjIsNullMsg", 0, ::sfall::ddrawIni)) {
		MakeCall(0x453FD2, dbg_error_hack, 1);
	}
}

}
