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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "Inventory.h"
#include "LoadGameHook.h"
#include "PartyControl.h"
#include "ScriptExtender.h"

#include "HookScripts.h"

namespace sfall
{

static const int maxArgs = 16;
static const int maxDepth = 8;
static const int numHooks = HOOK_COUNT;

struct HookScript {
	ScriptProgram prog;
	int callback; // proc number in script's proc table
	bool isGlobalScript; // false for hs_* scripts, true for gl* scripts
};

static std::vector<HookScript> hooks[numHooks];

DWORD initingHookScripts;

static DWORD args[maxArgs]; // current hook arguments
static DWORD oldargs[maxArgs * maxDepth];
static DWORD* argPtr;
static DWORD rets[16]; // current hook return values

static DWORD firstArg = 0;
static DWORD callDepth;
static DWORD lastCount[maxDepth];

static DWORD argCount;
static DWORD cArg; // how many arguments were taken by current hook script
static DWORD cRet; // how many return values were set by current hook script
static DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

#define hookbegin(a) __asm pushad __asm call BeginHook __asm popad __asm mov argCount, a
#define hookend __asm pushad __asm call EndHook __asm popad

static void _stdcall BeginHook() {
	if (callDepth <= maxDepth) {
		if (callDepth) {
			lastCount[callDepth - 1] = argCount;
			memcpy(&oldargs[maxArgs * (callDepth - 1)], args, maxArgs * sizeof(DWORD));
		}
		argPtr = args;
		for (DWORD i = 0; i < callDepth; i++) {
			argPtr += lastCount[i];
		}
	}
	callDepth++;
}

static void _stdcall EndHook() {
	callDepth--;
	if (callDepth && callDepth <= maxDepth) {
		argCount = lastCount[callDepth - 1];
		memcpy(args, &oldargs[maxArgs * (callDepth - 1)], maxArgs * sizeof(DWORD));
	}
}

static void _stdcall RunSpecificHookScript(HookScript *hook) {
	cArg = 0;
	cRetTmp = 0;
	if (hook->callback != -1) {
		fo::func::executeProcedure(hook->prog.ptr, hook->callback);
	} else {
		RunScriptProc(&hook->prog, fo::ScriptProc::start);
	}
}

static void _stdcall RunHookScript(DWORD hook) {
	if (hooks[hook].size()) {
		dlog_f("Running hook %d, which has %0d entries attached\n", DL_HOOK, hook, hooks[hook].size());
		cRet = 0;
		for (int i = hooks[hook].size() - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);
		}
	} else {
		cArg = 0;
		cRet = 0;
	}
}

// TODO: move specific hook scripts into separate files
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
		call fo::funcoffs::obj_pid_new_
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
aend:
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

static void __declspec(naked) ComputeDamageHook() {
	__asm {
		push edx;
		push ebx;
		push eax;
		call fo::funcoffs::compute_damage_
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
		hookbegin(11);
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
		call fo::funcoffs::qsort_;
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
		jl	defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call fo::funcoffs::protinst_use_item_on_
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
		jl	defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call fo::funcoffs::item_d_take_drug_;
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
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call fo::funcoffs::protinst_use_item_;
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
		call fo::funcoffs::barter_compute_value_;
		mov edx, ds:[FO_VAR_btable];
		mov args[8], eax;
		mov args[12], edx;
		xchg eax, edx;
		call fo::funcoffs::item_caps_total_;
		mov args[16], eax;
		mov eax, ds:[FO_VAR_btable]
		call fo::funcoffs::item_total_cost_;
		mov args[20], eax;
		mov eax, ds:[FO_VAR_ptable];
		mov args[24], eax;
		call fo::funcoffs::item_total_cost_;
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
		call fo::funcoffs::critter_compute_ap_from_distance_
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

static const DWORD _obj_blocking_at=0x48B84E;
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
		call fo::funcoffs::obj_ai_blocking_at_
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
		call fo::funcoffs::obj_shoot_blocking_at_
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
		call fo::funcoffs::obj_sight_blocking_at_
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
		call fo::funcoffs::roll_random_
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
		call fo::funcoffs::item_w_compute_ammo_cost_
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
		jmp end
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

void _stdcall KeyPressHook(DWORD dxKey, bool pressed, DWORD vKey) {
	if (!IsMapLoaded()) {
		return;
	}
	BeginHook();
	argCount = 3;
	args[0] = (DWORD)pressed;
	args[1] = dxKey;
	args[2] = vKey;
	RunHookScript(HOOK_KEYPRESS);
	EndHook();
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
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call fo::funcoffs::skill_use_
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
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call fo::funcoffs::skill_check_stealing_
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
		call fo::funcoffs::is_within_perception_
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

static int __stdcall SwitchHandHook2(fo::GameObject* item, fo::GameObject* itemReplaced, DWORD addr) {
	int tmp;
	if (itemReplaced && fo::func::item_get_type(itemReplaced) == 3 && fo::func::item_get_type(item) == 4) {
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
		call fo::funcoffs::switch_hand_;
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
  		mov eax, ds:[FO_VAR_i_worn]
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
		call fo::funcoffs::item_add_force_
skipcall:
		hookend;
		retn;
	}
}

static void _declspec(naked) invenWieldFunc_Hook() {
	using namespace fo;
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
		call fo::funcoffs::item_get_type_;
		cmp eax, item_type_armor;
		jz skip;
		mov args[8], 2; // INVEN_TYPE_LEFT_HAND
skip:		
		push HOOK_INVENWIELD;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end
defaulthandler:
		call fo::funcoffs::invenWieldFunc_
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
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end
defaulthandler:
		call fo::funcoffs::invenUnwieldFunc_;
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
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		jmp end
defaulthandler:
		call fo::funcoffs::correctFidForRemovedItem_;
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
	if (cArg == argCount) return 0;
	else return args[cArg++];
}

void _stdcall SetHSArg(DWORD id, DWORD value) {
	if(id<argCount) args[id]=value;
}

DWORD* _stdcall GetHSArgs() {
	return args;
}

void _stdcall SetHSReturn(DWORD d) {
	if (cRetTmp < 8) {
		rets[cRetTmp++] = d;
	}
	if (cRetTmp > cRet) {
		cRet = cRetTmp;
	}
}

void _stdcall RegisterHook(fo::Program* script, int id, int procNum) {
	if (id >= numHooks) return;
	for (std::vector<HookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister 
			return;
		}
	}
	ScriptProgram *prog = GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Global script %08x registered as hook id %d\n", DL_HOOK, script, id);
		HookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;
		hooks[id].push_back(hook);
	}
}

static void LoadHookScript(const char* name, int id) {
	if (id >= numHooks) return;

	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);
	if (fo::func::db_access(filename) && !IsGameScript(name)) {
		ScriptProgram prog;
		dlog(">", DL_HOOK);
		dlog(name, DL_HOOK);
		LoadScriptProgram(prog, name);
		if (prog.ptr) {
			dlogr(" Done", DL_HOOK);
			HookScript hook;
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
	MakeCall(0x423893, &AfterHitRollHook, true);

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
	MakeCall(0x478083, &CalcApCostHook2, false);

	LoadHookScript("hs_deathanim1", HOOK_DEATHANIM1);
	LoadHookScript("hs_deathanim2", HOOK_DEATHANIM2);
	HookCall(0x4109DE, &CalcDeathAnimHook);
	HookCall(0x410981, &CalcDeathAnimHook2);
	HookCall(0x4109A1, &CalcDeathAnimHook2);
	HookCall(0x4109BF, &CalcDeathAnimHook2);

	LoadHookScript("hs_combatdamage", HOOK_COMBATDAMAGE);
	HookCall(0x42326C, &ComputeDamageHook); // check_ranged_miss()
	HookCall(0x4233E3, &ComputeDamageHook); // shoot_along_path() - for extra burst targets
	HookCall(0x423AB7, &ComputeDamageHook); // compute_attack()
	HookCall(0x423BBF, &ComputeDamageHook); // compute_attack()
	HookCall(0x423DE7, &ComputeDamageHook); // compute_explosion_on_extras()
	HookCall(0x423E69, &ComputeDamageHook); // compute_explosion_on_extras()
	HookCall(0x424220, &ComputeDamageHook); // attack_crit_failure()
	HookCall(0x4242FB, &ComputeDamageHook); // attack_crit_failure()

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
	MakeCall(0x477490, &RemoveObjHook, true);

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
	MakeCall(0x48B848, &HexMBlockingHook, true);

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
	MakeCall(0x456BA2, &PerceptionRangeBonusHack, true);
	HookCall(0x458403, &PerceptionRangeHook);

	LoadHookScript("hs_inventorymove", HOOK_INVENTORYMOVE);
	HookCall(0x4712E3, &SwitchHandHook); // left slot
	HookCall(0x47136D, &SwitchHandHook); // right slot
	MakeCall(0x4713A3, &UseArmorHack, true);
	//HookCall(0x4711B3, &DropIntoContainerHook); 
	//HookCall(0x47147C, &DropIntoContainerHook); 
	HookCall(0x471200, &MoveInventoryHook); 
	//HookCall(0x4712C7, &DropAmmoIntoWeaponHook);
	//HookCall(0x471351, &DropAmmoIntoWeaponHook);
	MakeCall(0x476588, &DropAmmoIntoWeaponHack, true);

	LoadHookScript("hs_invenwield", HOOK_INVENWIELD);
	HookCall(0x47275E, &invenWieldFunc_Hook);
	HookCall(0x495FDF, &invenWieldFunc_Hook);
	HookCall(0x45967D, &invenUnwieldFunc_Hook);
	HookCall(0x472A5A, &invenUnwieldFunc_Hook);
	HookCall(0x495F0B, &invenUnwieldFunc_Hook);
	HookCall(0x45680C, &correctFidForRemovedItem_Hook);
	HookCall(0x45C4EA, &correctFidForRemovedItem_Hook);

	dlogr("Finished loading hook scripts", DL_HOOK|DL_INIT);
}

void HookScriptClear() {
	for(int i = 0; i < numHooks; i++) {
		hooks[i].clear();
	}
}

void LoadHookScripts() {
	isGlobalScriptLoading = 1; // this should allow to register global exported variables
	HookScriptInit2();
	initingHookScripts = 1;
	for (int i = 0; i < numHooks; i++) {
		if (hooks[i].size()) {
			InitScriptProgram(hooks[i][0].prog);// zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	isGlobalScriptLoading = 0;
	initingHookScripts = 0;
}

// run specific event procedure for all hook scripts
void _stdcall RunHookScriptsAtProc(DWORD procId) {
	for (int i = 0; i < numHooks; i++) {
		if (hooks[i].size() > 0 && !hooks[i][0].isGlobalScript) {
			RunScriptProc(&hooks[i][0].prog, procId);
		}
	}
}

void HookScripts::init() {
	OnKeyPressed() += KeyPressHook;
	OnMouseClick() += MouseClickHook;
}

}
