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
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		push next;
		push ecx;
		push esi;
		push edi;
		push ebp;
		mov ecx, eax;
		jmp _obj_blocking_at;
next:
		mov args[12], eax;
		pushad;
		push HOOK_HEXMOVEBLOCKING;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) HexABlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::obj_ai_blocking_at_;
		mov args[12], eax;
		pushad;
		push HOOK_HEXAIBLOCKING;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) HexShootBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::obj_shoot_blocking_at_;
		mov args[12], eax;
		pushad;
		push HOOK_HEXSHOOTBLOCKING;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) HexSightBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::obj_sight_blocking_at_;
		mov args[12], eax;
		pushad;
		push HOOK_HEXSIGHTBLOCKING;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
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
