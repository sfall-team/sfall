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


static DWORD EncounteredHorrigan;
static void _stdcall ForceEncounter4() {
	*(DWORD*)0x672E04 = EncounteredHorrigan;
	SafeWrite32(0x4C070E, 0x95);
	SafeWrite32(0x4C0718, 0x95);
	SafeWrite32(0x4C06D1, 0x2E043D83);
	SafeWrite32(0x4C071D, 0xFFFC2413);
	SafeWrite8(0x4C0706, 0x75);
}
static void __declspec(naked) ForceEncounter3() {
	__asm {
		push eax;
		push ecx;
		push edx;
		mov eax, [esp + 0xC];
		sub eax, 5;
		mov [esp + 0xC], eax;
		call ForceEncounter4;
		pop edx;
		pop ecx;
		pop eax;
		retn;
	}
}
static void _stdcall ForceEncounter2(DWORD mapID, DWORD flags) {
	EncounteredHorrigan = *(DWORD*)0x672E04;
	SafeWrite32(0x4C070E, mapID);
	SafeWrite32(0x4C0718, mapID);
	SafeWrite32(0x4C06D1, 0x18EBD231); //xor edx, edx / jmp 0x18
	HookCall(0x4C071C, &ForceEncounter3);
	if (flags & 1) SafeWrite8(0x4C0706, 0xEB);
}
static void __declspec(naked) ForceEncounter() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push 0;
		push eax;
		call ForceEncounter2;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) ForceEncounterWithFlags() {
	__asm {
		pushad
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
		push ebx;
		push eax;
		call ForceEncounter2;
end:
		popad
		retn;
	}
}

// world_map_functions
static void __declspec(naked) funcInWorldMap() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov esi, eax;
		call InWorldMap;
		mov edx, eax;
		mov eax, esi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, esi;
		call interpretPushShort_;
		pop esi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetGameMode() {
	__asm {
		pushad;
		mov edi, eax;
		call GetCurrentLoops;
		mov edx, eax;
		mov eax, edi;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call interpretPushShort_;
		popad;
		retn;
	}
}
static void __declspec(naked) GetWorldMapXPos() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[_world_xpos];
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}
static void __declspec(naked) GetWorldMapYPos() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[_world_ypos];
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
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

void _stdcall SetMapMulti(float d);
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
