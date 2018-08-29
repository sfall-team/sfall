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
#include "..\..\Explosions.h"
#include "..\..\Knockback.h"
#include "..\..\PartyControl.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "..\OpcodeContext.h"

#include "Objects.h"

namespace sfall
{
namespace script
{

void __declspec(naked) op_remove_script() {
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
		test eax, eax;
		jz end;
		mov edx, eax;
		mov eax, [eax + 0x78];
		cmp eax, 0xffffffff;
		jz end;
		call fo::funcoffs::scr_remove_;
		mov dword ptr[edx + 0x78], 0xffffffff;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

void __declspec(naked) op_set_script() {
	__asm {
		pushad;
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
		test eax, eax;
		jz end;
		mov esi, [eax + 0x78];
		cmp esi, 0xffffffff;
		jz newscript;
		push eax;
		mov eax, esi;
		call fo::funcoffs::scr_remove_;
		pop eax;
		mov dword ptr[eax + 0x78], 0xffffffff;
newscript:
		mov esi, 1;
		test ebx, 0x80000000;
		jz execMapEnter;
		xor esi, esi;
		xor ebx, 0x80000000;
execMapEnter:
		mov ecx, eax;
		mov edx, 3; // script_type_item
		mov edi, [eax + 0x64];
		shr edi, 24;
		cmp edi, 1;
		jnz notCritter;
		inc edx; // 4 - "critter" type script
notCritter:
		dec ebx;
		call fo::funcoffs::obj_new_sid_inst_;
		mov eax, [ecx + 0x78];
		mov edx, 1; // start
		call fo::funcoffs::exec_script_proc_;
		cmp esi, 1; // run map enter?
		jnz end;
		mov eax, [ecx + 0x78];
		mov edx, 0xf; // map_enter_p_proc
		call fo::funcoffs::exec_script_proc_;
end:
		popad;
		retn;
	}
}

void sf_create_spatial(OpcodeContext& ctx) {
	DWORD scriptIndex = ctx.arg(0).asInt(),
		tile = ctx.arg(1).asInt(),
		elevation = ctx.arg(2).asInt(),
		radius = ctx.arg(3).asInt(),
		scriptId, tmp, objectPtr,
		scriptPtr;
	__asm {
		lea eax, scriptId;
		mov edx, 1;
		call fo::funcoffs::scr_new_;
		mov tmp, eax;
	}
	if (tmp == -1)
		return;
	__asm {
		mov eax, scriptId;
		lea edx, scriptPtr;
		call fo::funcoffs::scr_ptr_;
		mov tmp, eax;
	}
	if (tmp == -1)
		return;
	// fill spatial script properties:
	*(DWORD*)(scriptPtr + 0x14) = scriptIndex - 1;
	*(DWORD*)(scriptPtr + 0x8) = (elevation << 29) & 0xE0000000 | tile;
	*(DWORD*)(scriptPtr + 0xC) = radius;
	// this will load appropriate script program and link it to the script instance we just created:
	__asm {
		mov eax, scriptId;
		mov edx, 1; // start_p_proc
		call fo::funcoffs::exec_script_proc_;
		mov eax, scriptPtr;
		mov eax, [eax + 0x18]; // program pointer
		call fo::funcoffs::scr_find_obj_from_program_;
		mov objectPtr, eax;
	}
	ctx.setReturn((int)objectPtr);
}

void sf_spatial_radius(OpcodeContext& ctx) {
	auto spatialObj = ctx.arg(0).asObject();
	fo::ScriptInstance* script;
	if (fo::func::scr_ptr(spatialObj->scriptId, &script) != -1) {
		ctx.setReturn(script->spatialRadius);
	}
}

void sf_get_script(OpcodeContext& ctx) {
	if (ctx.arg(0).isInt()) {
		auto obj = ctx.arg(0).asObject();
		ctx.setReturn(obj->scriptIndex);
	} else {
		ctx.setReturn(-1);
	}
}

void sf_set_critter_burst_disable(OpcodeContext& ctx) {
	if (ctx.arg(0).isInt() && ctx.arg(0).isInt()) {
		SetNoBurstMode(ctx.arg(0).asObject(), ctx.arg(1).asBool());
	}
}

void sf_get_weapon_ammo_pid(OpcodeContext& ctx) {
	if (ctx.arg(0).isInt()) {
		auto obj = ctx.arg(0).asObject();
		ctx.setReturn(obj->item.ammoPid);
	} else {
		ctx.setReturn(-1);
	}
}

void sf_set_weapon_ammo_pid(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	obj->item.ammoPid = ctx.arg(1).asInt();
}

void sf_get_weapon_ammo_count(OpcodeContext& ctx) {
	if (ctx.arg(0).isInt()) {
		auto obj = ctx.arg(0).asObject();
		ctx.setReturn(obj->item.charges);
	} else {
		ctx.setReturn(-1);
	}
}

void sf_set_weapon_ammo_count(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	obj->item.charges = ctx.arg(1).asInt();
}

static DWORD _stdcall obj_blocking_at_wrapper(DWORD obj, DWORD tile, DWORD elevation, DWORD func) {
	__asm {
		mov eax, obj;
		mov edx, tile;
		mov ebx, elevation;
		call func;
	}
}

static DWORD _stdcall make_straight_path_func_wrapper(fo::GameObject* obj, DWORD tileFrom, DWORD a3, DWORD tileTo, DWORD* result, DWORD a6, DWORD func) {
	__asm {
		mov eax, obj;
		mov edx, tileFrom;
		mov ecx, a3;
		mov ebx, tileTo;
		push func;
		push a6;
		push result;
		call fo::funcoffs::make_straight_path_func_;
	}
}

#define BLOCKING_TYPE_BLOCK		(0)
#define BLOCKING_TYPE_SHOOT		(1)
#define BLOCKING_TYPE_AI		(2)
#define BLOCKING_TYPE_SIGHT		(3)
#define BLOCKING_TYPE_SCROLL	(4)

static DWORD getBlockingFunc(DWORD type) {
	switch (type) {
	case BLOCKING_TYPE_BLOCK: default:
		return fo::funcoffs::obj_blocking_at_;
	case BLOCKING_TYPE_SHOOT:
		return fo::funcoffs::obj_shoot_blocking_at_;
	case BLOCKING_TYPE_AI:
		return fo::funcoffs::obj_ai_blocking_at_;
	case BLOCKING_TYPE_SIGHT:
		return fo::funcoffs::obj_sight_blocking_at_;
	//case 4: 
	//	return obj_scroll_blocking_at_;

	}
}

void sf_make_straight_path(OpcodeContext& ctx) {
	auto objFrom = ctx.arg(0).asObject();
	DWORD tileTo = ctx.arg(1).asInt(),
		type = ctx.arg(2).asInt(),
		resultObj, arg6;
	arg6 = (type == BLOCKING_TYPE_SHOOT) ? 32 : 0;
	make_straight_path_func_wrapper(objFrom, objFrom->tile, 0, tileTo, &resultObj, arg6, getBlockingFunc(type));
	ctx.setReturn(resultObj, DataType::INT);
}

void sf_make_path(OpcodeContext& ctx) {
	auto objFrom = ctx.arg(0).asObject();
	auto tileTo = ctx.arg(1).asInt(),
		type = ctx.arg(2).asInt();
	auto func = getBlockingFunc(type);
	long pathLength, a5 = 1;
	char pathData[800];
	pathLength = fo::func::make_path_func(objFrom, objFrom->tile, tileTo, pathData, a5, (void*)func);
	auto arrayId = TempArray(pathLength, 0);
	for (int i = 0; i < pathLength; i++) {
		arrays[arrayId].val[i].set((long)pathData[i]);
	}
	ctx.setReturn(arrayId, DataType::INT);
}

void sf_obj_blocking_at(OpcodeContext& ctx) {
	DWORD tile = ctx.arg(0).asInt(),
		elevation = ctx.arg(1).asInt(),
		type = ctx.arg(2).asInt(),
		resultObj;
	resultObj = obj_blocking_at_wrapper(0, tile, elevation, getBlockingFunc(type));
	if (resultObj && type == BLOCKING_TYPE_SHOOT && (*(DWORD*)(resultObj + 39) & 0x80)) { // don't know what this flag means, copy-pasted from the engine code
		// this check was added because the engine always does exactly this when using shoot blocking checks
		resultObj = 0;
	}
	ctx.setReturn(resultObj, DataType::INT);
}

void sf_tile_get_objects(OpcodeContext& ctx) {
	DWORD tile = ctx.arg(0).asInt(),
		elevation = ctx.arg(1).asInt();
	DWORD arrayId = TempArray(0, 4);
	auto obj = fo::func::obj_find_first_at_tile(elevation, tile);
	while (obj) {
		arrays[arrayId].push_back(reinterpret_cast<long>(obj));
		obj = fo::func::obj_find_next_at_tile();
	}
	ctx.setReturn(arrayId, DataType::INT);
}

void sf_get_party_members(OpcodeContext& ctx) {
	auto includeHidden = ctx.arg(0).asInt();
	int actualCount = fo::var::partyMemberCount;
	DWORD arrayId = TempArray(0, 4);
	auto partyMemberList = fo::var::partyMemberList;
	for (int i = 0; i < actualCount; i++) {
		auto obj = reinterpret_cast<fo::GameObject*>(partyMemberList[i * 4]);
		if (includeHidden || (obj->type() == fo::OBJ_TYPE_CRITTER && !fo::func::critter_is_dead(obj) && !(obj->flags & fo::ObjectFlag::Mouse_3d))) {
			arrays[arrayId].push_back((long)obj);
		}
	}
	ctx.setReturn(arrayId, DataType::INT);
}

void sf_art_exists(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::art_exists(ctx.arg(0).asInt()));
}

void sf_obj_is_carrying_obj(OpcodeContext& ctx) {
	int num = 0;
	const ScriptValue &invenObjArg = ctx.arg(0),
		&itemObjArg = ctx.arg(1);

	if (invenObjArg.isInt() && itemObjArg.isInt()) {
		fo::GameObject *invenObj = (fo::GameObject*)invenObjArg.asObject(),
			*itemObj = (fo::GameObject*)itemObjArg.asObject();
		if (invenObj != nullptr && itemObj != nullptr) {
			for (int i = 0; i < invenObj->invenSize; i++) {
				if (invenObj->invenTable[i].object == itemObj) {
					num = invenObj->invenTable[i].count;
					break;
				}
			}
		}
	}
	ctx.setReturn(num);
}

void sf_critter_inven_obj2(OpcodeContext& ctx) {
	fo::GameObject* critter = ctx.arg(0).asObject();
	int slot = ctx.arg(1).asInt();
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
		ctx.printOpcodeError("critter_inven_obj2() - invalid type.");
	}
}

void sf_set_outline(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	int color = ctx.arg(1).asInt();
	obj->outline = color;
}

void sf_get_outline(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	ctx.setReturn(obj->outline);
}

void sf_set_flags(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	int flags = ctx.arg(1).asInt();
	obj->flags = flags;
}

void sf_get_flags(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	ctx.setReturn(obj->flags);
}

void sf_outlined_object(OpcodeContext& ctx) {
	ctx.setReturn(fo::var::outlined_object);
}

void sf_item_weight(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::item_weight(ctx.arg(0).asObject()));
}

void sf_set_dude_obj(OpcodeContext& ctx) {
	auto obj = ctx.arg(0).asObject();
	if (obj->type() == fo::ObjType::OBJ_TYPE_CRITTER) {
		PartyControl::SwitchToCritter(obj);
	} else {
		ctx.printOpcodeError("Object is not a critter!");
		ctx.setReturn(-1);
	}
}

void sf_real_dude_obj(OpcodeContext& ctx) {
	ctx.setReturn(PartyControl::RealDudeObject());
}

void sf_car_gas_amount(OpcodeContext& ctx) {
	ctx.setReturn(fo::var::carGasAmount);
}

void sf_lock_is_jammed(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::obj_lock_is_jammed(ctx.arg(0).asObject()));
}

void sf_unjam_lock(OpcodeContext& ctx) {
	fo::func::obj_unjam_lock(ctx.arg(0).asObject());
}

void sf_set_unjam_locks_time(OpcodeContext& ctx) {
	int time = ctx.arg(0).asInt();
	if (time < 0 || time > 127) {
		ctx.printOpcodeError("set_unjam_locks_time() - time argument must be in the range of 0 to 127.");
	} else {
		ScriptExtender::SetAutoUnjamLockTime(time);
	}
}

void sf_item_make_explosive(OpcodeContext& ctx) {
	DWORD pid = ctx.arg(0).rawValue();
	DWORD pidActive = ctx.arg(1).rawValue();
	DWORD min = ctx.arg(2).rawValue();
	DWORD max = (ctx.numArgs() == 4) ? ctx.arg(3).rawValue() : min;

	if (min > max) {
		max = min;
		ctx.printOpcodeError("item_make_explosive() - Warning: argument max has a value less than the argument min.");
	}

	if (pid > 0 && pidActive > 0) {
		Explosions::AddToExplosives(pid, pidActive, min, max);
	} else {
		ctx.printOpcodeError("item_make_explosive() - PID's arguments must be greater 0.");
	}
}

}
}
