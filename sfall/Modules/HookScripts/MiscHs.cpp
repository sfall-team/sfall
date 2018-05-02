#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Karma.h"
#include "Common.h"

#include "MiscHs.h"

// Misc. hook scripts 
namespace sfall
{

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
		jmp end;
defaulthandler:
		call fo::funcoffs::protinst_use_item_on_;
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
		jmp end;
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
		mov eax, ds:[FO_VAR_btable];
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
		jmp end;
defaulthandler:
		call fo::funcoffs::skill_use_;
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
		jmp end;
defaulthandler:
		call fo::funcoffs::skill_check_stealing_;
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
		call fo::funcoffs::is_within_perception_;
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
		mov dword ptr[esp + 16], 1;
		jmp PerceptionRangeBonusHack_skip_blocking_check;
nevermind:
		jmp PerceptionRangeBonusHack_back;
	}
}

static constexpr long maxGasAmount = 80000;
static void CarTravelHookScript() {
	BeginHook();
	argCount = 2;
	// calculate vanilla speed
	int carSpeed = 3;
	if (fo::func::game_get_global_var(fo::GVAR_CAR_BLOWER)) {
		carSpeed += 1;
	}
	if (fo::func::game_get_global_var(fo::GVAR_NEW_RENO_CAR_UPGRADE)) {
		carSpeed += 1;
	}
	if (fo::func::game_get_global_var(fo::GVAR_NEW_RENO_SUPER_CAR)) {
		carSpeed += 3;
	}
	// calculate vanilla fuel consumption
	int originalGas = fo::var::carGasAmount;
	fo::var::carGasAmount = maxGasAmount;
	fo::func::wmCarUseGas(100);
	int consumption = maxGasAmount - fo::var::carGasAmount;

	// run script
	args[0] = carSpeed;
	args[1] = consumption;
	RunHookScript(HOOK_CARTRAVEL);

	// override travel speed
	if (cRet > 0 && static_cast<long>(rets[0]) >= 0) {
		carSpeed = rets[0];
	}
	// move car on map
	for (int i = 0; i < carSpeed; ++i) {
		fo::func::wmPartyWalkingStep();
	}

	// override fuel consumption
	if (cRet > 1) {
		consumption = static_cast<long>(rets[1]);
	}
	// consume fuel
	fo::var::carGasAmount = max(originalGas - consumption, 0);

	EndHook();
}

static const DWORD CarTravelHack_back = 0x4BFF43;
static void __declspec(naked) CarTravelHack() {
	__asm {
		pushad;
		call CarTravelHookScript;
		popad;
		jmp CarTravelHack_back;
	}
}

static int newGVarValue;
static void _stdcall GlobalVarHookScript(DWORD number, int value) {
	int old = fo::var::game_global_vars[number];

	BeginHook();
	argCount = 2;
	args[0] = number;
	args[1] = value;
	RunHookScript(HOOK_SETGLOBALVAR);
	EndHook();

	if (cRet == 1) value = rets[0];

	if (number == fo::GVAR_PLAYER_REPUTATION && displayKarmaChanges) {
		int diff = value - old;
		if (diff != 0) Karma::DisplayKarma(diff);
	}
	newGVarValue = value;
}

static void __declspec(naked) SetGlobalVarHook() {
	__asm {
		pushad;
		push edx; // value
		push eax; // number
		call GlobalVarHookScript;
		popad;
		mov edx, newGVarValue;
		jmp fo::funcoffs::game_set_global_var_;
	}
}

static void _stdcall RestTimerHookScript() {
	int addrHook;
	__asm {
		mov args[0], eax;
		mov addrHook, ebx;
	}
	addrHook -= 5;

	BeginHook();
	argCount = 2;
	args[1] = (addrHook == 0x499DF2 || addrHook == 0x499BE0) ? 1 : 0;
	RunHookScript(HOOK_TIMERREST);
	EndHook();
}

static void __declspec(naked) RestTimerLoopHook() {
	__asm {
		pushad;
		mov ebx, [esp + 32];
		call RestTimerHookScript;
		popad;
		cmp cRet, 1;
		jl	skip;
		cmp rets[0], 1;
		jnz	skip;
		mov edi, rets[0];
skip:
		jmp fo::funcoffs::set_game_time_;
	}
}
void InitMiscHookScripts() {
	LoadHookScript("hs_useobjon", HOOK_USEOBJON);
	HookCalls(UseObjOnHook, { 0x49C606, 0x473619 });

	// the following hooks allows to catch drug use of AI and from action cursor
	HookCalls(UseObjOnHook_item_d_take_drug, {
		0x4285DF, // ai_check_drugs
		0x4286F8, // ai_check_drugs
		0x4287F8, // ai_check_drugs
		0x473573 // inven_action_cursor
	});

	LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	HookCalls(BarterPriceHook, {
		0x474D4C,
		0x475735,
		0x475762
	});

	LoadHookScript("hs_useobj", HOOK_USEOBJ);
	HookCalls(UseObjHook, { 0x42AEBF, 0x473607, 0x49C12E });

	LoadHookScript("hs_useskill", HOOK_USESKILL);
	HookCalls(UseSkillHook, { 0x49C48F, 0x49D12E });

	LoadHookScript("hs_steal", HOOK_STEAL);
	HookCalls(StealCheckHook, { 0x4749A2, 0x474A69 });

	LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	HookCalls(PerceptionRangeHook, {
		0x429157,
		0x42B4ED,
		0x42BC87,
		0x42BC9F,
		0x42BD04,
		0x458403
	});
	MakeJump(0x456BA2, PerceptionRangeBonusHack);

	LoadHookScript("hs_cartravel", HOOK_CARTRAVEL);
	MakeJump(0x4BFEF1, CarTravelHack);
	BlockCall(0x4BFF6E); // vanilla wnCarUseGas(100) call

	LoadHookScript("hs_setglobalvar", HOOK_SETGLOBALVAR);
	HookCall(0x455A6D, SetGlobalVarHook);
	
	LoadHookScript("hs_timerrest", HOOK_TIMERREST);
	HookCalls(RestTimerLoopHook, { 0x499B4B, 0x499BE0, 0x499D2C, 0x499DF2 });

}

}
