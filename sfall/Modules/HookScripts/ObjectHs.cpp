#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "ObjectHs.h"

// Object hook scripts
namespace sfall
{

// Before animation of using map object
static void __declspec(naked) UseObjectMapHook() {
	__asm {
		mov args[0], eax;           // source critter
		mov args[8], edx;           // anim code
		//
		cmp dword ptr [esp], 0x412292 + 5;
		jne contr;
		mov args[4], ebp;           // map object
		jmp next;
contr:
		mov args[4], edi;
next:
		pushad;
	}
	
	BeginHook();
	argCount = 3;
	RunHookScript(HOOK_USEOBJECTMAP);
	EndHook();
	
	if (cRet > 0 && static_cast<long>(rets[0]) > 64) cRet = 0;

	__asm {
		popad;
		cmp cRet, 0;
		jle skip;
		cmp rets[0], -1;
		jle end;
		mov edx, rets[0];
skip:
		call fo::funcoffs::register_object_animate_;
end:
		retn;
	}
}

void Inject_UseObjectMapHook() {
	HookCalls(UseObjectMapHook, { 0x4120C1, 0x412292 });
}

void InitObjectHookScripts() {

	LoadHookScript("hs_useobjectmap", HOOK_USEOBJECTMAP);
}

}
