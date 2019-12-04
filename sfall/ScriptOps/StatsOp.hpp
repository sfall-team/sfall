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

#include "main.h"

#include "Combat.h"
#include "Criticals.h"
#include "ScriptExtender.h"
#include "Skills.h"
#include "Stats.h"

const char* invalidStat = "%s() - stat number out of range.";
const char* objNotCritter = "%s() - the object is not a critter.";

// stat_funcs
static void _stdcall SetPCBaseStat2() {
	const ScriptValue &statArg = opHandler.arg(0),
					  &valArg = opHandler.arg(1);

	if (statArg.isInt() && valArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < STAT_max_stat) {
			((long*)_pc_proto)[9 + stat] = valArg.rawValue();
		} else {
			opHandler.printOpcodeError(invalidStat, "set_pc_base_stat");
		}
	} else {
		OpcodeInvalidArgs("set_pc_base_stat");
	}
}

static void __declspec(naked) SetPCBaseStat() {
	_WRAP_OPCODE(SetPCBaseStat2, 2, 0)
}

static void _stdcall SetPCExtraStat2() {
	const ScriptValue &statArg = opHandler.arg(0),
					  &valArg = opHandler.arg(1);

	if (statArg.isInt() && valArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < STAT_max_stat) {
			((long*)_pc_proto)[44 + stat] = valArg.rawValue();
		} else {
			opHandler.printOpcodeError(invalidStat, "set_pc_extra_stat");
		}
	} else {
		OpcodeInvalidArgs("set_pc_extra_stat");
	}
}

static void __declspec(naked) SetPCExtraStat() {
	_WRAP_OPCODE(SetPCExtraStat2, 2, 0)
}

static void _stdcall GetPCBaseStat2() {
	int value = 0;
	const ScriptValue &statArg = opHandler.arg(0);

	if (statArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < STAT_max_stat) {
			value = ((long*)_pc_proto)[9 + stat];
		} else {
			opHandler.printOpcodeError(invalidStat, "get_pc_base_stat");
		}
	} else {
		OpcodeInvalidArgs("get_pc_base_stat");
	}
	opHandler.setReturn(value, DATATYPE_INT);

}

static void __declspec(naked) GetPCBaseStat() {
	_WRAP_OPCODE(GetPCBaseStat2, 1, 1)
}

static void _stdcall GetPCExtraStat2() {
	int value = 0;
	const ScriptValue &statArg = opHandler.arg(0);

	if (statArg.isInt()) {
		int stat = statArg.rawValue();
		if (stat >= 0 && stat < STAT_max_stat) {
			value = ((long*)_pc_proto)[44 + stat];
		} else {
			opHandler.printOpcodeError(invalidStat, "get_pc_extra_stat");
		}
	} else {
		OpcodeInvalidArgs("get_pc_extra_stat");
	}
	opHandler.setReturn(value, DATATYPE_INT);
}

static void __declspec(naked) GetPCExtraStat() {
	_WRAP_OPCODE(GetPCExtraStat2, 1, 1)
}

static void _stdcall SetCritterBaseStat2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1),
					  &valArg = opHandler.arg(2);

	if (obj && statArg.isInt() && valArg.isInt()) {
		if (obj->pid >> 24 == OBJ_TYPE_CRITTER) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < STAT_max_stat) {
				char* proto = GetProtoPtr(obj->pid);
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

static void __declspec(naked) SetCritterBaseStat() {
	_WRAP_OPCODE(SetCritterBaseStat2, 3, 0)
}

static void _stdcall SetCritterExtraStat2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1),
					  &valArg = opHandler.arg(2);

	if (obj && statArg.isInt() && valArg.isInt()) {
		if (obj->pid >> 24 == OBJ_TYPE_CRITTER) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < STAT_max_stat) {
				char* proto = GetProtoPtr(obj->pid);
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

static void __declspec(naked) SetCritterExtraStat() {
	_WRAP_OPCODE(SetCritterExtraStat2, 3, 0)
}

static void _stdcall GetCritterBaseStat2() {
	int result = 0;
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1);

	if (obj && statArg.isInt()) {
		if (obj->pid >> 24 == OBJ_TYPE_CRITTER) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < STAT_max_stat) {
				char* proto = GetProtoPtr(obj->pid);
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
	opHandler.setReturn(result, DATATYPE_INT);
}

static void __declspec(naked) GetCritterBaseStat() {
	_WRAP_OPCODE(GetCritterBaseStat2, 2, 1)
}

static void _stdcall GetCritterExtraStat2() {
	int result = 0;
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &statArg = opHandler.arg(1);

	if (obj && statArg.isInt()) {
		if (obj->pid >> 24 == OBJ_TYPE_CRITTER) {
			int stat = statArg.rawValue();
			if (stat >= 0 && stat < STAT_max_stat) {
				char* proto = GetProtoPtr(obj->pid);
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
	opHandler.setReturn(result, DATATYPE_INT);
}

static void __declspec(naked) GetCritterExtraStat() {
	_WRAP_OPCODE(GetCritterExtraStat2, 2, 1)
}

static void __declspec(naked) set_critter_skill_points() {
	__asm {
		pushad;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
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
		call proto_ptr_;
		mov eax, [esp];
		mov [eax + 0x13C + esi * 4], edi;
end:
		//Restore registers and return
		add esp, 12;
		popad
		retn;
	}
}

static void __declspec(naked) get_critter_skill_points() {
	__asm {
		pushad;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax;
		mov eax, ecx;
		call interpretPopLong_;
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
		call proto_ptr_;
		mov eax, [esp];
		mov edx, [eax + 0x13C + esi * 4];
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		add esp, 8;
		popad
		retn;
	}
}

static void __declspec(naked) set_available_skill_points() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  edx, eax;
		xor  eax, eax;
		call stat_pc_set_;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) get_available_skill_points() {
	__asm {
		mov  edx, dword ptr ds:[_curr_pc_stat];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) mod_skill_points_per_level() {
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

static void __declspec(naked) GetCritterAP() {
	__asm {
		//Store registers
		push ecx;
		push edx;
		push edi;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
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
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		pop edi;
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) SetCritterAP() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		//Check args are valid
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		mov [eax + 0x40], ebx;
		mov ecx, ds:[_obj_dude]
		cmp ecx, eax;
		jne end;
		mov eax, ebx;
		mov edx, ds:[_combat_free_move]
		call intface_update_move_points_;
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

static void __declspec(naked) fSetPickpocketMax() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, 100;
		cmp  eax, ecx;
		cmova eax, ecx; // 0 - 100
		push 0;
		push eax;
		push 0xFFFFFFFF;
		call SetPickpocketMax;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) fSetHitChanceMax() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, 100;
		cmp  eax, ecx;
		cmova eax, ecx; // 0 - 100
		push 0;
		push eax;
		push 0xFFFFFFFF;
		call SetHitChanceMax;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) SetCritterHitChance() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
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

static void __declspec(naked) SetBaseHitChance() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
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

static void __declspec(naked) SetCritterPickpocket() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
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

static void __declspec(naked) SetBasePickpocket() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		test ebx, ebx;
		jnz end;
		push ecx;
		push eax;
		push 0xFFFFFFFF;
		call SetPickpocketMax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) SetCritterSkillMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
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

static void __declspec(naked) SetBaseSkillMod() { // same as set_skill_max
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

static void __declspec(naked) fSetSkillMax() {
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

static void __declspec(naked) SetStatMax() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static void __declspec(naked) SetStatMin() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static void __declspec(naked) fSetPCStatMax() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static void __declspec(naked) fSetPCStatMin() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static void __declspec(naked) fSetNPCStatMax() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static void __declspec(naked) fSetNPCStatMin() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
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

static DWORD xpTemp;
static void __declspec(naked) statPCAddExperienceCheckPMs_hack() {
	__asm {
		mov  ebp, [esp];  // return addr
		mov  xpTemp, eax; // experience
		fild xpTemp;
		fmul ExperienceMod;
		fistp xpTemp;
		mov  eax, xpTemp;
		sub  esp, 0xC; // instead of 0x10
		mov  edi, eax;
		jmp  ebp;
	}
}

static void _stdcall SetXpMod2() {
	const ScriptValue &pctArg = opHandler.arg(0);

	if (pctArg.isInt()) {
		static bool xpModPatch = false;
		DWORD percent = pctArg.rawValue() & 0xFFFF;
		ExperienceMod = (float)percent / 100.0f;

		if (xpModPatch) return;
		xpModPatch = true;
		MakeCall(0x4AFABD, statPCAddExperienceCheckPMs_hack);
	} else {
		OpcodeInvalidArgs("set_xp_mod");
	}
}

static void __declspec(naked) SetXpMod() {
	_WRAP_OPCODE(SetXpMod2, 1, 0)
}
