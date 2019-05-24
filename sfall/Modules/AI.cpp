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

#include "..\main.h"

#include "AI.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\SafeWrite.h"

namespace sfall
{
using namespace fo;
using namespace Fields;

typedef std::unordered_map<DWORD, DWORD>::const_iterator iter;

static std::unordered_map<DWORD, DWORD> targets;
static std::unordered_map<DWORD, DWORD> sources;

static void __declspec(naked) ai_try_attack_hook_FleeFix() {
	__asm {
		or   byte ptr [esi + combatState], 8; // set new 'ReTarget' flag
		jmp  fo::funcoffs::ai_run_away_;
	}
}

static const DWORD combat_ai_hook_flee_Ret = 0x42B22F;
static void __declspec(naked) combat_ai_hook_FleeFix() {
	__asm {
		test byte ptr [ebp], 8; // 'ReTarget' flag (critter.combat_state)
		jnz  reTarget;
		jmp  fo::funcoffs::critter_name_;
reTarget:
		and  byte ptr [ebp], ~(4 | 8); // unset Flee/ReTarget flags
		xor  edi, edi;
		mov  dword ptr [esi + whoHitMe], edi;
		add  esp, 4;
		jmp  combat_ai_hook_flee_Ret;
	}
}

static const DWORD combat_ai_hack_Ret = 0x42B204;
static void __declspec(naked) combat_ai_hack() {
	__asm {
		cmp  eax, [ebx + 0x10]; // cap.min_hp - minimum hp, below which NPC will run away
		jl   tryHeal; // curr_hp < min_hp
end:
		add  esp, 4;
		jmp  combat_ai_hack_Ret;
tryHeal:
		mov  eax, esi;
		call fo::funcoffs::ai_check_drugs_;
		mov  eax, esi;
		mov  edx, STAT_current_hp;
		call fo::funcoffs::stat_level_;
		cmp  eax, [ebx + 0x10]; // cap.min_hp
		jge  end;
		retn; // flee
	}
}

////////////////////////////////////////////////////////////////////////////////

static DWORD RetryCombatLastAP;
static DWORD RetryCombatMinAP;
static void __declspec(naked) RetryCombatHook() {
	__asm {
		mov  RetryCombatLastAP, 0;
retry:
		call fo::funcoffs::combat_ai_;
process:
		cmp  dword ptr ds:[FO_VAR_combat_turn_running], 0;
		jle  next;
		call fo::funcoffs::process_bk_;
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

////////////////////////////////////////////////////////////////////////////////

static void __fastcall CombatAttackHook(DWORD source, DWORD target) {
	sources[target] = source;
	targets[source] = target;
}

static void __declspec(naked) combat_attack_hook() {
	_asm {
		push ecx;
		push edx;
		push eax;
		mov  ecx, eax;         // source
		call CombatAttackHook; // edx - target
		pop  eax;
		pop  edx;
		pop  ecx;
		jmp  fo::funcoffs::combat_attack_;
	}
}

static DWORD combatDisabled;
void _stdcall AIBlockCombat(DWORD i) {
	combatDisabled = i ? 1 : 0;
}

static std::string combatBlockedMessage;
static void _stdcall CombatBlocked() {
	fo::func::display_print(combatBlockedMessage.c_str());
}

static const DWORD BlockCombatHook1Ret1 = 0x45F6B4;
static const DWORD BlockCombatHook1Ret2 = 0x45F6D7;
static void __declspec(naked) BlockCombatHook1() {
	__asm {
		mov  eax, combatDisabled;
		test eax, eax;
		jz   end;
		call CombatBlocked;
		jmp  BlockCombatHook1Ret2;
end:
		mov  eax, 0x14;
		jmp  BlockCombatHook1Ret1;
	}
}

static void __declspec(naked) BlockCombatHook2() {
	__asm {
		mov  eax, dword ptr ds:[FO_VAR_intfaceEnabled];
		test eax, eax;
		jz   end;
		mov  eax, combatDisabled;
		test eax, eax;
		jz   succeed;
		push ecx;
		push edx;
		call CombatBlocked;
		pop  edx;
		pop  ecx;
		xor  eax, eax;
		retn;
succeed:
		inc  eax;
end:
		retn;
	}
}

void AI::init() {
	//HookCall(0x42AE1D, ai_attack_hook);
	//HookCall(0x42AE5C, ai_attack_hook);
	HookCall(0x426A95, combat_attack_hook);  // combat_attack_this_
	HookCall(0x42A796, combat_attack_hook);  // ai_attack_

	MakeJump(0x45F6AF, BlockCombatHook1);    // intface_use_item_
	HookCall(0x4432A6, BlockCombatHook2);    // game_handle_input_
	combatBlockedMessage = Translate("sfall", "BlockedCombat", "You cannot enter combat at this time.");

	RetryCombatMinAP = GetConfigInt("Misc", "NPCsTryToSpendExtraAP", 0);
	if (RetryCombatMinAP > 0) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, RetryCombatHook); // combat_turn_
		dlogr(" Done", DL_INIT);
	}

	/////////////////////// Combat AI behavior fixes ///////////////////////

	// Fix to allow fleeing NPC to use drugs
	MakeCall(0x42B1DC, combat_ai_hack);
	// Fix for NPC stuck in fleeing mode when the hit chance of a target was too low
	HookCall(0x42B1E3, combat_ai_hook_FleeFix);
	HookCalls(ai_try_attack_hook_FleeFix, {0x42ABA8, 0x42ACE5});
	// Disable fleeing when NPC cannot move closer to target
	BlockCall(0x42ADF6); // ai_try_attack_
}

DWORD _stdcall AIGetLastAttacker(DWORD target) {
	iter itr = sources.find(target);
	return (itr != sources.end()) ? itr->second: 0;
}

DWORD _stdcall AIGetLastTarget(DWORD source) {
	iter itr = targets.find(source);
	return (itr != targets.end()) ? itr->second : 0;
}

void _stdcall AICombatStart() {
	targets.clear();
	sources.clear();
}

void _stdcall AICombatEnd() {
	targets.clear();
	sources.clear();
}

}
