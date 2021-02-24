/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "Perks.h"

static void __declspec(naked) op_get_perk_owed() {
	__asm {
		movzx edx, byte ptr ds:[FO_VAR_free_perk];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_set_perk_owed() {
	__asm {
		_GET_ARG_INT(end);
		and  eax, 0xFF;
		cmp  eax, 250;
		jg   end;
		mov  byte ptr ds:[FO_VAR_free_perk], al;
end:
		retn;
	}
}

static void __declspec(naked) op_set_perk_freq() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call SetPerkFreq;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __stdcall op_get_perk_available2() {
	const ScriptValue &perkIdArg = opHandler.arg(0);

	int result = 0;
	if (perkIdArg.isInt()) {
		int perkId = perkIdArg.rawValue();
		if (perkId >= 0 && perkId < PERK_count) {
			result = fo_perk_can_add(*ptr_obj_dude, perkId);
		}
	} else {
		OpcodeInvalidArgs("get_perk_available");
	}
	opHandler.setReturn(result);
}

static void __declspec(naked) op_get_perk_available() {
	_WRAP_OPCODE(op_get_perk_available2, 1, 1)
}

static void __stdcall op_set_perk_name2() {
	const ScriptValue &perkIdArg = opHandler.arg(0),
					  &stringArg = opHandler.arg(1);

	if (perkIdArg.isInt() && stringArg.isString()) {
		SetPerkName(perkIdArg.rawValue(), stringArg.strValue());
	} else {
		OpcodeInvalidArgs("set_perk_name");
	}
}

static void __declspec(naked) op_set_perk_name() {
	_WRAP_OPCODE(op_set_perk_name2, 2, 0)
}

static void __stdcall op_set_perk_desc2() {
	const ScriptValue &perkIdArg = opHandler.arg(0),
					  &stringArg = opHandler.arg(1);

	if (perkIdArg.isInt() && stringArg.isString()) {
		SetPerkDesc(perkIdArg.rawValue(), stringArg.strValue());
	} else {
		OpcodeInvalidArgs("set_perk_desc");
	}
}

static void __declspec(naked) op_set_perk_desc() {
	_WRAP_OPCODE(op_set_perk_desc2, 2, 0)
}

static void __declspec(naked) op_set_perk_value() {
	__asm { // edx - opcode
		push edi;
		push ecx;
		sub  edx, 0x5E0 - 8; // offset of value into perk struct: edx = ((edx/4) - 0x178 + 0x8) * 4
		mov  edi, edx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;      // value
		mov  edx, edi; // param (offset)
		mov  ecx, eax; // perk id
		call SetPerkValue;
end:
		pop  ecx;
		pop  edi;
		retn;
	}
}

static void __stdcall op_set_selectable_perk2() {
	const ScriptValue &nameArg = opHandler.arg(0),
					  &activeArg = opHandler.arg(1),
					  &imageArg = opHandler.arg(2),
					  &descArg = opHandler.arg(3);

	if (nameArg.isString() && activeArg.isInt() && imageArg.isInt() && descArg.isString()) {
		SetSelectablePerk(nameArg.strValue(), activeArg.rawValue(), imageArg.rawValue(), descArg.strValue());
	} else {
		OpcodeInvalidArgs("set_selectable_perk");
	}
}

static void __declspec(naked) op_set_selectable_perk() {
	_WRAP_OPCODE(op_set_selectable_perk2, 4, 0)
}

static void __stdcall op_set_fake_perk2() {
	const ScriptValue &nameArg = opHandler.arg(0),
					  &levelArg = opHandler.arg(1),
					  &imageArg = opHandler.arg(2),
					  &descArg = opHandler.arg(3);

	if (nameArg.isString() && levelArg.isInt() && imageArg.isInt() && descArg.isString()) {
		SetFakePerk(nameArg.strValue(), levelArg.rawValue(), imageArg.rawValue(), descArg.strValue());
	} else {
		OpcodeInvalidArgs("set_fake_perk");
	}
}

static void __declspec(naked) op_set_fake_perk() {
	_WRAP_OPCODE(op_set_fake_perk2, 4, 0)
}

static void __stdcall op_set_fake_trait2() {
	const ScriptValue &nameArg = opHandler.arg(0),
					  &activeArg = opHandler.arg(1),
					  &imageArg = opHandler.arg(2),
					  &descArg = opHandler.arg(3);

	if (nameArg.isString() && activeArg.isInt() && imageArg.isInt() && descArg.isString()) {
		SetFakeTrait(nameArg.strValue(), activeArg.rawValue(), imageArg.rawValue(), descArg.strValue());
	} else {
		OpcodeInvalidArgs("set_fake_trait");
	}
}

static void __declspec(naked) op_set_fake_trait() {
	_WRAP_OPCODE(op_set_fake_trait2, 4, 0)
}

static void __declspec(naked) op_set_perkbox_title() {
	__asm {
		mov  esi, ecx;
		push ebx;
		mov  ecx, eax; // keep script
		_GET_ARG(ebx, edx);
		cmp  dx, VAR_TYPE_STR2;
		jz   next;
		cmp  dx, VAR_TYPE_STR;
		jnz  end;
next:
		mov  eax, ecx; // script
		call interpretGetString_;
		mov  ecx, eax;
		call SetPerkboxTitle;
end:
		mov  ecx, esi;
		pop  ebx;
		retn;
	}
}

static void __declspec(naked) op_hide_real_perks() {
	__asm {
		mov  esi, ecx;
		call IgnoreDefaultPerks;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_show_real_perks() {
	__asm {
		mov  esi, ecx;
		call RestoreDefaultPerks;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_clear_selectable_perks() {
	__asm {
		mov  esi, ecx;
		call ClearSelectablePerks;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_has_fake_perk() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		call HasFakePerk;
end:
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_has_fake_trait() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		call HasFakeTrait;
end:
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_perk_add_mode() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call AddPerkMode;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_remove_trait() {
	__asm {
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		mov  edx, -1;
		cmp  eax, ds:[FO_VAR_pc_trait];
		jne  next;
		mov  esi, ds:[FO_VAR_pc_trait2];
		mov  ds:[FO_VAR_pc_trait], esi;
		mov  ds:[FO_VAR_pc_trait2], edx;
		retn;
next:
		cmp  eax, ds:[FO_VAR_pc_trait2];
		jne  end;
		mov  ds:[FO_VAR_pc_trait2], edx;
end:
		retn;
	}
}

static void __declspec(naked) op_set_pyromaniac_mod() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		push 0x424AB6;
		call SafeWrite8;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_apply_heaveho_fix() {
	__asm {
		mov  esi, ecx;
		call ApplyHeaveHoFix;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_swiftlearner_mod() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		push 0x4AFAE2;
		call SafeWrite32;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) perk_can_add_hook() {
	__asm {
		call stat_pc_get_;
		add  eax, PerkLevelMod;
		js   jneg; // level < 0
		retn;
jneg:
		xor  eax, eax;
		retn;
	}
}

static void __fastcall SetPerkLevelMod(long mod) {
	static bool perkLevelModPatch = false;
	if (mod < -25 || mod > 25) return;
	PerkLevelMod = mod;

	if (perkLevelModPatch) return;
	perkLevelModPatch = true;
	HookCall(0x49687F, perk_can_add_hook);
}

static void __declspec(naked) op_set_perk_level_mod() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, eax;
		call SetPerkLevelMod;
end:
		mov  ecx, esi;
		retn;
	}
}

static void mf_add_trait() {
	if ((*ptr_obj_dude)->protoId != PID_Player) {
		opHandler.printOpcodeError("add_trait() - traits can be added only to the player.");
		opHandler.setReturn(-1);
		return;
	}
	long traitId = opHandler.arg(0).rawValue();
	if (traitId >= TRAIT_fast_metabolism && traitId <= TRAIT_gifted) {
		if (ptr_pc_trait[0] == -1) {
			ptr_pc_trait[0] = traitId;
		} else if (ptr_pc_trait[0] != traitId && ptr_pc_trait[1] == -1) {
			ptr_pc_trait[1] = traitId;
		} else {
			opHandler.printOpcodeError("add_trait() - cannot add the trait ID: %d", traitId);
		}
	} else {
		opHandler.printOpcodeError("add_trait() - invalid trait ID.");
	}
}
