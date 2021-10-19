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

#include "Worldmap.h"

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

static void __declspec(naked) wmRndEncounterOccurred_hack() {
	__asm {
		test ForceEncounterFlags, 0x1; // _NoCar flag
		jnz  noCar;
		cmp  ds:[FO_VAR_Move_on_Car], 0;
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

static void __stdcall op_force_encounter2() {
	if (ForceEncounterFlags & (1 << 31)) return; // wait prev. encounter

	const ScriptValue &mapIDArg = opHandler.arg(0);
	if (mapIDArg.isInt()) {
		DWORD flags = 0;
		if (opHandler.numArgs() > 1) {
			const ScriptValue &flagsArg = opHandler.arg(1);
			if (!flagsArg.isInt()) goto invalidArgs;
			flags = flagsArg.rawValue();
			if (flags & 2) { // _Lock flag
				flags |= (1 << 31); // set bit 31
			} else {
				flags &= ~(1 << 31);
			}
		}
		DWORD mapID = mapIDArg.rawValue();
		if (mapID < 0) {
			opHandler.printOpcodeError("force_encounter/force_encounter_with_flags() - invalid map number.");
			return;
		}
		if (ForceEncounterMapID == -1) MakeJump(0x4C06D1, wmRndEncounterOccurred_hack);

		ForceEncounterMapID = mapID;
		ForceEncounterFlags = flags;
	} else {
invalidArgs:
		OpcodeInvalidArgs("force_encounter/force_encounter_with_flags");
	}
}

static void __declspec(naked) op_force_encounter() {
	_WRAP_OPCODE(op_force_encounter2, 1, 0)
}

static void __declspec(naked) op_force_encounter_with_flags() {
	_WRAP_OPCODE(op_force_encounter2, 2, 0)
}

// world_map_functions
static void __declspec(naked) op_in_world_map() {
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

static void __declspec(naked) op_get_game_mode() {
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

static void __declspec(naked) op_get_world_map_x_pos() {
	__asm {
		mov  edx, ds:[FO_VAR_world_xpos];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_get_world_map_y_pos() {
	__asm {
		mov  edx, ds:[FO_VAR_world_ypos];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) op_set_world_map_pos() {
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

static void __declspec(naked) op_set_map_time_multi() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov  ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov  edx, eax;
		mov  eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		cmp  dx, VAR_TYPE_FLOAT;
		jz   paramWasFloat;
		cmp  dx, VAR_TYPE_INT;
		jnz  fail;
		push eax;
		fild dword ptr [esp];
		fstp dword ptr [esp];
		jmp  end;
paramWasFloat:
		push eax;
end:
		call SetMapMulti;
fail:
		pop  edx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static void mf_set_car_intface_art() {
	Worldmap_SetCarInterfaceArt(opHandler.arg(0).rawValue());
}

static void mf_set_map_enter_position() {
	int tile = opHandler.arg(0).rawValue();
	int elev = opHandler.arg(1).rawValue();
	int rot = opHandler.arg(2).rawValue();

	if (tile > -1 && tile < 40000) {
		*fo::ptr::tile = tile;
	}
	if (elev > -1 && elev < 3) {
		*fo::ptr::elevation = elev;
	}
	if (rot > -1 && rot < 6) {
		*fo::ptr::rotation = rot;
	}
}

static void mf_get_map_enter_position() {
	DWORD id = CreateTempArray(3, 0);
	arrays[id].val[0].set((long)*fo::ptr::tile);
	arrays[id].val[1].set((long)*fo::ptr::elevation);
	arrays[id].val[2].set((long)*fo::ptr::rotation);
	opHandler.setReturn(id);
}

static void mf_tile_by_position() {
	opHandler.setReturn(fo::func::tile_num(opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue()));
}

static void mf_set_terrain_name() {
	Worldmap_SetTerrainTypeName(opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue(), opHandler.arg(2).strValue());
}

static void mf_set_town_title() {
	Worldmap_SetCustomAreaTitle(opHandler.arg(0).rawValue(), opHandler.arg(1).strValue());
}
