/*
 *    sfall
 *    Copyright (C) 2012  The sfall team
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

#include <unordered_map>

#include "main.h"
#include "FalloutEngine.h"

#include "HookScripts.h"
#include "Inventory.h"
#include "ReplacementFuncs.h"

#include "AI.h"

using namespace Fields;

typedef std::tr1::unordered_map<TGameObj*, TGameObj*>::const_iterator iter;

static std::tr1::unordered_map<TGameObj*, TGameObj*> targets;
static std::tr1::unordered_map<TGameObj*, TGameObj*> sources;

// Returns the friendly critter or any blocking object in the line of fire
TGameObj* __stdcall AI_CheckShootAndFriendlyInLineOfFire(TGameObj* object, long targetTile, long team) {
	if (object && object->IsCritter() && object->critter.teamNum != team) { // is not friendly fire
		long objTile = object->tile;
		if (objTile == targetTile) return nullptr;

		if (object->flags & ObjectFlag::MultiHex) {
			long dir = fo_tile_dir(objTile, targetTile);
			objTile = fo_tile_num_in_direction(objTile, dir, 1);
			if (objTile == targetTile) return nullptr; // just in case
		}
		// continue checking the line of fire from object tile to targetTile
		TGameObj* obj = object; // for ignoring the object (multihex) when building the path
		fo_make_straight_path_func(object, objTile, targetTile, 0, (DWORD*)&obj, 32, (void*)obj_shoot_blocking_at_);
		if (!AI_CheckShootAndFriendlyInLineOfFire(obj, targetTile, team)) return nullptr;
	}
	return object;
}

// Returns the friendly critter in the line of fire
TGameObj* __stdcall AI_CheckFriendlyFire(TGameObj* target, TGameObj* attacker) {
	TGameObj* object = nullptr;
	fo_make_straight_path_func(attacker, attacker->tile, target->tile, 0, (DWORD*)&object, 32, (void*)obj_shoot_blocking_at_);
	object = AI_CheckShootAndFriendlyInLineOfFire(object, target->tile, attacker->critter.teamNum);
	return (object && object->IsCritter()) ? object : nullptr; // 0 if there are no friendly critters
}

bool __stdcall AI_AttackInRange(TGameObj* source, TGameObj* weapon, long distance) {
	if (sfgame_item_weapon_range(source, weapon, ATKTYPE_RWEAPON_PRIMARY) >= distance) return true;
	return (sfgame_item_weapon_range(source, weapon, ATKTYPE_RWEAPON_SECONDARY) >= distance);
}

////////////////////////////////////////////////////////////////////////////////

static void __declspec(naked) ai_try_attack_hook_FleeFix() {
	__asm {
		or   byte ptr [esi + combatState], CBTFLG_ReTarget; // set CombatStateFlag flag
		jmp  ai_run_away_;
	}
}

static void __declspec(naked) combat_ai_hook_FleeFix() {
	static const DWORD combat_ai_hook_flee_Ret = 0x42B206;
	__asm {
		test byte ptr [ebp], CBTFLG_ReTarget; // CombatStateFlag flag (critter.combat_state)
		jnz  reTarget;
		jmp  critter_name_;
reTarget:
		and  byte ptr [ebp], ~(CBTFLG_InFlee | CBTFLG_ReTarget); // unset CombatStateFlag flags
		xor  edi, edi;
		mov  dword ptr [esi + whoHitMe], edi;
		add  esp, 4;
		jmp  combat_ai_hook_flee_Ret;
	}
}

static void __declspec(naked) ai_try_attack_hook_runFix() {
	__asm {
		mov  ecx, [esi + combatState]; // save combat flags before ai_run_away
		call ai_run_away_;
		mov  [esi + combatState], ecx; // restore combat flags
		retn;
	}
}

static void __declspec(naked) combat_ai_hack() {
	static const DWORD combat_ai_hack_Ret = 0x42B204;
	__asm {
		mov  edx, [ebx + 0x10]; // cap.min_hp
		cmp  eax, edx;
		jl   tryHeal; // curr_hp < min_hp
end:
		add  esp, 4;
		jmp  combat_ai_hack_Ret;
tryHeal:
		mov  eax, esi;
		call ai_check_drugs_;
		cmp  [esi + health], edx; // edx - minimum hp, below which NPC will run away
		jge  end;
		retn; // flee
	}
}

static void __declspec(naked) ai_check_drugs_hook() {
	__asm {
		call stat_level_;                            // current hp
		mov  edx, dword ptr [esp + 0x34 - 0x1C + 4]; // ai cap
		mov  edx, [edx + 0x10];                      // min_hp
		cmp  eax, edx;                               // curr_hp < cap.min_hp
		cmovl edi, edx;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static bool __fastcall TargetExistInList(TGameObj* target, TGameObj** targetList) {
	char i = 4;
	do {
		if (*targetList == target) return true;
		targetList++;
	} while (--i);
	return false;
}

static void __declspec(naked) ai_find_attackers_hack_target2() {
	__asm {
		mov  edi, [esp + 0x24 - 0x24 + 4] // critter (target)
		pushadc;
		lea  edx, [ebp - 4]; // start list of targets
		mov  ecx, edi;
		call TargetExistInList;
		test al, al;
		popadc;
		jnz  skip;
		inc  edx;
		mov  [ebp], edi;
skip:
		retn;
	}
}

static void __declspec(naked) ai_find_attackers_hack_target3() {
	__asm {
		mov  edi, [esp + 0x24 - 0x20 + 4] // critter (target)
		push eax;
		push edx;
		mov  eax, 4; // count targets
		lea  edx, [ebp - 4 * 2]; // start list of targets
continue:
		cmp  edi, [edx];
		je   break;          // target == targetList
		lea  edx, [edx + 4]; // next target in list
		dec  al;
		jnz  continue;
break:
		test al, al;
		pop  edx;
		pop  eax;
		jz   skip;
		xor  edi, edi;
		retn;
skip:
		inc  edx;
		retn;
	}
}

static void __declspec(naked) ai_find_attackers_hack_target4() {
	__asm {
		mov  eax, [ecx + eax]; // critter (target)
		pushadc;
		lea  edx, [esi - 4 * 3]; // start list of targets
		mov  ecx, eax;
		call TargetExistInList;
		test al, al;
		popadc;
		jnz  skip;
		inc  edx;
		mov  [esi], eax;
skip:
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void __declspec(naked) ai_danger_source_hack_pm_newFind() {
	__asm {
		mov  ecx, [ebp + 0x18]; // source combat_data.who_hit_me
		test ecx, ecx;
		jnz  hasTarget;
		retn;
hasTarget:
		test [ecx + damageFlags], DAM_DEAD;
		jz   isNotDead;
		xor  ecx, ecx;
isNotDead:
		mov  dword ptr [ebp + 0x18], 0; // combat_data.who_hit_me (engine code)
		retn;
	}
}

static long __fastcall AICheckBeforeWeaponSwitch(TGameObj* target, long &hitMode, TGameObj* source, TGameObj* weapon) {
	if (source->critter.movePoints <= 0) return -1; // exit from ai_try_attack_
	if (!weapon) return 1; // no weapon in hand slot, call ai_switch_weapons_

	long _hitMode = fo_ai_pick_hit_mode(source, weapon, target);
	if (_hitMode != hitMode) {
		hitMode = _hitMode;
		return 0; // change hit mode, continue attack cycle
	}

	TGameObj* item = fo_ai_search_inven_weap(source, 1, target); // search based on AP
	if (!item) return 1; // no weapon in inventory, continue searching for weapons on the map (call ai_switch_weapons_)

	// is using a close range weapon?
	long wType = fo_item_w_subtype(item, ATKTYPE_RWEAPON_PRIMARY);
	if (wType <= ATKSUBTYPE_MELEE) { // unarmed and melee weapons, check the distance before switching
		if (!AI_AttackInRange(source, item, fo_obj_dist(source, target))) return -1; // target out of range, exit ai_try_attack_
	}
	return 1; // execute vanilla behavior of ai_switch_weapons_ function
}

static void __declspec(naked) ai_try_attack_hook_switch_weapon() {
	__asm {
		push edx;
		push [ebx]; // weapon (push dword ptr [esp + 0x364 - 0x3C + 8];)
		push esi;   // source
		call AICheckBeforeWeaponSwitch; // ecx - target, edx - hit mode
		pop  edx;
		test eax, eax;
		jle  noSwitch; // <= 0
		mov  ecx, ebp;
		mov  eax, esi;
		jmp  ai_switch_weapons_;
noSwitch:
		retn; // -1 - for exit from ai_try_attack_
	}
}

static long RetryCombatMinAP;

static void __declspec(naked) RetryCombatHook() {
	static DWORD RetryCombatLastAP = 0;
	__asm {
		mov  RetryCombatLastAP, 0;
retry:
		call combat_ai_;
process:
		cmp  dword ptr ds:[FO_VAR_combat_turn_running], 0;
		jle  next;
		call process_bk_;
		jmp  process;
next:
		mov  eax, [esi + movePoints];
		cmp  eax, RetryCombatMinAP;
		jl   end;
		cmp  eax, RetryCombatLastAP;
		je   end;
		mov  RetryCombatLastAP, eax;
		mov  eax, esi;
		xor  edx, edx;
		jmp  retry;
end:
		retn;
	}
}

static long __fastcall ai_weapon_reload_fix(TGameObj* weapon, TGameObj* ammo, TGameObj* critter) {
	sProto* proto = nullptr;
	long result = -1;
	long maxAmmo;

	TGameObj* _ammo = ammo;

	while (ammo) {
		result = fo_item_w_reload(weapon, ammo);
		if (result != 0) return result; // 1 - reload done, -1 - can't reload

		if (!proto) {
			proto = GetProto(weapon->protoId);
			maxAmmo = proto->item.weapon.maxAmmo;
		}
		if (weapon->item.charges >= maxAmmo) break; // magazine is full

		long pidAmmo = ammo->protoId;
		fo_obj_destroy(ammo);
		ammo = nullptr;

		DWORD currentSlot = -1; // begin find at first slot
		while (TGameObj* ammoFind = fo_inven_find_type(critter, item_type_ammo, &currentSlot)) {
			if (ammoFind->protoId == pidAmmo) {
				ammo = ammoFind;
				break;
			}
		}
	}
	if (_ammo != ammo) {
		fo_obj_destroy(ammo);
		return 1; // notifies the engine that the ammo has already been destroyed
	}
	return result;
}

static void __declspec(naked) item_w_reload_hook() {
	__asm {
		cmp  dword ptr [eax + protoId], PID_SOLAR_SCORCHER;
		je   skip;
		push ecx;
		push esi;      // source
		mov  ecx, eax; // weapon
		call ai_weapon_reload_fix; // edx - ammo
		pop  ecx;
		retn;
skip:
		jmp  item_w_reload_;
	}
}

static long __fastcall CheckWeaponRangeAndApCost(TGameObj* source, TGameObj* target) {
	long weaponRange = fo_item_w_range(source, ATKTYPE_RWEAPON_SECONDARY);
	long targetDist  = fo_obj_dist(source, target);
	if (targetDist > weaponRange) return 0; // don't use secondary mode

	return (source->critter.movePoints >= sfgame_item_w_mp_cost(source, ATKTYPE_RWEAPON_SECONDARY, 0)); // 1 - allow secondary mode
}

static void __declspec(naked) ai_pick_hit_mode_hook() {
	__asm {
		call caiHasWeapPrefType_;
		test eax, eax;
		jnz  evaluation;
		retn;
evaluation:
		mov  edx, edi;
		mov  ecx, esi;
		jmp  CheckWeaponRangeAndApCost;
	}
}

static void __declspec(naked) ai_danger_source_hook() {
	__asm {
		call combat_check_bad_shot_;
		cmp  dword ptr [esp + 56], 0x42B235 + 5; // called from combat_ai_
		je   fix;
		retn;
fix:	// check result
		cmp  eax, 1; // exception: 1 - no ammo
		setg al;     // set 0 for result OK
		retn;
	}
}

static void __declspec(naked) cai_perform_distance_prefs_hack() {
	__asm {
		mov  ebx, eax; // current distance to target
		mov  ecx, esi;
		push 0;        // no called shot
		mov  edx, ATKTYPE_RWEAPON_PRIMARY;
		call sfgame_item_w_mp_cost;
		mov  edx, [esi + movePoints];
		sub  edx, eax; // ap - cost = free AP's
		jle  moveAway; // <= 0
		lea  edx, [edx + ebx - 1];
		cmp  edx, 5;   // minimum threshold distance
		jge  skipMove; // distance >= 5?
		// check combat rating
		mov  eax, esi;
		call combatai_rating_;
		mov  edx, eax; // source rating
		mov  eax, edi;
		call combatai_rating_;
		cmp  eax, edx; // target vs source rating
		jl   skipMove; // target rating is low
moveAway:
		mov  ebx, 10;  // move away max distance
		retn;
skipMove:
		xor  ebx, ebx; // skip moving away at the beginning of the turn
		retn;
	}
}

static void __declspec(naked) ai_move_away_hook() {
	static const DWORD ai_move_away_hook_Ret = 0x4289DA;
	__asm {
		test ebx, ebx;
		jl   fix; // distance arg < 0
		jmp  ai_cap_;
fix:
		neg  ebx;
		mov  eax, [esi + movePoints]; // Current Action Points
		cmp  ebx, eax;
		cmovg ebx, eax; // if (distance > ap) dist = ap
		add  esp, 4;
		jmp  ai_move_away_hook_Ret;
	}
}

////////////////////////////////////////////////////////////////////////////////

static long __fastcall RollFriendlyFire(TGameObj* target, TGameObj* attacker) {
	if (AI_CheckFriendlyFire(target, attacker)) {
		long dice = fo_roll_random(1, 10);
		return (fo_stat_level(attacker, STAT_iq) >= dice); // 1 - is friendly
	}
	return 0;
}

static void __declspec(naked) combat_safety_invalidate_weapon_func_hook_check() {
	static const DWORD safety_invalidate_weapon_burst_friendly = 0x4216C9;
	__asm {
		pushadc;
		mov  ecx, esi; // target
		call RollFriendlyFire;
		test eax, eax;
		jnz  friendly;
		popadc;
		jmp  combat_ctd_init_;
friendly:
		lea  esp, [esp + 8 + 3*4];
		jmp  safety_invalidate_weapon_burst_friendly; // "Friendly was in the way!"
	}
}

////////////////////////////////////////////////////////////////////////////////

static void __declspec(naked) ai_try_attack_hack_check_safe_weapon() {
	__asm {
		mov  ebx, [esp + 0x364 - 0x38 + 4]; // hit mode
		retn;
	}
}

static void __fastcall CombatAttackHook(TGameObj* source, TGameObj* target) {
	sources[target] = source; // who attacked the 'target' from the last time
	targets[source] = target; // who was attacked by the 'source' from the last time
}

static void __declspec(naked) combat_attack_hook() {
	__asm {
		push eax;
		push ecx;
		push edx;
		mov  ecx, eax;         // source
		call CombatAttackHook; // edx - target
		pop  edx;
		pop  ecx;
		pop  eax;
		jmp  combat_attack_;
	}
}

void AICombatClear() {
	targets.clear();
	sources.clear();
}

void AI_Init() {
	const DWORD combatAttackAddr[] = {
		0x426A95, // combat_attack_this_
		0x42A796  // ai_attack_
	};
	HookCalls(combat_attack_hook, combatAttackAddr);

	RetryCombatMinAP = GetConfigInt("Misc", "NPCsTryToSpendExtraAP", 0);
	if (RetryCombatMinAP > 0) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, RetryCombatHook); // combat_turn_
		dlogr(" Done", DL_INIT);
	}

	#ifndef NDEBUG
	if (iniGetInt("Debugging", "AIBugFixes", 1, ddrawIniDef) == 0) return;
	#endif

	// Fix for NPCs not fully reloading a weapon if it has more ammo capacity than a box of ammo
	const DWORD itemWReloadAddr[] = {
		0x42AF15,           // cai_attempt_w_reload_
		0x42A970, 0x42AA56, // ai_try_attack_
	};
	HookCalls(item_w_reload_hook, itemWReloadAddr);

	// Adds a check for the weapon range and the AP cost when AI is choosing weapon attack modes
	HookCall(0x429F6D, ai_pick_hit_mode_hook);

	/////////////////////// Combat AI behavior fixes ///////////////////////

	// Fix to reduce friendly fire in burst attacks
	// Adds a check/roll for friendly critters in the line of fire when AI uses burst attacks
	HookCall(0x421666, combat_safety_invalidate_weapon_func_hook_check);

	// Fix AI weapon switching when not having enough action points
	// AI will try to change attack mode before deciding to switch weapon
	HookCall(0x42AB57, ai_try_attack_hook_switch_weapon);

	// Fix for duplicate critters being added to the list of potential targets for AI
	MakeCall(0x428E75, ai_find_attackers_hack_target2, 2);
	MakeCall(0x428EB5, ai_find_attackers_hack_target3);
	MakeCall(0x428EE5, ai_find_attackers_hack_target4, 1);

	// Tweak for finding new targets for party members
	// Save the current target in the "target1" variable and find other potential targets
	MakeCall(0x429074, ai_danger_source_hack_pm_newFind);
	SafeWrite16(0x429074 + 5, 0x47EB); // jmp 0x4290C2

	// Fix to allow fleeing NPC to use drugs
	MakeCall(0x42B1DC, combat_ai_hack);
	// Fix for AI not checking minimum hp properly for using stimpaks (prevents premature fleeing)
	HookCall(0x428579, ai_check_drugs_hook);

	// Fix for NPC stuck in fleeing mode when the hit chance of a target was too low
	HookCall(0x42B1E3, combat_ai_hook_FleeFix);
	const DWORD aiTryAttackFleeAddr[] = {0x42ABA8, 0x42ACE5};
	HookCalls(ai_try_attack_hook_FleeFix, aiTryAttackFleeAddr);

	// Restore combat flags after fleeing when NPC cannot move closer to target
	HookCall(0x42ADF6, ai_try_attack_hook_runFix);

	// Fix AI target selection for combat_check_bad_shot_ function returning a no_ammo result
	const DWORD aiDangerSrcBadShotAddr[] = {0x42903A, 0x42918A};
	HookCalls(ai_danger_source_hook, aiDangerSrcBadShotAddr);

	// Fix AI behavior for "Snipe" distance preference
	// The attacker will try to shoot the target instead of always running away from it at the beginning of the turn
	MakeCall(0x42B086, cai_perform_distance_prefs_hack);

	// Fix for ai_move_away_ engine function not working correctly in cases when needing to move a distance away from the target
	// now the function also takes the distance argument in a negative value for moving away at a distance
	HookCall(0x4289A7, ai_move_away_hook);
	// also patch combat_safety_invalidate_weapon_func_ for returning out_range argument in a negative value
	SafeWrite8(0x421628, 0xD0);    // sub edx, eax > sub eax, edx
	SafeWrite16(0x42162A, 0xFF40); // lea eax, [edx+1] > lea eax, [eax-1]

	// Check the safety of weapons based on the selected attack mode instead of always the primary weapon hit mode
	MakeCall(0x42A8D9, ai_try_attack_hack_check_safe_weapon);
}

TGameObj* __stdcall AIGetLastAttacker(TGameObj* target) {
	iter itr = sources.find(target);
	return (itr != sources.end()) ? itr->second : 0;
}

TGameObj* __stdcall AIGetLastTarget(TGameObj* source) {
	iter itr = targets.find(source);
	return (itr != targets.end()) ? itr->second : 0;
}
