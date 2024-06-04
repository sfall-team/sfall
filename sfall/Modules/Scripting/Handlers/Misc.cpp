/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\..\SubModules\ObjectName.h"

#include "..\Arrays.h"

#include "Misc.h"

namespace sfall
{
namespace script
{

static const char* stringTooLong = "%s() - the string exceeds maximum length of 64 characters.";

__declspec(naked) void op_stop_game() {
	__asm {
		jmp fo::funcoffs::map_disable_bk_processes_;
	}
}

__declspec(naked) void op_resume_game() {
	__asm {
		jmp fo::funcoffs::map_enable_bk_processes_;
	}
}

void op_set_dm_model(OpcodeContext& ctx) {
	auto model = ctx.arg(0).strValue();
	if (strlen(model) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(defaultMaleModelName, model);
}

void op_set_df_model(OpcodeContext& ctx) {
	auto model = ctx.arg(0).strValue();
	if (strlen(model) > 64) {
		ctx.printOpcodeError(stringTooLong, ctx.getOpcodeName());
		return;
	}
	strcpy(defaultFemaleModelName, model);
}

void op_set_movie_path(OpcodeContext& ctx) {
	long movieID = ctx.arg(1).rawValue();
	if (movieID < 0 || movieID >= MaxMovies) return;
	auto fileName = ctx.arg(0).strValue();
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

__declspec(naked) void op_eax_available() {
	__asm {
		xor  edx, edx; // EAX support has been removed since 2.1a
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_set_eax_environment() {
	__asm {
		_GET_ARG_INT(end);
		xor  eax, eax; // EAX support has been removed since 2.1a
end:
		retn;
	}
}

__declspec(naked) void op_get_uptime() {
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

__declspec(naked) void op_set_car_current_town() {
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
__declspec(naked) void op_nb_create_char() {
	__asm {
		xor  edx, edx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_hero_select_win() { // for opening the appearance selection window
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

__declspec(naked) void op_set_hero_style() { // for setting the hero style/appearance takes an 1 int
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

__declspec(naked) void op_set_hero_race() { // for setting the hero race takes an 1 int
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

__declspec(naked) void op_get_light_level() {
	__asm {
		mov  edx, ds:[FO_VAR_ambient_light];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_refresh_pc_art() {
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

__declspec(naked) void op_stop_sfall_sound() {
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

void op_get_tile_fid(OpcodeContext& ctx) {
	long tileX, tileY, squareNum, squareData, result,
	     tileAndElev = ctx.arg(0).rawValue(),
	     tileNum = tileAndElev & 0xFFFFFF,
	     elevation = (tileAndElev >> 24) & 0x0F,
	     mode = tileAndElev >> 28;

	if (tileNum >= 40000 || elevation > 2) {
		ctx.printOpcodeError("%s() - invalid tile data argument.", ctx.getMetaruleName());
		ctx.setReturn(0);
		return;
	}

	fo::func::tile_coord(tileNum, &tileX, &tileY);
	squareNum = fo::func::square_num(tileX, tileY, elevation);
	squareData = fo::var::square[elevation][squareNum];
	switch (mode) {
	case 1:
		result = (squareData >> 16) & 0x3FFF; // roof
		break;
	case 2:
		result = squareData; // raw data
		break;
	default:
		// Vanilla uses 12 bits for Tile FID, which means 4096 possible values, the mask was 0x0FFF
		// BUT sfall's FRM Limit patch extended it to 14 bits, so we need to use mask 0x3FFF
		result = squareData & 0x3FFF;  // this is how opcode worked up to 4.3.8
	}
	ctx.setReturn(result);
}

__declspec(naked) void op_mark_movie_played() {
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

__declspec(naked) void op_tile_under_cursor() {
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

__declspec(naked) void op_gdialog_get_barter_mod() {
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

void mf_set_quest_failure_value(OpcodeContext& ctx) {
	QuestList::AddQuestFailureValue(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void mf_set_scr_name(OpcodeContext& ctx) {
	long sid = fo::func::scr_find_sid_from_program(ctx.program());
	if (sid == -1) return;

	ObjectName::SetName(sid, ctx.arg(0).strValue());
}

void mf_signal_close_game(OpcodeContext& ctx) {
	// force ESC key in the main menu
	SafeWrite8(0x481B2A, 0xB8);
	SafeWrite32(0x481B2B, VK_ESCAPE); // mov eax, 27

	fo::var::game_user_wants_to_quit = 2; // return to the main menu
}

}
}
