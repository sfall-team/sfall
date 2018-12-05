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
		HookBegin;
		mov args[0], edx; // target
		mov args[4], eax; // user
		mov args[8], ebx; // object
		pushad;
	}

	argCount = 3;
	RunHookScript(HOOK_USEOBJON);

	__asm {
		popad;
		cmp cRet, 1;
		jl  defaultHandler;
		cmp rets[0], -1;
		jz  defaultHandler;
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
		pushad;
	}

	argCount = 3;
	RunHookScript(HOOK_USEOBJON);

	__asm {
		popad;
		cmp cRet, 1;
		jl  defaultHandler;
		cmp rets[0], -1;
		jz  defaultHandler;
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
		pushad;
	}
	
	argCount = 2;
	RunHookScript(HOOK_USEOBJ);

	__asm {
		popad;
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

static DWORD __fastcall UseAnimateObjHook_Script(DWORD critter, DWORD animCode, DWORD object) {
	BeginHook();
	argCount = 3;

	args[0] = critter;
	args[1] = object;
	args[2] = animCode;

	RunHookScript(HOOK_USEANIMOBJ);

	if (cRet > 0) {
		if (static_cast<long>(rets[0]) <= 64) {
			animCode = rets[0]; // new anim code
		}
	}
	EndHook();

	return animCode;
}

// Before animation of using map object
static void __declspec(naked) UseAnimateObjHook() {
	__asm {
		cmp  dword ptr [esp], 0x412292 + 5;
		push eax;
		push ecx;
		jne  contr;
		push ebp;                      // map object
		jmp  next;
contr:
		push edi;                      // map object
next:
		mov  ecx, eax;                 // source critter
		call UseAnimateObjHook_Script; // edx - anim code
		pop  ecx;
		cmp  eax, -1;                  // return anim code
		jle  end;                      // goto no animate
		mov  edx, eax;                 // restore vanilla or hook anim code
		pop  eax;
		jmp  fo::funcoffs::register_object_animate_;
end:
		pop  eax;
		retn;
	}
}

static DWORD __stdcall DescriptionObjHook_Script(DWORD object) {
	BeginHook();
	argCount = 1;

	args[0] = object;

	RunHookScript(HOOK_DESCRIPTIONOBJ);

	DWORD textPrt = (cRet > 0) ? rets[0] : 0;
	EndHook();

	return textPrt;
}

static void __declspec(naked) DescriptionObjHook() {
	__asm {
		push eax;
		push edx;
		push ecx;
		push eax;          // object
		call DescriptionObjHook_Script;
		pop  ecx;
		pop  edx;
		test eax, eax;     // pointer to text
		jz   skip;
		add  esp, 4;       // destroy push eax
		retn;
skip:
		pop  eax;
		jmp  fo::funcoffs::item_description_;
	}
}

void Inject_UseObjOnHook() {
	HookCalls(UseObjOnHook, { 0x49C606, 0x473619 });

	// the following hooks allows to catch drug use of AI and from action cursor
	HookCalls(Drug_UseObjOnHook, {
		0x4285DF, // ai_check_drugs
		0x4286F8, // ai_check_drugs
		0x4287F8, // ai_check_drugs
		0x473573 // inven_action_cursor
	});
}

void Inject_UseObjHook() {
	HookCalls(UseObjHook, { 0x42AEBF, 0x473607, 0x49C12E });
}

void Inject_UseAnimateObjHook() {
	HookCalls(UseAnimateObjHook, { 0x4120C1, 0x412292 });
}

void Inject_DescriptionObjHook() {
	HookCall(0x48C925, DescriptionObjHook);
}

void InitObjectHookScripts() {

	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	LoadHookScript("hs_useobj", HOOK_USEOBJ);
	LoadHookScript("hs_useanimobj", HOOK_USEANIMOBJ);
	LoadHookScript("hs_descriptionobj", HOOK_DESCRIPTIONOBJ);
}

}
