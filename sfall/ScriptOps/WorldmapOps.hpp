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
#include "Arrays.h"
#include "ScriptExtender.h"
#include "Worldmap.h"

static DWORD ForceEncounterMapID = -1;
static DWORD ForceEncounterFlags;

DWORD ForceEncounterRestore() {
	if (ForceEncounterMapID == -1) return 0;
	long long data = 0x672E043D83; // cmp ds:_Meet_Frank_Horrigan, 0
	SafeWriteBytes(0x4C06D1, (BYTE*)&data, 5);
	ForceEncounterFlags = 0;
	DWORD mapID = ForceEncounterMapID;
	ForceEncounterMapID = -1;
	return mapID;
}

static void ForceEncounterEffects() {
	if (ForceEncounterFlags & 0x10) { // _FadeOut flag
		__asm mov  eax, _black_palette;
		__asm call palette_fade_to_;
		return;
	};

	// implements a flashing encounter icon
	if (ForceEncounterFlags & 4) return; // _NoIcon flag
	long iconType = (ForceEncounterFlags & 8) ? 3 : 1; // icon type flag (special: 0-3, normal: 0-1)

	*(DWORD*)_wmEncounterIconShow = 1;
	*(DWORD*)_wmRndCursorFid = 0;

	for (size_t n = 8; n > 0; --n) {
		long iconFidIndex = iconType - *(DWORD*)_wmRndCursorFid;
		*(DWORD*)_wmRndCursorFid = iconFidIndex;
		__asm call wmInterfaceRefresh_;
		BlockForTocks(200);
	}
	*(DWORD*)_wmEncounterIconShow = 0;
}

static void __declspec(naked) wmRndEncounterOccurred_hack() {
	__asm {
		test ForceEncounterFlags, 0x1; // _NoCar flag
		jnz  noCar;
		cmp  ds:[_Move_on_Car], 0;
		jz   noCar;
		mov  edx, _CarCurrArea;
		mov  eax, ForceEncounterMapID;
		call wmMatchAreaContainingMapIdx_;
noCar:
		call ForceEncounterEffects;
		call ForceEncounterRestore;
		push 0x4C0721; // return addr
		jmp  map_load_idx_; // eax - mapID
	}
}

static void _stdcall ForceEncounter2() {
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

static void __declspec(naked) ForceEncounter() {
	_WRAP_OPCODE(ForceEncounter2, 1, 0)
}

static void __declspec(naked) ForceEncounterWithFlags() {
	_WRAP_OPCODE(ForceEncounter2, 2, 0)
}

// world_map_functions
static void __declspec(naked) funcInWorldMap() {
	__asm {
		mov  esi, ecx;
		call InWorldMap;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT2;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) GetGameMode() {
	__asm {
		mov  esi, ecx;
		call GetLoopFlags;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT2;
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) GetWorldMapXPos() {
	__asm {
		mov  edx, ds:[_world_xpos];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) GetWorldMapYPos() {
	__asm {
		mov  edx, ds:[_world_ypos];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __declspec(naked) SetWorldMapPos() {
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
		mov ds:[_world_xpos], eax;
		mov ds:[_world_ypos], edi;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

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

static void sf_set_map_enter_position() {
	int tile = opHandler.arg(0).rawValue();
	int elev = opHandler.arg(1).rawValue();
	int rot = opHandler.arg(2).rawValue();

	if (tile > -1 && tile < 40000) {
		*ptr_tile = tile;
	}
	if (elev > -1 && elev < 3) {
		*ptr_elevation = elev;
	}
	if (rot > -1 && rot < 6) {
		*ptr_rotation = rot;
	}
}

static void sf_get_map_enter_position() {
	DWORD id = TempArray(3, 0);
	arrays[id].val[0].set((long)*ptr_tile);
	arrays[id].val[1].set((long)*ptr_elevation);
	arrays[id].val[2].set((long)*ptr_rotation);
	opHandler.setReturn(id);
}

static void sf_tile_by_position() {
	opHandler.setReturn(TileNum(opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue()));
}

static void sf_set_terrain_name() {
	Wmap_SetTerrainTypeName(opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue(), opHandler.arg(2).strValue());
}

static void sf_set_town_title() {
	Wmap_SetCustomAreaTitle(opHandler.arg(0).rawValue(), opHandler.arg(1).strValue());
}
