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

#include "..\..\..\FalloutEngine\AsmMacros.h"
#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\LoadGameHook.h"
#include "..\..\Inventory.h"

#include "..\..\..\Game\inventory.h"

#include "Inventory.h"

namespace sfall
{
namespace script
{

__declspec(naked) void op_active_hand() {
	__asm {
		mov  edx, dword ptr ds:[FO_VAR_itemCurrentItem];
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

__declspec(naked) void op_toggle_active_hand() {
	__asm {
		mov eax, 1;
		jmp fo::funcoffs::intface_toggle_items_;
	}
}

__declspec(naked) void op_set_inven_ap_cost() {
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

void mf_get_inven_ap_cost(OpcodeContext& ctx) {
	ctx.setReturn(Inventory::GetInvenApCost());
}

void op_obj_is_carrying_obj(OpcodeContext& ctx) {
	const ScriptValue &invenObjArg = ctx.arg(0),
	                  &itemObjArg = ctx.arg(1);

	fo::GameObject *invenObj = invenObjArg.object(),
	               *itemObj = itemObjArg.object();

	int count = 0;
	if (invenObj != nullptr && itemObj != nullptr) {
		for (int i = 0; i < invenObj->invenSize; i++) {
			if (invenObj->invenTable[i].object == itemObj) {
				if (invenObj->invenTable[i].count <= 0) {
					invenObj->invenTable[i].count = 1; // fix stack count
				}
				count = invenObj->invenTable[i].count;
				break;
			}
		}
	}
	ctx.setReturn(count);
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
		ctx.printOpcodeError("%s() - invalid type number.", ctx.getMetaruleName());
	}
}

void mf_item_weight(OpcodeContext& ctx) {
	ctx.setReturn(fo::func::item_weight(ctx.arg(0).object()));
}

void mf_get_current_inven_size(OpcodeContext& ctx) {
	ctx.setReturn(game::Inventory::item_total_size(ctx.arg(0).object()));
}

void mf_unwield_slot(OpcodeContext& ctx) {
	fo::InvenType slot = static_cast<fo::InvenType>(ctx.arg(1).rawValue());
	if (slot < fo::INVEN_TYPE_WORN || slot > fo::INVEN_TYPE_LEFT_HAND) {
		ctx.printOpcodeError("%s() - incorrect slot number.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	fo::GameObject* critter = ctx.arg(0).object();
	if (critter->IsNotCritter()) {
		ctx.printOpcodeError("%s() - the object is not a critter.", ctx.getMetaruleName());
		ctx.setReturn(-1);
		return;
	}
	bool isDude = (critter == fo::var::obj_dude);
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
				item = fo::var::i_lhand;
				itemRef = (long*)FO_VAR_i_lhand;
			} else {
				item = fo::var::i_rhand;
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
			if (isDude) item = fo::var::i_worn;
			if (!item) {
				item = fo::func::inven_worn(critter);
			} else {
				fo::var::i_worn = nullptr;
				forceAdd = true;
			}
			if (item) {
				if (!game::Inventory::correctFidForRemovedItem(critter, item, fo::ObjectFlag::Worn)) {
					if (forceAdd) fo::var::i_worn = item;
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
}
