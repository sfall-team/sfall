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

#include "HookScripts.h"
#include "ScriptExtender.h"
#include "FalloutEngine.h"
#include "PartyControl.h"
#include "Inventory.h"
//#include "vector9x.cpp"
#include <vector>
#include <string>
#include "Logging.h"

#define MAXDEPTH (8)
static const int numHooks = 25;

struct sHookScript {
	sScriptProgram prog;
	int callback; // proc number in script's proc table
	bool isGlobalScript; // false for hs_* scripts, true for gl* scripts
};

static std::vector<sHookScript> hooks[numHooks];

DWORD InitingHookScripts;

static DWORD args[16]; // current hook arguments
static DWORD oldargs[8*MAXDEPTH];
static DWORD* argPtr;
static DWORD rets[16]; // current hook return values

static DWORD firstArg=0;
static DWORD callDepth;
static DWORD lastCount[MAXDEPTH];

static DWORD ArgCount; 
static DWORD cArg; // how many arguments were taken by current hook script
static DWORD cRet; // how many return values were set by current hook script
static DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

#define hookbegin(a) __asm pushad __asm call BeginHook __asm popad __asm mov ArgCount, a
#define hookend __asm pushad __asm call EndHook __asm popad

static void _stdcall BeginHook() {
	if(callDepth <= MAXDEPTH) {
		if(callDepth) {
			lastCount[callDepth-1]=ArgCount;
			memcpy(&oldargs[8*(callDepth-1)], args, 8*sizeof(DWORD));
		}
		argPtr=args;
		for(DWORD i=0;i<callDepth;i++) argPtr+=lastCount[i];
	}
	callDepth++;
}

static void _stdcall EndHook() {
	callDepth--;
	if(callDepth && callDepth <= MAXDEPTH) {
		ArgCount=lastCount[callDepth-1];
		memcpy(args, &oldargs[8*(callDepth-1)], 8*sizeof(DWORD));
	}
}

static void _stdcall RunSpecificHookScript(sHookScript *hook) {
	cArg=0;
	cRetTmp=0;
	if (hook->callback != -1)
		RunScriptProcByNum(hook->prog.ptr, hook->callback);
	else
		RunScriptProc(&hook->prog, start_proc);
}
static void _stdcall RunHookScript(DWORD hook) {
	if(hooks[hook].size()) {
#ifdef TRACE
	char buf[256];
	sprintf_s(buf, "Running hook %d, which has %0d entries attached", hook, hooks[hook].size());
	dlogr(buf, DL_HOOK);
#endif
		cRet=0;
		for(int i=hooks[hook].size()-1;i>=0;i--) RunSpecificHookScript(&hooks[hook][i]);
	} else {
		cArg=0;
		cRet=0;
	}
}

static const DWORD ToHitAddr=0x4243A8;
static void __declspec(naked) ToHitHook() {
	__asm {
		hookbegin(4);
		mov args[4], eax;
		mov args[8], ebx;
		mov args[12], ecx;
		push [esp+8];
		push [esp+8];
		call ToHitAddr;
		mov args[0], eax;
		pushad;
		push 0;
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
static const DWORD AfterHitRollAddr=0x423898;
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
		push 1;
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

static const DWORD item_w_mp_cost_=0x478B24;
static void __declspec(naked) CalcApCostHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call item_w_mp_cost_;
		mov args[12], eax;
		pushad;
		push 2;
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
		push 2;
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

static const DWORD ObjPidNewAddr=0x489C9C;
static const DWORD ObjEraseObjectAddr=0x48B0FC;
static const DWORD DeathAnimAddr=0x41060C;
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
		push 3;
		call RunHookScript;
		cmp cRet, 1;
		jl end1;
		sub esp, 4;
		mov edx, rets[0];
		mov args[0], edx;
		mov eax, esp;
		call ObjPidNewAddr;
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
		call DeathAnimAddr;
		mov args[16], eax;
		mov eax, args[16];
		mov ArgCount, 5;
		pushad;
		push 4;
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
		call ObjEraseObjectAddr;
aend:
		pop eax;
		hookend;
		retn 8;
	}
}
static const DWORD check_death_=0x410814;
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
		push 4;
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
static const DWORD CombatDamageAddr=0x4247B8;
static void __declspec(naked) CombatDamageHook() {
	__asm {
		push edx;
		push ebx;
		push eax;
		call CombatDamageAddr;
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
		push 5;
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
static const DWORD critter_kill=0x42DA64;
static void __declspec(naked) OnDeathHook() {
	__asm {
		hookbegin(1);
		mov args[0], eax;
		call critter_kill;
		pushad;
		push 6;
		call RunHookScript;
		popad;
		hookend;
		retn;
	}
}
static const DWORD OnDeathHook2Ret=0x4944DC;
static void __declspec(naked) OnDeathHook2() {
	__asm {
		hookbegin(1);
		mov args[0], esi;
		call OnDeathHook2Ret;
		pushad;
		push 6;
		call RunHookScript;
		popad;
		hookend;
		retn;
	}
}

static const DWORD _qsort=0x4F05B6;
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
		push 7
		call RunHookScript;
		popad;
		cmp cRet, 4;
		jge cont;
		call _qsort;
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
static const DWORD _protinst_use_item_on=0x49C3CC;
static void __declspec(naked) UseObjOnHook() {
	__asm {
		hookbegin(3);
		mov args[0], edx; // target
		mov args[4], eax; // user
		mov args[8], ebx; // object
		pushad;
		push 8;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call _protinst_use_item_on;
end:
		hookend;
		retn;
	}
}
static const DWORD item_d_take_drug_=0x479F60;
static void __declspec(naked) UseObjOnHook_item_d_take_drug() {
	__asm {
		hookbegin(3);
		mov args[0], eax; // target
		mov args[4], eax; // user
		mov args[8], edx; // object
		pushad;
		push 8; // useobjon
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call item_d_take_drug_;
end:
		hookend;
		retn;
	}
}
static const DWORD protinst_use_item_=0x49BF38;
static void __declspec(naked) UseObjHook() {
	__asm {
		hookbegin(2);
		mov args[0], eax; // user
		mov args[4], edx; // object
		pushad;
		push 18;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end;
defaulthandler:
		call protinst_use_item_;
end:
		hookend;
		retn;
	}
}
static const DWORD RemoveObjHookRet=0x477497;
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
		push 9;
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
static const DWORD _barter_compute_value=0x474B2C;
static const DWORD _item_caps_total=0x47A6A8;
static const DWORD _item_total_cost=0x477DAC;
static void __declspec(naked) BarterPriceHook() {
	__asm {
		hookbegin(6);
		mov args[0], eax;
		mov args[4], edx;
		call _barter_compute_value;
		mov edx, ds:[0x59E944];
		mov args[8], eax;
		mov args[12], edx;
		xchg eax, edx;
		call _item_caps_total;
		mov args[16], eax;
		mov eax, ds:[0x59E944];
		call _item_total_cost;
		mov args[20], eax;
		mov eax, edx;
		pushad;
		push 10;
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
static const DWORD _critter_compute_ap_from_distance=0x42E62C;
static void __declspec(naked) MoveCostHook() {
	__asm {
		hookbegin(3);
		mov args[0], eax;
		mov args[4], edx;
		call _critter_compute_ap_from_distance;
		mov args[8], eax;
		pushad;
		push 11;
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
		push 12;
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
static const DWORD _obj_ai_blocking_at=0x48BA20;
static void __declspec(naked) HexABlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call _obj_ai_blocking_at;
		mov args[12], eax;
		pushad;
		push 13;
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
static const DWORD _obj_shoot_blocking_at=0x48B930;
static void __declspec(naked) HexShootBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call _obj_shoot_blocking_at;
		mov args[12], eax;
		pushad;
		push 14;
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
static const DWORD _obj_sight_blocking_at=0x48BB88;
static void __declspec(naked) HexSightBlockingHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax;
		mov args[4], edx;
		mov args[8], ebx;
		call _obj_sight_blocking_at;
		mov args[12], eax;
		pushad;
		push 15;
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
static const DWORD _roll_random=0x4A30C0;
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
		push 16;
		call RunHookScript;
		popad;
		cmp cRet, 0;
		je runrandom;
		mov eax, rets[0];
		cmp cRet, 1;
		je end;
		mov edx, rets[4];
runrandom:
		call _roll_random;
end:
		hookend;
		retn;
	}
}
static const DWORD item_w_compute_ammo_cost_ = 0x4790AC; // signed int aWeapon<eax>, int *aRoundsSpent<edx>

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

		push 17;
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
void _stdcall KeyPressHook( DWORD dxKey, bool pressed, DWORD vKey )
{
	BeginHook();
	ArgCount = 3;
	args[0] = (DWORD)pressed;
	args[1] = dxKey;
	args[2] = vKey;
	RunHookScript(19);
	InventoryKeyPressedHook(dxKey, pressed, vKey);
	EndHook();
}
void _stdcall MouseClickHook(DWORD button, bool pressed) {
	BeginHook();
	ArgCount = 2;
	args[0] = (DWORD)pressed;
	args[1] = button;
	RunHookScript(20);
	EndHook();
}

static const DWORD skill_use_=0x4AAD08;
static void __declspec(naked) UseSkillHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // user
		mov args[4], edx; // target
		mov args[8], ebx; // skill id
		mov args[12], ecx; // skill bonus
		pushad;
		push 21;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call skill_use_;
end:
		hookend;
		retn;
	}
}

static const DWORD skill_check_stealing_=0x4ABBE4;
static void __declspec(naked) StealCheckHook() {
	__asm {
		hookbegin(4);
		mov args[0], eax; // thief
		mov args[4], edx; // target
		mov args[8], ebx; // item
		mov args[12], ecx; // is planting
		pushad;
		push 22;
		call RunHookScript;
		popad;
		cmp cRet, 1;
		jl	defaulthandler;
		cmp rets[0], -1;
		je defaulthandler;
		mov eax, rets[0];
		jmp end
defaulthandler:
		call skill_check_stealing_;
end:
		hookend;
		retn;
	}
}

static const DWORD is_within_perception_=0x42BA04;
static void __declspec(naked) PerceptionRangeHook() {
	__asm {
		hookbegin(3);
		mov args[0], eax; // watcher
		mov args[4], edx; // target
		call is_within_perception_;
		mov args[8], eax; // check result
		pushad;
		push 23;
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
	ArgCount = 3;
	args[0] = (addr < 0x47136D) ? 1 : 2;
	args[1] = (DWORD)item;
	args[2] = (DWORD)itemReplaced;
	RunHookScript(24); // moveinventory
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
static const DWORD switch_hand_ = 0x4714E0;
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
static const DWORD* i_worn = (DWORD*)0x59E954; // item in armor slot
// This hack is called when an armor is dropped into the armor slot at inventory screen
static void _declspec(naked) UseArmorHack() {
	__asm {
		cmp eax, 0;
		jne skip; // not armor
		hookbegin(3);
		mov args[0], 3;
		mov eax, [esp+24]; // item
		mov args[4], eax;
		mov eax, i_worn;
		mov eax, [eax];
		mov args[8], eax;
		pushad;
		push 24;
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

//static const DWORD drop_into_container_ = 0x476464;
static const DWORD item_add_force_ = 0x4772B8;
static void _declspec(naked) MoveInventoryHook() {
	__asm {
		hookbegin(3);
		mov args[0], 0;
		mov args[4], edx;
		mov args[8], 0; // no item being replaced here..
		pushad;
		push 24;
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

//static const DWORD drop_ammo_into_weapon_ = 0x47650C;
static const DWORD DropAmmoIntoWeaponHack_back = 0x47658D; // proceed with reloading
static const DWORD DropAmmoIntoWeaponHack_return = 0x476643;
static const DWORD item_w_can_reload_ = 0x478874;
static void _declspec(naked) DropAmmoIntoWeaponHack() {
	__asm {
		hookbegin(3);
		mov args[0], 4;
		mov eax, [esp];
		mov args[4], eax;
		mov args[8], ebp;
		pushad;
		push 24;
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
	return ArgCount;
}
DWORD _stdcall GetHSArg() {
	if(cArg==ArgCount) return 0;
	else return args[cArg++];
}
void _stdcall SetHSArg(DWORD id, DWORD value) {
	if(id<ArgCount) args[id]=value;
}
DWORD* _stdcall GetHSArgs() {
	return args;
}
void _stdcall SetHSReturn(DWORD d) {
	if (cRetTmp < 8) rets[cRetTmp++]=d;
	if (cRetTmp > cRet)
		cRet = cRetTmp;
}
void _stdcall RegisterHook( DWORD script, DWORD id, DWORD procNum )
{
	if (id >= numHooks) return;
	for (std::vector<sHookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister 
			return;
		}
	}
	sScriptProgram *prog = GetGlobalScriptProgram(script);
	if (prog) {
#ifdef TRACE
		dlog_f( "Global script %8x registered as hook id %d ", DL_HOOK, script, id);
#endif
		sHookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;
		hooks[id].push_back(hook);
	}
}
#define LoadHookScript(a,b) _LoadHookScript("data\\scripts\\hs_" a ".int", b)
static void _LoadHookScript(const char* path, int id) {
	if(id>=numHooks) return;
	WIN32_FIND_DATA file;
	HANDLE h;

	h = FindFirstFileA(path, &file);
	if(h != INVALID_HANDLE_VALUE) {
		sScriptProgram prog;
		dlog("Loading hook script: ", DL_HOOK);
		dlogr(path, DL_HOOK);
		char* fName = file.cFileName;
		fName[strlen(fName) - 4] = 0;
		LoadScriptProgram(prog, fName);
		FindClose(h);
		if (prog.ptr) {
			sHookScript hook;
			hook.prog = prog;
			hook.callback = -1;
			hook.isGlobalScript = false;
			hooks[id].push_back(hook);
			AddProgramToMap(prog);
		}
	}
}
static void HookScriptInit2() {
	dlogr("Initing hook scripts", DL_HOOK|DL_INIT);

	LoadHookScript("tohit", 0);
	HookCall(0x421686, &ToHitHook);
	HookCall(0x4231D9, &ToHitHook);
	HookCall(0x42331F, &ToHitHook);
	HookCall(0x4237FC, &ToHitHook);
	HookCall(0x424379, &ToHitHook);
	HookCall(0x42438D, &ToHitHook);
	HookCall(0x42439C, &ToHitHook);
	HookCall(0x42679A, &ToHitHook);

	LoadHookScript("afterhitroll", 1);
	MakeCall(0x423893, &AfterHitRollHook, true);

	LoadHookScript("calcapcost", 2);
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

	LoadHookScript("deathanim1", 3);
	LoadHookScript("deathanim2", 4);
	HookCall(0x4109DE, &CalcDeathAnimHook);
	HookCall(0x410981, &CalcDeathAnimHook2);
	HookCall(0x4109A1, &CalcDeathAnimHook2);
	HookCall(0x4109BF, &CalcDeathAnimHook2);

	LoadHookScript("combatdamage", 5);
	HookCall(0x42326C, &CombatDamageHook); // check_ranged_miss()
	HookCall(0x4233E3, &CombatDamageHook); // shoot_along_path() - for extra burst targets
	HookCall(0x423AB7, &CombatDamageHook); // compute_attack()
	HookCall(0x423BBF, &CombatDamageHook); // compute_attack()
	HookCall(0x423DE7, &CombatDamageHook); // compute_explosion_on_extras()
	HookCall(0x423E69, &CombatDamageHook); // compute_explosion_on_extras()
	HookCall(0x424220, &CombatDamageHook); // attack_crit_failure()
	HookCall(0x4242FB, &CombatDamageHook); // attack_crit_failure()

	LoadHookScript("ondeath", 6);
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

	LoadHookScript("findtarget", 7);
	HookCall(0x429143, &FindTargetHook);

	LoadHookScript("useobjon", 8);
	HookCall(0x49C606, &UseObjOnHook);
	HookCall(0x473619, &UseObjOnHook);
	// the following hooks allows to catch drug use of AI and from action cursor
	HookCall(0x4285DF, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x4286F8, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x4287F8, &UseObjOnHook_item_d_take_drug); // ai_check_drugs
	HookCall(0x473573, &UseObjOnHook_item_d_take_drug); // inven_action_cursor

	LoadHookScript("removeinvenobj", 9);
	MakeCall(0x477490, &RemoveObjHook, true);

	LoadHookScript("barterprice", 10);
	HookCall(0x474D4C, &BarterPriceHook);
	HookCall(0x475735, &BarterPriceHook);
	HookCall(0x475762, &BarterPriceHook);

	LoadHookScript("movecost", 11);
	HookCall(0x417665, &MoveCostHook);
	HookCall(0x44B88A, &MoveCostHook);

	LoadHookScript("hexmoveblocking", 12);
	LoadHookScript("hexaiblocking", 13);
	LoadHookScript("hexshootblocking", 14);
	LoadHookScript("hexsightblocking", 15);
	SafeWrite32(0x413979, (DWORD)&HexSightBlockingHook);
	SafeWrite32(0x4C1A88, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x423178, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x4232D4, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x423B4D, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x426CF8, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x42A570, (DWORD)&HexShootBlockingHook);
	SafeWrite32(0x42A0A4, (DWORD)&HexABlockingHook);
	MakeCall(0x48B848, &HexMBlockingHook, true);

	LoadHookScript("itemdamage", 16);
	HookCall(0x478560, &ItemDamageHook);

	LoadHookScript("ammocost", 17);
	HookCall(0x423A7C, &AmmoCostHook);

	LoadHookScript("useobj", 18);
	HookCall(0x42AEBF, &UseObjHook);
	HookCall(0x473607, &UseObjHook);
	HookCall(0x49C12E, &UseObjHook);

	LoadHookScript("keypress", 19);
	LoadHookScript("mouseclick", 20);

	LoadHookScript("useskill", 21);
	HookCall(0x49C48F, &UseSkillHook);
	HookCall(0x49D12E, &UseSkillHook);

	LoadHookScript("steal", 22);
	HookCall(0x4749A2, &StealCheckHook);
	HookCall(0x474A69, &StealCheckHook);

	LoadHookScript("withinperception", 23);
	HookCall(0x429157, &PerceptionRangeHook);
	HookCall(0x42B4ED, &PerceptionRangeHook);
	HookCall(0x42BC87, &PerceptionRangeHook);
	HookCall(0x42BC9F, &PerceptionRangeHook);
	HookCall(0x42BD04, &PerceptionRangeHook);
	MakeCall(0x456BA2, &PerceptionRangeBonusHack, true);
	HookCall(0x458403, &PerceptionRangeHook);

	LoadHookScript("inventorymove", 24);
	HookCall(0x4712E3, &SwitchHandHook); // left slot
	HookCall(0x47136D, &SwitchHandHook); // right slot
	MakeCall(0x4713A3, &UseArmorHack, true);
	//HookCall(0x4711B3, &DropIntoContainerHook); 
	//HookCall(0x47147C, &DropIntoContainerHook); 
	HookCall(0x471200, &MoveInventoryHook); 
	//HookCall(0x4712C7, &DropAmmoIntoWeaponHook);
	//HookCall(0x471351, &DropAmmoIntoWeaponHook);
	MakeCall(0x476588, &DropAmmoIntoWeaponHack, true);

	dlogr("Completed hook script init", DL_HOOK|DL_INIT);

}

void HookScriptClear() {
	for(int i = 0; i < numHooks; i++) {
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

