#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "ObjectHs.h"

// Object hook scripts
namespace sfall
{

static void __declspec(naked) UseObjOnHook() {
	__asm {
		hookbegin(3);
		mov args[0], edx; // target
		mov args[4], eax; // user
		mov args[8], ebx; // object
		pushad;
		push HOOK_USEOBJON;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl  defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call fo::funcoffs::protinst_use_item_on_;
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) UseObjOnHook_item_d_take_drug() {
	__asm {
		hookbegin(3);
		mov args[0], eax; // target
		mov args[4], eax; // user
		mov args[8], edx; // object
		pushad;
		push HOOK_USEOBJON; // useobjon
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl  defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call fo::funcoffs::item_d_take_drug_;
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) UseObjHook() {
	__asm {
		hookbegin(2);
		mov args[0], eax; // user
		mov args[4], edx; // object
		pushad;
		push HOOK_USEOBJ;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl  defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call fo::funcoffs::protinst_use_item_;
end:
		hookend;
		retn;
	}
}

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

void Inject_UseObjOnHook() {
	HookCalls(UseObjOnHook, { 0x49C606, 0x473619 });

	// the following hooks allows to catch drug use of AI and from action cursor
	HookCalls(UseObjOnHook_item_d_take_drug, {
		0x4285DF, // ai_check_drugs
		0x4286F8, // ai_check_drugs
		0x4287F8, // ai_check_drugs
		0x473573 // inven_action_cursor
	});
}

void Inject_UseObjHook() {
	HookCalls(UseObjHook, { 0x42AEBF, 0x473607, 0x49C12E });
}

void Inject_UseObjectMapHook() {
	HookCalls(UseObjectMapHook, { 0x4120C1, 0x412292 });
}

void InitObjectHookScripts() {

	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	LoadHookScript("hs_useobj", HOOK_USEOBJ);
	LoadHookScript("hs_useobjectmap", HOOK_USEOBJECTMAP);
}

}
