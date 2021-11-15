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

#include "Combat.h"
#include "Objects.h"
#include "PartyControl.h"

namespace sfall
{

static const char* protoFailedLoad = "%s() - failed to load a prototype ID: %d";

static const char* nameNPCToInc;
static long pidNPCToInc;
static bool onceNpcLoop;

static void __cdecl IncNPCLevel(const char* fmt, const char* name) {
	fo::GameObject* mObj;
	__asm {
		push edx;
		mov  eax, [ebp + 0x150 - 0x1C + 16]; // ebp <- esp
		mov  edx, [eax];
		mov  mObj, edx;
	}

	if ((pidNPCToInc && (mObj && mObj->protoId == pidNPCToInc)) || (!pidNPCToInc && !_stricmp(name, nameNPCToInc))) {
		fo::func::debug_printf(fmt, name);

		SafeWrite32(0x495C50, 0x01FB840F); // Want to keep this check intact. (restore)

		SafeMemSet(0x495C77, CodeType::Nop, 6);   // Check that the player is high enough for the npc to consider this level
		//SafeMemSet(0x495C8C, CodeType::Nop, 6); // Check that the npc isn't already at its maximum level
		SafeMemSet(0x495CEC, CodeType::Nop, 6);   // Check that the npc hasn't already levelled up recently
		if (!npcAutoLevelEnabled) {
			SafeWrite8(0x495CFB, CodeType::JumpShort); // Disable random element
		}
		__asm mov [ebp + 0x150 - 0x28 + 16], 255; // set counter for exit loop
	} else {
		if (!onceNpcLoop) {
			SafeWrite32(0x495C50, 0x01FCE9); // set goto next member
			onceNpcLoop = true;
		}
	}
	__asm pop edx;
}

static void __stdcall op_inc_npc_level2() {
	nameNPCToInc = opHandler.arg(0).asString();
	pidNPCToInc = opHandler.arg(0).asInt(); // set to 0 if passing npc name
	if (pidNPCToInc == 0 && nameNPCToInc[0] == 0) return;

	MakeCall(0x495BF1, IncNPCLevel);  // Replace the debug output
	__asm call fo::funcoffs::partyMemberIncLevels_;
	onceNpcLoop = false;

	// restore code
	SafeWrite32(0x495C50, 0x01FB840F);
	__int64 data = 0x01D48C0F;
	SafeWriteBytes(0x495C77, (BYTE*)&data, 6);
	//SafeWrite16(0x495C8C, 0x8D0F);
	//SafeWrite32(0x495C8E, 0x000001BF);
	data = 0x0130850F;
	SafeWriteBytes(0x495CEC, (BYTE*)&data, 6);
	if (!npcAutoLevelEnabled) {
		SafeWrite8(0x495CFB, CodeType::JumpZ);
	}
}

static void __declspec(naked) op_inc_npc_level() {
	_WRAP_OPCODE(op_inc_npc_level2, 1, 0)
}

static void __stdcall op_get_npc_level2() {
	const ScriptValue &npcArg = opHandler.arg(0);

	if (!npcArg.isFloat()) {
		int level = -1;
		DWORD findPid = npcArg.asInt(); // set to 0 if passing npc name
		const char *critterName, *name = npcArg.asString();

		if (findPid || name[0] != 0) {
			DWORD pid = 0;
			fo::ObjectListData* members = *fo::ptr::partyMemberList;
			for (DWORD i = 0; i < *fo::ptr::partyMemberCount; i++) {
				if (!findPid) {
					__asm {
						mov  eax, members;
						mov  eax, [eax];
						call fo::funcoffs::critter_name_;
						mov  critterName, eax;
					}
					if (!_stricmp(name, critterName)) { // found npc
						pid = members[i].object->protoId;
						break;
					}
				} else {
					DWORD _pid = members[i].object->protoId;
					if (findPid == _pid) {
						pid = _pid;
						break;
					}
				}
			}
			if (pid) {
				DWORD* pidList = *fo::ptr::partyMemberPidList;
				DWORD* lvlUpInfo = *fo::ptr::partyMemberLevelUpInfoList;
				for (DWORD j = 0; j < *fo::ptr::partyMemberMaxCount; j++) {
					if (pidList[j] == pid) {
						level = lvlUpInfo[j * 3];
						break;
					}
				}
			}
		}
		opHandler.setReturn(level);
	} else {
		OpcodeInvalidArgs("get_npc_level");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_get_npc_level() {
	_WRAP_OPCODE(op_get_npc_level2, 1, 1)
}

static void __stdcall op_remove_script2() {
	fo::GameObject* object = opHandler.arg(0).asObject();
	if (object) {
		if (object->scriptId != 0xFFFFFFFF) {
			fo::func::scr_remove(object->scriptId);
			object->scriptId = 0xFFFFFFFF;
		}
	} else {
		OpcodeInvalidArgs("remove_script");
	}
}

static void __declspec(naked) op_remove_script() {
	_WRAP_OPCODE(op_remove_script2, 1, 0)
}

#define exec_script_proc(script, proc) __asm {  \
	__asm mov  eax, script                      \
	__asm mov  edx, proc                        \
	__asm call fo::funcoffs::exec_script_proc_  \
}

static void __stdcall op_set_script2() {
	using fo::Scripts::start;
	using fo::Scripts::map_enter_p_proc;

	fo::GameObject* object = opHandler.arg(0).asObject();
	const ScriptValue &scriptIdxArg = opHandler.arg(1);

	if (object && scriptIdxArg.isInt()) {
		long scriptType;
		unsigned long valArg = scriptIdxArg.rawValue();

		long scriptIndex = valArg & ~0xF0000000;
		if (scriptIndex == 0 || valArg > 0x8FFFFFFF) { // negative values are not allowed
			opHandler.printOpcodeError("set_script() - the script index number is incorrect.");
			return;
		}
		scriptIndex--;

		if (object->scriptId != 0xFFFFFFFF) {
			fo::func::scr_remove(object->scriptId);
			object->scriptId = 0xFFFFFFFF;
		}
		if (object->IsCritter()) {
			scriptType = fo::Scripts::ScriptTypes::SCRIPT_CRITTER;
		} else {
			scriptType = fo::Scripts::ScriptTypes::SCRIPT_ITEM;
		}
		fo::func::obj_new_sid_inst(object, scriptType, scriptIndex);

		long scriptId = object->scriptId;
		exec_script_proc(scriptId, start);
		if ((valArg & 0x80000000) == 0) exec_script_proc(scriptId, map_enter_p_proc);
	} else {
		OpcodeInvalidArgs("set_script");
	}
}

static void __declspec(naked) op_set_script() {
	_WRAP_OPCODE(op_set_script2, 2, 0)
}

static void __stdcall op_create_spatial2() {
	using fo::Scripts::start;

	const ScriptValue &scriptIdxArg = opHandler.arg(0),
	                  &tileArg = opHandler.arg(1),
	                  &elevArg = opHandler.arg(2),
	                  &radiusArg = opHandler.arg(3);

	if (scriptIdxArg.isInt() && tileArg.isInt() && elevArg.isInt() && radiusArg.isInt()) {
		DWORD scriptIndex = scriptIdxArg.rawValue(),
		      tile = tileArg.rawValue(),
		      elevation = elevArg.rawValue(),
		      radius = radiusArg.rawValue();

		long scriptId;
		fo::ScriptInstance* scriptPtr;
		if (fo::func::scr_new(&scriptId, fo::Scripts::ScriptTypes::SCRIPT_SPATIAL) == -1 || fo::func::scr_ptr(scriptId, &scriptPtr) == -1) return;

		// set spatial script properties:
		scriptPtr->scriptIdx = scriptIndex - 1;
		scriptPtr->elevationAndTile = (elevation << 29) & 0xE0000000 | tile;
		scriptPtr->spatialRadius = radius;

		// this will load appropriate script program and link it to the script instance we just created:
		exec_script_proc(scriptId, start);

		opHandler.setReturn(fo::func::scr_find_obj_from_program(scriptPtr->program));
	} else {
		OpcodeInvalidArgs("create_spatial");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_create_spatial() {
	_WRAP_OPCODE(op_create_spatial2, 4, 1)
}

#undef exec_script_proc

static void mf_spatial_radius() {
	fo::GameObject* spatialObj = opHandler.arg(0).asObject();
	if (spatialObj) {
		fo::ScriptInstance* script;
		if (fo::func::scr_ptr(spatialObj->scriptId, &script) != -1) {
			opHandler.setReturn(script->spatialRadius);
		}
	} else {
		OpcodeInvalidArgs("spatial_radius");
		opHandler.setReturn(0);
	}
}

static void __stdcall op_get_script2() {
	fo::GameObject* object = opHandler.arg(0).asObject();
	if (object) {
		long scriptIndex = object->scriptIndex;
		opHandler.setReturn((scriptIndex >= 0) ? ++scriptIndex : 0);
	} else {
		OpcodeInvalidArgs("get_script");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_get_script() {
	_WRAP_OPCODE(op_get_script2, 1, 1)
}

static void __stdcall op_set_critter_burst_disable2() {
	fo::GameObject* critter = opHandler.arg(0).asObject();
	const ScriptValue &disableArg = opHandler.arg(1);

	if (critter && disableArg.isInt()) {
		SetNoBurstMode(critter, disableArg.asBool());
	} else {
		OpcodeInvalidArgs("set_critter_burst_disable");
	}
}

static void __declspec(naked) op_set_critter_burst_disable() {
	_WRAP_OPCODE(op_set_critter_burst_disable2, 2, 0)
}

static void __stdcall op_get_weapon_ammo_pid2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	if (obj) {
		long pid = -1;
		fo::Proto* proto;
		if (obj->IsItem() && fo::util::GetProto(obj->protoId, &proto)) {
			long type = proto->item.type;
			if (type == fo::ItemType::item_type_weapon || type == fo::ItemType::item_type_misc_item) {
				pid = obj->item.ammoPid;
			}
		}
		opHandler.setReturn(pid);
	} else {
		OpcodeInvalidArgs("get_weapon_ammo_pid");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_get_weapon_ammo_pid() {
	_WRAP_OPCODE(op_get_weapon_ammo_pid2, 1, 1)
}

static void __stdcall op_set_weapon_ammo_pid2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &pidArg = opHandler.arg(1);

	if (obj && pidArg.isInt()) {
		if (obj->IsNotItem()) return;

		fo::Proto* proto;
		if (fo::util::GetProto(obj->protoId, &proto)) {
			long type = proto->item.type;
			if (type == fo::ItemType::item_type_weapon || type == fo::ItemType::item_type_misc_item) {
				obj->item.ammoPid = pidArg.rawValue();
			}
		} else {
			opHandler.printOpcodeError(protoFailedLoad, "set_weapon_ammo_pid", obj->protoId);
		}
	} else {
		OpcodeInvalidArgs("set_weapon_ammo_pid");
	}
}

static void __declspec(naked) op_set_weapon_ammo_pid() {
	_WRAP_OPCODE(op_set_weapon_ammo_pid2, 2, 0)
}

static void __stdcall op_get_weapon_ammo_count2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn((obj->IsItem()) ? obj->item.charges : 0);
	} else {
		OpcodeInvalidArgs("get_weapon_ammo_count");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_weapon_ammo_count() {
	_WRAP_OPCODE(op_get_weapon_ammo_count2, 1, 1)
}

static void __stdcall op_set_weapon_ammo_count2() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &countArg = opHandler.arg(1);

	if (obj && countArg.isInt()) {
		if (obj->IsItem()) obj->item.charges = countArg.rawValue();
	} else {
		OpcodeInvalidArgs("set_weapon_ammo_count");
	}
}

static void __declspec(naked) op_set_weapon_ammo_count() {
	_WRAP_OPCODE(op_set_weapon_ammo_count2, 2, 0)
}

enum BlockType {
	BLOCKING_TYPE_BLOCK  = 0,
	BLOCKING_TYPE_SHOOT  = 1,
	BLOCKING_TYPE_AI     = 2,
	BLOCKING_TYPE_SIGHT  = 3,
	BLOCKING_TYPE_SCROLL = 4
};

static DWORD getBlockingFunc(BlockType type) {
	switch (type) {
	case BLOCKING_TYPE_BLOCK: default:
		return fo::funcoffs::obj_blocking_at_;       // with calling hook
	case BLOCKING_TYPE_SHOOT:
		return fo::funcoffs::obj_shoot_blocking_at_; // w/o calling hook
	case BLOCKING_TYPE_AI:
		return fo::funcoffs::obj_ai_blocking_at_;    // w/o calling hook
	case BLOCKING_TYPE_SIGHT:
		return fo::funcoffs::obj_sight_blocking_at_; // w/o calling hook
	//case 4:
	//	return fo::funcoffs::obj_scroll_blocking_at_;
	}
}

static void __stdcall op_make_straight_path2() {
	fo::GameObject* objFrom = opHandler.arg(0).asObject();
	const ScriptValue &tileToArg = opHandler.arg(1),
	                  &typeArg = opHandler.arg(2);

	if (objFrom && tileToArg.isInt() && typeArg.isInt()) {
		DWORD tileTo = tileToArg.rawValue();
		BlockType type = (BlockType)typeArg.rawValue();

		long flag = (type == BLOCKING_TYPE_SHOOT) ? 32 : 0;
		fo::GameObject* resultObj = nullptr;
		fo::func::make_straight_path_func(objFrom, objFrom->tile, tileTo, 0, (DWORD*)&resultObj, flag, (void*)getBlockingFunc(type));
		opHandler.setReturn(resultObj);
	} else {
		OpcodeInvalidArgs("obj_blocking_line");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_make_straight_path() {
	_WRAP_OPCODE(op_make_straight_path2, 3, 1)
}

static void __stdcall op_make_path2() {
	fo::GameObject* objFrom = opHandler.arg(0).asObject();
	const ScriptValue &tileToArg = opHandler.arg(1),
	                  &typeArg = opHandler.arg(2);

	if (objFrom && tileToArg.isInt() && typeArg.isInt()) {
		DWORD tileTo = tileToArg.rawValue(),
		      func = getBlockingFunc((BlockType)typeArg.rawValue());

		// if the object is not a critter, then there is no need to check tile (tileTo) for blocking
		long checkFlag = (objFrom->IsCritter());

		char pathData[800];
		long pathLength = fo::func::make_path_func(objFrom, objFrom->tile, tileTo, pathData, checkFlag, (void*)func);
		DWORD arrayId = CreateTempArray(pathLength, 0);
		for (int i = 0; i < pathLength; i++) {
			arrays[arrayId].val[i].set((long)pathData[i]);
		}
		opHandler.setReturn(arrayId);
	} else {
		OpcodeInvalidArgs("path_find_to");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_make_path() {
	_WRAP_OPCODE(op_make_path2, 3, 1)
}

static void __stdcall op_obj_blocking_at2() {
	const ScriptValue &tileArg = opHandler.arg(0),
	                  &elevArg = opHandler.arg(1),
	                  &typeArg = opHandler.arg(2);

	if (tileArg.isInt() && elevArg.isInt() && typeArg.isInt()) {
		DWORD tile = tileArg.rawValue(),
		      elevation = elevArg.rawValue();
		BlockType type = (BlockType)typeArg.rawValue();

		fo::GameObject* resultObj = fo::func::obj_blocking_at_wrapper(0, tile, elevation, (void*)getBlockingFunc(type));
		if (resultObj && type == BLOCKING_TYPE_SHOOT && (resultObj->flags & fo::ObjectFlag::ShootThru)) { // don't know what this flag means, copy-pasted from the engine code
			// this check was added because the engine always does exactly this when using shoot blocking checks
			resultObj = nullptr;
		}
		opHandler.setReturn(resultObj);
	} else {
		OpcodeInvalidArgs("obj_blocking_tile");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_obj_blocking_at() {
	_WRAP_OPCODE(op_obj_blocking_at2, 3, 1)
}

static void __stdcall op_tile_get_objects2() {
	const ScriptValue &tileArg = opHandler.arg(0),
	                  &elevArg = opHandler.arg(1);

	if (tileArg.isInt() && elevArg.isInt()) {
		DWORD tile = tileArg.rawValue(),
		      elevation = elevArg.rawValue();
		DWORD arrayId = CreateTempArray(0, 4);
		fo::GameObject* obj = fo::func::obj_find_first_at_tile(elevation, tile);
		while (obj) {
			arrays[arrayId].push_back(reinterpret_cast<long>(obj));
			obj = fo::func::obj_find_next_at_tile();
		}
		opHandler.setReturn(arrayId);
	} else {
		OpcodeInvalidArgs("tile_get_objs");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_tile_get_objects() {
	_WRAP_OPCODE(op_tile_get_objects2, 2, 1)
}

static void __stdcall op_get_party_members2() {
	const ScriptValue &modeArg = opHandler.arg(0);

	if (modeArg.isInt()) {
		DWORD includeHidden = modeArg.rawValue();
		int actualCount = *fo::ptr::partyMemberCount;
		DWORD arrayId = CreateTempArray(0, 4);
		for (int i = 0; i < actualCount; i++) {
			fo::GameObject* obj = (*fo::ptr::partyMemberList)[i].object;
			if (includeHidden || (obj->IsCritter() && !fo::func::critter_is_dead(obj) && !(obj->flags & fo::ObjectFlag::Mouse_3d))) {
				arrays[arrayId].push_back((long)obj);
			}
		}
		opHandler.setReturn(arrayId);
	} else {
		OpcodeInvalidArgs("party_member_list");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_get_party_members() {
	_WRAP_OPCODE(op_get_party_members2, 1, 1)
}

static void mf_set_outline() {
	fo::GameObject* obj = opHandler.arg(0).object();
	int color = opHandler.arg(1).rawValue();
	obj->outline = color;
}

static void mf_get_outline() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(obj->outline);
	} else {
		OpcodeInvalidArgs("get_outline");
		opHandler.setReturn(0);
	}
}

static void mf_set_flags() {
	fo::GameObject* obj = opHandler.arg(0).object();
	int flags = opHandler.arg(1).rawValue();
	obj->flags = flags;
}

static void mf_get_flags() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(obj->flags);
	} else {
		OpcodeInvalidArgs("get_flags");
		opHandler.setReturn(0);
	}
}

static void mf_outlined_object() {
	opHandler.setReturn(*fo::ptr::outlined_object);
}

static void mf_real_dude_obj() {
	opHandler.setReturn(PartyControl::RealDudeObject());
}

static void mf_car_gas_amount() {
	opHandler.setReturn(*fo::ptr::carGasAmount);
}

static void mf_lock_is_jammed() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(fo::func::obj_lock_is_jammed(obj));
	} else {
		OpcodeInvalidArgs("lock_is_jammed");
		opHandler.setReturn(0);
	}
}

static void mf_unjam_lock() {
	fo::func::obj_unjam_lock(opHandler.arg(0).object());
}

static void mf_set_unjam_locks_time() {
	int time = opHandler.arg(0).rawValue();
	if (time < 0 || time > 127) {
		opHandler.printOpcodeError("set_unjam_locks_time() - time argument must be in the range of 0 to 127.");
		opHandler.setReturn(-1);
	} else {
		Objects::SetAutoUnjamLockTime(time);
	}
}

static void mf_get_dialog_object() {
	opHandler.setReturn(InDialog() ? *fo::ptr::dialog_target : 0);
}

static void mf_obj_under_cursor() {
	const ScriptValue &crSwitchArg = opHandler.arg(0),
	                  &inclDudeArg = opHandler.arg(1);

	if (crSwitchArg.isInt() && inclDudeArg.isInt()) {
		opHandler.setReturn(
			fo::func::object_under_mouse(crSwitchArg.asBool() ? 1 : -1, inclDudeArg.rawValue(), *fo::ptr::map_elevation)
		);
	} else {
		OpcodeInvalidArgs("obj_under_cursor");
		opHandler.setReturn(0);
	}
}

static void mf_get_loot_object() {
	opHandler.setReturn((GetLoopFlags() & INTFACELOOT) ? fo::ptr::target_stack[*fo::ptr::target_curr_stack] : 0);
}

static bool protoMaxLimitPatch = false;

static void __stdcall op_get_proto_data2() {
	const ScriptValue &pidArg = opHandler.arg(0),
	                  &offsetArg = opHandler.arg(1);

	if (pidArg.isInt() && offsetArg.isInt()) {
		long result = -1;
		fo::Proto* protoPtr;
		int pid = pidArg.rawValue();
		if (fo::util::CheckProtoID(pid) && fo::func::proto_ptr(pid, &protoPtr) != result) {
			result = *(long*)((BYTE*)protoPtr + offsetArg.rawValue());
		} else {
			opHandler.printOpcodeError(protoFailedLoad, "get_proto_data", pid);
		}
		opHandler.setReturn(result);
	} else {
		OpcodeInvalidArgs("get_proto_data");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_get_proto_data() {
	_WRAP_OPCODE(op_get_proto_data2, 2, 1)
}

static void __stdcall op_set_proto_data2() {
	const ScriptValue &pidArg = opHandler.arg(0),
	                  &offsetArg = opHandler.arg(1),
	                  &valueArg = opHandler.arg(2);

	if (pidArg.isInt() && offsetArg.isInt() && valueArg.isInt()) {
		fo::Proto* protoPtr;
		int pid = pidArg.rawValue();
		if (fo::util::CheckProtoID(pid) && fo::func::proto_ptr(pid, &protoPtr) != -1) {
			*(long*)((BYTE*)protoPtr + offsetArg.rawValue()) = valueArg.rawValue();
			if (!protoMaxLimitPatch) {
				Objects::LoadProtoAutoMaxLimit();
				protoMaxLimitPatch = true;
			}
		} else {
			opHandler.printOpcodeError(protoFailedLoad, "set_proto_data", pid);
		}
	} else {
		OpcodeInvalidArgs("set_proto_data");
	}
}

static void __declspec(naked) op_set_proto_data() {
	_WRAP_OPCODE(op_set_proto_data2, 3, 0)
}

static void mf_get_object_data() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	const ScriptValue &offsetArg = opHandler.arg(1);

	if (obj && offsetArg.isInt()) {
		DWORD* object_ptr = (DWORD*)obj;
		opHandler.setReturn(*(long*)((BYTE*)object_ptr + offsetArg.rawValue()));
	} else {
		OpcodeInvalidArgs("get_object_data");
		opHandler.setReturn(0);
	}
}

static void mf_set_object_data() {
	DWORD* object_ptr = (DWORD*)opHandler.arg(0).rawValue();
	*(long*)((BYTE*)object_ptr + opHandler.arg(1).rawValue()) = opHandler.arg(2).rawValue();
}

static void mf_set_unique_id() {
	fo::GameObject* obj = opHandler.arg(0).object();
	long id;
	if (opHandler.arg(1).rawValue() == -1) {
		id = fo::func::new_obj_id();
		obj->id = id;
	} else {
		id = Objects::SetObjectUniqueID(obj);
	}
	opHandler.setReturn(id);
}

static void mf_objects_in_radius() {
	const ScriptValue &tileArg = opHandler.arg(0),
	                  &radiusArg = opHandler.arg(1),
	                  &elevArg = opHandler.arg(2);

	if (tileArg.isInt() && radiusArg.isInt() && elevArg.isInt()) {
		long type = -1;
		if (opHandler.numArgs() > 3) {
			const ScriptValue &typeArg = opHandler.arg(3);
			if (!typeArg.isInt()) goto invalidArgs;
			type = typeArg.rawValue();
		}
		long radius = radiusArg.rawValue();
		if (radius <= 0) radius = 1; else if (radius > 50) radius = 50;
		long elev = elevArg.rawValue();
		if (elev < 0) elev = 0; else if (elev > 2) elev = 2;

		std::vector<fo::GameObject*> objects;
		objects.reserve(25);
		fo::util::GetObjectsTileRadius(objects, tileArg.rawValue(), radius, elev, type);
		size_t sz = objects.size();
		DWORD id = CreateTempArray(sz, 0);
		for (size_t i = 0; i < sz; i++) {
			arrays[id].val[i].set((long)objects[i]);
		}
		opHandler.setReturn(id);
	} else {
invalidArgs:
		OpcodeInvalidArgs("get_objects_at_radius");
		opHandler.setReturn(0);
	}
}

static void mf_npc_engine_level_up() {
	if (opHandler.arg(0).asBool()) {
		if (!npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0x840F); // enable
		npcEngineLevelUp = true;
	} else {
		if (npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0xE990);
		npcEngineLevelUp = false;
	}
}

static void mf_obj_is_openable() {
	fo::GameObject* object = opHandler.arg(0).asObject();
	if (object) {
		opHandler.setReturn(fo::util::ObjIsOpenable(object));
	} else {
		OpcodeInvalidArgs("obj_is_openable");
		opHandler.setReturn(0);
	}
}

}
