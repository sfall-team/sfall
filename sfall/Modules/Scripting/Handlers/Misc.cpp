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

#include "..\..\..\FalloutEngine\AsmMacros.h"
#include "..\..\..\FalloutEngine\Fallout2.h"

#include "..\..\..\Utils.h"
#include "..\..\Criticals.h"
#include "..\..\HeroAppearance.h"
//#include "..\..\MiscPatches.h"
#include "..\..\Movies.h"
#include "..\..\PlayerModel.h"
#include "..\..\QuestList.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Sound.h"

#include "..\Arrays.h"

#include "Misc.h"

namespace sfall
{
namespace script
{

const char* stringTooLong = "%s() - the string exceeds maximum length of 64 characters.";

void __declspec(naked) op_stop_game() {
	__asm {
		jmp fo::funcoffs::map_disable_bk_processes_;
	}
}

void __declspec(naked) op_resume_game() {
	__asm {
		jmp fo::funcoffs::map_enable_bk_processes_;
	}
}

void op_set_dm_model(OpcodeContext& ctx) {
	const char* model = ctx.arg(0).strValue();
	if (strlen(model) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(defaultMaleModelName, model);
}

void op_set_df_model(OpcodeContext& ctx) {
	const char* model = ctx.arg(0).strValue();
	if (strlen(model) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(defaultFemaleModelName, model);
}

void op_set_movie_path(OpcodeContext& ctx) {
	long movieID = ctx.arg(1).rawValue();
	if (movieID < 0 || movieID >= MaxMovies) return;
	const char* fileName = ctx.arg(0).strValue();
	if (strlen(fileName) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(&MoviePaths[movieID * 65], fileName);
}

void op_get_year(OpcodeContext& ctx) {
	int year = 0;
	__asm {
		xor  eax, eax;
		xor  edx, edx;
		lea  ebx, year;
		call fo::funcoffs::game_time_date_;
	}
	ctx.setReturn(year);
}

void __declspec(naked) op_eax_available() {
	__asm {
		xor  edx, edx
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
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
	file[2 + filelen] = 0;

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
		IniReader::GetString(section, key, "", ScriptExtender::gTextBuffer, 256, file);
		return (DWORD)&ScriptExtender::gTextBuffer[0];
	} else {
		return IniReader::GetInt(section, key, -1, file);
	}
}

void op_get_ini_setting(OpcodeContext& ctx) {
	ctx.setReturn(GetIniSetting(ctx.arg(0).strValue(), false));
}

void op_get_ini_string(OpcodeContext& ctx) {
	DWORD result = GetIniSetting(ctx.arg(0).strValue(), true);
	ctx.setReturn(result, (result != -1) ? DATATYPE_STR : DATATYPE_INT);
}

static DWORD __stdcall GetTickCount2() {
	return GetTickCount();
}

void __declspec(naked) op_get_uptime() {
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

void __declspec(naked) op_set_car_current_town() {
	__asm {
		_GET_ARG_INT(end);
		mov  ds:[FO_VAR_carCurrentArea], eax;
end:
		retn;
	}
}

static const char* valueOutRange = "%s() - argument values out of range.";

void op_set_critical_table(OpcodeContext& ctx) {
	DWORD critter  = ctx.arg(0).rawValue(),
	      bodypart = ctx.arg(1).rawValue(),
	      slot     = ctx.arg(2).rawValue(),
	      element  = ctx.arg(3).rawValue();

	if (critter >= Criticals::critTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
		ctx.printOpcodeError(valueOutRange, ctx.getOpcodeName());
	} else {
		Criticals::SetCriticalTable(critter, bodypart, slot, element, ctx.arg(4).rawValue());
	}
}

void op_get_critical_table(OpcodeContext& ctx) {
	DWORD critter  = ctx.arg(0).rawValue(),
	      bodypart = ctx.arg(1).rawValue(),
	      slot     = ctx.arg(2).rawValue(),
	      element  = ctx.arg(3).rawValue();

	if (critter >= Criticals::critTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
		ctx.printOpcodeError(valueOutRange, ctx.getOpcodeName());
	} else {
		ctx.setReturn(Criticals::GetCriticalTable(critter, bodypart, slot, element));
	}
}

void op_reset_critical_table(OpcodeContext& ctx) {
	DWORD critter  = ctx.arg(0).rawValue(),
	      bodypart = ctx.arg(1).rawValue(),
	      slot     = ctx.arg(2).rawValue(),
	      element  = ctx.arg(3).rawValue();

	if (critter >= Criticals::critTableCount || bodypart >= 9 || slot >= 6 || element >= 7) {
		ctx.printOpcodeError(valueOutRange, ctx.getOpcodeName());
	} else {
		Criticals::ResetCriticalTable(critter, bodypart, slot, element);
	}
}

void op_set_palette(OpcodeContext& ctx) {
	const char* palette = ctx.arg(0).strValue();
	__asm {
		mov  eax, palette;
		call fo::funcoffs::loadColorTable_;
		mov  eax, FO_VAR_cmap;
		call fo::funcoffs::palette_set_to_;
	}
}

//numbers subgame functions
void __declspec(naked) op_nb_create_char() {
	__asm retn;
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

void op_play_sfall_sound(OpcodeContext& ctx) {
	DWORD soundID = 0;
	long mode = ctx.arg(1).rawValue();
	if (mode >= 0) soundID = Sound::PlaySfallSound(ctx.arg(0).strValue(), mode);
	ctx.setReturn(soundID);
}

void __declspec(naked) op_stop_sfall_sound() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call Sound::StopSfallSound;
end:
		mov  ecx, esi;
		retn;
	}
}

// TODO: It seems that this function does not work...
void __declspec(naked) op_get_tile_fid() {
	__asm {
		push ecx;
		_GET_ARG_INT(fail); // get tile value
		mov  esi, ebx;      // keep script
		sub  esp, 8;        // x/y buf
		lea  edx, [esp];
		lea  ebx, [esp + 4];
		call fo::funcoffs::tile_coord_;
		pop  eax; // x
		pop  edx; // y
		call fo::funcoffs::square_num_;
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

void __declspec(naked) op_modified_ini() {
	__asm {
		mov  edx, IniReader::modifiedIni;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
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
	}
}

void __declspec(naked) op_gdialog_get_barter_mod() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_gdBarterMod];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

void op_sneak_success(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::is_pc_sneak_working());
}

void op_tile_light(OpcodeContext& ctx) {
	int lightLevel = fo::func::light_get_tile(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
	ctx.setReturn(lightLevel);
}

void mf_exec_map_update_scripts(OpcodeContext& ctx) {
	__asm call fo::funcoffs::scr_exec_map_update_scripts_
}

void mf_set_ini_setting(OpcodeContext& ctx) {
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
		result = WritePrivateProfileStringA(section, key, saveValue, file);
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

void mf_get_ini_sections(OpcodeContext& ctx) {
	std::string fileName = std::string(".\\") + ctx.arg(0).strValue();
	if (!GetPrivateProfileSectionNamesA(ScriptExtender::gTextBuffer, ScriptExtender::TextBufferSize(), fileName.c_str())) {
		ctx.setReturn(CreateTempArray(0, 0));
		return;
	}
	std::vector<char*> sections;
	char* section = ScriptExtender::gTextBuffer;
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
	ctx.setReturn(arrayId);
}

void mf_get_ini_section(OpcodeContext& ctx) {
	std::string fileName = std::string(".\\") + ctx.arg(0).strValue();
	const char* section = ctx.arg(1).strValue();
	int arrayId = CreateTempArray(-1, 0); // associative

	if (GetPrivateProfileSectionA(section, ScriptExtender::gTextBuffer, ScriptExtender::TextBufferSize(), fileName.c_str())) {
		sArrayVar &arr = arrays[arrayId];
		char *key = ScriptExtender::gTextBuffer, *val = nullptr;
		while (*key != 0) {
			char* val = std::strpbrk(key, "=");
			if (val != nullptr) {
				*val = '\0';
				val += 1;

				setArray(arrayId, ScriptValue(key), ScriptValue(val), false);

				key = val + std::strlen(val) + 1;
			} else {
				key += std::strlen(key) + 1;
			}
		}
	}
	ctx.setReturn(arrayId);
}

void mf_set_quest_failure_value(OpcodeContext& ctx) {
	QuestList::AddQuestFailureValue(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void mf_set_scr_name(OpcodeContext& ctx) {
	long sid = fo::func::scr_find_sid_from_program(ctx.program());
	if (sid == -1) return;

	ObjectName::SetName(sid, ctx.arg(0).strValue());
}

}
}
