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
#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Knockback.h"

namespace sfall
{

static std::vector<fo::GameObject*> noBursts;

struct KnockbackModifier {
	fo::GameObject* id;
	DWORD type;
	double value;
};

static std::vector<KnockbackModifier> mTargets;
static std::vector<KnockbackModifier> mAttackers;
static std::vector<KnockbackModifier> mWeapons;

struct ChanceModifier {
	fo::GameObject* id;
	int maximum { 95 };
	int mod { 0 };
};

static std::vector<ChanceModifier> hitChanceMods;
static std::vector<ChanceModifier> pickpocketMods;

static ChanceModifier baseHitChance;
static ChanceModifier basePickpocket;

static bool hookedAimedShot;
static std::vector<DWORD> disabledAS;
static std::vector<DWORD> forcedAS;

static double ApplyModifiers(std::vector<KnockbackModifier>* mods, fo::GameObject* id, double val) {
	for (DWORD i = 0; i < mods->size(); i++) {
		KnockbackModifier* mod = &(*mods)[i];
		if (mod->id == id) {
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

static DWORD _fastcall CalcKnockbackMod(int knockValue, int damage, fo::GameObject* weapon, fo::GameObject* attacker, fo::GameObject* target) {
	double result = (double)damage / (double)knockValue;
	result = ApplyModifiers(&mWeapons, weapon, result);
	result = ApplyModifiers(&mAttackers, attacker, result);
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
		call CalcKnockbackMod;  // ecx - Knockback value (5\10), edx - Damage
		retn;
	}
}

static const DWORD KnockbackRetAddr = 0x4136E1;
static void __declspec(naked) compute_dmg_damage_hack() {
	__asm {
		push ecx
		push esi;               // Target
		mov  eax, -1;
		push eax;               // Attacker
		push eax;               // Weapon
		mov  edx, ebx;          // Damage
		mov  ecx, 10;           // Knockback value
		call CalcKnockbackMod;
		pop  ecx
		jmp  KnockbackRetAddr;
	}
}

static int _fastcall PickpocketMod(int base, fo::GameObject* critter) {
	for (DWORD i = 0; i < pickpocketMods.size(); i++) {
		if (critter == pickpocketMods[i].id) {
			return min(base + pickpocketMods[i].mod, pickpocketMods[i].maximum);
		}
	}
	return min(base + basePickpocket.mod, basePickpocket.maximum);
}

static void __declspec(naked) skill_check_stealing_hack() {
	__asm {
		push edx;
		push ecx;
		mov  edx, esi;          // critter
		mov  ecx, eax;          // base (calculate chance)
		call PickpocketMod;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static int _fastcall HitChanceMod(int base, fo::GameObject* critter) {
	for (DWORD i = 0; i < hitChanceMods.size(); i++) {
		if (critter == hitChanceMods[i].id) {
			return min(base + hitChanceMods[i].mod, hitChanceMods[i].maximum);
		}
	}
	return min(base + baseHitChance.mod, baseHitChance.maximum);
}

static void __declspec(naked) determine_to_hit_func_hack() {
	__asm {
		mov  edx, edi;          // critter
		mov  ecx, esi;          // base (calculate hit)
		call HitChanceMod;
		mov  esi, eax;
		retn;
	}
}

static long _fastcall CheckDisableBurst(fo::GameObject* critter) {
	for (size_t i = 0; i < noBursts.size(); i++) {
		if (noBursts[i] == critter) {
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

void _stdcall KnockbackSetMod(fo::GameObject* id, DWORD type, float val, DWORD on) {
	std::vector<KnockbackModifier>* mods;
	switch (on) {
	case 0:
		mods = &mWeapons;
		break;
	case 1:
		mods = &mTargets;
		break;
	case 2:
		mods = &mAttackers;
		break;
	default: return;
	}
	KnockbackModifier mod = { id, type, (double)val };
	for (DWORD i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == id) {
			(*mods)[i] = mod;
			return;
		}
	}
	mods->push_back(mod);
}

void _stdcall KnockbackRemoveMod(fo::GameObject* id, DWORD on) {
	std::vector<KnockbackModifier>* mods;
	switch (on) {
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
		if ((*mods)[i].id == id) {
			mods->erase(mods->begin() + i);
			return;
		}
	}
}

void _stdcall SetHitChanceMax(fo::GameObject* critter, DWORD maximum, DWORD mod) {
	if ((DWORD)critter == -1) {
		baseHitChance.maximum = maximum;
		baseHitChance.mod = mod;
		return;
	}
	for (DWORD i = 0; i < hitChanceMods.size(); i++) {
		if (critter == hitChanceMods[i].id) {
			hitChanceMods[i].maximum = maximum;
			hitChanceMods[i].mod = mod;
			return;
		}
	}
	ChanceModifier cm;
	cm.id = critter;
	cm.maximum = maximum;
	cm.mod = mod;
	hitChanceMods.push_back(cm);
}

void _stdcall SetPickpocketMax(fo::GameObject* critter, DWORD maximum, DWORD mod) {
	if ((DWORD)critter == -1) {
		basePickpocket.maximum = maximum;
		basePickpocket.mod = mod;
		return;
	}
	for (DWORD i = 0; i < pickpocketMods.size(); i++) {
		if (critter == pickpocketMods[i].id) {
			pickpocketMods[i].maximum = maximum;
			pickpocketMods[i].mod = mod;
			return;
		}
	}
	ChanceModifier cm;
	cm.id = critter;
	cm.maximum = maximum;
	cm.mod = mod;
	pickpocketMods.push_back(cm);
}

void _stdcall SetNoBurstMode(fo::GameObject* critter, bool on) {
	for (size_t i = 0; i < noBursts.size(); i++) {
		if (noBursts[i] == critter) {
			if (!on) noBursts.erase(noBursts.begin() + i); // off
			return;
		}
	}
	if (on) noBursts.push_back(critter);
}

static int _fastcall AimedShotTest(DWORD pid) {
	if (pid) pid = ((fo::GameObject*)pid)->protoId;
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

static void ResetOnGameLoad() {
	baseHitChance.maximum = 95;
	baseHitChance.mod = 0;
	basePickpocket.maximum = 95;
	basePickpocket.mod = 0;
	mTargets.clear();
	mAttackers.clear();
	mWeapons.clear();
	hitChanceMods.clear();
	pickpocketMods.clear();
	noBursts.clear();
	disabledAS.clear();
	forcedAS.clear();
}

void Knockback::init() {

	MakeCall(0x424B76, compute_damage_hack, 2);     // KnockbackMod
	MakeJump(0x4136D3, compute_dmg_damage_hack);    // for op_critter_dmg

	MakeCall(0x424791, determine_to_hit_func_hack); // HitChanceMod
	BlockCall(0x424796);

	MakeCall(0x4ABC62, skill_check_stealing_hack);  // PickpocketMod
	SafeWrite8(0x4ABC67, 0x89);                     // mov [esp + 0x54], eax
	SafeWrite32(0x4ABC6B, 0x90909090);

	// Actually disables all secondary attacks for the critter, regardless of whether the weapon has a burst attack
	MakeCall(0x429E44, ai_pick_hit_mode_hack, 1);   // NoBurst

	LoadGameHook::OnGameReset() += ResetOnGameLoad;
}

}
