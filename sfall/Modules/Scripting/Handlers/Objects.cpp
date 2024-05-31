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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\Combat.h"
#include "..\..\CritterStats.h"
#include "..\..\Drugs.h"
#include "..\..\Explosions.h"
#include "..\..\LoadGameHook.h"
#include "..\..\Objects.h"
#include "..\..\PartyControl.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"

#include "Objects.h"

namespace sfall
{
namespace script
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
		SafeMemSet(0x495CE3, CodeType::Nop, 5);   // Check if npc had "early" level up before the next scheduled one, resets the "early" flag
		SafeMemSet(0x495CEC, CodeType::Nop, 6);   // Related to above
		SafeWrite8(0x495CFB, CodeType::JumpShort); // Skip random roll for early level up
		__asm mov dword ptr [ebp + 0x150 - 0x28 + 16], 255; // set counter for exit loop
	} else {
		if (!onceNpcLoop) {
			SafeWrite32(0x495C50, 0x01FCE9); // set goto next member
			onceNpcLoop = true;
		}
	}
	__asm pop edx;
}

void op_inc_npc_level(OpcodeContext& ctx) {
	nameNPCToInc = ctx.arg(0).asString();
	pidNPCToInc = ctx.arg(0).asInt(); // set to 0 if passing npc name
	if (pidNPCToInc == 0 && nameNPCToInc[0] == 0) return;

	HookCall(0x495BF1, IncNPCLevel);  // Replace the debug output
	__asm call fo::funcoffs::partyMemberIncLevels_;
	onceNpcLoop = false;

	// restore code
	SafeWrite32(0x495BF1 + 1, 0x031352); // restore debug_printf call
	SafeWrite32(0x495C50, 0x01FB840F);
	__int64 data = 0x01D48C0F;
	SafeWriteBytes(0x495C77, (BYTE*)&data, 6);
	//SafeWrite16(0x495C8C, 0x8D0F);
	//SafeWrite32(0x495C8E, 0x000001BF);
	data = 0x0169E9;
	SafeWriteBytes(0x495CE3, (BYTE*)&data, 5);
	data = 0x015F850F;
	SafeWriteBytes(0x495CEC, (BYTE*)&data, 6);
	SafeWrite8(0x495CFB, CodeType::JumpZ);
}

void op_get_npc_level(OpcodeContext& ctx) {
	int level = -1;
	DWORD findPid = ctx.arg(0).asInt(); // set to 0 if passing npc name
	const char *critterName, *name = ctx.arg(0).asString();

	if (findPid || name[0] != 0) {
		DWORD pid = 0;
		auto members = fo::var::partyMemberList;
		for (DWORD i = 0; i < fo::var::partyMemberCount; i++) {
			if (!findPid) {
				critterName = fo::func::critter_name(members[i].object);
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
			DWORD* pidList = fo::var::partyMemberPidList;
			for (DWORD j = 0; j < fo::var::partyMemberMaxCount; j++) {
				if (pidList[j] == pid) {
					level = fo::var::partyMemberLevelUpInfoList[j * 3];
					break;
				}
			}
		}
	}
	ctx.setReturn(level);
}

void op_remove_script(OpcodeContext& ctx) {
	auto object = ctx.arg(0).object();
	if (object->scriptId != 0xFFFFFFFF) {
		fo::func::scr_remove(object->scriptId);
		object->scriptId = 0xFFFFFFFF;
	}
}

#define exec_script_proc(script, proc) __asm {  \
	__asm mov  eax, script                      \
	__asm mov  edx, proc                        \
	__asm call fo::funcoffs::exec_script_proc_  \
}

void op_set_script(OpcodeContext& ctx) {
	using fo::Scripts::start;
	using fo::Scripts::map_enter_p_proc;

	long scriptType;
	auto object = ctx.arg(0).object();
	unsigned long valArg = ctx.arg(1).rawValue();

	long scriptIndex = valArg & ~0xF0000000;
	if (scriptIndex == 0 || valArg > 0x8FFFFFFF) { // negative values are not allowed
		ctx.printOpcodeError("%s() - invalid script index number.", ctx.getOpcodeName());
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

	fo::GameObject* obj = fo::func::scr_find_obj_from_program(scriptPtr->program);
	// set script index because scr_find_obj_from_program() doesn't do it when creating a hidden "spatial" object
	obj->scriptIndex = scriptIndex - 1;
	ctx.setReturn(obj);
}

#undef exec_script_proc

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
	long pid = -1;
	fo::Proto* proto;
	if (obj->IsItem() && fo::util::GetProto(obj->protoId, &proto)) {
		long type = proto->item.type;
		if (type == fo::ItemType::item_type_weapon || type == fo::ItemType::item_type_misc_item) {
			pid = obj->item.ammoPid;
		}
	}
	ctx.setReturn(pid);
}

void op_set_weapon_ammo_pid(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	if (obj->IsNotItem()) return;

	fo::Proto* proto;
	if (fo::util::GetProto(obj->protoId, &proto)) {
		long type = proto->item.type;
		if (type == fo::ItemType::item_type_weapon || type == fo::ItemType::item_type_misc_item) {
			obj->item.ammoPid = (fo::ProtoID)ctx.arg(1).rawValue();
		}
	} else {
		ctx.printOpcodeError(protoFailedLoad, ctx.getOpcodeName(), obj->protoId);
	}
}

void op_get_weapon_ammo_count(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	ctx.setReturn((obj->IsItem()) ? obj->item.charges : 0);
}

void op_set_weapon_ammo_count(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).object();
	if (obj->IsItem()) obj->item.charges = ctx.arg(1).rawValue();
}

enum class BlockType {
	BLOCKING_TYPE_BLOCK  = 0,
	BLOCKING_TYPE_SHOOT  = 1,
	BLOCKING_TYPE_AI     = 2,
	BLOCKING_TYPE_SIGHT  = 3,
	BLOCKING_TYPE_SCROLL = 4
};

static DWORD getBlockingFunc(BlockType type) {
	switch (type) {
	case BlockType::BLOCKING_TYPE_BLOCK: default:
		return fo::funcoffs::obj_blocking_at_;       // with calling hook
	case BlockType::BLOCKING_TYPE_SHOOT:
		return fo::funcoffs::obj_shoot_blocking_at_; // w/o calling hook
	case BlockType::BLOCKING_TYPE_AI:
		return fo::funcoffs::obj_ai_blocking_at_;    // w/o calling hook
	case BlockType::BLOCKING_TYPE_SIGHT:
		return fo::funcoffs::obj_sight_blocking_at_; // w/o calling hook
	//case 4:
	//	return fo::funcoffs::obj_scroll_blocking_at_;
	}
}

void op_make_straight_path(OpcodeContext& ctx) {
	auto objFrom = ctx.arg(0).object();
	DWORD tileTo = ctx.arg(1).rawValue();
	BlockType type = (BlockType)ctx.arg(2).rawValue();

	long flag = (type == BlockType::BLOCKING_TYPE_SHOOT) ? 32 : 0;
	fo::GameObject* resultObj = nullptr;
	fo::func::make_straight_path_func(objFrom, objFrom->tile, tileTo, 0, (DWORD*)&resultObj, flag, (void*)getBlockingFunc(type));
	ctx.setReturn(resultObj);
}

void op_make_path(OpcodeContext& ctx) {
	auto objFrom = ctx.arg(0).object();
	auto tileTo = ctx.arg(1).rawValue();
	auto func = getBlockingFunc((BlockType)ctx.arg(2).rawValue());

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
	      elevation = ctx.arg(1).rawValue();
	BlockType type = (BlockType)ctx.arg(2).rawValue();

	fo::GameObject* resultObj = fo::func::obj_blocking_at_wrapper(0, tile, elevation, (void*)getBlockingFunc(type));
	if (resultObj && type == BlockType::BLOCKING_TYPE_SHOOT && (resultObj->flags & fo::ObjectFlag::ShootThru)) { // don't know what this flag means, copy-pasted from the engine code
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
	for (int i = 0; i < actualCount; i++) {
		fo::GameObject* obj = fo::var::partyMemberList[i].object;
		if (includeHidden || (obj->IsCritter() && !fo::func::critter_is_dead(obj) && !(obj->flags & fo::ObjectFlag::Hidden))) {
			arrays[arrayId].push_back((long)obj);
		}
	}
	ctx.setReturn(arrayId);
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
		ctx.printOpcodeError("%s() - invalid PID number.", ctx.getMetaruleName());
		ctx.setReturn(-1);
	}
}

void mf_get_dialog_object(OpcodeContext& ctx) {
	ctx.setReturn(InDialog() ? fo::var::dialog_target : 0);
}

void mf_obj_under_cursor(OpcodeContext& ctx) {
	ctx.setReturn(
		fo::func::object_under_mouse(ctx.arg(0).asBool() ? 1 : -1, ctx.arg(1).rawValue(), fo::var::map_elevation)
	);
}

void mf_get_loot_object(OpcodeContext& ctx) {
	ctx.setReturn((GetLoopFlags() & INTFACELOOT) ? fo::var::target_stack[fo::var::target_curr_stack] : 0);
}

static bool protoMaxLimitPatch = false;

void op_get_proto_data(OpcodeContext& ctx) {
	long result = -1;
	fo::Proto* protoPtr;
	int pid = ctx.arg(0).rawValue();
	if (fo::util::CheckProtoID(pid) && fo::util::GetProto(pid, &protoPtr)) {
		result = *(long*)((BYTE*)protoPtr + ctx.arg(1).rawValue());
	} else {
		ctx.printOpcodeError(protoFailedLoad, ctx.getOpcodeName(), pid);
	}
	ctx.setReturn(result);
}

void op_set_proto_data(OpcodeContext& ctx) {
	int pid = ctx.arg(0).rawValue();
	if (fo::util::CheckProtoID(pid) && CritterStats::SetProtoData(pid, ctx.arg(1).rawValue(), ctx.arg(2).rawValue()) != -1) {
		if (!protoMaxLimitPatch) {
			Objects::LoadProtoAutoMaxLimit();
			protoMaxLimitPatch = true;
		}
	} else {
		ctx.printOpcodeError(protoFailedLoad, ctx.getOpcodeName(), pid);
	}
}

static const char* invalidObjPtr = "%s() - invalid object pointer.";

void mf_get_object_data(OpcodeContext& ctx) {
	long result = 0;
	DWORD* object_ptr = (DWORD*)ctx.arg(0).rawValue();
	if (*(object_ptr - 1) != 0xFEEDFACE && !(fo::var::combat_state & fo::CombatStateFlag::InCombat)) {
		ctx.printOpcodeError(invalidObjPtr, ctx.getMetaruleName());
	} else {
		result = *(long*)((BYTE*)object_ptr + ctx.arg(1).rawValue());
	}
	ctx.setReturn(result);
}

void mf_set_object_data(OpcodeContext& ctx) {
	DWORD* object_ptr = (DWORD*)ctx.arg(0).rawValue();
	if (*(object_ptr - 1) != 0xFEEDFACE && !(fo::var::combat_state & fo::CombatStateFlag::InCombat)) {
		ctx.printOpcodeError(invalidObjPtr, ctx.getMetaruleName());
		ctx.setReturn(-1);
	} else {
		*(long*)((BYTE*)object_ptr + ctx.arg(1).rawValue()) = ctx.arg(2).rawValue();
	}
}

void mf_get_object_ai_data(OpcodeContext& ctx) {
	fo::AIcap* cap = fo::func::ai_cap(ctx.arg(0).object());
	DWORD arrayId, value = -1;
	switch (ctx.arg(1).rawValue()) {
	case 0:
		value = cap->aggression;
		break;
	case 1:
		value = (long)cap->area_attack_mode;
		break;
	case 2:
		value = (long)cap->attack_who;
		break;
	case 3:
		value = (long)cap->pref_weapon;
		break;
	case 4:
		value = (long)cap->chem_use;
		break;
	case 5:
		value = (long)cap->disposition;
		break;
	case 6:
		value = (long)cap->distance;
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
		value = (long)cap->run_away_mode;
		break;
	case 12:
		value = cap->secondary_freq;
		break;
	case 13:
		value = cap->called_freq;
		break;
	case 14:
		arrayId = CreateTempArray(3, 0);
		arrays[arrayId].val[0].set(cap->chem_primary_desire[0]);
		arrays[arrayId].val[1].set(cap->chem_primary_desire[1]);
		arrays[arrayId].val[2].set(cap->chem_primary_desire[2]);
		value = arrayId;
		break;
	default:
		ctx.printOpcodeError("%s() - invalid aiParam number.", ctx.getMetaruleName());
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
		ctx.printOpcodeError("%s() - invalid type number.", ctx.getMetaruleName());
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
	fo::util::GetObjectsTileRadius(objects, ctx.arg(0).rawValue(), radius, elev, type);
	size_t sz = objects.size();
	DWORD id = CreateTempArray(sz, 0);
	for (size_t i = 0; i < sz; i++) {
		arrays[id].val[i].set((long)objects[i]);
	}
	ctx.setReturn(id);
}

void mf_npc_engine_level_up(OpcodeContext& ctx) {
	if (ctx.arg(0).asBool()) {
		if (!npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0x840F); // enable
		npcEngineLevelUp = true;
	} else {
		if (npcEngineLevelUp) SafeWrite16(0x4AFC1C, 0xE990);
		npcEngineLevelUp = false;
	}
}

void mf_obj_is_openable(OpcodeContext& ctx) {
	ctx.setReturn(fo::util::ObjIsOpenable(ctx.arg(0).object()));
}

}
}
