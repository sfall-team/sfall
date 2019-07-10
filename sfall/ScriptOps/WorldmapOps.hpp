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

static DWORD EncounteredHorrigan;
static DWORD ForceEnconterMapID;

static void _stdcall ForceEncounterRestore() {
	*(DWORD*)0x672E04 = EncounteredHorrigan;
	SafeWrite32(0x4C070E, *(DWORD*)0x4C0718); // map id
	SafeWrite16(0x4C06D8, 0x5175);
	SafeWrite32(0x4C071D, 0xFFFC2413);
	SafeWrite8(0x4C0706, 0x75);
}

static void __declspec(naked) wmRndEncounterOccurred_hook() {
	__asm {
		push ecx;
		call ForceEncounterRestore;
		pop  ecx;
		mov  eax, ForceEnconterMapID;
		jmp  map_load_idx_;
	}
}

static void __fastcall ForceEncounter2(DWORD mapID, DWORD flags) {
	EncounteredHorrigan = *(DWORD*)0x672E04;
	ForceEnconterMapID = mapID;
	SafeWrite32(0x4C070E, mapID);
	SafeWrite16(0x4C06D8, 0x13EB); // jmp 0x4C06ED
	HookCall(0x4C071C, wmRndEncounterOccurred_hook);

	if (flags & 1) SafeWrite8(0x4C0706, 0xEB);
}

static void __declspec(naked) ForceEncounter() {
	__asm {
		push ecx;
		push edx;
		_GET_ARG_INT(end);
		xor  edx, edx; // flags
		mov  ecx, eax; // mapID
		call ForceEncounter2;
end:
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) ForceEncounterWithFlags() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov ebx, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		cmp di, VAR_TYPE_INT;
		jnz end;
		mov edx, ebx; // flags
		mov ecx, eax; // mapID
		call ForceEncounter2;
end:
		popad;
		retn;
	}
}

// world_map_functions
static void __declspec(naked) funcInWorldMap() {
	__asm {
		push ecx;
		push edx;
		push eax;
		call InWorldMap;
		mov  edx, eax;
		pop  eax;
		_RET_VAL_INT2(ecx);
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) GetGameMode() {
	__asm {
		push ecx;
		push edx;
		push eax;
		call GetLoopFlags;
		mov  edx, eax;
		pop  eax;
		_RET_VAL_INT2(ecx);
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) GetWorldMapXPos() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_world_xpos];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
	}
}

static void __declspec(naked) GetWorldMapYPos() {
	__asm {
		push edx;
		push ecx;
		mov  edx, ds:[_world_ypos];
		_RET_VAL_INT2(ecx);
		pop  ecx;
		pop  edx;
		retn;
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
	int tile = opHandler.arg(0).asInt();
	int elev = opHandler.arg(1).asInt();
	int rot = opHandler.arg(2).asInt();

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
	opHandler.setReturn(id, DATATYPE_INT);
}
