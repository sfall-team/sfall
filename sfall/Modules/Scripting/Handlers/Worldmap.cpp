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

#include "..\..\..\FalloutEngine\AsmMacros.h"
#include "..\..\..\FalloutEngine\Fallout2.h"

#include "..\..\..\SafeWrite.h"
#include "..\..\LoadGameHook.h"
#include "..\..\ScriptExtender.h"
#include "..\..\Worldmap.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "Worldmap.h"

namespace sfall
{
namespace script
{

static DWORD ForceEncounterMapID = -1;
static DWORD ForceEncounterFlags;

DWORD ForceEncounterRestore() {
	if (ForceEncounterMapID == -1) return 0;
	__int64 data = 0x672E043D83; // cmp ds:_Meet_Frank_Horrigan, 0
	SafeWriteBytes(0x4C06D1, (BYTE*)&data, 5);
	ForceEncounterFlags = 0;
	DWORD mapID = ForceEncounterMapID;
	ForceEncounterMapID = -1;
	return mapID;
}

static void ForceEncounterEffects() {
	if (ForceEncounterFlags & 0x10) { // _FadeOut flag
		__asm mov  eax, FO_VAR_black_palette;
		__asm call fo::funcoffs::palette_fade_to_;
		return;
	};

	// implements a flashing encounter icon
	if (ForceEncounterFlags & 4) return; // _NoIcon flag
	long iconType = (ForceEncounterFlags & 8) ? 3 : 1; // icon type flag (special: 0-3, normal: 0-1)

	fo::var::setInt(FO_VAR_wmEncounterIconShow) = 1;
	fo::var::setInt(FO_VAR_wmRndCursorFid) = 0;

	for (size_t n = 8; n > 0; --n) {
		long iconFidIndex = iconType - fo::var::getInt(FO_VAR_wmRndCursorFid);
		fo::var::setInt(FO_VAR_wmRndCursorFid) = iconFidIndex;
		__asm call fo::funcoffs::wmInterfaceRefresh_;
		fo::func::block_for_tocks(200);
	}
	fo::var::setInt(FO_VAR_wmEncounterIconShow) = 0;
}

static __declspec(naked) void wmRndEncounterOccurred_hack() {
	__asm {
		test ForceEncounterFlags, 0x1; // _NoCar flag
		jnz  noCar;
		cmp  dword ptr ds:[FO_VAR_Move_on_Car], 0;
		jz   noCar;
		mov  edx, FO_VAR_carCurrentArea;
		mov  eax, ForceEncounterMapID;
		call fo::funcoffs::wmMatchAreaContainingMapIdx_;
noCar:
		call ForceEncounterEffects;
		call ForceEncounterRestore;
		push 0x4C0721; // return addr
		jmp  fo::funcoffs::map_load_idx_; // eax - mapID
	}
}

void op_force_encounter(OpcodeContext& cxt) {
	if (ForceEncounterFlags & (1 << 31)) return; // wait prev. encounter

	DWORD mapID = cxt.arg(0).rawValue();
	if (mapID < 0) {
		cxt.printOpcodeError("%s() - invalid map number.", cxt.getOpcodeName());
		return;
	}
	if (ForceEncounterMapID == -1) MakeJump(0x4C06D1, wmRndEncounterOccurred_hack);

	ForceEncounterMapID = mapID;
	DWORD flags = 0;
	if (cxt.numArgs() > 1) {
		flags = cxt.arg(1).rawValue();
		if (flags & 2) { // _Lock flag
			flags |= (1 << 31); // set bit 31
		} else {
			flags &= ~(1 << 31);
		}
	}
	ForceEncounterFlags = flags;
}

// world_map_functions
__declspec(naked) void op_in_world_map() {
	__asm {
		mov  esi, ecx;
		call InWorldMap;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_get_game_mode() {
	__asm {
		mov  esi, ecx;
		call GetLoopFlags;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

__declspec(naked) void op_get_world_map_x_pos() {
	__asm {
		mov  edx, ds:[FO_VAR_world_xpos];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

__declspec(naked) void op_get_world_map_y_pos() {
	__asm {
		mov  edx, ds:[FO_VAR_world_ypos];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

__declspec(naked) void op_set_world_map_pos() {
	__asm {
		push ecx;
		_GET_ARG(ecx, esi); // get y value
		mov  eax, ebx;
		_GET_ARG_INT(end);  // get x value
		cmp  si, VAR_TYPE_INT;
		jne  end;
		mov  ds:[FO_VAR_world_xpos], eax;
		mov  ds:[FO_VAR_world_ypos], ecx;
end:
		pop  ecx;
		retn;
	}
}

void op_set_map_time_multi(OpcodeContext& ctx) {
	Worldmap::SetMapMulti(ctx.arg(0).asFloat());
}

void mf_set_car_intface_art(OpcodeContext& ctx) {
	Worldmap::SetCarInterfaceArt(ctx.arg(0).rawValue());
}

void mf_set_map_enter_position(OpcodeContext& ctx) {
	int tile = ctx.arg(0).rawValue();
	int elev = ctx.arg(1).rawValue();
	int rot = ctx.arg(2).rawValue();

	if (tile > -1 && tile < 40000) {
		fo::var::tile = tile;
	}
	if (elev > -1 && elev < 3) {
		fo::var::elevation = elev;
	}
	if (rot > -1 && rot < 6) {
		fo::var::rotation = rot;
	}
}

void mf_get_map_enter_position(OpcodeContext& ctx) {
	DWORD id = CreateTempArray(3, 0);
	arrays[id].val[0].set((long)fo::var::tile);
	arrays[id].val[1].set((long)fo::var::elevation);
	arrays[id].val[2].set((long)fo::var::rotation);
	ctx.setReturn(id);
}

void mf_set_rest_heal_time(OpcodeContext& ctx) {
	Worldmap::SetRestHealTime(ctx.arg(0).rawValue());
}

void mf_set_worldmap_heal_time(OpcodeContext& ctx) {
	Worldmap::SetWorldMapHealTime(ctx.arg(0).rawValue());
}

void mf_set_rest_mode(OpcodeContext& ctx) {
	Worldmap::SetRestMode(ctx.arg(0).rawValue());
}

void mf_set_rest_on_map(OpcodeContext& ctx) {
	long mapId = ctx.arg(0).rawValue();
	if (mapId < 0) {
		ctx.printOpcodeError("%s() - invalid map number.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	long elev = ctx.arg(1).rawValue();
	if (elev < -1 || elev > 2) {
		ctx.printOpcodeError("%s() - invalid map elevation.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	} else {
		Worldmap::SetRestMapLevel(mapId, elev, ctx.arg(2).asBool());
	}
}

void mf_get_rest_on_map(OpcodeContext& ctx) {
	long result = -1;
	long elev = ctx.arg(1).rawValue();
	if (elev < 0 || elev > 2) {
		ctx.printOpcodeError("%s() - invalid map elevation.", ctx.getMetaruleName());
	} else {
		result = Worldmap::GetRestMapLevel(elev, ctx.arg(0).rawValue());
	}
	ctx.setReturn(result);
}

void mf_tile_by_position(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::tile_num(ctx.arg(0).rawValue(), ctx.arg(1).rawValue()));
}

static const char* invalidSubTilePos = "%s() - invalid x/y coordinates for the sub-tile.";

void mf_set_terrain_name(OpcodeContext& ctx) {
	long x = ctx.arg(0).rawValue();
	long y = ctx.arg(1).rawValue();

	if (x < 0 || x >= (long)(7 * fo::var::wmNumHorizontalTiles) ||
	    y < 0 || y >= (long)(6 * (fo::var::wmMaxTileNum / fo::var::wmNumHorizontalTiles)))
	{
		ctx.printOpcodeError(invalidSubTilePos, ctx.getMetaruleName());
	} else {
		Worldmap::SetTerrainTypeName(x, y, ctx.arg(2).strValue());
	}
}

void mf_get_terrain_name(OpcodeContext& ctx) {
	if (ctx.numArgs() < 2) {
		ctx.setReturn(Worldmap::GetCurrentTerrainName());
	} else {
		long x = ctx.arg(0).rawValue();
		long y = ctx.arg(1).rawValue();

		if (x < 0 || x >= (long)(7 * fo::var::wmNumHorizontalTiles) ||
		    y < 0 || y >= (long)(6 * (fo::var::wmMaxTileNum / fo::var::wmNumHorizontalTiles)))
		{
			ctx.printOpcodeError(invalidSubTilePos, ctx.getMetaruleName());
			ctx.setReturn("Error");
		} else {
			ctx.setReturn(Worldmap::GetTerrainTypeName(x, y));
		}
	}
}

void mf_set_town_title(OpcodeContext& ctx) {
	Worldmap::SetCustomAreaTitle(ctx.arg(0).rawValue(), ctx.arg(1).strValue());
}

}
}
