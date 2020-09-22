#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "DeathHs.h"

namespace sfall
{

static bool registerHookDeathAnim1 = false;
static bool registerHookDeathAnim2 = false;

static DWORD __fastcall CalcDeathAnimHook_Script(DWORD damage, fo::GameObject* target, fo::GameObject* attacker, fo::GameObject* weapon, int animation, int hitBack) {
	BeginHook();
	argCount = 5; // was 4

	args[1] = (DWORD)attacker;
	args[2] = (DWORD)target;
	args[3] = damage;

	// weapon_ptr
	args[0] = (weapon) ? weapon->protoId : -1; // attack is unarmed

	bool createNewObj = false;
	if (registerHookDeathAnim1) {
		args[4] = -1; // set 'no anim' for combined hooks use
		RunHookScript(HOOK_DEATHANIM1);
		if (cRet > 0) {
			DWORD pid = rets[0];
			args[0] = pid; // replace for HOOK_DEATHANIM2
			fo::GameObject* object = nullptr;
			if (fo::func::obj_pid_new((fo::GameObject*)&object, pid) != -1) { // create new object
				createNewObj = true;
				weapon = object; // replace pointer with newly created weapon object
			}
			cRet = 0; // reset rets from HOOK_DEATHANIM1
		}
	}

	DWORD animDeath = fo::func::pick_death(attacker, target, weapon, damage, animation, hitBack); // vanilla pick death

	if (registerHookDeathAnim2) {
		//argCount = 5;
		args[4] = animDeath;
		RunHookScript(HOOK_DEATHANIM2);

		if (cRet > 0) animDeath = rets[0];
	}
	EndHook();

	if (createNewObj) fo::func::obj_erase_object(weapon, 0); // delete created object

	return animDeath;
}

static void __declspec(naked) CalcDeathAnimHook() {
	__asm {
		push ecx;
		push edx;
		push [esp + 4 + 12]; // hit_from_back
		push [esp + 4 + 12]; // animation
		push ebx;            // weapon_ptr
		push eax;            // attacker
		call CalcDeathAnimHook_Script; // ecx - damage, edx - target
		pop  edx;
		pop  ecx;
		retn 8;
	}
}

static void __declspec(naked) CalcDeathAnim2Hook() {
	__asm {
		call fo::funcoffs::check_death_; // call original function
		HookBegin;
		mov ebx, [esp + 60];
		mov args[4], ebx;    // attacker
		mov args[8], esi;    // target
		mov ebx, [esp + 12];
		mov args[12], ebx;   // dmgAmount
		mov args[16], eax;   // calculated animID
		pushad;
	}

	argCount = 5;
	args[0] = -1;     // weaponPid
	RunHookScript(HOOK_DEATHANIM2);

	__asm {
		popad;
		cmp cRet, 1;
		cmovge eax, rets[0];
		HookEnd;
		retn;
	}
}

static void __declspec(naked) OnDeathHook() {
	using namespace fo::Fields;
	__asm {
		push edx;
		call BeginHook;
		mov  args[0], esi;
	}

	argCount = 1;
	RunHookScript(HOOK_ONDEATH);
	EndHook();

	__asm {
		pop edx;
		// engine code
		mov eax, [esi + protoId];
		mov ebp, ebx;
		retn;
	}
}

static void __declspec(naked) OnDeathHook2() {
	__asm {
		HookBegin;
		mov  args[0], esi;
		call fo::funcoffs::partyMemberRemove_;
		pushad;
	}

	argCount = 1;
	RunHookScript(HOOK_ONDEATH);
	EndHook();

	__asm popad;
	__asm retn;
}

void Inject_DeathAnim1Hook() {
	registerHookDeathAnim1 = true;
	if (!registerHookDeathAnim2) {
		HookCall(0x4109DE, CalcDeathAnimHook);
	}
}

void Inject_DeathAnim2Hook() {
	HookCalls(CalcDeathAnim2Hook, {
		0x410981, // show_damage_to_object_
		0x4109A1, // show_damage_to_object_
		0x4109BF  // show_damage_to_object_
	});
	registerHookDeathAnim2 = true;
	if (!registerHookDeathAnim1) {
		HookCall(0x4109DE, CalcDeathAnimHook); // show_damage_to_object_
	}
}

void Inject_OnDeathHook() {
	MakeCall(0x42DA6D, OnDeathHook);  // critter_kill_
	HookCall(0x425161, OnDeathHook2); // damage_object_
}

void InitDeathHookScripts() {
	HookScripts::LoadHookScript("hs_deathanim1", HOOK_DEATHANIM1);
	HookScripts::LoadHookScript("hs_deathanim2", HOOK_DEATHANIM2);
	HookScripts::LoadHookScript("hs_ondeath", HOOK_ONDEATH);
}

}
