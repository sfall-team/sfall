/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2011, 2012  The sfall team
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

#include <math.h>

#include "main.h"

#include "FalloutEngine.h"
#include "HookScripts.h"
#include "Objects.h"

#include "Combat.h"

static const DWORD bodypartAddr[] = {
	0x425562,                     // combat_display_
	0x42A68F, 0x42A739,           // ai_called_shot_
	0x429E82, 0x429EC2, 0x429EFF, // ai_pick_hit_mode_
	0x423231, 0x423268,           // check_ranged_miss_
	0x4242D4,                     // attack_crit_failure_
	// for combat_ctd_init_ func
	0x412E9C,                     // action_explode_
	0x413519,                     // action_dmg_
	0x421656, 0x421675,           // combat_safety_invalidate_weapon_func_
	0x421CC0,                     // combat_begin_extra_
	0x4229A9,                     // combat_turn_
	0x42330E, 0x4233AB,           // shoot_along_path_
	0x423E25, 0x423E2A,           // compute_explosion_on_extras_
	0x425F83,                     // combat_anim_finished_
	0x42946D,                     // ai_best_weapon_
	0x46FCC8,                     // exit_inventory_
	0x49C00C,                     // protinstTestDroppedExplosive_
};

static struct BodyParts {
	long Head;
	long Left_Arm;
	long Right_Arm;
	long Torso;
	long Right_Leg;
	long Left_Leg;
	long Eyes;
	long Groin;
	long Uncalled;
} bodypartHit;

struct KnockbackModifier {
	long id;
	DWORD type;
	double value;
};

static std::vector<long> noBursts; // object id

static std::vector<KnockbackModifier> mTargets;
static std::vector<KnockbackModifier> mAttackers;
static std::vector<KnockbackModifier> mWeapons;

static std::vector<ChanceModifier> hitChanceMods;
static ChanceModifier baseHitChance;

static bool hookedAimedShot;
static std::vector<DWORD> disabledAS;
static std::vector<DWORD> forcedAS;


static bool combatDisabled;
static char combatBlockedMessage[128];

static void _stdcall CombatBlocked() {
	DisplayConsoleMessage(combatBlockedMessage);
}

static const DWORD BlockCombatHook1Ret1 = 0x45F6AF;
static const DWORD BlockCombatHook1Ret2 = 0x45F6D7;
static void __declspec(naked) intface_use_item_hook() {
	__asm {
		cmp  combatDisabled, 0;
		jne  block;
		jmp  BlockCombatHook1Ret1;
block:
		call CombatBlocked;
		jmp  BlockCombatHook1Ret2;
	}
}

static void __declspec(naked) game_handle_input_hook() {
	__asm {
		mov  eax, dword ptr ds:[_intfaceEnabled];
		test eax, eax;
		jz   end;
		cmp  combatDisabled, 0; // eax = 1
		je   end; // no blocked
		push edx;
		call CombatBlocked;
		pop  edx;
		xor  eax, eax;
end:
		retn;
	}
}

void __stdcall SetBlockCombat(long toggle) {
	combatDisabled = toggle != 0;
}

static DWORD __fastcall add_check_for_item_ammo_cost(register TGameObj* weapon, DWORD hitMode) {
	DWORD rounds = 1;

	DWORD anim = ItemWAnimWeap(weapon, hitMode);
	if (anim == 46 || anim == 47) {   // ANIM_fire_burst or ANIM_fire_continuous
		rounds = ItemWRounds(weapon); // ammo in burst
	}
	AmmoCostHook_Script(1, weapon, rounds); // get rounds cost from hook
	DWORD currAmmo = ItemWCurrAmmo(weapon);

	DWORD cost = 1; // default cost
	if (currAmmo > 0) {
		cost = rounds / currAmmo;
		if (rounds % currAmmo) cost++; // round up
	}
	return (cost > currAmmo) ? 0 : 1;  // 0 - this will force "Out of ammo", 1 - this will force success (enough ammo)
}

// adds check for weapons which require more than 1 ammo for single shot (super cattle prod & mega power fist) and burst rounds
static void __declspec(naked) combat_check_bad_shot_hook() {
	__asm {
		push edx;
		push ecx;         // weapon
		mov  edx, edi;    // hitMode
		call add_check_for_item_ammo_cost;
		pop  ecx;
		pop  edx;
		retn;
	}
}

// check if there is enough ammo to shoot
static void __declspec(naked) ai_search_inven_weap_hook() {
	__asm {
		push ecx;
		mov  ecx, eax;                      // weapon
		mov  edx, 2;                        // hitMode - ATKTYPE_RWEAPON_PRIMARY
		call add_check_for_item_ammo_cost;  // enough ammo?
		pop  ecx;
		retn;
	}
}

// switch weapon mode from secondary to primary if there is not enough ammo to shoot
static const DWORD ai_try_attack_search_ammo = 0x42AA1E;
static const DWORD ai_try_attack_continue = 0x42A929;
static void __declspec(naked) ai_try_attack_hook() {
	__asm {
		mov  ebx, [esp + 0x364 - 0x38]; // hit mode
		cmp  ebx, 3;                    // ATKTYPE_RWEAPON_SECONDARY
		jne  searchAmmo;
		mov  edx, [esp + 0x364 - 0x3C]; // weapon
		mov  eax, [edx + 0x3C];         // curr ammo
		test eax, eax;
		jnz  tryAttack;                 // have ammo
searchAmmo:
		jmp  ai_try_attack_search_ammo;
tryAttack:
		mov  ebx, 2;                    // ATKTYPE_RWEAPON_PRIMARY
		mov  [esp + 0x364 - 0x38], ebx; // change hit mode
		jmp  ai_try_attack_continue;
	}
}

static DWORD __fastcall divide_burst_rounds_by_ammo_cost(TGameObj* weapon, register DWORD currAmmo, DWORD burstRounds) {
	DWORD rounds = 1; // default multiply

	rounds = burstRounds;                 // rounds in burst
	AmmoCostHook_Script(2, weapon, rounds);

	DWORD cost = burstRounds * rounds;    // so much ammo is required for this burst
	if (cost > currAmmo) cost = currAmmo; // if cost ammo more than current ammo, set it to current

	return (cost / rounds);               // divide back to get proper number of rounds for damage calculations
}

static void __declspec(naked) compute_spray_hack() {
	__asm {
		push edx;         // weapon
		push ecx;         // current ammo in weapon
		xchg ecx, edx;
		push eax;         // eax - rounds in burst attack, need to set ebp
		call divide_burst_rounds_by_ammo_cost;
		mov  ebp, eax;    // overwriten code
		pop  ecx;
		pop  edx;
		retn;
	}
}

static double ApplyModifiers(std::vector<KnockbackModifier>* mods, TGameObj* object, double val) {
	for (DWORD i = 0; i < mods->size(); i++) {
		KnockbackModifier* mod = &(*mods)[i];
		if (mod->id == object->ID) {
			switch (mod->type) {
			case 0:
				val = mod->value;
				break;
			case 1:
				val *= mod->value;
				break;
			}
			break;
		}
	}
	return val;
}

static DWORD __fastcall CalcKnockbackMod(int knockValue, int damage, TGameObj* weapon, TGameObj* attacker, TGameObj* target) {
	double result = (double)damage / (double)knockValue;
	if (weapon) result = ApplyModifiers(&mWeapons, weapon, result);
	if (attacker) result = ApplyModifiers(&mAttackers, attacker, result);
	result = ApplyModifiers(&mTargets, target, result);
	return (DWORD)floor(result);
}

static void __declspec(naked) compute_damage_hack() {
	__asm {
		mov  eax, [esp + 0x14 + 4];
		push eax;               // Target
		mov  eax, [esi];
		push eax;               // Attacker
		mov  eax, [esi + 8];
		push eax;               // Weapon
		call CalcKnockbackMod;  // ecx - Knockback value (5 or 10), edx - Damage
		retn;
	}
}

static const DWORD KnockbackRetAddr = 0x4136E1;
static void __declspec(naked) compute_dmg_damage_hack() {
	__asm {
		push ecx
		push esi;               // Target
		xor  eax, eax;
		push eax;               // Attacker (no attacker)
		push eax;               // Weapon   (no weapon)
		mov  edx, ebx;          // Damage
		mov  ecx, 10;           // Knockback value
		call CalcKnockbackMod;
		pop  ecx
		jmp  KnockbackRetAddr;
	}
}

static int __fastcall HitChanceMod(int base, TGameObj* critter) {
	for (DWORD i = 0; i < hitChanceMods.size(); i++) {
		if (critter->ID == hitChanceMods[i].id) {
			return min(base + hitChanceMods[i].mod, hitChanceMods[i].maximum);
		}
	}
	return min(base + baseHitChance.mod, baseHitChance.maximum);
}

static void __declspec(naked) determine_to_hit_func_hack() {
	__asm {
		mov  edx, edi;          // critter
		mov  ecx, esi;          // base (calculated hit chance)
		call HitChanceMod;
		mov  esi, eax;
		retn;
	}
}

static long __fastcall CheckDisableBurst(TGameObj* critter) {
	for (size_t i = 0; i < noBursts.size(); i++) {
		if (noBursts[i] == critter->ID) {
			return 10; // Disable Burst (area_attack_mode - non-existent value)
		}
	}
	return 0;
}

static void __declspec(naked) ai_pick_hit_mode_hack() {
	__asm {
		mov  ebx, [eax + 0x94]; // cap->area_attack_mode
		push eax;
		push ecx;
		mov  ecx, esi;          // source
		call CheckDisableBurst;
		test eax, eax;
		cmovnz ebx, eax;
		pop  ecx;
		pop  eax;
		retn;
	}
}

void _stdcall KnockbackSetMod(TGameObj* object, DWORD type, float val, DWORD mode) {
	std::vector<KnockbackModifier>* mods;
	switch (mode) {
	case 0:
		if (object->pid >> 24 != OBJ_TYPE_ITEM) {
			DebugPrintf("\nOPCODE ERROR: set_weapon_knockback() - the object is not an item.");
			return;
		}
		mods = &mWeapons;
		break;
	case 1:
		if (object->pid >> 24 != OBJ_TYPE_CRITTER) {
			DebugPrintf("\nOPCODE ERROR: set_target_knockback() - the object is not a critter.");
			return;
		}
		mods = &mTargets;
		break;
	case 2:
		if (object->pid >> 24 != OBJ_TYPE_CRITTER) {
			DebugPrintf("\nOPCODE ERROR: set_attacker_knockback() - the object is not a critter.");
			return;
		}
		mods = &mAttackers;
		break;
	default:
		return;
	}

	long id = (mode == 0)
			? SetSpecialID(object)
			: SetObjectUniqueID(object);

	KnockbackModifier mod = { id, type, (double)val };
	for (DWORD i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == id) {
			(*mods)[i] = mod;
			return;
		}
	}
	mods->push_back(mod);
}

void _stdcall KnockbackRemoveMod(TGameObj* object, DWORD mode) {
	std::vector<KnockbackModifier>* mods;
	switch (mode) {
	case 0:
		mods = &mWeapons;
		break;
	case 1:
		mods = &mTargets;
		break;
	case 2:
		mods = &mAttackers;
		break;
	default:
		return;
	}
	for (DWORD i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == object->ID) {
			mods->erase(mods->begin() + i);
			if (mode == 0) SetNewEngineID(object); // revert to engine range id
			return;
		}
	}
}

void _stdcall SetHitChanceMax(TGameObj* critter, DWORD maximum, DWORD mod) {
	if ((DWORD)critter == -1) {
		baseHitChance.maximum = maximum;
		baseHitChance.mod = mod;
		return;
	}
	if (critter->pid >> 24 != OBJ_TYPE_CRITTER) return;
	long id = SetObjectUniqueID(critter);
	for (DWORD i = 0; i < hitChanceMods.size(); i++) {
		if (id == hitChanceMods[i].id) {
			hitChanceMods[i].maximum = maximum;
			hitChanceMods[i].mod = mod;
			return;
		}
	}
	ChanceModifier cm;
	cm.id = id;
	cm.maximum = maximum;
	cm.mod = mod;
	hitChanceMods.push_back(cm);
}

void _stdcall SetNoBurstMode(TGameObj* critter, bool on) {
	if (critter == *ptr_obj_dude || critter->pid >> 24 != OBJ_TYPE_CRITTER) return;

	long id = SetObjectUniqueID(critter);
	for (size_t i = 0; i < noBursts.size(); i++) {
		if (noBursts[i] == id) {
			if (!on) noBursts.erase(noBursts.begin() + i); // off
			return;
		}
	}
	if (on) noBursts.push_back(id);
}

static int __fastcall AimedShotTest(DWORD pid) {
	if (pid) pid = ((TGameObj*)pid)->pid;
	for (DWORD i = 0; i < disabledAS.size(); i++) {
		if (disabledAS[i] == pid) return -1;
	}
	for (DWORD i = 0; i < forcedAS.size(); i++) {
		if (forcedAS[i] == pid) return 1;
	}
	return 0;
}

static const DWORD aimedShotRet1 = 0x478EE4;
static const DWORD aimedShotRet2 = 0x478EEA;
static void __declspec(naked) item_w_called_shot_hook() {
	__asm {
		push edx;
		mov  ecx, edx;       // item
		call AimedShotTest;
		test eax, eax;
		jg   force;
		jl   disable;
		pop  edx;
		mov  eax, ebx;
		jmp  item_w_damage_type_;
force:
		add  esp, 8;
		jmp  aimedShotRet2;
disable:
		add  esp, 8;
		jmp  aimedShotRet1;
	}
}

static void HookAimedShots() {
	HookCall(0x478EC6, item_w_called_shot_hook); // AimedShot function
	hookedAimedShot = true;
}

void _stdcall DisableAimedShots(DWORD pid) {
	if (!hookedAimedShot) HookAimedShots();
	for (DWORD i = 0; i < forcedAS.size(); i++) {
		if (forcedAS[i] == pid) forcedAS.erase(forcedAS.begin() + (i--));
	}
	for (DWORD i = 0; i < disabledAS.size(); i++) {
		if (disabledAS[i] == pid) return;
	}
	disabledAS.push_back(pid);
}

void _stdcall ForceAimedShots(DWORD pid) {
	if (!hookedAimedShot) HookAimedShots();
	for (DWORD i = 0; i < disabledAS.size(); i++) {
		if (disabledAS[i] == pid) disabledAS.erase(disabledAS.begin() + (i--));
	}
	for (DWORD i = 0; i < forcedAS.size(); i++) {
		if (forcedAS[i] == pid) return;
	}
	forcedAS.push_back(pid);
}

static void BodypartHitChances() {
	ptr_hit_location_penalty[0] = bodypartHit.Head;
	ptr_hit_location_penalty[1] = bodypartHit.Left_Arm;
	ptr_hit_location_penalty[2] = bodypartHit.Right_Arm;
	ptr_hit_location_penalty[3] = bodypartHit.Torso;
	ptr_hit_location_penalty[4] = bodypartHit.Right_Leg;
	ptr_hit_location_penalty[5] = bodypartHit.Left_Leg;
	ptr_hit_location_penalty[6] = bodypartHit.Eyes;
	ptr_hit_location_penalty[7] = bodypartHit.Groin;
	ptr_hit_location_penalty[8] = bodypartHit.Uncalled;
}

static void BodypartHitReadConfig() {
	bodypartHit.Head      = static_cast<long>(GetConfigInt("Misc", "BodyHit_Head", -40));
	bodypartHit.Left_Arm  = static_cast<long>(GetConfigInt("Misc", "BodyHit_Left_Arm", -30));
	bodypartHit.Right_Arm = static_cast<long>(GetConfigInt("Misc", "BodyHit_Right_Arm", -30));
	bodypartHit.Torso     = static_cast<long>(GetConfigInt("Misc", "BodyHit_Torso", 0));
	bodypartHit.Right_Leg = static_cast<long>(GetConfigInt("Misc", "BodyHit_Right_Leg", -20));
	bodypartHit.Left_Leg  = static_cast<long>(GetConfigInt("Misc", "BodyHit_Left_Leg", -20));
	bodypartHit.Eyes      = static_cast<long>(GetConfigInt("Misc", "BodyHit_Eyes", -60));
	bodypartHit.Groin     = static_cast<long>(GetConfigInt("Misc", "BodyHit_Groin", -30));
	bodypartHit.Uncalled  = static_cast<long>(GetConfigInt("Misc", "BodyHit_Torso_Uncalled", 0));
}

static void __declspec(naked) apply_damage_hack() {
	__asm {
		xor  edx, edx;
		inc  edx;              // COMBAT_SUBTYPE_WEAPON_USED
		test [esi + 0x15], dl; // ctd.flags2Source & DAM_HIT_
		jz   end;              // no hit
		inc  edx;              // COMBAT_SUBTYPE_HIT_SUCCEEDED
end:
		retn;
	}
}

static void CombatProcFix() {
	//Ray's combat_p_proc fix
	dlog("Applying Ray's combat_p_proc patch.", DL_INIT);
	MakeCall(0x424DD9, apply_damage_hack);
	SafeWrite16(0x424DC6, 0x9090);
	dlogr(" Done", DL_INIT);
}

void Combat_OnGameLoad() {
	baseHitChance.maximum = 95;
	baseHitChance.mod = 0;
	mTargets.clear();
	mAttackers.clear();
	mWeapons.clear();
	hitChanceMods.clear();
	noBursts.clear();
	disabledAS.clear();
	forcedAS.clear();

	BodypartHitChances(); // was in ClearGlobalScripts()
}

void CombatInit() {
	HookCall(0x45F626, intface_use_item_hook); // jnz hook
	HookCall(0x4432A6, game_handle_input_hook);
	Translate("sfall", "BlockedCombat", "You cannot enter combat at this time.", combatBlockedMessage);

	CombatProcFix();

	MakeCall(0x424B76, compute_damage_hack, 2);     // KnockbackMod
	MakeJump(0x4136D3, compute_dmg_damage_hack);    // for op_critter_dmg

	MakeCall(0x424791, determine_to_hit_func_hack); // HitChanceMod
	BlockCall(0x424796);

	// Actually disables all secondary attacks for the critter, regardless of whether the weapon has a burst attack
	MakeCall(0x429E44, ai_pick_hit_mode_hack, 1);   // NoBurst

	if (GetConfigInt("Misc", "CheckWeaponAmmoCost", 0)) {
		MakeCall(0x4234B3, compute_spray_hack, 1);
		HookCall(0x4266E9, combat_check_bad_shot_hook);
		HookCall(0x429A37, ai_search_inven_weap_hook);
		HookCall(0x42A95D, ai_try_attack_hook); // jz func
	}

	SimplePatch<DWORD>(0x424FA7, "Misc", "KnockoutTime", 35, 35, 100);

	BodypartHitReadConfig();

	// Remove the dependency of Body_Torso from Body_Uncalled
	SafeWrite8(0x423830, 0xEB); // compute_attack_
	BlockCall(0x42303F); // block Body_Torso check (combat_attack_)
	SafeWrite8(0x42A713, 7); // Body_Uncalled > Body_Groin (ai_called_shot_)
	for (int i = 0; i < sizeof(bodypartAddr) / 4; i++) { // replace Body_Torso with Body_Uncalled
		SafeWrite8(bodypartAddr[i], 8);
	}
}
