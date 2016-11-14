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
#include "..\..\Knockback.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"
#include "AsmMacros.h"

#include "Objects.h"

void __declspec(naked) op_remove_script() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz end;
		test eax, eax;
		jz end;
		mov edx, eax;
		mov eax, [eax + 0x78];
		cmp eax, 0xffffffff;
		jz end;
		call FuncOffs::scr_remove_;
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
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		mov ebx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz end;
		cmp di, 0xc001;
		jnz end;
		test eax, eax;
		jz end;
		mov esi, [eax + 0x78];
		cmp esi, 0xffffffff;
		jz newscript;
		push eax;
		mov eax, esi;
		call FuncOffs::scr_remove_;
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
		call FuncOffs::obj_new_sid_inst_;
		mov eax, [ecx + 0x78];
		mov edx, 1; // start
		call FuncOffs::exec_script_proc_;
		cmp esi, 1; // run map enter?
		jnz end;
		mov eax, [ecx + 0x78];
		mov edx, 0xf; // map_enter_p_proc
		call FuncOffs::exec_script_proc_;
end:
		popad;
		retn;
	}
}

void sf_create_spatial(OpcodeHandler& opHandler) {
	DWORD scriptIndex = opHandler.arg(0).asInt(),
		tile = opHandler.arg(1).asInt(),
		elevation = opHandler.arg(2).asInt(),
		radius = opHandler.arg(3).asInt(),
		scriptId, tmp, objectPtr,
		scriptPtr;
	__asm {
		lea eax, scriptId;
		mov edx, 1;
		call FuncOffs::scr_new_;
		mov tmp, eax;
	}
	if (tmp == -1)
		return;
	__asm {
		mov eax, scriptId;
		lea edx, scriptPtr;
		call FuncOffs::scr_ptr_;
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
		call FuncOffs::exec_script_proc_;
		mov eax, scriptPtr;
		mov eax, [eax + 0x18]; // program pointer
		call FuncOffs::scr_find_obj_from_program_;
		mov objectPtr, eax;
	}
	opHandler.setReturn((int)objectPtr);
}

void __declspec(naked) op_create_spatial() {
	_WRAP_OPCODE(sf_create_spatial, 4, 1)
}

void sf_spatial_radius(OpcodeHandler& opHandler) {
	TGameObj* spatialObj = opHandler.arg(0).asObject();
	TScript* script;
	if (Wrapper::scr_ptr(spatialObj->scriptID, &script) != -1) {
		opHandler.setReturn(script->spatial_radius);
	}
}

void __declspec(naked) op_get_script() {
	__asm {
		pushad;
		mov ecx, eax;
		mov ecx, eax;
		call FuncOffs::interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call FuncOffs::interpretPopLong_;
		cmp dx, 0xc001;
		jnz fail;
		test eax, eax;
		jz fail;
		mov edx, [eax + 0x80];
		cmp edx, -1;
		jz fail;
		inc edx;
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ecx;
		call FuncOffs::interpretPushLong_;
		mov eax, ecx;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_critter_burst_disable() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		push ecx;
		push eax;
		call SetNoBurstMode;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_get_weapon_ammo_pid() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		test eax, eax;
		jz fail;
		mov edx, [eax + 0x40];
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_weapon_ammo_pid() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		test eax, eax;
		jz end;
		mov[eax + 0x40], ecx;
end:
		popad;
		retn;
	}
}

void __declspec(naked) op_get_weapon_ammo_count() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz fail;
		test eax, eax;
		jz fail;
		mov edx, [eax + 0x3c];
		jmp end;
fail:
		xor edx, edx;
		dec edx;
end:
		mov eax, ebp;
		call FuncOffs::interpretPushLong_;
		mov eax, ebp;
		mov edx, 0xc001;
		call FuncOffs::interpretPushShort_;
		popad;
		retn;
	}
}

void __declspec(naked) op_set_weapon_ammo_count() {
	__asm {
		pushad;
		mov ebp, eax;
		call FuncOffs::interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		mov ecx, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopShort_;
		mov esi, eax;
		mov eax, ebp;
		call FuncOffs::interpretPopLong_;
		cmp di, 0xc001;
		jnz end;
		cmp si, 0xc001;
		jnz end;
		test eax, eax;
		jz end;
		mov[eax + 0x3c], ecx;
end:
		popad;
		retn;
	}
}


static DWORD _stdcall obj_blocking_at_wrapper(DWORD obj, DWORD tile, DWORD elevation, DWORD func) {
	__asm {
		mov eax, obj;
		mov edx, tile;
		mov ebx, elevation;
		call func;
	}
}

static DWORD _stdcall make_straight_path_func_wrapper(DWORD obj, DWORD tileFrom, DWORD a3, DWORD tileTo, DWORD* result, DWORD a6, DWORD func) {
	__asm {
		mov eax, obj;
		mov edx, tileFrom;
		mov ecx, a3;
		mov ebx, tileTo;
		push func;
		push a6;
		push result;
		call FuncOffs::make_straight_path_func_;
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
		return FuncOffs::obj_blocking_at_;
	case BLOCKING_TYPE_SHOOT:
		return FuncOffs::obj_shoot_blocking_at_;
	case BLOCKING_TYPE_AI:
		return FuncOffs::obj_ai_blocking_at_;
	case BLOCKING_TYPE_SIGHT:
		return FuncOffs::obj_sight_blocking_at_;
	//case 4: 
	//	return obj_scroll_blocking_at_;

	}
}

void sf_make_straight_path(OpcodeHandler& opHandler) {
	DWORD objFrom = opHandler.arg(0).asInt(),
		tileTo = opHandler.arg(1).asInt(),
		type = opHandler.arg(2).asInt(),
		resultObj, arg6;
	arg6 = (type == BLOCKING_TYPE_SHOOT) ? 32 : 0;
	make_straight_path_func_wrapper(objFrom, *(DWORD*)(objFrom + 4), 0, tileTo, &resultObj, arg6, getBlockingFunc(type));
	opHandler.setReturn(resultObj, DATATYPE_INT);
}

void __declspec(naked) op_make_straight_path() {
	_WRAP_OPCODE(sf_make_straight_path, 3, 1)
}

static void sf_make_path(OpcodeHandler& opHandler) {
	DWORD objFrom = opHandler.arg(0).asInt(),
		tileFrom = 0,
		tileTo = opHandler.arg(1).asInt(),
		type = opHandler.arg(2).asInt(),
		func = getBlockingFunc(type),
		arr;
	long pathLength, a5 = 1;
	if (!objFrom) {
		opHandler.setReturn(0, DATATYPE_INT);
		return;
	}
	tileFrom = *(DWORD*)(objFrom + 4);
	char pathData[800];
	char* pathDataPtr = pathData;
	__asm {
		mov eax, objFrom;
		mov edx, tileFrom;
		mov ecx, pathDataPtr;
		mov ebx, tileTo;
		push func;
		push a5;
		call FuncOffs::make_path_func_;
		mov pathLength, eax;
	}
	arr = TempArray(pathLength, 0);
	for (int i = 0; i < pathLength; i++) {
		arrays[arr].val[i].set((long)pathData[i]);
	}
	opHandler.setReturn(arr, DATATYPE_INT);
}

void __declspec(naked) op_make_path() {
	_WRAP_OPCODE(sf_make_path, 3, 1)
}

static void sf_obj_blocking_at(OpcodeHandler& opHandler) {
	DWORD tile = opHandler.arg(0).asInt(),
		elevation = opHandler.arg(1).asInt(),
		type = opHandler.arg(2).asInt(),
		resultObj;
	resultObj = obj_blocking_at_wrapper(0, tile, elevation, getBlockingFunc(type));
	if (resultObj && type == BLOCKING_TYPE_SHOOT && (*(DWORD*)(resultObj + 39) & 0x80)) { // don't know what this flag means, copy-pasted from the engine code
		// this check was added because the engine always does exactly this when using shoot blocking checks
		resultObj = 0;
	}
	opHandler.setReturn(resultObj, DATATYPE_INT);
}

void __declspec(naked) op_obj_blocking_at() {
	_WRAP_OPCODE(sf_obj_blocking_at, 3, 1)
}

static void sf_tile_get_objects(OpcodeHandler& opHandler) {
	DWORD tile = opHandler.arg(0).asInt(),
		elevation = opHandler.arg(1).asInt(),
		obj;
	DWORD arrayId = TempArray(0, 4);
	__asm {
		mov eax, elevation;
		mov edx, tile;
		call FuncOffs::obj_find_first_at_tile_;
		mov obj, eax;
	}
	while (obj) {
		arrays[arrayId].push_back((long)obj);
		__asm {
			call FuncOffs::obj_find_next_at_tile_;
			mov obj, eax;
		}
	}
	opHandler.setReturn(arrayId, DATATYPE_INT);
}

void __declspec(naked) op_tile_get_objects() {
	_WRAP_OPCODE(sf_tile_get_objects, 2, 1)
}

static void sf_get_party_members(OpcodeHandler& opHandler) {
	DWORD obj, mode = opHandler.arg(0).asInt(), isDead;
	int i, actualCount = VarPtr::partyMemberCount;
	DWORD arrayId = TempArray(0, 4);
	DWORD* partyMemberList = VarPtr::partyMemberList;
	for (i = 0; i < actualCount; i++) {
		obj = partyMemberList[i * 4];
		if (mode == 0) { // mode 0 will act just like op_party_member_count in fallout2
			if ((*(DWORD*)(obj + 100) >> 24) != OBJ_TYPE_CRITTER)  // obj type != critter
				continue;
			__asm {
				mov eax, obj;
				call FuncOffs::critter_is_dead_;
				mov isDead, eax;
			}
			if (isDead)
				continue;
			if (*(DWORD*)(obj + 36) & 1) // no idea..
				continue;
		}
		arrays[arrayId].push_back((long)obj);
	}
	opHandler.setReturn(arrayId, DATATYPE_INT);
}

void __declspec(naked) op_get_party_members() {
	_WRAP_OPCODE(sf_get_party_members, 1, 1)
}

// TODO: rewrite
void __declspec(naked) op_art_exists() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ecx, eax)
	_CHECK_ARG_INT(cx, fail)
	__asm {
		call FuncOffs::art_exists_;
		jmp end;
fail:
		xor eax, eax;
end:
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void sf_obj_is_carrying_obj(OpcodeHandler& opHandler) {
	int num = 0;
	const ScriptValue &invenObjArg = opHandler.arg(0),
		&itemObjArg = opHandler.arg(1);

	if (invenObjArg.isInt() && itemObjArg.isInt()) {
		TGameObj *invenObj = (TGameObj*)invenObjArg.asObject(),
			*itemObj = (TGameObj*)itemObjArg.asObject();
		if (invenObj != nullptr && itemObj != nullptr) {
			for (int i = 0; i < invenObj->invenCount; i++) {
				if (invenObj->invenTable[i].object == itemObj) {
					num = invenObj->invenTable[i].count;
					break;
				}
			}
		}
	}
	opHandler.setReturn(num);
}

void __declspec(naked) op_obj_is_carrying_obj() {
	_WRAP_OPCODE(sf_obj_is_carrying_obj, 2, 1)
}

void sf_critter_inven_obj2(OpcodeHandler& opHandler) {
	TGameObj* critter = opHandler.arg(0).asObject();
	int slot = opHandler.arg(1).asInt();
	switch (slot) {
	case 0:
		opHandler.setReturn(Wrapper::inven_worn(critter));
		break;
	case 1:
		opHandler.setReturn(Wrapper::inven_right_hand(critter));
		break;
	case 2:
		opHandler.setReturn(Wrapper::inven_left_hand(critter));
		break;
	case -2:
		opHandler.setReturn(critter->invenCount);
		break;
	default:
		opHandler.printOpcodeError("critter_inven_obj2() - invalid type.");
	}
}
