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

#include "..\..\Combat.h"
#include "..\..\Criticals.h"
#include "..\..\CritterStats.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Skills.h"
#include "..\..\Stats.h"
#include "..\OpcodeContext.h"

#include "Stats.h"

namespace sfall
{
namespace script
{

static const char* invalidStat = "%s() - stat number out of range.";
static const char* objNotCritter = "%s() - the object is not a critter.";

__declspec(naked) void op_set_hp_per_level_mod() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax; // allowed -/+127
		push 0x4AFBC1;
		call SafeWrite8;
end:
		mov  ecx, esi;
		retn;
	}
}

void op_set_pc_base_stat(OpcodeContext& ctx) {
	int stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		((long*)FO_VAR_pc_proto)[9 + stat] = ctx.arg(1).rawValue();
		if (stat <= fo::STAT_lu) fo::func::stat_recalc_derived(fo::var::obj_dude);
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
}

void op_set_pc_extra_stat(OpcodeContext& ctx) {
	int stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		((long*)FO_VAR_pc_proto)[44 + stat] = ctx.arg(1).rawValue();
		if (stat <= fo::STAT_lu) fo::func::stat_recalc_derived(fo::var::obj_dude);
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
}

void op_get_pc_base_stat(OpcodeContext& ctx) {
	int value = 0,
		stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		value = ((long*)FO_VAR_pc_proto)[9 + stat];
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
	ctx.setReturn(value);
}

void op_get_pc_extra_stat(OpcodeContext& ctx) {
	int value = 0,
		stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		value = ((long*)FO_VAR_pc_proto)[44 + stat];
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
	ctx.setReturn(value);
}

void op_set_critter_base_stat(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj && obj->IsCritter()) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			CritterStats::SetStat(obj, stat, ctx.arg(2).rawValue(), 9);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

void op_set_critter_extra_stat(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj && obj->IsCritter()) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			CritterStats::SetStat(obj, stat, ctx.arg(2).rawValue(), 44);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

void op_get_critter_base_stat(OpcodeContext& ctx) {
	int result = 0;
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj && obj->IsCritter()) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			result = CritterStats::GetStat(obj, stat, 9);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
	ctx.setReturn(result);
}

void op_get_critter_extra_stat(OpcodeContext& ctx) {
	int result = 0;
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj && obj->IsCritter()) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			result = CritterStats::GetStat(obj, stat, 44);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
	ctx.setReturn(result);
}

void op_set_critter_skill_points(OpcodeContext& ctx) {
	unsigned long skill = ctx.arg(1).rawValue();
	if (skill > 17) {
		ctx.printOpcodeError("%s() - incorrect skill number.", ctx.getOpcodeName());
		return;
	}

	fo::GameObject* obj = ctx.arg(0).object();
	if (obj->IsCritter()) {
		fo::Proto* proto;
		if (fo::util::GetProto(obj->protoId, &proto)) proto->critter.skills[skill] = ctx.arg(2).rawValue();
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

void op_get_critter_skill_points(OpcodeContext& ctx) {
	unsigned long skill = ctx.arg(1).rawValue();
	if (skill > 17) {
		ctx.printOpcodeError("%s() - incorrect skill number.", ctx.getOpcodeName());
		return;
	}

	fo::GameObject* obj = ctx.arg(0).object();
	if (obj->IsCritter()) {
		fo::Proto* proto;
		if (fo::util::GetProto(obj->protoId, &proto)) ctx.setReturn(proto->critter.skills[skill]);
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

__declspec(naked) void op_set_available_skill_points() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  edx, eax;
		xor  eax, eax;
		call fo::funcoffs::stat_pc_set_;
end:
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_get_available_skill_points() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_curr_pc_stat];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_mod_skill_points_per_level() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, 100;
		cmp  eax, ecx;
		cmovg eax, ecx;
		neg  ecx; // -100
		cmp  eax, ecx;
		cmovl eax, ecx;
		add  eax, 5; // add fallout default points
		push eax;
		push 0x43C27A;
		call SafeWrite8;
end:
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_set_unspent_ap_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  Stats::standardApAcBonus, eax;
end:
		retn;
	}
}

__declspec(naked) void op_get_unspent_ap_bonus() {
	__asm {
		mov  edx, Stats::standardApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_set_unspent_ap_perk_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  Stats::extraApAcBonus, eax;
end:
		retn;
	}
}

__declspec(naked) void op_get_unspent_ap_perk_bonus() {
	__asm {
		mov  edx, Stats::extraApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_get_critter_current_ap() {
	using namespace fo;
	using namespace Fields;
	__asm {
		_GET_ARG_INT(fail); // Get function arg and check if valid
		test eax, eax;
		jz   fail;
		mov  edx, [eax + protoId];
		shr  edx, 24;
		cmp  edx, OBJ_TYPE_CRITTER;
		jnz  fail;
		mov  edx, [eax + movePoints];
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT); // Pass back the result
fail:
		xor  edx, edx;
		jmp  end;
	}
}

void op_set_critter_current_ap(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj->IsCritter()) {
		long ap = ctx.arg(1).rawValue();
		if (ap < 0) ap = 0;
		obj->critter.movePoints = ap;

		if (obj == fo::var::obj_dude) fo::func::intface_update_move_points(ap, fo::var::combat_free_move);
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

__declspec(naked) void op_set_pickpocket_max() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		xor  edx, edx;
		test eax, eax;
		cmovs eax, edx;  // max < 0
		mov  ecx, 999;
		cmp  eax, ecx;
		cmova eax, ecx;  // 0 - 999 (maximum)
		push edx;        // mod (0)
		push eax;        // maximum
		push 0xFFFFFFFF; // critter
		call SetPickpocketMax;
end:
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_set_hit_chance_max() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		xor  edx, edx;
		test eax, eax;
		cmovs eax, edx;  // max < 0
		mov  ecx, 999;
		cmp  eax, ecx;
		cmova eax, ecx;  // 0 - 999 (maximum)
		push edx;        // mod (0)
		push eax;        // maximum
		push 0xFFFFFFFF; // critter
		call SetHitChanceMax;
end:
		mov  ecx, esi;
		retn;
	}
}

void op_set_critter_hit_chance_mod(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj->IsCritter()) {
		SetHitChanceMax(obj, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

__declspec(naked) void op_set_base_hit_chance_mod() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		push 0xFFFFFFFF;
		call SetHitChanceMax;
end:
		pop  ecx;
		retn;
	}
}

void op_set_critter_pickpocket_mod(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj->IsCritter()) {
		SetPickpocketMax(obj, ctx.arg(1).rawValue(), ctx.arg(2).rawValue());
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

__declspec(naked) void op_set_base_pickpocket_mod() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		push 0xFFFFFFFF;
		call SetPickpocketMax;
end:
		pop  ecx;
		retn;
	}
}

void op_set_critter_skill_mod(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	if (obj->IsCritter()) {
		SetSkillMax(obj, ctx.arg(1).rawValue());
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

__declspec(naked) void op_set_base_skill_mod() { // same as set_skill_max
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		push 0xFFFFFFFF;
		call SetSkillMax;
end:
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_set_skill_max() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, 300;
		cmp  eax, ecx;
		cmova eax, ecx; // 0 - 300
		push eax;
		push 0xFFFFFFFF;
		call SetSkillMax;
end:
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_set_stat_max() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		push ecx;
		push eax;
		call SetPCStatMax;
		call SetNPCStatMax;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_set_stat_min() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		push ecx;
		push eax;
		call SetPCStatMin;
		call SetNPCStatMin;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_set_pc_stat_max() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		call SetPCStatMax;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_set_pc_stat_min() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		call SetPCStatMin;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_set_npc_stat_max() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		call SetNPCStatMax;
end:
		pop  ecx;
		retn;
	}
}

__declspec(naked) void op_set_npc_stat_min() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi);
		mov  eax, ebx;
		_GET_ARG_INT(end);
		cmp  si, VAR_TYPE_INT;
		jne  end;
		push ecx;
		push eax;
		call SetNPCStatMin;
end:
		pop  ecx;
		retn;
	}
}

void mf_get_stat_max(OpcodeContext& ctx) {
	int who = (ctx.numArgs() > 1) ? ctx.arg(1).rawValue() : 0;
	ctx.setReturn(
		Stats::GetStatMax(ctx.arg(0).rawValue(), who)
	);
}

void mf_get_stat_min(OpcodeContext& ctx) {
	int who = (ctx.numArgs() > 1) ? ctx.arg(1).rawValue() : 0;
	ctx.setReturn(
		Stats::GetStatMin(ctx.arg(0).rawValue(), who)
	);
}

static __declspec(naked) void statPCAddExperienceCheckPMs_hack() {
	static DWORD xpTemp;
	__asm {
		mov  ebp, [esp];  // return addr
		mov  xpTemp, eax; // experience
		fild xpTemp;
		fmul Stats::experienceMod;
		fistp xpTemp;
		mov  eax, xpTemp;
		sub  esp, 0xC; // instead of 0x10
		mov  edi, eax;
		jmp  ebp;
	}
}

void op_set_xp_mod(OpcodeContext& ctx) {
	static bool xpModPatch = false;
	long percent = ctx.arg(0).rawValue() & 0xFFFF;
	Stats::experienceMod = percent / 100.0f;

	if (xpModPatch) return;
	xpModPatch = true;
	MakeCall(0x4AFABD, statPCAddExperienceCheckPMs_hack);
}

}
}
