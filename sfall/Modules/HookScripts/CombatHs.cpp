#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "CombatHs.h"

namespace sfall
{

static void __declspec(naked) ToHitHook() {
	__asm {
		hookbegin(7);
		mov args[4], eax; // attacker
		mov args[8], ebx; // target
		mov args[12], ecx; // body part
		mov args[16], edx; // source tile
		mov eax, [esp + 4]; // attack type
		mov args[20], eax;
		mov eax, [esp + 8]; // is ranged
		mov args[24], eax;
		mov eax, args[4];
		push[esp + 8];
		push[esp + 8];
		call fo::funcoffs::determine_to_hit_func_;
		mov args[0], eax;
		pushad;
		push HOOK_TOHIT;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn 8;
	}
}

static const DWORD AfterHitRollAddr = 0x423898;
static void __declspec(naked) AfterHitRollHook() {
	__asm {
		hookbegin(5);
		mov args[0], eax; //was it a hit?
		mov ebx, [esi];
		mov args[4], ebx; //Attacker
		mov ebx, [esi + 0x20];
		mov args[8], ebx; //Target
		mov ebx, [esi + 0x28];
		mov args[12], ebx; //bodypart
		mov ebx, [esp + 0x18];
		mov args[16], ebx; //hit chance
		pushad;
		push HOOK_AFTERHITROLL;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
		cmp cRet, 2;
		jl end;
		mov ebx, rets[4];
		mov[esi + 0x28], ebx;
		cmp cRet, 3;
		jl end;
		mov ebx, rets[8];
		mov[esi + 0x20], ebx;
end:
		mov ebx, eax;
		hookend;
		cmp ebx, 1;
		jmp AfterHitRollAddr;
	}
}

static void __declspec(naked) CalcApCostHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call fo::funcoffs::item_w_mp_cost_;
		mov args[12], eax;
		pushad;
		push HOOK_CALCAPCOST;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

// this is for using non-weapon items, always 2 AP in vanilla
static void __declspec(naked) CalcApCostHook2() {
	__asm {
		hookbegin(4);
		mov args[0], ecx; // critter
		mov args[4], edx; // attack type (to determine hand)
		mov args[8], ebx;
		mov eax, 2; // vanilla value
		mov args[12], eax;
		pushad;
		push HOOK_CALCAPCOST;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) ComputeDamageHook() {
	__asm {
		push edx;
		push ebx;
		push eax;
		call fo::funcoffs::compute_damage_;
		pop edx;

		//zero damage insta death criticals fix
		mov ebx, [edx + 0x2c];
		test ebx, ebx;
		jnz hookscript;
		mov ebx, [edx + 0x30];
		test bl, 0x80;
		jz hookscript;
		inc dword ptr ds : [edx + 0x2c];
hookscript:
		hookbegin(11);
		mov ebx, [edx + 0x20];
		mov args[0x00], ebx;
		mov ebx, [edx + 0x00];
		mov args[0x04], ebx;
		mov ebx, [edx + 0x2c];
		mov args[0x08], ebx;
		mov ebx, [edx + 0x10];
		mov args[0x0c], ebx;
		mov ebx, [edx + 0x30];
		mov args[0x10], ebx;
		mov ebx, [edx + 0x14];
		mov args[0x14], ebx;
		mov ebx, [edx + 0x08];
		mov args[0x18], ebx;
		mov ebx, [edx + 0x28];
		mov args[0x1c], ebx;
		pop ebx; // roll result
		mov args[0x20], ebx;
		pop ebx; // num rounds
		mov args[0x24], ebx;
		mov ebx, [edx + 0x34]; // knockback value
		mov args[0x28], ebx;

		pushad;
		push HOOK_COMBATDAMAGE;
		call RunHookScript;
		popad;

		cmp cRet, 1;
		jl end;
		mov ebx, rets[0x00];
		mov[edx + 0x2c], ebx;
		cmp cRet, 2;
		jl end;
		mov ebx, rets[0x04];
		mov[edx + 0x10], ebx;
		cmp cRet, 3;
		jl end;
		mov ebx, rets[0x08];
		mov[edx + 0x30], ebx;
		cmp cRet, 4;
		jl end;
		mov ebx, rets[0x0c];
		mov[edx + 0x14], ebx;
		cmp cRet, 5;
		jl end;
		mov ebx, rets[0x10];
		mov[edx + 0x34], ebx; // knockback
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) FindTargetHook() {
	__asm {
		hookbegin(5);
		mov args[0], esi; //attacker
		mov edi, [eax + 0];
		mov args[4], edi;
		mov edi, [eax + 4];
		mov args[8], edi;
		mov edi, [eax + 8];
		mov args[12], edi;
		mov edi, [eax + 12];
		mov args[16], edi;
		pushad;
		push HOOK_FINDTARGET;
		call RunHookScript;
		popad;
		cmp cRet, 4;
		jge cont;
		call fo::funcoffs::qsort_;
		jmp end;
cont:
		mov edi, rets[0];
		mov[eax + 0], edi;
		mov edi, rets[4];
		mov[eax + 4], edi;
		mov edi, rets[8];
		mov[eax + 8], edi;
		mov edi, rets[12];
		mov[eax + 12], edi;
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) ItemDamageHook() {
	__asm {
		hookbegin(6);
		mov args[0], eax; //min
		mov args[4], edx; //max
		mov args[8], edi; //weapon
		mov args[12], ecx; //critter
		mov args[16], esi; //type
		mov args[20], ebp; //non-zero for weapon melee attack
		test edi, edi;
		jnz skip;
		add args[16], 8;
skip:
		pushad;
		push HOOK_ITEMDAMAGE;
		call RunHookScript;
		popad;
		cmp cRet, 0;
		je runrandom;
		mov eax, rets[0];
		cmp cRet, 1;
		je end;
		mov edx, rets[4];
runrandom:
		call fo::funcoffs::roll_random_;
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) AmmoCostHook_internal() {
	__asm {
		pushad;
		mov args[0], eax; //weapon
		mov ebx, [edx];
		mov args[4], ebx; //rounds in attack
		call fo::funcoffs::item_w_compute_ammo_cost_;
		cmp eax, -1;
		je fail;
		mov ebx, [edx];
		mov args[8], ebx; //rounds as computed by game

		push HOOK_AMMOCOST;
		call RunHookScript;
		popad;
		cmp cRet, 0;
		je end;
		mov eax, rets[0];
		mov[edx], eax; // override result
		mov eax, 0;
		jmp end;
fail:
		popad;
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) AmmoCostHook() {
	__asm {
		hookbegin(4);
		mov args[12], 0; // type of hook
		jmp AmmoCostHook_internal;
	}
}

void __declspec(naked) AmmoCostHookWrapper() {
	__asm {
		hookbegin(4);
		push eax;
		mov eax, [esp + 8]; // hook type
		mov args[12], eax;
		pop eax;
		call AmmoCostHook_internal;
		retn;
	}
}

void InitCombatHookScripts() {
	LoadHookScript("hs_tohit", HOOK_TOHIT);
	HookCalls(ToHitHook, {
		0x421686, // combat_safety_invalidate_weapon_func_
		0x4231D9, // check_ranged_miss_
		0x42331F, // shoot_along_path_
		0x4237FC, // compute_attack_
		0x424379, // determine_to_hit_
		0x42438D, // determine_to_hit_no_range_
		0x42439C, // determine_to_hit_from_tile_
		0x42679A // combat_to_hit_
	});

	LoadHookScript("hs_afterhitroll", HOOK_AFTERHITROLL);
	MakeJump(0x423893, AfterHitRollHook);

	LoadHookScript("hs_calcapcost", HOOK_CALCAPCOST);
	HookCalls(CalcApCostHook, {
		0x42307A,
		0x42669F,
		0x42687B,
		0x42A625,
		0x42A655,
		0x42A686,
		0x42AE32,
		0x42AE71,
		0x460048,
		0x47807B
	});
	MakeCall(0x478083, CalcApCostHook2);

	LoadHookScript("hs_combatdamage", HOOK_COMBATDAMAGE);
	HookCalls(ComputeDamageHook, {
		0x42326C, // check_ranged_miss()
		0x4233E3, // shoot_along_path() - for extra burst targets
		0x423AB7, // compute_attack()
		0x423BBF, // compute_attack()
		0x423DE7, // compute_explosion_on_extras()
		0x423E69, // compute_explosion_on_extras()
		0x424220, // attack_crit_failure()
		0x4242FB, // attack_crit_failure()
	});

	LoadHookScript("hs_findtarget", HOOK_FINDTARGET);
	HookCalls(FindTargetHook, { 0x429143 });

	LoadHookScript("hs_itemdamage", HOOK_ITEMDAMAGE);
	HookCalls(ItemDamageHook, { 0x478560 });

	LoadHookScript("hs_ammocost", HOOK_AMMOCOST);
	HookCalls(AmmoCostHook, { 0x423A7C });
}

}
