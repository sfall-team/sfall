#include "Numbers.h"
#include "Imports.h"
#include "Defines.h"
#include "SafeWrite.h"
#include "Input.h"

static void RunNumbers(DWORD load) {
	if(!load) {

	}
	exit(0);
}

static void NewGameRet=0x481A00;
static void _declspec(naked) NewGameHook() {
	_asm {
		pushad;
		push DIK_LSHIFT;
		call KeyDown;
		test eax, eax;
		jz fail;
		popad;
		xor eax, eax;
		push eax;
		call RunNumbers();
fail:
		popad;
		jmp NewGameRet;
	}
}

static void LoadGameRet=0x480AFE;
static void _declspec(naked) LoadGameHook() {
	_asm {
		pushad;
		push DIK_LSHIFT;
		call KeyDown;
		test eax, eax;
		jz fail;
		popad;
		push 1;
		call RunNumbers();
fail:
		popad;
		mov ecx, 0x1e0;
		jmp LoadGameRet;
	}
}

NumbersInit() {
	HookCall(0x480A81, &NewGameHook);
	MakeCall(0x480AF9, &LoadGameHook, true);
}