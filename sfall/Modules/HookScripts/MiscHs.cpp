#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Karma.h"
#include "..\LoadGameHook.h"
#include "Common.h"

#include "MiscHs.h"

namespace sfall
{
	
static DWORD lastTableCostPC; // keep last cost for pc
static DWORD lastTableCostNPC;

// The hook is executed twice when entering the barter screen and after transaction: the first time is for the player; the second time is for NPC
static DWORD __fastcall BarterPriceHook_Script(fo::GameObject* source, fo::GameObject* target, DWORD callAddr) {
	bool barterIsParty = (fo::var::dialog_target_is_party != 0);
	long computeCost = fo::func::barter_compute_value(source, target);

	BeginHook();
	argCount = 10;

	args[0] = (DWORD)source;
	args[1] = (DWORD)target;
	args[2] = !barterIsParty ? computeCost : 0;

	fo::GameObject* bTable = fo::var::btable;
	args[3] = (DWORD)bTable;
	args[4] = fo::func::item_caps_total(bTable);
	args[5] = fo::func::item_total_cost(bTable);

	fo::GameObject* pTable = fo::var::ptable;
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
	if (isPCHook) {
		lastTableCostPC = cost;
	} else {
		lastTableCostNPC = cost;
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
		retn;
	}
}

static void __declspec(naked) OverrideCost_BarterPriceHook() {
	__asm {
		mov eax, lastTableCostPC;
		retn;
	}
}

void BarterGetTableCosts(long* outPcTableCost, long* outNpcTableCost) {
	if (!HookScripts::HookHasScript(HOOK_BARTERPRICE)) {
		*outPcTableCost = fo::func::item_total_cost(fo::var::ptable);
		*outNpcTableCost = fo::func::barter_compute_value(fo::var::obj_dude, fo::var::target_stack[0]);
		return;
	}
	*outPcTableCost = lastTableCostPC;
	*outNpcTableCost = lastTableCostNPC;
}

static fo::GameObject* sourceSkillOn = nullptr;
void SourceUseSkillOnInit() { sourceSkillOn = fo::var::obj_dude; }

static char resultSkillOn; // -1 - cancel handler, 1 - replace user
static long bakupCombatState;

static void __fastcall UseSkillOnHook_Script(DWORD source, DWORD target, DWORD skillId) {
	BeginHook();
	argCount = 3;

	args[0] = source;  // user
	args[1] = target;  // target
	args[2] = skillId; // skill id

	sourceSkillOn = fo::var::obj_dude;
	resultSkillOn = 0;
	bakupCombatState = -1;

	RunHookScript(HOOK_USESKILLON);

	if (skillId != fo::Skill::SKILL_STEAL && cRet > 0) { // not work for steal skill
		if (rets[0] != 0) {
			resultSkillOn = (rets[0] == -1) ? -1 : 1;
			if (resultSkillOn == 1) {
				sourceSkillOn = (fo::GameObject*)rets[0];
			}
		}
		if (resultSkillOn != -1 && cRet > 1 && rets[1] == 1) {
			bakupCombatState = fo::var::combat_state;
			fo::var::combat_state = 0;
		}
	}
	EndHook();
}

static void __declspec(naked) UseSkillOnHook() {
	__asm {
		push eax;
		push ecx;
		push edx;
		push ebx;                    // skill id
		mov  ecx, eax;               // user
		call UseSkillOnHook_Script;  // edx - target
		pop  edx;
		pop  ecx;
		pop  eax;
		cmp  resultSkillOn, -1;      // skip handler
		jnz  handler;
		retn;
handler:
		jmp  fo::funcoffs::action_use_skill_on_;
	}
}

static void __declspec(naked) UseSkillOnHack() {
	__asm {
		cmp bakupCombatState, -1;
		jz  skip;
		mov ebp, bakupCombatState;
		mov dword ptr ds:[FO_VAR_combat_state], ebp;
skip:
		cmp resultSkillOn, 0;
		jz  default;
		mov edi, sourceSkillOn; // skill user (override from hook)
		retn;  // flag ZF = 0
default:
		// engine code
		cmp eax, dword ptr ds:[FO_VAR_obj_dude];
		retn;
	}
}

static void __declspec(naked) skill_use_hack() {
	__asm {
		cmp   ebp, dword ptr ds:[FO_VAR_obj_dude];
		setnz al;
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

static long stealExpOverride;

static void __declspec(naked) StealHook_ExpOverrideHack() {
	__asm {
		mov ecx, [esp + 0x150 - 0x18 + 4]; // total exp
		cmp stealExpOverride, -1;
		jle vanillaExp;
		add ecx, stealExpOverride; // add overridden exp value
		jmp end;
vanillaExp:
		add ecx, edi; // add vanilla exp value
end:
		add edi, 10; // vanilla exp increment for next success
		mov [esp + 0x150 - 0x18 + 4], ecx; // set total exp
		mov ecx, [esp];
		add ecx, 14; // shift return address
		mov [esp], ecx;
		retn;
	}
}

static void __declspec(naked) StealCheckHook() {
	static const DWORD StealSkipRet = 0x474B18;
	__asm {
		HookBegin;
		mov args[0], eax;  // thief
		mov args[4], edx;  // target
		mov args[8], ebx;  // item
		mov args[12], ecx; // is planting
		mov args[16], esi; // quantity
		pushadc;
	}

	argCount = 5;
	stealExpOverride = -1;
	RunHookScript(HOOK_STEAL);

	__asm {
		popadc;
		cmp  cRet, 1;
		jl   defaultHandler; // no return values, use vanilla path
		cmp  cRet, 2;
		jl   skipExpOverride;
		push eax;
		mov  eax, rets[4]; // override experience points for steal
		mov  stealExpOverride, eax;
		pop  eax;
skipExpOverride:
		cmp  rets[0], -1; // if <= -1, use vanilla path
		jle  defaultHandler;
		cmp  rets[0], 2; // 2 - steal failed but didn't get cought
		jnz  normalReturn;
		HookEnd;
		add  esp, 4;
		jmp  StealSkipRet;
normalReturn:
		mov  eax, rets[0];
		HookEnd;
		retn;
defaultHandler:
		HookEnd;
		jmp  fo::funcoffs::skill_check_stealing_;
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

static constexpr long maxGasAmount = 80000;

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
	fo::var::carGasAmount = max(0, originalGas - consumption);

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

static long __fastcall GlobalVarHook_Script(DWORD number, int value) {
	int old = fo::var::game_global_vars[number];

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
			fo::var::setInt(FO_VAR_old_world_xpos) = fo::var::world_xpos;
			fo::var::setInt(FO_VAR_old_world_ypos) = fo::var::world_ypos;
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

static long __stdcall RollCheckHook_Script(long roll, long chance, long bonus, long randomChance, long calledFrom) {
	long hookType;
	switch (calledFrom - 5) {
	case 0x42388E:    // compute_attack_
	case 0x4234D1:    // compute_spray_
		hookType = 1; // single and burst attack hit event
		break;
	case 0x42356C:    // compute_spray_
		hookType = 2; // burst attack bullet hit event
		break;
	case 0x4AAB29:    // skill_result_
		hookType = 3; // common skill check event
		 break;
	case 0x4AB3B6:    // skill_use_
		hookType = 4; // SKILL_REPAIR
		break;
	case 0x4AB8B5:    // skill_use_
		hookType = 5; // SKILL_DOCTOR
		break;
	case 0x4ABC9F:    // skill_check_stealing_
		hookType = 6; // SKILL_STEAL - source stealing check event
		break;
	case 0x4ABCE6:    // skill_check_stealing_
		hookType = 7; // SKILL_STEAL - target stealing check event (fail for success stealing)
		break;
	default:
		return roll;  // unsupported hook
	}

	BeginHook();
	argCount = 5;

	args[0] = hookType;
	args[1] = roll;
	args[2] = chance;
	args[3] = bonus;
	args[4] = randomChance;

	RunHookScript(HOOK_ROLLCHECK);
	if (cRet) roll = rets[0];

	EndHook();
	return roll;
}

static void __declspec(naked) roll_check_hook() {
	__asm {
		push ecx;
		push [esp + 0xC + 8]; // calledFrom
		push ebx; // random chance value
		push esi; // bonus
		push edi; // chance value
		call fo::funcoffs::roll_check_critical_;
		push eax; // roll result
		call RollCheckHook_Script;
		pop  ecx;
		retn;
	}
}

void Inject_BarterPriceHook() {
	HookCalls(BarterPriceHook, {
		0x474D4C, // barter_attempt_transaction_ (offers button)
		0x475735, // display_table_inventories_ (for party members)
		0x475762  // display_table_inventories_
	});
	HookCalls(PC_BarterPriceHook, {0x4754F4, 0x47551A}); // display_table_inventories_
	HookCall(0x474D3F, OverrideCost_BarterPriceHook);    // barter_attempt_transaction_ (just overrides cost of offered goods)
}

void Inject_UseSkillHook() {
	HookCalls(UseSkillHook, { 0x49C48F, 0x49D12E });
}

void Inject_StealCheckHook() {
	HookCalls(StealCheckHook, { 0x4749A2, 0x474A69 });
	MakeCalls(StealHook_ExpOverrideHack, { 0x4742C5, 0x4743E1 });
}

void Inject_SneakCheckHook() {
	HookCall(0x42E3D9, SneakCheckHook);
}

void Inject_WithinPerceptionHook() {
	HookCalls(PerceptionRangeHook, {
		0x42B4ED,
		0x42BC87,
		0x42BC9F,
		0x42BD04,
	});
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
	HookCalls(RestTimerLoopHook, { 0x499B4B, 0x499BE0, 0x499D2C, 0x499DF2 });
	MakeCalls(RestTimerEscapeHook, { 0x499B63, 0x499CA1 });
}

void Inject_ExplosiveTimerHook() {
	HookCall(0x49BDC4, ExplosiveTimerHook);
}

void Inject_UseSkillOnHook() {
	HookCalls(UseSkillOnHook, { 0x44C3CA, 0x44C81C });
	MakeCall(0x4127BA, UseSkillOnHack, 1);
	MakeCalls(skill_use_hack, {0x4AB05D, 0x4AB558, 0x4ABA60}); // fix checking obj_dude's target

	// replace _obj_dude with source skill user (skill_use_ function)
	SafeWriteBatch<DWORD>((DWORD)&sourceSkillOn, {
		0x4AAF47, 0x4AB051, 0x4AB3FB, 0x4AB550, 0x4AB8FA, 0x4ABA54,
		0x4AB0EF, 0x4AB5C0, 0x4ABAF2 // fix for time increment
	});
}

void Inject_EncounterHook() {
	HookCall(0x4C02AF, wmWorldMap_hook);
	HookCall(0x4C095C, wmRndEncounterOccurred_hook);
}

void Inject_RollCheckHook() {
	HookCall(0x4A3020, roll_check_hook);
}

void InitMiscHookScripts() {
	HookScripts::LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	HookScripts::LoadHookScript("hs_useskillon", HOOK_USESKILLON);
	HookScripts::LoadHookScript("hs_useskill", HOOK_USESKILL);
	HookScripts::LoadHookScript("hs_steal", HOOK_STEAL);
	HookScripts::LoadHookScript("hs_sneak", HOOK_SNEAK);
	HookScripts::LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	HookScripts::LoadHookScript("hs_cartravel", HOOK_CARTRAVEL);
	HookScripts::LoadHookScript("hs_setglobalvar", HOOK_SETGLOBALVAR);
	HookScripts::LoadHookScript("hs_resttimer", HOOK_RESTTIMER);
	HookScripts::LoadHookScript("hs_explosivetimer", HOOK_EXPLOSIVETIMER);
	HookScripts::LoadHookScript("hs_encounter", HOOK_ENCOUNTER);
	HookScripts::LoadHookScript("hs_rollcheck", HOOK_ROLLCHECK);
}

}
