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

#include "..\..\main.h"

#include "..\Criticals.h"
#include "..\Knockback.h"
#include "..\ScriptExtender.h"
#include "..\Skills.h"
#include "..\Stats.h"

// stat_funcs
static void __declspec(naked) SetPCBaseStat() {
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
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		//eax now contains the stat ID, edi contains its new value
		//Check args are valid
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge end;
		//set the new value
		mov ds:[eax*4 + 0x51C394], edi;
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
static void __declspec(naked) SetPCExtraStat() {
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
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		//eax now contains the stat ID, edi contains its new value
		//Check args are valid
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge end;
		//set the new value
		mov ds:[eax*4 + 0x51C420], edi;
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
static void __declspec(naked) GetPCBaseStat() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		//Check arg is valid
		cmp dx, 0xC001;
		jnz fail;
		test eax, eax;
		jl fail;
		cmp eax, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge fail;
		//set the new value
		mov edx, ds:[eax*4 + 0x51C394];
		jmp end;
fail:
		xor edx, edx;
end:
		//Pass back the result
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetPCExtraStat() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		//Check arg is valid
		cmp dx, 0xC001;
		jnz fail;
		test eax, eax;
		jl fail;
		cmp eax, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge fail;
		//set the new value
		mov edx, ds:[eax*4 + 0x51C420];
		jmp end;
fail:
		xor edx, edx;
end:
		//Pass back the result
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetCritterBaseStat() {
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
		//eax now contains the critter ID, esi the stat ID, and edi the new value
		//Check args are valid
		mov ebx, [esp];
		cmp bx, 0xC001;
		jnz end;
		mov ebx, [esp+4];
		cmp bx, 0xC001;
		jnz end;
		mov ebx, [esp+8];
		cmp bx, 0xC001;
		jnz end;
		test esi, esi;
		jl end;
		cmp esi, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge end;
		//set the new value
		mov edx, esp;
		mov eax, [eax+0x64];
		call proto_ptr_;
		mov eax, [esp];
		add eax, 0x20;
		mov ds:[eax + esi*4 + 4], edi;
end:
		//Restore registers and return
		add esp, 12;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetCritterExtraStat() {
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
		//eax now contains the critter ID, esi the stat ID, and edi the new value
		//Check args are valid
		mov ebx, [esp];
		cmp bx, 0xC001;
		jnz end;
		mov ebx, [esp+4];
		cmp bx, 0xC001;
		jnz end;
		mov ebx, [esp+8];
		cmp bx, 0xC001;
		jnz end;
		test esi, esi;
		jl end;
		cmp esi, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge end;
		//set the new value
		mov edx, esp;
		mov eax, [eax+0x64];
		call proto_ptr_;
		mov eax, [esp];
		add eax, 0x20;
		mov ds:[eax + esi*4 + 0x90], edi;
end:
		//Restore registers and return
		add esp, 12;
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetCritterBaseStat() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		push edi;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		push eax
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax
		mov eax, ecx;
		call interpretPopLong_;
		//eax contains the critter pointer, and edi contains the stat id
		//Check args are valid
		mov ebx, [esp];
		cmp bx, 0xC001;
		jnz fail;
		mov ebx, [esp+4];
		cmp bx, 0xc001;
		jnz fail;
		test edi, edi;
		jl fail;
		cmp edi, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge fail;
		//set the new value
		mov edx, esp;
		mov eax, [eax+0x64];
		call proto_ptr_;
		mov eax, [esp];
		add eax, 0x20;
		mov edx, ds:[eax + edi*4 + 4];
		jmp end;
fail:
		xor edx, edx;
end:
		//Pass back the result
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		add esp, 8;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetCritterExtraStat() {
	__asm {
		//Store registers
		push ebx;
		push ecx;
		push edx;
		push edi;
		//Get function args
		mov ecx, eax;
		call interpretPopShort_;
		push eax
		mov eax, ecx;
		call interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopShort_;
		push eax
		mov eax, ecx;
		call interpretPopLong_;
		//eax contains the critter pointer, and edi contains the stat id
		//Check args are valid
		mov ebx, [esp];
		cmp bx, 0xC001;
		jnz fail;
		mov ebx, [esp+4];
		cmp bx, 0xc001;
		jnz fail;
		test edi, edi;
		jl fail;
		cmp edi, 0x23; //23, 24 and 25 are valid, but stored elsewhere. Ignore for now.
		jge fail;
		//set the new value
		mov edx, esp;
		mov eax, [eax+0x64];
		call proto_ptr_;
		mov eax, [esp];
		add eax, 0x20;
		mov edx, ds:[eax + edi*4 + 0x90];
		jmp end;
fail:
		xor edx, edx;
end:
		//Pass back the result
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call interpretPushShort_;
		//Restore registers and return
		add esp, 8;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
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
		cmp bx, 0xC001;
		jnz end;
		mov ebx, [esp+4];
		cmp bx, 0xC001;
		jnz end;
		mov ebx, [esp+8];
		cmp bx, 0xC001;
		jnz end;
		test esi, esi;
		jl end;
		cmp esi, 18;
		jge end;
		//set the new value
		mov eax, [eax+0x64];
		mov edx, esp;
		call proto_ptr_;
		mov eax, [esp];
		mov [eax+0x13c+esi*4], edi;
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
		cmp bx, 0xC001;
		jnz fail;
		mov ebx, [esp+4];
		cmp bx, 0xC001;
		jnz fail;
		test esi, esi;
		jl fail;
		cmp esi, 18;
		jge fail;
		//get the value
		mov eax, [eax+0x64];
		mov edx, esp;
		call proto_ptr_;
		mov eax, [esp];
		mov edx, [eax+0x13c+esi*4];
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, 0xc001;
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
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		xchg eax, ecx;
		call interpretPopLong_;
		cmp cx, 0xc001;
		jnz end;
		mov edx, eax;
		xor eax, eax;
		call stat_pc_set_;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) get_available_skill_points() {
	__asm {
		pushad;
		mov ecx, eax;
		mov edx, dword ptr ds:[_curr_pc_stat];
		call interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call interpretPushShort_
		popad;
		retn;
	}
}
static void __declspec(naked) mod_skill_points_per_level() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		xchg eax, ecx;
		call interpretPopLong_;
		cmp cx, 0xc001;
		jnz end;
		cmp eax, 100;
		jg end;
		cmp eax, -100;
		jl end;
		add eax, 5;
		push eax;
		push 0x43C27a;
		call SafeWrite8;
end:
		popad;
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
		cmp di, 0xC001;
		jnz fail;
		mov edx, [eax+0x40];
		jmp end;
fail:
		xor edx, edx;
end:
		//Pass back the result
		mov eax, ecx;
		call interpretPushLong_;
		mov edx, 0xc001;
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
		cmp di, 0xC001;
		jnz end;
		cmp si, 0xC001;
		jnz end;
		mov [eax+0x40], ebx;
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
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, 0xC001;
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
static void __declspec(naked) fSetHitChanceMax() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, 0xC001;
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
static void __declspec(naked) SetCritterHitChance() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
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
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
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
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
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
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
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
static void __declspec(naked) SetCritterSkillMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, 0xc001;
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
static void __declspec(naked) SetBaseSkillMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		xor ebx, ebx
		call interpretPopShort_;
		cmp ax, 0xc001;
		cmovne ebx, edi;
		mov eax, edi;
		call interpretPopLong_;
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
static void __declspec(naked) fSetSkillMax() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, 0xC001;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
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
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
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
	SafeWrite8(0x004AFAB8, 0xe9);
	SafeWrite32(0x004AFAB9, (DWORD)&SetXpMod3 - 0x004AFABD);
	xpmod=(float)percent/100.0f;
}
static void __declspec(naked) SetXpMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, 0xC001;
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
static int PerkLevelMod;
static void __declspec(naked) SetPerkLevelMod3() {
	__asm {
		push ebx;
		call stat_pc_get_;
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
	if(mod<-25||mod>25) return;
	PerkLevelMod=mod;
	SafeWrite32(0x00496880, (DWORD)&SetPerkLevelMod3 - 0x00496884);
}
static void __declspec(naked) SetPerkLevelMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call SetPerkLevelMod2;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
