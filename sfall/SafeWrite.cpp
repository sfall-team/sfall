#include "SafeWrite.h"

namespace sfall
{

static void __stdcall SafeWriteFunc(BYTE code, DWORD addr, void* func) {
	DWORD oldProtect, data = (DWORD)func - (addr + 5);

	VirtualProtect((void*)addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((BYTE*)addr) = code;
	*((DWORD*)(addr + 1)) = data;
	VirtualProtect((void*)addr, 5, oldProtect, &oldProtect);

	CheckConflict(addr, 5);
}

static __declspec(noinline) void __stdcall SafeWriteFunc(BYTE code, DWORD addr, void* func, size_t len) {
	DWORD oldProtect,
		protectLen = len + 5,
		addrMem = addr + 5,
		data = (DWORD)func - addrMem;

	VirtualProtect((void*)addr, protectLen, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((BYTE*)addr) = code;
	*((DWORD*)(addr + 1)) = data;

	do {
		*((BYTE*)addrMem++) = CodeType::Nop;
	} while (--len);
	VirtualProtect((void*)addr, protectLen, oldProtect, &oldProtect);

	CheckConflict(addr, protectLen);
}

void SafeWriteBytes(DWORD addr, BYTE* data, size_t count) {
	DWORD oldProtect;

	VirtualProtect((void*)addr, count, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((void*)addr, data, count);
	VirtualProtect((void*)addr, count, oldProtect, &oldProtect);

	AddrAddToList(addr, count);
}

void __stdcall SafeWrite8(DWORD addr, BYTE data) {
	DWORD oldProtect;

	VirtualProtect((void*)addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((BYTE*)addr) = data;
	VirtualProtect((void*)addr, 1, oldProtect, &oldProtect);

	AddrAddToList(addr, 1);
}

void __stdcall SafeWrite16(DWORD addr, WORD data) {
	DWORD oldProtect;

	VirtualProtect((void*)addr, 2, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((WORD*)addr) = data;
	VirtualProtect((void*)addr, 2, oldProtect, &oldProtect);

	AddrAddToList(addr, 2);
}

void __stdcall SafeWrite32(DWORD addr, DWORD data) {
	DWORD oldProtect;

	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((DWORD*)addr) = data;
	VirtualProtect((void*)addr, 4, oldProtect, &oldProtect);

	AddrAddToList(addr, 4);
}

void __stdcall SafeWriteStr(DWORD addr, const char* data) {
	DWORD oldProtect;
	long len = strlen(data) + 1;

	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	strcpy((char*)addr, data);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);

	AddrAddToList(addr, len);
}

void HookCall(DWORD addr, void* func) {
	SafeWrite32(addr + 1, (DWORD)func - (addr + 5));

	CheckConflict(addr, 1);
}

void MakeCall(DWORD addr, void* func) {
	SafeWriteFunc(CodeType::Call, addr, func);
}

void MakeCall(DWORD addr, void* func, size_t len) {
	SafeWriteFunc(CodeType::Call, addr, func, len);
}

void MakeJump(DWORD addr, void* func) {
	SafeWriteFunc(CodeType::Jump, addr, func);
}

void MakeJump(DWORD addr, void* func, size_t len) {
	SafeWriteFunc(CodeType::Jump, addr, func, len);
}

void HookCalls(void* func, std::initializer_list<DWORD> addrs) {
	for (auto& addr : addrs) {
		HookCall(addr, func);
	}
}

void MakeCalls(void* func, std::initializer_list<DWORD> addrs) {
	for (auto& addr : addrs) {
		MakeCall(addr, func);
	}
}

void MakeJumps(void* func, std::initializer_list<DWORD> addrs) {
	for (auto& addr : addrs) {
		MakeJump(addr, func);
	}
}

void SafeMemSet(DWORD addr, BYTE val, size_t len) {
	DWORD oldProtect;

	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memset((void*)addr, val, len);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);

	AddrAddToList(addr, len);
}

void BlockCall(DWORD addr) {
	DWORD oldProtect;

	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((DWORD*)addr) = 0x00441F0F; // long NOP (0F1F4400-XX)
	VirtualProtect((void*)addr, 4, oldProtect, &oldProtect);

	CheckConflict(addr, 5);
}

}
