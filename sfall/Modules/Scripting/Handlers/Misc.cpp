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

#include "..\..\..\Utils.h"
#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\AI.h"
#include "..\..\Combat.h"
#include "..\..\Criticals.h"
#include "..\..\HeroAppearance.h"
#include "..\..\Inventory.h"
#include "..\..\KillCounter.h"
//#include "..\..\MiscPatches.h"
#include "..\..\Movies.h"
#include "..\..\PartyControl.h"
#include "..\..\PlayerModel.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Stats.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "..\..\SubModules\CombatBlock.h"

#include "Misc.h"

namespace sfall
{
namespace script
{

const char* stringTooLong = "%s() - the string exceeds maximum length of 64 characters.";

void sf_set_dm_model(OpcodeContext& ctx) {
	auto model = ctx.arg(0).strValue();
	if (strlen(model) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(defaultMaleModelName, model);
}

void sf_set_df_model(OpcodeContext& ctx) {
	auto model = ctx.arg(0).strValue();
	if (strlen(model) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(defaultFemaleModelName, model);
}

void sf_set_movie_path(OpcodeContext& ctx) {
	long movieID = ctx.arg(1).rawValue();
	if (movieID < 0 || movieID >= MaxMovies) return;
	auto fileName = ctx.arg(0).strValue();
	if (strlen(fileName) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(&MoviePaths[movieID * 65], fileName);
}

void sf_get_year(OpcodeContext& ctx) {
	int year = 0;
	__asm {
		xor eax, eax;
		xor edx, edx;
		lea ebx, year;
		call fo::funcoffs::game_time_date_;
	}
	ctx.setReturn(year);
}

void __declspec(naked) op_game_loaded() {
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

void __declspec(naked) op_set_pipboy_available() {
	__asm {
		_GET_ARG_INT(end);
		cmp  eax, 0;
		jl   end;
		cmp  eax, 1;
		jg   end;
		mov  byte ptr ds:[FO_VAR_gmovie_played_list][0x3], al;
end:
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

void sf_set_object_knockback(OpcodeContext& ctx) {
	int mode = 0;
	switch (ctx.opcode()) {
	case 0x196:
		mode = 1;
		break;
	case 0x197:
		mode = 2;
		break;
	}
	fo::GameObject* object = ctx.arg(0).object();
	if (mode) {
		if (object->Type() != fo::OBJ_TYPE_CRITTER) {
			ctx.printOpcodeError("%s() - the object is not a critter.", ctx.getOpcodeName());
			return;
		}
	} else {
		if (object->Type() != fo::OBJ_TYPE_ITEM) {
			ctx.printOpcodeError("%s() - the object is not an item.", ctx.getOpcodeName());
			return;
		}
	}
	KnockbackSetMod(object, ctx.arg(1).rawValue(), ctx.arg(2).asFloat(), mode);
}

void sf_remove_object_knockback(OpcodeContext& ctx) {
	int mode = 0;
	switch (ctx.opcode()) {
	case 0x199:
		mode = 1;
		break;
	case 0x19a:
		mode = 2;
		break;
	}
	KnockbackRemoveMod(ctx.arg(0).object(), mode);
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
		mov  edx, dword ptr ds:[FO_VAR_itemCurrentItem];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

void __declspec(naked) op_toggle_active_hand() {
	__asm {
		mov eax, 1;
		jmp fo::funcoffs::intface_toggle_items_;
	}
}

void __declspec(naked) op_eax_available() {
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
	fo::GameObject* mObj;
	__asm {
		push edx;
		mov  eax, [ebp + 0x150 - 0x1C + 16]; // ebp <- esp
		mov  edx, [eax];
		mov  mObj, edx;
	}

	if ((pidNPCToInc && (mObj && mObj->protoId == pidNPCToInc)) || (!pidNPCToInc && !_stricmp(name, nameNPCToInc))) {
		fo::func::debug_printf(fmt, name);

		SafeWrite32(0x495C50, 0x01FB840F); // Want to keep this check intact. (restore)

		SafeMemSet(0x495C77, 0x90, 6);     // Check that the player is high enough for the npc to consider this level
		//SafeMemSet(0x495C8C, 0x90, 6);   // Check that the npc isn't already at its maximum level
		SafeMemSet(0x495CEC, 0x90, 6);     // Check that the npc hasn't already levelled up recently
		if (!npcAutoLevelEnabled) {
			SafeWrite8(0x495CFB, 0xEB);    // Disable random element
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

void sf_inc_npc_level(OpcodeContext& ctx) {
	nameNPCToInc = ctx.arg(0).asString();
	pidNPCToInc = ctx.arg(0).asInt(); // set to 0 if passing npc name
	if (pidNPCToInc == 0 && nameNPCToInc[0] == 0) return;

	MakeCall(0x495BF1, IncNPCLevel);  // Replace the debug output
	__asm call fo::funcoffs::partyMemberIncLevels_;
	onceNpcLoop = false;

	// restore code
	SafeWrite32(0x495C50, 0x01FB840F);
	long long data = 0x01D48C0F;
	SafeWriteBytes(0x495C77, (BYTE*)&data, 6);
	//SafeWrite16(0x495C8C, 0x8D0F);
	//SafeWrite32(0x495C8E, 0x000001BF);
	data = 0x0130850F;
	SafeWriteBytes(0x495CEC, (BYTE*)&data, 6);
	if (!npcAutoLevelEnabled) {
		SafeWrite8(0x495CFB, 0x74);
	}
}

void sf_get_npc_level(OpcodeContext& ctx) {
	int level = -1;
	DWORD findPid = ctx.arg(0).asInt(); // set to 0 if passing npc name
	const char *critterName, *name = ctx.arg(0).asString();

	if (findPid || name[0] != 0) {
		DWORD pid = 0;
		DWORD* members = fo::var::partyMemberList;
		for (DWORD i = 0; i < fo::var::partyMemberCount; i++) {
			if (!findPid) {
				__asm {
					mov  eax, members;
					mov  eax, [eax];
					call fo::funcoffs::critter_name_;
					mov  critterName, eax;
				}
				if (!_stricmp(name, critterName)) { // found npc
					pid = ((fo::GameObject*)*members)->protoId;
					break;
				}
			} else {
				DWORD _pid = ((fo::GameObject*)*members)->protoId;
				if (findPid == _pid) {
					pid = _pid;
					break;
				}
			}
			members += 4;
		}
		if (pid) {
			DWORD* pids = fo::var::partyMemberPidList;
			for (DWORD j = 0; j < fo::var::partyMemberMaxCount; j++) {
				if (pids[j] == pid) {
					level = fo::var::partyMemberLevelUpInfoList[j * 3];
					break;
				}
			}
		}
	}
	ctx.setReturn(level);
}

static bool IsSpecialIni(const char* str, const char* end) {
	const char* pos = strfind(str, &::sfall::ddrawIni[2]);
	if (pos && pos < end) return true;
	pos = strfind(str, "f2_res.ini");
	if (pos && pos < end) return true;
	return false;
}

static int ParseIniSetting(const char* iniString, const char* &key, char section[], char file[]) {
	key = strstr(iniString, "|");
	if (!key) return -1;

	DWORD filelen = (DWORD)key - (DWORD)iniString;
	if (ScriptExtender::iniConfigFolder.empty() && filelen >= 64) return -1;
	const char* fileEnd = key;

	key = strstr(key + 1, "|");
	if (!key) return -1;

	DWORD seclen = (DWORD)key - ((DWORD)iniString + filelen + 1);
	if (seclen > 32) return -1;

	file[0] = '.';
	file[1] = '\\';

	if (!ScriptExtender::iniConfigFolder.empty() && !IsSpecialIni(iniString, fileEnd)) {
		size_t len = ScriptExtender::iniConfigFolder.length(); // limit up to 62 characters
		memcpy(&file[2], ScriptExtender::iniConfigFolder.c_str(), len);
		memcpy(&file[2 + len], iniString, filelen); // copy path and file
		file[2 + len + filelen] = 0;
		if (GetFileAttributesA(file) & FILE_ATTRIBUTE_DIRECTORY) goto defRoot; // also file not found
	} else {
defRoot:
		memcpy(&file[2], iniString, filelen);
		file[2 + filelen] = 0;
	}
	memcpy(section, &iniString[filelen + 1], seclen);
	section[seclen] = 0;

	key++;
	return 1;
}

static DWORD GetIniSetting(const char* str, bool isString) {
	const char* key;
	char section[33], file[128];

	if (ParseIniSetting(str, key, section, file) < 0) {
		return -1;
	}
	if (isString) {
		ScriptExtender::gTextBuffer[0] = 0;
		iniGetString(section, key, "", ScriptExtender::gTextBuffer, 256, file);
		return (DWORD)&ScriptExtender::gTextBuffer[0];
	} else {
		return iniGetInt(section, key, -1, file);
	}
}

void sf_get_ini_setting(OpcodeContext& ctx) {
	ctx.setReturn(GetIniSetting(ctx.arg(0).strValue(), false));
}

void sf_get_ini_string(OpcodeContext& ctx) {
	DWORD result = GetIniSetting(ctx.arg(0).strValue(), true);
	ctx.setReturn(result, (result != -1) ? DataType::STR : DataType::INT);
}

void __declspec(naked) op_get_uptime() {
	__asm {
		mov  esi, ecx;
		call GetTickCount;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

void __declspec(naked) op_set_car_current_town() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_carCurrentArea], eax;
end:
		retn;
	}
}

void __declspec(naked) op_set_hp_per_level_mod() {
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
		test eax, eax
		jl   fail
		cmp  eax, 8                               // Body_Uncalled?
		jg   fail
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
		test eax, eax
		jl   end
		cmp  eax, 8                               // Body_Uncalled?
		jg   end
		mov  ds:[FO_VAR_hit_location_penalty+eax*4], ebx
end:
		pop  edx
		pop  ecx
		pop  ebx
		retn
	}
}

static const char* valueOutRange = "%s() - argument values out of range.";

void sf_set_critical_table(OpcodeContext& ctx) {
	DWORD critter = ctx.arg(0).rawValue(),
		bodypart  = ctx.arg(1).rawValue(),
		slot      = ctx.arg(2).rawValue(),
		element   = ctx.arg(3).rawValue();

	if (critter >= Criticals::critTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
		ctx.printOpcodeError(valueOutRange, ctx.getOpcodeName());
	} else {
		Criticals::SetCriticalTable(critter, bodypart, slot, element, ctx.arg(4).rawValue());
	}
}

void sf_get_critical_table(OpcodeContext& ctx) {
	DWORD critter = ctx.arg(0).rawValue(),
		bodypart  = ctx.arg(1).rawValue(),
		slot      = ctx.arg(2).rawValue(),
		element   = ctx.arg(3).rawValue();

	if (critter >= Criticals::critTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
		ctx.printOpcodeError(valueOutRange, ctx.getOpcodeName());
	} else {
		ctx.setReturn(Criticals::GetCriticalTable(critter, bodypart, slot, element));
	}
}

void sf_reset_critical_table(OpcodeContext& ctx) {
	DWORD critter = ctx.arg(0).rawValue(),
		bodypart  = ctx.arg(1).rawValue(),
		slot      = ctx.arg(2).rawValue(),
		element   = ctx.arg(3).rawValue();

	if (critter >= Criticals::critTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
		ctx.printOpcodeError(valueOutRange, ctx.getOpcodeName());
	} else {
		Criticals::ResetCriticalTable(critter, bodypart, slot, element);
	}
}

void __declspec(naked) op_set_unspent_ap_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  Stats::standardApAcBonus, eax;
end:
		retn;
	}
}

void __declspec(naked) op_get_unspent_ap_bonus() {
	__asm {
		mov  edx, Stats::standardApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

void __declspec(naked) op_set_unspent_ap_perk_bonus() {
	__asm {
		_GET_ARG_INT(end);
		mov  Stats::extraApAcBonus, eax;
end:
		retn;
	}
}

void __declspec(naked) op_get_unspent_ap_perk_bonus() {
	__asm {
		mov  edx, Stats::extraApAcBonus;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
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
		retn;
	}
}

void __declspec(naked) op_hero_select_win() { // for opening the appearance selection window
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

void __declspec(naked) op_set_hero_style() { // for setting the hero style/appearance takes an 1 int
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

void __declspec(naked) op_set_hero_race() { // for setting the hero race takes an 1 int
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

void __declspec(naked) op_get_light_level() {
	__asm {
		mov  edx, ds:[FO_VAR_ambient_light];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

void __declspec(naked) op_refresh_pc_art() {
	__asm {
		mov  esi, ecx;
		call RefreshPCArt;
		mov  ecx, esi;
		retn;
	}
}

void sf_get_attack_type(OpcodeContext& ctx) {
	ctx.setReturn(fo::GetCurrentAttackMode());
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
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call StopSfallSound;
end:
		mov  ecx, esi;
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
		mov  edx, modifiedIni;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

void __declspec(naked) op_force_aimed_shots() {
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

void __declspec(naked) op_disable_aimed_shots() {
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

void __declspec(naked) op_mark_movie_played() {
	__asm {
		_GET_ARG_INT(end);
		test eax, eax;
		jl   end;
		cmp  eax, 17;
		jge  end;
		mov  byte ptr ds:[eax + FO_VAR_gmovie_played_list], 1;
end:
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
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call SetBlockCombat;
end:
		mov  ecx, esi;
		retn;
	}
}

void __declspec(naked) op_tile_under_cursor() {
	__asm {
		mov  esi, ebx;
		sub  esp, 8;
		lea  edx, [esp];
		lea  eax, [esp + 4];
		call fo::funcoffs::mouse_get_position_;
		pop  edx;
		pop  eax;
		call fo::funcoffs::tile_num_; // ebx - unused
		mov  edx, eax; // tile
		mov  ebx, esi;
		mov  eax, esi;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

void __declspec(naked) op_gdialog_get_barter_mod() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_gdBarterMod];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

void __declspec(naked) op_set_inven_ap_cost() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, eax;
		call Inventory::SetInvenApCost;
end:
		mov  ecx, esi;
		retn;
	}
}

void sf_get_inven_ap_cost(OpcodeContext& ctx) {
	ctx.setReturn(Inventory::GetInvenApCost());
}

void sf_attack_is_aimed(OpcodeContext& ctx) {
	DWORD isAimed, unused;
	ctx.setReturn(!fo::func::intface_get_attack(&unused, &isAimed) ? isAimed : 0);
}

void sf_sneak_success(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::is_pc_sneak_working());
}

void sf_tile_light(OpcodeContext& ctx) {
	int lightLevel = fo::func::light_get_tile(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
	ctx.setReturn(lightLevel);
}

void sf_exec_map_update_scripts(OpcodeContext& ctx) {
	__asm call fo::funcoffs::scr_exec_map_update_scripts_
}

void sf_set_ini_setting(OpcodeContext& ctx) {
	const ScriptValue &argVal = ctx.arg(1);

	const char* saveValue;
	if (argVal.isInt()) {
		_itoa_s(argVal.rawValue(), ScriptExtender::gTextBuffer, 10);
		saveValue = ScriptExtender::gTextBuffer;
	} else {
		saveValue = argVal.strValue();
	}
	const char* key;
	char section[33], file[128];
	int result = ParseIniSetting(ctx.arg(0).strValue(), key, section, file);
	if (result > 0) {
		result = WritePrivateProfileString(section, key, saveValue, file);
	}

	switch (result) {
	case 0:
		ctx.printOpcodeError("%s() - value save error.", ctx.getMetaruleName());
		break;
	case -1:
		ctx.printOpcodeError("%s() - invalid setting argument.", ctx.getMetaruleName());
		break;
	default:
		return;
	}
	ctx.setReturn(-1);
}

static std::string GetIniFilePath(const ScriptValue &arg) {
	std::string fileName(".\\");
	if (ScriptExtender::iniConfigFolder.empty()) {
		fileName += arg.strValue();
	} else {
		fileName += ScriptExtender::iniConfigFolder;
		fileName += arg.strValue();
		if (GetFileAttributesA(fileName.c_str()) & FILE_ATTRIBUTE_DIRECTORY) {
			auto str = arg.strValue();
			for (size_t i = 2; ; i++, str++) {
				//if (*str == '.') str += (str[1] == '.') ? 3 : 2; // skip '.\' or '..\'
				fileName[i] = *str;
				if (!*str) break;
			}
		}
	}
	return fileName;
}

void sf_get_ini_sections(OpcodeContext& ctx) {
	if (!GetPrivateProfileSectionNamesA(ScriptExtender::gTextBuffer, ScriptExtender::TextBufferSize(), GetIniFilePath(ctx.arg(0)).c_str())) {
		ctx.setReturn(TempArray(0, 0));
		return;
	}
	std::vector<char*> sections;
	char* section = ScriptExtender::gTextBuffer;
	while (*section != 0) {
		sections.push_back(section); // position
		section += std::strlen(section) + 1;
	}
	size_t sz = sections.size();
	int arrayId = TempArray(sz, 0);
	auto& arr = arrays[arrayId];

	for (size_t i = 0; i < sz; ++i) {
		size_t j = i + 1;
		int len = (j < sz) ? sections[j] - sections[i] - 1 : -1;
		arr.val[i].set(sections[i], len); // copy string from buffer
	}
	ctx.setReturn(arrayId);
}

void sf_get_ini_section(OpcodeContext& ctx) {
	auto section = ctx.arg(1).strValue();
	int arrayId = TempArray(-1, 0); // associative

	if (GetPrivateProfileSectionA(section, ScriptExtender::gTextBuffer, ScriptExtender::TextBufferSize(), GetIniFilePath(ctx.arg(0)).c_str())) {
		auto& arr = arrays[arrayId];
		char *key = ScriptExtender::gTextBuffer, *val = nullptr;
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
	}
	ctx.setReturn(arrayId);
}

void sf_npc_engine_level_up(OpcodeContext& ctx) {
	if (ctx.arg(0).asBool()) {
		if (!npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0x840F); // enable
		npcEngineLevelUp = true;
	} else {
		if (npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0xE990);
		npcEngineLevelUp = false;
	}
}

}
}
