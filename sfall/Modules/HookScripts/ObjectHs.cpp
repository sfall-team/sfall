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

	if (cRet > 0 && static_cast<long>(rets[0]) <= 64) {
		animCode = rets[0]; // new anim code
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

static void __declspec(naked) SetObjectLightHook() {
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

static void __declspec(naked) SetMapLightHook() {
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
	argCount = 3;

	args[0] = numHandler;
	args[1] = (DWORD)script->selfObject;
	args[2] = (DWORD)script->sourceObject;
	RunHookScript(HOOK_STDPROCEDURE);

	if (cRet > 0) {
		long retval = rets[0];
		if (retval == -1) procTable = retval;
	}
	EndHook();
	return procTable;
}

static void __declspec(naked) ScriptStdProcedureHook() {
	using namespace fo::ScriptProc;
	__asm {
		mov  eax, [eax + 0x54]; // Script.procedure_table
		test eax, eax;
		jle  end;
		cmp  ecx, critter_p_proc;
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

void Inject_SetLightingHook() {
	HookCall(0x48ACA0, SetObjectLightHook);
	MakeCall(0x47A934, SetMapLightHook, 1);
}

void Inject_ScriptProcedureHook() {
	MakeCall(0x4A491F, ScriptStdProcedureHook);
}

void InitObjectHookScripts() {

	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	LoadHookScript("hs_useobj", HOOK_USEOBJ);
	LoadHookScript("hs_useanimobj", HOOK_USEANIMOBJ);
	LoadHookScript("hs_descriptionobj", HOOK_DESCRIPTIONOBJ);
	LoadHookScript("hs_setlighting", HOOK_SETLIGHTING);
	LoadHookScript("hs_stdprocedure", HOOK_STDPROCEDURE);
}

}
