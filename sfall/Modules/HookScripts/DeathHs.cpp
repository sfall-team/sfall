#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "DeathHs.h"

namespace sfall
{

static void __declspec(naked) CalcDeathAnimHook() {
	__asm {
		hookbegin(4);
		mov args[24], ebx;
		test ebx, ebx;
		jz noweap
			mov ebx, [ebx + 0x64];
		and ebx, 0xfff;
		jmp weapend;
noweap:
		dec ebx;
weapend:
		mov args[0], ebx;
		mov ebx, args[24];
		mov args[4], eax;
		mov args[8], edx;
		mov args[12], ecx;
		mov args[20], 0;
		pushad;
		push HOOK_DEATHANIM1;
		call RunHookScript;
		cmp cRet, 1;
		jl end1;
		sub esp, 4;
		mov edx, rets[0];
		mov args[0], edx;
		mov eax, esp;
		call fo::funcoffs::obj_pid_new_
			add esp, 4;
		cmp eax, 0xffffffff;
		jz end1;
		mov eax, [esp - 4];
		mov args[20], 1;
		mov args[24], eax;
end1:
		popad;
		mov eax, [esp + 8];
		mov ebx, [esp + 4];
		push eax;
		push ebx;
		mov eax, args[4];
		mov ebx, args[24];
		call fo::funcoffs::pick_death_
			mov args[16], eax;
		mov eax, args[16];
		mov argCount, 5;
		pushad;
		push HOOK_DEATHANIM2;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl skip2;
		mov eax, rets[0];
		mov args[16], eax;
skip2:
		mov eax, args[16];
		push eax;
		mov eax, args[20];
		test eax, eax;
		jz aend;
		mov eax, args[24];
		xor edx, edx;
		call fo::funcoffs::obj_erase_object_
			aend :
		pop eax;
		hookend;
		retn 8;
	}
}

static void __declspec(naked) CalcDeathAnimHook2() {
	__asm {
		hookbegin(5);
		call fo::funcoffs::check_death_; // call original function
		mov args[0], -1; // weaponPid, -1
		mov	ebx, [esp + 60]
			mov args[4], ebx; // attacker
		mov args[8], esi; // target
		mov ebx, [esp + 12]
			mov args[12], ebx; // dmgAmount
		mov args[16], eax; // calculated animID
		pushad;
		push HOOK_DEATHANIM2;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl skip;
		mov eax, rets[0];
		mov args[16], eax;
skip:
		mov eax, args[16];
		hookend;
		retn;
	}
}

static void __declspec(naked) OnDeathHook() {
	__asm {
		hookbegin(1);
		mov args[0], eax;
		call fo::funcoffs::critter_kill_
			pushad;
		push HOOK_ONDEATH;
		call RunHookScript;
		popad;
		hookend;
		retn;
	}
}

static void __declspec(naked) OnDeathHook2() {
	__asm {
		hookbegin(1);
		mov args[0], esi;
		call fo::funcoffs::partyMemberRemove_
			pushad;
		push HOOK_ONDEATH;
		call RunHookScript;
		popad;
		hookend;
		retn;
	}
}

void Inject_DeathAnim1Hook() {
	HookCalls(CalcDeathAnimHook, { 0x4109DE });
}

void Inject_DeathAnim2Hook() {
	HookCalls(CalcDeathAnimHook2, {
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
	HookCalls(OnDeathHook2, { 0x425161 });
}

void InitDeathHookScripts() {

	LoadHookScript("hs_deathanim1", HOOK_DEATHANIM1);
	LoadHookScript("hs_deathanim2", HOOK_DEATHANIM2);
	LoadHookScript("hs_ondeath", HOOK_ONDEATH);

}

}
