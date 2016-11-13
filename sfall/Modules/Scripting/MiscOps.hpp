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

#include "..\AI.h"
#include "..\Criticals.h"
#include "..\HeroAppearance.h"
#include "..\Inventory.h"
#include "..\KillCounter.h"
#include "..\Knockback.h"
#include "..\Movies.h"
#include "..\ScriptExtender.h"
#include "..\Stats.h"

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

static void __declspec(naked) op_set_dm_model() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
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

static void __declspec(naked) op_set_df_model() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
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

static void __declspec(naked) op_set_movie_path() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		cmp bx, 0xc001;
		jnz end;
		cmp esi, 0;
		jl end;
		cmp esi, MaxMovies;
		jge end;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
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

static void __declspec(naked) op_get_year() {
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
		call FuncOffs::game_time_date_;
		mov edx, [esp];
		mov eax, AddUnarmedStatToGetYear;
		test eax, eax;
		jz end;
		add edx, ds:[VARPTR_pc_proto + 0x4C];
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		add esp, 4;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_game_loaded() {
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
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void _stdcall SetMapMulti(float d);
static void __declspec(naked) op_set_map_time_multi() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xa001;
		jz paramWasFloat;
		cmp dx, 0xc001;
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

static void __declspec(naked) op_set_pipboy_available() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		cmp eax, 0;
		jl end;
		cmp eax, 1;
		jg end;
		mov byte ptr ds:[VARPTR_gmovie_played_list + 0x3], al;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}



// Kill counters
static void __declspec(naked) op_get_kill_counter() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz fail;
		cmp eax, 19;
		jge fail;
		mov edx, ds:[VARPTR_pc_kill_counts+eax*4];
		jmp end;
fail:

		xor edx, edx;
end:
		mov eax, ecx
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_mod_kill_counter() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
		jnz end;
		cmp eax, 19;
		jge end;
		add ds:[VARPTR_pc_kill_counts+eax*4], edi;
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
		call FuncOffs::interpretPopShort_; //First arg type
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;  //First arg
		mov [esp+8], eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_; //Second arg type
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;  //Second arg
		mov [esp+4], eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_; //Third arg type
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;  //Third arg
		mov [esp], eax;
		//Error check
		cmp di, 0xa001;
		jz paramWasFloat;
		cmp di, 0xc001;
		jnz fail;
		fild [esp+8];
		fstp [esp+8];
paramWasFloat:
		cmp dx, 0xc001;
		jnz fail;
		cmp si, 0xc001;
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

static void __declspec(naked) op_set_weapon_knockback() {
	__asm {
		pushad;
		push 0;
		jmp SetKnockback;
	}
}

static void __declspec(naked) op_set_target_knockback() {
	__asm {
		pushad;
		push 1;
		jmp SetKnockback;
	}
}

static void __declspec(naked) op_set_attacker_knockback() {
	__asm {
		pushad;
		push 2;
		jmp SetKnockback;
	}
}

static void __declspec(naked) RemoveKnockback() {
	__asm {
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
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

static void __declspec(naked) op_remove_weapon_knockback() {
	__asm {
		pushad;
		push 0;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_remove_target_knockback() {
	__asm {
		pushad;
		push 1;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_remove_attacker_knockback() {
	__asm {
		pushad;
		push 2;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_get_kill_counter2() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz fail;
		cmp eax, 38;
		jge fail;
		movzx edx, word ptr ds:[VARPTR_pc_kill_counts+eax*2];
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_mod_kill_counter2() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		cmp si, 0xC001;
		jnz end;
		cmp eax, 38;
		jge end;
		add word ptr ds:[VARPTR_pc_kill_counts+eax*2], di;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_active_hand() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, dword ptr ds:[VARPTR_itemCurrentItem];
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_toggle_active_hand() {
	__asm {
		mov eax, 1;
		call FuncOffs::intface_toggle_items_;
		retn;
	}
}

static void __declspec(naked) op_eax_available() {
	__asm {
		push ebx;
		push eax;
		push edx;
		push edi;
		mov edi, eax;
		xor edx, edx
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
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
		SafeWrite8(0x00495C50, 0xe9);
		SafeWrite32(0x00495C51, 0x000001fc);
	} else {
		SafeWrite16(0x00495C50, 0x840f);	//Want to keep this check intact.
		SafeWrite32(0x00495C52, 0x000001fb);

		//SafeWrite16(0x00495C50, 0x9090);
		//SafeWrite32(0x00495C52, 0x90909090);
		SafeWrite16(0x00495C77, 0x9090);	//Check that the player is high enough for the npc to consider this level
		SafeWrite32(0x00495C79, 0x90909090);
		//SafeWrite16(0x00495C8C, 0x9090);	//Check that the npc isn't already at its maximum level
		//SafeWrite32(0x00495C8E, 0x90909090);
		SafeWrite16(0x00495CEC, 0x9090);	//Check that the npc hasn't already levelled up recently
		SafeWrite32(0x00495CEE, 0x90909090);
		if (!npcautolevel) {
			SafeWrite16(0x00495D22, 0x9090);//Random element
			SafeWrite32(0x00495D24, 0x90909090);
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
	SafeWrite8(0x00495BEB, 0xe9);	//Replace the debug output with a jmp
	SafeWrite32(0x00495BEC, ((DWORD)&IncNPCLevel3) - 0x00495BF0);
	__asm {
		call FuncOffs::partyMemberIncLevels_;
	}
	SafeWrite16(0x00495C50, 0x840f);
	SafeWrite32(0x00495C52, 0x000001fb);
	SafeWrite16(0x00495C77, 0x8c0f);
	SafeWrite32(0x00495C79, 0x000001d4);
	//SafeWrite16(0x00495C8C, 0x8d0f);
	//SafeWrite32(0x00495C8E, 0x000001bf);
	SafeWrite16(0x00495CEC, 0x850f);
	SafeWrite32(0x00495CEE, 0x00000130);
	if (!npcautolevel) {
		SafeWrite16(0x00495D22, 0x8f0f);
		SafeWrite32(0x00495D24, 0x00000129);
	}
}

static void __declspec(naked) op_inc_npc_level() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
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
			call FuncOffs::critter_name_
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

static void __declspec(naked) op_get_npc_level() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz fail;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push eax;
		push ds:[VARPTR_partyMemberLevelUpInfoList];
		push ds:[VARPTR_partyMemberPidList];
		push ds:[VARPTR_partyMemberList];
		push ds:[VARPTR_partyMemberMaxCount];
		push ds:[VARPTR_partyMemberCount];
		call get_npc_level2;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov eax, edi;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
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

static void __declspec(naked) op_get_ini_setting() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push 0;
		push eax;
		call GetIniSetting2;
		mov edx, eax;
		jmp result;
error:
		mov edx, -1;
result:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xC001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_get_ini_string() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push 1;
		push eax;
		call GetIniSetting2;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretAddString_;
		mov edx, eax;
		mov ebx, 0x9801;
		jmp result;
error:
		xor edx, edx;
		mov ebx, 0xc001
result:
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, ebx;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
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

static void __declspec(naked) op_get_uptime() {
	__asm {
		pushad;
		mov edi, eax;
		call GetTickCount2;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov edx, 0xc001;
		mov eax, edi;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_set_car_current_town() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		mov ds:[VARPTR_CarCurrArea], eax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) op_set_hp_per_level_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
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

static void __declspec(naked) op_get_bodypart_hit_modifier() {
	__asm {
		push ecx
		push edx
		mov  ecx, eax
		call FuncOffs::interpretPopShort_
		mov  edx, eax
		mov  eax, ecx
		call FuncOffs::interpretPopLong_
		cmp  dx, VAR_TYPE_INT
		jnz  fail
		cmp  eax, 8                               // Body_Uncalled?
		jg   fail
		test eax, eax
		jl   fail
		mov  edx, ds:[VARPTR_hit_location_penalty+eax*4]
		jmp  end
fail:
		xor  edx, edx
end:
		mov  eax, ecx
		call FuncOffs::interpretPushLong_
		mov  eax, ecx
		mov  edx, VAR_TYPE_INT
		call FuncOffs::interpretPushShort_
		pop  edx
		pop  ecx
		retn
	}
}

static void __declspec(naked) op_set_bodypart_hit_modifier() {
	__asm {
		push ebx
		push ecx
		push edx
		mov  ecx, eax
		call FuncOffs::interpretPopShort_
		mov  edx, eax
		mov  eax, ecx
		call FuncOffs::interpretPopLong_
		mov  ebx, eax
		mov  eax, ecx
		call FuncOffs::interpretPopShort_
		xchg eax, ecx
		call FuncOffs::interpretPopLong_
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
		mov  ds:[VARPTR_hit_location_penalty+eax*4], ebx
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

static void __declspec(naked) op_set_critical_table() {
	__asm {
		pushad;
		mov ecx, eax;
		xor ebx, ebx;
		mov edx, 5;
loops:
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		cmp ax, 0xc001;
		jz skip1;
		inc ebx;
skip1:
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
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

static void __declspec(naked) op_get_critical_table() {
	__asm {
		pushad;
		mov edi, eax;
		xor ebx, ebx;
		mov dl, 4;
loops:
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		cmp ax, 0xc001;
		jz skip1;
		inc ebx;
skip1:
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
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
		call FuncOffs::interpretPushLong_;
		mov eax, edi;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_reset_critical_table() {
	__asm {
		pushad;
		mov ecx, eax;
		xor ebx, ebx;
		mov dl, 4;
loops:
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		cmp ax, 0xc001;
		jz skip1;
		inc ebx;
skip1:
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
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

static void __declspec(naked) op_set_unspent_ap_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz end;
		mov StandardApAcBonus, ax;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_get_unspent_ap_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		movzx edx, StandardApAcBonus;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_set_unspent_ap_perk_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz end;
		mov ExtraApAcBonus, ax;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_get_unspent_ap_perk_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		movzx edx, ExtraApAcBonus;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_set_palette() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		mov ebx, eax;
		//mov eax, _black_palette;
		//call FuncOffs::palette_set_to_;
		mov eax, ecx;
		call FuncOffs::interpretGetString_;
  call FuncOffs::loadColorTable_
		mov eax, VARPTR_cmap
  call FuncOffs::palette_set_to_
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

//numbers subgame functions
static void __declspec(naked) op_nb_create_char() {
	__asm {
		/*pushad;
		push eax;
		call NumbersCreateChar;
		mov edx, eax;
		pop eax;
		mov ecx, eax;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;*/
		retn;
	}
}

static void __declspec(naked) op_get_proto_data() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		cmp si, 0xc001;
		jnz fail;
		mov edx, esp;
		call FuncOffs::proto_ptr_;
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
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

static void __declspec(naked) op_set_proto_data() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ebx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		xchg eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		cmp bp, 0xc001;
		jnz end;
		//mov eax, [eax+0x64];
		mov edx, esp;
		call FuncOffs::proto_ptr_;
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

static void __declspec(naked) op_hero_select_win() {//for opening the appearance selection window
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		push eax;
		cmp dx, 0xC001;
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

static void __declspec(naked) op_set_hero_style() {//for setting the hero style/appearance takes an 1 int
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		push eax;
		cmp dx, 0xC001;
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

static void __declspec(naked) op_set_hero_race() {// for setting the hero race takes an 1 int
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		push eax;
		cmp dx, 0xC001;
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

static void __declspec(naked) op_get_light_level() {
	__asm {
		pushad;
		mov ecx, eax;
		mov edx, ds:[VARPTR_ambient_light];
		call FuncOffs::interpretPushLong_
		mov edx, 0xc001;
		mov eax, ecx;
		call FuncOffs::interpretPushShort_
		popad;
		retn;
	}
}

static void __declspec(naked) op_refresh_pc_art() {
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

static void __declspec(naked) op_get_attack_type() {
	__asm {
		push edx;
		push ecx;
		mov ecx, eax;
		mov edx, ds:[VARPTR_main_ctd + 0x4];
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}

static void __declspec(naked) op_play_sfall_sound() {
	__asm {
		pushad
		mov edi, eax;
		call FuncOffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0x9001;
		jz next;
		cmp dx, 0x9801;
		jnz end;
next:
		cmp bx, 0xc001;
		jnz end;
		mov ebx, eax;
		mov eax, edi;
		call FuncOffs::interpretGetString_;
		push esi;
		push eax;
		mov eax, esi;
		mov esi, 65;
		call PlaySfallSound;
		mov edx, eax;
		mov eax, edi;
		call FuncOffs::interpretPushLong_;
		mov eax, edi;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
end:
		popad
		retn;
	}
}

static void __declspec(naked) op_stop_sfall_sound() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		push eax;
		call StopSfallSound;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) op_get_tile_fid() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		sub esp, 8;
		lea edx, [esp];
		lea ebx, [esp+4];
		call FuncOffs::tile_coord_;
		mov eax, [esp];
		mov edx, [esp+4];
		call FuncOffs::square_num_;
		mov edx, ds:[VARPTR_square];
		movzx edx, word ptr ds:[edx+eax*4];
		add esp, 8;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}
static DWORD modifiedIni;
static void __declspec(naked) op_modified_ini() {
	__asm {
		pushad;
		mov edx, modifiedIni;
		mov ebp, eax;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_force_aimed_shots() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call ForceAimedShots;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_disable_aimed_shots() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call DisableAimedShots;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_mark_movie_played() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 0x11;
		jge end;
		mov byte ptr ds:[eax + VARPTR_gmovie_played_list], 1;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

static void __declspec(naked) op_get_last_attacker() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		push eax;
		call AIGetLastAttacker;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_get_last_target() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		push eax;
		call AIGetLastTarget;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) op_block_combat() {
	__asm {
		pushad;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
		jnz end;
		push eax;
		call AIBlockCombat;
end:
		popad
		retn;
	}
}

static void __declspec(naked) op_tile_under_cursor() {
	__asm {
		push edx;
		push ecx;
		push ebx;
		mov ecx, eax;
		sub esp, 8;
		lea edx, [esp];
		lea eax, [esp+4];
		call FuncOffs::mouse_get_position_;
		mov ebx, dword ptr ds:[VARPTR_map_elevation];
		mov edx, [esp];
		mov eax, [esp+4];
		call FuncOffs::tile_num_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		add esp, 8;
		pop ebx;
		pop ecx;
		pop edx;
		retn;
	}
}

static void __declspec(naked) op_gdialog_get_barter_mod() {
	__asm {
		push edx;
		push ecx;
		mov ecx, eax;
		mov edx, dword ptr ds:[VARPTR_gdBarterMod];
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}

static void __declspec(naked) op_set_inven_ap_cost() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xC001;
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
		mov eax, ds:[VARPTR_sneak_working]
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
		call FuncOffs::light_get_tile_
		jmp end
fail:
		mov eax, 0
end:
	}
	_RET_VAL_INT(ebp)
	_OP_END
}


static void sf_exec_map_update_scripts() {
	__asm call FuncOffs::scr_exec_map_update_scripts_
}
