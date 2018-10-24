#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "..\Karma.h"
#include "Common.h"

#include "MiscHs.h"

// Misc. hook scripts
namespace sfall
{

static DWORD __fastcall BarterPriceHook_Script(register fo::GameObject* source, register fo::GameObject* target, DWORD callAddr) {

	int computeCost = fo::func::barter_compute_value(source, target);

	BeginHook();
	argCount = 9;

	args[0] = (DWORD)source;
	args[1] = (DWORD)target;
	args[2] = computeCost;

	fo::GameObject* bTable = (fo::GameObject*)fo::var::btable;
	args[3] = (DWORD)bTable;
	args[4] = fo::func::item_caps_total(bTable);
	args[5] = fo::func::item_total_cost(bTable);

	fo::GameObject* pTable = (fo::GameObject*)fo::var::ptable;
	args[6] = (DWORD)pTable;
	int pcCost = fo::func::item_total_cost(pTable);
	args[7] = pcCost;

	args[8] = (DWORD)(callAddr == 0x474D51); // check offers button

	RunHookScript(HOOK_BARTERPRICE);
	EndHook();

	return (callAddr == 0x47551F) ? pcCost : computeCost;
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
		cmp  cRet, 1;
		jb   skip;
		cmp  rets[0], -1;
		cmovg eax, rets[0];
skip:
		retn;
	}
}

static DWORD offersGoodsCost; // keep last cost
static void __declspec(naked) PC_BarterPriceHook() {
	__asm {
		push edx;
		push ecx;
		//-------
		push [esp + 8];                                // address on call stack
		mov  ecx, dword ptr ds:[FO_VAR_obj_dude];      // source
		mov  edx, dword ptr ds:[FO_VAR_target_stack];  // target
		call BarterPriceHook_Script;
		pop  ecx;
		pop  edx;
		cmp  cRet, 2;
		cmovnb eax, rets[4];
		mov  offersGoodsCost, eax;
		retn;
	}
}

static const DWORD OverrideCostRet = 0x474D44;
static void __declspec(naked) OverrideCost_BarterPriceHack() {
	__asm {
		mov eax, offersGoodsCost;
		jmp OverrideCostRet;
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
	EndHook();

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
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		jb  defaultHandler;
		cmp rets[0], -1;
		je  defaultHandler;
		mov eax, rets[0];
		retn;
defaultHandler:
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
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_STEAL);
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		jb  defaultHandler;
		cmp rets[0], -1;
		je  defaultHandler;
		mov eax, rets[0];
		retn;
defaultHandler:
		jmp fo::funcoffs::skill_check_stealing_;
	}
}

static void __stdcall PerceptionRangeHook_Script(int type) {
	__asm {
		HookBegin;
		mov  args[0], eax; // watcher
		mov  args[4], edx; // target
		call fo::funcoffs::is_within_perception_;
		mov  args[8], eax; // check result
		push eax;
	}

	args[3] = type;

	argCount = 4;
	RunHookScript(HOOK_WITHINPERCEPTION);
	EndHook();

	__asm {
		pop eax;
		cmp cRet, 1;
		cmovnb eax, rets[0];
	}
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
		mov  dword ptr[esp + 0x2C - 0x1C + 4], eax; // set 1, skip blocking check
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

static void __fastcall GlobalVarHook_Script(register DWORD number, register int value) {
	int old = fo::var::game_global_vars[number];

	BeginHook();
	argCount = 2;
	args[0] = number;
	args[1] = value;
	RunHookScript(HOOK_SETGLOBALVAR);
	EndHook();

	if (cRet > 0) value = rets[0];

	if (number == fo::GVAR_PLAYER_REPUTATION && displayKarmaChanges) {
		int diff = value - old;
		if (diff != 0) Karma::DisplayKarma(diff);
	}
}

static void __declspec(naked) SetGlobalVarHook() {
	__asm {
		pushad;
		mov  ecx, eax;             // number
		call GlobalVarHook_Script; // edx - value
		popad;
		cmp  cRet, 1;
		cmovnb edx, dword ptr rets[0];
		jmp  fo::funcoffs::game_set_global_var_;
	}
}

static int restTicks;
static void _stdcall RestTimerHook_Script() {
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
	EndHook();
}

static void __declspec(naked) RestTimerLoopHook() {
	__asm {
		pushad;
		mov  ebx, [esp + 32];
		mov  ecx, [esp + 36 + 0x40]; // hours_
		mov  edx, [esp + 36 + 0x44]; // minutes_
		call RestTimerHook_Script;
		popad;
		cmp  cRet, 1;
		jb   skip;
		cmp  rets[0], 1;
		cmovz edi, rets[0];
skip:
		jmp  fo::funcoffs::set_game_time_;
	}
}

static void __declspec(naked) RestTimerEscapeHook() {
	__asm {
		cmp  eax, 0x1B; // ESC ASCII code
		jnz  skip;
		pushad;
		mov  ebx, [esp + 32];
		mov  ecx, [esp + 36 + 0x40]; // hours_
		mov  edx, [esp + 36 + 0x44]; // minutes_
		call RestTimerHook_Script;
		popad;
		mov  edi, 1;
		cmp  cRet, 1;
		jb   skip;
		cmp  rets[0], 0;
		cmovz edi, rets[0]; // ret 0 for cancel escape
skip:
		retn;
	}
}

static int __fastcall ExplosiveTimerHook_Script(DWORD type, DWORD item, DWORD time) {

	BeginHook();
	argCount = 3;

	args[0] = time;
	args[1] = item;
	args[2] = (type == 11) ? fo::ROLL_FAILURE : fo::ROLL_SUCCESS;

	RunHookScript(HOOK_EXPLOSIVETIMER);
	EndHook();

	int result = 0;
	if (cRet > 0 && rets[0] >= 0) {
		if (rets[0] > 18000) rets[0] = 18000;  // max 30 minutes
		if (cRet < 2 || (rets[1] < fo::ROLL_CRITICAL_FAILURE || rets[1] > fo::ROLL_CRITICAL_SUCCESS)) {
			result--;       // use vanilla type
		} else {
			result++;       // use returned type
		}
	}
	return result;
}

static void _declspec(naked) ExplosiveTimerHook() {
	using namespace fo;
	__asm {
		push eax;
		push edx;
		push ecx;
		//-------
		push edi;                       // time in ticks
		call ExplosiveTimerHook_Script; // ecx - type, edx - item
		cmp  eax, 0;
		pop  ecx;
		pop  edx;
		pop  eax;
		jz   end;
		mov  eax, rets[0];           // time in ticks
		jl   end;
		mov  ecx, 8;                 // type SUCCESS
		cmp  rets[4], ROLL_FAILURE;
		jg   end;
		add  ecx, 3;                 // type FAILURE (11)
end:
		call fo::funcoffs::queue_add_;
		retn;
	}
}

void Inject_BarterPriceHook() {
	HookCalls(BarterPriceHook, {
		0x474D4C,
		0x475735,
		0x475762
	});
	HookCall(0X47551A, PC_BarterPriceHook);
	MakeJump(0x474D3F, OverrideCost_BarterPriceHack); // just overrides cost of offered goods
}

void Inject_UseSkillHook() {
	HookCalls(UseSkillHook, { 0x49C48F, 0x49D12E });
}

void Inject_StealCheckHook() {
	HookCalls(StealCheckHook, { 0x4749A2, 0x474A69 });
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
	MakeCalls(skill_use_hack, {0x4AB05D, 0x4AB558, 0x4ABA60}); // fix check obj_dude target

	// replace source skill user
	SafeWriteBatch<DWORD>((DWORD)&sourceSkillOn, {0x4AAF47, 0x4AB051, 0x4AB3FB, 0x4AB550, 0x4AB8FA, 0x4ABA54});
	SafeWriteBatch<DWORD>((DWORD)&sourceSkillOn, {0x4AB0EF, 0x4AB5C0, 0x4ABAF2}); // fix for time
}

void InitMiscHookScripts() {

	LoadHookScript("hs_barterprice", HOOK_BARTERPRICE);
	LoadHookScript("hs_useskillon", HOOK_USESKILLON);
	LoadHookScript("hs_useskill", HOOK_USESKILL);
	LoadHookScript("hs_steal", HOOK_STEAL);
	LoadHookScript("hs_withinperception", HOOK_WITHINPERCEPTION);
	LoadHookScript("hs_cartravel", HOOK_CARTRAVEL);
	LoadHookScript("hs_setglobalvar", HOOK_SETGLOBALVAR);
	LoadHookScript("hs_resttimer", HOOK_RESTTIMER);
	LoadHookScript("hs_explosivetimer", HOOK_EXPLOSIVETIMER);

}

}
