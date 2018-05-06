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

#include <cstring>

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\AI.h"
#include "..\..\Criticals.h"
#include "..\..\HeroAppearance.h"
#include "..\..\Inventory.h"
#include "..\..\KillCounter.h"
#include "..\..\Knockback.h"
#include "..\..\MiscPatches.h"
#include "..\..\Movies.h"
#include "..\..\PlayerModel.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Stats.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "Misc.h"

namespace sfall
{
namespace script
{

static DWORD defaultMaleModelNamePtr = (DWORD)defaultMaleModelName;
static DWORD defaultFemaleModelNamePtr = (DWORD)defaultFemaleModelName;
static DWORD movieNamesPtr = (DWORD)MoviePaths;


//// *** End Helios *** ///
static void _stdcall strcpy_p(char* to, const char* from) {
	strcpy_s(to, 64, from);
}

/************************************************************************/
/* TODO: Rewrite these raw handlers using OpcodeContext                 */
/************************************************************************/

void __declspec(naked) op_set_dm_model() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push defaultMaleModelNamePtr;
		call strcpy_p;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_df_model() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push defaultFemaleModelNamePtr;
		call strcpy_p;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_movie_path() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		push esi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
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
		call fo::funcoffs::interpretGetString_;
		push eax;
		mov eax, esi;
		mov esi, 65;
		mul si;
		add eax, movieNamesPtr;
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

void __declspec(naked) op_get_year() {
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
		call fo::funcoffs::game_time_date_;
		mov edx, [esp];
		mov eax, addUnarmedStatToGetYear;
		test eax, eax;
		jz end;
		add edx, ds:[FO_VAR_pc_proto + 0x4C];
end:
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call fo::funcoffs::interpretPushShort_;
		add esp, 4;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_game_loaded() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push eax;
		push eax;
		call ScriptHasLoaded;
		xor edx, edx;
		mov dl, al;
		pop eax;
		mov ecx, eax;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_map_time_multi() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_set_pipboy_available() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp eax, 0;
		jl end;
		cmp eax, 1;
		jg end;
		mov byte ptr ds:[FO_VAR_gmovie_played_list + 0x3], al;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}


// Kill counters
void __declspec(naked) op_get_kill_counter() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp eax, 19;
		jge fail;
		mov edx, ds:[FO_VAR_pc_kill_counts+eax*4];
		jmp end;
fail:

		xor edx, edx;
end:
		mov eax, ecx
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_mod_kill_counter() {
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
		cmp eax, 19;
		jge end;
		add ds:[FO_VAR_pc_kill_counts+eax*4], edi;
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
void __declspec(naked) SetKnockback() {
	__asm {
		sub esp, 0xc;
		mov ecx, eax;
		//Get args
		call fo::funcoffs::interpretPopShort_; //First arg type
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;  //First arg
		mov [esp+8], eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_; //Second arg type
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;  //Second arg
		mov [esp+4], eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_; //Third arg type
		mov esi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;  //Third arg
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

void __declspec(naked) op_set_weapon_knockback() {
	__asm {
		pushad;
		push 0;
		jmp SetKnockback;
	}
}

void __declspec(naked) op_set_target_knockback() {
	__asm {
		pushad;
		push 1;
		jmp SetKnockback;
	}
}

void __declspec(naked) op_set_attacker_knockback() {
	__asm {
		pushad;
		push 2;
		jmp SetKnockback;
	}
}

static void __declspec(naked) RemoveKnockback() {
	__asm {
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_remove_weapon_knockback() {
	__asm {
		pushad;
		push 0;
		jmp RemoveKnockback;
	}
}

void __declspec(naked) op_remove_target_knockback() {
	__asm {
		pushad;
		push 1;
		jmp RemoveKnockback;
	}
}

void __declspec(naked) op_remove_attacker_knockback() {
	__asm {
		pushad;
		push 2;
		jmp RemoveKnockback;
	}
}

void __declspec(naked) op_get_kill_counter2() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz fail;
		cmp eax, 38;
		jge fail;
		movzx edx, word ptr ds:[FO_VAR_pc_kill_counts+eax*2];
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ecx
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_mod_kill_counter2() {
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
		cmp eax, 38;
		jge end;
		add word ptr ds:[FO_VAR_pc_kill_counts+eax*2], di;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_active_hand() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, dword ptr ds:[FO_VAR_itemCurrentItem];
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_toggle_active_hand() {
	__asm {
		mov eax, 1;
		call fo::funcoffs::intface_toggle_items_;
		retn;
	}
}

void __declspec(naked) op_eax_available() {
	__asm {
		push ebx;
		push eax;
		push edx;
		push edi;
		mov edi, eax;
		xor edx, edx
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call fo::funcoffs::interpretPushShort_;
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

		//SafeMemSet(0x495C50, 0x90, 6);
		SafeMemSet(0x495C77, 0x90, 6);     //Check that the player is high enough for the npc to consider this level
		//SafeMemSet(0x495C8C, 0x90, 6);   //Check that the npc isn't already at its maximum level
		SafeMemSet(0x495CEC, 0x90, 6);     //Check that the npc hasn't already levelled up recently
		if (!npcAutoLevelEnabled) {
			SafeMemSet(0x495D22, 0x90, 6);  //Random element
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
		call fo::funcoffs::partyMemberIncLevels_;
	}
	SafeWrite16(0x495C50, 0x840F);
	SafeWrite32(0x495C52, 0x000001FB);
	SafeWrite16(0x495C77, 0x8C0F);
	SafeWrite32(0x495C79, 0x000001D4);
	//SafeWrite16(0x495C8C, 0x8D0F);
	//SafeWrite32(0x495C8E, 0x000001BF);
	SafeWrite16(0x495CEC, 0x850F);
	SafeWrite32(0x495CEE, 0x00000130);
	if (!npcAutoLevelEnabled) {
		SafeWrite16(0x495D22, 0x8F0F);
		SafeWrite32(0x495D24, 0x00000129);
	}
}

void __declspec(naked) op_inc_npc_level() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
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
			call fo::funcoffs::critter_name_
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

void __declspec(naked) op_get_npc_level() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz fail;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push eax;
		push ds:[FO_VAR_partyMemberLevelUpInfoList];
		push ds:[FO_VAR_partyMemberPidList];
		push ds:[FO_VAR_partyMemberList];
		push ds:[FO_VAR_partyMemberMaxCount];
		push ds:[FO_VAR_partyMemberCount];
		call get_npc_level2;
		mov edx, eax;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static int ParseIniSetting(const char* iniString, const char* &key, char section[], char file[]) {
	key = strstr(iniString, "|");
	if (!key) return -1;

	DWORD filelen = (DWORD)key - (DWORD)iniString;
	if (filelen >= 64) return -1;

	key = strstr(key + 1, "|");
	if (!key) return -1;

	DWORD seclen = (DWORD)key - ((DWORD)iniString + filelen + 1);
	if (seclen > 32) return -1;

	file[0] = '.';
	file[1] = '\\';
	memcpy(&file[2], iniString, filelen);
	file[filelen + 2] = 0;

	memcpy(section, &iniString[filelen + 1], seclen);
	section[seclen] = 0;

	key++;
	return 1;
}

static char IniStrBuffer[128];
static DWORD _stdcall GetIniSetting2(const char* c, DWORD string) {
	const char* key;
	char section[33], file[67];
	
	if (ParseIniSetting(c, key, section, file) < 0) {
		return -1;
	}
	if (string) {
		IniStrBuffer[0] = 0;
		GetPrivateProfileStringA(section, key, "", IniStrBuffer, 128, file);
		return (DWORD)&IniStrBuffer[0];
	} else {
		return GetPrivateProfileIntA(section, key, -1, file);
	}
}

void __declspec(naked) op_get_ini_setting() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push 0;
		push eax;
		call GetIniSetting2;
		mov edx, eax;
		jmp result;
error:
		mov edx, -1;
result:
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call fo::funcoffs::interpretPushShort_;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_ini_string() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push edi;
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz error;
next:
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push 1;
		push eax;
		call GetIniSetting2;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretAddString_;
		mov edx, eax;
		mov ebx, VAR_TYPE_STR;
		jmp result;
error:
		xor edx, edx;
		mov ebx, VAR_TYPE_INT
result:
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov edx, ebx;
		mov eax, edi;
		call fo::funcoffs::interpretPushShort_;
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

void __declspec(naked) op_get_uptime() {
	__asm {
		pushad;
		mov edi, eax;
		call GetTickCount2;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_car_current_town() {
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
		mov ds:[FO_VAR_carCurrentArea], eax;
end:
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_hp_per_level_mod() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_get_bodypart_hit_modifier() {
	__asm {
		push ecx
		push edx
		mov  ecx, eax
		call fo::funcoffs::interpretPopShort_
		mov  edx, eax
		mov  eax, ecx
		call fo::funcoffs::interpretPopLong_
		cmp  dx, VAR_TYPE_INT
		jnz  fail
		cmp  eax, 8                               // Body_Uncalled?
		jg   fail
		test eax, eax
		jl   fail
		mov  edx, ds:[FO_VAR_hit_location_penalty+eax*4]
		jmp  end
fail:
		xor  edx, edx
end:
		mov  eax, ecx
		call fo::funcoffs::interpretPushLong_
		mov  eax, ecx
		mov  edx, VAR_TYPE_INT
		call fo::funcoffs::interpretPushShort_
		pop  edx
		pop  ecx
		retn
	}
}

void __declspec(naked) op_set_bodypart_hit_modifier() {
	__asm {
		push ebx
		push ecx
		push edx
		mov  ecx, eax
		call fo::funcoffs::interpretPopShort_
		mov  edx, eax
		mov  eax, ecx
		call fo::funcoffs::interpretPopLong_
		mov  ebx, eax
		mov  eax, ecx
		call fo::funcoffs::interpretPopShort_
		xchg eax, ecx
		call fo::funcoffs::interpretPopLong_
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
		mov  ds:[FO_VAR_hit_location_penalty+eax*4], ebx
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

void __declspec(naked) op_set_critical_table() {
	__asm {
		pushad;
		mov ecx, eax;
		xor ebx, ebx;
		mov edx, 5;
loops:
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		jz skip1;
		inc ebx;
skip1:
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_get_critical_table() {
	__asm {
		pushad;
		mov edi, eax;
		xor ebx, ebx;
		mov dl, 4;
loops:
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		jz skip1;
		inc ebx;
skip1:
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
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
		call fo::funcoffs::interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_reset_critical_table() {
	__asm {
		pushad;
		mov ecx, eax;
		xor ebx, ebx;
		mov dl, 4;
loops:
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		cmp ax, VAR_TYPE_INT;
		jz skip1;
		inc ebx;
skip1:
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_set_unspent_ap_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov standardApAcBonus, ax;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_unspent_ap_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		movzx edx, standardApAcBonus;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_set_unspent_ap_perk_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		mov extraApAcBonus, ax;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_unspent_ap_perk_bonus() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		movzx edx, extraApAcBonus;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_set_palette() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		mov ebx, eax;
		//mov eax, _black_palette;
		//call fo::funcoffs::palette_set_to_;
		mov eax, ecx;
		call fo::funcoffs::interpretGetString_;
		call fo::funcoffs::loadColorTable_;
		mov eax, FO_VAR_cmap;
		call fo::funcoffs::palette_set_to_;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

//numbers subgame functions
void __declspec(naked) op_nb_create_char() {
	__asm {
		/*pushad;
		push eax;
		call NumbersCreateChar;
		mov edx, eax;
		pop eax;
		mov ecx, eax;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;*/
		retn;
	}
}

void __declspec(naked) op_get_proto_data() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		cmp si, VAR_TYPE_INT;
		jnz fail;
		mov edx, esp;
		call fo::funcoffs::proto_ptr_;
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
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		add esp, 4;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_proto_data() {
	__asm {
		pushad;
		sub esp, 4;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		mov ebx, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopShort_;
		xchg eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		cmp bp, VAR_TYPE_INT;
		jnz end;
		//mov eax, [eax+0x64];
		mov edx, esp;
		call fo::funcoffs::proto_ptr_;
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

void __declspec(naked) op_hero_select_win() {//for opening the appearance selection window
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_set_hero_style() {//for setting the hero style/appearance takes an 1 int
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_set_hero_race() {// for setting the hero race takes an 1 int
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_get_light_level() {
	__asm {
		pushad;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_ambient_light];
		call fo::funcoffs::interpretPushLong_
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call fo::funcoffs::interpretPushShort_
		popad;
		retn;
	}
}

void __declspec(naked) op_refresh_pc_art() {
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

void _stdcall intf_attack_type() {
	__asm {
		sub esp, 8;
		lea edx, [esp];
		lea eax, [esp+4];
		call fo::funcoffs::intface_get_attack_;
		pop edx; // is_secondary
		pop ecx; // hit_mode
	}
}

void __declspec(naked) op_get_attack_type() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call intf_attack_type;
		test eax, eax;
		jz skip;
		mov ecx, eax; // result = -1
skip:
		mov edx, ecx; // hit_mode
		pop ecx;
		mov eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}

void __declspec(naked) op_play_sfall_sound() {
	__asm {
		pushad
		mov edi, eax;
		call fo::funcoffs::interpretPopShort_;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		mov esi, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_STR2;
		jz next;
		cmp dx, VAR_TYPE_STR;
		jnz end;
next:
		cmp bx, VAR_TYPE_INT;
		jnz end;
		mov ebx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretGetString_;
		push esi;
		push eax;
		mov eax, esi;
		mov esi, 65;
		call PlaySfallSound;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov eax, edi;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
end:
		popad
		retn;
	}
}

void __declspec(naked) op_stop_sfall_sound() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		push eax;
		call StopSfallSound;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_get_tile_fid() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz fail;
		sub esp, 8;
		lea edx, [esp];
		lea ebx, [esp+4];
		call fo::funcoffs::tile_coord_;
		mov eax, [esp];
		mov edx, [esp+4];
		call fo::funcoffs::square_num_;
		mov edx, ds:[FO_VAR_square];
		movzx edx, word ptr ds:[edx+eax*4];
		add esp, 8;
		jmp end;
fail:
		xor edx, edx;
end:
		mov eax, ebp;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_modified_ini() {
	__asm {
		pushad;
		mov edx, modifiedIni;
		mov ebp, eax;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_force_aimed_shots() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_disable_aimed_shots() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void __declspec(naked) op_mark_movie_played() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		test eax, eax;
		jl end;
		cmp eax, 0x11;
		jge end;
		mov byte ptr ds:[eax + FO_VAR_gmovie_played_list], 1;
end:
		pop edx;
		pop ecx;
		retn;
	}
}

void __declspec(naked) op_get_last_attacker() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
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
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_get_last_target() {
	__asm {
		pushad;
		mov ebp, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call fo::funcoffs::interpretPopLong_;
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
		call fo::funcoffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_block_combat() {
	__asm {
		pushad;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call AIBlockCombat;
end:
		popad
		retn;
	}
}

void __declspec(naked) op_tile_under_cursor() {
	__asm {
		push edx;
		push ecx;
		push ebx;
		mov ecx, eax;
		sub esp, 8;
		lea edx, [esp];
		lea eax, [esp+4];
		call fo::funcoffs::mouse_get_position_;
		mov ebx, dword ptr ds:[FO_VAR_map_elevation];
		mov edx, [esp];
		mov eax, [esp+4];
		call fo::funcoffs::tile_num_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		add esp, 8;
		pop ebx;
		pop ecx;
		pop edx;
		retn;
	}
}

void __declspec(naked) op_gdialog_get_barter_mod() {
	__asm {
		push edx;
		push ecx;
		mov ecx, eax;
		mov edx, dword ptr ds:[FO_VAR_gdBarterMod];
		call fo::funcoffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call fo::funcoffs::interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}

void __declspec(naked) op_set_inven_ap_cost() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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

void sf_attack_is_aimed(OpcodeContext& ctx) {
	int is_secondary, result;
	__asm {
		call intf_attack_type;
		mov result, eax;
		mov is_secondary, edx;
	}
	ctx.setReturn((result != -1) ? is_secondary : -1);
}

void sf_sneak_success(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::is_pc_sneak_working());
}

void sf_tile_light(OpcodeContext& ctx) {
	int lightLevel = fo::func::light_get_tile(ctx.arg(0).asInt(), ctx.arg(1).asInt());
	ctx.setReturn(lightLevel);
}

void sf_exec_map_update_scripts(OpcodeContext& ctx) {
	__asm call fo::funcoffs::scr_exec_map_update_scripts_
}

void sf_set_ini_setting(OpcodeContext& ctx) {
	const char* iniString = ctx.arg(0).asString();
	const ScriptValue &argVal = ctx.arg(1);

	if (argVal.isInt()) {
		_itoa_s(argVal.asInt(), IniStrBuffer, 10);
	} else {
		strcpy_s(IniStrBuffer, argVal.asString());
	}

	const char* key;
	char section[33], file[67];
	int result = ParseIniSetting(iniString, key, section, file);
	if (result > 0) {
		result = WritePrivateProfileString(section, key, IniStrBuffer, file);
	}

	switch (result) {
	case 0:
		ctx.printOpcodeError("set_ini_setting() - value save error.");
		break;
	case -1:
		ctx.printOpcodeError("set_ini_setting() - invalid setting argument.");
		break;
	}
}

char getIniSectionBuf[512];
void sf_get_ini_sections(OpcodeContext& ctx) {
	auto fileName = std::string(".\\") + ctx.arg(0).asString();
	GetPrivateProfileSectionNamesA(getIniSectionBuf, 512, fileName.data());
	std::vector<char*> sections;
	char* section = getIniSectionBuf;
	while (*section != 0) {
		sections.push_back(section);
		section += std::strlen(section) + 1;
	}
	int arrayId = TempArray(sections.size(), 0);
	auto& arr = arrays[arrayId];

	for (size_t i = 0; i < sections.size(); ++i) {
		arr.val[i].set(sections[i]);
	}
	ctx.setReturn(arrayId);
}

void sf_get_ini_section(OpcodeContext& ctx) {
	auto fileName = std::string(".\\") + ctx.arg(0).asString();
	auto section = ctx.arg(1).asString();
	GetPrivateProfileSectionA(section, getIniSectionBuf, 512, fileName.data());
	int arrayId = TempArray(-1, 0); // associative
	auto& arr = arrays[arrayId];
	char *key = getIniSectionBuf, *val = nullptr;
	while (*key != 0) {
		char* val = std::strpbrk(key, "=");
		if (val != nullptr) {
			*val = '\0';
			val += 1;

			SetArray(arrayId, ScriptValue(key), ScriptValue(val), false);

			key = val + std::strlen(val) + 1;
		} else {
			key += std::strlen(key) + 1;
		}
	}
	ctx.setReturn(arrayId);
}

}
}
