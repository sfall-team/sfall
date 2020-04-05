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

#include "AI.h"

typedef std::tr1::unordered_map<TGameObj*, TGameObj*>::const_iterator iter;

static std::tr1::unordered_map<TGameObj*, TGameObj*> targets;
static std::tr1::unordered_map<TGameObj*, TGameObj*> sources;

static void __declspec(naked) ai_try_attack_hook_FleeFix() {
	__asm {
		or   byte ptr [esi + 0x3C], 8; // set new 'ReTarget' flag
		jmp  ai_run_away_;
	}
}

static void __declspec(naked) combat_ai_hook_FleeFix() {
	static const DWORD combat_ai_hook_flee_Ret = 0x42B22F;
	__asm {
		test byte ptr [ebp], 8; // 'ReTarget' flag (critter.combat_state)
		jnz  reTarget;
		jmp  critter_name_;
reTarget:
		and  byte ptr [ebp], ~(4 | 8); // unset Flee/ReTarget flags
		xor  edi, edi;
		mov  dword ptr [esi + 0x54], edi; // critter.who_hit_me
		add  esp, 4;
		jmp  combat_ai_hook_flee_Ret;
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
		cmp  [esi + 0x58], edx; // edx - minimum hp, below which NPC will run away
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

static DWORD RetryCombatMinAP;

static void __declspec(naked) RetryCombatHook() {
	static DWORD RetryCombatLastAP = 0;
	__asm {
		mov  RetryCombatLastAP, 0;
retry:
		call combat_ai_;
process:
		cmp  dword ptr ds:[_combat_turn_running], 0;
		jle  next;
		call process_bk_;
		jmp  process;
next:
		mov  eax, [esi + 0x40];
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

static long __fastcall sf_ai_weapon_reload(TGameObj* weapon, TGameObj* ammo, TGameObj* critter) {
	char* proto = nullptr;
	long result = -1;
	long maxAmmo;

	TGameObj* _ammo = ammo;

	while (ammo) {
		result = ItemWReload(weapon, ammo);
		if (result != 0) break; // 1 - reload done, -1 - can't reload

		if (!proto) {
			proto = GetProtoPtr(weapon->pid);
			maxAmmo = *(long*)(proto + 96); // item.weapon.maxAmmo
		}
		if (weapon->itemCharges >= maxAmmo) break; // magazine is full

		long pidAmmo = ammo->pid;
		ObjDestroy(ammo);
		ammo = nullptr;

		DWORD currentSlot = -1; // begin find at first slot
		while (TGameObj* ammoFind = InvenFindType(critter, item_type_ammo, &currentSlot)) {
			if (ammoFind->pid == pidAmmo) {
				ammo = ammoFind;
				break;
			}
		}
	}
	if (!result && _ammo != ammo) {
		ObjDestroy(ammo);
		return 1; // notifies the engine that the ammo has already been destroyed
	}
	return result;
}

static void __declspec(naked) item_w_reload_hook() {
	__asm {
		cmp  dword ptr [eax + 0x64], PID_SOLAR_SCORCHER;
		je   skip;
		push ecx;
		push esi;      // source
		mov  ecx, eax; // weapon
		call sf_ai_weapon_reload; // edx - ammo
		pop  ecx;
		retn;
skip:
		jmp  item_w_reload_;
	}
}

////////////////////////////////////////////////////////////////////////////////

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

void AIInit() {
	HookCall(0x426A95, combat_attack_hook);  // combat_attack_this_
	HookCall(0x42A796, combat_attack_hook);  // ai_attack_

	RetryCombatMinAP = GetConfigInt("Misc", "NPCsTryToSpendExtraAP", 0);
	if (RetryCombatMinAP > 0) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, RetryCombatHook); // combat_turn_
		dlogr(" Done", DL_INIT);
	}

	// Fix for NPCs not fully reloading a weapon if it has more ammo capacity than a box of ammo
	const DWORD itemWReloadAddr[] = {
		0x42AF15,           // cai_attempt_w_reload_
		0x42A970, 0x42AA56, // ai_try_attack_
	};
	for (int i = 0; i < sizeof(itemWReloadAddr) / 4; i++) {
		HookCall(itemWReloadAddr[i], item_w_reload_hook);
	}

	/////////////////////// Combat AI behavior fixes ///////////////////////

	// Fix to allow fleeing NPC to use drugs
	MakeCall(0x42B1DC, combat_ai_hack);
	// Fix for AI not checking minimum hp properly for using stimpaks (prevents premature fleeing)
	HookCall(0x428579, ai_check_drugs_hook);

	// Fix for NPC stuck in fleeing mode when the hit chance of a target was too low
	HookCall(0x42B1E3, combat_ai_hook_FleeFix);
	HookCall(0x42ABA8, ai_try_attack_hook_FleeFix);
	HookCall(0x42ACE5, ai_try_attack_hook_FleeFix);
	// Disable fleeing when NPC cannot move closer to target
	BlockCall(0x42ADF6); // ai_try_attack_

	// Fix for duplicate critters being added to the list of potential targets for AI
	MakeCall(0x428E75, ai_find_attackers_hack_target2, 2);
	MakeCall(0x428EB5, ai_find_attackers_hack_target3);
	MakeCall(0x428EE5, ai_find_attackers_hack_target4, 1);
}

TGameObj* __stdcall AIGetLastAttacker(TGameObj* target) {
	iter itr = sources.find(target);
	return (itr != sources.end()) ? itr->second : 0;
}

TGameObj* __stdcall AIGetLastTarget(TGameObj* source) {
	iter itr = targets.find(source);
	return (itr != targets.end()) ? itr->second : 0;
}

void __stdcall AICombatStart() {
	targets.clear();
	sources.clear();
}

void __stdcall AICombatEnd() {
	targets.clear();
	sources.clear();
}
