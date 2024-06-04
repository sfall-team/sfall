#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "HexBlockingHs.h"

namespace sfall
{

static __declspec(naked) void HexMoveBlockingHook() {
	static const DWORD _obj_blocking_at = 0x48B84E;
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		push return;
		// engine code
		push ecx;
		push esi;
		push edi;
		push ebp;
		mov  ecx, eax;
		// end engine code
		jmp  _obj_blocking_at;
return:
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXMOVEBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void HexAIBlockingHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call fo::funcoffs::obj_ai_blocking_at_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXAIBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void HexShootBlockingHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call fo::funcoffs::obj_shoot_blocking_at_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXSHOOTBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void HexSightBlockingHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call fo::funcoffs::obj_sight_blocking_at_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXSIGHTBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

void Inject_HexSightBlockHook() {
	SafeWrite32(0x413979, (DWORD)&HexSightBlockingHook);
}

void Inject_HexShootBlockHook() {
	const DWORD shootBlockingAddr[] = {0x4C1A88, 0x423178, 0x4232D4, 0x423B4D, 0x426CF8, 0x42A570};
	SafeWriteBatch<DWORD>((DWORD)&HexShootBlockingHook, shootBlockingAddr);
}

void Inject_HexIABlockHook() {
	SafeWrite32(0x42A0A4, (DWORD)&HexAIBlockingHook);
}

void Inject_HexMoveBlockHook() {
	MakeJump(0x48B848, HexMoveBlockingHook);
}

void InitHexBlockingHookScripts() {
	HookScripts::LoadHookScript("hs_hexmoveblocking", HOOK_HEXMOVEBLOCKING);
	HookScripts::LoadHookScript("hs_hexaiblocking", HOOK_HEXAIBLOCKING);
	HookScripts::LoadHookScript("hs_hexshootblocking", HOOK_HEXSHOOTBLOCKING);
	HookScripts::LoadHookScript("hs_hexsightblocking", HOOK_HEXSIGHTBLOCKING);
}

}
