/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\HeroAppearance.h"
#include "..\Modules\PartyControl.h"

#include "..\Modules\HookScripts\InventoryHs.h"

#include "inventory.h"

namespace game
{

namespace sf = sfall;

// Custom implementation of correctFidForRemovedItem_ engine function with the HOOK_INVENWIELD hook
long Inventory::correctFidForRemovedItem(fo::GameObject* critter, fo::GameObject* item, long flags) {
	long result = sf::InvenWieldHook_Invoke(critter, item, flags);
	if (result) fo::func::correctFidForRemovedItem(critter, item, flags);
	return result;
}

DWORD __stdcall Inventory::item_total_size(fo::GameObject* critter) {
	int totalSize = fo::func::item_c_curr_size(critter);

	if (critter->TypeFid() == fo::OBJ_TYPE_CRITTER) {
		fo::GameObject* item = fo::func::inven_right_hand(critter);
		if (item && !(item->flags & fo::ObjectFlag::Right_Hand)) {
			totalSize += fo::func::item_size(item);
		}

		fo::GameObject* itemL = fo::func::inven_left_hand(critter);
		if (itemL && item != itemL && !(itemL->flags & fo::ObjectFlag::Left_Hand)) {
			totalSize += fo::func::item_size(itemL);
		}

		item = fo::func::inven_worn(critter);
		if (item && !(item->flags & fo::ObjectFlag::Worn)) {
			totalSize += fo::func::item_size(item);
		}
	}
	return totalSize;
}

// Reimplementation of adjust_fid engine function
// Differences from vanilla:
// - doesn't use art_vault_guy_num as default art, uses current critter FID instead
// - calls AdjustFidHook that allows to hook into FID calculation
DWORD __stdcall Inventory::adjust_fid() {
	DWORD fid;
	if ((*fo::ptr::inven_dude)->TypeFid() == fo::OBJ_TYPE_CRITTER) {
		DWORD indexNum;
		DWORD weaponAnimCode = 0;
		if (sf::PartyControl::IsNpcControlled()) {
			// if NPC is under control, use current FID of critter
			indexNum = (*fo::ptr::inven_dude)->artFid & 0xFFF;
		} else {
			// vanilla logic:
			indexNum = *fo::ptr::art_vault_guy_num;
			fo::Proto* critterPro;
			if (fo::util::GetProto(*fo::ptr::inven_pid, &critterPro)) {
				indexNum = critterPro->fid & 0xFFF;
			}
			if (*fo::ptr::i_worn != nullptr) {
				fo::Proto* armorPro = fo::util::GetProto((*fo::ptr::i_worn)->protoId);
				DWORD armorFid = fo::func::stat_level(*fo::ptr::inven_dude, fo::STAT_gender) == fo::GENDER_FEMALE
				               ? armorPro->item.armor.femaleFID
				               : armorPro->item.armor.maleFID;

				if (armorFid != -1) {
					indexNum = armorFid;
				}
			}
		}
		fo::GameObject* itemInHand = fo::func::intface_is_item_right_hand()
		                           ? *fo::ptr::i_rhand
		                           : *fo::ptr::i_lhand;

		if (itemInHand != nullptr) {
			fo::Proto* itemPro;
			if (fo::util::GetProto(itemInHand->protoId, &itemPro) && itemPro->item.type == fo::item_type_weapon) {
				weaponAnimCode = itemPro->item.weapon.animationCode;
			}
		}
		fid = fo::func::art_id(fo::OBJ_TYPE_CRITTER, indexNum, 0, weaponAnimCode, 0);
	} else {
		fid = (*fo::ptr::inven_dude)->artFid;
	}
	*fo::ptr::i_fid = fid;
	// OnAdjustFid
	if (sf::HeroAppearance::appModEnabled) sf::AdjustHeroArmorArt(fid);
	sf::AdjustFidHook(fid); // should be called last
	return *fo::ptr::i_fid;
}

static __declspec(naked) void adjust_fid_replacement() {
	__asm {
		push ecx;
		push edx;
		call Inventory::adjust_fid; // return fid
		pop  edx;
		pop  ecx;
		retn;
	}
}

void Inventory::init() {
	// Replace adjust_fid_ function
	sf::MakeJump(fo::funcoffs::adjust_fid_, adjust_fid_replacement); // 0x4716E8
}

}
