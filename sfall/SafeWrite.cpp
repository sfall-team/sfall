#include "main.h"

#pragma warning(disable:4996)

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
}

void MakeCall(DWORD addr, void* func) {
	SafeWrite8(addr, 0xE8);
	HookCall(addr, func);
}

void MakeCall(DWORD addr, void* func, int len) {
	SafeMemSet(addr + 5, 0x90, len);
	MakeCall(addr, func);
}

void MakeJump(DWORD addr, void* func) {
	SafeWrite8(addr, 0xE9);
	HookCall(addr, func);
}

void MakeJump(DWORD addr, void* func, int len) {
	SafeMemSet(addr + 5, 0x90, len);
	MakeJump(addr, func);
}

void SafeMemSet(DWORD addr, BYTE val, int len) {
	DWORD	oldProtect;

	VirtualProtect((void *)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memset((void*)addr, val, len);
	VirtualProtect((void *)addr, len, oldProtect, &oldProtect);
}

void BlockCall(DWORD addr) {
	SafeMemSet(addr, 0x90, 5);
}
