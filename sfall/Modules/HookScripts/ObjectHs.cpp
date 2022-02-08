#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "ObjectHs.h"

// Object hook scripts
namespace sfall
{

static long UseObjOnHook_Script(fo::GameObject* source, fo::GameObject* item, fo::GameObject* target) {
	BeginHook();
	argCount = 3;

	args[0] = (DWORD)target; // target
	args[1] = (DWORD)source; // user
	args[2] = (DWORD)item;   // item

	RunHookScript(HOOK_USEOBJON);

	long result = (cRet > 0) ? rets[0] : -1;
	EndHook();

	return result; // -1 - default handler
}

long UseObjOnHook_Invoke(fo::GameObject* source, fo::GameObject* item, fo::GameObject* target) {
	if (!HookScripts::HookHasScript(HOOK_USEOBJON)) return -1;
	return UseObjOnHook_Script(source, item, target);
}

static void __declspec(naked) UseObjOnHook() {
	__asm {
		HookBegin;
		mov args[0], edx; // target
		mov args[4], eax; // user
		mov args[8], ebx; // object
		pushadc;
	}

	argCount = 3;
	RunHookScript(HOOK_USEOBJON);

	__asm {
		popadc;
		cmp cRet, 1;
		jl  defaultHandler;
		cmp rets[0], -1;
		je  defaultHandler;
		mov eax, rets[0];
		HookEnd;
		retn;
defaultHandler:
		HookEnd;
		jmp fo::funcoffs::protinst_use_item_on_;
	}
}

static void __declspec(naked) Drug_UseObjOnHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // target
		mov args[4], eax; // user
		mov args[8], edx; // object
		pushadc;
	}

	argCount = 3;
	RunHookScript(HOOK_USEOBJON);

	__asm {
		popadc;
		cmp cRet, 1;
		jl  defaultHandler;
		cmp rets[0], -1;
		je  defaultHandler;
		mov eax, rets[0];
		HookEnd;
		retn;
defaultHandler:
		HookEnd;
		jmp fo::funcoffs::item_d_take_drug_;
	}
}

static void __declspec(naked) UseObjHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // user
		mov args[4], edx; // object
		pushadc;
	}

	argCount = 2;
	RunHookScript(HOOK_USEOBJ);

	__asm {
		popadc;
		cmp cRet, 1;
		jl  defaultHandler;
		cmp rets[0], -1;
		je  defaultHandler;
		mov eax, rets[0];
		HookEnd;
		retn;
defaultHandler:
		HookEnd;
		jmp fo::funcoffs::protinst_use_item_;
	}
}

void Inject_UseObjOnHook() {
	const DWORD useObjOnHkAddr[] = {0x49C606, 0x473619};
	HookCalls(UseObjOnHook, useObjOnHkAddr);
	// the following hooks allows to catch drug use of AI and from action cursor
	const DWORD drugUseObjOnHkAddr[] = {
		//0x4285DF, // ai_check_drugs
		//0x4286F8, // ai_check_drugs
		//0x4287F8, // ai_check_drugs
		0x473573  // inven_action_cursor
	};
	HookCalls(Drug_UseObjOnHook, drugUseObjOnHkAddr);
}

void Inject_UseObjHook() {
	const DWORD useObjHkAddr[] = {0x42AEBF, 0x473607, 0x49C12E};
	HookCalls(UseObjHook, useObjHkAddr);
}

void InitObjectHookScripts() {
	HookScripts::LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	HookScripts::LoadHookScript("hs_useobj", HOOK_USEOBJ);
}

}
