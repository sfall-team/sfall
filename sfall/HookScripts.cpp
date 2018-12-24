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
	if (hooks[hook].size()) {
		if (callDepth > 8) {
			DebugPrintf("\n[SFALL] The hook ID: %d cannot be executed.", hook);
			dlog_f("The hook %d cannot be executed due to exceeded depth limit\n", DL_MAIN, hook);
			return;
		}
		currentRunHook = hook;
		dlog_f("Running hook %d, which has %0d entries attached, depth: %d\n", DL_HOOK, hook, hooks[hook].size(), callDepth);
		for (int i = hooks[hook].size() - 1; i >= 0; i--) {
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
		mov	ebx, [esp+60]
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

static void __declspec(naked) CombatDamageHook() {
	__asm {
		push edx;
		push ebx;
		push eax;
		call compute_damage_;
		pop edx;

		//zero damage insta death criticals fix
		mov ebx, [edx+0x2c];
		test ebx, ebx;
		jnz hookscript;
		mov ebx, [edx+0x30];
		test bl, 0x80;
		jz hookscript;
		inc dword ptr ds:[edx+0x2c];
hookscript:
		hookbegin(12);
		mov ebx, [edx+0x20];
		mov args[0x00], ebx;
		mov ebx, [edx+0x00];
		mov args[0x04], ebx;
		mov ebx, [edx+0x2c];
		mov args[0x08], ebx;
		mov ebx, [edx+0x10];
		mov args[0x0c], ebx;
		mov ebx, [edx+0x30];
		mov args[0x10], ebx;
		mov ebx, [edx+0x14];
		mov args[0x14], ebx;
		mov ebx, [edx+0x08];
		mov args[0x18], ebx;
		mov ebx, [edx+0x28];
		mov args[0x1c], ebx;
		pop ebx; // roll result
		mov args[0x20], ebx;
		pop ebx; // num rounds
		mov args[0x24], ebx;
		mov ebx, [edx+0x34]; // knockback value
		mov args[0x28], ebx;
		mov ebx, [edx+0x04]; // attack type
		mov args[0x2c], ebx;

		pushad;
		push HOOK_COMBATDAMAGE;
		call RunHookScript;
		popad;

		cmp cRet, 1;
		jl end;
		mov ebx, rets[0x00];
		mov [edx+0x2c], ebx;
		cmp cRet, 2;
		jl end;
		mov ebx, rets[0x04];
		mov [edx+0x10], ebx;
		cmp cRet, 3;
		jl end;
		mov ebx, rets[0x08];
		mov [edx+0x30], ebx;
		cmp cRet, 4;
		jl end;
		mov ebx, rets[0x0c];
		mov [edx+0x14], ebx;
		cmp cRet, 5;
		jl end;
		mov ebx, rets[0x10];
		mov [edx+0x34], ebx; // knockback
end:
		hookend;
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

static void __declspec(naked) FindTargetHook() {
	__asm {
		hookbegin(5);
		mov args[0], esi; //attacker
		mov edi, [eax+0];
		mov args[4], edi;
		mov edi, [eax+4];
		mov args[8], edi;
		mov edi, [eax+8];
		mov args[12], edi;
		mov edi, [eax+12];
		mov args[16], edi;
		pushad;
		push HOOK_FINDTARGET;
		call RunHookScript;
		popad;
		cmp cRet, 4;
		jge cont;
		call qsort_;
		jmp end;
cont:
		mov edi, rets[0];
		mov [eax+0], edi;
		mov edi, rets[4];
		mov [eax+4], edi;
		mov edi, rets[8];
		mov [eax+8], edi;
		mov edi, rets[12];
		mov [eax+12], edi;
end:
		hookend;
		retn;
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
		push ecx;
		mov ecx, [esp+4];
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		mov args[12], ecx;
		pushad;
		push HOOK_REMOVEINVENOBJ;
		call RunHookScript;
		popad;
		hookend;
		push esi;
		push edi;
		push ebp;
		sub esp, 0xc;
		jmp RemoveObjHookRet;
	}
}

static void __declspec(naked) BarterPriceHook() {
	__asm {
		hookbegin(9);
		mov args[0], eax;
		mov args[4], edx;
		call barter_compute_value_;
		mov edx, ds:[_btable]
		mov args[8], eax;
		mov args[12], edx;
		xchg eax, edx;
		call item_caps_total_;
		mov args[16], eax;
		mov eax, ds:[_btable];
		call item_total_cost_;
		mov args[20], eax;
		mov eax, ds:[_ptable];
		mov args[24], eax;
		call item_total_cost_;
		mov args[28], eax;
		xor eax, eax;
		mov edx, [esp]; // check offers button
		cmp edx, 0x474D51; // last address on call stack
		jne skip;
		inc eax;
skip:
		mov args[32], eax;
		mov eax, args[8];
		pushad;
		push HOOK_BARTERPRICE;
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

static void __declspec(naked) AmmoCostHook_internal() {
	__asm {
		pushad;
		mov args[0], eax; //weapon
		mov ebx, [edx]
		mov args[4], ebx; //rounds in attack
		call item_w_compute_ammo_cost_;
		cmp eax, -1
		je fail
		mov ebx, [edx]
		mov args[8], ebx; //rounds as computed by game

		push HOOK_AMMOCOST;
		call RunHookScript;
		popad;
		cmp cRet, 0;
		je end;
		mov eax, rets[0]
		mov [edx], eax; // override result
		mov eax, 0
		jmp end;
fail:
		popad
end:
		hookend;
		retn;
	}
}

static void __declspec(naked) AmmoCostHook() {
	__asm {
		hookbegin(4);
		mov args[12], 0 // type of hook
		jmp AmmoCostHook_internal;
	}
}

void __declspec(naked) AmmoCostHookWrapper() {
	__asm {
		hookbegin(4);
		push eax;
		mov eax, [esp+8]; // hook type
		mov args[12], eax;
		pop eax;
		call AmmoCostHook_internal;
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

static void __declspec(naked) PerceptionRangeHook() {
	__asm {
		hookbegin(3);
		mov args[0], eax; // watcher
		mov args[4], edx; // target
		call is_within_perception_;
		mov args[8], eax; // check result
		pushad;
		push HOOK_WITHINPERCEPTION;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	end;
		mov eax, rets[0];
end:
		hookend;
		retn;
	}
}

// jmp here, not call
static const DWORD PerceptionRangeBonusHack_back = 0x456BA7;
static const DWORD PerceptionRangeBonusHack_skip_blocking_check = 0x456BDC;
static void __declspec(naked) PerceptionRangeBonusHack() {
	__asm {
		call PerceptionRangeHook;
		cmp eax, 2;
		jne nevermind;
		mov dword ptr [esp+16], 1;
		jmp PerceptionRangeBonusHack_skip_blocking_check;
nevermind:
		jmp PerceptionRangeBonusHack_back;
	}
}

static int __stdcall SwitchHandHook2(TGameObj* item, TGameObj* itemReplaced, DWORD addr) {
	int tmp;
	if (itemReplaced && ItemGetType(itemReplaced) == 3 && ItemGetType(item) == 4) {
		return -1; // to prevent inappropriate hook call after dropping ammo on weapon
	}
	BeginHook();
	argCount = 3;
	args[0] = (addr < 0x47136D) ? 1 : 2;
	args[1] = (DWORD)item;
	args[2] = (DWORD)itemReplaced;
	RunHookScript(HOOK_INVENTORYMOVE); // moveinventory
	tmp = PartyControl_SwitchHandHook(item);
	if (tmp != -1) {
		cRetTmp = 0;
		SetHSReturn(tmp);
	}
	EndHook();
	if (cRet > 0)
		return rets[0];
	return -1;
}

/*
	This hook is called every time an item is placed into either hand slot via inventory screen drag&drop
	If switch_hand_ function is not called, item is not placed anywhere (it remains in main inventory)
*/
static void _declspec(naked) SwitchHandHook() {
	_asm {
		pushad;
		mov ecx, eax;
		mov eax, [esp+32]; // back address
		push eax;
		mov edx, [edx];
		push edx; // other item
		push ecx; // item being moved
		call SwitchHandHook2;
		cmp eax, -1;
		popad;
		jne skip;
		call switch_hand_;
skip:
		retn;
	}
}

static const DWORD UseArmorHack_back = 0x4713A9; // normal operation
static const DWORD UseArmorHack_skip = 0x471481; // skip code, prevent wearing armor
// This hack is called when an armor is dropped into the armor slot at inventory screen
static void _declspec(naked) UseArmorHack() {
	__asm {
		cmp eax, 0;
		jne skip; // not armor
		hookbegin(3);
		mov args[0], 3;
		mov eax, [esp+24]; // item
		mov args[4], eax;
  		mov eax, ds:[_i_worn]
		mov args[8], eax;
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl back;
		cmp rets[0], -1;
		jne skip;
back:
		hookend;
		jmp UseArmorHack_back;
skip:
		hookend;
		jmp UseArmorHack_skip;
	}
}

static void _declspec(naked) MoveInventoryHook() {
	__asm {
		hookbegin(3);
		mov args[0], 0;
		mov args[4], edx;
		mov args[8], 0; // no item being replaced here..
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl skipcheck;
		cmp rets[0], -1;
		jne skipcall;
skipcheck:
		call item_add_force_;
skipcall:
		hookend;
		retn;
	}
}

static void _declspec(naked) invenWieldFunc_Hook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], ebx; // slot
		mov args[12], 1; // wield flag
		pushad;
		cmp ebx, 1; // right hand slot?
		je skip;
		mov eax, edx;
		call item_get_type_;
		cmp eax, item_type_armor;
		jz skip;
		mov args[8], 2; // INVEN_TYPE_LEFT_HAND
skip:
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end;
defaulthandler:
		call invenWieldFunc_;
end:
		hookend;
		retn;
	}
}

// called when unwielding weapons
static void _declspec(naked) invenUnwieldFunc_Hook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // critter
		mov args[4], 0; // item
		mov args[8], edx; // slot
		mov args[12], 0; // wield flag
		cmp edx, 0; // left hand slot?
		jne notlefthand;
		mov args[8], 2; // left hand
notlefthand:
		pushad;
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end;
defaulthandler:
		call invenUnwieldFunc_;
end:
		hookend;
		retn;
	}
}

static void _declspec(naked) correctFidForRemovedItem_Hook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // critter
		mov args[4], edx; // item
		mov args[8], 0; // slot
		mov args[12], 0 // wield flag (armor by default)
		test ebx, 0x02000000; // right hand slot?
		jz notrighthand;
		mov args[8], 1; // right hand
notrighthand:
		test ebx, 0x01000000; // left hand slot?
		jz notlefthand;
		mov args[8], 2; // left hand
notlefthand:
		pushad;
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end;
defaulthandler:
		call correctFidForRemovedItem_;
end:
		hookend;
		retn;
	}
}

static const DWORD DropAmmoIntoWeaponHack_back = 0x47658D; // proceed with reloading
static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
static void _declspec(naked) DropAmmoIntoWeaponHack() {
	__asm {
		hookbegin(3);
		mov args[0], 4;
		mov eax, [esp];
		mov args[4], eax;
		mov args[8], ebp;
		pushad;
		push HOOK_INVENTORYMOVE;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl proceedreloading;
		cmp rets[0], -1;
		jne donothing;
proceedreloading:
		hookend;
		mov ebx, 1; // overwritten code
		jmp DropAmmoIntoWeaponHack_back;
donothing:
		hookend;
		mov eax, 0;
		jmp DropAmmoIntoWeaponHack_return;
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
	sScriptProgram *prog = GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Global script %08x registered as hook id %d\n", DL_HOOK, script, id);
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
		dlog(">", DL_HOOK);
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
	dlogr("Loading hook scripts", DL_HOOK|DL_INIT);

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
	HookCall(0x42326C, &CombatDamageHook); // check_ranged_miss()
	HookCall(0x4233E3, &CombatDamageHook); // shoot_along_path() - for extra burst targets
	HookCall(0x423AB7, &CombatDamageHook); // compute_attack()
	HookCall(0x423BBF, &CombatDamageHook); // compute_attack()
	HookCall(0x423DE7, &CombatDamageHook); // compute_explosion_on_extras()
	HookCall(0x423E69, &CombatDamageHook); // compute_explosion_on_extras()
	HookCall(0x424220, &CombatDamageHook); // attack_crit_failure()
	HookCall(0x4242FB, &CombatDamageHook); // attack_crit_failure()

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
	HookCall(0x429143, &FindTargetHook);

	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	HookCall(0x49C606, &UseObjOnHook);
	HookCall(0x473619, &UseObjOnHook);
	// the following hooks allows to catch drug use of AI and from action cursor
	HookCall(0x4285DF, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x4286F8, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x4287F8, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x473573, &UseObjOnHook_item_d_take_drug); // inven_action_cursor

	LoadHookScript("hs_removeinvenobj", HOOK_REMOVEINVENOBJ);
	MakeJump(0x477490, RemoveObjHook);

	LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	HookCall(0x474D4C, &BarterPriceHook);
	HookCall(0x475735, &BarterPriceHook);
	HookCall(0x475762, &BarterPriceHook);

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
	HookCall(0x423A7C, &AmmoCostHook);

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
	HookCall(0x429157, &PerceptionRangeHook);
	HookCall(0x42B4ED, &PerceptionRangeHook);
	HookCall(0x42BC87, &PerceptionRangeHook);
	HookCall(0x42BC9F, &PerceptionRangeHook);
	HookCall(0x42BD04, &PerceptionRangeHook);
	MakeJump(0x456BA2, PerceptionRangeBonusHack);
	HookCall(0x458403, &PerceptionRangeHook);

	LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	HookCall(0x4712E3, &SwitchHandHook); // left slot
	HookCall(0x47136D, &SwitchHandHook); // right slot
	MakeJump(0x4713A3, UseArmorHack);
	//HookCall(0x4711B3, &DropIntoContainerHook);
	//HookCall(0x47147C, &DropIntoContainerHook);
	HookCall(0x471200, &MoveInventoryHook);
	//HookCall(0x4712C7, &DropAmmoIntoWeaponHook);
	//HookCall(0x471351, &DropAmmoIntoWeaponHook);
	MakeJump(0x476588, DropAmmoIntoWeaponHack);

	LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	HookCall(0x47275E, &invenWieldFunc_Hook);
	HookCall(0x495FDF, &invenWieldFunc_Hook);
	HookCall(0x45967D, &invenUnwieldFunc_Hook);
	HookCall(0x472A5A, &invenUnwieldFunc_Hook);
	HookCall(0x495F0B, &invenUnwieldFunc_Hook);
	HookCall(0x45680C, &correctFidForRemovedItem_Hook);
	HookCall(0x45C4EA, &correctFidForRemovedItem_Hook);

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
		if (hooks[i].size()) {
			InitScriptProgram(hooks[i][0].prog);// zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	isGlobalScriptLoading = 0;
	InitingHookScripts = 0;
}

// run specific event procedure for all hook scripts
void _stdcall RunHookScriptsAtProc(DWORD procId) {
	for (int i = 0; i < numHooks; i++) {
		if (hooks[i].size() > 0 && !hooks[i][0].isGlobalScript) {
			RunScriptProc(&hooks[i][0].prog, procId);
		}
	}
}

