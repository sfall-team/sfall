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
#include "..\..\Combat.h"
#include "..\..\CritterStats.h"
#include "..\..\Drugs.h"
#include "..\..\Explosions.h"
#include "..\..\Inventory.h"
#include "..\..\LoadGameHook.h"
#include "..\..\Objects.h"
#include "..\..\PartyControl.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "Objects.h"

namespace sfall
{
namespace script
{

#define exec_script_proc(script, proc) __asm {  \
	__asm mov  eax, script                      \
	__asm mov  edx, proc                        \
	__asm call fo::funcoffs::exec_script_proc_  \
}

void op_remove_script(OpcodeContext& ctx) {
	auto object = ctx.arg(0).object();
	if (object->scriptId != 0xFFFFFFFF) {
		fo::func::scr_remove(object->scriptId);
		object->scriptId = 0xFFFFFFFF;
	}
}

void op_set_script(OpcodeContext& ctx) {
	using fo::Scripts::start;
	using fo::Scripts::map_enter_p_proc;

	long scriptType;
	auto object = ctx.arg(0).object();
	unsigned long valArg = ctx.arg(1).rawValue();

	long scriptIndex = valArg & ~0xF0000000;
	if (scriptIndex == 0 || valArg > 0x8FFFFFFF) { // negative values are not allowed
		ctx.printOpcodeError("%s() - the script index number is incorrect.", ctx.getOpcodeName());
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
}

void op_create_spatial(OpcodeContext& ctx) {
	using fo::Scripts::start;

	DWORD scriptIndex = ctx.arg(0).rawValue(),
		tile = ctx.arg(1).rawValue(),
		elevation = ctx.arg(2).rawValue(),
		radius = ctx.arg(3).rawValue();

	long scriptId;
	fo::ScriptInstance* scriptPtr;
	if (fo::func::scr_new(&scriptId, fo::Scripts::ScriptTypes::SCRIPT_SPATIAL) == -1 || fo::func::scr_ptr(scriptId, &scriptPtr) == -1) return;

	// set spatial script properties:
	scriptPtr->scriptIdx = scriptIndex - 1;
	scriptPtr->elevationAndTile = (elevation << 29) & 0xE0000000 | tile;
	scriptPtr->spatialRadius = radius;

	// this will load appropriate script program and link it to the script instance we just created:
	exec_script_proc(scriptId, start);

	ctx.setReturn(fo::func::scr_find_obj_from_program(scriptPtr->program));
}

void mf_spatial_radius(OpcodeContext& ctx) {
	auto spatialObj = ctx.arg(0).object();
	fo::ScriptInstance* script;
	if (fo::func::scr_ptr(spatialObj->scriptId, &script) != -1) {
		ctx.setReturn(script->spatialRadius);
	}
}

void op_get_script(OpcodeContext& ctx) {
	auto scriptIndex = ctx.arg(0).object()->scriptIndex;
	ctx.setReturn((scriptIndex >= 0) ? ++scriptIndex : 0);
}

void op_set_critter_burst_disable(OpcodeContext& ctx) {
	SetNoBurstMode(ctx.arg(0).object(), ctx.arg(1).asBool());
}

void op_get_weapon_ammo_pid(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	ctx.setReturn(obj->item.ammoPid);
}

void op_set_weapon_ammo_pid(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	obj->item.ammoPid = ctx.arg(1).rawValue();
}

void op_get_weapon_ammo_count(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	ctx.setReturn(obj->item.charges);
}

void op_set_weapon_ammo_count(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	obj->item.charges = ctx.arg(1).rawValue();
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
		return fo::funcoffs::obj_blocking_at_;       // with calling hook
	case BLOCKING_TYPE_SHOOT:
		return fo::funcoffs::obj_shoot_blocking_at_; // w/o calling hook
	case BLOCKING_TYPE_AI:
		return fo::funcoffs::obj_ai_blocking_at_;    // w/o calling hook
	case BLOCKING_TYPE_SIGHT:
		return fo::funcoffs::obj_sight_blocking_at_; // w/o calling hook
	//case 4:
	//	return obj_scroll_blocking_at_;
	}
}

void op_make_straight_path(OpcodeContext& ctx) {
	auto objFrom = ctx.arg(0).object();
	DWORD tileTo = ctx.arg(1).rawValue(),
		  type = ctx.arg(2).rawValue();

	long flag = (type == BLOCKING_TYPE_SHOOT) ? 32 : 0;
	DWORD resultObj = 0;
	fo::func::make_straight_path_func(objFrom, objFrom->tile, tileTo, 0, &resultObj, flag, (void*)getBlockingFunc(type));
	ctx.setReturn(resultObj);
}

void op_make_path(OpcodeContext& ctx) {
	auto objFrom = ctx.arg(0).object();
	auto tileTo = ctx.arg(1).rawValue(),
		 type = ctx.arg(2).rawValue();
	auto func = getBlockingFunc(type);

	// if the object is not a critter, then there is no need to check tile (tileTo) for blocking
	long checkFlag = (objFrom->IsCritter());

	char pathData[800];
	long pathLength = fo::func::make_path_func(objFrom, objFrom->tile, tileTo, pathData, checkFlag, (void*)func);
	auto arrayId = CreateTempArray(pathLength, 0);
	for (int i = 0; i < pathLength; i++) {
		arrays[arrayId].val[i].set((long)pathData[i]);
	}
	ctx.setReturn(arrayId);
}

void op_obj_blocking_at(OpcodeContext& ctx) {
	DWORD tile = ctx.arg(0).rawValue(),
		  elevation = ctx.arg(1).rawValue(),
		  type = ctx.arg(2).rawValue();

	fo::GameObject* resultObj = fo::func::obj_blocking_at_wrapper(0, tile, elevation, (void*)getBlockingFunc(type));
	if (resultObj && type == BLOCKING_TYPE_SHOOT && (resultObj->flags & fo::ObjectFlag::ShootThru)) { // don't know what this flag means, copy-pasted from the engine code
		// this check was added because the engine always does exactly this when using shoot blocking checks
		resultObj = nullptr;
	}
	ctx.setReturn(resultObj);
}

void op_tile_get_objects(OpcodeContext& ctx) {
	DWORD tile = ctx.arg(0).rawValue(),
		elevation = ctx.arg(1).rawValue();
	DWORD arrayId = CreateTempArray(0, 4);
	auto obj = fo::func::obj_find_first_at_tile(elevation, tile);
	while (obj) {
		arrays[arrayId].push_back(reinterpret_cast<long>(obj));
		obj = fo::func::obj_find_next_at_tile();
	}
	ctx.setReturn(arrayId);
}

void op_get_party_members(OpcodeContext& ctx) {
	auto includeHidden = ctx.arg(0).rawValue();
	int actualCount = fo::var::partyMemberCount;
	DWORD arrayId = CreateTempArray(0, 4);
	auto partyMemberList = fo::var::partyMemberList;
	for (int i = 0; i < actualCount; i++) {
		auto obj = reinterpret_cast<fo::GameObject*>(partyMemberList[i * 4]);
		if (includeHidden || (obj->IsCritter() && !fo::func::critter_is_dead(obj) && !(obj->flags & fo::ObjectFlag::Mouse_3d))) {
			arrays[arrayId].push_back((long)obj);
		}
	}
	ctx.setReturn(arrayId);
}

void op_art_exists(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::art_exists(ctx.arg(0).rawValue()));
}

void op_obj_is_carrying_obj(OpcodeContext& ctx) {
	int num = 0;
	const ScriptValue &invenObjArg = ctx.arg(0),
		&itemObjArg = ctx.arg(1);

	fo::GameObject *invenObj = invenObjArg.object(),
		*itemObj = itemObjArg.object();
	if (invenObj != nullptr && itemObj != nullptr) {
		for (int i = 0; i < invenObj->invenSize; i++) {
			if (invenObj->invenTable[i].object == itemObj) {
				num = invenObj->invenTable[i].count;
				break;
			}
		}
	}
	ctx.setReturn(num);
}

void mf_critter_inven_obj2(OpcodeContext& ctx) {
	fo::GameObject* critter = ctx.arg(0).object();
	int slot = ctx.arg(1).rawValue();
	switch (slot) {
	case 0:
		ctx.setReturn(fo::func::inven_worn(critter));
		break;
	case 1:
		ctx.setReturn(fo::func::inven_right_hand(critter));
		break;
	case 2:
		ctx.setReturn(fo::func::inven_left_hand(critter));
		break;
	case -2:
		ctx.setReturn(critter->invenSize);
		break;
	default:
		ctx.printOpcodeError("%s() - invalid type.", ctx.getMetaruleName());
	}
}

void mf_set_outline(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	int color = ctx.arg(1).rawValue();
	obj->outline = color;
}

void mf_get_outline(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	ctx.setReturn(obj->outline);
}

void mf_set_flags(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	int flags = ctx.arg(1).rawValue();
	obj->flags = flags;
}

void mf_get_flags(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	ctx.setReturn(obj->flags);
}

void mf_outlined_object(OpcodeContext& ctx) {
	ctx.setReturn(fo::var::outlined_object);
}

void mf_item_weight(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::item_weight(ctx.arg(0).object()));
}

void mf_set_dude_obj(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	if (obj == nullptr || obj->IsCritter()) {
		//if (!InCombat && obj && obj != PartyControl::RealDudeObject()) {
		//	ctx.printOpcodeError("%s() - controlling of the critter is only allowed in combat mode.", ctx.getMetaruleName());
		//} else {
			PartyControl::SwitchToCritter(obj);
		//}
	} else {
		ctx.printOpcodeError("%s() - the object is not a critter.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_real_dude_obj(OpcodeContext& ctx) {
	ctx.setReturn(PartyControl::RealDudeObject());
}

void mf_car_gas_amount(OpcodeContext& ctx) {
	ctx.setReturn(fo::var::carGasAmount);
}

void mf_lock_is_jammed(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::obj_lock_is_jammed(ctx.arg(0).object()));
}

void mf_unjam_lock(OpcodeContext& ctx) {
	fo::func::obj_unjam_lock(ctx.arg(0).object());
}

void mf_set_unjam_locks_time(OpcodeContext& ctx) {
	int time = ctx.arg(0).rawValue();
	if (time < 0 || time > 127) {
		ctx.printOpcodeError("%s() - time argument must be in the range of 0 to 127.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	} else {
		Objects::SetAutoUnjamLockTime(time);
	}
}

void mf_item_make_explosive(OpcodeContext& ctx) {
	DWORD pid = ctx.arg(0).rawValue();
	DWORD pidActive = ctx.arg(1).rawValue();
	DWORD min = ctx.arg(2).rawValue();
	DWORD max = (ctx.numArgs() == 4) ? ctx.arg(3).rawValue() : min;

	if (min > max) {
		max = min;
		ctx.printOpcodeError("%s() - Warning: value of max argument is less than the min argument.", ctx.getMetaruleName());
	}

	if (pid > 0 && pidActive > 0) {
		Explosions::AddToExplosives(pid, pidActive, min, max);
	} else {
		ctx.printOpcodeError("%s() - invalid PID number, must be greater than 0.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_get_current_inven_size(OpcodeContext& ctx) {
	ctx.setReturn(sf_item_total_size(ctx.arg(0).object()));
}

void mf_get_dialog_object(OpcodeContext& ctx) {
	ctx.setReturn(InDialog() ? fo::var::dialog_target : 0);
}

void mf_obj_under_cursor(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::object_under_mouse(ctx.arg(0).asBool() ? 1 : -1, ctx.arg(1).rawValue(), fo::var::map_elevation));
}

void mf_get_loot_object(OpcodeContext& ctx) {
	ctx.setReturn((GetLoopFlags() & INTFACELOOT) ? fo::var::target_stack[fo::var::target_curr_stack] : 0);
}

static const char* failedLoad = "%s() - failed to load a prototype ID: %d";
static bool protoMaxLimitPatch = false;

void op_get_proto_data(OpcodeContext& ctx) {
	fo::Proto* protoPtr;
	int pid = ctx.arg(0).rawValue();
	int result = fo::func::proto_ptr(pid, &protoPtr);
	if (result != -1) {
		result = *(long*)((BYTE*)protoPtr + ctx.arg(1).rawValue());
	} else {
		ctx.printOpcodeError(failedLoad, ctx.getOpcodeName(), pid);
	}
	ctx.setReturn(result);
}

void op_set_proto_data(OpcodeContext& ctx) {
	int pid = ctx.arg(0).rawValue();
	if (CritterStats::SetProtoData(pid, ctx.arg(1).rawValue(), ctx.arg(2).rawValue()) != -1) {
		if (!protoMaxLimitPatch) {
			Objects::LoadProtoAutoMaxLimit();
			protoMaxLimitPatch = true;
		}
	} else {
		ctx.printOpcodeError(failedLoad, ctx.getOpcodeName(), pid);
	}
}

void mf_get_object_data(OpcodeContext& ctx) {
	DWORD* object_ptr = (DWORD*)ctx.arg(0).rawValue();
	ctx.setReturn(*(long*)((BYTE*)object_ptr + ctx.arg(1).rawValue()));
}

void mf_set_object_data(OpcodeContext& ctx) {
	DWORD* object_ptr = (DWORD*)ctx.arg(0).rawValue();
	*(long*)((BYTE*)object_ptr + ctx.arg(1).rawValue()) = ctx.arg(2).rawValue();
}

void mf_get_object_ai_data(OpcodeContext& ctx) {
	fo::AIcap* cap = fo::func::ai_cap(ctx.arg(0).object());
	DWORD arrayId, value = -1;
	switch (ctx.arg(1).rawValue()) {
	case 0:
		value = cap->aggression;
		break;
	case 1:
		value = cap->area_attack_mode;
		break;
	case 2:
		value = cap->attack_who;
		break;
	case 3:
		value = cap->best_weapon;
		break;
	case 4:
		value = cap->chem_use;
		break;
	case 5:
		value = cap->disposition;
		break;
	case 6:
		value = cap->distance;
		break;
	case 7:
		value = cap->max_dist;
		break;
	case 8:
		value = cap->min_hp;
		break;
	case 9:
		value = cap->min_to_hit;
		break;
	case 10:
		value = cap->hurt_too_much; // DAM_BLIND/DAM_CRIP_* flags
		break;
	case 11:
		value = cap->run_away_mode;
		break;
	case 12:
		value = cap->secondary_freq;
		break;
	case 13:
		value = cap->called_freq;
		break;
	case 14:
		arrayId = CreateTempArray(3, 0);
		arrays[arrayId].val[0].set(cap->chem_primary_desire);
		arrays[arrayId].val[1].set(cap->chem_primary_desire1);
		arrays[arrayId].val[2].set(cap->chem_primary_desire2);
		value = arrayId;
		break;
	default:
		ctx.printOpcodeError("%s() - invalid value for AI argument.", ctx.getMetaruleName());
	}
	ctx.setReturn(value);
}

void mf_set_drugs_data(OpcodeContext& ctx) {
	int type = ctx.arg(0).rawValue();
	int pid = ctx.arg(1).rawValue();
	int val = ctx.arg(2).rawValue();
	int result;
	switch (type) {
	case 0:
		result = Drugs::SetDrugNumEffect(pid, val);
		break;
	case 1:
		result = Drugs::SetDrugAddictTimeOff(pid, val);
		break;
	default:
		ctx.printOpcodeError("%s() - invalid value for type argument.", ctx.getMetaruleName());
		return;
	}
	if (result) {
		ctx.printOpcodeError("%s() - drug PID not found in the configuration file.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_set_unique_id(OpcodeContext& ctx) {
	fo::GameObject* obj = ctx.arg(0).object();
	long id;
	if (ctx.arg(1).rawValue() == -1) {
		id = fo::func::new_obj_id();
		obj->id = id;
	} else {
		id = Objects::SetObjectUniqueID(obj);
	}
	ctx.setReturn(id);
}

void mf_objects_in_radius(OpcodeContext& ctx) {
	long radius = ctx.arg(1).rawValue();
	if (radius <= 0) radius = 1; else if (radius > 50) radius = 50;
	long elev = ctx.arg(2).rawValue();
	if (elev < 0) elev = 0; else if (elev > 2) elev = 2;
	long type = (ctx.numArgs() > 3) ? ctx.arg(3).rawValue() : -1;

	std::vector<fo::GameObject*> objects;
	objects.reserve(25);
	fo::GetObjectsTileRadius(objects, ctx.arg(0).rawValue(), radius, elev, type);
	size_t sz = objects.size();
	DWORD id = CreateTempArray(sz, 0);
	for (size_t i = 0; i < sz; i++) {
		arrays[id].val[i].set((long)objects[i]);
	}
	ctx.setReturn(id);
}

}
}
