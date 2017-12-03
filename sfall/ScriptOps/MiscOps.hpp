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

#include "AI.h"
#include "HeroAppearance.h"
#include "KillCounter.h"
#include "Knockback.h"
#include "movies.h"
#include "ScriptExtender.h"

/*
 *	Misc operators
 */

static DWORD dmModelNamePtr=(DWORD)dmModelName;
static DWORD dfModelNamePtr=(DWORD)dfModelName;
static DWORD MovieNamesPtr=(DWORD)MoviePaths;


//// *** End Helios *** ///
static void _stdcall strcpy_p(char* to, const char* from) {
	strcpy_s(to, 64, from);
}
static void __declspec(naked) SetDMModel() {
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
		push dmModelNamePtr;
		call strcpy_p;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetDFModel() {
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
		push dfModelNamePtr;
		call strcpy_p;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetMoviePath() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov edi, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		cmp bx, VAR_TYPE_INT;
		jnz end;
		cmp esi, 0;
		jl end;
		cmp esi, MaxMovies;
		jge end;
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		mov eax, esi;
		mov esi, 65;
		mul si;
		add eax, MovieNamesPtr;
		push eax;
		call strcpy_p;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetYear() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		sub esp, 4;
		xor eax, eax;
		xor edx, edx;
		mov ebx, esp;
		call game_time_date_;
		mov edx, [esp];
		mov eax, AddUnarmedStatToGetYear;
		test eax, eax;
		jz end;
		add edx, ds:[_pc_proto + 0x4C];
end:
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		add esp, 4;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GameLoaded() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push eax;
		push eax;
		call ScriptHasLoaded;
		mov edx, eax;
		pop eax;
		mov ecx, eax;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void _stdcall SetMapMulti(float d);
static void __declspec(naked) set_map_time_multi() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_FLOAT;
		jz paramWasFloat;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		push eax;
		fild dword ptr [esp];
		fstp dword ptr [esp];
		jmp end;
paramWasFloat:
		push eax;
end:
		call SetMapMulti;
fail:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) SetPipBoyAvailable() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp eax, 0;
		jl end;
		cmp eax, 1;
		jg end;
		mov byte ptr ds:[_gmovie_played_list + 0x3], al;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}



// Kill counters
static void __declspec(naked) GetKillCounter() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp eax, 19;
		jge fail;
		mov edx, ds:[_pc_kill_counts+eax*4];
		jmp end;
fail:

		xor edx, edx;
end:
		mov eax, ecx
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) ModKillCounter() {
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
		cmp eax, 19;
		jge end;
		add ds:[_pc_kill_counts+eax*4], edi;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
//Knockback
static void __declspec(naked) SetKnockback() {
	__asm {
		sub esp, 0xc;
		mov ecx, eax;
		//Get args
		call interpretPopShort_; //First arg type
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;  //First arg
		mov [esp+8], eax;
		mov eax, ecx;
		call interpretPopShort_; //Second arg type
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;  //Second arg
		mov [esp+4], eax;
		mov eax, ecx;
		call interpretPopShort_; //Third arg type
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;  //Third arg
		mov [esp], eax;
		//Error check
		cmp di, VAR_TYPE_FLOAT;
		jz paramWasFloat;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		fild [esp+8];
		fstp [esp+8];
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
		popad;
		retn;
	}
}
static void __declspec(naked) SetWeaponKnockback() {
	__asm {
		pushad;
		push 0;
		jmp SetKnockback;
	}
}
static void __declspec(naked) SetTargetKnockback() {
	__asm {
		pushad;
		push 1;
		jmp SetKnockback;
	}
}
static void __declspec(naked) SetAttackerKnockback() {
	__asm {
		pushad;
		push 2;
		jmp SetKnockback;
	}
}
static void __declspec(naked) RemoveKnockback() {
	__asm {
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call KnockbackRemoveMod;
		jmp end;
fail:
		add esp, 4;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) RemoveWeaponKnockback() {
	__asm {
		pushad;
		push 0;
		jmp RemoveKnockback;
	}
}
static void __declspec(naked) RemoveTargetKnockback() {
	__asm {
		pushad;
		push 1;
		jmp RemoveKnockback;
	}
}
static void __declspec(naked) RemoveAttackerKnockback() {
	__asm {
		pushad;
		push 2;
		jmp RemoveKnockback;
	}
}
static void __declspec(naked) GetKillCounter2() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp eax, 38;
		jge fail;
		movzx edx, word ptr ds:[_pc_kill_counts+eax*2];
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) ModKillCounter2() {
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
		cmp eax, 38;
		jge end;
		add word ptr ds:[_pc_kill_counts+eax*2], di;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) GetActiveHand() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, dword ptr ds:[_itemCurrentItem];
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) ToggleActiveHand() {
	__asm {
		push ebx;
		mov eax, 1;
		mov ebx, intface_toggle_items_;
		call ebx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) EaxAvailable() {
	__asm {
		push ebx;
		push eax;
		push edx;
		push edi;
		mov edi, eax;
		xor edx, edx
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static char* NPCToInc;
static void _stdcall IncNPCLevel4(char* npc) {
	if (_stricmp(npc, NPCToInc)) {
		SafeWrite8(0x495C50, 0xE9);
		SafeWrite32(0x495C51, 0x000001FC);
	} else {
		SafeWrite16(0x495C50, 0x840F);	//Want to keep this check intact.
		SafeWrite32(0x495C52, 0x000001FB);

		//SafeWrite16(0x495C50, 0x9090);
		//SafeWrite32(0x495C52, 0x90909090);
		SafeWrite16(0x495C77, 0x9090);	//Check that the player is high enough for the npc to consider this level
		SafeWrite32(0x495C79, 0x90909090);
		//SafeWrite16(0x495C8C, 0x9090);	//Check that the npc isn't already at its maximum level
		//SafeWrite32(0x495C8E, 0x90909090);
		SafeWrite16(0x495CEC, 0x9090);	//Check that the npc hasn't already levelled up recently
		SafeWrite32(0x495CEE, 0x90909090);
		if (!npcautolevel) {
			SafeWrite16(0x495D22, 0x9090);//Random element
			SafeWrite32(0x495D24, 0x90909090);
		}
	}
}
static void __declspec(naked) IncNPCLevel3() {
	__asm {
		pushad;
		push eax;
		call IncNPCLevel4;
		popad;
		push 0x495BF9;
		retn;
	}
}
static void _stdcall IncNPCLevel2(char* npc) {
	NPCToInc = npc;
	MakeJump(0x495BEB, IncNPCLevel3);	//Replace the debug output with a jmp
	__asm {
		call partyMemberIncLevels_;
	}
	SafeWrite16(0x495C50, 0x840F);
	SafeWrite32(0x495C52, 0x000001FB);
	SafeWrite16(0x495C77, 0x8C0F);
	SafeWrite32(0x495C79, 0x000001D4);
	//SafeWrite16(0x495C8C, 0x8D0F);
	//SafeWrite32(0x495C8E, 0x000001BF);
	SafeWrite16(0x495CEC, 0x850F);
	SafeWrite32(0x495CEE, 0x00000130);
	if (!npcautolevel) {
		SafeWrite16(0x495D22, 0x8F0F);
		SafeWrite32(0x495D24, 0x00000129);
	}
}
static void __declspec(naked) IncNPCLevel() {
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
		call IncNPCLevel2;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static int _stdcall get_npc_level2(DWORD numMembers, DWORD maxMembers, DWORD* members, DWORD* pids, DWORD* words, const char* name) {
	for (DWORD i = 0; i < numMembers; i++) {
		const char* name2;
		__asm {
			mov eax, members;
			mov eax, [eax];
			call critter_name_
			mov name2, eax;
		}
		if (_stricmp(name, name2)) {
			members += 4;
			continue;
		}

		DWORD pid = ((DWORD*)members[0])[25];
		DWORD id = -1;
		for (DWORD j = 0; j < maxMembers; j++) {
			if (pids[j] == pid) {
				id = j;
				break;
			}
		}
		if (id == -1) break;

		return words[id * 3];
	}
	return 0;
}
static void __declspec(naked) get_npc_level() {
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
		jnz fail;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push eax;
		push ds:[_partyMemberLevelUpInfoList];
		push ds:[_partyMemberPidList];
		push ds:[_partyMemberList];
		push ds:[_partyMemberMaxCount];
		push ds:[_partyMemberCount];
		call get_npc_level2;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
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

static char IniStrBuffer[128];
static DWORD _stdcall GetIniSetting2(const char* c, DWORD string) {
	const char* key = strstr(c, "|");
	if (!key) return -1;
	DWORD filelen = (DWORD)key - (DWORD)c;
	if (filelen >= 64) return -1;
	key = strstr(key + 1, "|");
	if (!key) return -1;
	DWORD seclen = (DWORD)key - ((DWORD)c + filelen + 1);
	if (seclen > 32) return -1;

	char file[67];
	file[0] = '.';
	file[1] = '\\';
	memcpy(&file[2], c, filelen);
	file[filelen + 2] = 0;

	char section[33];
	memcpy(section, &c[filelen + 1], seclen);
	section[seclen] = 0;

	key++;
	if (string) {
		IniStrBuffer[0] = 0;
		GetPrivateProfileStringA(section, key, "", IniStrBuffer, 128, file);
		return (DWORD)&IniStrBuffer[0];
	} else {
		return GetPrivateProfileIntA(section, key, -1, file);
	}
}
static void __declspec(naked) GetIniSetting() {
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
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push 0;
		push eax;
		call GetIniSetting2;
		mov edx, eax;
		jmp result;
error:
		mov edx, -1;
result:
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetIniString() {
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
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push 1;
		push eax;
		call GetIniSetting2;
		mov edx, eax;
		mov eax, edi;
		call interpretAddString_;
		mov edx, eax;
		mov ebx, VAR_TYPE_STR;
		jmp result;
error:
		xor edx, edx;
		mov ebx, VAR_TYPE_INT
result:
		mov eax, edi;
		call interpretPushLong_;
		mov edx, ebx;
		mov eax, edi;
		call interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static DWORD _stdcall GetTickCount2() {
	return GetTickCount();
}
static void __declspec(naked) funcGetTickCount() {
	__asm {
		pushad;
		mov edi, eax;
		call GetTickCount2;
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) SetCarTown() {
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
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov ds:[_CarCurrArea], eax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) SetLevelHPMod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		push 0x4AFBC1;
		call SafeWrite8;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) GetBodypartHitModifier() {
	__asm {
		push ecx
		push edx
		mov  ecx, eax
		call interpretPopShort_
		mov  edx, eax
		mov  eax, ecx
		call interpretPopLong_
		cmp  dx, VAR_TYPE_INT
		jnz  fail
		cmp  eax, 8                               // Body_Uncalled?
		jg   fail
		test eax, eax
		jl   fail
		mov  edx, ds:[_hit_location_penalty+eax*4]
		jmp  end
fail:
		xor  edx, edx
end:
		mov  eax, ecx
		call interpretPushLong_
		mov  eax, ecx
		mov  edx, VAR_TYPE_INT
		call interpretPushShort_
		pop  edx
		pop  ecx
		retn
	}
}

static void __declspec(naked) SetBodypartHitModifier() {
	__asm {
		push ebx
		push ecx
		push edx
		mov  ecx, eax
		call interpretPopShort_
		mov  edx, eax
		mov  eax, ecx
		call interpretPopLong_
		mov  ebx, eax
		mov  eax, ecx
		call interpretPopShort_
		xchg eax, ecx
		call interpretPopLong_
		cmp  dx, VAR_TYPE_INT
		jnz  end
		cmp  cx, VAR_TYPE_INT
		jnz  end
		cmp  eax, 8                               // Body_Uncalled?
		jg   end
		cmp  eax, 3                               // Body_Torso?
		jne  skip                                 // No
		add  eax, 5
skip:
		test eax, eax
		jl   end
		mov  ds:[_hit_location_penalty+eax*4], ebx
		cmp  eax, 8                               // Body_Uncalled?
		jne  end                                  // No
		sub  eax, 5                               // Body_Torso
		jmp  skip
end:
		pop  edx
		pop  ecx
		pop  ebx
		retn
	}
}

static void __declspec(naked) funcSetCriticalTable() {
	__asm {
		pushad;
		mov ecx, eax;
		xor ebx, ebx;
		mov edx, 5;
loops:
		mov eax, ecx;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		jz skip1;
		inc ebx;
skip1:
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		dec dx;
		jnz loops;
		test ebx, ebx;
		jnz fail;
		call SetCriticalTable;
		jmp end;
fail:
		add esp, 20;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) funcGetCriticalTable() {
	__asm {
		pushad;
		mov edi, eax;
		xor ebx, ebx;
		mov dl, 4;
loops:
		mov eax, edi;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		jz skip1;
		inc ebx;
skip1:
		mov eax, edi;
		call interpretPopLong_;
		push eax;
		dec dx;
		jnz loops;
		test ebx, ebx;
		jz fail;
		call ResetCriticalTable;
		mov edx, eax;
		jmp end;
fail:
		add esp, 16;
		xor edx, edx;
end:
		mov eax, edi;
		call interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) funcResetCriticalTable() {
	__asm {
		pushad;
		mov ecx, eax;
		xor ebx, ebx;
		mov dl, 4;
loops:
		mov eax, ecx;
		call interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		jz skip1;
		inc ebx;
skip1:
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		dec dx;
		jnz loops;
		test ebx, ebx;
		jz fail;
		call ResetCriticalTable;
		jmp end;
fail:
		add esp, 16;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) SetApAcBonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov StandardApAcBonus, ax;
end:
		pop edx;
		pop ecx;
		retn;
	}
}
static void __declspec(naked) GetApAcBonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		movzx edx, StandardApAcBonus;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}
static void __declspec(naked) SetApAcEBonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov ExtraApAcBonus, ax;
end:
		pop edx;
		pop ecx;
		retn;
	}
}
static void __declspec(naked) GetApAcEBonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		movzx edx, ExtraApAcBonus;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) SetPalette() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		//mov eax, _black_palette;
		//call palette_set_to_;
		mov eax, ecx;
		call interpretGetString_;
  call loadColorTable_
		mov eax, _cmap
  call palette_set_to_
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

//numbers subgame functions
static void __declspec(naked) NBCreateChar() {
	__asm {
		/*pushad;
		push eax;
		call NumbersCreateChar;
		mov edx, eax;
		pop eax;
		mov ecx, eax;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;*/
		retn;
	}
}

static void __declspec(naked) get_proto_data() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		cmp si, VAR_TYPE_INT;
		jnz fail;
		mov edx, esp;
		call proto_ptr_;
		mov eax, [esp];
		test eax, eax;
		jz fail;
		mov edx, [eax+ecx/**4*/];
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}
static void __declspec(naked) set_proto_data() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		mov ebx, eax;
		mov eax, ebp;
		call interpretPopShort_;
		xchg eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		cmp bp, VAR_TYPE_INT;
		jnz end;
		//mov eax, [eax+0x64];
		mov edx, esp;
		call proto_ptr_;
		mov eax, [esp];
		test eax, eax;
		jz end;
		mov [eax+ebx/**4*/], ecx;
end:
		add esp, 4;
		popad;
		retn;
	}
}

static void __declspec(naked) funcHeroSelectWin() {//for opening the appearance selection window
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		call HeroSelectWindow;
		jmp end;
fail:
		pop eax;
end:
		pop esi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) funcSetHeroStyle() {//for setting the hero style/appearance takes an 1 int
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		call SetHeroStyle;
		jmp end;
fail:
		pop eax;
end:
		pop esi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) funcSetHeroRace() {// for setting the hero race takes an 1 int
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		push eax;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		call SetHeroRace;
		jmp end;
fail:
		pop eax;
end:
		pop esi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) get_light_level() {
	__asm {
		pushad;
		mov ecx, eax;
		mov edx, ds:[_ambient_light];
		call interpretPushLong_
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_
		popad;
		retn;
	}
}
static void __declspec(naked) refresh_pc_art() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		call RefreshPCArt;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) get_attack_type() {
	__asm {
		push edx;
		push ecx;
		mov ecx, eax;
		mov edx, ds:[_main_ctd + 0x4];
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}
static void __declspec(naked) play_sfall_sound() {
	__asm {
		pushad
		mov edi, eax;
		call interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		cmp bx, VAR_TYPE_INT;
		jnz end;
		mov ebx, eax;
		mov eax, edi;
		call interpretGetString_;
		push esi;
		push eax;
		mov eax, esi;
		mov esi, 65;
		call PlaySfallSound;
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
end:
		popad
		retn;
	}
}
static void __declspec(naked) stop_sfall_sound() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		push eax;
		call StopSfallSound;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) get_tile_pid() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		sub esp, 8;
		lea edx, [esp];
		lea ebx, [esp+4];
		call tile_coord_;
		mov eax, [esp];
		mov edx, [esp+4];
		call square_num_;
		mov edx, ds:[_square];
		movzx edx, word ptr ds:[edx+eax*4];
		add esp, 8;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static DWORD modifiedIni;
static void __declspec(naked) modified_ini() {
	__asm {
		pushad;
		mov edx, modifiedIni;
		mov ebp, eax;
		call interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) force_aimed_shots() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call ForceAimedShots;
end:
		pop edx;
		pop ecx;
		retn;
	}
}
static void __declspec(naked) disable_aimed_shots() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call DisableAimedShots;
end:
		pop edx;
		pop ecx;
		retn;
	}
}
static void __declspec(naked) mark_movie_played() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 0x11;
		jge end;
		mov byte ptr ds:[eax+_gmovie_played_list], 1;
end:
		pop edx;
		pop ecx;
		retn;
	}
}
static void __declspec(naked) get_last_attacker() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call AIGetLastAttacker;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) get_last_target() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		push eax;
		call AIGetLastTarget;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) block_combat() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call AIBlockCombat;
end:
		popad
		retn;
	}
}
static void __declspec(naked) tile_under_cursor() {
	__asm {
		push edx;
		push ecx;
		push ebx;
		mov ecx, eax;
		sub esp, 8;
		lea edx, [esp];
		lea eax, [esp+4];
		call mouse_get_position_;
		mov ebx, dword ptr ds:[_map_elevation];
		mov edx, [esp];
		mov eax, [esp+4];
		call tile_num_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		add esp, 8;
		pop ebx;
		pop ecx;
		pop edx;
		retn;
	}
}
static void __declspec(naked) gdialog_get_barter_mod() {
	__asm {
		push edx;
		push ecx;
		mov ecx, eax;
		mov edx, dword ptr ds:[_gdBarterMod];
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}
static void __declspec(naked) set_inven_ap_cost() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call SetInvenApCost;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_sneak_success() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, ds:[_sneak_working]
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) op_tile_light() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ebx, edi)  // arg2 - tile
	_GET_ARG_R32(ebp, ecx, esi)  // arg1 - elevation
	_CHECK_ARG_INT(bx, fail)
	__asm {
		mov eax, esi
		mov edx, edi
		call light_get_tile_
		jmp end
fail:
		mov eax, 0
end:
	}
	_RET_VAL_INT(ebp)
	_OP_END
}


static void sf_exec_map_update_scripts() {
	__asm call scr_exec_map_update_scripts_
}