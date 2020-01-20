#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Karma.h"
#include "..\LoadGameHook.h"
#include "Common.h"

#include "MiscHs.h"

// Misc. hook scripts
namespace sfall
{

// The hook is executed twice when entering the barter screen and after transaction: the first time is for the player; the second time is for NPC
static DWORD __fastcall BarterPriceHook_Script(register fo::GameObject* source, register fo::GameObject* target, DWORD callAddr) {
	bool barterIsParty = (*(DWORD*)FO_VAR_dialog_target_is_party != 0);
	long computeCost = fo::func::barter_compute_value(source, target);

	BeginHook();
	argCount = 10;

	args[0] = (DWORD)source;
	args[1] = (DWORD)target;
	args[2] = !barterIsParty ? computeCost : 0;

	fo::GameObject* bTable = (fo::GameObject*)fo::var::btable;
	args[3] = (DWORD)bTable;
	args[4] = fo::func::item_caps_total(bTable);
	args[5] = fo::func::item_total_cost(bTable);

	fo::GameObject* pTable = (fo::GameObject*)fo::var::ptable;
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

static fo::GameObject* sourceSkillOn = nullptr;
void SourceUseSkillOnInit() { sourceSkillOn = fo::var::obj_dude; }

static char resultSkillOn;
static long bakupCombatState;
static void __fastcall UseSkillOnHook_Script(DWORD source, DWORD target, register DWORD skillId) {
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
		mov edi, sourceSkillOn;
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
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_USESKILL);

	__asm {
		popad;
		cmp cRet, 1;
		jb  defaultHandler;
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
		jb  defaultHandler;
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

static long __stdcall PerceptionRangeHook_Script(int type) {
	long result;
	__asm {
		HookBegin;
		mov  args[0], eax; // watcher
		mov  args[4], edx; // target
		call fo::funcoffs::is_within_perception_;
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
	fo::var::carGasAmount = max(originalGas - consumption, 0);

	EndHook();
}

static const DWORD CarTravelHack_back = 0x4BFF43;
static void __declspec(naked) CarTravelHack() {
	__asm {
		pushad;
		call CarTravelHook_Script;
		popad;
		jmp CarTravelHack_back;
	}
}

static long __fastcall GlobalVarHook_Script(register DWORD number, register int value) {
	int old = fo::var::game_global_vars[number];

	if (IsMapLoaded() && HookScripts::HookHasScript(HOOK_SETGLOBALVAR)) { // IsMapLoaded - don't execute hook until loading sfall scripts
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
static long _stdcall RestTimerHook_Script() {
	DWORD addrHook;
	__asm {
		mov addrHook, ebx;
		HookBegin;
		mov args[0], eax;
		mov args[8], ecx;
		mov args[12], edx;
	}

	argCount = 4;
	addrHook -= 5;
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
		push eax;
		push edx;
		push ecx;
		push ebx;
		mov  ebx, [esp + 16];
		mov  ecx, [esp + 20 + 0x40]; // hours_
		mov  edx, [esp + 20 + 0x44]; // minutes_
		call RestTimerHook_Script;
		pop  ebx;
		pop  ecx;
		pop  edx;
		cmp  eax, 0;
		cmovge edi, eax;             // return 1 to interrupt resting
		pop  eax;
		jmp  fo::funcoffs::set_game_time_;
	}
}

static void __declspec(naked) RestTimerEscapeHook() {
	__asm {
		mov  edi, 1;    // engine code
		cmp  eax, 0x1B; // ESC ASCII code
		jnz  skip;
		push eax;
		push edx;
		push ecx;
		push ebx;
		mov  ebx, [esp + 16];
		mov  ecx, [esp + 20 + 0x40]; // hours_
		mov  edx, [esp + 20 + 0x44]; // minutes_
		call RestTimerHook_Script;
		pop  ebx;
		pop  ecx;
		pop  edx;
		cmp  eax, 0;
		cmovge edi, eax;             // return 0 for cancel ESC key
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
			*(DWORD*)FO_VAR_old_world_xpos = fo::var::world_xpos;
			*(DWORD*)FO_VAR_old_world_ypos = fo::var::world_ypos;
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
		cmp  ds:[FO_VAR_Move_on_Car], 0;
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
		mov  ds:[FO_VAR_wmEncounterIconShow], 0;
		add  esp, 4;
		mov  ebx, 0x4C0BC7;
		jmp  ebx;
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
}

void Inject_SneakCheckHook() {
	HookCall(0x42E3D9, SneakCheckHook);
}

void Inject_WithinPerceptionHook() {
	HookCalls(PerceptionRangeHook, {
		0x429157,
		0x42B4ED,
		0x42BC87,
		0x42BC9F,
		0x42BD04,
	});
	HookCall(0x456BA2, PerceptionRangeSeeHook);
	HookCall(0x458403, PerceptionRangeHearHook);
}

void Inject_CarTravelHook() {
	MakeJump(0x4BFEF1, CarTravelHack);
	BlockCall(0x4BFF6E); // vanilla wnCarUseGas(100) call
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

	// replace source skill user
	SafeWriteBatch<DWORD>((DWORD)&sourceSkillOn, {0x4AAF47, 0x4AB051, 0x4AB3FB, 0x4AB550, 0x4AB8FA, 0x4ABA54});
	SafeWriteBatch<DWORD>((DWORD)&sourceSkillOn, {0x4AB0EF, 0x4AB5C0, 0x4ABAF2}); // fix for time
}

void Inject_EncounterHook() {
	HookCall(0x4C02AF, wmWorldMap_hook);
	HookCall(0x4C095C, wmRndEncounterOccurred_hook);
}

void InitMiscHookScripts() {
	LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	LoadHookScript("hs_useskillon", HOOK_USESKILLON);
	LoadHookScript("hs_useskill", HOOK_USESKILL);
	LoadHookScript("hs_steal", HOOK_STEAL);
	LoadHookScript("hs_sneak", HOOK_SNEAK);
	LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	LoadHookScript("hs_cartravel", HOOK_CARTRAVEL);
	LoadHookScript("hs_setglobalvar", HOOK_SETGLOBALVAR);
	LoadHookScript("hs_resttimer", HOOK_RESTTIMER);
	LoadHookScript("hs_explosivetimer", HOOK_EXPLOSIVETIMER);
	LoadHookScript("hs_encounter", HOOK_ENCOUNTER);
}

}
