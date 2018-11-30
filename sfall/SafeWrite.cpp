#ifndef NDEBUG
#include <list>
#endif

#include "main.h"

#pragma warning(disable:4996)

namespace sfall 
{

enum CodeType : BYTE {
	Call = 0xE8,
	Jump = 0xE9,
	Nop =  0x90
};

#ifndef NDEBUG
std::list<long> writeAddress;

static void CheckConflict(DWORD addr) {
	bool exist = false;
	for (const auto &wa : writeAddress) {
		if (addr == wa) {
			exist = true;
			char buf[256];
			sprintf_s(buf, "Memory writing conflict at address 0x%x. The address has already been overwritten by other code.", addr);
			MessageBoxA(0, buf, "Conflict Detected", MB_TASKMODAL);
		}
	}
	if (!exist) writeAddress.push_back(addr);
}
#endif

static void _stdcall SafeWriteFunc(BYTE code, DWORD addr, void* func) {
	DWORD oldProtect, data = (DWORD)func - (addr + 5);

	VirtualProtect((void *)addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((BYTE*)addr) = code;
	*((DWORD*)(addr + 1)) = data;
	VirtualProtect((void *)addr, 5, oldProtect, &oldProtect);

	#ifndef NDEBUG
		CheckConflict(addr);
	#endif
}

static _declspec(noinline) void _stdcall SafeWriteFunc(BYTE code, DWORD addr, void* func, DWORD len) {
	DWORD oldProtect,
		protectLen = len + 5,
		addrMem = addr + 5,
		data = (DWORD)func - addrMem;

	VirtualProtect((void *)addr, protectLen, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((BYTE*)addr) = code;
	*((DWORD*)(addr + 1)) = data;

	for (unsigned int i = 0; i < len; i++) {
		*((BYTE*)(addrMem + i)) = CodeType::Nop;
	}
	VirtualProtect((void *)addr, protectLen, oldProtect, &oldProtect);

	#ifndef NDEBUG
		CheckConflict(addr);
	#endif
}

void SafeWriteBytes(DWORD addr, BYTE* data, int count) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, count, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((void*)addr, data, count);
	VirtualProtect((void *)addr, count, oldProtect, &oldProtect);
}

void _stdcall SafeWrite8(DWORD addr, BYTE data) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((BYTE*)addr) = data;
	VirtualProtect((void *)addr, 1, oldProtect, &oldProtect);
}

void _stdcall SafeWrite16(DWORD addr, WORD data) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, 2, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((WORD*)addr) = data;
	VirtualProtect((void *)addr, 2, oldProtect, &oldProtect);
}

void _stdcall SafeWrite32(DWORD addr, DWORD data) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((DWORD*)addr) = data;
	VirtualProtect((void *)addr, 4, oldProtect, &oldProtect);
}

void _stdcall SafeWriteStr(DWORD addr, const char* data) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, strlen(data) + 1, PAGE_EXECUTE_READWRITE, &oldProtect);
	strcpy((char *)addr, data);
	VirtualProtect((void *)addr, strlen(data) + 1, oldProtect, &oldProtect);
}

void HookCall(DWORD addr, void* func) {
	SafeWrite32(addr + 1, (DWORD)func - (addr + 5));
	#ifndef NDEBUG
		CheckConflict(addr);
	#endif
}

void MakeCall(DWORD addr, void* func) {
	SafeWriteFunc(CodeType::Call, addr, func);
}

void MakeCall(DWORD addr, void* func, int len) {
	SafeWriteFunc(CodeType::Call, addr, func, len);
}

void MakeJump(DWORD addr, void* func) {
	SafeWriteFunc(CodeType::Jump, addr, func);
}

void MakeJump(DWORD addr, void* func, int len) {
	SafeWriteFunc(CodeType::Jump, addr, func, len);
}

void HookCalls(void* func, std::initializer_list<DWORD> addrs) {
	for (auto addr : addrs) {
		HookCall(addr, func);
	}
}

void MakeCalls(void* func, std::initializer_list<DWORD> addrs) {
	for (auto addr : addrs) {
		MakeCall(addr, func);
	}
}

void SafeMemSet(DWORD addr, BYTE val, int len) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memset((void*)addr, val, len);
	VirtualProtect((void *)addr, len, oldProtect, &oldProtect);
}

void BlockCall(DWORD addr) {
	SafeMemSet(addr, CodeType::Nop, 5);
	#ifndef NDEBUG
		CheckConflict(addr);
	#endif
}

}
