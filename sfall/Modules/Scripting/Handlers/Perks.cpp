/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\..\Perks.h"
#include "..\..\ScriptExtender.h"
#include "..\OpcodeContext.h"

#include "Perks.h"

namespace sfall
{
namespace script
{

__declspec(naked) void op_get_perk_owed() {
	__asm {
		movzx edx, byte ptr ds:[FO_VAR_free_perk];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

__declspec(naked) void op_set_perk_owed() {
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

__declspec(naked) void op_set_perk_freq() {
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

void op_get_perk_available(OpcodeContext& ctx) {
	int result = 0, perkId = ctx.arg(0).rawValue();
	if (perkId >= 0 && perkId < 256) { // start fake id
		result = fo::func::perk_can_add(fo::var::obj_dude, perkId);
	}
	ctx.setReturn(result);
}

void op_set_perk_name(OpcodeContext& ctx) {
	Perks::SetPerkName(ctx.arg(0).rawValue(), ctx.arg(1).strValue());
}

void op_set_perk_desc(OpcodeContext& ctx) {
	Perks::SetPerkDesc(ctx.arg(0).rawValue(), ctx.arg(1).strValue());
}

__declspec(naked) void op_set_perk_value() {
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
		call Perks::SetPerkValue;
end:
		pop  ecx;
		pop  edi;
		retn;
	}
}

void op_set_selectable_perk(OpcodeContext& ctx) {
	Perks::SetSelectablePerk(ctx.arg(0).strValue(), ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), ctx.arg(3).strValue());
}

void op_set_fake_perk(OpcodeContext& ctx) {
	Perks::SetFakePerk(ctx.arg(0).strValue(), ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), ctx.arg(3).strValue());
}

void op_set_fake_trait(OpcodeContext& ctx) {
	Perks::SetFakeTrait(ctx.arg(0).strValue(), ctx.arg(1).rawValue(), ctx.arg(2).rawValue(), ctx.arg(3).strValue());
}

static const char* notPartyMemberErr = "%s() - the object is not a party member.";

void mf_set_selectable_perk_npc(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	if (obj->IsCritter() && fo::func::isPartyMember(obj)) {
		Perks::SetSelectablePerk(ctx.arg(1).strValue(), ctx.arg(2).rawValue(), ctx.arg(3).rawValue(), ctx.arg(4).strValue(), (obj->id != fo::PLAYER_ID) ? obj->id : 0);
	} else {
		ctx.printOpcodeError(notPartyMemberErr, ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_set_fake_perk_npc(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	if (obj->IsCritter() && fo::func::isPartyMember(obj)) {
		Perks::SetFakePerk(ctx.arg(1).strValue(), ctx.arg(2).rawValue(), ctx.arg(3).rawValue(), ctx.arg(4).strValue(), (obj->id != fo::PLAYER_ID) ? obj->id : 0);
	} else {
		ctx.printOpcodeError(notPartyMemberErr, ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_set_fake_trait_npc(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	if (obj->IsCritter() && fo::func::isPartyMember(obj)) {
		Perks::SetFakeTrait(ctx.arg(1).strValue(), ctx.arg(2).rawValue(), ctx.arg(3).rawValue(), ctx.arg(4).strValue(), (obj->id != fo::PLAYER_ID) ? obj->id : 0);
	} else {
		ctx.printOpcodeError(notPartyMemberErr, ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

__declspec(naked) void op_set_perkbox_title() {
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
		call fo::funcoffs::interpretGetString_;
		mov  ecx, eax;
		call Perks::SetPerkboxTitle;
end:
		mov  ecx, esi;
		pop  ebx;
		retn;
	}
}

__declspec(naked) void op_hide_real_perks() {
	__asm {
		mov  esi, ecx;
		call IgnoreDefaultPerks;
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_show_real_perks() {
	__asm {
		mov  esi, ecx;
		call RestoreDefaultPerks;
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_clear_selectable_perks() {
	__asm {
		mov  esi, ecx;
		call ClearSelectablePerks;
		mov  ecx, esi;
		retn;
	}
}

void op_has_fake_perk(OpcodeContext& ctx) {
	ctx.setReturn(Perks::HasFakePerk(ctx.arg(0).asString(), ctx.arg(0).asInt()));
}

void op_has_fake_trait(OpcodeContext& ctx) {
	ctx.setReturn(Perks::HasFakeTrait(ctx.arg(0).strValue()));
}

void mf_has_fake_perk_npc(OpcodeContext& ctx) {
	long result = 0;
	auto obj = ctx.arg(0).object();
	if (obj->IsCritter() && fo::func::isPartyMember(obj)) {
		result = Perks::HasFakePerkOwner(ctx.arg(1).strValue(), (obj->id != fo::PLAYER_ID) ? obj->id : 0);
	} else {
		ctx.printOpcodeError(notPartyMemberErr, ctx.getMetaruleName());
	}
	ctx.setReturn(result);
}

void mf_has_fake_trait_npc(OpcodeContext& ctx) {
	long result = 0;
	auto obj = ctx.arg(0).object();
	if (obj->IsCritter() && fo::func::isPartyMember(obj)) {
		result = Perks::HasFakeTraitOwner(ctx.arg(1).strValue(), (obj->id != fo::PLAYER_ID) ? obj->id : 0);
	} else {
		ctx.printOpcodeError(notPartyMemberErr, ctx.getMetaruleName());
	}
	ctx.setReturn(result);
}

__declspec(naked) void op_perk_add_mode() {
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

__declspec(naked) void op_remove_trait() {
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

__declspec(naked) void op_set_pyromaniac_mod() {
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

__declspec(naked) void op_apply_heaveho_fix() {
	__asm {
		mov  esi, ecx;
		call Perks::ApplyHeaveHoFix;
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_set_swiftlearner_mod() {
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

static __declspec(naked) void perk_can_add_hook() {
	__asm {
		call fo::funcoffs::stat_pc_get_;
		add  eax, Perks::PerkLevelMod;
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
	Perks::PerkLevelMod = mod;

	if (perkLevelModPatch) return;
	perkLevelModPatch = true;
	HookCall(0x49687F, perk_can_add_hook);
}

__declspec(naked) void op_set_perk_level_mod() {
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

void mf_add_trait(OpcodeContext& ctx) {
	if (fo::var::obj_dude->protoId != fo::PID_Player) {
		ctx.printOpcodeError("%s() - traits can be added only to the player.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	long traitId = ctx.arg(0).rawValue();
	if (traitId >= fo::TRAIT_fast_metabolism && traitId <= fo::TRAIT_gifted) {
		if (fo::var::pc_trait[0] == -1) {
			fo::var::pc_trait[0] = traitId;
		} else if (fo::var::pc_trait[0] != traitId && fo::var::pc_trait[1] == -1) {
			fo::var::pc_trait[1] = traitId;
		} else {
			ctx.printOpcodeError("%s() - cannot add the trait ID: %d", ctx.getMetaruleName(), traitId);
		}
	} else {
		ctx.printOpcodeError("%s() - invalid trait ID.", ctx.getMetaruleName());
	}
}

}
}
