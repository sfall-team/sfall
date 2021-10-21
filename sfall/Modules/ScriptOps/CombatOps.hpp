/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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

#pragma once

#include "AI.h"
#include "Combat.h"
#include "KillCounter.h"

namespace sfall
{

// Kill counters
static bool extraKillCounter;

static void SetExtraKillCounter(bool value) { extraKillCounter = value; }

static void __declspec(naked) op_get_kill_counter() {
	__asm {
		_GET_ARG_INT(fail); // get kill type value
		cmp  extraKillCounter, 1;
		jne  skip;
		cmp  eax, 38;
		jae  fail;
		movzx edx, word ptr ds:[FO_VAR_pc_kill_counts][eax * 2];
		jmp  end;
skip:
		cmp  eax, 19;
		jae  fail;
		mov  edx, ds:[FO_VAR_pc_kill_counts][eax * 4];
end:
		mov  eax, ebx; // script
		_RET_VAL_INT;
		retn;
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_mod_kill_counter() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi); // get mod value
		mov  eax, ebx;
		_GET_ARG_INT(end);  // get kill type value
		cmp  si, VAR_TYPE_INT;
		jnz  end;
		cmp  extraKillCounter, 1;
		je   skip;
		cmp  eax, 19;
		jae  end;
		add  ds:[FO_VAR_pc_kill_counts][eax * 4], ecx;
		pop  ecx;
		retn;
skip:
		cmp  eax, 38;
		jae  end;
		add  word ptr ds:[FO_VAR_pc_kill_counts][eax * 2], cx;
end:
		pop  ecx;
		retn;
	}
}

//Knockback
static void __declspec(naked) SetKnockback() {
	__asm {
		sub esp, 0xC;
		mov ecx, eax;
		//Get args
		call fo::funcoffs::interpretPopShort_; //First arg type
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;  //First arg
		mov [esp + 8], eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_; //Second arg type
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;  //Second arg
		mov [esp + 4], eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_; //Third arg type
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;  //Third arg
		mov [esp], eax;
		//Error check
		cmp di, VAR_TYPE_FLOAT;
		jz paramWasFloat;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		fild [esp + 8];
		fstp [esp + 8];
paramWasFloat:
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp si, VAR_TYPE_INT;
		jnz fail;
		call KnockbackSetMod;
		jmp end;
fail:
		add esp, 0x10;
end:
		popaop;
		retn;
	}
}

static void __declspec(naked) op_set_weapon_knockback() {
	__asm {
		pushaop;
		push 0;
		jmp SetKnockback;
	}
}

static void __declspec(naked) op_set_target_knockback() {
	__asm {
		pushaop;
		push 1;
		jmp SetKnockback;
	}
}

static void __declspec(naked) op_set_attacker_knockback() {
	__asm {
		pushaop;
		push 2;
		jmp SetKnockback;
	}
}

static void __declspec(naked) RemoveKnockback() {
	__asm {
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call KnockbackRemoveMod;
		jmp end;
fail:
		add esp, 4;
end:
		popaop;
		retn;
	}
}

static void __declspec(naked) op_remove_weapon_knockback() {
	__asm {
		pushaop;
		push 0;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_remove_target_knockback() {
	__asm {
		pushaop;
		push 1;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_remove_attacker_knockback() {
	__asm {
		pushaop;
		push 2;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_get_bodypart_hit_modifier() {
	__asm {
		_GET_ARG_INT(fail); // get body value
		cmp  eax, 8; // Body_Head - Body_Uncalled
		ja   fail;
		mov  edx, ds:[FO_VAR_hit_location_penalty][eax * 4];
end:
		mov  eax, ebx; // script
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_set_bodypart_hit_modifier() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi); // get body value
		mov  eax, ebx;
		_GET_ARG_INT(end);  // get modifier value
		cmp  si, VAR_TYPE_INT;
		jnz  end;
		cmp  eax, 8; // Body_Head - Body_Uncalled
		ja   end;
		mov  ds:[FO_VAR_hit_location_penalty][eax * 4], ecx;
end:
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) op_get_attack_type() {
	__asm {
		mov  esi, ecx;
		call fo::util::GetCurrentAttackMode;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_force_aimed_shots() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call ForceAimedShots;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_disable_aimed_shots() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call DisableAimedShots;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_get_last_attacker() {
	__asm {
		_GET_ARG_INT(fail);
		mov  esi, ecx;
		push eax;
		call AIGetLastAttacker;
		mov  edx, eax;
		mov  ecx, esi;
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_get_last_target() {
	__asm {
		_GET_ARG_INT(fail);
		mov  esi, ecx;
		push eax;
		call AIGetLastTarget;
		mov  edx, eax;
		mov  ecx, esi;
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_block_combat() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call SetBlockCombat;
end:
		mov  ecx, esi;
		retn;
	}
}

static void mf_attack_is_aimed() {
	DWORD isAimed, unused;
	opHandler.setReturn(!fo::func::intface_get_attack(&unused, &isAimed) ? isAimed : 0);
}

static void mf_combat_data() {
	fo::ComputeAttackResult* ctd = nullptr;
	if (*fo::ptr::combat_state & 1) {
		ctd = fo::ptr::main_ctd;
	}
	opHandler.setReturn((DWORD)ctd, DATATYPE_INT);
}

}
