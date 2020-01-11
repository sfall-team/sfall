/*
 *    sfall
 *    Copyright (C) 2008-2020  The sfall team
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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "CombatBlock.h"

namespace sfall
{

static bool combatDisabled;
static std::string combatBlockedMessage;

static void __stdcall CombatBlocked() {
	fo::func::display_print(combatBlockedMessage.c_str());
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

void __stdcall SetBlockCombat(long toggle) {
	combatDisabled = toggle != 0;
}

void CombatBlockedInit() {
	HookCall(0x45F626, intface_use_item_hook); // jnz hook
	HookCall(0x4432A6, game_handle_input_hook);

	combatBlockedMessage = Translate("sfall", "BlockedCombat", "You cannot enter combat at this time.");
}

}
