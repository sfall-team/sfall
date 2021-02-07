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

#include "Combat.h"
#include "Inventory.h"
#include "Objects.h"
#include "PartyControl.h"

#define exec_script_proc(script, proc) __asm {  \
	__asm mov  eax, script                      \
	__asm mov  edx, proc                        \
	__asm call exec_script_proc_                \
}

static void __stdcall op_remove_script2() {
	TGameObj* object = opHandler.arg(0).asObject();
	if (object) {
		if (object->scriptId != 0xFFFFFFFF) {
			fo_scr_remove(object->scriptId);
			object->scriptId = 0xFFFFFFFF;
		}
	} else {
		OpcodeInvalidArgs("remove_script");
	}
}

static void __declspec(naked) op_remove_script() {
	_WRAP_OPCODE(op_remove_script2, 1, 0)
}

static void __stdcall op_set_script2() {
	using Scripts::start;
	using Scripts::map_enter_p_proc;

	TGameObj* object = opHandler.arg(0).asObject();
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
			fo_scr_remove(object->scriptId);
			object->scriptId = 0xFFFFFFFF;
		}
		if (object->IsCritter()) {
			scriptType = Scripts::SCRIPT_CRITTER;
		} else {
			scriptType = Scripts::SCRIPT_ITEM;
		}
		fo_obj_new_sid_inst(object, scriptType, scriptIndex);
	
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
	using Scripts::start;

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
		TScript* scriptPtr;
		if (fo_scr_new(&scriptId, Scripts::SCRIPT_SPATIAL) == -1 || fo_scr_ptr(scriptId, &scriptPtr) == -1) return;

		// set spatial script properties:
		scriptPtr->scriptIdx = scriptIndex - 1;
		scriptPtr->elevationAndTile = (elevation << 29) & 0xE0000000 | tile;
		scriptPtr->spatialRadius = radius;

		// this will load appropriate script program and link it to the script instance we just created:
		exec_script_proc(scriptId, start);

		opHandler.setReturn(fo_scr_find_obj_from_program(scriptPtr->program));
	} else {
		OpcodeInvalidArgs("create_spatial");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_create_spatial() {
	_WRAP_OPCODE(op_create_spatial2, 4, 1)
}

static void mf_spatial_radius() {
	TGameObj* spatialObj = opHandler.arg(0).asObject();
	if (spatialObj) {
		TScript* script;
		if (fo_scr_ptr(spatialObj->scriptId, &script) != -1) {
			opHandler.setReturn(script->spatialRadius);
		}
	} else {
		OpcodeInvalidArgs("spatial_radius");
		opHandler.setReturn(0);
	}
}

static void __stdcall op_get_script2() {
	TGameObj* object = opHandler.arg(0).asObject();
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
	TGameObj* critter = opHandler.arg(0).asObject();
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
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(obj->item.ammoPid);
	} else {
		OpcodeInvalidArgs("get_weapon_ammo_pid");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) op_get_weapon_ammo_pid() {
	_WRAP_OPCODE(op_get_weapon_ammo_pid2, 1, 1)
}

static void __stdcall op_set_weapon_ammo_pid2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &pidArg = opHandler.arg(1);

	if (obj && pidArg.isInt()) {
		obj->item.ammoPid = pidArg.rawValue();
	} else {
		OpcodeInvalidArgs("set_weapon_ammo_pid");
	}
}

static void __declspec(naked) op_set_weapon_ammo_pid() {
	_WRAP_OPCODE(op_set_weapon_ammo_pid2, 2, 0)
}

static void __stdcall op_get_weapon_ammo_count2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(obj->item.charges);
	} else {
		OpcodeInvalidArgs("get_weapon_ammo_count");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_weapon_ammo_count() {
	_WRAP_OPCODE(op_get_weapon_ammo_count2, 1, 1)
}

static void __stdcall op_set_weapon_ammo_count2() {
	TGameObj* obj = opHandler.arg(0).asObject();
	const ScriptValue &countArg = opHandler.arg(1);

	if (obj && countArg.isInt()) {
		obj->item.charges = countArg.rawValue();
	} else {
		OpcodeInvalidArgs("set_weapon_ammo_count");
	}
}

static void __declspec(naked) op_set_weapon_ammo_count() {
	_WRAP_OPCODE(op_set_weapon_ammo_count2, 2, 0)
}

enum {
	BLOCKING_TYPE_BLOCK  = 0,
	BLOCKING_TYPE_SHOOT  = 1,
	BLOCKING_TYPE_AI     = 2,
	BLOCKING_TYPE_SIGHT  = 3,
	BLOCKING_TYPE_SCROLL = 4
};

static DWORD getBlockingFunc(DWORD type) {
	switch (type) {
		case BLOCKING_TYPE_BLOCK: default:
			return obj_blocking_at_;       // with calling hook
		case BLOCKING_TYPE_SHOOT:
			return obj_shoot_blocking_at_; // w/o calling hook
		case BLOCKING_TYPE_AI:
			return obj_ai_blocking_at_;    // w/o calling hook
		case BLOCKING_TYPE_SIGHT:
			return obj_sight_blocking_at_; // w/o calling hook
		//case 4:
		//	return obj_scroll_blocking_at_;
	}
}

static void __stdcall op_make_straight_path2() {
	TGameObj* objFrom = opHandler.arg(0).asObject();
	const ScriptValue &tileToArg = opHandler.arg(1),
					  &typeArg = opHandler.arg(2);

	if (objFrom && tileToArg.isInt() && typeArg.isInt()) {
		DWORD tileTo = tileToArg.rawValue(),
			  type = typeArg.rawValue();

		long flag = (type == BLOCKING_TYPE_SHOOT) ? 32 : 0;
		TGameObj* resultObj = nullptr;
		fo_make_straight_path_func(objFrom, objFrom->tile, tileTo, 0, (DWORD*)&resultObj, flag, (void*)getBlockingFunc(type));
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
	TGameObj* objFrom = opHandler.arg(0).asObject();
	const ScriptValue &tileToArg = opHandler.arg(1),
					  &typeArg = opHandler.arg(2);

	if (objFrom && tileToArg.isInt() && typeArg.isInt()) {
		DWORD tileTo = tileToArg.rawValue(),
			  type = typeArg.rawValue(),
			  func = getBlockingFunc(type);

		// if the object is not a critter, then there is no need to check tile (tileTo) for blocking
		long checkFlag = (objFrom->IsCritter());

		char pathData[800];
		long pathLength = fo_make_path_func(objFrom, objFrom->tile, tileTo, pathData, checkFlag, (void*)func);
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
			  elevation = elevArg.rawValue(),
			  type = typeArg.rawValue();

		TGameObj* resultObj = obj_blocking_at_wrapper(0, tile, elevation, (void*)getBlockingFunc(type));
		if (resultObj && type == BLOCKING_TYPE_SHOOT && (resultObj->flags & ObjectFlag::ShootThru)) { // don't know what this flag means, copy-pasted from the engine code
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
		TGameObj* obj = fo_obj_find_first_at_tile(elevation, tile);
		while (obj) {
			arrays[arrayId].push_back(reinterpret_cast<long>(obj));
			obj = fo_obj_find_next_at_tile();
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
		int actualCount = *ptr_partyMemberCount;
		DWORD arrayId = CreateTempArray(0, 4);
		DWORD* partyMemberList = *ptr_partyMemberList;
		for (int i = 0; i < actualCount; i++) {
			TGameObj* obj = reinterpret_cast<TGameObj*>(partyMemberList[i * 4]);
			if (includeHidden || (obj->IsCritter() && !fo_critter_is_dead(obj) && !(obj->flags & ObjectFlag::Mouse_3d))) {
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

static void __declspec(naked) op_art_exists() {
	__asm {
		_GET_ARG_INT(fail);
		call art_exists_;
		mov  edx, eax;
end:
		mov  eax, ebx;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
fail:
		xor  edx, edx; // return 0
		jmp  end;
	}
}

static void __stdcall op_obj_is_carrying_obj2() {
	int num = 0;
	const ScriptValue &invenObjArg = opHandler.arg(0),
					  &itemObjArg = opHandler.arg(1);

	TGameObj *invenObj = invenObjArg.asObject(),
			 *itemObj = itemObjArg.asObject();
	if (invenObj != nullptr && itemObj != nullptr) {
		for (int i = 0; i < invenObj->invenSize; i++) {
			if (invenObj->invenTable[i].object == itemObj) {
				num = invenObj->invenTable[i].count;
				break;
			}
		}
	} else {
		OpcodeInvalidArgs("obj_is_carrying_obj");
	}
	opHandler.setReturn(num);
}

static void __declspec(naked) op_obj_is_carrying_obj() {
	_WRAP_OPCODE(op_obj_is_carrying_obj2, 2, 1)
}

static void mf_critter_inven_obj2() {
	TGameObj* critter = opHandler.arg(0).asObject();
	const ScriptValue &slotArg = opHandler.arg(1);

	if (critter && slotArg.isInt()) {
		int slot = slotArg.rawValue();
		switch (slot) {
		case 0:
			opHandler.setReturn(fo_inven_worn(critter));
			break;
		case 1:
			opHandler.setReturn(fo_inven_right_hand(critter));
			break;
		case 2:
			opHandler.setReturn(fo_inven_left_hand(critter));
			break;
		case -2:
			opHandler.setReturn(critter->invenSize);
			break;
		default:
			opHandler.printOpcodeError("critter_inven_obj2() - invalid type.");
		}
	} else {
		OpcodeInvalidArgs("critter_inven_obj2");
		opHandler.setReturn(0);
	}
}

static void mf_set_outline() {
	TGameObj* obj = opHandler.arg(0).object();
	int color = opHandler.arg(1).rawValue();
	obj->outline = color;
}

static void mf_get_outline() {
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(obj->outline);
	} else {
		OpcodeInvalidArgs("get_outline");
		opHandler.setReturn(0);
	}
}

static void mf_set_flags() {
	TGameObj* obj = opHandler.arg(0).object();
	int flags = opHandler.arg(1).rawValue();
	obj->flags = flags;
}

static void mf_get_flags() {
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(obj->flags);
	} else {
		OpcodeInvalidArgs("get_flags");
		opHandler.setReturn(0);
	}
}

static void mf_outlined_object() {
	opHandler.setReturn(*ptr_outlined_object);
}

static void mf_item_weight() {
	TGameObj* item = opHandler.arg(0).asObject();
	if (item) {
		opHandler.setReturn(fo_item_weight(item));
	} else {
		OpcodeInvalidArgs("item_weight");
		opHandler.setReturn(0);
	}
}

static void mf_real_dude_obj() {
	opHandler.setReturn(PartyControl_RealDudeObject());
}

static void mf_car_gas_amount() {
	opHandler.setReturn(*ptr_carGasAmount);
}

static void mf_lock_is_jammed() {
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(fo_obj_lock_is_jammed(obj));
	} else {
		OpcodeInvalidArgs("lock_is_jammed");
		opHandler.setReturn(0);
	}
}

static void mf_unjam_lock() {
	fo_obj_unjam_lock(opHandler.arg(0).object());
}

static void mf_set_unjam_locks_time() {
	int time = opHandler.arg(0).rawValue();
	if (time < 0 || time > 127) {
		opHandler.printOpcodeError("set_unjam_locks_time() - time argument must be in the range of 0 to 127.");
		opHandler.setReturn(-1);
	} else {
		Objects_SetAutoUnjamLockTime(time);
	}
}

static void mf_get_current_inven_size() {
	TGameObj* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(sfgame_item_total_size(obj));
	} else {
		OpcodeInvalidArgs("get_current_inven_size");
		opHandler.setReturn(0);
	}
}

static void mf_get_dialog_object() {
	opHandler.setReturn(InDialog() ? *ptr_dialog_target : 0);
}

static void mf_obj_under_cursor() {
	const ScriptValue &crSwitchArg = opHandler.arg(0),
					  &inclDudeArg = opHandler.arg(1);

	if (crSwitchArg.isInt() && inclDudeArg.isInt()) {
		opHandler.setReturn((*ptr_gmouse_3d_current_mode != 0)
							? fo_object_under_mouse(crSwitchArg.asBool() ? 1 : -1, inclDudeArg.rawValue(), *ptr_map_elevation)
							: 0);
	} else {
		OpcodeInvalidArgs("obj_under_cursor");
		opHandler.setReturn(0);
	}
}

static void mf_get_loot_object() {
	opHandler.setReturn((GetLoopFlags() & INTFACELOOT) ? ptr_target_stack[*ptr_target_curr_stack] : 0);
}

static const char* failedLoad = "%s() - failed to load a prototype ID: %d";
static bool protoMaxLimitPatch = false;

static void __stdcall op_get_proto_data2() {
	const ScriptValue &pidArg = opHandler.arg(0),
					  &offsetArg = opHandler.arg(1);

	if (pidArg.isInt() && offsetArg.isInt()) {
		sProto* protoPtr;
		int pid = pidArg.rawValue();
		int result = fo_proto_ptr(pid, &protoPtr);
		if (result != -1) {
			result = *(long*)((BYTE*)protoPtr + offsetArg.rawValue());
		} else {
			opHandler.printOpcodeError(failedLoad, "get_proto_data", pid);
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
		sProto* protoPtr;
		int pid = pidArg.rawValue();
		if (fo_proto_ptr(pid, &protoPtr) != -1) {
			*(long*)((BYTE*)protoPtr + offsetArg.rawValue()) = valueArg.rawValue();
			if (!protoMaxLimitPatch) {
				Objects_LoadProtoAutoMaxLimit();
				protoMaxLimitPatch = true;
			}
		} else {
			opHandler.printOpcodeError(failedLoad, "set_proto_data", pid);
		}
	} else {
		OpcodeInvalidArgs("set_proto_data");
	}
}

static void __declspec(naked) op_set_proto_data() {
	_WRAP_OPCODE(op_set_proto_data2, 3, 0)
}

static void mf_get_object_data() {
	TGameObj* obj = opHandler.arg(0).asObject();
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
	TGameObj* obj = opHandler.arg(0).object();
	long id;
	if (opHandler.arg(1).rawValue() == -1) {
		id = fo_new_obj_id();
		obj->id = id;
	} else {
		id = Objects_SetObjectUniqueID(obj);
	}
	opHandler.setReturn(id);
}

static void mf_objects_in_radius() {
	const ScriptValue &tileArg = opHandler.arg(0),
					  &radiusArg = opHandler.arg(1),
					  &elevArg = opHandler.arg(2);

	DWORD id = 0;
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

		std::vector<TGameObj*> objects;
		objects.reserve(25);
		GetObjectsTileRadius(objects, tileArg.rawValue(), radius, elev, type);
		size_t sz = objects.size();
		id = CreateTempArray(sz, 0);
		for (size_t i = 0; i < sz; i++) {
			arrays[id].val[i].set((long)objects[i]);
		}
	} else {
invalidArgs:
		OpcodeInvalidArgs("get_objects_at_radius");
	}
	opHandler.setReturn(id);
}
