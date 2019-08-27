/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

#include <string>
#include <vector>

#include "Define.h"
#include "FalloutEngine.h"
#include "HookScripts.h"
#include "Inventory.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "PartyControl.h"
#include "ScriptExtender.h"


static const int maxArgs = 16;
static const int maxRets = 8;
static const int maxDepth = 8;
static const int numHooks = HOOK_COUNT;

struct sHookScript {
	sScriptProgram prog;
	int callback; // proc number in script's proc table
	bool isGlobalScript; // false for hs_* scripts, true for gl* scripts
};

static std::vector<sHookScript> hooks[numHooks];

struct {
	DWORD hookID;
	DWORD argCount;
	DWORD cArg;
	DWORD cRet;
	DWORD cRetTmp;
	DWORD oldArgs[maxArgs];
	DWORD oldRets[maxRets];
} savedArgs[maxDepth];

static DWORD callDepth;
static DWORD currentRunHook = -1;

DWORD InitingHookScripts;

static DWORD args[maxArgs]; // current hook arguments
static DWORD rets[maxRets]; // current hook return values

static DWORD argCount;
static DWORD cArg;    // how many arguments were taken by current hook script
static DWORD cRet;    // how many return values were set by current hook script
static DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

#define hookbegin(a) pushadc __asm call BeginHook popadc __asm mov argCount, a
#define hookend pushadc __asm call EndHook popadc
#define HookBegin pushadc __asm call BeginHook popadc
//#define HookEnd pushadc __asm call EndHook popadc

static void _stdcall BeginHook() {
	if (callDepth && callDepth <= maxDepth) {
		// save all values of the current hook if another hook was called during the execution of the current hook
		int cDepth = callDepth - 1;
		savedArgs[cDepth].hookID = currentRunHook;
		savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
		savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
		savedArgs[cDepth].cRet = cRet;                                             // number of return values for the current hook
		savedArgs[cDepth].cRetTmp = cRetTmp;
		memcpy(&savedArgs[cDepth].oldArgs, args, argCount * sizeof(DWORD));        // values of the arguments
		if (cRet) memcpy(&savedArgs[cDepth].oldRets, rets, cRet * sizeof(DWORD));  // return values

		// for debugging
		/*dlog_f("\nSaved cArgs/cRets: %d / %d(%d)\n", DL_HOOK, savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
		for (unsigned int i = 0; i < maxArgs; i++) {
			dlog_f("Saved Args/Rets: %d / %d\n", DL_HOOK, savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1));
		}*/
	}
	callDepth++;
	#ifndef NDEBUG
		dlog_f("Begin running hook, current depth: %d, current executable hook: %d\n", DL_HOOK, callDepth, currentRunHook);
	#endif
}

static void _stdcall EndHook() {
	#ifndef NDEBUG
		dlog_f("End running hook %d, current depth: %d\n", DL_HOOK, currentRunHook, callDepth);
	#endif
	callDepth--;
	if (callDepth) {
		if (callDepth <= maxDepth) {
			// restore all saved values of the previous hook
			int cDepth = callDepth - 1;
			currentRunHook = savedArgs[cDepth].hookID;
			argCount = savedArgs[cDepth].argCount;
			cArg = savedArgs[cDepth].cArg;
			cRet = savedArgs[cDepth].cRet;
			cRetTmp = savedArgs[cDepth].cRetTmp;  // also restore current count of the number of return values
			memcpy(args, &savedArgs[cDepth].oldArgs, argCount * sizeof(DWORD));
			if (cRet) memcpy(rets, &savedArgs[cDepth].oldRets, cRet * sizeof(DWORD));

			// for debugging
			/*dlog_f("Restored cArgs/cRets: %d / %d(%d)\n", DL_HOOK, argCount, cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlog_f("Restored Args/Rets: %d / %d\n", args[i], ((i < maxRets) ? rets[i] : -1));
			}*/
		}
	} else {
		currentRunHook = -1;
	}
}

static void _stdcall RunSpecificHookScript(sHookScript *hook) {
	cArg = 0;
	cRetTmp = 0;
	if (hook->callback != -1) {
		RunScriptProcByNum(hook->prog.ptr, hook->callback);
	} else {
		RunScriptProc(&hook->prog, start);
	}
}

static void _stdcall RunHookScript(DWORD hook) {
	cRet = 0;
	if (!hooks[hook].empty()) {
		if (callDepth > 8) {
			DebugPrintf("\n[SFALL] The hook ID: %d cannot be executed.", hook);
			dlog_f("The hook %d cannot be executed due to exceeded depth limit\n", DL_MAIN, hook);
			return;
		}
		currentRunHook = hook;
		size_t hooksCount = hooks[hook].size();
		dlog_f("Running hook %d, which has %0d entries attached, depth: %d\n", DL_HOOK, hook, hooksCount, callDepth);
		for (int i = hooksCount - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);

			// for debugging
			/*dlog_f("> Hook: %d, script entry: %d done\n", DL_HOOK, hook, i);
			dlog_f("> Check cArg/cRet: %d / %d(%d)\n", DL_HOOK, cArg, cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlog_f("> Check Args/Rets: %d / %d\n", DL_HOOK, args[i], ((i < maxRets) ? rets[i] : -1));
			}*/
		}
	} else {
		cArg = 0;
		#ifndef NDEBUG
			dlog_f(">>> Try running hook ID: %d\n", DL_HOOK, hook);
		#endif
	}
}

static void __declspec(naked) ToHitHook() {
	__asm {
		hookbegin(7);
		mov args[4], eax; // attacker
		mov args[8], ebx; // target
		mov args[12], ecx; // body part
		mov args[16], edx; // source tile
		mov eax, [esp+4]; // attack type
		mov args[20], eax;
		mov eax, [esp+8]; // is ranged
		mov args[24], eax;
		mov eax, args[4];
		push [esp+8];
		push [esp+8];
		call determine_to_hit_func_;
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
		mov ebx, [esi+0x20];
		mov args[8], ebx; //Target
		mov ebx, [esi+0x28];
		mov args[12], ebx; //bodypart
		mov ebx, [esp+0x18];
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
		mov [esi+0x28], ebx;
		cmp cRet, 3;
		jl end;
		mov ebx, rets[8];
		mov [esi+0x20], ebx;
end:
		mov ebx, eax;
		hookend;
		cmp ebx, ROLL_FAILURE;
		jmp AfterHitRollAddr;
	}
}

static void __declspec(naked) CalcApCostHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call item_w_mp_cost_;
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

static void __declspec(naked) CalcDeathAnimHook() {
	__asm {
		hookbegin(4);
		mov args[24], ebx;
		test ebx, ebx;
		jz noweap
		mov ebx, [ebx+0x64];
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
		call obj_pid_new_;
		add esp, 4;
		cmp eax, 0xffffffff;
		jz end1;
		mov eax, [esp-4];
		mov args[20], 1;
		mov args[24], eax;
end1:
		popad;
		mov eax, [esp+8];
		mov ebx, [esp+4];
		push eax;
		push ebx;
		mov eax, args[4];
		mov ebx, args[24];
		call pick_death_;
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
		call obj_erase_object_;
aend:
		pop eax;
		hookend;
		retn 8;
	}
}

static void __declspec(naked) CalcDeathAnimHook2() {
	__asm {
		hookbegin(5);
		call check_death_; // call original function
		mov args[0], -1; // weaponPid, -1
		mov ebx, [esp+60];
		mov args[4], ebx; // attacker
		mov args[8], esi; // target
		mov ebx, [esp+12]
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

// 4.x backport
static void __fastcall ComputeDamageHook_Script(TComputeAttack &ctd, DWORD rounds, DWORD multiplier) {
	BeginHook();
	argCount = 12;

	args[0] = (DWORD)ctd.target;           // Target
	args[1] = (DWORD)ctd.attacker;         // Attacker
	args[2] = ctd.targetDamage;            // amountTarget
	args[3] = ctd.attackerDamage;          // amountSource
	args[4] = ctd.targetFlags;             // flagsTarget
	args[5] = ctd.attackerFlags;           // flagsSource
	args[6] = (DWORD)ctd.weapon;
	args[7] = ctd.bodyPart;
	args[8] = multiplier;                  // damage multiplier
	args[9] = rounds;                      // number of rounds
	args[10] = ctd.knockbackValue;
	args[11] = ctd.hitMode;                // attack type

	RunHookScript(HOOK_COMBATDAMAGE);

	if (cRet > 0) {
		ctd.targetDamage = rets[0];
		if (cRet > 1) {
			ctd.attackerDamage = rets[1];
			if (cRet > 2) {
				ctd.targetFlags = rets[2];         // flagsTarget
				if (cRet > 3) {
					ctd.attackerFlags = rets[3];   // flagsSource
					if (cRet > 4) ctd.knockbackValue = rets[4];
				}
			}
		}
	}
	EndHook();
}

static void __declspec(naked) ComputeDamageHook() {
	__asm {
		push ecx;
		push ebx;         // store dmg multiplier  args[8]
		push edx;         // store num of rounds   args[9]
		push eax;         // store ctd
		call compute_damage_;
		pop  ecx;         // restore ctd (eax)
		pop  edx;         // restore num of rounds
		call ComputeDamageHook_Script;  // stack - arg multiplier
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) OnDeathHook() {
	__asm {
		hookbegin(1);
		mov args[0], eax;
		call critter_kill_;
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
		call partyMemberRemove_;
		pushad;
		push HOOK_ONDEATH;
		call RunHookScript;
		popad;
		hookend;
		retn;
	}
}

// 4.x backport
static void __fastcall FindTargetHook_Script(DWORD* target, DWORD attacker) {
	BeginHook();
	argCount = 5;

	args[0] = attacker;
	args[1] = target[0];
	args[2] = target[1];
	args[3] = target[2];
	args[4] = target[3];

	RunHookScript(HOOK_FINDTARGET);

	if (cRet > 0) {
		if (rets[0] != -1) target[0] = rets[0];
		if (cRet > 1 && rets[1] != -1) target[1] = rets[1];
		if (cRet > 2 && rets[2] != -1) target[2] = rets[2];
		if (cRet > 3 && rets[3] != -1) target[3] = rets[3];
	}
	EndHook();
}

static void __declspec(naked) FindTargetHook() {
	__asm {
		push eax;
		call qsort_;
		pop  ecx;          // targets (base)
		mov  edx, esi;     // attacker
		jmp  FindTargetHook_Script;
	}
}

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
		cmp rets[0], -1;
		jz  defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call protinst_use_item_on_;
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
		cmp rets[0], -1;
		jz  defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call item_d_take_drug_;
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
		je  defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call protinst_use_item_;
end:
		hookend;
		retn;
	}
}

static const DWORD RemoveObjHookRet = 0x477497;
static void __declspec(naked) RemoveObjHook() {
	__asm {
		mov ecx, [esp + 8]; // call addr
		hookbegin(5);
		mov args[0], eax;   // source
		mov args[4], edx;   // item
		mov args[8], ebx;   // count
		mov args[12], ecx;
		xor esi, esi;
		xor ecx, 0x47761D;  // from item_move_func_
		cmovz esi, ebp;     // target
		mov args[16], esi;
		push edi;
		push ebp;
		pushad;
		push HOOK_REMOVEINVENOBJ;
		call RunHookScript;
		popad;
		hookend;
		sub esp, 0x0C;
		jmp RemoveObjHookRet;
	}
}

// 4.x backport
// The hook is executed twice when entering the barter screen or after transaction: the first time is for the player and the second time is for NPC
static DWORD __fastcall BarterPriceHook_Script(register TGameObj* source, register TGameObj* target, DWORD callAddr) {
	bool barterIsParty = (*ptr_dialog_target_is_party != 0);
	int computeCost = BarterComputeValue(source, target);

	BeginHook();
	argCount = 10;

	args[0] = (DWORD)source;
	args[1] = (DWORD)target;
	args[2] = !barterIsParty ? computeCost : 0;

	TGameObj* bTable = (TGameObj*)*ptr_btable;
	args[3] = (DWORD)bTable;
	args[4] = ItemCapsTotal(bTable);
	args[5] = ItemTotalCost(bTable);

	TGameObj* pTable = (TGameObj*)*ptr_ptable;
	args[6] = (DWORD)pTable;
	int pcCost = !barterIsParty ? ItemTotalCost(pTable) : ItemTotalWeight(pTable);
	args[7] = !barterIsParty ? pcCost : 0;

	args[8] = (DWORD)(callAddr == 0x474D51); // offers button is pressed
	args[9] = (DWORD)barterIsParty;

	RunHookScript(HOOK_BARTERPRICE);

	bool isPCHook = (callAddr == -1);
	int cost = isPCHook ? pcCost : computeCost;
	if (!barterIsParty && cRet > 0) {
		if (isPCHook) {
			if (cRet > 1) cost = rets[1];     // new cost for pc
		} else if ((int)rets[0] > -1) {
			cost = rets[0];                   // new cost for npc
		}
	}
	EndHook();
	return cost;
}

static void __declspec(naked) BarterPriceHook() {
	__asm {
		push edx;
		push ecx;
		//-------
		push [esp + 8];               // address on call stack
		mov  ecx, eax;                // source
		call BarterPriceHook_Script;  // edx - target
		pop  ecx;
		pop  edx;
		retn;
	}
}

static DWORD offersGoodsCost; // keep last cost for pc
static void __declspec(naked) PC_BarterPriceHook() {
	__asm {
		push edx;
		push ecx;
		//-------
		push -1;                                 // address on call stack
		mov  ecx, dword ptr ds:[_obj_dude];      // source
		mov  edx, dword ptr ds:[_target_stack];  // target
		call BarterPriceHook_Script;
		pop  ecx;
		pop  edx;
		mov  offersGoodsCost, eax;
		retn;
	}
}

static void __declspec(naked) OverrideCost_BarterPriceHook() {
	__asm {
		mov eax, offersGoodsCost;
		retn;
	}
}

static void __declspec(naked) MoveCostHook() {
	__asm {
		hookbegin(3);
		mov args[0], eax;
		mov args[4], edx;
		call critter_compute_ap_from_distance_;
		mov args[8], eax;
		pushad;
		push HOOK_MOVECOST;
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

static const DWORD _obj_blocking_at = 0x48B84E;
static void __declspec(naked) HexMBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		push next;
		push ecx;
		push esi;
		push edi;
		push ebp;
		mov ecx, eax;
		jmp _obj_blocking_at;
next:
		mov args[12], eax;
		pushad;
		push HOOK_HEXMOVEBLOCKING;
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

static void __declspec(naked) HexABlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call obj_ai_blocking_at_;
		mov args[12], eax;
		pushad;
		push HOOK_HEXAIBLOCKING;
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

static void __declspec(naked) HexShootBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call obj_shoot_blocking_at_;
		mov args[12], eax;
		pushad;
		push HOOK_HEXSHOOTBLOCKING;
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

static void __declspec(naked) HexSightBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call obj_sight_blocking_at_;
		mov args[12], eax;
		pushad;
		push HOOK_HEXSIGHTBLOCKING;
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
		call roll_random_;
end:
		hookend;
		retn;
	}
}

// code backported from 4.x
int __fastcall AmmoCostHook_Script(DWORD hookType, TGameObj* weapon, DWORD &rounds) {
	int result = 0;

	BeginHook();
	argCount = 4;

	args[0] = (DWORD)weapon;
	args[1] = rounds;           // rounds in attack
	args[3] = hookType;

	if (hookType == 2) {        // burst hook
		rounds = 1;             // set default multiply for check burst attack
	} else {
		result = ItemWComputeAmmoCost(weapon, &rounds);
		if (result == -1) goto failed; // failed computed
	}
	args[2] = rounds;           // rounds as computed by game (cost)

	RunHookScript(HOOK_AMMOCOST);

	if (cRet > 0) rounds = rets[0]; // override rounds

failed:
	EndHook();
	return result;
}

static void __declspec(naked) AmmoCostHook() {
	__asm {
		xor  ecx, ecx;             // type of hook (0)
		cmp  dword ptr [esp + 0x1C + 4], 46; // ANIM_fire_burst
		jl   skip;
		cmp  dword ptr [esp + 0x1C + 4], 47; // ANIM_fire_continuous
		jg   skip;
		mov  ecx, 3;               // hook type burst
skip:
		xchg eax, edx;
		push eax;                  // rounds in attack
		call AmmoCostHook_Script;  // edx - weapon
		retn;
	}
}

DWORD _stdcall KeyPressHook(DWORD dxKey, bool pressed, DWORD vKey) {
	if (!IsMapLoaded()) {
		return 0;
	}
	DWORD result = 0;
	BeginHook();
	argCount = 3;
	args[0] = (DWORD)pressed;
	args[1] = dxKey;
	args[2] = vKey;
	RunHookScript(HOOK_KEYPRESS);
	if (cRet != 0) dxKey = result = rets[0];
	InventoryKeyPressedHook(dxKey, pressed, vKey);
	EndHook();
	return result;
}

void _stdcall MouseClickHook(DWORD button, bool pressed) {
	if (!IsMapLoaded()) {
		return;
	}
	BeginHook();
	argCount = 2;
	args[0] = (DWORD)pressed;
	args[1] = button;
	RunHookScript(HOOK_MOUSECLICK);
	EndHook();
}

static void __declspec(naked) UseSkillHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // user
		mov args[4], edx; // target
		mov args[8], ebx; // skill id
		mov args[12], ecx; // skill bonus
		pushad;
		push HOOK_USESKILL;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call skill_use_;
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) StealCheckHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // thief
		mov args[4], edx; // target
		mov args[8], ebx; // item
		mov args[12], ecx; // is planting
		pushad;
		push HOOK_STEAL;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call skill_check_stealing_;
end:
		hookend;
		retn;
	}
}

// 4.x backport
static long __stdcall PerceptionRangeHook_Script(int type) {
	long result;
	__asm {
		HookBegin;
		mov  args[0], eax; // watcher
		mov  args[4], edx; // target
		call is_within_perception_;
		mov  result, eax;  // check result
	}
	args[2] = result;
	args[3] = type;

	argCount = 4;
	RunHookScript(HOOK_WITHINPERCEPTION);

	if (cRet > 0) result = rets[0];
	EndHook();

	return result;
}

static void __declspec(naked) PerceptionRangeHook() {
	__asm {
		push ecx;
		push 0;
		call PerceptionRangeHook_Script;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) PerceptionRangeSeeHook() {
	__asm {
		push ecx;
		push 1;
		call PerceptionRangeHook_Script;
		pop  ecx;
		cmp  eax, 2;
		jne  nevermind; // normal return
		dec  eax;
		mov  dword ptr [esp + 0x2C - 0x1C + 4], eax; // set 1, skip blocking check
		dec  eax;
nevermind:
		retn;
	}
}

static void __declspec(naked) PerceptionRangeHearHook() {
	__asm {
		push ecx;
		push 2;
		call PerceptionRangeHook_Script;
		pop  ecx;
		retn;
	}
}

// 4.x backport
static int __fastcall SwitchHandHook_Script(TGameObj* item, TGameObj* itemReplaced, DWORD addr) {
	if (itemReplaced && ItemGetType(itemReplaced) == item_type_weapon && ItemGetType(item) == item_type_ammo) {
		return -1; // to prevent inappropriate hook call after dropping ammo on weapon
	}

	BeginHook();
	argCount = 3;

	args[0] = (addr < 0x47136D) ? 1 : 2;    // slot: 1 - left, 2 - right
	args[1] = (DWORD)item;
	args[2] = (DWORD)itemReplaced;

	RunHookScript(HOOK_INVENTORYMOVE);
	int result = PartyControl_SwitchHandHook(item);
	if (result != -1) {
		cRetTmp = 0;
		SetHSReturn(result);
	}
	result = (cRet > 0) ? rets[0] : -1;
	EndHook();

	return result;
}

/*
	This hook is called every time an item is placed into either hand slot via inventory screen drag&drop
	If switch_hand_ function is not called, item is not placed anywhere (it remains in main inventory)
*/
static void __declspec(naked) SwitchHandHook() {
	__asm {
		pushadc;
		mov  ecx, eax;           // item being moved
		mov  edx, [edx];         // other item
		mov  eax, [esp + 12];    // back address
		push eax;
		call SwitchHandHook_Script;
		cmp  eax, -1;            // ret value
		popadc;
		jne  skip;
		jmp  switch_hand_;
skip:
		retn;
	}
}

/* Common inventory move hook */
static int __fastcall InventoryMoveHook_Script(DWORD itemReplace, DWORD item, int type) {
	BeginHook();
	argCount = 3;

	args[0] = type;         // event type
	args[1] = item;         // item being dropped
	args[2] = itemReplace;  // item being replaced here

	RunHookScript(HOOK_INVENTORYMOVE);

	int result = (cRet > 0) ? rets[0] : -1;
	EndHook();

	return result;
}

static const DWORD UseArmorHack_back = 0x4713AF; // normal operation (old 0x4713A9)
static const DWORD UseArmorHack_skip = 0x471481; // skip code, prevent wearing armor
// This hack is called when an armor is dropped into the armor slot at inventory screen
static void __declspec(naked) UseArmorHack() {
	__asm {
		mov  ecx, ds:[_i_worn];             // replacement item (override code)
		mov  edx, [esp + 0x58 - 0x40];      // item
		push ecx;
		push 3;                             // event: armor slot
		call InventoryMoveHook_Script;      // ecx - replacement item
		cmp  eax, -1;                       // ret value
		pop  ecx;
		jne  skip;
		jmp  UseArmorHack_back;
skip:
		jmp  UseArmorHack_skip;
	}
}

static void __declspec(naked) MoveInventoryHook() {
	__asm {
		pushadc;
		xor eax, eax;
		mov ecx, eax;                    // no item replace
		cmp dword ptr ds:[_curr_stack], 0;
		jle noCont;
		mov ecx, eax;                    // contaner ptr
		mov eax, 5;
noCont:
		push eax;                        // event: 0 - main backpack, 5 - contaner
		call InventoryMoveHook_Script;   // edx - item
		cmp  eax, -1;                    // ret value
		popadc;
		jne  skip;
		jmp  item_add_force_;
skip:
		retn;
	}
}

// Hooks into dropping item from inventory to ground
// - allows to prevent item being dropped if 0 is returned with set_sfall_return
// - called for every item when dropping multiple items in stack (except caps)
// - when dropping caps it called always once
// - if 0 is returned while dropping caps, selected amount - 1 will still disappear from inventory (fixed)
static DWORD nextHookDropSkip = 0;
static int dropResult = -1;
static const DWORD InvenActionObjDropRet = 0x473874;
static void __declspec(naked) InvenActionCursorObjDropHook() {
	if (nextHookDropSkip) {
		nextHookDropSkip = 0;
		goto skipHook;
	} else {
		__asm {
			pushadc;
			xor  ecx, ecx;                       // no itemReplace
			push 6;                              // event: item drop ground
			call InventoryMoveHook_Script;       // edx - item
			mov  dropResult, eax;                // ret value
			popadc;
			cmp  dword ptr [esp], 0x47379A + 5;  // caps call address
			jz   capsMultiDrop;
		}
	}

	if (dropResult == -1) {
skipHook:
		_asm call obj_drop_;
	}
	_asm retn;

/* for only caps multi drop */
capsMultiDrop:
	if (dropResult == -1) {
		nextHookDropSkip = 1;
		_asm call item_remove_mult_;
		_asm retn;
	}
	_asm add esp, 4;
	_asm jmp InvenActionObjDropRet;    // no caps drop
}

static void __declspec(naked) InvenActionExplosiveDropHack() {
	__asm {
		pushadc;
		xor  ecx, ecx;                       // no itemReplace
		push 6;                              // event: item drop ground
		call InventoryMoveHook_Script;       // edx - item
		cmp  eax, -1;                        // ret value
		popadc;
		jnz  noDrop;
		mov  dword ptr ds:[_dropped_explosive], ebp; // overwritten engine code (ebp = 1)
		mov  nextHookDropSkip, ebp;
		retn;
noDrop:
		add  esp, 4;
		jmp  InvenActionObjDropRet;           // no drop
	}
}

static int __fastcall DropIntoContainer(DWORD ptrCont, DWORD item, DWORD addrCall) {
	int type = 5;                   // event: move to container (Crafty compatibility)

	if (addrCall == 0x47147C + 5) { // drop out contaner
		ptrCont = 0;
		type = 0;                   // event: move to main backpack
	}
	return InventoryMoveHook_Script(ptrCont, item, type);
}

static const DWORD DropIntoContainer_back = 0x47649D; // normal operation
static const DWORD DropIntoContainer_skip = 0x476503;
static void __declspec(naked) DropIntoContainerHack() {
	__asm {
		pushadc;
		mov  ecx, ebp;                // contaner ptr
		mov  edx, esi;                // item
		mov  eax, [esp + 0x10 + 12];  // call address
		push eax;
		call DropIntoContainer;
		cmp  eax, -1;                 // ret value
		popadc;
		jne  skipdrop;
		jmp  DropIntoContainer_back;
skipdrop:
		mov  eax, -1;
		jmp  DropIntoContainer_skip;
	}
}

static const DWORD DropIntoContainerRet = 0x471481;
static void __declspec(naked) DropIntoContainerHandSlotHack() {
	__asm {
		call drop_into_container_;
		jmp  DropIntoContainerRet;
	}
}

//static const DWORD DropAmmoIntoWeaponHack_back = 0x47658D; // proceed with reloading
static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
static void __declspec(naked) DropAmmoIntoWeaponHook() {
	__asm {
		pushadc;
		mov  ecx, ebp;              // weapon ptr
		mov  edx, [esp + 16];       // item var: ammo_
		push 4;                     // event: weapon reloading
		call InventoryMoveHook_Script;
		cmp  eax, -1;               // ret value
		popadc;
		jne  donothing;
		jmp  item_w_can_reload_;
		//mov  ebx, 1;   // overwritten code
		//jmp  DropAmmoIntoWeaponHack_back;
donothing:
		add  esp, 4;   // destroy return address
		xor  eax, eax; // result 0
		jmp  DropAmmoIntoWeaponHack_return;
	}
}

static void __declspec(naked) PickupObjectHack() {
	__asm {
		cmp  edi, ds:[_obj_dude];
		je   runHook;
		xor  edx, edx; // engine handler
		retn;
runHook:
		push eax;
		push ecx;
		mov  edx, ecx;
		xor  ecx, ecx;                 // no itemReplace
		push 7;                        // event: item pickup
		call InventoryMoveHook_Script; // edx - item
		mov  edx, eax;                 // ret value
		pop  ecx;
		pop  eax;
		inc  edx; // 0 - engine handler, otherwise cancel pickup
		retn;
	}
}

static void __declspec(naked) InvenPickupHook() {
	__asm {
		call mouse_click_in_;
		test eax, eax;
		jnz  runHook;
		retn;
runHook:
		cmp  dword ptr ds:[_curr_stack], 0;
		jnz  skip;
		mov  edx, [esp + 0x58 - 0x40 + 4]; // item
		xor  ecx, ecx;                     // no itemReplace
		push 8;                            // event: drop item on character portrait
		call InventoryMoveHook_Script;
		cmp  eax, -1;  // ret value
		je   skip;
		xor  eax, eax; // 0 - cancel, otherwise engine handler
skip:
		retn;
	}
}

/* Common InvenWield hook */
static bool InvenWieldHook_Script(int flag) {
	argCount = 4;
	args[3] = flag;  // invenwield flag

	RunHookScript(HOOK_INVENWIELD);

	bool result = (cRet == 0 || rets[0] == -1);
	EndHook();

	return result; // True - use engine handler
}

static void __declspec(naked) InvenWieldFuncHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // slot
		pushad;
	}

	// right hand slot?
	if (args[2] != INVEN_TYPE_RIGHT_HAND && ItemGetType((TGameObj*)args[1]) != item_type_armor) {
		args[2] = INVEN_TYPE_LEFT_HAND;
	}

	InvenWieldHook_Script(1); // wield flag

	__asm {
		test al, al;
		popad;
		jz   skip;
		jmp  invenWieldFunc_;
skip:
		mov  eax, -1;
		retn;
	}
}

// called when unwielding weapons
static void __declspec(naked) InvenUnwieldFuncHook() {
	__asm {
		HookBegin;
		mov args[0], eax;   // critter
		mov args[8], edx;   // slot
		pushad;
	}

	// set slot
	if (args[2] == 0) { // left hand slot?
		args[2] = INVEN_TYPE_LEFT_HAND;
	}
	args[1] = (DWORD)GetItemPtrSlot((TGameObj*)args[0], args[2]); // get item

	InvenWieldHook_Script(0); // unwield flag

	__asm {
		test al, al;
		popad;
		jz   skip;
		jmp  invenUnwieldFunc_;
skip:
		mov  eax, -1;
		retn;
	}
}

static void __declspec(naked) CorrectFidForRemovedItemHook() {
	__asm {
		HookBegin;
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // item flag
		pushad;
	}

	// set slot
	if (args[2] & 0x2000000) {        // right hand slot
		args[2] = INVEN_TYPE_RIGHT_HAND;
	} else if (args[2] & 0x1000000) { // left hand slot
		args[2] = INVEN_TYPE_LEFT_HAND;
	} else {
		args[2] = INVEN_TYPE_WORN;    // armor slot
	}

	InvenWieldHook_Script(0); // unwield flag (armor by default)

	__asm {
		test al, al;
		popad;
		jz   skip;
		jmp  correctFidForRemovedItem_;
skip:
		mov  eax, -1;
		retn;
	}
}

DWORD _stdcall GetHSArgCount() {
	return argCount;
}

DWORD _stdcall GetHSArg() {
	return (cArg == argCount) ? 0 : args[cArg++];
}

void _stdcall SetHSArg(DWORD id, DWORD value) {
	if (id < argCount) args[id] = value;
}

DWORD* _stdcall GetHSArgs() {
	return args;
}

void _stdcall SetHSReturn(DWORD value) {
	if (cRetTmp < maxRets) {
		rets[cRetTmp++] = value;
	}
	if (cRetTmp > cRet) {
		cRet = cRetTmp;
	}
}

void _stdcall RegisterHook(DWORD script, DWORD id, DWORD procNum) {
	if (id >= numHooks) return;
	for (std::vector<sHookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister
			return;
		}
	}
	if (procNum == 0) return; // prevent registration to first location in procedure when reusing "unregister" method

	sScriptProgram *prog = GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Global script %08x registered as hook ID %d\n", DL_HOOK, script, id);
		sHookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;
		hooks[id].push_back(hook);
	}
}

static void LoadHookScript(const char* name, int id) {
	if (id >= numHooks || isGameScript(name)) return;

	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);
	bool fileExist;
	__asm {
		lea  eax, filename
		call db_access_
		mov  fileExist, al
	}

	if (fileExist) {
		sScriptProgram prog;
		dlog("> ", DL_HOOK);
		dlog(name, DL_HOOK);
		LoadScriptProgram(prog, name);
		if (prog.ptr) {
			dlogr(" Done", DL_HOOK);
			sHookScript hook;
			hook.prog = prog;
			hook.callback = -1;
			hook.isGlobalScript = false;
			hooks[id].push_back(hook);
			AddProgramToMap(prog);
		} else {
			dlogr(" Error!", DL_HOOK);
		}
	}
}

static void HookScriptInit2() {
	dlogr("Loading hook scripts:", DL_HOOK|DL_INIT);

	char* mask = "scripts\\hs_*.int";
	DWORD *filenames;
	__asm {
		xor  ecx, ecx
		xor  ebx, ebx
		lea  edx, filenames
		mov  eax, mask
		call db_get_file_list_
	}

	LoadHookScript("hs_tohit", HOOK_TOHIT);
	HookCall(0x421686, &ToHitHook); // combat_safety_invalidate_weapon_func_
	HookCall(0x4231D9, &ToHitHook); // check_ranged_miss_
	HookCall(0x42331F, &ToHitHook); // shoot_along_path_
	HookCall(0x4237FC, &ToHitHook); // compute_attack_
	HookCall(0x424379, &ToHitHook); // determine_to_hit_
	HookCall(0x42438D, &ToHitHook); // determine_to_hit_no_range_
	HookCall(0x42439C, &ToHitHook); // determine_to_hit_from_tile_
	HookCall(0x42679A, &ToHitHook); // combat_to_hit_

	LoadHookScript("hs_afterhitroll", HOOK_AFTERHITROLL);
	MakeJump(0x423893, AfterHitRollHook);

	LoadHookScript("hs_calcapcost", HOOK_CALCAPCOST);
	HookCall(0x42307A, &CalcApCostHook);
	HookCall(0x42669F, &CalcApCostHook);
	HookCall(0x42687B, &CalcApCostHook);
	HookCall(0x42A625, &CalcApCostHook);
	HookCall(0x42A655, &CalcApCostHook);
	HookCall(0x42A686, &CalcApCostHook);
	HookCall(0x42AE32, &CalcApCostHook);
	HookCall(0x42AE71, &CalcApCostHook);
	HookCall(0x460048, &CalcApCostHook);
	HookCall(0x47807B, &CalcApCostHook);
	MakeCall(0x478083, CalcApCostHook2);

	LoadHookScript("hs_deathanim1", HOOK_DEATHANIM1);
	LoadHookScript("hs_deathanim2", HOOK_DEATHANIM2);
	HookCall(0x4109DE, &CalcDeathAnimHook);
	HookCall(0x410981, &CalcDeathAnimHook2);
	HookCall(0x4109A1, &CalcDeathAnimHook2);
	HookCall(0x4109BF, &CalcDeathAnimHook2);

	LoadHookScript("hs_combatdamage", HOOK_COMBATDAMAGE);
	HookCall(0x42326C, ComputeDamageHook); // check_ranged_miss()
	HookCall(0x4233E3, ComputeDamageHook); // shoot_along_path() - for extra burst targets
	HookCall(0x423AB7, ComputeDamageHook); // compute_attack()
	HookCall(0x423BBF, ComputeDamageHook); // compute_attack()
//	HookCall(0x423DE7, ComputeDamageHook); // compute_explosion_on_extras()
	HookCall(0x423E69, ComputeDamageHook); // compute_explosion_on_extras()
	HookCall(0x424220, ComputeDamageHook); // attack_crit_failure()
	HookCall(0x4242FB, ComputeDamageHook); // attack_crit_failure()
	MakeCall(0x423DEB, ComputeDamageHook); // compute_explosion_on_extras() - for the attacker

	LoadHookScript("hs_ondeath", HOOK_ONDEATH);
	HookCall(0x4130CC, &OnDeathHook);
	HookCall(0x4130EF, &OnDeathHook);
	HookCall(0x413603, &OnDeathHook);
	HookCall(0x426EF0, &OnDeathHook);
	HookCall(0x42D1EC, &OnDeathHook);
	HookCall(0x42D6F9, &OnDeathHook);
	HookCall(0x457BC5, &OnDeathHook);
	HookCall(0x457E3A, &OnDeathHook);
	HookCall(0x457E54, &OnDeathHook);
	HookCall(0x4C14F9, &OnDeathHook);
	HookCall(0x425161, &OnDeathHook2);

	LoadHookScript("hs_findtarget", HOOK_FINDTARGET);
	HookCall(0x429143, FindTargetHook);

	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	HookCall(0x49C606, &UseObjOnHook);
	HookCall(0x473619, &UseObjOnHook);
	// the following hooks allows to catch drug use of AI and from action cursor
	HookCall(0x4285DF, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x4286F8, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x4287F8, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x473573, &UseObjOnHook_item_d_take_drug); // inven_action_cursor

	LoadHookScript("hs_removeinvenobj", HOOK_REMOVEINVENOBJ);
	MakeJump(0x477492, RemoveObjHook); // old 0x477490

	LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	HookCall(0x474D4C, BarterPriceHook); // barter_attempt_transaction_ (offers button)
	HookCall(0x475735, BarterPriceHook); // display_table_inventories_ (for party members)
	HookCall(0x475762, BarterPriceHook); // display_table_inventories_
	HookCall(0x4754F4, PC_BarterPriceHook); // display_table_inventories_
	HookCall(0X47551A, PC_BarterPriceHook); // display_table_inventories_
	HookCall(0x474D3F, OverrideCost_BarterPriceHook); // barter_attempt_transaction_ (just overrides cost of offered goods)

	LoadHookScript("hs_movecost", HOOK_MOVECOST);
	HookCall(0x417665, &MoveCostHook);
	HookCall(0x44B88A, &MoveCostHook);

	LoadHookScript("hs_hexmoveblocking", HOOK_HEXMOVEBLOCKING);
	LoadHookScript("hs_hexaiblocking", HOOK_HEXAIBLOCKING);
	LoadHookScript("hs_hexshootblocking", HOOK_HEXSHOOTBLOCKING);
	LoadHookScript("hs_hexsightblocking", HOOK_HEXSIGHTBLOCKING);
	SafeWrite32(0x413979, (DWORD)&HexSightBlockingHook);
	SafeWrite32(0x4C1A88, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x423178, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x4232D4, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x423B4D, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x426CF8, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x42A570, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x42A0A4, (DWORD)&HexABlockingHook);
	MakeJump(0x48B848, HexMBlockingHook);

	LoadHookScript("hs_itemdamage", HOOK_ITEMDAMAGE);
	HookCall(0x478560, &ItemDamageHook);

	LoadHookScript("hs_ammocost", HOOK_AMMOCOST);
	HookCall(0x423A7C, AmmoCostHook);

	LoadHookScript("hs_useobj", HOOK_USEOBJ);
	HookCall(0x42AEBF, &UseObjHook);
	HookCall(0x473607, &UseObjHook);
	HookCall(0x49C12E, &UseObjHook);

	LoadHookScript("hs_keypress", HOOK_KEYPRESS);
	LoadHookScript("hs_mouseclick", HOOK_MOUSECLICK);

	LoadHookScript("hs_useskill", HOOK_USESKILL);
	HookCall(0x49C48F, &UseSkillHook);
	HookCall(0x49D12E, &UseSkillHook);

	LoadHookScript("hs_steal", HOOK_STEAL);
	HookCall(0x4749A2, &StealCheckHook);
	HookCall(0x474A69, &StealCheckHook);

	LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	HookCall(0x429157, PerceptionRangeHook);
	HookCall(0x42B4ED, PerceptionRangeHook);
	HookCall(0x42BC87, PerceptionRangeHook);
	HookCall(0x42BC9F, PerceptionRangeHook);
	HookCall(0x42BD04, PerceptionRangeHook);
	HookCall(0x456BA2, PerceptionRangeSeeHook);
	HookCall(0x458403, PerceptionRangeHearHook);

	LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	HookCall(0x4712E3, SwitchHandHook); // left slot
	HookCall(0x47136D, SwitchHandHook); // right slot
	MakeJump(0x4713A9, UseArmorHack); // old 0x4713A3
	MakeJump(0x476491, DropIntoContainerHack);
	MakeJump(0x471338, DropIntoContainerHandSlotHack);
	MakeJump(0x4712AB, DropIntoContainerHandSlotHack);
	HookCall(0x471200, MoveInventoryHook);
	HookCall(0x476549, DropAmmoIntoWeaponHook); // old 0x476588
	HookCall(0x473851, InvenActionCursorObjDropHook);
	HookCall(0x47386F, InvenActionCursorObjDropHook);
	HookCall(0x47379A, InvenActionCursorObjDropHook); // caps multi drop
	MakeCall(0x473807, InvenActionExplosiveDropHack, 1); // drop active explosives
	MakeCall(0x49B660, PickupObjectHack);
	SafeWrite32(0x49B665, 0x850FD285); // test edx, edx
	SafeWrite32(0x49B669, 0xC2);       // jnz  0x49B72F
	SafeWrite8(0x49B66E, 0xFE); // cmp edi > cmp esi
	HookCall(0x471457, InvenPickupHook);

	LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	HookCall(0x47275E, InvenWieldFuncHook); // inven_wield_
	HookCall(0x495FDF, InvenWieldFuncHook); // partyMemberCopyLevelInfo_
	HookCall(0x45967D, InvenUnwieldFuncHook); // op_metarule_
	HookCall(0x472A5A, InvenUnwieldFuncHook); // inven_unwield_
	HookCall(0x495F0B, InvenUnwieldFuncHook); // partyMemberCopyLevelInfo_
	HookCall(0x45680C, CorrectFidForRemovedItemHook); // op_rm_obj_from_inven_
	HookCall(0x45C4EA, CorrectFidForRemovedItemHook); // op_move_obj_inven_to_obj_

	__asm {
		xor  edx, edx
		lea  eax, filenames
		call db_free_file_list_
	}

	dlogr("Finished loading hook scripts.", DL_HOOK|DL_INIT);
}

void HookScriptClear() {
	for (int i = 0; i < numHooks; i++) {
		hooks[i].clear();
	}
}

void HookScriptInit() {
	isGlobalScriptLoading = 1; // this should allow to register global exported variables
	HookScriptInit2();
	InitingHookScripts = 1;
	for (int i = 0; i < numHooks; i++) {
		if (!hooks[i].empty()) {
			InitScriptProgram(hooks[i][0].prog);// zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	isGlobalScriptLoading = 0;
	InitingHookScripts = 0;
}

// run specific event procedure for all hook scripts
void _stdcall RunHookScriptsAtProc(DWORD procId) {
	for (int i = 0; i < numHooks; i++) {
		if (!hooks[i].empty() && !hooks[i][0].isGlobalScript) {
			RunScriptProc(&hooks[i][0].prog, procId); // run hs_*.int
		}
	}
}

