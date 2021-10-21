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

#include "Combat.h"
#include "Criticals.h"
#include "Skills.h"
#include "Stats.h"

namespace sfall
{

const char* invalidStat = "%s() - stat number out of range.";
const char* objNotCritter = "%s() - the object is not a critter.";

static void __declspec(naked) op_set_hp_per_level_mod() {
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

// stat_funcs
static void __stdcall op_set_pc_base_stat2() {
	const ScriptValue &statArg = opHandler.arg(0),
	                  &valArg = opHandler.arg(1);

	if (statArg.isInt() && valArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			((long*)FO_VAR_pc_proto)[9 + stat] = valArg.rawValue();
		} else {
			opHandler.printOpcodeError(invalidStat, "set_pc_base_stat");
		}
	} else {
		OpcodeInvalidArgs("set_pc_base_stat");
	}
}

static void __declspec(naked) op_set_pc_base_stat() {
	_WRAP_OPCODE(op_set_pc_base_stat2, 2, 0)
}

static void __stdcall op_set_pc_extra_stat2() {
	const ScriptValue &statArg = opHandler.arg(0),
	                  &valArg = opHandler.arg(1);

	if (statArg.isInt() && valArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			((long*)FO_VAR_pc_proto)[44 + stat] = valArg.rawValue();
		} else {
			opHandler.printOpcodeError(invalidStat, "set_pc_extra_stat");
		}
	} else {
		OpcodeInvalidArgs("set_pc_extra_stat");
	}
}

static void __declspec(naked) op_set_pc_extra_stat() {
	_WRAP_OPCODE(op_set_pc_extra_stat2, 2, 0)
}

static void __stdcall op_get_pc_base_stat2() {
	int value = 0;
	const ScriptValue &statArg = opHandler.arg(0);

	if (statArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			value = ((long*)FO_VAR_pc_proto)[9 + stat];
		} else {
			opHandler.printOpcodeError(invalidStat, "get_pc_base_stat");
		}
	} else {
		OpcodeInvalidArgs("get_pc_base_stat");
	}
	opHandler.setReturn(value);

}

static void __declspec(naked) op_get_pc_base_stat() {
	_WRAP_OPCODE(op_get_pc_base_stat2, 1, 1)
}

static void __stdcall op_get_pc_extra_stat2() {
	int value = 0;
	const ScriptValue &statArg = opHandler.arg(0);

	if (statArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < fo::STAT_max_stat) {
			value = ((long*)FO_VAR_pc_proto)[44 + stat];
		} else {
			opHandler.printOpcodeError(invalidStat, "get_pc_extra_stat");
		}
	} else {
		OpcodeInvalidArgs("get_pc_extra_stat");
	}
	opHandler.setReturn(value);
}

static void __declspec(naked) op_get_pc_extra_stat() {
	_WRAP_OPCODE(op_get_pc_extra_stat2, 1, 1)
}

static void __stdcall op_set_critter_base_stat2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1),
	                  &valArg = opHandler.arg(2);

	if (obj && statArg.isInt() && valArg.isInt()) {
		if (obj->IsCritter()) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < fo::STAT_max_stat) {
				fo::Proto* proto = fo::util::GetProto(obj->protoId);
				if (proto != nullptr) ((long*)proto)[9 + stat] = valArg.rawValue();
			} else {
				opHandler.printOpcodeError(invalidStat, "set_critter_base_stat");
			}
		} else {
			opHandler.printOpcodeError(objNotCritter, "set_critter_base_stat");
		}
	} else {
		OpcodeInvalidArgs("set_critter_base_stat");
	}
}

static void __declspec(naked) op_set_critter_base_stat() {
	_WRAP_OPCODE(op_set_critter_base_stat2, 3, 0)
}

static void __stdcall op_set_critter_extra_stat2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1),
	                  &valArg = opHandler.arg(2);

	if (obj && statArg.isInt() && valArg.isInt()) {
		if (obj->IsCritter()) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < fo::STAT_max_stat) {
				fo::Proto* proto = fo::util::GetProto(obj->protoId);
				if (proto != nullptr) ((long*)proto)[44 + stat] = valArg.rawValue();
			} else {
				opHandler.printOpcodeError(invalidStat, "set_critter_extra_stat");
			}
		} else {
			opHandler.printOpcodeError(objNotCritter, "set_critter_extra_stat");
		}
	} else {
		OpcodeInvalidArgs("set_critter_extra_stat");
	}
}

static void __declspec(naked) op_set_critter_extra_stat() {
	_WRAP_OPCODE(op_set_critter_extra_stat2, 3, 0)
}

static void __stdcall op_get_critter_base_stat2() {
	int result = 0;
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1);

	if (obj && statArg.isInt()) {
		if (obj->IsCritter()) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < fo::STAT_max_stat) {
				fo::Proto* proto = fo::util::GetProto(obj->protoId);
				if (proto != nullptr) result = ((long*)proto)[9 + stat];
			} else {
				opHandler.printOpcodeError(invalidStat, "get_critter_base_stat");
			}
		} else {
			opHandler.printOpcodeError(objNotCritter, "get_critter_base_stat");
		}
	} else {
		OpcodeInvalidArgs("get_critter_base_stat");
	}
	opHandler.setReturn(result);
}

static void __declspec(naked) op_get_critter_base_stat() {
	_WRAP_OPCODE(op_get_critter_base_stat2, 2, 1)
}

static void __stdcall op_get_critter_extra_stat2() {
	int result = 0;
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1);

	if (obj && statArg.isInt()) {
		if (obj->IsCritter()) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < fo::STAT_max_stat) {
				fo::Proto* proto = fo::util::GetProto(obj->protoId);
				if (proto != nullptr) result = ((long*)proto)[44 + stat];
			} else {
				opHandler.printOpcodeError(invalidStat, "get_critter_extra_stat");
			}
		} else {
			opHandler.printOpcodeError(objNotCritter, "get_critter_extra_stat");
		}
	} else {
		OpcodeInvalidArgs("get_critter_extra_stat");
	}
	opHandler.setReturn(result);
}

static void __declspec(naked) op_get_critter_extra_stat() {
	_WRAP_OPCODE(op_get_critter_extra_stat2, 2, 1)
}

static void __declspec(naked) op_set_critter_skill_points() {
	__asm {
		pushaop;
		//Get function args
		mov  ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov  edi, eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov  esi, eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		//eax now contains the critter ID, esi the skill ID, and edi the new value
		//Check args are valid
		mov  ebx, [esp];
		cmp  bx, VAR_TYPE_INT;
		jnz  end;
		mov  ebx, [esp + 4];
		cmp  bx, VAR_TYPE_INT;
		jnz  end;
		mov  ebx, [esp + 8];
		cmp  bx, VAR_TYPE_INT;
		jnz  end;
		test esi, esi;
		jl   end;
		cmp  esi, 18;
		jge  end;
		//set the new value
		mov  eax, [eax + 0x64];
		mov  edx, esp;
		call fo::funcoffs::proto_ptr_;
		test eax, eax;
		js   end;
		mov  eax, [esp];
		mov  [eax + 0x13C + esi * 4], edi;
end:
		//Restore registers and return
		add  esp, 12;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_get_critter_skill_points() {
	__asm {
		pushaop;
		//Get function args
		mov  ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov  esi, eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		push eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		//eax now contains the critter ID, esi the skill ID
		//Check args are valid
		mov  ebx, [esp];
		cmp  bx, VAR_TYPE_INT;
		jnz  fail;
		mov  ebx, [esp + 4];
		cmp  bx, VAR_TYPE_INT;
		jnz  fail;
		test esi, esi;
		jl   fail;
		cmp  esi, 18;
		jge  fail;
		//get the value
		mov  eax, [eax + 0x64];
		mov  edx, esp;
		call fo::funcoffs::proto_ptr_;
		test eax, eax;
		js   fail;
		mov  eax, [esp];
		mov  edx, [eax + 0x13C + esi * 4];
		jmp  end;
fail:
		xor  edx, edx;
end:
		mov  eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov  edx, VAR_TYPE_INT;
		mov  eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		//Restore registers and return
		add  esp, 8;
		popaop;
		retn;
	}
}

static void __declspec(naked) op_set_available_skill_points() {
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

static void __declspec(naked) op_get_available_skill_points() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_curr_pc_stat];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_mod_skill_points_per_level() {
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

static void __declspec(naked) op_set_unspent_ap_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  standardApAcBonus, eax;
end:
		retn;
	}
}

static void __declspec(naked) op_get_unspent_ap_bonus() {
	__asm {
		mov  edx, standardApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_set_unspent_ap_perk_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  extraApAcBonus, eax;
end:
		retn;
	}
}

static void __declspec(naked) op_get_unspent_ap_perk_bonus() {
	__asm {
		mov  edx, extraApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_get_critter_current_ap() {
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

static void __stdcall op_set_critter_current_ap2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &apArg = opHandler.arg(1);

	if (obj && apArg.isInt()) {
		if (obj->IsCritter()) {
			long ap = apArg.rawValue();
			if (ap < 0) ap = 0;
			obj->critter.movePoints = ap;

			if (obj == *fo::ptr::obj_dude) fo::func::intface_update_move_points(ap, *fo::ptr::combat_free_move);
		} else {
			opHandler.printOpcodeError(objNotCritter, "set_critter_current_ap");
		}
	} else {
		OpcodeInvalidArgs("set_critter_current_ap");
	}
}

static void __declspec(naked) op_set_critter_current_ap() {
	_WRAP_OPCODE(op_set_critter_current_ap2, 2, 0)
}

static void __declspec(naked) op_set_pickpocket_max() {
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

static void __declspec(naked) op_set_hit_chance_max() {
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

static void __stdcall op_set_critter_hit_chance_mod2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &maxArg = opHandler.arg(1),
	                  &modArg = opHandler.arg(2);

	if (obj && maxArg.isInt() && modArg.isInt()) {
		if (obj->IsCritter()) {
			SetHitChanceMax(obj, maxArg.rawValue(), modArg.rawValue());
		} else {
			opHandler.printOpcodeError(objNotCritter, "set_critter_hit_chance_mod");
		}
	} else {
		OpcodeInvalidArgs("set_critter_hit_chance_mod");
	}
}

static void __declspec(naked) op_set_critter_hit_chance_mod() {
	_WRAP_OPCODE(op_set_critter_hit_chance_mod2, 3, 0)
}

static void __declspec(naked) op_set_base_hit_chance_mod() {
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

static void __stdcall op_set_critter_pickpocket_mod2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &maxArg = opHandler.arg(1),
	                  &modArg = opHandler.arg(2);

	if (obj && maxArg.isInt() && modArg.isInt()) {
		if (obj->IsCritter()) {
			SetPickpocketMax(obj, maxArg.rawValue(), modArg.rawValue());
		} else {
			opHandler.printOpcodeError(objNotCritter, "set_critter_pickpocket_mod");
		}
	} else {
		OpcodeInvalidArgs("set_critter_pickpocket_mod");
	}
}

static void __declspec(naked) op_set_critter_pickpocket_mod() {
	_WRAP_OPCODE(op_set_critter_pickpocket_mod2, 3, 0)
}

static void __declspec(naked) op_set_base_pickpocket_mod() {
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

static void __stdcall op_set_critter_skill_mod2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &maxArg = opHandler.arg(1);

	if (obj && maxArg.isInt()) {
		if (obj->IsCritter()) {
			SetSkillMax(obj, maxArg.rawValue());
		} else {
			opHandler.printOpcodeError(objNotCritter, "set_critter_skill_mod");
		}
	} else {
		OpcodeInvalidArgs("set_critter_skill_mod");
	}
}

static void __declspec(naked) op_set_critter_skill_mod() {
	_WRAP_OPCODE(op_set_critter_skill_mod2, 2, 0)
}

static void __declspec(naked) op_set_base_skill_mod() { // same as set_skill_max
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

static void __declspec(naked) op_set_skill_max() {
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

static void __declspec(naked) op_set_stat_max() {
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

static void __declspec(naked) op_set_stat_min() {
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

static void __declspec(naked) op_set_pc_stat_max() {
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

static void __declspec(naked) op_set_pc_stat_min() {
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

static void __declspec(naked) op_set_npc_stat_max() {
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

static void __declspec(naked) op_set_npc_stat_min() {
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

static void mf_get_stat_max() {
	const ScriptValue &statArg = opHandler.arg(0);

	if (statArg.isInt()) {
		int who = 0;
		if (opHandler.numArgs() > 1) {
			const ScriptValue &whoArg = opHandler.arg(1);
			if (!whoArg.isInt()) goto invalidArgs;
			who = whoArg.rawValue();
		}
		opHandler.setReturn(
			GetStatMax(statArg.rawValue(), who)
		);
	} else {
invalidArgs:
		OpcodeInvalidArgs("get_stat_max");
		opHandler.setReturn(0);
	}
}

static void mf_get_stat_min() {
	const ScriptValue &statArg = opHandler.arg(0);

	if (statArg.isInt()) {
		int who = 0;
		if (opHandler.numArgs() > 1) {
			const ScriptValue &whoArg = opHandler.arg(1);
			if (!whoArg.isInt()) goto invalidArgs;
			who = whoArg.rawValue();
		}
		opHandler.setReturn(
			GetStatMin(statArg.rawValue(), who)
		);
	} else {
invalidArgs:
		OpcodeInvalidArgs("get_stat_min");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) statPCAddExperienceCheckPMs_hack() {
	static DWORD xpTemp;
	__asm {
		mov  ebp, [esp];  // return addr
		mov  xpTemp, eax; // experience
		fild xpTemp;
		fmul experienceMod;
		fistp xpTemp;
		mov  eax, xpTemp;
		sub  esp, 0xC; // instead of 0x10
		mov  edi, eax;
		jmp  ebp;
	}
}

static void __stdcall op_set_xp_mod2() {
	const ScriptValue &pctArg = opHandler.arg(0);

	if (pctArg.isInt()) {
		static bool xpModPatch = false;
		long percent = pctArg.rawValue() & 0xFFFF;
		experienceMod = percent / 100.0f;

		if (xpModPatch) return;
		xpModPatch = true;
		MakeCall(0x4AFABD, statPCAddExperienceCheckPMs_hack);
	} else {
		OpcodeInvalidArgs("set_xp_mod");
	}
}

static void __declspec(naked) op_set_xp_mod() {
	_WRAP_OPCODE(op_set_xp_mod2, 1, 0)
}

}
