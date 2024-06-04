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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"
#include "Stats.h"

#include "Objects.h"

namespace sfall
{

static int unjamTimeState;
static int maxCountLoadProto = 512;

long Objects::uniqueID = UniqueID::Start; // current counter id, saving to sfallgv.sav

bool Objects::IsUniqueID(long id) {
	return (id > UniqueID::Start || (id >= fo::PLAYER_ID && id < 83536)); // 65535 maximum possible number of prototypes
}

static void SetScriptObjectID(fo::GameObject* obj) {
	fo::ScriptInstance* script;
	if (fo::func::scr_ptr(obj->scriptId, &script) != -1) {
		script->ownerObjectId = obj->id;
	}
}

// Assigns a new unique identifier to an object if it has not been previously assigned
// the identifier is saved with the object in the saved game and this can used in various script
// player ID = 18000, all party members have ID = 18000 + its pid (file number of prototype)
long __fastcall Objects::SetObjectUniqueID(fo::GameObject* obj) {
	long id = obj->id;
	if (IsUniqueID(id)) return id;

	if ((DWORD)uniqueID >= (DWORD)UniqueID::End) uniqueID = UniqueID::Start;
	obj->id = ++uniqueID;
	SetScriptObjectID(obj);
	return uniqueID;
}

// Assigns a unique ID in the negative range (0xFFFFFFF6 - 0x8FFFFFF7)
long __fastcall Objects::SetSpecialID(fo::GameObject* obj) {
	long id = obj->id;
	if (id <= -10 || id > UniqueID::Start) return id;

	if ((DWORD)uniqueID >= (DWORD)UniqueID::End) uniqueID = UniqueID::Start;
	id = -9 - (++uniqueID - UniqueID::Start);
	obj->id = id;
	SetScriptObjectID(obj);
	return id;
}

void Objects::SetNewEngineID(fo::GameObject* obj) {
	if (obj->id > UniqueID::Start) return;
	obj->id = fo::func::new_obj_id();
	SetScriptObjectID(obj);
}

static __declspec(naked) void item_identical_hack() {
	using namespace fo::Fields;
	__asm {
		mov  ecx, [edi]; // item id
		cmp  ecx, Start; // start unique ID
		jg   notIdentical;
		mov  eax, [esi + scriptId];
		cmp  eax, ebx;
notIdentical:
		retn; // if ZF == 0 then item is not identical
	}
}

static __declspec(naked) void new_obj_id_hook() {
	__asm {
		mov  eax, 83535;
		cmp  dword ptr ds:[FO_VAR_cur_id], eax;
		jle  pickNewID;
		retn;
pickNewID: // skip PM range (18000 - 83535)
		mov  ds:[FO_VAR_cur_id], eax;
		jmp  fo::funcoffs::new_obj_id_;
	}
}

// Reassigns object IDs to all critters upon first loading a map and updates their HP stats
// TODO: for items?
static void map_fix_critter_id() {
	long npcStartID = 4096; // 0x1000
	fo::GameObject* obj = fo::func::obj_find_first();
	while (obj) {
		if (obj->IsCritter()) {
			if (obj->id < fo::PLAYER_ID) {
				obj->id = npcStartID++;
				SetScriptObjectID(obj);
			}
			Stats::UpdateHPStat(obj);
		}
		obj = fo::func::obj_find_next();
	}
}

static __declspec(naked) void map_load_file_hook() {
	__asm {
		call map_fix_critter_id;
		jmp  fo::funcoffs::map_fix_critter_combat_data_;
	}
}

static __declspec(naked) void queue_add_hack() {
	using namespace fo;
	using namespace Fields;
	__asm {
		// engine code
		mov  [edx + 8], edi; // queue.object
		mov  [edx], esi;     // queue.time
		//---
		cmp  ds:[FO_VAR_loadingGame], 1; // don't change the object ID when loading a saved game (e.g. fix: NPC turns into a container)
		je   skip;
		test edi, edi;
		jnz  fix;
skip:
		retn;
fix:
		mov  eax, [edi + protoId];
		and  eax, 0x0F000000;
		jnz  notItem; // object is not an item?
		push ecx;
		push edx;
		mov  ecx, edi;
		call Objects::SetSpecialID;
		pop  edx;
		pop  ecx;
		retn;
notItem:
		cmp  ecx, script_timer_event; // QueueType
		je   end;
		cmp  eax, OBJ_TYPE_CRITTER << 24;
		jne  end;
		push ecx;
		push edx;
		mov  ecx, edi;
		call Objects::SetObjectUniqueID;
		pop  edx;
		pop  ecx;
end:
		xor  edi, edi; // fix: don't set "Used" flag for non-item objects
		retn;
	}
}

void Objects::SetAutoUnjamLockTime(DWORD time) {
	if (!unjamTimeState) BlockCall(0x4A364A); // disable auto unjam at midnight

	if (time > 0) {
		SafeWrite8(0x4831D9, (BYTE)time);
		if (unjamTimeState == 2) {
			SafeWrite8(0x4831DA, 0x7C);
		}
		unjamTimeState = 1;
	} else {
		SafeWrite8(0x4831DA, CodeType::JumpShort); // disable auto unjam
		unjamTimeState = 2;
	}
}

static void RestoreObjUnjamAllLocks() {
	if (unjamTimeState) {
		SafeWrite8(0x4A364A, 0xE8);
		SafeWrite32(0x4A364B, 0xFFFF9E69);
		SafeWrite8(0x4831DA, 0x7C);
		SafeWrite8(0x4831D9, 24);
		unjamTimeState = 0;
	}
}

static __declspec(naked) void proto_ptr_hack() {
	__asm {
		mov  ecx, maxCountLoadProto;
		cmp  ecx, 4096;
		jae  skip;
		cmp  eax, ecx;
		jb   end;
		add  ecx, 256;
		mov  maxCountLoadProto, ecx;
skip:
		cmp  eax, ecx;
end:
		retn;
	}
}

void Objects::LoadProtoAutoMaxLimit() {
	MakeCall(0x4A21B2, proto_ptr_hack);
}

// Places the PID_CORPSE_BLOOD object on the lower layer of objects on the tile
static __declspec(naked) void obj_insert_hack() {
	using namespace fo;
	using namespace Fields;
	__asm {
		// engine code
		mov  edi, [ebx]; // tableNodes.objectPtr
		mov  [esp + 0x38 - 0x1C + 4], esi; // 0
		test edi, edi;   // objectPtr
		jnz  insert;
		retn;
insert: //----------
		mov  esi, [ecx]; // esi - inserted object
		mov  edi, [edi]; // object placed on the tile
		xor  edx, edx;
		cmp  dword ptr [esi + protoId], PID_CORPSE_BLOOD;
		je   fix;
		cmp  dword ptr [edi + protoId], PID_CORPSE_BLOOD;
		jne  skip;
		sete dl; // set to 1 if PID_CORPSE_BLOOD is already located on the map
fix:
		mov  esi, [esi + elevation];
		cmp  [edi + elevation], esi;
		jne  skip;
		xor  edi, edi;
		test dl, dl;
		jz   skip;
		mov  ebx, [ebx]; // tableNodes.objectPtr
		add  ebx, 4;     // tableNodes.nextObjectPtr
		mov  edi, [ebx]; // tableNodes.objectPtr
skip:
		retn;
	}
}

void Objects::init() {
	LoadGameHook::OnGameReset() += []() {
		RestoreObjUnjamAllLocks();
		//fo::var::cur_id = 4;
	};

	HookCall(0x4A38A5, new_obj_id_hook);
	SafeWrite8(0x4A38B3, CodeType::Nop); // fix ID increment

	// Fix the ID range check for item objects with IDs in the negative range
	// Special ID values are assigned in the negative range
	SafeWrite8(0x495273, 0x73); // jge > jae (partyMemberItemSave_)

	MakeCall(0x477A0E, item_identical_hack); // don't put item with unique ID to items stack

	// Fix mapper bug by reassigning object IDs to critters (for unvisited maps)
	HookCall(0x482DE2, map_load_file_hook);
	// Additionally fix object IDs for queued events
	MakeCall(0x4A25BA, queue_add_hack);

	// Place some objects on the lower z-layer of the tile
	MakeCall(0x48D918, obj_insert_hack, 1);
}

}
