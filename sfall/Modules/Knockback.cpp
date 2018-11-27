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

static std::vector<fo::GameObject*> NoBursts;

struct KnockbackModifier {
	DWORD id;
	DWORD type;
	double value;
};

static std::vector<KnockbackModifier> mTargets;
static std::vector<KnockbackModifier> mAttackers;
static std::vector<KnockbackModifier> mWeapons;

struct ChanceModifier {
	DWORD id;
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

static double ApplyModifiers(std::vector<KnockbackModifier>* mods, DWORD id, double val) {
	for (DWORD i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == id) {
			KnockbackModifier* mod = &(*mods)[i];
			switch (mod->type) {
			case 0: val = mod->value; break;
			case 1: val *= mod->value; break;
			}
			break;
		}
	}
	return val;
}

static DWORD _stdcall CalcKnockback(int flags, int damage, DWORD target, DWORD attacker, DWORD weapon) {
	double result = (double)damage / (flags == 0x3d ? 5.0 : 10.0);
	result = ApplyModifiers(&mWeapons, weapon, result);
	result = ApplyModifiers(&mAttackers, attacker, result);
	result = ApplyModifiers(&mTargets, target, result);
	return (DWORD)floor(result);
}

static const DWORD KnockbackRetAddr=0x424B85;
static void __declspec(naked) KnockbackHook() {
	__asm {
		mov ecx, [esp+0x14];
		push ebx;
		mov ebx, [esi+8];
		push ebx;	//Weapon
		mov ebx, [esi];
		push ebx;	//Attacker
		push ecx;	//target
		mov edi, [edi];
		push edi;	//Damage
		push eax;	//Weapon flags
		call CalcKnockback;
		pop ebx;
		mov ecx, [esp+0x10];
		mov [ecx], eax;
		jmp KnockbackRetAddr;
	}
}

static const DWORD KnockbackRetAddr2=0x4136E3;
static void __declspec(naked) KnockbackHook2() {
	__asm {
		push ecx
		push -1;	//Weapon
		push -1;	//Attacker
		push esi;	//target
		push ebx;	//Damage
		push 0;	  //Weapon flags
		call CalcKnockback;
		pop ecx
		mov [ecx], eax;
		jmp KnockbackRetAddr2;
	}
}

static const DWORD KnockbackAddr = (DWORD)&KnockbackHook;
static int _stdcall PickpocketHook2(int base, DWORD critter) {
	for (DWORD i = 0; i < pickpocketMods.size(); i++) {
		if (critter == pickpocketMods[i].id) {
			return min(base + pickpocketMods[i].mod, pickpocketMods[i].maximum);
		}
	}
	return min(base + basePickpocket.mod, basePickpocket.maximum);
}

static void __declspec(naked) PickpocketHook() {
	__asm {
		push edx;
		push ecx;
		push ebx;
		push esi;
		push eax;
		call PickpocketHook2;
		pop  ebx;
		pop  ecx;
		pop  edx;
		mov  [esp + 84], eax;
		push 0x4ABC6F;
		retn;
	}
}

static int _stdcall HitChanceHook2(int base, DWORD critter) {
	for (DWORD i = 0; i < hitChanceMods.size(); i++) {
		if (critter == hitChanceMods[i].id) {
			return min(base + hitChanceMods[i].mod, hitChanceMods[i].maximum);
		}
	}
	return min(base + baseHitChance.mod, baseHitChance.maximum);
}

static void __declspec(naked) HitChanceHook() {
	__asm {
		push edi;
		push esi;
		call HitChanceHook2;
		mov  esi, eax;
		push 0x42479B;
		retn;
	}
}

static DWORD BurstTestResult;
static const DWORD BurstHookRet = 0x429E4A;
static void _stdcall BurstTest(fo::GameObject* critter) {
	BurstTestResult = 0;
	for (size_t i = 0; i < NoBursts.size(); i++) {
		if (NoBursts[i] == critter) {
			BurstTestResult = 1;
			return;
		}
	}
}

static void __declspec(naked) BurstHook() {
	__asm {
		pushad;
		push esi;
		call BurstTest;
		popad;
		mov ebx, BurstTestResult;
		test ebx, ebx;
		jz fail;
		mov ebx, 10;
		jmp BurstHookRet;
fail:
		mov ebx, [eax+0x94];
		jmp BurstHookRet;
	}
}

void Knockback_OnGameLoad() {
	mTargets.clear();
	mAttackers.clear();
	mWeapons.clear();
	hitChanceMods.clear();
	baseHitChance.maximum = 95;
	baseHitChance.mod = 0;
	pickpocketMods.clear();
	basePickpocket.maximum = 95;
	basePickpocket.mod = 0;
	NoBursts.clear();
	disabledAS.clear();
	forcedAS.clear();
}

void _stdcall KnockbackSetMod(DWORD id, DWORD type, float val, DWORD on) {
	std::vector<KnockbackModifier>* mods;
	switch (on) {
	case 0: mods = &mWeapons; break;
	case 1: mods = &mTargets; break;
	case 2: mods = &mAttackers; break;
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

void _stdcall KnockbackRemoveMod(DWORD id, DWORD on) {
	std::vector<KnockbackModifier>* mods;
	switch (on) {
	case 0: mods = &mWeapons; break;
	case 1: mods = &mTargets; break;
	case 2: mods = &mAttackers; break;
	default: return;
	}
	for (DWORD i = 0; i < mods->size(); i++) {
		if ((*mods)[i].id == id) {
			mods->erase(mods->begin() + i);
			return;
		}
	}
}

void _stdcall SetHitChanceMax(DWORD critter, DWORD maximum, DWORD mod) {
	if (critter == -1) {
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

void _stdcall SetPickpocketMax(DWORD critter, DWORD maximum, DWORD mod) {
	if (critter == -1) {
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
	if (on) {
		for (size_t i = 0; i < NoBursts.size(); i++) {
			if (NoBursts[i] == critter) return;
		}
		NoBursts.push_back(critter);
	} else {
		for (size_t i = 0; i < NoBursts.size(); i++) {
			if (NoBursts[i] == critter) {
				NoBursts.erase(NoBursts.begin() + i);
				return;
			}
		}
	}
}

static int _stdcall AimedShotTest(DWORD pid) {
	for (DWORD i = 0; i < disabledAS.size(); i++) if (disabledAS[i] == pid) return -1;
	for (DWORD i = 0; i < forcedAS.size(); i++) if (forcedAS[i] == pid) return 1;
	return 0;
}

static const DWORD aimedShotRet1 = 0x478EE4;
static const DWORD aimedShotRet2 = 0x478EEA;
static void __declspec(naked) AimedShotHook() {
	__asm {
		push eax;
		push edx;
		test edx, edx;
		jz skippid;
		mov edx, [edx+0x64];
skippid:
		push edx;
		call AimedShotTest;
		test eax, eax;
		jz realfunc;
		jl disable;
		add esp, 12;
		jmp aimedShotRet2;
disable:
		add esp, 12;
		jmp aimedShotRet1;
realfunc:
		pop edx;
		pop eax;
		jmp fo::funcoffs::item_w_damage_type_;
	}
}

static void HookAimedShots() {
	HookCall(0x478EC6, &AimedShotHook);
	hookedAimedShot = true;
}

void _stdcall DisableAimedShots(DWORD pid) {
	if (!hookedAimedShot) HookAimedShots();
	for (DWORD i = 0; i < forcedAS.size(); i++) if (forcedAS[i] == pid) forcedAS.erase(forcedAS.begin() + (i--));
	for (DWORD i = 0; i < disabledAS.size(); i++) if (disabledAS[i] == pid) return;
	disabledAS.push_back(pid);
}

void _stdcall ForceAimedShots(DWORD pid) {
	if (!hookedAimedShot) HookAimedShots();
	for (DWORD i = 0; i < disabledAS.size(); i++) if (disabledAS[i] == pid) disabledAS.erase(disabledAS.begin() + (i--));
	for (DWORD i = 0; i < forcedAS.size(); i++) if (forcedAS[i] == pid) return;
	forcedAS.push_back(pid);
}

void Knockback::init() {
	SafeWrite16(0x424B61, 0x25ff);
	SafeWrite32(0x424B63, (DWORD)&KnockbackAddr);
	MakeJump(0x4136D3, KnockbackHook2); // for op_critter_dmg
	MakeJump(0x424791, HitChanceHook);
	MakeJump(0x4ABC62, PickpocketHook);
	MakeJump(0x429E44, BurstHook);

	LoadGameHook::OnGameReset() += Knockback_OnGameLoad;
}

}
