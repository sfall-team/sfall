/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\Combat.h"
#include "..\..\Criticals.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Skills.h"
#include "..\..\Stats.h"
#include "..\OpcodeContext.h"

#include "Stats.h"

namespace sfall
{
namespace script
{

const char* invalidStat   = "%s() - stat number out of range.";
const char* objNotCritter = "%s() - the object is not a critter.";

void sf_set_pc_base_stat(OpcodeContext& ctx) {
	int stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		((long*)FO_VAR_pc_proto)[9 + stat] = ctx.arg(1).rawValue();
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
}

void sf_set_pc_extra_stat(OpcodeContext& ctx) {
	int stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		((long*)FO_VAR_pc_proto)[44 + stat] = ctx.arg(1).rawValue();
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
}

void sf_get_pc_base_stat(OpcodeContext& ctx) {
	int value = 0,
		stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		value = ((long*)FO_VAR_pc_proto)[9 + stat];
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
	ctx.setReturn(value);
}

void sf_get_pc_extra_stat(OpcodeContext& ctx) {
	int value = 0,
		stat = ctx.arg(0).rawValue();
	if (stat >= 0 && stat < fo::STAT_max_stat) {
		value = ((long*)FO_VAR_pc_proto)[44 + stat];
	} else {
		ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
	}
	ctx.setReturn(value);
}

void sf_set_critter_base_stat(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).asObject();
	if (obj && obj->Type() == fo::OBJ_TYPE_CRITTER) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			Stats::SetStat(obj, stat, ctx.arg(2).rawValue(), 9);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

void sf_set_critter_extra_stat(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).asObject();
	if (obj && obj->Type() == fo::OBJ_TYPE_CRITTER) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			Stats::SetStat(obj, stat, ctx.arg(2).rawValue(), 44);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
}

void sf_get_critter_base_stat(OpcodeContext& ctx) {
	int result = 0;
	fo::GameObject* obj = ctx.arg(0).asObject();
	if (obj && obj->Type() == fo::OBJ_TYPE_CRITTER) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			result = Stats::GetStat(obj, stat, 9);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
	ctx.setReturn(result);
}

void sf_get_critter_extra_stat(OpcodeContext& ctx) {
	int result = 0;
	fo::GameObject* obj = ctx.arg(0).asObject();
	if (obj && obj->Type() == fo::OBJ_TYPE_CRITTER) {
		int stat = ctx.arg(1).rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			result = Stats::GetStat(obj, stat, 44);
		} else {
			ctx.printOpcodeError(invalidStat, ctx.getOpcodeName());
		}
	} else {
		ctx.printOpcodeError(objNotCritter, ctx.getOpcodeName());
	}
	ctx.setReturn(result);
}

void __declspec(naked) op_set_critter_skill_points() {
	__asm {
		pushad;
		//Get function args
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		//eax now contains the critter ID, esi the skill ID, and edi the new value
		//Check args are valid
		mov ebx, [esp];
		cmp bx, VAR_TYPE_INT;
		jnz end;
		mov ebx, [esp + 4];
		cmp bx, VAR_TYPE_INT;
		jnz end;
		mov ebx, [esp + 8];
		cmp bx, VAR_TYPE_INT;
		jnz end;
		test esi, esi;
		jl end;
		cmp esi, 18;
		jge end;
		//set the new value
		mov eax, [eax + 0x64];
		mov edx, esp;
		call fo::funcoffs::proto_ptr_;
		mov eax, [esp];
		mov [eax + 0x13c + esi * 4], edi;
end:
		//Restore registers and return
		add esp, 12;
		popad
		retn;
	}
}

void __declspec(naked) op_get_critter_skill_points() {
	__asm {
		pushad;
		//Get function args
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		//eax now contains the critter ID, esi the skill ID
		//Check args are valid
		mov ebx, [esp];
		cmp bx, VAR_TYPE_INT;
		jnz fail;
		mov ebx, [esp + 4];
		cmp bx, VAR_TYPE_INT;
		jnz fail;
		test esi, esi;
		jl fail;
		cmp esi, 18;
		jge fail;
		//get the value
		mov eax, [eax + 0x64];
		mov edx, esp;
		call fo::funcoffs::proto_ptr_;
		mov eax, [esp];
		mov edx, [eax + 0x13c + esi * 4];
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		//Restore registers and return
		add esp, 8;
		popad
		retn;
	}
}

void __declspec(naked) op_set_available_skill_points() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		mov  edx, eax;
		xor  eax, eax;
		call fo::funcoffs::stat_pc_set_;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_get_available_skill_points() {
	__asm {
		push edx;
		push ecx;
		mov  edx, dword ptr ds:[FO_VAR_curr_pc_stat];
		_RET_VAL_INT(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

void __declspec(naked) op_mod_skill_points_per_level() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		cmp  eax, 100;
		jg   end;
		cmp  eax, -100;
		jl   end;
		add  eax, 5;
		push eax;
		push 0x43C27A;
		call SafeWrite8;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

void __declspec(naked) op_get_critter_current_ap() {
	__asm {
		//Store registers
		push ecx;
		push edx;
		push edi;
		//Get function args
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		//Check args are valid
		cmp di, VAR_TYPE_INT;
		jnz fail;
		mov edx, [eax + 0x40];
		jmp end;
fail:
		xor edx, edx;
end:
		//Pass back the result
		mov eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		//Restore registers and return
		pop edi;
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_set_critter_current_ap() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		//Get function args
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov ebx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		//Check args are valid
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		mov [eax + 0x40], ebx;
		mov ecx, ds:[FO_VAR_obj_dude]
		cmp ecx, eax;
		jne end;
		mov eax, ebx;
		mov edx, ds:[FO_VAR_combat_free_move]
		call fo::funcoffs::intface_update_move_points_;
end:
		//Restore registers and return
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_pickpocket_max() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		and eax, 0xff;
		cmp eax, 100;
		jg end;
		push 0;
		push eax;
		push 0xffffffff;
		call SetPickpocketMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_hit_chance_max() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		and eax, 0xff;
		cmp eax, 100;
		jg end;
		push 0;
		push eax;
		push 0xffffffff;
		call SetHitChanceMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_critter_hit_chance_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		test ebx, ebx;
		jnz end;
		push ecx;
		push edx;
		push eax;
		call SetHitChanceMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_base_hit_chance_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		test ebx, ebx;
		jnz end;
		push ecx;
		push eax;
		push 0xffffffff;
		call SetHitChanceMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_critter_pickpocket_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		test ebx, ebx;
		jnz end;
		push ecx;
		push edx;
		push eax;
		call SetPickpocketMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_base_pickpocket_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		test ebx, ebx;
		jnz end;
		push ecx;
		push eax;
		push 0xffffffff;
		call SetPickpocketMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_critter_skill_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		test ebx, ebx;
		jnz end;
		push ecx;
		push eax;
		call SetSkillMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_base_skill_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		test ebx, ebx;
		jnz end;
		push eax;
		push 0xffffffff;
		call SetSkillMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_skill_max() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		and eax, 0xffff;
		cmp eax, 300;
		jg end;
		push eax;
		push 0xffffffff;
		call SetSkillMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_stat_max() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edi;
		push eax;
		push edi;
		push eax;
		call SetPCStatMax;
		call SetNPCStatMax;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_stat_min() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edi;
		push eax;
		push edi;
		push eax;
		call SetPCStatMin;
		call SetNPCStatMin;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_pc_stat_max() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edi;
		push eax;
		call SetPCStatMax;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_pc_stat_min() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edi;
		push eax;
		call SetPCStatMin;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_npc_stat_max() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edi;
		push eax;
		call SetNPCStatMax;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_npc_stat_min() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edi;
		push eax;
		call SetNPCStatMin;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static float xpmod;
static DWORD xptmp;
static void __declspec(naked) SetXpMod3() {
	__asm {
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		mov  xptmp, eax;
		fild xptmp;
		fmul xpmod;
		fistp xptmp;
		mov  eax, xptmp;
		push 0x4AFABD;
		retn;
	}
}

static void _stdcall SetXpMod2(DWORD percent) {
	MakeJump(0x4AFAB8, SetXpMod3);
	xpmod = (float)percent / 100.0f;
}

static int PerkLevelMod;
static void __declspec(naked) SetPerkLevelMod3() {
	__asm {
		push ebx;
		call fo::funcoffs::stat_pc_get_;
		pop ebx;
		add eax, PerkLevelMod;
		cmp eax, 0;
		jge end;
		xor eax, eax;
end:
		retn;
	}
}

static void _stdcall SetPerkLevelMod2(int mod) {
	if (mod < -25 || mod > 25) return;
	PerkLevelMod = mod;
	HookCall(0x49687F, &SetPerkLevelMod3);
}

void __declspec(naked) op_set_xp_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		and eax, 0xffff;
		push eax;
		call SetXpMod2;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_perk_level_mod() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		push eax;
		call SetPerkLevelMod2;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

}
}
