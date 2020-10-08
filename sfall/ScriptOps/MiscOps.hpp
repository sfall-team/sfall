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
#include "Combat.h"
#include "Criticals.h"
#include "HeroAppearance.h"
#include "Inventory.h"
#include "KillCounter.h"
#include "Movies.h"
#include "PartyControl.h"
#include "PlayerModel.h"
#include "ScriptExtender.h"
#include "Sound.h"
#include "Stats.h"

/*
 *	Misc operators
 */

const char* stringTooLong = "%s() - the string exceeds maximum length of 64 characters.";

static void __stdcall op_set_dm_model2() {
	const ScriptValue &modelArg = opHandler.arg(0);

	if (modelArg.isString()) {
		const char* model = modelArg.strValue();
		if (strlen(model) > 64) {
			opHandler.printOpcodeError(stringTooLong, "set_dm_model");
			return;
		}
		strcpy(defaultMaleModelName, model);
	} else {
		OpcodeInvalidArgs("set_dm_model");
	}
}

static void __declspec(naked) op_set_dm_model() {
	_WRAP_OPCODE(op_set_dm_model2, 1, 0)
}

static void __stdcall op_set_df_model2() {
	const ScriptValue &modelArg = opHandler.arg(0);

	if (modelArg.isString()) {
		const char* model = modelArg.strValue();
		if (strlen(model) > 64) {
			opHandler.printOpcodeError(stringTooLong, "set_df_model");
			return;
		}
		strcpy(defaultFemaleModelName, model);
	} else {
		OpcodeInvalidArgs("set_df_model");
	}
}

static void __declspec(naked) op_set_df_model() {
	_WRAP_OPCODE(op_set_df_model2, 1, 0)
}

static void __stdcall op_set_movie_path2() {
	const ScriptValue &fileNameArg = opHandler.arg(0),
					  &movieIdArg = opHandler.arg(1);

	if (fileNameArg.isString() && movieIdArg.isInt()) {
		long movieID = movieIdArg.rawValue();
		if (movieID < 0 || movieID >= MaxMovies) return;
		const char* fileName = fileNameArg.strValue();
		if (strlen(fileName) > 64) {
			opHandler.printOpcodeError(stringTooLong, "set_movie_path");
			return;
		}
		strcpy(&MoviePaths[movieID * 65], fileName);
	} else {
		OpcodeInvalidArgs("set_movie_path");
	}
}

static void __declspec(naked) op_set_movie_path() {
	_WRAP_OPCODE(op_set_movie_path2, 2, 0)
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
		call game_time_date_;
		mov edx, [esp];
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

static void __declspec(naked) op_game_loaded() {
	__asm {
		mov  esi, ecx;
		push eax; // script
		call ScriptHasLoaded;
		movzx edx, al;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_pipboy_available() {
	__asm {
		_GET_ARG_INT(end);
		cmp  eax, 0;
		jl   end;
		cmp  eax, 1;
		jg   end;
		mov  byte ptr ds:[_gmovie_played_list][0x3], al;
end:
		retn;
	}
}

// Kill counters
static bool extraKillCounter;
static void SetExtraKillCounter(bool value) { extraKillCounter = value; }

static void __declspec(naked) op_get_kill_counter() {
	__asm {
		_GET_ARG_INT(fail); // get kill type value
		cmp  extraKillCounter, 1;
		jne  skip;
		cmp  eax, 38;
		jae  fail;
		movzx edx, word ptr ds:[_pc_kill_counts][eax * 2];
		jmp  end;
skip:
		cmp  eax, 19;
		jae  fail;
		mov  edx, ds:[_pc_kill_counts][eax * 4];
end:
		mov  eax, ebx; // script
		_RET_VAL_INT;
		retn;
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_mod_kill_counter() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi); // get mod value
		mov  eax, ebx;
		_GET_ARG_INT(end);  // get kill type value
		cmp  si, VAR_TYPE_INT;
		jnz  end;
		cmp  extraKillCounter, 1;
		je   skip;
		cmp  eax, 19;
		jae  end;
		add  ds:[_pc_kill_counts][eax * 4], ecx;
		pop  ecx;
		retn;
skip:
		cmp  eax, 38;
		jae  end;
		add  word ptr ds:[_pc_kill_counts][eax * 2], cx;
end:
		pop  ecx;
		retn;
	}
}

//Knockback
static void __declspec(naked) SetKnockback() {
	__asm {
		sub esp, 0xC;
		mov ecx, eax;
		//Get args
		call interpretPopShort_; //First arg type
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;  //First arg
		mov [esp + 8], eax;
		mov eax, ecx;
		call interpretPopShort_; //Second arg type
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;  //Second arg
		mov [esp + 4], eax;
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
		fild [esp + 8];
		fstp [esp + 8];
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
		popaop;
		retn;
	}
}

static void __declspec(naked) op_set_weapon_knockback() {
	__asm {
		pushaop;
		push 0;
		jmp SetKnockback;
	}
}

static void __declspec(naked) op_set_target_knockback() {
	__asm {
		pushaop;
		push 1;
		jmp SetKnockback;
	}
}

static void __declspec(naked) op_set_attacker_knockback() {
	__asm {
		pushaop;
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
		popaop;
		retn;
	}
}

static void __declspec(naked) op_remove_weapon_knockback() {
	__asm {
		pushaop;
		push 0;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_remove_target_knockback() {
	__asm {
		pushaop;
		push 1;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_remove_attacker_knockback() {
	__asm {
		pushaop;
		push 2;
		jmp RemoveKnockback;
	}
}

static void __declspec(naked) op_active_hand() {
	__asm {
		mov  edx, dword ptr ds:[_itemCurrentItem];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_toggle_active_hand() {
	__asm {
		mov eax, 1;
		jmp intface_toggle_items_;
	}
}

static void __declspec(naked) op_eax_available() {
	__asm {
		xor  edx, edx
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static const char* nameNPCToInc;
static long pidNPCToInc;
static bool onceNpcLoop;

static void __cdecl IncNPCLevel(const char* fmt, const char* name) {
	TGameObj* mObj;
	__asm {
		push edx;
		mov  eax, [ebp + 0x150 - 0x1C + 16]; // ebp <- esp
		mov  edx, [eax];
		mov  mObj, edx;
	}

	if ((pidNPCToInc && (mObj && mObj->protoId == pidNPCToInc)) || (!pidNPCToInc && !_stricmp(name, nameNPCToInc))) {
		DebugPrintf(fmt, name);

		SafeWrite32(0x495C50, 0x01FB840F); // Want to keep this check intact. (restore)

		SafeMemSet(0x495C77, CODETYPE_Nop, 6);   // Check that the player is high enough for the npc to consider this level
		//SafeMemSet(0x495C8C, CODETYPE_Nop, 6); // Check that the npc isn't already at its maximum level
		SafeMemSet(0x495CEC, CODETYPE_Nop, 6);   // Check that the npc hasn't already levelled up recently
		if (!npcAutoLevelEnabled) {
			SafeWrite8(0x495CFB, CODETYPE_JumpShort); // Disable random element
		}
		__asm mov [ebp + 0x150 - 0x28 + 16], 255; // set counter for exit loop
	} else {
		if (!onceNpcLoop) {
			SafeWrite32(0x495C50, 0x01FCE9); // set goto next member
			onceNpcLoop = true;
		}
	}
	__asm pop edx;
}

static void __stdcall op_inc_npc_level2() {
	nameNPCToInc = opHandler.arg(0).asString();
	pidNPCToInc = opHandler.arg(0).asInt(); // set to 0 if passing npc name
	if (pidNPCToInc == 0 && nameNPCToInc[0] == 0) return;

	MakeCall(0x495BF1, IncNPCLevel);  // Replace the debug output
	__asm call partyMemberIncLevels_;
	onceNpcLoop = false;

	// restore code
	SafeWrite32(0x495C50, 0x01FB840F);
	__int64 data = 0x01D48C0F;
	SafeWriteBytes(0x495C77, (BYTE*)&data, 6);
	//SafeWrite16(0x495C8C, 0x8D0F);
	//SafeWrite32(0x495C8E, 0x000001BF);
	data = 0x0130850F;
	SafeWriteBytes(0x495CEC, (BYTE*)&data, 6);
	if (!npcAutoLevelEnabled) {
		SafeWrite8(0x495CFB, CODETYPE_JumpZ);
	}
}

static void __declspec(naked) op_inc_npc_level() {
	_WRAP_OPCODE(op_inc_npc_level2, 1, 0)
}

static void __stdcall op_get_npc_level2() {
	int level = -1;
	const ScriptValue &npcArg = opHandler.arg(0);

	if (!npcArg.isFloat()) {
		DWORD findPid = npcArg.asInt(); // set to 0 if passing npc name
		const char *critterName, *name = npcArg.asString();

		if (findPid || name[0] != 0) {
			DWORD pid = 0;
			DWORD* members = *ptr_partyMemberList;
			for (DWORD i = 0; i < *ptr_partyMemberCount; i++) {
				if (!findPid) {
					__asm {
						mov  eax, members;
						mov  eax, [eax];
						call critter_name_;
						mov  critterName, eax;
					}
					if (!_stricmp(name, critterName)) { // found npc
						pid = ((TGameObj*)*members)->protoId;
						break;
					}
				} else {
					DWORD _pid = ((TGameObj*)*members)->protoId;
					if (findPid == _pid) {
						pid = _pid;
						break;
					}
				}
				members += 4;
			}
			if (pid) {
				DWORD* pids = *ptr_partyMemberPidList;
				DWORD* lvlUpInfo = *ptr_partyMemberLevelUpInfoList;
				for (DWORD j = 0; j < *ptr_partyMemberMaxCount; j++) {
					if (pids[j] == pid) {
						level = lvlUpInfo[j * 3];
						break;
					}
				}
			}
		}
	} else {
		OpcodeInvalidArgs("get_npc_level");
	}
	opHandler.setReturn(level);
}

static void __declspec(naked) op_get_npc_level() {
	_WRAP_OPCODE(op_get_npc_level2, 1, 1)
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
	file[2 + filelen] = 0;

	memcpy(section, &iniString[filelen + 1], seclen);
	section[seclen] = 0;

	key++;
	return 1;
}

static DWORD __stdcall GetIniSetting(const char* str, DWORD isString) {
	const char* key;
	char section[33], file[67];

	if (ParseIniSetting(str, key, section, file) < 0) {
		return -1;
	}
	if (isString) {
		gTextBuffer[0] = 0;
		iniGetString(section, key, "", gTextBuffer, 256, file);
		return (DWORD)&gTextBuffer[0];
	} else {
		return iniGetInt(section, key, -1, file);
	}
}

static void __declspec(naked) op_get_ini_setting() {
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
		call GetIniSetting;
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

static void __declspec(naked) op_get_ini_string() {
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
		call GetIniSetting;
		mov edx, eax;
		mov eax, edi;
		call interpretAddString_;
		mov edx, eax;
		mov ebx, VAR_TYPE_STR;
		jmp result;
error:
		xor edx, edx;
		dec edx;
		mov ebx, VAR_TYPE_INT;
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

static DWORD __stdcall GetTickCount2() {
	return GetTickCount();
}

static void __declspec(naked) op_get_uptime() {
	__asm {
		mov  esi, ecx;
		call GetTickCount2;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_car_current_town() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[_carCurrentArea], eax;
end:
		retn;
	}
}

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

static void __declspec(naked) op_get_bodypart_hit_modifier() {
	__asm {
		_GET_ARG_INT(fail); // get body value
		cmp  eax, 8; // Body_Head - Body_Uncalled
		ja   fail;
		mov  edx, ds:[_hit_location_penalty][eax * 4];
end:
		mov  eax, ebx; // script
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_set_bodypart_hit_modifier() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi); // get body value
		mov  eax, ebx;
		_GET_ARG_INT(end);  // get modifier value
		cmp  si, VAR_TYPE_INT;
		jnz  end;
		cmp  eax, 8; // Body_Head - Body_Uncalled
		ja   end;
		mov  ds:[_hit_location_penalty][eax * 4], ecx;
end:
		pop  ecx;
		retn;
	}
}

static const char* valueOutRange = "%s() - argument values out of range.";

static void op_set_critical_table2() {
	const ScriptValue &critterArg = opHandler.arg(0),
					  &bodypartArg = opHandler.arg(1),
					  &slotArg = opHandler.arg(2),
					  &elementArg = opHandler.arg(3),
					  &valueArg = opHandler.arg(4);

	if (critterArg.isInt() && bodypartArg.isInt() && slotArg.isInt() && elementArg.isInt() && valueArg.isInt()) {
		DWORD critter = critterArg.rawValue(),
			bodypart  = bodypartArg.rawValue(),
			slot      = slotArg.rawValue(),
			element   = elementArg.rawValue();

		if (critter >= CritTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
			opHandler.printOpcodeError(valueOutRange, "set_critical_table");
		} else {
			SetCriticalTable(critter, bodypart, slot, element, valueArg.rawValue());
		}
	} else {
		OpcodeInvalidArgs("set_critical_table");
	}
}

static void __declspec(naked) op_set_critical_table() {
	_WRAP_OPCODE(op_set_critical_table2, 5, 0)
}

static void op_get_critical_table2() {
	const ScriptValue &critterArg = opHandler.arg(0),
					  &bodypartArg = opHandler.arg(1),
					  &slotArg = opHandler.arg(2),
					  &elementArg = opHandler.arg(3);

	if (critterArg.isInt() && bodypartArg.isInt() && slotArg.isInt() && elementArg.isInt()) {
		DWORD critter = critterArg.rawValue(),
			bodypart  = bodypartArg.rawValue(),
			slot      = slotArg.rawValue(),
			element   = elementArg.rawValue();

		if (critter >= CritTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
			opHandler.printOpcodeError(valueOutRange, "get_critical_table");
		} else {
			opHandler.setReturn(GetCriticalTable(critter, bodypart, slot, element));
		}
	} else {
		OpcodeInvalidArgs("get_critical_table");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_critical_table() {
	_WRAP_OPCODE(op_get_critical_table2, 4, 1)
}

static void op_reset_critical_table2() {
	const ScriptValue &critterArg = opHandler.arg(0),
					  &bodypartArg = opHandler.arg(1),
					  &slotArg = opHandler.arg(2),
					  &elementArg = opHandler.arg(3);

	if (critterArg.isInt() && bodypartArg.isInt() && slotArg.isInt() && elementArg.isInt()) {
		DWORD critter = critterArg.rawValue(),
			bodypart  = bodypartArg.rawValue(),
			slot      = slotArg.rawValue(),
			element   = elementArg.rawValue();

		if (critter >= CritTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
			opHandler.printOpcodeError(valueOutRange, "reset_critical_table");
		} else {
			ResetCriticalTable(critter, bodypart, slot, element);
		}
	} else {
		OpcodeInvalidArgs("reset_critical_table");
	}
}

static void __declspec(naked) op_reset_critical_table() {
	_WRAP_OPCODE(op_reset_critical_table2, 4, 0)
}

static void __declspec(naked) op_set_unspent_ap_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  StandardApAcBonus, eax;
end:
		retn;
	}
}

static void __declspec(naked) op_get_unspent_ap_bonus() {
	__asm {
		mov  edx, StandardApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_set_unspent_ap_perk_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  ExtraApAcBonus, eax;
end:
		retn;
	}
}

static void __declspec(naked) op_get_unspent_ap_perk_bonus() {
	__asm {
		mov  edx, ExtraApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_set_palette() {
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
		mov eax, ecx;
		call interpretGetString_;
		call loadColorTable_;
		mov eax, _cmap;
		call palette_set_to_;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

//numbers subgame functions
static void __declspec(naked) op_nb_create_char() {
	__asm retn;
}

static void __declspec(naked) op_hero_select_win() { // for opening the appearance selection window
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(fail);
		push eax;
		call HeroSelectWindow;
fail:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_hero_style() { // for setting the hero style/appearance takes an 1 int
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(fail);
		push eax;
		call SetHeroStyle;
fail:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_set_hero_race() { // for setting the hero race takes an 1 int
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(fail);
		push eax;
		call SetHeroRace;
fail:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_get_light_level() {
	__asm {
		mov  edx, ds:[_ambient_light];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_refresh_pc_art() {
	__asm {
		mov  esi, ecx;
		call RefreshPCArt;
		mov  ecx, esi;
		retn;
	}
}

static void __stdcall intface_attack_type() {
	__asm {
		sub esp, 8;
		lea edx, [esp];
		lea eax, [esp + 4];
		call intface_get_attack_;
		pop edx; // is_secondary
		pop ecx; // hit_mode
	}
}

static void __declspec(naked) op_get_attack_type() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call intface_attack_type;
		mov edx, ecx; // hit_mode
		test eax, eax;
		jz skip;
		// get reload
		cmp ds:[_interfaceWindow], eax;
		jz end;
		mov ecx, ds:[_itemCurrentItem];         // 0 - left, 1 - right
		imul edx, ecx, 0x18;
		cmp ds:[_itemButtonItems + 5 + edx], 1; // .itsWeapon
		jnz end;
		lea eax, [ecx + 6];
end:
		mov edx, eax; // result
skip:
		pop ecx;
		mov eax, ecx;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop ecx;
		pop edx;
		retn;
	}
}

static void __stdcall op_play_sfall_sound2() {
	const ScriptValue &fileArg = opHandler.arg(0),
					  &modeArg = opHandler.arg(1);

	if (fileArg.isString() && modeArg.isInt()) {
		DWORD soundID = 0;
		long mode = modeArg.rawValue();
		if (mode >= 0) soundID = PlaySfallSound(fileArg.strValue(), mode);
		opHandler.setReturn(soundID);
	} else {
		OpcodeInvalidArgs("play_sfall_sound");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_play_sfall_sound() {
	_WRAP_OPCODE(op_play_sfall_sound2, 2, 1)
}

static void __declspec(naked) op_stop_sfall_sound() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call StopSfallSound;
end:
		mov  ecx, esi;
		retn;
	}
}

// TODO: It seems that this function does not work...
static void __declspec(naked) op_get_tile_fid() {
	__asm {
		push ecx;
		_GET_ARG_INT(fail); // get tile value
		mov  esi, ebx;      // keep script
		sub  esp, 8;        // x/y buf
		lea  edx, [esp];
		lea  ebx, [esp + 4];
		call tile_coord_;
		pop  eax; // x
		pop  edx; // y
		call square_num_;
		mov  edx, ds:[_square];
		movzx edx, word ptr ds:[edx + eax * 4];
		mov  ebx, esi; // script
end:
		mov  eax, ebx;
		_RET_VAL_INT;
		pop  ecx;
		retn;
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_modified_ini() {
	__asm {
		mov  edx, modifiedIni;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_force_aimed_shots() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call ForceAimedShots;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_disable_aimed_shots() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call DisableAimedShots;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_mark_movie_played() {
	__asm {
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		cmp  eax, 17;
		jge  end;
		mov  byte ptr ds:[eax + _gmovie_played_list], 1;
end:
		retn;
	}
}

static void __declspec(naked) op_get_last_attacker() {
	__asm {
		_GET_ARG_INT(fail);
		mov  esi, ecx;
		push eax;
		call AIGetLastAttacker;
		mov  edx, eax;
		mov  ecx, esi;
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_get_last_target() {
	__asm {
		_GET_ARG_INT(fail);
		mov  esi, ecx;
		push eax;
		call AIGetLastTarget;
		mov  edx, eax;
		mov  ecx, esi;
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __declspec(naked) op_block_combat() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call SetBlockCombat;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_tile_under_cursor() {
	__asm {
		mov  esi, ebx;
		sub  esp, 8;
		lea  edx, [esp];
		lea  eax, [esp + 4];
		call mouse_get_position_;
		pop  edx;
		pop  eax;
		call tile_num_; // ebx - unused
		mov  edx, eax; // tile
		mov  ebx, esi;
		mov  eax, esi;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_gdialog_get_barter_mod() {
	__asm {
		mov  edx, dword ptr ds:[_gdBarterMod];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_set_inven_ap_cost() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, eax;
		call SetInvenApCost;
end:
		mov  ecx, esi;
		retn;
	}
}

static void mf_get_inven_ap_cost() {
	opHandler.setReturn(GetInvenApCost());
}

static void mf_attack_is_aimed() {
	DWORD isAimed, unused;
	opHandler.setReturn(!IntfaceGetAttack(&unused, &isAimed) ? isAimed : 0);
}

static void __declspec(naked) op_sneak_success() {
	__asm {
		call is_pc_sneak_working_;
		mov  edx, eax;
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __stdcall op_tile_light2() {
	const ScriptValue &elevArg = opHandler.arg(0),
					  &tileArg = opHandler.arg(1);

	if (elevArg.isInt() && tileArg.isInt()) {
		int lightLevel = LightGetTile(elevArg.rawValue(), tileArg.rawValue());
		opHandler.setReturn(lightLevel);
	} else {
		OpcodeInvalidArgs("tile_light");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_tile_light() {
	_WRAP_OPCODE(op_tile_light2, 2, 1)
}

static void mf_exec_map_update_scripts() {
	__asm call scr_exec_map_update_scripts_
}

static void mf_set_ini_setting() {
	const ScriptValue &argVal = opHandler.arg(1);

	const char* saveValue;
	if (argVal.isInt()) {
		_itoa_s(argVal.rawValue(), gTextBuffer, 10);
		saveValue = gTextBuffer;
	} else {
		saveValue = argVal.strValue();
	}
	const char* key;
	char section[33], file[67];
	int result = ParseIniSetting(opHandler.arg(0).strValue(), key, section, file);
	if (result > 0) {
		result = WritePrivateProfileStringA(section, key, saveValue, file);
	}

	switch (result) {
	case 0:
		opHandler.printOpcodeError("set_ini_setting() - value save error.");
		break;
	case -1:
		opHandler.printOpcodeError("set_ini_setting() - invalid setting argument.");
		break;
	default:
		return;
	}
	opHandler.setReturn(-1);
}

static void mf_npc_engine_level_up() {
	if (opHandler.arg(0).asBool()) {
		if (!npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0x840F); // enable
		npcEngineLevelUp = true;
	} else {
		if (npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0xE990);
		npcEngineLevelUp = false;
	}
}

static void mf_combat_data() {
	TComputeAttack* ctd = nullptr;
	if (*ptr_combat_state & 1) {
		ctd = ptr_main_ctd;
	}
	opHandler.setReturn((DWORD)ctd, DATATYPE_INT);
}
