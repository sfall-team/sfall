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

#include <string>
#include <vector>

#include "main.h"
#include "FalloutEngine.h"
#include "Combat.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "PartyControl.h"
#include "ScriptExtender.h"

#include "HookScripts.h"

// Number of types of hooks
static const int numHooks = HOOK_COUNT;

static const int maxArgs = 16; // Maximum number of hook arguments
static const int maxRets = 8;  // Maximum number of return values
static const int maxDepth = 8; // Maximum recursion depth for hook calls

// Struct for registered hook script
struct sHookScript {
	sScriptProgram prog;
	int callback;        // proc number in script's proc table
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

static DWORD args[maxArgs]; // current hook arguments
static DWORD rets[maxRets]; // current hook return values

static DWORD argCount;
static DWORD cArg;    // how many arguments were taken by current hook script
static DWORD cRet;    // how many return values were set by current hook script
static DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

static struct HooksPositionInfo {
	long hsPosition;    // index of the hs_* script, or the beginning of the position for registering scripts using register_hook
//	long positionShift; // offset to the last script registered by register_hook
	bool hasHsScript;
} hooksInfo[numHooks];

DWORD initingHookScripts;

#define HookBegin pushadc __asm call BeginHook popadc
#define HookEnd pushadc __asm call EndHook popadc

static void __stdcall BeginHook() {
	if (callDepth && callDepth <= maxDepth) {
		// save all values of the current hook if another hook was called during the execution of the current hook
		int cDepth = callDepth - 1;
		savedArgs[cDepth].hookID = currentRunHook;
		savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
		savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
		savedArgs[cDepth].cRet = cRet;                                             // number of return values for the current hook
		savedArgs[cDepth].cRetTmp = cRetTmp;
		std::memcpy(&savedArgs[cDepth].oldArgs, args, maxArgs * sizeof(DWORD));           // values of the arguments
		if (cRet) std::memcpy(&savedArgs[cDepth].oldRets, rets, maxRets * sizeof(DWORD)); // return values

		//devlog_f("\nSaved cArgs/cRet: %d / %d(%d)\n", DL_HOOK, savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
		//for (unsigned int i = 0; i < maxArgs; i++) {
		//	devlog_f("Saved Args/Rets: %d / %d\n", DL_HOOK, savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1));
		//}
	}
	callDepth++;

	devlog_f("Begin running hook, current depth: %d, current executable hook: %d\n", DL_HOOK, callDepth, currentRunHook);
}

static void __stdcall RunSpecificHookScript(sHookScript *hook) {
	cArg = 0;
	cRetTmp = 0;
	if (hook->callback != -1) {
		ExecuteProcedure(hook->prog.ptr, hook->callback);
	} else {
		hook->callback = RunScriptStartProc(&hook->prog); // run start
	}
}

static void __stdcall RunHookScript(DWORD hook) {
	cRet = 0;
	if (!hooks[hook].empty()) {
		if (callDepth > 8) {
			DebugPrintf("\n[SFALL] The hook ID: %d cannot be executed.", hook);
			dlog_f("The hook %d cannot be executed due to exceeding depth limit\n", DL_MAIN, hook);
			return;
		}
		currentRunHook = hook;
		size_t hooksCount = hooks[hook].size();
		dlog_f("Running hook %d, which has %0d entries attached, depth: %d\n", DL_HOOK, hook, hooksCount, callDepth);
		for (int i = hooksCount - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);

			//devlog_f("> Hook: %d, script entry: %d done\n", DL_HOOK, hook, i);
			//devlog_f("> Check cArg/cRet: %d / %d(%d)\n", DL_HOOK, cArg, cRet, cRetTmp);
			//for (unsigned int i = 0; i < maxArgs; i++) {
			//	devlog_f("> Check Args/Rets: %d / %d\n", DL_HOOK, args[i], ((i < maxRets) ? rets[i] : -1));
			//}
		}
	} else {
		cArg = 0;

		devlog_f(">>> Try running hook ID: %d\n", DL_HOOK, hook);
	}
}

static void __stdcall EndHook() {
	devlog_f("End running hook %d, current depth: %d\n", DL_HOOK, currentRunHook, callDepth);

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
			std::memcpy(args, &savedArgs[cDepth].oldArgs, maxArgs * sizeof(DWORD));
			if (cRet) std::memcpy(rets, &savedArgs[cDepth].oldRets, maxRets * sizeof(DWORD));

			//devlog_f("Restored cArgs/cRet: %d / %d(%d)\n", DL_HOOK, argCount, cRet, cRetTmp);
			//for (unsigned int i = 0; i < maxArgs; i++) {
			//	devlog_f("Restored Args/Rets: %d / %d\n", DL_HOOK, args[i], ((i < maxRets) ? rets[i] : -1));
			//}
		}
	} else {
		currentRunHook = -1;
	}
}

// BEGIN HOOKS
static void __declspec(naked) ToHitHook() {
	__asm {
		HookBegin;
		mov  args[4], eax;    // attacker
		mov  args[8], ebx;    // target
		mov  args[12], ecx;   // body part
		mov  args[16], edx;   // source tile
		mov  eax, [esp + 8];
		mov  args[24], eax;   // is ranged
		push eax;
		mov  eax, [esp + 8];
		mov  args[20], eax;   // attack type
		push eax;
		mov  eax, args[4];    // restore
		call determine_to_hit_func_;
		mov  args[0], eax;
		mov  ebx, eax;
	}
	argCount = 8;

	args[7] = Combat_determineHitChance;
	RunHookScript(HOOK_TOHIT);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		retn 8;
	}
}

// 4.x backport
static DWORD __fastcall AfterHitRollHook_Script(TComputeAttack &ctd, DWORD hitChance, DWORD hit) {
	BeginHook();
	argCount = 5;

	args[0] = hit;
	args[1] = (DWORD)ctd.attacker;   // Attacker
	args[2] = (DWORD)ctd.target;     // Target
	args[3] = ctd.bodyPart;          // bodypart
	args[4] = hitChance;

	RunHookScript(HOOK_AFTERHITROLL);
	if (cRet > 0) {
		hit = rets[0];
		if (cRet > 1) {
			ctd.bodyPart = rets[1];
			if (cRet > 2) ctd.target = (TGameObj*)rets[2];
		}
	}
	EndHook();

	return hit;
}

static void __declspec(naked) AfterHitRollHook() {
	__asm {
		mov  ecx, esi;              // ctd
		mov  edx, [esp + 0x18 + 4]; // hit chance
		push eax;                   // was it a hit?
		call AfterHitRollHook_Script;
		// engine code
		mov  ebx, eax;
		cmp  eax, ROLL_FAILURE;
		retn;
	}
}

// Implementation of item_w_mp_cost_ engine function with the hook
long __fastcall sf_item_w_mp_cost(TGameObj* source, long hitMode, long isCalled) {
	long cost = ItemWMpCost(source, hitMode, isCalled);

	BeginHook();
	argCount = 4;

	args[0] = (DWORD)source;
	args[1] = hitMode;
	args[2] = isCalled;
	args[3] = cost;

	RunHookScript(HOOK_CALCAPCOST);

	if (cRet > 0) cost = rets[0];
	EndHook();

	return cost;
}

static void __declspec(naked) CalcApCostHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call item_w_mp_cost_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_CALCAPCOST);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

// this is for using non-weapon items, always 2 AP in vanilla
static void __declspec(naked) CalcApCostHook2() {
	__asm {
		HookBegin;
		mov args[0], ecx; // critter
		mov args[4], edx; // attack type (to determine hand)
		mov args[8], ebx;
		mov ebx, 2;       // vanilla cost value
		mov args[12], ebx;
		//push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_CALCAPCOST);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		//pop  ecx;
		retn;
	}
}

// 4.x backport
static DWORD __fastcall CalcDeathAnimHook_Script(DWORD damage, TGameObj* target, TGameObj* attacker, TGameObj* weapon, int animation, int hitBack) {
	BeginHook();
	argCount = 5; // was 4

	args[1] = (DWORD)attacker;
	args[2] = (DWORD)target;
	args[3] = damage;

	// weapon_ptr
	args[0] = (weapon) ? weapon->protoId : -1; // attack is unarmed

	bool createNewObj = false;
	args[4] = -1; // set 'no anim' for combined hooks use
	RunHookScript(HOOK_DEATHANIM1);
	if (cRet > 0) {
		DWORD pid = rets[0];
		args[0] = pid; // replace for HOOK_DEATHANIM2
		TGameObj* object = nullptr;
		if (ObjPidNew((TGameObj*)&object, pid) != -1) { // create new object
			createNewObj = true;
			weapon = object; // replace pointer with newly created weapon object
		}
		cRet = 0; // reset rets from HOOK_DEATHANIM1
	}

	DWORD animDeath = PickDeath(attacker, target, weapon, damage, animation, hitBack); // vanilla pick death

	args[4] = animDeath;
	RunHookScript(HOOK_DEATHANIM2);

	if (cRet > 0) animDeath = rets[0];
	EndHook();

	if (createNewObj) ObjEraseObject(weapon, 0); // delete created object

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
		call check_death_; // call original function
		mov  ebx, eax;
		call BeginHook;
		mov  eax, [esp + 60];
		mov  args[4], eax;    // attacker
		mov  args[8], esi;    // target
		mov  eax, [esp + 12];
		mov  args[12], eax;   // dmgAmount
		mov  args[16], ebx;   // calculated animID
	}

	argCount = 5;
	args[0] = -1;     // weaponPid
	RunHookScript(HOOK_DEATHANIM2);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		retn;
	}
}

// 4.x backport
static void __fastcall ComputeDamageHook_Script(TComputeAttack &ctd, DWORD rounds, DWORD multiplier) {
	BeginHook();
	argCount = 13;

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
	args[12] = (DWORD)&ctd;                // main_ctd/shoot_ctd/explosion_ctd

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
	using namespace Fields;
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
		call partyMemberRemove_;
		HookBegin;
		mov  args[0], esi;
		pushadc;
	}

	argCount = 1;
	RunHookScript(HOOK_ONDEATH);
	EndHook();

	__asm popadc;
	__asm retn;
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
		jmp protinst_use_item_on_;
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
		jmp item_d_take_drug_;
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
		jmp protinst_use_item_;
	}
}

static void __declspec(naked) RemoveObjHook() {
	static const DWORD RemoveObjHookRet = 0x477497;
	__asm {
		mov ecx, [esp + 8]; // call addr
		HookBegin;
		mov args[0], eax;   // source
		mov args[4], edx;   // item
		mov args[8], ebx;   // count
		mov args[12], ecx;
		xor esi, esi;
		xor ecx, 0x47761D;  // from item_move_func_
		cmovz esi, ebp;     // target
		mov  args[16], esi;
		push edi;
		push ebp;
		push eax;
		push edx;
	}

	argCount = 5;
	RunHookScript(HOOK_REMOVEINVENOBJ);
	EndHook();

	__asm {
		pop edx;
		pop eax;
		sub esp, 0x0C;
		jmp RemoveObjHookRet;
	}
}

// 4.x backport
// The hook is executed twice when entering the barter screen and after transaction: the first time is for the player; the second time is for NPC
static DWORD __fastcall BarterPriceHook_Script(register TGameObj* source, register TGameObj* target, DWORD callAddr) {
	bool barterIsParty = (*ptr_dialog_target_is_party != 0);
	long computeCost = BarterComputeValue(source, target);

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

	long pcCost = 0;
	if (barterIsParty) {
		args[7] = pcCost;
		pcCost = ItemTotalWeight(pTable);
	} else {
		args[7] = pcCost = ItemTotalCost(pTable);
	}

	args[8] = (DWORD)(callAddr == 0x474D51); // offers button pressed
	args[9] = (DWORD)barterIsParty;

	RunHookScript(HOOK_BARTERPRICE);

	bool isPCHook = (callAddr == -1);
	long cost = isPCHook ? pcCost : computeCost;
	if (!barterIsParty && cRet > 0) {
		if (isPCHook) {
			if (cRet > 1) cost = rets[1];     // new cost for pc
		} else if ((long)rets[0] > -1) {
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
		mov  ecx, dword ptr ds:[FO_VAR_obj_dude];      // source
		mov  edx, dword ptr ds:[FO_VAR_target_stack];  // target
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
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		call critter_compute_ap_from_distance_;
		mov  args[8], eax;
		push ebx;
		push ecx;
		mov  ebx, eax;
	}

	argCount = 3;
	RunHookScript(HOOK_MOVECOST);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static void __declspec(naked) HexMoveBlockingHook() {
	static const DWORD _obj_blocking_at = 0x48B84E;
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		push return;
		// engine code
		push ecx;
		push esi;
		push edi;
		push ebp;
		mov  ecx, eax;
		// end engine code
		jmp  _obj_blocking_at;
return:
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXMOVEBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) HexAIBlockingHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call obj_ai_blocking_at_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXAIBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) HexShootBlockingHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call obj_shoot_blocking_at_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXSHOOTBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) HexSightBlockingHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call obj_sight_blocking_at_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 4;
	RunHookScript(HOOK_HEXSIGHTBLOCKING);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) ItemDamageHook() {
	__asm {
		HookBegin;
		mov args[0], eax;  // min
		mov args[4], edx;  // max
		mov args[8], edi;  // weapon
		mov args[12], ecx; // critter
		mov args[16], esi; // type
		mov args[20], ebp; // non-zero for weapon melee attack (add to min/max melee damage)
		pushadc;
	}
	argCount = 6;

	if (args[2] == 0) {  // weapon arg
		args[4] += 8;    // type arg
	}

	RunHookScript(HOOK_ITEMDAMAGE);

	__asm popadc;
	if (cRet > 0) {
		__asm mov eax, rets[0];     // set min
		if (cRet > 1) {
			__asm mov edx, rets[4]; // set max
		} else {
			HookEnd;
			__asm retn;             // no calc random
		}
	}
	HookEnd;
	__asm jmp roll_random_;
}

// 4.x backport
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
		cmp  dword ptr [esp + 0x1C + 4], ANIM_fire_burst;
		jl   skip;
		cmp  dword ptr [esp + 0x1C + 4], ANIM_fire_continuous;
		jg   skip;
		mov  ecx, 3;               // hook type burst
skip:
		xchg eax, edx;
		push eax;                  // rounds in attack
		call AmmoCostHook_Script;  // edx - weapon
		retn;
	}
}

void __stdcall KeyPressHook(DWORD* dxKey, bool pressed, DWORD vKey) {
	if (!IsGameLoaded()) {
		return;
	}
	BeginHook();
	argCount = 3;
	args[0] = (DWORD)pressed;
	args[1] = *dxKey;
	args[2] = vKey;
	RunHookScript(HOOK_KEYPRESS);
	if (cRet != 0) *dxKey = rets[0];
	EndHook();
}

void __stdcall MouseClickHook(DWORD button, bool pressed) {
	if (!IsGameLoaded()) {
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
		HookBegin;
		mov args[0], eax;  // user
		mov args[4], edx;  // target
		mov args[8], ebx;  // skill id
		mov args[12], ecx; // skill bonus
		pushadc;
	}

	argCount = 4;
	RunHookScript(HOOK_USESKILL);

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
		jmp skill_use_;
	}
}

static void __declspec(naked) StealCheckHook() {
	__asm {
		HookBegin;
		mov args[0], eax;  // thief
		mov args[4], edx;  // target
		mov args[8], ebx;  // item
		mov args[12], ecx; // is planting
		pushadc;
	}

	argCount = 4;
	RunHookScript(HOOK_STEAL);

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
		jmp skill_check_stealing_;
	}
}

// 4.x backport
static long __fastcall PerceptionRangeHook_Script(TGameObj* watcher, TGameObj* target, int type) {
	long result = IsWithinPerception(watcher, target);

	BeginHook();
	argCount = 4;

	args[0] = (DWORD)watcher;
	args[1] = (DWORD)target;
	args[2] = result;
	args[3] = type;

	RunHookScript(HOOK_WITHINPERCEPTION);

	if (cRet > 0) result = rets[0];
	EndHook();

	return result;
}

// Implementation of is_within_perception_ engine function with the hook
long __fastcall sf_is_within_perception(TGameObj* watcher, TGameObj* target) { // TODO: add type arg
	return PerceptionRangeHook_Script(watcher, target, 0);
}

static void __declspec(naked) PerceptionRangeHook() {
	__asm {
		push ecx;
		push 0;
		mov  ecx, eax;
		call PerceptionRangeHook_Script;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) PerceptionRangeSeeHook() {
	__asm {
		push ecx;
		push 1;
		mov  ecx, eax;
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
		mov  ecx, eax;
		call PerceptionRangeHook_Script;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) PerceptionSearchTargetHook() {
	__asm {
		push ecx;
		push 3;
		mov  ecx, eax;
		call PerceptionRangeHook_Script;
		pop  ecx;
		retn;
	}
}

// 4.x backport
static int __fastcall SwitchHandHook_Script(TGameObj* item, TGameObj* itemReplaced, DWORD addr) {
	if (itemReplaced && GetItemType(itemReplaced) == item_type_weapon && GetItemType(item) == item_type_ammo) {
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

// This hack is called when an armor is dropped into the armor slot at inventory screen
static void __declspec(naked) UseArmorHack() {
	static const DWORD UseArmorHack_back = 0x4713AF; // normal operation (old 0x4713A9)
	static const DWORD UseArmorHack_skip = 0x471481; // skip code, prevent wearing armor
	__asm {
		mov  ecx, ds:[FO_VAR_i_worn];             // replacement item (override code)
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
		cmp dword ptr ds:[FO_VAR_curr_stack], 0;
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
		__asm call obj_drop_;
	}
	__asm retn;

/* for only caps multi drop */
capsMultiDrop:
	if (dropResult == -1) {
		nextHookDropSkip = 1;
		__asm call item_remove_mult_;
		__asm retn;
	}
	__asm add esp, 4;
	__asm jmp InvenActionObjDropRet; // no caps drop
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
		mov  dword ptr ds:[FO_VAR_dropped_explosive], ebp; // overwritten engine code (ebp = 1)
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

static void __declspec(naked) DropIntoContainerHack() {
	static const DWORD DropIntoContainer_back = 0x47649D; // normal operation
	static const DWORD DropIntoContainer_skip = 0x476503;
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

static void __declspec(naked) DropIntoContainerHandSlotHack() {
	static const DWORD DropIntoContainerRet = 0x471481;
	__asm {
		call drop_into_container_;
		jmp  DropIntoContainerRet;
	}
}

static void __declspec(naked) DropAmmoIntoWeaponHook() {
	//static const DWORD DropAmmoIntoWeaponHack_back = 0x47658D; // proceed with reloading
	static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
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
		cmp  edi, ds:[FO_VAR_obj_dude];
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
		cmp  dword ptr ds:[FO_VAR_curr_stack], 0;
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

// 4.x backport
/* Common InvenWield script hooks */
static long __fastcall InvenWieldHook_Script(TGameObj* critter, TGameObj* item, long slot, long isWield, long isRemove) {
	if (!isWield) {
		// for the critter, the right slot is always the active slot
		if (slot == INVEN_TYPE_LEFT_HAND && critter != *ptr_obj_dude) return 1;
		// check the current active slot for the player
		if (slot != INVEN_TYPE_WORN && critter == *ptr_obj_dude) {
			long _slot = (slot != INVEN_TYPE_LEFT_HAND);
			if (_slot != *ptr_itemCurrentItem) return 1; // item in non-active slot
		}
	}
	BeginHook();
	argCount = 5;

	args[0] = (DWORD)critter;
	args[1] = (DWORD)item;
	args[2] = slot;
	args[3] = isWield; // unwield/wield event
	args[4] = isRemove;

	RunHookScript(HOOK_INVENWIELD);

	long result = (cRet == 0 || rets[0] == -1);
	EndHook();

	return result; // 1 - use engine handler
}

static __declspec(noinline) bool InvenWieldHook_ScriptPart(long isWield, long isRemove = 0) {
	argCount = 5;

	args[3] = isWield; // unwield/wield event
	args[4] = isRemove;

	RunHookScript(HOOK_INVENWIELD);

	bool result = (cRet == 0 || rets[0] == -1);
	EndHook();

	return result; // true - use engine handler
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
	if (args[2] != INVEN_TYPE_RIGHT_HAND && GetItemType((TGameObj*)args[1]) != item_type_armor) {
		args[2] = INVEN_TYPE_LEFT_HAND;
	}
	InvenWieldHook_ScriptPart(1); // wield event

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
		mov args[0], eax; // critter
		mov args[8], edx; // slot
		pushadc;
	}

	// set slot
	if (args[2] == 0) { // left hand slot?
		args[2] = INVEN_TYPE_LEFT_HAND;
	}

	// get item
	args[1] = (DWORD)GetItemPtrSlot((TGameObj*)args[0], (InvenType)args[2]);

	InvenWieldHook_ScriptPart(0); // unwield event

	__asm {
		test al, al;
		popadc;
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
		pushadc;
	}

	// set slot
	if (args[2] & ObjectFlag::Right_Hand) {       // right hand slot
		args[2] = INVEN_TYPE_RIGHT_HAND;
	} else if (args[2] & ObjectFlag::Left_Hand) { // left hand slot
		args[2] = INVEN_TYPE_LEFT_HAND;
	} else {
		args[2] = INVEN_TYPE_WORN;                // armor slot
	}

	InvenWieldHook_ScriptPart(0, 1); // unwield event (armor by default)

	// engine handler is not overridden
	__asm {
		popadc;
		jmp  correctFidForRemovedItem_;
	}
}

static void __declspec(naked) item_drop_all_hack() {
	using namespace ObjectFlag;
	__asm {
		mov  ecx, 1;
		push eax;
		mov  [esp + 0x40 - 0x2C + 8], ecx; // itemIsEquipped
		push ecx; // remove event
		push 0;   // unwield event
		inc  ecx; // INVEN_TYPE_LEFT_HAND (2)
		test ah, Left_Hand >> 24;
		jnz  skip;
		test ah, Worn >> 24;
		setz cl; // set INVEN_TYPE_WORN or INVEN_TYPE_RIGHT_HAND
skip:
		push ecx;      // slot
		mov  edx, esi; // item
		mov  ecx, edi; // critter
		call InvenWieldHook_Script;
		//mov  [esp + 0x40 - 0x2C + 8], eax; // itemIsEquipped (eax - hook return result)
		pop  eax;
		retn;
	}
}

// called from bugfixes for obj_drop_
void __declspec(naked) InvenUnwield_HookDrop() { // ecx - critter, edx - item
	using namespace Fields;
	using namespace ObjectFlag;
	__asm {
		pushadc;
		mov  eax, INVEN_TYPE_LEFT_HAND;
		test byte ptr [edx + flags + 3], Left_Hand >> 24;
		jnz  isLeft;
		test byte ptr [edx + flags + 3], Worn >> 24;
		setz al;  // set INVEN_TYPE_WORN or INVEN_TYPE_RIGHT_HAND
isLeft:
		push 1;   // remove event
		push 0;   // unwield event
		push eax; // slot
		call InvenWieldHook_Script; // ecx - critter, edx - item
		// engine handler is not overridden
		popadc;
		retn;
	}
}

// called from bugfixes for op_move_obj_inven_to_obj_
void __declspec(naked) InvenUnwield_HookMove() { // eax - item, edx - critter
	__asm {
		pushadc;
		mov  ecx, edx;
		mov  edx, eax;
		push 1;   // remove event
		xor  eax, eax;
		push eax; // unwield event
		push eax; // slot
		call InvenWieldHook_Script; // ecx - critter, edx - item
		// engine handler is not overridden
		popadc;
		retn;
	}
}

// called when unwelding dude weapon and armor
static void __declspec(naked) op_move_obj_inven_to_obj_hook() {
	__asm {
		cmp  eax, ds:[FO_VAR_obj_dude];
		je   runHook;
		jmp  item_move_all_;
runHook:
		push eax;
		push edx;
		mov  ecx, eax; // keep source
		mov  edx, ds:[FO_VAR_itemCurrentItem]; // get player's active slot
		test edx, edx;
		jz   left;
		call inven_right_hand_;
		jmp  skip;
left:
		call inven_left_hand_;
skip:
		test eax, eax;
		jz   noWeapon;
		push 1; // remove event
		push 0; // unwield event
		mov  ebx, INVEN_TYPE_LEFT_HAND;
		sub  ebx, edx;
		push ebx;      // slot: INVEN_TYPE_LEFT_HAND or INVEN_TYPE_RIGHT_HAND
		mov  edx, eax; // weapon
		mov  ebx, ecx; // keep source
		call InvenWieldHook_Script; // ecx - source
		// engine handler is not overridden
noWeapon:
		mov  edx, [esp + 0x30 - 0x20 + 12]; // armor
		test edx, edx;
		jz   noArmor;
		xor  eax, eax;
		push 1;   // remove event
		push eax; // unwield event
		push eax; // slot: INVEN_TYPE_WORN
		mov  ecx, ebx; // source
		call InvenWieldHook_Script;
		// engine handler is not overridden
noArmor:
		pop  edx;
		pop  eax;
		jmp  item_move_all_;
	}
}

// internal function implementation with hook
long __stdcall CorrectFidForRemovedItem_wHook(TGameObj* critter, TGameObj* item, long flags) {
	long result = 1;
	if (!hooks[HOOK_INVENWIELD].empty()) {
		long slot = INVEN_TYPE_WORN;
		if (flags & ObjectFlag::Right_Hand) {       // right hand slot
			slot = INVEN_TYPE_RIGHT_HAND;
		} else if (flags & ObjectFlag::Left_Hand) { // left hand slot
			slot = INVEN_TYPE_LEFT_HAND;
		}
		result = InvenWieldHook_Script(critter, item, slot, 0, 0);
	}
	if (result) CorrectFidForRemovedItem(critter, item, flags);
	return result;
}

// 4.x backport
static unsigned long previousGameMode = 0;

void __stdcall GameModeChangeHook(DWORD exit) {
	BeginHook();
	argCount = 2;
	args[0] = exit;
	args[1] = previousGameMode;
	RunHookScript(HOOK_GAMEMODECHANGE);
	EndHook();

	previousGameMode = GetLoopFlags();
}
// END HOOKS

static void HookCommon_Reset() {
	previousGameMode = 0;
}

DWORD __stdcall GetHSArgCount() {
	return argCount;
}

DWORD __stdcall GetHSArg() {
	return (cArg == argCount) ? 0 : args[cArg++];
}

void __stdcall SetHSArg(DWORD id, DWORD value) {
	if (id < argCount) args[id] = value;
}

DWORD* __stdcall GetHSArgs() {
	return args;
}

DWORD __stdcall GetHSArgAt(DWORD id) {
	return args[id];
}

void __stdcall SetHSReturn(DWORD value) {
	if (cRetTmp < maxRets) {
		rets[cRetTmp++] = value;
	}
	if (cRetTmp > cRet) {
		cRet = cRetTmp;
	}
}

void __stdcall RegisterHook(TProgram* script, int id, int procNum, bool specReg) {
	if (id >= numHooks || (id > HOOK_INVENWIELD && id < HOOK_GAMEMODECHANGE)) return;
	for (std::vector<sHookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister
			return;
		}
	}
	if (procNum == 0) return; // prevent registration to first location in procedure when reusing "unregister" method

	sScriptProgram *prog = GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Global script: %08x registered as hook ID %d\n", DL_HOOK, script, id);
		sHookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;

		std::vector<sHookScript>::const_iterator c_it = hooks[id].cend();
		if (specReg) {
			c_it = hooks[id].cbegin();
			hooksInfo[id].hsPosition++;
		}
		hooks[id].insert(c_it, hook);
	}
}

static void LoadHookScript(const char* name, int id) {
	//if (id >= numHooks || IsGameScript(name)) return;

	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);

	if (DbAccess(filename)) {
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

static void HookScriptInit() {
	dlogr("Loading hook scripts:", DL_HOOK|DL_INIT);

	char* mask = "scripts\\hs_*.int";
	char** filenames;
	DbGetFileList(mask, &filenames);

	LoadHookScript("hs_tohit", HOOK_TOHIT);
	const DWORD toHitHkAddr[] = {
		0x421686, // combat_safety_invalidate_weapon_func_
		0x4231D9, // check_ranged_miss_
		0x42331F, // shoot_along_path_
		0x4237FC, // compute_attack_
		0x424379, // determine_to_hit_
		0x42438D, // determine_to_hit_no_range_
		0x42439C, // determine_to_hit_from_tile_
		0x42679A  // combat_to_hit_
	};
	HookCalls(ToHitHook, toHitHkAddr);

	LoadHookScript("hs_afterhitroll", HOOK_AFTERHITROLL);
	MakeCall(0x423893, AfterHitRollHook);

	LoadHookScript("hs_calcapcost", HOOK_CALCAPCOST);
	const DWORD calcApCostHkAddr[] = {
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
	};
	HookCalls(CalcApCostHook, calcApCostHkAddr);
	MakeCall(0x478083, CalcApCostHook2);

	LoadHookScript("hs_deathanim1", HOOK_DEATHANIM1);
	LoadHookScript("hs_deathanim2", HOOK_DEATHANIM2);
	HookCall(0x4109DE, CalcDeathAnimHook);  // show_damage_to_object_
	const DWORD calcDeathAnim2HkAddr[] = {
		0x410981, // show_damage_to_object_
		0x4109A1, // show_damage_to_object_
		0x4109BF  // show_damage_to_object_
	};
	HookCalls(CalcDeathAnim2Hook, calcDeathAnim2HkAddr);

	LoadHookScript("hs_combatdamage", HOOK_COMBATDAMAGE);
	const DWORD computeDamageHkAddr[] = {
		0x42326C, // check_ranged_miss()
		0x4233E3, // shoot_along_path() - for extra burst targets
		0x423AB7, // compute_attack()
		0x423BBF, // compute_attack()
//		0x423DE7, // compute_explosion_on_extras()
		0x423E69, // compute_explosion_on_extras()
		0x424220, // attack_crit_failure()
		0x4242FB  // attack_crit_failure()
	};
	HookCalls(ComputeDamageHook, computeDamageHkAddr);
	MakeCall(0x423DEB, ComputeDamageHook); // compute_explosion_on_extras() - for the attacker

	LoadHookScript("hs_ondeath", HOOK_ONDEATH);
	MakeCall(0x42DA6D, OnDeathHook);  // critter_kill_
	HookCall(0x425161, OnDeathHook2); // damage_object_

	LoadHookScript("hs_findtarget", HOOK_FINDTARGET);
	HookCall(0x429143, FindTargetHook);

	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	const DWORD useObjOnHkAddr[] = {0x49C606, 0x473619};
	HookCalls(UseObjOnHook, useObjOnHkAddr);
	// the following hooks allows to catch drug use of AI and from action cursor
	const DWORD drugUseObjOnHkAddr[] = {
		0x4285DF, // ai_check_drugs
		0x4286F8, // ai_check_drugs
		0x4287F8, // ai_check_drugs
		0x473573  // inven_action_cursor
	};
	HookCalls(Drug_UseObjOnHook, drugUseObjOnHkAddr);

	LoadHookScript("hs_removeinvenobj", HOOK_REMOVEINVENOBJ);
	MakeJump(0x477492, RemoveObjHook); // old 0x477490

	LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	const DWORD barterPriceHkAddr[] = {
		0x474D4C, // barter_attempt_transaction_ (offers button)
		0x475735, // display_table_inventories_ (for party members)
		0x475762  // display_table_inventories_
	};
	HookCalls(BarterPriceHook, barterPriceHkAddr);
	const DWORD pcBarterPriceHkAddr[] = {0x4754F4, 0X47551A}; // display_table_inventories_
	HookCalls(PC_BarterPriceHook, pcBarterPriceHkAddr);
	HookCall(0x474D3F, OverrideCost_BarterPriceHook); // barter_attempt_transaction_ (just overrides cost of offered goods)

	LoadHookScript("hs_movecost", HOOK_MOVECOST);
	const DWORD moveCostHkAddr[] = {0x417665, 0x44B88A};
	HookCalls(MoveCostHook, moveCostHkAddr);

	LoadHookScript("hs_hexmoveblocking", HOOK_HEXMOVEBLOCKING);
	LoadHookScript("hs_hexaiblocking", HOOK_HEXAIBLOCKING);
	LoadHookScript("hs_hexshootblocking", HOOK_HEXSHOOTBLOCKING);
	LoadHookScript("hs_hexsightblocking", HOOK_HEXSIGHTBLOCKING);
	SafeWrite32(0x413979, (DWORD)&HexSightBlockingHook);
	const DWORD shootBlockingAddr[] = {0x4C1A88, 0x423178, 0x4232D4, 0x423B4D, 0x426CF8, 0x42A570};
	SafeWriteBatch<DWORD>((DWORD)&HexShootBlockingHook, shootBlockingAddr);
	SafeWrite32(0x42A0A4, (DWORD)&HexAIBlockingHook);
	MakeJump(0x48B848, HexMoveBlockingHook);

	LoadHookScript("hs_itemdamage", HOOK_ITEMDAMAGE);
	HookCall(0x478560, ItemDamageHook);

	LoadHookScript("hs_ammocost", HOOK_AMMOCOST);
	HookCall(0x423A7C, AmmoCostHook);

	LoadHookScript("hs_useobj", HOOK_USEOBJ);
	const DWORD useObjHkAddr[] = {0x42AEBF, 0x473607, 0x49C12E};
	HookCalls(UseObjHook, useObjHkAddr);

	LoadHookScript("hs_keypress", HOOK_KEYPRESS);
	LoadHookScript("hs_mouseclick", HOOK_MOUSECLICK);

	LoadHookScript("hs_useskill", HOOK_USESKILL);
	const DWORD useSkillHkAddr[] = {0x49C48F, 0x49D12E};
	HookCalls(UseSkillHook, useSkillHkAddr);

	LoadHookScript("hs_steal", HOOK_STEAL);
	const DWORD stealCheckHkAddr[] = {0x4749A2, 0x474A69};
	HookCalls(StealCheckHook, stealCheckHkAddr);

	LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	const DWORD perceptionRngHkAddr[] = {
		0x42B4ED,
		0x42BC87,
		0x42BC9F,
		0x42BD04
	};
	HookCalls(PerceptionRangeHook, perceptionRngHkAddr);
	HookCall(0x429157, PerceptionSearchTargetHook);
	HookCall(0x456BA2, PerceptionRangeSeeHook);
	HookCall(0x458403, PerceptionRangeHearHook);

	LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	const DWORD switchHandHkAddr[] = {
		0x4712E3, // left slot
		0x47136D  // right slot
	};
	HookCalls(SwitchHandHook, switchHandHkAddr);
	MakeJump(0x4713A9, UseArmorHack); // old 0x4713A3
	MakeJump(0x476491, DropIntoContainerHack);
	MakeJump(0x471338, DropIntoContainerHandSlotHack);
	MakeJump(0x4712AB, DropIntoContainerHandSlotHack);
	HookCall(0x471200, MoveInventoryHook);
	HookCall(0x476549, DropAmmoIntoWeaponHook); // old 0x476588
	const DWORD actionCurObjDropHkAddr[] = {
		0x473851, 0x47386F,
		0x47379A  // caps multi drop
	};
	HookCalls(InvenActionCursorObjDropHook, actionCurObjDropHkAddr);
	MakeCall(0x473807, InvenActionExplosiveDropHack, 1); // drop active explosives
	MakeCall(0x49B660, PickupObjectHack);
	SafeWrite32(0x49B665, 0x850FD285); // test edx, edx
	SafeWrite32(0x49B669, 0xC2);       // jnz  0x49B72F
	SafeWrite8(0x49B66E, 0xFE);        // cmp edi > cmp esi
	HookCall(0x471457, InvenPickupHook);

	LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	const DWORD invWieldFuncHkAddr[] = {
		0x47275E, // inven_wield_
		0x495FDF  // partyMemberCopyLevelInfo_
	};
	HookCalls(InvenWieldFuncHook, invWieldFuncHkAddr);
	const DWORD invUnwieldFuncHkAddr[] = {
		0x45967D, // op_metarule_
		0x472A5A, // inven_unwield_
		0x495F0B  // partyMemberCopyLevelInfo_
	};
	HookCalls(InvenUnwieldFuncHook, invUnwieldFuncHkAddr);
	const DWORD fidRemovedItemHkAddr[] = {
		0x45680C, // op_rm_obj_from_inven_
		0x45C4EA  // op_move_obj_inven_to_obj_
	};
	HookCalls(CorrectFidForRemovedItemHook, fidRemovedItemHkAddr);
	HookCall(0x45C4F6, op_move_obj_inven_to_obj_hook);
	MakeCall(0x4778AF, item_drop_all_hack, 3);

	LoadHookScript("hs_gamemodechange", HOOK_GAMEMODECHANGE);

	DbFreeFileList(&filenames, 0);

	dlogr("Finished loading hook scripts.", DL_HOOK|DL_INIT);
}

void HookScriptClear() {
	for (int i = 0; i < numHooks; i++) {
		hooks[i].clear();
	}
	std::memset(hooksInfo, 0, numHooks * sizeof(HooksPositionInfo));
	HookCommon_Reset();
}

void LoadHookScripts() {
	isGlobalScriptLoading = 1; // this should allow to register global exported variables
	HookScriptInit();
	initingHookScripts = 1;
	for (int i = 0; i < numHooks; i++) {
		if (!hooks[i].empty()) {
			hooksInfo[i].hasHsScript = true;
			InitScriptProgram(hooks[i][0].prog); // zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	isGlobalScriptLoading = 0;
	initingHookScripts = 0;
}

// run specific event procedure for all hook scripts
void __stdcall RunHookScriptsAtProc(DWORD procId) {
	for (int i = 0; i < numHooks; i++) {
		if (hooksInfo[i].hasHsScript /*&& !hooks[i][hooksInfo[i].hsPosition].isGlobalScript*/) {
			RunScriptProc(&hooks[i][hooksInfo[i].hsPosition].prog, procId); // run hs_*.int
		}
	}
}
