/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

#include "..\..\..\FalloutEngine\AsmMacros.h"
#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\AI.h"
#include "..\..\Combat.h"
#include "..\..\KillCounter.h"

#include "Combat.h"

namespace sfall
{
namespace script
{

// Kill counters
static bool extraKillCounter;

void SetExtraKillCounter(bool value) { extraKillCounter = value; }

void __declspec(naked) op_get_kill_counter() {
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

void __declspec(naked) op_mod_kill_counter() {
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

void op_set_object_knockback(OpcodeContext& ctx) {
	int mode = 0;
	switch (ctx.opcode()) {
	case 0x196:
		mode = 1;
		break;
	case 0x197:
		mode = 2;
		break;
	}
	fo::GameObject* object = ctx.arg(0).object();
	if (mode) {
		if (object->IsNotCritter()) {
			ctx.printOpcodeError("%s() - the object is not a critter.", ctx.getOpcodeName());
			return;
		}
	} else {
		if (object->IsNotItem()) {
			ctx.printOpcodeError("%s() - the object is not an item.", ctx.getOpcodeName());
			return;
		}
	}
	KnockbackSetMod(object, ctx.arg(1).rawValue(), ctx.arg(2).asFloat(), mode);
}

void op_remove_object_knockback(OpcodeContext& ctx) {
	int mode = 0;
	switch (ctx.opcode()) {
	case 0x199:
		mode = 1;
		break;
	case 0x19a:
		mode = 2;
		break;
	}
	KnockbackRemoveMod(ctx.arg(0).object(), mode);
}

void __declspec(naked) op_get_bodypart_hit_modifier() {
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

void __declspec(naked) op_set_bodypart_hit_modifier() {
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

void op_get_attack_type(OpcodeContext& ctx) {
	ctx.setReturn(fo::util::GetCurrentAttackMode());
}

void __declspec(naked) op_force_aimed_shots() {
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

void __declspec(naked) op_disable_aimed_shots() {
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

void __declspec(naked) op_get_last_attacker() {
	__asm {
		_GET_ARG_INT(fail);
		mov  esi, ecx;
		push eax;
		call AI::AIGetLastAttacker;
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

void __declspec(naked) op_get_last_target() {
	__asm {
		_GET_ARG_INT(fail);
		mov  esi, ecx;
		push eax;
		call AI::AIGetLastTarget;
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

void __declspec(naked) op_block_combat() {
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

void mf_attack_is_aimed(OpcodeContext& ctx) {
	DWORD isAimed, unused;
	ctx.setReturn(!fo::func::intface_get_attack(&unused, &isAimed) ? isAimed : 0);
}

void mf_combat_data(OpcodeContext& ctx) {
	fo::ComputeAttackResult* ctd = nullptr;
	if (*fo::ptr::combat_state & 1) {
		ctd = fo::ptr::main_ctd;
	}
	ctx.setReturn((DWORD)ctd, DATATYPE_INT);
}

}
}
