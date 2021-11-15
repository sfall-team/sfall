/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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

#include "Inventory.h"
#include "..\Game\inventory.h"

namespace sfall
{

static void __declspec(naked) op_active_hand() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_itemCurrentItem];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_toggle_active_hand() {
	__asm {
		mov eax, 1;
		jmp fo::funcoffs::intface_toggle_items_;
	}
}

static void __declspec(naked) op_set_inven_ap_cost() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		mov  ecx, eax;
		call Inventory::SetInvenApCost;
end:
		mov  ecx, esi;
		retn;
	}
}

static void mf_get_inven_ap_cost() {
	opHandler.setReturn(Inventory::GetInvenApCost());
}

static void __stdcall op_obj_is_carrying_obj2() {
	const ScriptValue &invenObjArg = opHandler.arg(0),
	                  &itemObjArg = opHandler.arg(1);

	fo::GameObject *invenObj = invenObjArg.asObject(),
	         *itemObj = itemObjArg.asObject();

	int count = 0;
	if (invenObj != nullptr && itemObj != nullptr) {
		for (int i = 0; i < invenObj->invenSize; i++) {
			if (invenObj->invenTable[i].object == itemObj) {
				count = invenObj->invenTable[i].count;
				break;
			}
		}
	} else {
		OpcodeInvalidArgs("obj_is_carrying_obj");
	}
	opHandler.setReturn(count);
}

static void __declspec(naked) op_obj_is_carrying_obj() {
	_WRAP_OPCODE(op_obj_is_carrying_obj2, 2, 1)
}

static void mf_critter_inven_obj2() {
	fo::GameObject* critter = opHandler.arg(0).asObject();
	const ScriptValue &slotArg = opHandler.arg(1);

	if (critter && slotArg.isInt()) {
		int slot = slotArg.rawValue();
		switch (slot) {
		case 0:
			opHandler.setReturn(fo::func::inven_worn(critter));
			break;
		case 1:
			opHandler.setReturn(fo::func::inven_right_hand(critter));
			break;
		case 2:
			opHandler.setReturn(fo::func::inven_left_hand(critter));
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

static void mf_item_weight() {
	fo::GameObject* item = opHandler.arg(0).asObject();
	if (item) {
		opHandler.setReturn(fo::func::item_weight(item));
	} else {
		OpcodeInvalidArgs("item_weight");
		opHandler.setReturn(0);
	}
}

static void mf_get_current_inven_size() {
	fo::GameObject* obj = opHandler.arg(0).asObject();
	if (obj) {
		opHandler.setReturn(game::Inventory::item_total_size(obj));
	} else {
		OpcodeInvalidArgs("get_current_inven_size");
		opHandler.setReturn(0);
	}
}

static void mf_unwield_slot() {
	fo::InvenType slot = static_cast<fo::InvenType>(opHandler.arg(1).rawValue());
	if (slot < fo::INVEN_TYPE_WORN || slot > fo::INVEN_TYPE_LEFT_HAND) {
		opHandler.printOpcodeError("unwield_slot() - incorrect slot number.");
		opHandler.setReturn(-1);
		return;
	}
	fo::GameObject* critter = opHandler.arg(0).object();
	if (critter->IsNotCritter()) {
		opHandler.printOpcodeError("unwield_slot() - the object is not a critter.");
		opHandler.setReturn(-1);
		return;
	}
	bool isDude = (critter == *fo::ptr::obj_dude);
	bool update = false;
	if (slot && (GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | BARTER)) == false) {
		if (fo::func::inven_unwield(critter, (slot == fo::INVEN_TYPE_LEFT_HAND) ? fo::Left : fo::Right) == 0) {
			update = isDude;
		}
	} else {
		// force unwield for opened inventory
		bool forceAdd = false;
		fo::GameObject* item = nullptr;
		if (slot != fo::INVEN_TYPE_WORN) {
			if (!isDude) return;
			long* itemRef = nullptr;
			if (slot == fo::INVEN_TYPE_LEFT_HAND) {
				item = *fo::ptr::i_lhand;
				itemRef = (long*)FO_VAR_i_lhand;
			} else {
				item = *fo::ptr::i_rhand;
				itemRef = (long*)FO_VAR_i_rhand;
			}
			if (item) {
				if (!game::Inventory::correctFidForRemovedItem(critter, item, (slot == fo::INVEN_TYPE_LEFT_HAND) ? fo::ObjectFlag::Left_Hand : fo::ObjectFlag::Right_Hand)) {
					return;
				}
				*itemRef = 0;
				forceAdd = true;
				update = true;
			}
		} else {
			if (isDude) item = *fo::ptr::i_worn;
			if (!item) {
				item = fo::func::inven_worn(critter);
			} else {
				*fo::ptr::i_worn = nullptr;
				forceAdd = true;
			}
			if (item) {
				if (!game::Inventory::correctFidForRemovedItem(critter, item, fo::ObjectFlag::Worn)) {
					if (forceAdd) *fo::ptr::i_worn = item;
					return;
				}
				if (isDude) fo::func::intface_update_ac(0);
			}
		}
		if (forceAdd) fo::func::item_add_force(critter, item, 1);
	}
	if (update) fo::func::intface_update_items(0, -1, -1);
}

}
