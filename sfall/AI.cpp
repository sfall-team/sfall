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

#include <hash_map>

#include "main.h"

#include "AI.h"
#include "FalloutEngine.h"
#include "SafeWrite.h"

typedef stdext::hash_map<DWORD, DWORD>::const_iterator iter;

static stdext::hash_map<DWORD, DWORD> targets;
static stdext::hash_map<DWORD, DWORD> sources;

static DWORD RetryCombatLastAP;
static DWORD RetryCombatMinAP;
static void __declspec(naked) RetryCombatHook() {
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
		jmp  combat_attack_;
	}
}

static DWORD combatDisabled;
void _stdcall AIBlockCombat(DWORD i) {
	combatDisabled = i ? 1 : 0;
}

static char combatBlockedMessage[128];
static void _stdcall CombatBlocked() {
	DisplayConsoleMessage(combatBlockedMessage);
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
		mov  eax, dword ptr ds:[_intfaceEnabled];
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

void AIInit() {
	//HookCall(0x42AE1D, ai_attack_hook);
	//HookCall(0x42AE5C, ai_attack_hook);
	HookCall(0x426A95, combat_attack_hook);  // combat_attack_this_
	HookCall(0x42A796, combat_attack_hook);  // ai_attack_

	MakeJump(0x45F6AF, BlockCombatHook1);    // intface_use_item_
	HookCall(0x4432A6, BlockCombatHook2);    // game_handle_input_
	GetPrivateProfileString("sfall", "BlockedCombat", "You cannot enter combat at this time.", combatBlockedMessage, 128, translationIni);

	RetryCombatMinAP = GetPrivateProfileIntA("Misc", "NPCsTryToSpendExtraAP", 0, ini);
	if (RetryCombatMinAP > 0) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, RetryCombatHook); // combat_turn_
		dlogr(" Done", DL_INIT);
	}
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
