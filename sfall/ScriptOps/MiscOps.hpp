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

#include <cstring>

#include "main.h"

#include "Criticals.h"
#include "HeroAppearance.h"
#include "Movies.h"
#include "PlayerModel.h"
#include "QuestList.h"
#include "ScriptExtender.h"
#include "Sound.h"

/*
 *	Misc operators
 */

const char* stringTooLong = "%s() - the string exceeds maximum length of 64 characters.";

//Stop game, the same effect as open charsscreen or inventory
static void __declspec(naked) op_stop_game() {
	__asm {
		jmp map_disable_bk_processes_;
	}
}

//Resume the game when it is stopped
static void __declspec(naked) op_resume_game() {
	__asm {
		jmp map_enable_bk_processes_;
	}
}

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

static void __declspec(naked) op_eax_available() {
	__asm {
		xor  edx, edx
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static int __stdcall ParseIniSetting(const char* iniString, const char* &key, char section[], char file[]) {
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
	char section[33], file[128];

	if (ParseIniSetting(str, key, section, file) < 0) {
		return -1;
	}
	if (isString) {
		gTextBuffer[0] = 0;
		IniGetString(section, key, "", gTextBuffer, 256, file);
		return (DWORD)&gTextBuffer[0];
	} else {
		return IniGetInt(section, key, -1, file);
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
		mov  ds:[FO_VAR_carCurrentArea], eax;
end:
		retn;
	}
}

static const char* valueOutRange = "%s() - argument values out of range.";

static void __stdcall op_set_critical_table2() {
	const ScriptValue &critterArg = opHandler.arg(0),
	                  &bodypartArg = opHandler.arg(1),
	                  &slotArg = opHandler.arg(2),
	                  &elementArg = opHandler.arg(3),
	                  &valueArg = opHandler.arg(4);

	if (critterArg.isInt() && bodypartArg.isInt() && slotArg.isInt() && elementArg.isInt() && valueArg.isInt()) {
		DWORD critter = critterArg.rawValue(),
		      bodypart = bodypartArg.rawValue(),
		      slot = slotArg.rawValue(),
		      element = elementArg.rawValue();

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

static void __stdcall op_get_critical_table2() {
	const ScriptValue &critterArg = opHandler.arg(0),
	                  &bodypartArg = opHandler.arg(1),
	                  &slotArg = opHandler.arg(2),
	                  &elementArg = opHandler.arg(3);

	if (critterArg.isInt() && bodypartArg.isInt() && slotArg.isInt() && elementArg.isInt()) {
		DWORD critter = critterArg.rawValue(),
		      bodypart = bodypartArg.rawValue(),
		      slot = slotArg.rawValue(),
		      element = elementArg.rawValue();

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

static void __stdcall op_reset_critical_table2() {
	const ScriptValue &critterArg = opHandler.arg(0),
	                  &bodypartArg = opHandler.arg(1),
	                  &slotArg = opHandler.arg(2),
	                  &elementArg = opHandler.arg(3);

	if (critterArg.isInt() && bodypartArg.isInt() && slotArg.isInt() && elementArg.isInt()) {
		DWORD critter = critterArg.rawValue(),
		      bodypart = bodypartArg.rawValue(),
		      slot = slotArg.rawValue(),
		      element = elementArg.rawValue();

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
		mov eax, FO_VAR_cmap;
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
		mov  edx, ds:[FO_VAR_ambient_light];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
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
		mov  edx, ds:[FO_VAR_square];
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
	}
}

static void __declspec(naked) op_mark_movie_played() {
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
	}
}

static void __declspec(naked) op_gdialog_get_barter_mod() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_gdBarterMod];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
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
		int lightLevel = fo_light_get_tile(elevArg.rawValue(), tileArg.rawValue());
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
	char section[33], file[128];
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

static void mf_get_ini_sections() {
	std::string fileName = std::string(".\\") + opHandler.arg(0).strValue();
	if (!GetPrivateProfileSectionNamesA(gTextBuffer, GlblTextBufferSize(), fileName.c_str())) {
		opHandler.setReturn(CreateTempArray(0, 0));
		return;
	}
	std::vector<char*> sections;
	char* section = gTextBuffer;
	while (*section != 0) {
		sections.push_back(section); // position
		section += std::strlen(section) + 1;
	}
	size_t sz = sections.size();
	int arrayId = CreateTempArray(sz, 0);
	sArrayVar &arr = arrays[arrayId];

	for (size_t i = 0; i < sz; ++i) {
		size_t j = i + 1;
		int len = (j < sz) ? sections[j] - sections[i] - 1 : -1;
		arr.val[i].set(sections[i], len); // copy string from buffer
	}
	opHandler.setReturn(arrayId);
}

static void mf_get_ini_section() {
	std::string fileName = std::string(".\\") + opHandler.arg(0).strValue();
	const char* section = opHandler.arg(1).strValue();
	int arrayId = CreateTempArray(-1, 0); // associative

	if (GetPrivateProfileSectionA(section, gTextBuffer, GlblTextBufferSize(), fileName.c_str())) {
		sArrayVar &arr = arrays[arrayId];
		char *key = gTextBuffer, *val = nullptr;
		while (*key != 0) {
			char* val = std::strpbrk(key, "=");
			if (val != nullptr) {
				*val = '\0';
				val += 1;

				setArray(arrayId, ScriptValue(key).rawValue(), VAR_TYPE_STR, ScriptValue(val).rawValue(), VAR_TYPE_STR, 0);

				key = val + std::strlen(val) + 1;
			} else {
				key += std::strlen(key) + 1;
			}
		}
	}
	opHandler.setReturn(arrayId);
}

static void mf_set_quest_failure_value() {
	QuestList_AddQuestFailureValue(opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue());
}
