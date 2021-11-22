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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SimplePatch.h"
#include "..\Translate.h"

#include "HookScripts.h"
#include "Objects.h"

#include "Combat.h"

namespace sfall
{

static const DWORD bodypartAddr[] = {
	0x425562,                     // combat_display_
	0x42A68F, 0x42A739,           // ai_called_shot_
//	0x429E82, 0x429EC2, 0x429EFF, // ai_pick_hit_mode_ (used for hit mode)
	0x423231, 0x423268,           // check_ranged_miss_
	0x4242D4,                     // attack_crit_failure_
	// for combat_ctd_init_ func
	0x412E9C,                     // action_explode_
	0x413519,                     // action_dmg_
	0x421656, 0x421675,           // combat_safety_invalidate_weapon_func_
	0x421CC0,                     // combat_begin_extra_
	0x4229A9,                     // combat_turn_
	0x42330E, 0x4233A6, 0x4233AB, // shoot_along_path_
	0x423E25, 0x423E2A,           // compute_explosion_on_extras_
	0x425F83,                     // combat_anim_finished_
	0x42946D,                     // ai_best_weapon_
	0x46FCC8,                     // exit_inventory_
	0x49C00C,                     // protinstTestDroppedExplosive_
};

static struct {
	long Head;
	long Left_Arm;
	long Right_Arm;
	long Torso;
	long Right_Leg;
	long Left_Leg;
	long Eyes;
	long Groin;
	long Uncalled;
} bodyPartHit;

struct KnockbackModifier {
	long id;
	DWORD type;
	double value;
};

long Combat::determineHitChance; // the value of hit chance w/o any cap

static std::vector<long> noBursts; // critter id

static std::vector<KnockbackModifier> mTargets;
static std::vector<KnockbackModifier> mAttackers;
static std::vector<KnockbackModifier> mWeapons;

static std::vector<ChanceModifier> hitChanceMods;
static ChanceModifier baseHitChance;

static bool hookedAimedShot;
static std::vector<DWORD> disabledAS;
static std::vector<DWORD> forcedAS;

static bool checkWeaponAmmoCost;

///////////////////////////////// COMBAT BLOCK /////////////////////////////////

static bool combatDisabled;

static void __stdcall CombatBlocked() {
	fo::func::display_print(Translate::CombatBlockMessage());
}

static void __declspec(naked) intface_use_item_hook() {
	static const DWORD BlockCombatHook1Ret1 = 0x45F6AF;
	static const DWORD BlockCombatHook1Ret2 = 0x45F6D7;
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
		mov  eax, dword ptr ds:[FO_VAR_intfaceEnabled];
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

static void __declspec(naked) ai_can_use_weapon_hack() {
	using namespace fo;
	using namespace Fields;
	__asm {
		test dword ptr [esi + miscFlags], CantUse;
		jnz  cantUse;
		mov  eax, [edi + damageFlags];
		retn;
cantUse:
		mov  al, 0xFF;
		retn;
	}
}

static void __declspec(naked) can_use_weapon_hook() {
	static const DWORD cant_use_weapon_Ret = 0x477F9F;
	using namespace fo;
	using namespace Fields;
	__asm {
		call fo::funcoffs::item_get_type_;
		cmp  eax, item_type_weapon;
		je   checkFlag;
		retn; // eax - type
checkFlag:
		test dword ptr [edx + miscFlags], CantUse;
		jnz  cantUse;
		retn; // eax - type
cantUse:
		add  esp, 4;
		jmp  cant_use_weapon_Ret;
	}
}

// Note: in ai_try_attack_, the attacker will not be able to change unusable weapon, as it happens with crippled arms
static void __declspec(naked) combat_check_bad_shot_hack() {
	static const DWORD combat_check_bad_shot_Ret = 0x42673A;
	using namespace fo;
	using namespace Fields;
	__asm {
		test dword ptr [ecx + miscFlags], CantUse;
		jnz  cantUse;
		mov  eax, [esi + damageFlags];
		test al, DAM_CRIP_ARM_LEFT;
		retn;
cantUse:
		mov  eax, 4; // result same as TargetDead
		add  esp, 4;
		jmp  combat_check_bad_shot_Ret;
	}
}

void __stdcall SetBlockCombat(long toggle) {
	combatDisabled = toggle != 0;
}

////////////////////////////////////////////////////////////////////////////////

// Compares the cost (required count of rounds) for one shot with the current amount of ammo to make an attack or other checks
long __fastcall Combat::check_item_ammo_cost(fo::GameObject* weapon, fo::AttackType hitMode) {
	long currAmmo = fo::func::item_w_curr_ammo(weapon);
	if (!checkWeaponAmmoCost || currAmmo <= 0) return currAmmo;

	long rounds = 1; // default ammo for single shot

	long anim = fo::func::item_w_anim_weap(weapon, hitMode);
	if (anim == fo::Animation::ANIM_fire_burst || anim == fo::Animation::ANIM_fire_continuous) {
		rounds = fo::func::item_w_rounds(weapon); // ammo in burst
	}

	DWORD newRounds = rounds;

	AmmoCostHook_Script(1, weapon, newRounds); // newRounds returns the new "ammo" value multiplied by the cost

	// calculate the cost
	long cost = (newRounds != rounds) ? newRounds / rounds : 1; // 1 - default cost
	return (cost > currAmmo) ? 0 : currAmmo; // 0 - this will force "Not Enough Ammo"
}

// adds check for weapons which require more than 1 ammo for single shot (super cattle prod & mega power fist) and burst rounds
static void __declspec(naked) combat_check_bad_shot_hook() {
	__asm {
		push edx;
		push ecx;      // weapon
		mov  edx, edi; // hitMode
		call Combat::check_item_ammo_cost;
		pop  ecx;
		pop  edx;
		retn;
	}
}

// check if there is enough ammo to shoot
static void __declspec(naked) ai_search_inven_weap_hook() {
	using namespace fo;
	__asm {
		push ecx;
		mov  edx, ATKTYPE_RWEAPON_PRIMARY; // hitMode
		mov  ecx, eax;                     // weapon
		call Combat::check_item_ammo_cost; // enough ammo?
		pop  ecx;
		retn;
	}
}

// switch weapon mode from secondary to primary if there is not enough ammo to shoot
static void __declspec(naked) ai_try_attack_hook() {
	static const DWORD ai_try_attack_search_ammo = 0x42AA1E;
	static const DWORD ai_try_attack_continue = 0x42A929;
	using namespace fo;
	using namespace Fields;
	__asm {
		mov  ebx, [esp + 0x364 - 0x38]; // hit mode
		cmp  ebx, ATKTYPE_RWEAPON_SECONDARY;
		jne  searchAmmo;
		mov  edx, [esp + 0x364 - 0x3C]; // weapon
		mov  eax, [edx + charges];      // curr ammo
		test eax, eax;
		jnz  tryAttack;                 // have ammo
searchAmmo:
		jmp  ai_try_attack_search_ammo;
tryAttack:
		mov  ebx, ATKTYPE_RWEAPON_PRIMARY;
		mov  [esp + 0x364 - 0x38], ebx; // change hit mode
		jmp  ai_try_attack_continue;
	}
}

/*** The return value should set the correct count of rounds for the burst calculated based on the cost ***/
static long __fastcall divide_burst_rounds_by_ammo_cost(long currAmmo, fo::GameObject* weapon, long burstRounds) {
	DWORD roundsCost = 1; // default burst cost

	roundsCost = burstRounds;                   // rounds in burst (the number of rounds fired in the burst)
	AmmoCostHook_Script(2, weapon, roundsCost); // roundsCost returns the new cost

	long cost = burstRounds * roundsCost; // amount of ammo required for this burst (multiplied by 1 or by the value returned from HOOK_AMMOCOST)
	if (cost > currAmmo) cost = currAmmo; // if cost ammo more than current ammo, set it to current

	return (cost / roundsCost);           // divide back to get proper number of rounds for damage calculations
}

static void __declspec(naked) compute_spray_hack() {
	__asm { // ebp = current ammo
//		push edx;      // weapon
//		push ecx;      // current ammo in weapon
		push eax;      // rounds in burst attack, need to set ebp
		call divide_burst_rounds_by_ammo_cost;
		mov  ebp, eax; // overwriten code
//		pop  ecx;
//		pop  edx;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

static double ApplyModifiers(std::vector<KnockbackModifier>* mods, fo::GameObject* object, double val) {
	for (size_t i = 0; i < mods->size(); i++) {
		KnockbackModifier* mod = &(*mods)[i];
		if (mod->id == object->id) {
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

static DWORD __fastcall CalcKnockbackMod(int knockValue, int damage, fo::GameObject* weapon, fo::GameObject* attacker, fo::GameObject* target) {
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

static void __declspec(naked) compute_dmg_damage_hack() {
	static const DWORD KnockbackRetAddr = 0x4136E1;
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

static int __fastcall HitChanceMod(int base, fo::GameObject* critter) {
	Combat::determineHitChance = base;
	for (size_t i = 0; i < hitChanceMods.size(); i++) {
		if (critter->id == hitChanceMods[i].id) {
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

bool __stdcall Combat::IsBurstDisabled(fo::GameObject* critter) {
	for (size_t i = 0; i < noBursts.size(); i++) {
		if (noBursts[i] == critter->id) return true;
	}
	return false;
}

static long __fastcall CheckDisableBurst(fo::GameObject* critter, fo::GameObject* weapon, fo::AIcap* cap) {
	if (Combat::IsBurstDisabled(critter)) {
		long anim = fo::func::item_w_anim_weap(weapon, fo::AttackType::ATKTYPE_RWEAPON_SECONDARY);
		if (anim == fo::Animation::ANIM_fire_burst || anim == fo::Animation::ANIM_fire_continuous) {
			return 10; // Disable Burst (area_attack_mode - non-existent value)
		}
	}
	return cap->area_attack_mode; // default engine code
}

static void __declspec(naked) ai_pick_hit_mode_hack_noBurst() {
	__asm {
		push eax;
		push ecx;
		push eax;      // cap
		mov  edx, ebp; // weapon
		mov  ecx, esi; // source
		call CheckDisableBurst;
		mov  ebx, eax;
		pop  ecx;
		pop  eax;
		retn;
	}
}

void __stdcall KnockbackSetMod(fo::GameObject* object, DWORD type, float val, DWORD mode) {
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

	long id = (mode == 0)
	        ? Objects::SetSpecialID(object)
	        : Objects::SetObjectUniqueID(object);

	KnockbackModifier mod = { id, type, (double)val };
	for (size_t i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == id) {
			(*mods)[i] = mod;
			return;
		}
	}
	mods->push_back(mod);
}

void __stdcall KnockbackRemoveMod(fo::GameObject* object, DWORD mode) {
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
	for (size_t i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == object->id) {
			mods->erase(mods->begin() + i);
			if (mode == 0) Objects::SetNewEngineID(object); // revert to engine range id
			return;
		}
	}
}

void __stdcall SetHitChanceMax(fo::GameObject* critter, DWORD maximum, DWORD mod) {
	if ((DWORD)critter == -1) {
		baseHitChance.maximum = maximum;
		baseHitChance.mod = mod;
		return;
	}
	if (critter->IsNotCritter()) return;
	long id = Objects::SetObjectUniqueID(critter);
	for (size_t i = 0; i < hitChanceMods.size(); i++) {
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

void __stdcall SetNoBurstMode(fo::GameObject* critter, bool on) {
	if (critter->protoId == fo::PID_Player || critter->IsNotCritter()) return;

	long id = Objects::SetObjectUniqueID(critter);
	for (size_t i = 0; i < noBursts.size(); i++) {
		if (noBursts[i] == id) {
			if (!on) noBursts.erase(noBursts.begin() + i); // off
			return;
		}
	}
	if (on) noBursts.push_back(id);
}

static int __fastcall AimedShotTest(DWORD pid) {
	if (pid) pid = ((fo::GameObject*)pid)->protoId;
	for (size_t i = 0; i < disabledAS.size(); i++) {
		if (disabledAS[i] == pid) return -1;
	}
	for (size_t i = 0; i < forcedAS.size(); i++) {
		if (forcedAS[i] == pid) return 1;
	}
	return 0;
}

static void __declspec(naked) item_w_called_shot_hook() {
	static const DWORD aimedShotRet1 = 0x478EE4;
	static const DWORD aimedShotRet2 = 0x478EEA;
	__asm {
		push edx;
		mov  ecx, edx; // item
		call AimedShotTest;
		test eax, eax;
		jg   force;
		jl   disable;
		pop  edx;
		mov  eax, ebx;
		jmp  fo::funcoffs::item_w_damage_type_;
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

void __stdcall DisableAimedShots(DWORD pid) {
	if (!hookedAimedShot) HookAimedShots();
	for (size_t i = 0; i < forcedAS.size(); i++) {
		if (forcedAS[i] == pid) forcedAS.erase(forcedAS.begin() + (i--));
	}
	for (size_t i = 0; i < disabledAS.size(); i++) {
		if (disabledAS[i] == pid) return;
	}
	disabledAS.push_back(pid);
}

void __stdcall ForceAimedShots(DWORD pid) {
	if (!hookedAimedShot) HookAimedShots();
	for (size_t i = 0; i < disabledAS.size(); i++) {
		if (disabledAS[i] == pid) disabledAS.erase(disabledAS.begin() + (i--));
	}
	for (size_t i = 0; i < forcedAS.size(); i++) {
		if (forcedAS[i] == pid) return;
	}
	forcedAS.push_back(pid);
}

static void BodypartHitChances() {
	using fo::ptr::hit_location_penalty;
	hit_location_penalty[0] = bodyPartHit.Head;
	hit_location_penalty[1] = bodyPartHit.Left_Arm;
	hit_location_penalty[2] = bodyPartHit.Right_Arm;
	hit_location_penalty[3] = bodyPartHit.Torso;
	hit_location_penalty[4] = bodyPartHit.Right_Leg;
	hit_location_penalty[5] = bodyPartHit.Left_Leg;
	hit_location_penalty[6] = bodyPartHit.Eyes;
	hit_location_penalty[7] = bodyPartHit.Groin;
	hit_location_penalty[8] = bodyPartHit.Uncalled;
}

static void BodypartHitReadConfig() {
	bodyPartHit.Head      = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Head",          -40));
	bodyPartHit.Left_Arm  = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Left_Arm",      -30));
	bodyPartHit.Right_Arm = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Right_Arm",     -30));
	bodyPartHit.Torso     = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Torso",           0));
	bodyPartHit.Right_Leg = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Right_Leg",     -20));
	bodyPartHit.Left_Leg  = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Left_Leg",      -20));
	bodyPartHit.Eyes      = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Eyes",          -60));
	bodyPartHit.Groin     = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Groin",         -30));
	bodyPartHit.Uncalled  = static_cast<long>(IniReader::GetConfigInt("Misc", "BodyHit_Torso_Uncalled",  0));
}

static void __declspec(naked)  ai_pick_hit_mode_hook_bodypart() {
	using fo::Uncalled;
	__asm {
		mov  ebx, Uncalled; // replace Body_Torso with Body_Uncalled
		jmp  fo::funcoffs::determine_to_hit_;
	}
}

////////////////////////////////////////////////////////////////////////////////

static void __declspec(naked) apply_damage_hack_main() {
	using namespace fo::Fields;
	__asm {
		mov  edx, [eax + teamNum]; // ctd.target.team_num
		mov  ecx, [ebx + teamNum]; // ctd.source.team_num (attacker)
		cmp  edx, ecx;
		jnz  check;
		retn; // skip combatai_check_retaliation_
check:  // does the target belong to the player's team?
		test edx, edx;
		jz   dudeTeam;
		retn; // call combatai_check_retaliation_
dudeTeam: // check who the attacker was attacking
		mov  ecx, [esi + ctdMainTarget]; // ctd.mainTarget
		cmp  edx, [ecx + teamNum];       // dude.team_num == mainTarget.team_num?
		jne  skipSetHitTarget;           // target is not main
		or   edx, 1;
		retn; // call combatai_check_retaliation_
skipSetHitTarget:
		xor  edx, edx;
		retn;
	}
}

static void __declspec(naked) apply_damage_hook_extra() {
	using namespace fo::Fields;
	__asm { // eax - target1-6
		// does the target belong to the player's team?
		test ebx, ebx; // target.team_num
		jz   dudeTeam;
default:
		jmp  fo::funcoffs::combatai_check_retaliation_;
dudeTeam: // check who the attacker was attacking
		mov  ecx, [esi + ctdMainTarget]; // ctd.mainTarget
		cmp  ebx, [ecx + teamNum];       // dude.team_num == mainTarget.team_num?
		je   default;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

// Ray's combat_p_proc patch
static void __declspec(naked) apply_damage_hack() {
	using namespace fo::Fields;
	__asm {
		xor  edx, edx;
		inc  edx;                          // 1 - COMBAT_SUBTYPE_WEAPON_USED
		test [esi + ctdAttackerFlags], dl; // ctd.flags2Source & DAM_HIT_
		jz   end;                          // no hit
		inc  edx;                          // 2 - COMBAT_SUBTYPE_HIT_SUCCEEDED
end:
		retn;
	}
}

static void CombatProcPatch() {
	dlog("Applying Ray's combat_p_proc patch.", DL_INIT);
	MakeCall(0x424DD9, apply_damage_hack);
	SafeWrite16(0x424DC6, 0x9090);
	dlogr(" Done", DL_INIT);
}

void Combat::OnBeforeGameStart() {
	BodypartHitChances(); // set on start & load
}

void Combat::OnGameLoad() {
	baseHitChance.SetDefault();
	mTargets.clear();
	mAttackers.clear();
	mWeapons.clear();
	hitChanceMods.clear();
	noBursts.clear();
	disabledAS.clear();
	forcedAS.clear();
}

void Combat::init() {
	HookCall(0x45F626, intface_use_item_hook); // jnz hook
	HookCall(0x4432A6, game_handle_input_hook);

	// Add an additional "Can't Use" flag to the misc flags of item objects (offset 0x0038)
	// Misc Flags:
	// 0x00000010 - Can't Use (makes the weapon object unusable in combat)
	HookCall(0x477F4C, can_use_weapon_hook);
	MakeCall(0x4298F4, ai_can_use_weapon_hack);
	MakeCall(0x426669, combat_check_bad_shot_hack);

	CombatProcPatch();

	// Prevents NPC aggression when non-hostile NPCs accidentally hit the player or members of the player's team
	MakeCall(0x424D6B, apply_damage_hack_main, 1);
	HookCall(0x424E58, apply_damage_hook_extra);

	MakeCall(0x424B76, compute_damage_hack, 2);     // KnockbackMod
	MakeJump(0x4136D3, compute_dmg_damage_hack);    // for op_critter_dmg

	MakeCall(0x424791, determine_to_hit_func_hack); // HitChanceMod
	BlockCall(0x424796);

	// Disables secondary burst attacks for the critter
	MakeCall(0x429E44, ai_pick_hit_mode_hack_noBurst, 1);

	checkWeaponAmmoCost = (IniReader::GetConfigInt("Misc", "CheckWeaponAmmoCost", 0) != 0);
	if (checkWeaponAmmoCost) {
		MakeCall(0x4234B3, compute_spray_hack, 1);
		HookCall(0x4266E9, combat_check_bad_shot_hook);
		HookCall(0x429A37, ai_search_inven_weap_hook); // check if there is enough ammo to shoot
		HookCall(0x42A95D, ai_try_attack_hook); // jz func
	}

	SimplePatch<DWORD>(0x424FA7, "Misc", "KnockoutTime", 35, 35, 100);

	BodypartHitReadConfig();

	// Remove the dependency of Body_Torso from Body_Uncalled
	SafeWrite8(0x423830, CodeType::JumpShort); // compute_attack_
	BlockCall(0x42303F); // block Body_Torso check (combat_attack_)
	SafeWrite8(0x42A713, fo::BodyPart::Groin); // Body_Uncalled > Body_Groin (ai_called_shot_)
	SafeWriteBatch<BYTE>(fo::BodyPart::Uncalled, bodypartAddr); // replace Body_Torso with Body_Uncalled
	const DWORD pickHitBodypartAddr[] = {0x429E8C, 0x429ECC, 0x429F09};
	HookCalls(ai_pick_hit_mode_hook_bodypart, pickHitBodypartAddr);
}

}
