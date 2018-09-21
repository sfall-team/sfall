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
		push ebx;
		push ecx;
		push edx;
		mov eax, [esp + 0x10];
		sub eax, 5;
		mov[esp + 0x10], eax;
		call ForceEncounter4;
		pop edx;
		pop ecx;
		pop ebx;
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

void __declspec(naked) op_force_encounter() {
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

void __declspec(naked) op_force_encounter_with_flags() {
	__asm {
		pushad
		mov ecx, eax;
		call fo::funcoffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
		mov ebx, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call fo::funcoffs::interpretPopLong_;
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
void __declspec(naked) op_in_world_map() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		push esi;
		mov esi, eax;
		call InWorldMap;
		mov edx, eax;
		mov eax, esi;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, esi;
		call fo::funcoffs::interpretPushShort_;
		pop esi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_get_game_mode() {
	__asm {
		pushad;
		mov edi, eax;
		call GetLoopFlags;
		mov edx, eax;
		mov eax, edi;
		call fo::funcoffs::interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, edi;
		call fo::funcoffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_get_world_map_x_pos() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_world_xpos];
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

void __declspec(naked) op_get_world_map_y_pos() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, ds:[FO_VAR_world_ypos];
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

void __declspec(naked) op_set_world_map_pos() {
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
		mov ds : [FO_VAR_world_xpos], eax;
		mov ds : [FO_VAR_world_ypos], edi;
end:
		pop esi;
		pop edi;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void sf_set_car_intface_art(OpcodeContext& ctx) {
	Worldmap::SetCarInterfaceArt(ctx.arg(0).asInt());
}

void sf_set_map_enter_position(OpcodeContext& ctx) {
	int tile = ctx.arg(0).asInt();
	int elev = ctx.arg(1).asInt();
	int rot = ctx.arg(2).asInt();

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

void sf_get_map_enter_position(OpcodeContext& ctx) {
	DWORD id = TempArray(3, 0);
	arrays[id].val[0].set((long)fo::var::tile);
	arrays[id].val[1].set((long)fo::var::elevation);
	arrays[id].val[2].set((long)fo::var::rotation);
	ctx.setReturn(id, DataType::INT);
}

void sf_set_rest_heal_time(OpcodeContext& ctx) {
	Worldmap::SetRestHealTime(ctx.arg(0).asInt());
}

void sf_set_rest_mode(OpcodeContext& ctx) {
	Worldmap::SetRestMode(ctx.arg(0).asInt());
}

void sf_set_rest_on_map(OpcodeContext& ctx) {
	long mapId = ctx.arg(0).asInt();
	if (mapId < 0) {
		ctx.printOpcodeError("set_rest_on_map() - invalid map number argument.");
		return;
	}
	long elev = ctx.arg(1).asInt();
	if (elev < -1 && elev > 2) {
		ctx.printOpcodeError("set_rest_on_map() - wrong map elevation argument.");
	} else {
		Worldmap::SetRestMapLevel(mapId, elev, ctx.arg(2).asBool());
	}
}

void sf_get_rest_on_map(OpcodeContext& ctx) {
	long elev = ctx.arg(1).asInt();
	if (elev < 0 && elev > 2) {
		ctx.printOpcodeError("get_rest_on_map() - wrong map elevation argument.");
	} else {
		ctx.setReturn(Worldmap::GetRestMapLevel(elev, ctx.arg(0).asInt()));
	}
}

}
}
