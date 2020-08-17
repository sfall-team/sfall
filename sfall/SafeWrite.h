#pragma once

enum CodeType : BYTE {
	CODETYPE_Ret       = 0xC3,
	CODETYPE_Call      = 0xE8,
	CODETYPE_Jump      = 0xE9,
	CODETYPE_Nop       = 0x90,
	CODETYPE_JumpShort = 0xEB, // 0xEB [jmp short ...]
	CODETYPE_JumpNZ    = 0x75, // 0x75 [jnz short ...]
	CODETYPE_JumpZ     = 0x74, // 0x74 [jz  short ...]
};

template <typename T>
void __stdcall SafeWrite(DWORD addr, T data) {
	DWORD oldProtect;
	VirtualProtect((void*)addr, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
	*((T*)addr) = data;
	VirtualProtect((void*)addr, sizeof(T), oldProtect, &oldProtect);
}

template <typename T, class ForwardIteratorType>
void __stdcall SafeWriteBatch(T data, ForwardIteratorType begin, ForwardIteratorType end) {
	for (ForwardIteratorType it = begin; it != end; ++it) {
		SafeWrite<T>(*it, data);
	}
}

template <class T, size_t N>
void __stdcall SafeWriteBatch(T data, const DWORD (&addrs)[N]) {
	SafeWriteBatch<T>(data, std::begin(addrs), std::end(addrs));
}

void __stdcall SafeWrite8(DWORD addr, BYTE data);
void __stdcall SafeWrite16(DWORD addr, WORD data);
void __stdcall SafeWrite32(DWORD addr, DWORD data);
void __stdcall SafeWriteStr(DWORD addr, const char* data);

void SafeMemSet(DWORD addr, BYTE val, int len);
void SafeWriteBytes(DWORD addr, BYTE* data, int count);

void HookCall(DWORD addr, void* func);
void MakeCall(DWORD addr, void* func);
void MakeCall(DWORD addr, void* func, int len);
void MakeJump(DWORD addr, void* func);
void MakeJump(DWORD addr, void* func, int len);
void BlockCall(DWORD addr);

// emulation of 4.x HookCalls/MakeCalls/MakeJumps

template <class ForwardIteratorType>
void HookCalls(void* func, ForwardIteratorType begin, ForwardIteratorType end) {
	for (ForwardIteratorType it = begin; it != end; ++it) {
		HookCall(*it, func);
	}
}

template <size_t N>
void HookCalls(void* func, const DWORD (&addrs)[N]) {
	HookCalls(func, std::begin(addrs), std::end(addrs));
}

template <class ForwardIteratorType>
void MakeCalls(void* func, ForwardIteratorType begin, ForwardIteratorType end) {
	for (ForwardIteratorType it = begin; it != end; ++it) {
		MakeCall(*it, func);
	}
}

template <size_t N>
void MakeCalls(void* func, const DWORD (&addrs)[N]) {
	MakeCalls(func, std::begin(addrs), std::end(addrs));
}

template <class ForwardIteratorType>
void MakeJumps(void* func, ForwardIteratorType begin, ForwardIteratorType end) {
	for (ForwardIteratorType it = begin; it != end; ++it) {
		MakeJump(*it, func);
	}
}

template <size_t N>
void MakeJumps(void* func, const DWORD (&addrs)[N]) {
	MakeJumps(func, std::begin(addrs), std::end(addrs));
}
