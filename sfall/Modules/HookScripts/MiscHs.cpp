#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Karma.h"
#include "..\LoadGameHook.h"
#include "Common.h"

#include "MiscHs.h"

namespace sfall
{

// The hook is executed twice when entering the barter screen and after transaction: the first time is for the player; the second time is for NPC
static DWORD __fastcall BarterPriceHook_Script(register fo::GameObject* source, register fo::GameObject* target, DWORD callAddr) {
	bool barterIsParty = (*fo::ptr::dialog_target_is_party != 0);
	long computeCost = fo::func::barter_compute_value(source, target);

	BeginHook();
	argCount = 10;

	args[0] = (DWORD)source;
	args[1] = (DWORD)target;
	args[2] = !barterIsParty ? computeCost : 0;

	fo::GameObject* bTable = (fo::GameObject*)*fo::ptr::btable;
	args[3] = (DWORD)bTable;
	args[4] = fo::func::item_caps_total(bTable);
	args[5] = fo::func::item_total_cost(bTable);

	fo::GameObject* pTable = (fo::GameObject*)*fo::ptr::ptable;
	args[6] = (DWORD)pTable;

	long pcCost = 0;
	if (barterIsParty) {
		args[7] = pcCost;
		pcCost = fo::func::item_total_weight(pTable);
	} else {
		args[7] = pcCost = fo::func::item_total_cost(pTable);
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
		push -1;                                       // address on call stack
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
		jmp fo::funcoffs::skill_use_;
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
		jmp fo::funcoffs::skill_check_stealing_;
	}
}

static __forceinline long PerceptionRangeHook_Script(fo::GameObject* watcher, fo::GameObject* target, int type, long result) {
	BeginHook();
	argCount = 4;

	args[0] = (DWORD)watcher;
	args[1] = (DWORD)target;
	args[2] = result; // engine result
	args[3] = type;

	RunHookScript(HOOK_WITHINPERCEPTION);

	if (cRet > 0) result = rets[0];
	EndHook();

	return result;
}

long PerceptionRangeHook_Invoke(fo::GameObject* watcher, fo::GameObject* target, long type, long result) {
	if (!HookScripts::HookHasScript(HOOK_WITHINPERCEPTION)) return result;
	return PerceptionRangeHook_Script(watcher, target, type, result);
}

static long __fastcall PerceptionRange_Hook(fo::GameObject* watcher, fo::GameObject* target, int type) {
	return PerceptionRangeHook_Script(watcher, target, type, fo::func::is_within_perception(watcher, target));
}

static void __declspec(naked) PerceptionRangeHook() {
	__asm {
		push ecx;
		push 0;
		mov  ecx, eax;
		call PerceptionRange_Hook;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) PerceptionRangeSeeHook() {
	__asm {
		push ecx;
		push 1;
		mov  ecx, eax;
		call PerceptionRange_Hook;
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
		call PerceptionRange_Hook;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) PerceptionSearchTargetHook() {
	__asm {
		push ecx;
		push 3;
		mov  ecx, eax;
		call PerceptionRange_Hook;
		pop  ecx;
		retn;
	}
}

void Inject_BarterPriceHook() {
	const DWORD barterPriceHkAddr[] = {
		0x474D4C, // barter_attempt_transaction_ (offers button)
		0x475735, // display_table_inventories_ (for party members)
		0x475762  // display_table_inventories_
	};
	HookCalls(BarterPriceHook, barterPriceHkAddr);
	const DWORD pcBarterPriceHkAddr[] = {0x4754F4, 0X47551A}; // display_table_inventories_
	HookCalls(PC_BarterPriceHook, pcBarterPriceHkAddr);
	HookCall(0x474D3F, OverrideCost_BarterPriceHook); // barter_attempt_transaction_ (just overrides cost of offered goods)
}

void Inject_UseSkillHook() {
	const DWORD useSkillHkAddr[] = {0x49C48F, 0x49D12E};
	HookCalls(UseSkillHook, useSkillHkAddr);
}

void Inject_StealCheckHook() {
	const DWORD stealCheckHkAddr[] = {0x4749A2, 0x474A69};
	HookCalls(StealCheckHook, stealCheckHkAddr);
}

void Inject_WithinPerceptionHook() {
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
}

void InitMiscHookScripts() {
	HookScripts::LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	HookScripts::LoadHookScript("hs_useskill", HOOK_USESKILL);
	HookScripts::LoadHookScript("hs_steal", HOOK_STEAL);
	HookScripts::LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
}

}
