#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "CombatHs.h"

namespace sfall
{

static void __declspec(naked) ToHitHook() {
	__asm {
		HookBegin;
		mov  args[4],  eax;   // attacker
		mov  args[8],  ebx;   // target
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
		pushad;
	}

	argCount = 7;
	RunHookScript(HOOK_TOHIT);
	EndHook();

	__asm {
		popad;
		cmp  cRet, 1;
		cmovnb eax, rets[0];
		retn 8;
	}
}

static void __fastcall AfterHitRollHook_Script(fo::ComputeAttackResult &ctd, DWORD hitChance, DWORD hit) {

	BeginHook();
	argCount = 5;

	args[0] = hit;
	args[1] = (DWORD)ctd.attacker;   // Attacker
	args[2] = (DWORD)ctd.target;     // Target
	args[3] = ctd.bodyPart;          // bodypart
	args[4] = hitChance;

	RunHookScript(HOOK_AFTERHITROLL);
	if (cRet > 1) {
		ctd.bodyPart = rets[1];
		if (cRet > 2) ctd.target = (fo::GameObject*)rets[2];
	}
	EndHook();
}

static const DWORD AfterHitRollAddr = 0x423898;
static void __declspec(naked) AfterHitRollHook() {
	using namespace fo;
	__asm {
		pushad;
		mov  ecx, esi;                 // ctd
		mov  edx, [esp + 0x18 + 32];   // hit chance
		push eax;                      // was it a hit?
		call AfterHitRollHook_Script;
		popad;
		cmp  cRet, 1;
		cmovnb eax, rets[0];
		// engine code
		mov  ebx, eax;
		cmp  ebx, ROLL_FAILURE;
		jmp  AfterHitRollAddr;
	}
}

static void __declspec(naked) CalcApCostHook() {
	__asm {
		HookBegin;
		mov  args[0], eax;
		mov  args[4], edx;
		mov  args[8], ebx;
		call fo::funcoffs::item_w_mp_cost_;
		mov  args[12], eax;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_CALCAPCOST);
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		retn;
	}
}

// this is for using non-weapon items, always 2 AP in vanilla
static void __declspec(naked) CalcApCostHook2() {
	__asm {
		HookBegin;
		mov args[0], ecx; // critter
		mov args[4], edx; // attack type (to determine hand)
		mov args[8], ebx;
		mov eax, 2;       // vanilla value
		mov args[12], eax;
		pushad;
	}

	argCount = 4;
	RunHookScript(HOOK_CALCAPCOST);
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0];
		retn;
	}
}

static void __fastcall ComputeDamageHook_Script(fo::ComputeAttackResult &ctd, DWORD rounds, DWORD multiply) {

	BeginHook();
	argCount = 12;

	args[0] = (DWORD)ctd.target;           // Target
	args[1] = (DWORD)ctd.attacker;         // Attacker
	args[2] = ctd.targetDamage;            // amountTarget
	args[3] = ctd.attackerDamage;          // amountSource
	args[4] = ctd.targetFlags;             // flagsTarget
	args[5] = ctd.attackerFlags;           // flagsSource
	args[6] = (DWORD)ctd.weapon;
	args[7] = ctd.bodyPart;
	args[8] = multiply;                    // multiply damage
	args[9] = rounds;                      // number rounds
	args[10] = ctd.knockbackValue;
	args[11] = ctd.hitMode;                // attack type

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
		push ebx;         // store dmg multiply  args[8]
		push edx;         // store num rounds    args[9]
		push eax;         // store ctd
		call fo::funcoffs::compute_damage_;
		pop  ecx;         // restore ctd (eax)
		pop  edx;         // restore num rounds
		call ComputeDamageHook_Script;  // stack - arg multiply
		pop  ecx;
		retn;
	}
}

static void __fastcall FindTargetHook_Script(DWORD* target, DWORD attacker) {

	BeginHook();
	argCount = 5;

	args[0] = attacker;
	args[1] = target[0];
	args[2] = target[1];
	args[3] = target[2];
	args[4] = target[3];

	RunHookScript(HOOK_FINDTARGET);

	if (cRet >= 4) {
		target[0] = args[1];
		target[1] = args[2];
		target[2] = args[3];
		target[3] = args[4];
	}
	EndHook();
}

static void __declspec(naked) FindTargetHook() {
	__asm {
		pushad;
		mov  ecx, eax;     // targets (base)
		mov  edx, esi;     // attacker
		call FindTargetHook_Script;
		popad;
		cmp  cRet, 4;
		jge  skip;
		call fo::funcoffs::qsort_;
skip:
		retn;
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
		pushad;
	}

	if (args[2] == 0) {  // weapon arg
		args[4] += 8;    // type arg
	}

	argCount = 6;
	RunHookScript(HOOK_ITEMDAMAGE);
	EndHook();

	_asm popad;
	if (cRet > 0) {
		_asm mov eax, rets[0];
		if (cRet > 1) {
			_asm mov edx, rets[4];
		} else {
			_asm retn;
		}
	}
	_asm call fo::funcoffs::roll_random_;
	_asm retn;
}

int __fastcall AmmoCostHook_Script(DWORD hookType, fo::GameObject* weapon, DWORD* rounds) {
	int result = 0;

	BeginHook();
	argCount = 4;

	args[0] = (DWORD)weapon;
	args[1] = *rounds;          // rounds in attack
	args[3] = hookType;

	if (hookType == 2) {        // burst hook
		*rounds = 1;            // set default multiply for check burst attack
	} else {
		result = fo::func::item_w_compute_ammo_cost(weapon, rounds);
		if (result == -1) goto failed; // failed computed
	}
	args[2] = *rounds;          // rounds as computed by game (cost)

	RunHookScript(HOOK_AMMOCOST);

	if (cRet > 0) *rounds = rets[0]; // override rounds

failed:
	EndHook();
	return result;
}

static void __declspec(naked) AmmoCostHook() {
	using namespace fo;
	__asm {
		xor  ecx, ecx;             // type of hook (0)
		cmp dword ptr [esp + 0x1C + 4], ANIM_fire_burst;
		jl skip;
		cmp dword ptr [esp + 0x1C + 4], ANIM_fire_continuous;
		jg skip;
		mov  ecx, 3;               // hook type burst
skip:		
		xchg eax, edx;
		push eax;                  // rounds in attack
		call AmmoCostHook_Script;  // edx - weapon
		retn;
	}
}

// hooks combat_turn function
static void _declspec(naked) CombatTurnHook() {
	__asm {
		HookBegin;
		mov args[0], 1;   // turn begin
		mov args[4], eax; // critter
		mov args[8], edx; // unknown (1 = dude turn)
		pushad;
	}

	argCount = 3;
	RunHookScript(HOOK_COMBATTURN); // Start of turn

	if (cRet > 0) {
		EndHook();
		_asm popad;
		_asm mov eax, rets[0];
		_asm retn;        // exit hook
	}

// set_sfall_return not used, proceed normally
	__asm {
		popad;
		call fo::funcoffs::combat_turn_;
		mov  args[0], eax;
		pushad;
	}

	cRet = 0; // reset number of return values
	RunHookScript(HOOK_COMBATTURN); // End of turn
	EndHook();

	__asm {
		popad;
		cmp cRet, 1;
		cmovnb eax, rets[0]; // override result of turn
		retn;
	}
}

// hack to exit from combat_add_noncoms function without crashing when you load game during NPC turn
static const DWORD CombatHack_add_noncoms_back = 0x422359;
static void _declspec(naked) CombatAddNoncoms_CombatTurnHack() {
	__asm {
		call CombatTurnHook;
		cmp  eax, -1;
		jne  normalTurn;
		mov  ecx, FO_VAR_list_com;
		mov  dword ptr [ecx], 0;
		mov  ecx, [esp];
normalTurn:
		jmp  CombatHack_add_noncoms_back;
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

static void _declspec(naked) ComputeExplosionOnExtrasHook() {
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

void Inject_ToHitHook() {
	HookCalls(ToHitHook, {
		0x421686, // combat_safety_invalidate_weapon_func_
		0x4231D9, // check_ranged_miss_
		0x42331F, // shoot_along_path_
		0x4237FC, // compute_attack_
		0x424379, // determine_to_hit_
		0x42438D, // determine_to_hit_no_range_
		0x42439C, // determine_to_hit_from_tile_
		0x42679A  // combat_to_hit_
	});
}

void Inject_AfterHitRollHook() {
	MakeJump(0x423893, AfterHitRollHook);
}

void Inject_CalcApCostHook() {
	HookCalls(CalcApCostHook, {
		0x42307A,
		0x42669F,
		0x42687B,
		0x42A625,
		0x42A655,
		0x42A686,
		0x42AE32,
		0x42AE71,
		0x460048,
		0x47807B
	});
	MakeCall(0x478083, CalcApCostHook2);
}

void Inject_CombatDamageHook() {
	HookCalls(ComputeDamageHook, {
		0x42326C, // check_ranged_miss()
		0x4233E3, // shoot_along_path() - for extra burst targets
		0x423AB7, // compute_attack()
		0x423BBF, // compute_attack()
		0x423DE7, // compute_explosion_on_extras()
		0x423E69, // compute_explosion_on_extras()
		0x424220, // attack_crit_failure()
		0x4242FB, // attack_crit_failure()
	});
}

void Inject_FindTargetHook() {
	HookCall(0x429143, FindTargetHook);
}

void Inject_ItemDamageHook() {
	HookCall(0x478560, ItemDamageHook);
}

void Inject_AmmoCostHook() {
	HookCall(0x423A7C, AmmoCostHook);
}

void Inject_CombatTurnHook() {
	MakeJump(0x422354, CombatAddNoncoms_CombatTurnHack);
	HookCalls(CombatTurnHook, { 0x422D87, 0x422E20 });
}

void Inject_OnExplosionHook() {
	HookCall(0x423D70, ComputeExplosionOnExtrasHook);
}

void InitCombatHookScripts() {

	LoadHookScript("hs_tohit", HOOK_TOHIT);
	LoadHookScript("hs_afterhitroll", HOOK_AFTERHITROLL);
	LoadHookScript("hs_calcapcost", HOOK_CALCAPCOST);
	LoadHookScript("hs_combatdamage", HOOK_COMBATDAMAGE);
	LoadHookScript("hs_findtarget", HOOK_FINDTARGET);
	LoadHookScript("hs_itemdamage", HOOK_ITEMDAMAGE);
	LoadHookScript("hs_ammocost", HOOK_AMMOCOST);
	LoadHookScript("hs_combatturn", HOOK_COMBATTURN);
	LoadHookScript("hs_onexplosion", HOOK_ONEXPLOSION);
}

}
