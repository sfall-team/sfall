#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\Combat.h"
#include "..\DamageMod.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "CombatHs.h"

namespace sfall
{

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
		call fo::funcoffs::determine_to_hit_func_;
		mov  args[0], eax;
		mov  ebx, eax;
	}
	argCount = 8;

	args[7] = Combat::determineHitChance;
	RunHookScript(HOOK_TOHIT);

	__asm {
		cmp  cRet, 1;
		cmovge ebx, rets[0];
		call EndHook;
		mov  eax, ebx;
		retn 8;
	}
}

static DWORD __fastcall AfterHitRollHook_Script(fo::ComputeAttackResult &ctd, DWORD hitChance, DWORD hit) {
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
			if (cRet > 2) ctd.target = (fo::GameObject*)rets[2];
		}
	}
	EndHook();

	return hit;
}

static void __declspec(naked) AfterHitRollHook() {
	using namespace fo;
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

static long CalcApCostHook_Script(fo::GameObject* source, long hitMode, long isCalled, long cost, fo::GameObject* weapon) {
	BeginHook();
	argCount = 5;

	args[0] = (DWORD)source;
	args[1] = hitMode;
	args[2] = isCalled;
	args[3] = cost;
	args[4] = (DWORD)weapon;

	RunHookScript(HOOK_CALCAPCOST);

	if (cRet > 0) cost = rets[0];
	EndHook();

	return cost;
}

long CalcApCostHook_Invoke(fo::GameObject* source, long hitMode, long isCalled, long cost, fo::GameObject* weapon) {
	return (HookScripts::HookHasScript(HOOK_CALCAPCOST))
	       ? CalcApCostHook_Script(source, hitMode, isCalled, cost, weapon)
	       : cost;
}
/*
static void __declspec(naked) CalcApCostHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call fo::funcoffs::item_w_mp_cost_;
		mov  args[12], eax;
		mov  ebx, eax;
		push ecx;
	}

	argCount = 5;
	args[4] = 0;

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
*/
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

	argCount = 5;
	args[4] = 0;

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

static void __fastcall ComputeDamageHook_Script(fo::ComputeAttackResult &ctd, DWORD rounds, DWORD multiplier) {
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
		call fo::funcoffs::compute_damage_;
		pop  ecx;         // restore ctd (eax)
		pop  edx;         // restore num of rounds
		call ComputeDamageHook_Script;  // stack - arg multiplier
		pop  ecx;
		retn;
	}
}

static void __fastcall FindTargetHook_Script(DWORD* target, fo::GameObject* attacker) {
	BeginHook();
	argCount = 5;

	args[0] = (DWORD)attacker;
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
		call fo::funcoffs::qsort_;
		pop  ecx;          // targets (base)
		mov  edx, esi;     // attacker
		jmp  FindTargetHook_Script;
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

	// tweak for 0x4784AA (obsolete)
	//if (args[2] == 0) { // weapon arg
	//	args[4] += 8;     // type arg
	//}

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
	__asm jmp fo::funcoffs::roll_random_;
}

int __fastcall AmmoCostHook_Script(DWORD hookType, fo::GameObject* weapon, DWORD &rounds) {
	int result = 0;

	BeginHook();
	argCount = 4;

	args[0] = (DWORD)weapon;
	args[1] = rounds;           // rounds in attack
	args[3] = hookType;

	if (hookType == 2) {        // burst hook
		rounds = 1;             // set default multiply for check burst attack
	} else {
		result = fo::func::item_w_compute_ammo_cost(weapon, &rounds);
		if (result == -1) goto failed; // computation failed
	}
	args[2] = rounds;           // rounds as computed by game (cost)

	RunHookScript(HOOK_AMMOCOST);

	if (cRet > 0) rounds = rets[0]; // override rounds

failed:
	EndHook();
	return result;
}

static void __declspec(naked) AmmoCostHook() {
	using namespace fo;
	__asm {
		xor  ecx, ecx;             // type of hook (0)
		cmp  dword ptr [esp + 0x1C + 4], ANIM_fire_burst;
		jl   skip;
		cmp  dword ptr [esp + 0x1C + 4], ANIM_fire_continuous;
		jg   skip;
		mov  ecx, 3;               // hook type burst
skip:
		xchg eax, edx;
		push eax;                  // rounds in attack ref
		call AmmoCostHook_Script;  // edx - weapon
		retn;
	}
}

fo::GameObject* __fastcall ComputeExplosionOnExtrasHook_Script(fo::GameObject* object, DWORD isCheck, DWORD checkTile, fo::ComputeAttackResult* ctdSource, DWORD isThrowing, fo::GameObject* who) {
	fo::GameObject* result = object;

	BeginHook();
	argCount = 7;

	args[0] = isCheck;
	args[1] = (DWORD)ctdSource->attacker;
	args[2] = ctdSource->targetTile;
	args[3] = checkTile;
	args[4] = (DWORD)object;
	args[5] = (DWORD)who;
	args[6] = isThrowing;

	RunHookScript(HOOK_ONEXPLOSION);

	if (cRet > 0) result = (fo::GameObject*)rets[0]; // override object

	EndHook();
	return result;
}

static void __declspec(naked) ComputeExplosionOnExtrasHook() {
	__asm {
		cmp  dword ptr [esp + 0x34 + 4], 0x429533;  // skip hook when AI assesses the situation in choosing the best weapon
		jz   end;
		cmp  dword ptr [esp + 0x34 + 4], 0x4296BC;
		jnz  hook;
end:
		jmp  fo::funcoffs::obj_blocking_at_;
hook:
		push ecx;
		push eax;                                   // who (target/source)
		call fo::funcoffs::obj_blocking_at_;
		push [esp + 0x34 - 0x24 + 12];              // isThrowing
		xor  edx, edx;
		cmp  dword ptr [esp + 0x34 + 16], 0x412F11; // check called from action_explode_
		jz   skip;
		mov  edx, [esp + 0x34 - 0x34 + 16];         // isCheck (bypass damage)
skip:
		push esi;                                   // source ctd
		push edi;                                   // check tile
		mov  ecx, eax;                              // object
		call ComputeExplosionOnExtrasHook_Script;   // edx - isCheck
		pop  ecx;
		retn;
	}
}

static DWORD targetRet = 0;
static bool targetObjHookHasRet = false;

static long __fastcall TargetObjectHook(DWORD isValid, DWORD object, long type) {
	if (isValid > 1) isValid = 1;

	BeginHook();
	argCount = 3;

	args[0] = type;    // 0 - mouse hovering over target, 1 - mouse clicking on target
	args[1] = isValid; // 1 - target is valid
	args[2] = object;  // target object

	if (isValid == 0) object = 0; // it is necessary for the proper operation of the engine code
	if (type == 0) targetRet = 0; // unset ret from the previous execution of the hook

	RunHookScript(HOOK_TARGETOBJECT);

	if (cRet > 0) {
		targetRet = (rets[0] != 0) ? rets[0] : object; // 0 - default object, -1 - invalid target, or object override
		object = (targetRet != -1) ? targetRet : 0;    // object can't be -1
		targetObjHookHasRet = true;
	} else if (targetObjHookHasRet && type == 1) {
		targetObjHookHasRet = false;
		if (targetRet != -1) object = targetRet;
	}
	EndHook();
	return object; // null or object
}

static void __declspec(naked) gmouse_bk_process_hook() {
	__asm {
		push 0;        // type
		mov  ecx, eax; // 1 - valid(object) or 0 - invalid
		mov  edx, edi; // object under mouse
		call TargetObjectHook;
		mov  edi, eax;
		retn;
	}
}

static void __declspec(naked) gmouse_handle_event_hook() {
	__asm {
		push 1;        // type
		mov  ecx, eax; // 1 - valid(object) or 0 - invalid
		cmp  dword ptr ds:[targetRet], 0;
		je   default;
		// override
		mov  ecx, 1;
		xor  eax, eax;
		cmp  dword ptr ds:[targetRet], -1;
		cmove  ecx, eax;       // if true - set invalid
		cmovne edx, targetRet; // if false - set override object
default:
		call TargetObjectHook;
		mov  edx, eax;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

void Inject_ToHitHook() {
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
}

void Inject_AfterHitRollHook() {
	MakeCall(0x423893, AfterHitRollHook);
}

void Inject_CalcApCostHook() {
	//const DWORD calcApCostHkAddr[] = {
	//	0x42307A,
	//	0x42669F,
	//	0x42687B,
	//	0x42A625,
	//	0x42A655,
	//	0x42A686,
	//	0x42AE32,
	//	0x42AE71,
	//	0x460048,
	//	0x47807B
	//};
	//HookCalls(CalcApCostHook, calcApCostHkAddr);
	MakeCall(0x478083, CalcApCostHook2);
}

void Inject_CombatDamageHook() {
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
	MakeCall(0x423DEB, ComputeDamageHook); // compute_explosion_on_extras() - fix for the attacker
}

void Inject_FindTargetHook() {
	HookCall(0x429143, FindTargetHook);
}

void Inject_ItemDamageHook() {
	HookCall(0x478560, ItemDamageHook);
}

void Inject_AmmoCostHook() {
	HookCall(0x423A7C, AmmoCostHook); // compute_attack_
}

void Inject_OnExplosionHook() {
	HookCall(0x423D70, ComputeExplosionOnExtrasHook);
}

void Inject_TargetObjectHook() {
	MakeCall(0x44BB16, gmouse_bk_process_hook, 1);
	SafeWrite8(0x44BB00, 0x15);

	MakeCall(0x44C286, gmouse_handle_event_hook, 1);
	SafeWrite8(0x44C26E, 0x17);
}

void InitCombatHookScripts() {
	HookScripts::LoadHookScript("hs_tohit", HOOK_TOHIT);
	HookScripts::LoadHookScript("hs_afterhitroll", HOOK_AFTERHITROLL);
	HookScripts::LoadHookScript("hs_calcapcost", HOOK_CALCAPCOST);
	HookScripts::LoadHookScript("hs_combatdamage", HOOK_COMBATDAMAGE);
	HookScripts::LoadHookScript("hs_findtarget", HOOK_FINDTARGET);
	HookScripts::LoadHookScript("hs_itemdamage", HOOK_ITEMDAMAGE);
	HookScripts::LoadHookScript("hs_ammocost", HOOK_AMMOCOST);
	HookScripts::LoadHookScript("hs_onexplosion", HOOK_ONEXPLOSION);
	HookScripts::LoadHookScript("hs_targetobject", HOOK_TARGETOBJECT);
}

}
