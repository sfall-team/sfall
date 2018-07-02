#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "DeathHs.h"

namespace sfall
{

static DWORD __fastcall CalcDeathAnimHook_Script(register DWORD damage, fo::GameObject* target, fo::GameObject* attacker, fo::GameObject* weapon, int animation, int hitBack) {

	BeginHook();
	argCount = 5; // was 4

	args[1] = (DWORD)attacker;
	args[2] = (DWORD)target;
	args[3] = damage;
	args[4] = -1;

	if (weapon) { // weapon_ptr
		args[0] = weapon->protoId;
	} else {
		args[0] = -1; // attack is unarmed
	}

	RunHookScript(HOOK_DEATHANIM1);

	bool createNewObj = false;
	if (cRet > 0) {
		register DWORD pid = rets[0];
		args[0] = pid;
		fo::GameObject* object = nullptr; 
		if (fo::func::obj_pid_new((fo::GameObject*)&object, pid) != -1) { // create new object
			createNewObj = true;
			weapon = object;  // replace pointer to created object
		}
	}

	long animDeath = fo::func::pick_death(attacker, target, weapon, damage, animation, hitBack); // vanilla pick death

	//argCount = 5;
	args[4] = animDeath;
	RunHookScript(HOOK_DEATHANIM2);
	EndHook();

	if (createNewObj) fo::func::obj_erase_object(weapon, 0); // delete created object

	return (cRet > 0) ? rets[0] : animDeath;
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
		mov	ebx, [esp + 60];
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
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		retn;
	}
}

static void __declspec(naked) OnDeathHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		call fo::funcoffs::critter_kill_;
		pushad;
	}

	argCount = 1;
	RunHookScript(HOOK_ONDEATH);
	EndHook();

	_asm popad;
	_asm retn;
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

	_asm popad;
	_asm retn;
}

void Inject_DeathAnim1Hook() {
	HookCall(0x4109DE, CalcDeathAnimHook);
}

void Inject_DeathAnim2Hook() {
	HookCalls(CalcDeathAnim2Hook, {
		0x410981,
		0x4109A1,
		0x4109BF
	});
}

void Inject_OnDeathHook() {
	HookCalls(OnDeathHook, {
		0x4130CC,
		0x4130EF,
		0x413603,
		0x426EF0,
		0x42D1EC,
		0x42D6F9,
		0x457BC5,
		0x457E3A,
		0x457E54,
		0x4C14F9
	});
	HookCall(0x425161, OnDeathHook2);
}

void InitDeathHookScripts() {

	LoadHookScript("hs_deathanim1", HOOK_DEATHANIM1);
	LoadHookScript("hs_deathanim2", HOOK_DEATHANIM2);
	LoadHookScript("hs_ondeath", HOOK_ONDEATH);

}

}
