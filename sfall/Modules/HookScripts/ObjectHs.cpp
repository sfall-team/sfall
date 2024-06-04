#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "ObjectHs.h"

using namespace sfall::script;

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

static __declspec(naked) void UseObjOnHook() {
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

static __declspec(naked) void Drug_UseObjOnHook() {
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

static __declspec(naked) void UseObjHook() {
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

static DWORD __fastcall UseAnimateObjHook_Script(DWORD critter, DWORD animCode, DWORD object) {
	BeginHook();
	argCount = 3;

	args[0] = critter;
	args[1] = object;
	args[2] = animCode;

	RunHookScript(HOOK_USEANIMOBJ);

	if (cRet > 0 && static_cast<long>(rets[0]) <= 64) {
		animCode = rets[0]; // new anim code
	}
	EndHook();

	return animCode;
}

// Before animation of using map object
static __declspec(naked) void UseAnimateObjHook() {
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

static DWORD __fastcall DescriptionObjHook_Script(DWORD object) {
	BeginHook();
	allowNonIntReturn = true;
	argCount = 1;

	args[0] = object;

	RunHookScript(HOOK_DESCRIPTIONOBJ);

	DWORD textPtr = cRet > 0 && (retTypes[0] == DATATYPE_INT || retTypes[0] == DATATYPE_STR)
	              ? rets[0]
	              : 0;

	EndHook();
	return textPtr;
}

static __declspec(naked) void DescriptionObjHook() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, eax; // object
		call DescriptionObjHook_Script;
		pop  edx;
		pop  ecx;
		test eax, eax; // pointer to text
		jnz  skip;
		mov  eax, ebp;
		jmp  fo::funcoffs::object_description_;
skip:
		retn;
	}
}

static bool __fastcall SetLightingHook_Script(DWORD &intensity, DWORD &radius, DWORD object) {
	BeginHook();
	argCount = 3;

	args[0] = object;
	args[1] = intensity;
	args[2] = radius;
	RunHookScript(HOOK_SETLIGHTING);

	bool result = false;
	if (cRet > 0) {
		int light = rets[0];
		if (light < 0) light = 0;
		intensity = light;
		if (cRet > 1 && object != -1) {
			int dist = rets[1];
			if (dist < 0) dist = 0;
			radius = dist;
		}
		result = true;
	}
	EndHook();
	return result;
}

static __declspec(naked) void SetObjectLightHook() {
	__asm {
		pushadc;
		push ebp;
		mov  edx, esp;  // &radius
		push ebx;
		mov  ecx, esp;  // &intensity
		push esi;       // object
		call SetLightingHook_Script;
		test al, al;
		jz   skip;
		pop  ebx;       // return intensity value
		pop  ebp;       // return radius value
		jmp  end;
skip:
		add  esp, 8;
end:
		popadc;
		jmp  fo::funcoffs::obj_turn_off_light_;
	}
}

static __declspec(naked) void SetMapLightHook() {
	__asm {
		push ecx;
		push ebx;
		mov  ecx, esp;  // &intensity
		push -1;        // no object (it's a map)
		mov  edx, esp;  // no radius
		call SetLightingHook_Script;
		add  esp, 4;
		test al, al;
		jz   skip;
		mov  ebx, [esp - 4]; // return intensity value
skip:
		pop  ecx;
		cmp  ebx, dword ptr ds:[0x47A93D]; // get miminal ambient light intensity (16384)
		retn;
	}
}

static DWORD __fastcall StdProcedureHook_Script(long numHandler, fo::ScriptInstance* script, DWORD procTable) {
	BeginHook();
	argCount = 6;

	args[0] = numHandler;
	args[1] = (DWORD)script->selfObject;
	args[2] = (DWORD)script->sourceObject;
	args[4] = (DWORD)script->targetObject;
	args[5] = script->fixedParam;

	if (procTable) {
		args[3] = 0;
		RunHookScript(HOOK_STDPROCEDURE);

		if (cRet > 0) {
			long retval = rets[0];
			if (retval == -1) procTable = retval;
		}
	} else {
		args[3] = 1;
		RunHookScript(HOOK_STDPROCEDURE_END);
	}
	EndHook();
	return procTable;
}

static __declspec(naked) void ScriptStdProcedureHook() {
	using namespace fo::Scripts;
	__asm {
		mov  eax, [eax + 0x54]; // Script.procedure_table
		test eax, eax;
		jle  end;
		cmp  ecx, critter_p_proc;
		je   skip;
		cmp  ecx, timed_event_p_proc;
		je   skip;
		cmp  ecx, map_update_p_proc;
		je   skip;
		cmp  ecx, start;
		jle  skip;
		push ecx;
		push eax;      // procTable
		mov  edx, esi; // script
		call StdProcedureHook_Script; // ecx - numHandler
		pop  ecx;
skip:
		test eax, eax;
end:
		retn;
	}
}

static __declspec(naked) void After_ScriptStdProcedureHook() {
	using namespace fo::Scripts;
	__asm {
		call fo::funcoffs::executeProcedure_;
		cmp  ecx, critter_p_proc;
		je   skip;
		cmp  ecx, timed_event_p_proc;
		je   skip;
		cmp  ecx, map_update_p_proc;
		je   skip;
		cmp  ecx, start;
		jle  skip;
		mov  edx, [esp + 0x28 - 0x18 + 4]; // script
		push 0;                            // procTable
		call StdProcedureHook_Script;      // ecx - numHandler
skip:
		retn;
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

void Inject_UseAnimateObjHook() {
	const DWORD useAnimateObjHkAddr[] = {0x4120C1, 0x412292};
	HookCalls(UseAnimateObjHook, useAnimateObjHkAddr);
}

void Inject_DescriptionObjHook() {
	HookCall(0x49AE28, DescriptionObjHook);
}

void Inject_SetLightingHook() {
	HookCall(0x48ACA0, SetObjectLightHook);
	MakeCall(0x47A934, SetMapLightHook, 1);
}

void Inject_ScriptProcedureHook() {
	MakeCall(0x4A491F, ScriptStdProcedureHook);
}

void Inject_ScriptProcedureHook2() {
	HookCall(0x4A49A7, After_ScriptStdProcedureHook);
}

void InitObjectHookScripts() {
	HookScripts::LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	HookScripts::LoadHookScript("hs_useobj", HOOK_USEOBJ);
	HookScripts::LoadHookScript("hs_useanimobj", HOOK_USEANIMOBJ);
	HookScripts::LoadHookScript("hs_descriptionobj", HOOK_DESCRIPTIONOBJ);
	HookScripts::LoadHookScript("hs_setlighting", HOOK_SETLIGHTING);
	HookScripts::LoadHookScript("hs_stdprocedure", HOOK_STDPROCEDURE); // combo hook
	HookScripts::LoadHookScript("hs_stdprocedure", HOOK_STDPROCEDURE_END);
}

}
