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

static void __declspec(naked) SneakCheckHook() {
	__asm {
		HookBegin;
		mov esi, ds:[FO_VAR_sneak_working];
		mov args[0], esi; // 1 = successful sneak
		mov args[4], eax; // timer
		mov args[8], edx; // critter
		pushadc;
	}

	argCount = 3;
	RunHookScript(HOOK_SNEAK);

	__asm {
		popadc;
		cmp  cRet, 1;
		jb   skip;
		mov  esi, rets[0];
		mov  ds:[FO_VAR_sneak_working], esi;
		cmova eax, rets[4]; // override timer
skip:
		HookEnd;
		jmp  fo::funcoffs::queue_add_;
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

static const long maxGasAmount = 80000;

static void CarTravelHook_Script() {
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
	int originalGas = *fo::ptr::carGasAmount;
	*fo::ptr::carGasAmount = maxGasAmount;
	fo::func::wmCarUseGas(100);
	int consumption = maxGasAmount - *fo::ptr::carGasAmount;

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
	*fo::ptr::carGasAmount = max(0, originalGas - consumption); // Note: the reverse breaks the result in VS2010

	EndHook();
}

static void __declspec(naked) CarTravelHack() {
	static const DWORD CarTravelHack_back = 0x4BFF43;
	__asm {
		pushad;
		call CarTravelHook_Script;
		popad;
		jmp CarTravelHack_back;
	}
}

static long __fastcall GlobalVarHook_Script(register DWORD number, register int value) {
	int old = (*fo::ptr::game_global_vars)[number];

	if (IsGameLoaded() && HookScripts::HookHasScript(HOOK_SETGLOBALVAR)) { // IsGameLoaded - don't execute hook until loading sfall scripts
		BeginHook();
		argCount = 2;
		args[0] = number;
		args[1] = value;

		RunHookScript(HOOK_SETGLOBALVAR);
		if (cRet > 0) value = rets[0];
		EndHook();
	}
	if (number == fo::GVAR_PLAYER_REPUTATION && displayKarmaChanges) {
		int diff = value - old;
		if (diff != 0) Karma::DisplayKarma(diff);
	}
	return value;
}

static void __declspec(naked) SetGlobalVarHook() {
	__asm {
		push eax;
		push ecx;
		push edx;
		mov  ecx, eax;             // number
		call GlobalVarHook_Script; // edx - value
		pop  edx;
		pop  ecx;
		cmp  eax, edx;             // if return value != set value
		cmovne edx, eax;
		pop  eax;
		jmp  fo::funcoffs::game_set_global_var_;
	}
}

static int restTicks;

static long __fastcall RestTimerHook_Script(DWORD hours, DWORD minutes, DWORD gameTime, DWORD addrHook) {
	addrHook -= 5;

	BeginHook();
	argCount = 4;

	args[0] = gameTime;
	args[2] = hours;
	args[3] = minutes;

	if (addrHook == 0x499CA1 || addrHook == 0x499B63) {
		args[0] = restTicks;
		args[1] = -1;
	} else {
		restTicks = args[0];
		args[1] = (addrHook == 0x499DF2 || (args[2] == 0 && addrHook == 0x499BE0)) ? 1 : 0;
	}
	RunHookScript(HOOK_RESTTIMER);

	long result = -1;
	if (cRet > 0) {
		result = (rets[0] != 0) ? 1 : 0;
	}
	EndHook();

	return result;
}

static void __declspec(naked) RestTimerLoopHook() {
	__asm {
		pushadc;
		mov  edx, [esp + 16 + 0x44]; // minutes_
		mov  ecx, [esp + 16 + 0x40]; // hours_
		push [esp + 12];             // addrHook
		push eax;                    // gameTime
		call RestTimerHook_Script;
		pop  ecx;
		pop  edx;
		test eax, eax;   // result >= 0
		cmovge edi, eax; // return 1 to interrupt resting
		pop  eax;
		jmp  fo::funcoffs::set_game_time_;
	}
}

static void __declspec(naked) RestTimerEscapeHook() {
	__asm {
		mov  edi, 1;    // engine code
		cmp  eax, 0x1B; // ESC ASCII code
		jnz  skip;
		pushadc;
		mov  edx, [esp + 16 + 0x44]; // minutes_
		mov  ecx, [esp + 16 + 0x40]; // hours_
		push [esp + 12];             // addrHook
		push eax;                    // gameTime
		call RestTimerHook_Script;
		pop  ecx;
		pop  edx;
		test eax, eax;   // result >= 0
		cmovge edi, eax; // return 0 for cancel ESC key
		pop  eax;
skip:
		retn;
	}
}

static int __fastcall ExplosiveTimerHook_Script(DWORD &type, DWORD item, DWORD time) {
	BeginHook();
	argCount = 3;

	args[0] = time;
	args[1] = item;
	args[2] = (type == 11) ? fo::ROLL_FAILURE : fo::ROLL_SUCCESS;

	RunHookScript(HOOK_EXPLOSIVETIMER);

	time = -1;
	if (cRet > 0 && (long)rets[0] >= 0) {
		time = min(rets[0], 18000); // max 30 minutes
		if (cRet > 1) {
			int typeRet = rets[1];
			if (typeRet >= fo::ROLL_CRITICAL_FAILURE && typeRet <= fo::ROLL_CRITICAL_SUCCESS) {
				type = (typeRet > fo::ROLL_FAILURE) ? 8 : 11; // returned new type (8 = SUCCESS, 11 = FAILURE)
			}
		}
	}
	EndHook();
	return time;
}

static void __declspec(naked) ExplosiveTimerHook() {
	__asm {
		push eax; // time in ticks for queue_add_
		push edx;
		//-------
		push ecx;
		mov  ecx, esp;                  // ptr to type
		push edi;                       // time in ticks (w/o failure penalty)
		call ExplosiveTimerHook_Script; // ecx - type, edx - item
		pop  ecx;
		pop  edx;
		cmp  eax, -1; // return new time in ticks
		jne  skip;
		pop  eax;
		jmp  end;
skip:
		add  esp, 4;
end:
		jmp  fo::funcoffs::queue_add_;
	}
}

// Hook does not work for scripted encounters and meeting Horrigan
static long __fastcall EncounterHook_Script(long encounterMapID, long eventType, long encType) {
	BeginHook();
	argCount = 3;

	args[0] = eventType; // 1 - enter local map from the world map
	args[1] = encounterMapID;
	args[2] = (encType == 3); // 1 - special random encounter

	RunHookScript(HOOK_ENCOUNTER);

	if (cRet) {
		encounterMapID = rets[0];
		if (encounterMapID < -1) encounterMapID = -1;
		if (eventType == 0 && cRet > 1 && encounterMapID != -1 && rets[1] == 1) { // 1 - cancel the encounter and load the specified map
			encounterMapID = -encounterMapID - 2; // map number in negative value
		}
		if (encounterMapID < 0) {
			// set the coordinates of the last encounter
			fo::var::setInt(FO_VAR_old_world_xpos) = *fo::ptr::world_xpos;
			fo::var::setInt(FO_VAR_old_world_ypos) = *fo::ptr::world_ypos;
		}
	}
	EndHook();
	return encounterMapID;
}

static void __declspec(naked) wmWorldMap_hook() {
	__asm {
		mov  ebx, eax; // keep map id
		mov  edx, 1;
		mov  ecx, eax;
		push 0;
		call EncounterHook_Script;
		test eax, eax;
		cmovl eax, ebx; // restore map if map < 0
		jmp  fo::funcoffs::map_load_idx_; // eax - map id
	}
}

static void __declspec(naked) wmRndEncounterOccurred_hook() {
	static long hkEncounterMapID = -1;
	__asm {
		cmp  hkEncounterMapID, -1;
		jnz  blinkIcon;
		test edx, edx;
		jz   hookRun;
		jmp  fo::funcoffs::wmInterfaceRefresh_;
hookRun: /////////////////////////////////
		push edx;
		push ecx;
		push ecx; // encType
		xor  edx, edx;
		mov  ecx, dword ptr ds:[FO_VAR_EncounterMapID];
		call EncounterHook_Script;
		pop  ecx;
		pop  edx;
		mov  dword ptr ds:[FO_VAR_EncounterMapID], eax;
		cmp  eax, -1;
		je   cancelEnc;
		jl   clearEnc;
		jmp  fo::funcoffs::wmInterfaceRefresh_;
clearEnc: /////////////////////////////////
		mov  dword ptr ds:[FO_VAR_EncounterMapID], -1;
		neg  eax;
		sub  eax, 2; // recover correct map number from negative value
		mov  hkEncounterMapID, eax;
		dec  edx;
blinkIcon:
		cmp  edx, 6; // counter of blinking
		jge  break;
		jmp  fo::funcoffs::wmInterfaceRefresh_;
break:
		mov  eax, hkEncounterMapID;
		cmp  dword ptr ds:[FO_VAR_Move_on_Car], 0;
		je   noCar;
		mov  edx, FO_VAR_carCurrentArea;
		call fo::funcoffs::wmMatchAreaContainingMapIdx_;
		mov  eax, hkEncounterMapID;
noCar:
		call fo::funcoffs::map_load_idx_;
		xor  eax, eax;
		mov  hkEncounterMapID, -1;
cancelEnc:
		inc  eax; // 0 - continue movement, 1 - interrupt
		mov  dword ptr ds:[FO_VAR_wmEncounterIconShow], 0;
		add  esp, 4;
		mov  ebx, 0x4C0BC7;
		jmp  ebx;
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

void Inject_SneakCheckHook() {
	HookCall(0x42E3D9, SneakCheckHook);
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

void Inject_CarTravelHook() {
	MakeJump(0x4BFEF1, CarTravelHack);
	BlockCall(0x4BFF6E); // vanilla wmCarUseGas(100) call
}

void Inject_SetGlobalVarHook() {
	HookCall(0x455A6D, SetGlobalVarHook);
}

void Inject_RestTimerHook() {
	const DWORD restTimerLoopHkAddr[] = {0x499B4B, 0x499BE0, 0x499D2C, 0x499DF2};
	HookCalls(RestTimerLoopHook, restTimerLoopHkAddr);
	const DWORD restTimerEscHkAddr[] = {0x499B63, 0x499CA1};
	MakeCalls(RestTimerEscapeHook, restTimerEscHkAddr);
}

void Inject_ExplosiveTimerHook() {
	HookCall(0x49BDC4, ExplosiveTimerHook);
}

void Inject_EncounterHook() {
	HookCall(0x4C02AF, wmWorldMap_hook);
	HookCall(0x4C095C, wmRndEncounterOccurred_hook);
}

void InitMiscHookScripts() {
	HookScripts::LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	HookScripts::LoadHookScript("hs_useskill", HOOK_USESKILL);
	HookScripts::LoadHookScript("hs_steal", HOOK_STEAL);
	HookScripts::LoadHookScript("hs_sneak", HOOK_SNEAK);
	HookScripts::LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	HookScripts::LoadHookScript("hs_cartravel", HOOK_CARTRAVEL);
	HookScripts::LoadHookScript("hs_setglobalvar", HOOK_SETGLOBALVAR);
	HookScripts::LoadHookScript("hs_resttimer", HOOK_RESTTIMER);
	HookScripts::LoadHookScript("hs_explosivetimer", HOOK_EXPLOSIVETIMER);
	HookScripts::LoadHookScript("hs_encounter", HOOK_ENCOUNTER);
}

}
