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
#include "Graphics.h"
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
	CODE_GET_LOCVARS = 11,
	CODE_SET_LOCVARS = 12,
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
	if (Graphics::mode != 0) fo::func::process_bk();
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
				if (obj->IsCritter()) {
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
		case CODE_GET_LOCVARS:
			{
				InternalRecv(s, &id, 4); // sid
				val = fo::GetScriptLocalVars(id);
				InternalSend(s, &val, 4);
				if (val) {
					std::vector<int> values(val);
					long varVal;
					for (int i = 0; i < val; i++) {
						fo::func::scr_get_local_var(id, i, &varVal);
						values[i] = varVal;
					}
					InternalSend(s, values.data(), val * 4);
				}
			}
			break;
		case CODE_SET_LOCVARS:
			{
				InternalRecv(s, &id, 4);  // sid
				InternalRecv(s, &val, 4); // len data
				std::vector<int> values(val);
				InternalRecv(s, values.data(), val * 4);
				for (int i = 0; i < val; i++) {
					fo::func::scr_set_local_var(id, i, values[i]);
				}
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

	if (!CreateProcessA("FalloutDebug.exe", "FalloutDebug.exe -debugedit", 0, 0, false, 0, 0, 0, &si, &pi)) {
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

	MouseDeviceUnacquire(true);
	RunEditorInternal(client);

	closesocket(client);
	closesocket(sock);
	WSACleanup();
}

static void __declspec(naked) dbg_error_hack() {
	static const DWORD dbg_error_ret = 0x453FD8;
	__asm {
		cmp  ebx, 1;
		je   hide;
		sub  esp, 0x104;
		jmp  dbg_error_ret;
hide:
		pop  esi;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) art_data_size_hook() {
	static char* artDbgMsg = "\nERROR: File not found: %s\n";
	__asm {
		test edi, edi;
		jz   artNotExist;
		retn;
artNotExist:
		mov  edx, [esp + 0x124 - 0x1C + 4]; // filename
		push edx;
		push artDbgMsg;
		call fo::funcoffs::debug_printf_;
		cmp  isDebug, 0;
		jne  display;
		add  esp, 8;
		retn;
display:
		push edx; // filename
		push artDbgMsg;
		lea  eax, [esp + 0x124 - 0x124 + 20]; // buf
		push eax;
		call fo::funcoffs::sprintf_;
		add  esp, 20;
		lea  eax, [esp + 4];
		jmp  fo::funcoffs::display_print_;
	}
}

static void __declspec(naked) proto_load_pid_hack() {
	static const char* proDbgMsg = "\nERROR reading prototype file: %s\n";
	__asm {
		mov  dword ptr [esp + 0x120 - 0x1C + 4], -1;
		lea  eax, [esp + 0x120 - 0x120 + 4]; // pro file
		push eax;
		push proDbgMsg;
		call fo::funcoffs::debug_printf_;
		add  esp, 8;
		mov  eax, 0x500494; // 'iisxxxx1'
		jmp  fo::funcoffs::gsound_play_sfx_file_;
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

static void __declspec(naked) debug_log_hack() {
	__asm {
		push eax;      // text
		push 0x5016EC; // fmt '%s'
		push ebx;      // log file
		call fo::funcoffs::fprintf_;
		add  esp, 12;
		retn;
	}
}

const char* scrNameFmt = "\nScript: %s ";

static void __declspec(naked) debugMsg() {
	__asm {
		mov  edx, ds:[FO_VAR_currentProgram];
		push [edx]; // script name
		push scrNameFmt;
		call fo::funcoffs::debug_printf_;
		add  esp, 8;
		jmp  fo::funcoffs::debug_printf_; // ERROR: attempt to reference...
	}
}

// Shifts the string one character to the right and inserts a newline control character at the beginning
static void MoveDebugString(char* messageAddr) {
	int i = 0;
	do {
		messageAddr[i + 1] = messageAddr[i];
		i--;
	} while (messageAddr[i] != '\0');
	messageAddr[i + 1] = '\n';
}

static const DWORD addrDotChar[] = {
	0x50B244, 0x50B27C, 0x50B2B6, 0x50B2EE // "ERROR: attempt to reference * var out of range: %d"
};

static const DWORD addrNewLineChar[] = {
	0x500A64, // "Friendly was in the way!"
};

static void DebugModePatch() {
	int dbgMode = IniReader::GetIntDefaultConfig("Debugging", "DebugMode", 0);
	if (dbgMode > 0) {
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
				SafeWrite8(0x4C6EF2, CodeType::JumpShort);
				SafeWrite8(0x4C7034, 0x0);
				MakeCall(0x4DC319, win_debug_hook, 2);
			}
		} else {
			SafeWrite32(0x4C6D9C, (DWORD)debugGnw);
		}
		if (IniReader::GetIntDefaultConfig("Debugging", "HideObjIsNullMsg", 0)) {
			MakeJump(0x453FD2, dbg_error_hack);
		}
		// prints a debug message about a missing critter art file to both debug.log and the message window in sfall debugging mode
		HookCall(0x419B65, art_data_size_hook);

		// Fix to prevent crashes when there is a '%' character in the printed message
		if (dbgMode > 1) {
			MakeCall(0x4C703F, debug_log_hack);
			BlockCall(0x4C7044); // just nop code
		}
		// replace calling debug_printf_ with _debug_func
		__int64 data = 0x51DF0415FFF08990; // mov eax, esi; call ds:_debug_func
		SafeWriteBytes(0x455419, (BYTE*)&data, 8); // op_display_msg_

		// set the position of the debug window
		SafeWrite8(0x4DC34D, 15);

		// Fix the format of some debug messages
		SafeWriteBatch<BYTE>('\n', addrNewLineChar);
		SafeWriteBatch<BYTE>('.', addrDotChar);
		HookCalls(debugMsg, {
			0x482240, // map_set_global_var_
			0x482274, // map_get_global_var_
			0x4822A0, // map_set_local_var_
			0x4822D4  // map_get_local_var_
		});
		if (dbgMode != 1) {
			MoveDebugString((char*)0x500A9B); // "computing attack..."
		}
		dlogr(" Done", DL_INIT);
	}
}

static void DontDeleteProtosPatch() {
	if (IniReader::GetIntDefaultConfig("Debugging", "DontDeleteProtos", 0)) {
		dlog("Applying permanent protos patch.", DL_INIT);
		SafeWrite8(0x48007E, CodeType::JumpShort);
		dlogr(" Done", DL_INIT);
	}
}

/*
void CheckTimerResolution() {
	DWORD ticksList[50];
	DWORD old_ticks = GetTickCount();
	for (size_t i = 0; i < 50;) {
		DWORD ticks = GetTickCount();
		if (ticks != old_ticks) {
			old_ticks = ticks;
			ticksList[i++] = ticks;
		}
	}
	int min = 100, max = 0;
	for (size_t i = 0; i < 49; i++) {
		int ms = ticksList[i + 1] - ticksList[i];
		if (ms < min) min = ms;
		if (ms > max) max = ms;
	}
	fo::func::debug_printf("System timer resolution: %d - %d ms.\n", min, max);
}
*/

void DebugEditor::init() {
	DebugModePatch();

	// Notifies and prints a debug message about a corrupted proto file to debug.log
	MakeCall(0x4A1D73, proto_load_pid_hack, 6);

	if (!isDebug) return;
	DontDeleteProtosPatch();

	debugEditorKey = GetConfigInt("Input", "DebugEditorKey", 0);
	if (debugEditorKey != 0) {
		OnKeyPressed() += [](DWORD scanCode, bool pressed) {
			if (scanCode == debugEditorKey && pressed && IsGameLoaded()) {
				RunDebugEditor();
			}
		};
	}
}

}
