#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "HexBlockingHs.h"

namespace sfall
{

static const DWORD _obj_blocking_at = 0x48B84E;
static void __declspec(naked) HexMBlockingHook() {
	__asm {
		HookBegin;
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		push return;
		// engine code
		push ecx;
		push esi;
		push edi;
		push ebp;
		mov ecx, eax;
		jmp _obj_blocking_at;
		// end engine code
return:
		mov args[12], eax;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXMOVEBLOCKING);

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		HookEnd;
		retn;
	}
}

static void __declspec(naked) HexABlockingHook() {
	__asm {
		HookBegin;
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::obj_ai_blocking_at_;
		mov args[12], eax;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXAIBLOCKING);

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		HookEnd;
		retn;
	}
}

static void __declspec(naked) HexShootBlockingHook() {
	__asm {
		HookBegin;
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::obj_shoot_blocking_at_;
		mov args[12], eax;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXSHOOTBLOCKING);

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		HookEnd;
		retn;
	}
}

static void __declspec(naked) HexSightBlockingHook() {
	__asm {
		HookBegin;
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::obj_sight_blocking_at_;
		mov args[12], eax;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXSIGHTBLOCKING);

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		HookEnd;
		retn;
	}
}

void Inject_HexSightBlockHook() {
	SafeWriteBatch<DWORD>((DWORD)&HexSightBlockingHook, { 0x413979 });
}

void Inject_HexShootBlockHook() {
	SafeWriteBatch<DWORD>((DWORD)&HexShootBlockingHook, { 0x4C1A88, 0x423178, 0x4232D4, 0x423B4D, 0x426CF8, 0x42A570 });
}

void Inject_HexIABlockHook() {
	SafeWriteBatch<DWORD>((DWORD)&HexABlockingHook, { 0x42A0A4 });
}

void Inject_HexMoveBlockHook() {
	MakeJump(0x48B848, HexMBlockingHook);
}

void InitHexBlockingHookScripts() {

	LoadHookScript("hs_hexmoveblocking", HOOK_HEXMOVEBLOCKING);
	LoadHookScript("hs_hexaiblocking", HOOK_HEXAIBLOCKING);
	LoadHookScript("hs_hexshootblocking", HOOK_HEXSHOOTBLOCKING);
	LoadHookScript("hs_hexsightblocking", HOOK_HEXSIGHTBLOCKING);

}

}
