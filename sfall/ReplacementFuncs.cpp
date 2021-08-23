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

#include <array>

#include "main.h"
#include "FalloutEngine.h"

#include "Graphics.h"
#include "HeroAppearance.h"
#include "HookScripts.h"
#include "PartyControl.h"
#include "Perks.h"

#include "ReplacementFuncs.h"

////////////////////////////////// COMBAT AI ///////////////////////////////////

static const long aiUseItemAPCost = 2;

static long drugUsePerfFixMode;

void __stdcall sfgame_ai_check_drugs(TGameObj* source) {
	if (fo_critter_body_type(source)) return; // Robotic/Quadruped cannot use drugs

	DWORD slot = -1;
	long noInvenDrug = 0;
	bool drugWasUsed = false;

	TGameObj* lastItem = nullptr; // combatAIInfoGetLastItem_(source); unused function, always returns 0
	if (!lastItem) {
		AIcap* cap = fo_ai_cap(source);
		if (!cap) return;

		long hpPercent = 50;
		long chance = 0;

		switch ((AIpref::chem_use_mode)cap->chem_use) {
		case AIpref::CHEM_stims_when_hurt_little: // use only healing drugs
			hpPercent = 60;
			break;
		case AIpref::CHEM_stims_when_hurt_lots:   // use only healing drugs
			hpPercent = 30;
			break;
		case AIpref::CHEM_sometimes:
			if (!(*ptr_combatNumTurns % 3)) chance = 25; // every three turns
			//hpPercent = 50;
			break;
		case AIpref::CHEM_anytime:
			if (!(*ptr_combatNumTurns % 3)) chance = 75; // every three turns
			//hpPercent = 50;
			break;
		case AIpref::CHEM_always:
			chance = 100; // 99%
			break;
		case AIpref::CHEM_clean:
			return; // exit: don't use drugs
		}

		long minHP = (hpPercent * fo_stat_level(source, STAT_max_hit_points)) / 100;

		// [FIX] for AI not checking minimum hp properly for using healing drugs (prevents premature fleeing)
		if (cap->min_hp > minHP) minHP = cap->min_hp;

		while (fo_stat_level(source, STAT_current_hp) < minHP && source->critter.movePoints >= aiUseItemAPCost) {
			TGameObj* itemFind = fo_inven_find_type(source, item_type_drug, &slot);
			if (!itemFind) {
				noInvenDrug = 2; // healing drugs were not found in the inventory (was 1)
				break;
			}

			if (sfgame_IsHealingItem(itemFind) && !sfgame_item_remove_mult(source, itemFind, 1, 0x4286F0)) { // HOOK_REMOVEINVENOBJ
				if (!sfgame_UseDrugItemFunc(source, itemFind)) {                                             // HOOK_USEOBJON
					drugWasUsed = true;
				}

				source->critter.decreaseAP(aiUseItemAPCost);
				slot = -1;
			}
		}

		// use any drug (except healing drugs) if there is a chance of using it
		if (!drugWasUsed && chance > 0 && fo_roll_random(0, 100) < chance) {
			long usedCount = 0;
			slot = -1; // [FIX] start the search from the first slot
			while (source->critter.movePoints >= aiUseItemAPCost) {
				TGameObj* item = fo_inven_find_type(source, item_type_drug, &slot);
				if (!item) {
					noInvenDrug = 1;
					break;
				}

				long counter = 0;

				if (drugUsePerfFixMode > 0) {
					TGameObj* firstFindDrug = item;
					long _slot = slot; // keep current slot
					do {
						// [FIX] Allow using only the drugs listed in chem_primary_desire and healing drugs (AIDrugUsePerfFix == 2)
						while (item->protoId != cap->chem_primary_desire[counter]) if (++counter > 2) break;
						if (counter <= 2) break; // there is a match

						// [FIX] for AI not taking chem_primary_desire in AI.txt as a preference list when using drugs in the inventory
						if (drugUsePerfFixMode == 1) {
							counter = 0;
							item = fo_inven_find_type(source, item_type_drug, &slot);
							if (!item) {
								item = firstFindDrug;
								slot = _slot; // back to slot
								break;
							}
						}
					} while (counter < 3);
				} else {
					// if the drug is equal to the item in the preference list, then check the next (vanilla behavior)
					while (item->protoId == cap->chem_primary_desire[counter]) if (++counter > 2) break;
				}

				// if the preference counter is less than 3, then AI can use the drug
				if (counter < 3) {
					// if the item is NOT a healing drug
					if (!sfgame_IsHealingItem(item) && !sfgame_item_remove_mult(source, item, 1, 0x4286F0)) { // HOOK_REMOVEINVENOBJ
						if (!sfgame_UseDrugItemFunc(source, item)) {                                          // HOOK_USEOBJON
							drugWasUsed = true;
							usedCount++;
						}

						source->critter.decreaseAP(aiUseItemAPCost);
						slot = -1;

						AIpref::chem_use_mode chemUse = (AIpref::chem_use_mode)cap->chem_use;
						if (chemUse == AIpref::CHEM_sometimes ||
						    (chemUse == AIpref::CHEM_anytime && usedCount >= 2))
						{
							break;
						}
					}
				}
			}
		}
	}
	// search for drugs on the map
	if (lastItem || (!drugWasUsed && noInvenDrug)) {
		do {
			if (!lastItem) lastItem = fo_ai_search_environ(source, item_type_drug);
			if (!lastItem) lastItem = fo_ai_search_environ(source, item_type_misc_item);
			if (lastItem) lastItem = fo_ai_retrieve_object(source, lastItem);

			// [FIX] Prevent the use of healing drugs when not necessary
			// noInvenDrug: is set to 2 that healing is required
			if (lastItem && noInvenDrug != 2 && sfgame_IsHealingItem(lastItem)) {
				long maxHP = fo_stat_level(source, STAT_max_hit_points);
				if (10 + source->critter.health >= maxHP) { // quick check current HP
					return; // exit: don't use healing item
				}
			}

			if (lastItem && !sfgame_item_remove_mult(source, lastItem, 1, 0x4286F0)) { // HOOK_REMOVEINVENOBJ
				if (!sfgame_UseDrugItemFunc(source, lastItem)) lastItem = nullptr;     // HOOK_USEOBJON

				source->critter.decreaseAP(aiUseItemAPCost);
			}
		} while (lastItem && source->critter.movePoints >= aiUseItemAPCost);
	}
}

static void __declspec(naked) ai_check_drugs_hack() {
	__asm {
		push ecx;
		push eax; // source
		call sfgame_ai_check_drugs;
		pop  ecx;
		xor  eax, eax;
		retn;
	}
}

static void __declspec(naked) ai_can_use_drug_hack() {
	__asm {
		push ecx; // item
		call sfgame_IsHealingItem;
		pop  ecx;
		test al, al;
		retn;
	}
}

////////////////////////////////// INVENTORY ///////////////////////////////////

// Custom implementation of correctFidForRemovedItem_ engine function with the HOOK_INVENWIELD hook
long __stdcall sfgame_correctFidForRemovedItem(TGameObj* critter, TGameObj* item, long flags) {
	long result = InvenWieldHook_Invoke(critter, item, flags);
	if (result) fo_correctFidForRemovedItem(critter, item, flags);
	return result;
}

DWORD __stdcall sfgame_item_total_size(TGameObj* critter) {
	int totalSize = fo_item_c_curr_size(critter);

	if (critter->TypeFid() == OBJ_TYPE_CRITTER) {
		TGameObj* item = fo_inven_right_hand(critter);
		if (item && !(item->flags & ObjectFlag::Right_Hand)) {
			totalSize += fo_item_size(item);
		}

		TGameObj* itemL = fo_inven_left_hand(critter);
		if (itemL && item != itemL && !(itemL->flags & ObjectFlag::Left_Hand)) {
			totalSize += fo_item_size(itemL);
		}

		item = fo_inven_worn(critter);
		if (item && !(item->flags & ObjectFlag::Worn)) {
			totalSize += fo_item_size(item);
		}
	}
	return totalSize;
}

// Reimplementation of adjust_fid engine function
// Differences from vanilla:
// - doesn't use art_vault_guy_num as default art, uses current critter FID instead
// - calls AdjustFidHook that allows to hook into FID calculation
DWORD __stdcall sfgame_adjust_fid() {
	DWORD fid;
	if ((*ptr_inven_dude)->TypeFid() == OBJ_TYPE_CRITTER) {
		DWORD indexNum;
		DWORD weaponAnimCode = 0;
		if (PartyControl_IsNpcControlled()) {
			// if NPC is under control, use current FID of critter
			indexNum = (*ptr_inven_dude)->artFid & 0xFFF;
		} else {
			// vanilla logic:
			indexNum = *ptr_art_vault_guy_num;
			sProto* critterPro;
			if (GetProto(*ptr_inven_pid, &critterPro)) {
				indexNum = critterPro->fid & 0xFFF;
			}
			if (*ptr_i_worn != nullptr) {
				sProto* armorPro = GetProto((*ptr_i_worn)->protoId);
				DWORD armorFid = fo_stat_level(*ptr_inven_dude, STAT_gender) == GENDER_FEMALE
				               ? armorPro->item.armor.femaleFID
				               : armorPro->item.armor.maleFID;

				if (armorFid != -1) {
					indexNum = armorFid;
				}
			}
		}
		TGameObj* itemInHand = fo_intface_is_item_right_hand()
		                     ? *ptr_i_rhand
		                     : *ptr_i_lhand;

		if (itemInHand != nullptr) {
			sProto* itemPro;
			if (GetProto(itemInHand->protoId, &itemPro) && itemPro->item.type == item_type_weapon) {
				weaponAnimCode = itemPro->item.weapon.animationCode;
			}
		}
		fid = fo_art_id(OBJ_TYPE_CRITTER, indexNum, 0, weaponAnimCode, 0);
	} else {
		fid = (*ptr_inven_dude)->artFid;
	}
	*ptr_i_fid = fid;
	// OnAdjustFid
	if (appModEnabled) AdjustHeroArmorArt(fid);
	AdjustFidHook(fid); // should be called last
	return *ptr_i_fid;
}

static void __declspec(naked) adjust_fid_hack() {
	__asm {
		push ecx;
		push edx;
		call sfgame_adjust_fid; // return fid
		pop  edx;
		pop  ecx;
		retn;
	}
}

//////////////////////////////////// ITEMS /////////////////////////////////////

static const int reloadCostAP = 2; // engine default reload AP cost

static std::tr1::array<long, 3> healingItemPids = {PID_STIMPAK, PID_SUPER_STIMPAK, PID_HEALING_POWDER};

long __stdcall sfgame_GetHealingPID(long index) {
	return healingItemPids[index];
}

void __stdcall sfgame_SetHealingPID(long index, long pid) {
	healingItemPids[index] = pid;
}

bool __fastcall sfgame_IsHealingItem(TGameObj* item) {
	//for each (long pid in healingItemPids) if (pid == item->protoId) return true;
	if (healingItemPids[0] == item->protoId || healingItemPids[1] == item->protoId || healingItemPids[2] == item->protoId) {
		return true;
	}

	sProto* proto;
	if (GetProto(item->protoId, &proto)) {
		return (proto->item.flagsExt & IFLG_HealingItem) != 0;
	}
	return false;
}

bool __stdcall sfgame_UseDrugItemFunc(TGameObj* source, TGameObj* item) {
	bool result = (sfgame_item_d_take_drug(source, item) == -1);
	if (result) {
		fo_item_add_force(source, item, 1);
	} else {
		fo_ai_magic_hands(source, item, 5000);
		fo_obj_connect(item, source->tile, source->elevation, 0);
		fo_obj_destroy(item);
	}
	return result;
}

// Implementation of item_d_take_ engine function with the HOOK_USEOBJON hook
long __stdcall sfgame_item_d_take_drug(TGameObj* source, TGameObj* item) {
	if (UseObjOnHook_Invoke(source, item, source) == -1) { // default handler
		return fo_item_d_take_drug(source, item);
	}
	return -1; // cancel the drug use
}

long __stdcall sfgame_item_remove_mult(TGameObj* source, TGameObj* item, long count, long rmType) {
	SetRemoveObjectType(rmType);
	return fo_item_remove_mult(source, item, count);
}

long __stdcall sfgame_item_count(TGameObj* who, TGameObj* item) {
	for (int i = 0; i < who->invenSize; i++) {
		TGameObj::InvenItem* tableItem = &who->invenTable[i];
		if (tableItem->object == item) {
			return tableItem->count; // fix
		} else if (fo_item_get_type(tableItem->object) == item_type_container) {
			int count = sfgame_item_count(tableItem->object, item);
			if (count > 0) return count;
		}
	}
	return 0;
}

long __stdcall sfgame_item_weapon_range(TGameObj* source, TGameObj* weapon, long hitMode) {
	sProto* wProto;
	if (!GetProto(weapon->protoId, &wProto)) return 0;

	long isSecondMode = (hitMode && hitMode != ATKTYPE_RWEAPON_PRIMARY) ? 1 : 0;
	long range = wProto->item.weapon.maxRange[isSecondMode];

	long flagExt = wProto->item.flagsExt;
	if (isSecondMode) flagExt = (flagExt >> 4);
	long type = GetWeaponType(flagExt);

	if (type == ATKSUBTYPE_THROWING) {
		long heaveHoMod = fo_perk_level(source, PERK_heave_ho);
		long stRange = fo_stat_level(source, STAT_st);

		if (perkHeaveHoModTweak) {
			stRange *= 3;
			if (stRange > range) stRange = range;
			return stRange + (heaveHoMod * 6);
		}

		// vanilla
		stRange += (heaveHoMod * 2);
		if (stRange > 10) stRange = 10; // fix for Heave Ho!
		stRange *= 3;
		if (stRange < range) range = stRange;
	}
	return range;
}

// TODO
//long __stdcall sfgame_item_w_range(TGameObj* source, long hitMode) {
//	return sfgame_item_weapon_range(source, fo_item_hit_with(source, hitMode), hitMode);
//}

static bool fastShotTweak = false;

// Implementation of item_w_primary_mp_cost_ and item_w_secondary_mp_cost_ engine functions in a single function with the HOOK_CALCAPCOST hook
long __fastcall sfgame_item_weapon_mp_cost(TGameObj* source, TGameObj* weapon, long hitMode, long isCalled) {
	long cost = 0;

	switch (hitMode) {
	case ATKTYPE_LWEAPON_PRIMARY:
	case ATKTYPE_RWEAPON_PRIMARY:
		cost = fo_item_w_primary_mp_cost(weapon);
		break;
	case ATKTYPE_LWEAPON_SECONDARY:
	case ATKTYPE_RWEAPON_SECONDARY:
		cost = fo_item_w_secondary_mp_cost(weapon);
		break;
	case ATKTYPE_LWEAPON_RELOAD:
	case ATKTYPE_RWEAPON_RELOAD:
		if (weapon && weapon->protoId != PID_SOLAR_SCORCHER) { // Solar Scorcher has no reload AP cost
			cost = reloadCostAP;
			if (GetProto(weapon->protoId)->item.weapon.perk == PERK_weapon_fast_reload) {
				cost--;
			}
		}
	}
	if (hitMode < ATKTYPE_LWEAPON_RELOAD) {
		if (isCalled) cost++;
		if (cost < 0) cost = 0;

		long type = fo_item_w_subtype(weapon, hitMode);

		if (source->protoId == PID_Player && DudeHasTrait(TRAIT_fast_shot)) {
			if (fastShotTweak || // Fallout 1 behavior and Alternative behavior (allowed for all weapons)
			    (fo_item_w_range(source, hitMode) >= 2 && type > ATKSUBTYPE_MELEE)) // Fallout 2 behavior (with fix) and Haenlomal's tweak
			{
				cost--;
			}
		}
		if ((type == ATKSUBTYPE_MELEE || type == ATKSUBTYPE_UNARMED) && fo_perk_level(source, PERK_bonus_hth_attacks)) {
			cost--;
		}
		if (type == ATKSUBTYPE_GUNS && fo_perk_level(source, PERK_bonus_rate_of_fire)) {
			cost--;
		}
		if (cost < 1) cost = 1;
	}
	return CalcApCostHook_Invoke(source, hitMode, isCalled, cost, weapon);
}

// Implementation of item_w_mp_cost_ engine function with the HOOK_CALCAPCOST hook
long __stdcall sfgame_item_w_mp_cost(TGameObj* source, long hitMode, long isCalled) {
	long cost = fo_item_w_mp_cost(source, hitMode, isCalled);
	return CalcApCostHook_Invoke(source, hitMode, isCalled, cost, nullptr);
}

static void __declspec(naked) ai_search_inven_weap_hook() {
	__asm {
		push 0;        // no called
		push ATKTYPE_RWEAPON_PRIMARY;
		mov  edx, esi; // found weapon
		mov  ecx, edi; // source
		call sfgame_item_weapon_mp_cost;
		retn;
	}
}

/////////////////////////////////// OBJECTS ////////////////////////////////////

// Implementation of is_within_perception_ engine function with the HOOK_WITHINPERCEPTION hook
long __stdcall sfgame_is_within_perception(TGameObj* watcher, TGameObj* target, long hookType) {
	return PerceptionRangeHook_Invoke(watcher, target, hookType, fo_is_within_perception(watcher, target));
}

// Alternative implementation of objFindObjPtrFromID_ engine function with the type of object to find
TGameObj* __fastcall sfgame_FindObjectFromID(long id, long type) {
	TGameObj* obj = fo_obj_find_first();
	while (obj) {
		if (obj->id == id && obj->Type() == type) return obj;
		obj = fo_obj_find_next();
	}
	return nullptr;
}

//////////////////////////////////// RENDER ////////////////////////////////////

static BYTE* __stdcall GetBuffer() {
	return (BYTE*)*(DWORD*)FO_VAR_screen_buffer;
}

static void __stdcall Draw(WINinfo* win, BYTE* surface, long width, long height, long widthFrom, BYTE* toBuffer, long toWidth, RECT &rect, RECT* updateRect) {
	auto drawFunc = (win->flags & WinFlags::Transparent && win->wID) ? fo_trans_buf_to_buf : fo_buf_to_buf;
	if (toBuffer) {
		drawFunc(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		drawFunc(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth); // copy to buffer instead of DD surface (buffering)
	}

	if (!win->randY) return;
	surface = &WinRender_GetOverlaySurface(win)[rect.left - win->rect.x] + ((rect.top - win->rect.y) * win->width);

	if (toBuffer) {
		fo_trans_buf_to_buf(surface, width, height, widthFrom, &toBuffer[rect.left - updateRect->left] + ((rect.top - updateRect->top) * toWidth), toWidth);
	} else {
		fo_trans_buf_to_buf(surface, width, height, widthFrom, &GetBuffer()[rect.left] + (rect.top * toWidth), toWidth);
	}
}

void __fastcall sfgame_GNW_win_refresh(WINinfo* win, RECT* updateRect, BYTE* toBuffer) {
	if (win->flags & WinFlags::Hidden) return;
	RectList* rects;

	if (win->flags & WinFlags::Transparent && !*(DWORD*)FO_VAR_doing_refresh_all) {
		__asm {
			mov  eax, updateRect;
			mov  edx, ds:[FO_VAR_screen_buffer];
			call refresh_all_;
		}
		int w = (updateRect->right - updateRect->left) + 1;

		if (*ptr_mouse_is_hidden || !fo_mouse_in(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
			/*__asm {
				mov  eax, win;
				mov  edx, updateRect;
				call GNW_button_refresh_;
			}*/
			int h = (updateRect->bottom - updateRect->top) + 1;

			UpdateDDSurface(GetBuffer(), w, h, w, updateRect); // update the entire rectangle area

		} else {
			fo_mouse_show(); // for updating background cursor area
			RECT mouseRect;
			__asm {
				lea  eax, mouseRect;
				mov  edx, eax;
				call mouse_get_rect_;
				mov  eax, updateRect;
				call rect_clip_;
				mov  rects, eax;
			}
			while (rects) { // updates everything except the cursor area
				/*__asm {
					mov  eax, win;
					mov  edx, rects;
					call GNW_button_refresh_;
				}*/

				int wRect = (rects->wRect.right - rects->wRect.left) + 1;
				int hRect = (rects->wRect.bottom - rects->wRect.top) + 1;

				UpdateDDSurface(&GetBuffer()[rects->wRect.left - updateRect->left] + (rects->wRect.top - updateRect->top) * w, wRect, hRect, w, &rects->wRect);

				RectList* free = rects;
				rects = rects->nextRect;
				sf_rect_free(free);
			}
		}
		return;
	}

	/* Allocates memory for 10 RectList (if no memory was allocated), returns the first Rect and removes it from the list */
	__asm call rect_malloc_;
	__asm mov  rects, eax;
	if (!rects) return;

	rects->rect.x = updateRect->left;
	rects->rect.y = updateRect->top;
	rects->rect.offx = updateRect->right;
	rects->rect.offy = updateRect->bottom;
	rects->nextRect = nullptr;

	/*
		If the border of the updateRect rectangle is located outside the window, then assign to rects->rect the border of the window rectangle
		Otherwise, rects->rect contains the borders from the update rectangle (updateRect)
	*/
	if (rects->wRect.left   < win->wRect.left)   rects->wRect.left   = win->wRect.left;
	if (rects->wRect.top    < win->wRect.top)    rects->wRect.top    = win->wRect.top;
	if (rects->wRect.right  > win->wRect.right)  rects->wRect.right  = win->wRect.right;
	if (rects->wRect.bottom > win->wRect.bottom) rects->wRect.bottom = win->wRect.bottom;

	if (rects->wRect.right < rects->wRect.left || rects->wRect.bottom < rects->wRect.top) {
		sf_rect_free(rects);
		return;
	}

	int widthFrom = win->width;
	int toWidth = (toBuffer) ? (updateRect->right - updateRect->left) + 1 : Gfx_GetGameWidthRes();

	fo_win_clip(win, &rects, toBuffer);

	RectList* currRect = rects;
	while (currRect) {
		int width = (currRect->wRect.right - currRect->wRect.left) + 1;   // for current rectangle
		int height = (currRect->wRect.bottom - currRect->wRect.top) + 1;; // for current rectangle

		BYTE* surface;
		if (win->wID > 0) {
			__asm {
				mov  eax, win;
				mov  edx, currRect;
				call GNW_button_refresh_;
			}
			surface = &win->surface[currRect->wRect.left - win->rect.x] + ((currRect->wRect.top - win->rect.y) * win->width);
		} else {
			surface = new BYTE[height * width](); // black background
			widthFrom = width; // replace with rectangle
		}

		Draw(win, surface, width, height, widthFrom, toBuffer, toWidth, currRect->wRect, updateRect);

		if (win->wID == 0) delete[] surface;

		currRect = currRect->nextRect;
	}

	while (rects) {
		// copy all rectangles from the buffer to the DD surface (buffering)
		if (!toBuffer) {
			int width = (rects->rect.offx - rects->rect.x) + 1;
			int height = (rects->rect.offy - rects->rect.y) + 1;
			int widthFrom = toWidth;

			UpdateDDSurface(&GetBuffer()[rects->rect.x] + (rects->rect.y * widthFrom), width, height, widthFrom, &rects->wRect);
		}
		RectList* next = rects->nextRect;
		sf_rect_free(rects);
		rects = next;
	}

	if (!toBuffer && !*(DWORD*)FO_VAR_doing_refresh_all && !*ptr_mouse_is_hidden && fo_mouse_in(updateRect->left, updateRect->top, updateRect->right, updateRect->bottom)) {
		fo_mouse_show();
	}
}

static __declspec(naked) void GNW_win_refresh_hack() {
	__asm {
		push ebx; // toBuffer
		mov  ecx, eax;
		call sfgame_GNW_win_refresh;
		pop  ecx;
		retn;
	}
}

//////////////////////////////////// SKILLS ////////////////////////////////////

int __stdcall sfgame_trait_adjust_skill(DWORD skillID) {
	int result = 0;
	if (TraitsModEnable()) {
		if (ptr_pc_trait[0] != -1) result += GetTraitSkillBonus(skillID, 0);
		if (ptr_pc_trait[1] != -1) result += GetTraitSkillBonus(skillID, 1);
	}

	if (sfgame_trait_level(TRAIT_gifted)) result -= 10;

	if (sfgame_trait_level(TRAIT_good_natured)) {
		if (skillID <= SKILL_THROWING) {
			result -= 10;
		} else if (skillID == SKILL_FIRST_AID || skillID == SKILL_DOCTOR || skillID == SKILL_CONVERSANT || skillID == SKILL_BARTER) {
			result += 15;
		}
	}
	return result;
}

static void __declspec(naked) trait_adjust_skill_hack() {
	__asm {
		push edx;
		push ecx;
		push eax; // skillID
		call sfgame_trait_adjust_skill;
		pop  ecx;
		pop  edx;
		retn;
	}
}

//////////////////////////////////// STATS /////////////////////////////////////

static bool smallFrameTraitFix = false;

int __stdcall sfgame_trait_level(DWORD traitID) {
	return DudeHasTrait(traitID);
}

static int DudeGetBaseStat(DWORD statID) {
	return fo_stat_get_base_direct(*ptr_obj_dude, statID);
}

int __stdcall sfgame_trait_adjust_stat(DWORD statID) {
	if (statID > STAT_max_derived) return 0;

	int result = 0;
	if (TraitsModEnable()) {
		if (ptr_pc_trait[0] != -1) result += GetTraitStatBonus(statID, 0);
		if (ptr_pc_trait[1] != -1) result += GetTraitStatBonus(statID, 1);
	}

	switch (statID) {
	case STAT_st:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		if (sfgame_trait_level(TRAIT_bruiser)) result += 2;
		break;
	case STAT_pe:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		break;
	case STAT_en:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		break;
	case STAT_ch:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		break;
	case STAT_iq:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		break;
	case STAT_ag:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		if (sfgame_trait_level(TRAIT_small_frame)) result++;
		break;
	case STAT_lu:
		if (sfgame_trait_level(TRAIT_gifted)) result++;
		break;
	case STAT_max_move_points:
		if (sfgame_trait_level(TRAIT_bruiser)) result -= 2;
		break;
	case STAT_ac:
		if (sfgame_trait_level(TRAIT_kamikaze)) return -DudeGetBaseStat(STAT_ac);
		break;
	case STAT_melee_dmg:
		if (sfgame_trait_level(TRAIT_heavy_handed)) result += 4;
		break;
	case STAT_carry_amt:
		if (sfgame_trait_level(TRAIT_small_frame)) {
			int st;
			if (smallFrameTraitFix) {
				st = fo_stat_level(*ptr_obj_dude, STAT_st);
			} else {
				st = DudeGetBaseStat(STAT_st);
			}
			result -= st * 10;
		}
		break;
	case STAT_sequence:
		if (sfgame_trait_level(TRAIT_kamikaze)) result += 5;
		break;
	case STAT_heal_rate:
		if (sfgame_trait_level(TRAIT_fast_metabolism)) result += 2;
		break;
	case STAT_crit_chance:
		if (sfgame_trait_level(TRAIT_finesse)) result += 10;
		break;
	case STAT_better_crit:
		if (sfgame_trait_level(TRAIT_heavy_handed)) result -= 30;
		break;
	case STAT_rad_resist:
		if (sfgame_trait_level(TRAIT_fast_metabolism)) return -DudeGetBaseStat(STAT_rad_resist);
		break;
	case STAT_poison_resist:
		if (sfgame_trait_level(TRAIT_fast_metabolism)) return -DudeGetBaseStat(STAT_poison_resist);
		break;
	}
	return result;
}

static void __declspec(naked) trait_adjust_stat_hack() {
	__asm {
		push edx;
		push ecx;
		push eax; // statID
		call sfgame_trait_adjust_stat;
		pop  ecx;
		pop  edx;
		retn;
	}
}

/////////////////////////////////// TILEMAP ////////////////////////////////////

//static std::vector<int> buildLineTiles;

//static bool __stdcall TileExists(long tile) {
//	return (std::find(buildLineTiles.cbegin(), buildLineTiles.cend(), tile) != buildLineTiles.cend());
//}

// Fixed and improved implementation of tile_num_beyond_ engine function
// - correctly gets the tile from the constructed line
// - fixed the range when the target tile was positioned at the maximum distance
long __fastcall sfgame_tile_num_beyond(long sourceTile, long targetTile, long maxRange) {
	if (maxRange <= 0 || sourceTile == targetTile) return sourceTile;

	/*if (buildLineTiles.empty()) {
		buildLineTiles.reserve(100);
	} else {
		buildLineTiles.clear();
	}*/

	long currentRange = fo_tile_dist(sourceTile, targetTile);
	//fo_debug_printf("\ntile_dist: %d", currentRange);
	if (currentRange == maxRange) maxRange++; // increase the range if the target is located at a distance equal to maxRange (fix range)

	long lastTile = targetTile;
	long source_X, source_Y, target_X, target_Y;

	fo_tile_coord(sourceTile, &source_X, &source_Y);
	fo_tile_coord(targetTile, &target_X, &target_Y);

	// set the point to the center of the hexagon
	source_X += 16;
	source_Y += 8;
	target_X += 16;
	target_Y += 8;

	long diffX = target_X - source_X;
	long addToX = -1;
	if (diffX >= 0) addToX = (diffX > 0);

	long diffY = target_Y - source_Y;
	long addToY = -1;
	if (diffY >= 0) addToY = (diffY > 0);

	long diffX_x2 = 2 * std::abs(diffX);
	long diffY_x2 = 2 * std::abs(diffY);

	const int step = 4;
	long stepCounter = 1;

	if (diffX_x2 > diffY_x2) {
		long stepY = diffY_x2 - (diffX_x2 >> 1);
		while (true) {
			if (!--stepCounter) {
				long tile = fo_tile_num(target_X, target_Y);
				//fo_debug_printf("\ntile_num: %d [x:%d y:%d]", tile, target_X, target_Y);
				if (tile != lastTile) {
					//if (!TileExists(tile)) {
						long dist = fo_tile_dist(targetTile, tile);
						if ((dist + currentRange) >= maxRange || fo_tile_on_edge(tile)) return tile;
					//	buildLineTiles.push_back(tile);
					//}
					lastTile = tile;
				}
				stepCounter = step;
			}
			if (stepY >= 0) {
				stepY -= diffX_x2;
				target_Y += addToY;
			}
			stepY += diffY_x2;
			target_X += addToX;

			// Example of an algorithm constructing a straight line from point A to B
			// source: x = 784(800), y = 278(286)
			// target: x = 640(656), y = 314(322) - the target is located to the left and below
			// 0. x = 800, y = 286, stepY = -72
			// 1. x = 799, y = 286, stepY = -72+72=0
			// 2. x = 798, y = 287, stepY = 0-288+72=-216
			// 3. x = 797, y = 287, stepY = -216+72=-144
			// 4. x = 796, y = 287, stepY = -144+72=-72
			// 5. x = 795, y = 287, stepY = -72+72=0
			// 6. x = 794, y = 288, stepY = 0-288+72=-216
		}
	} else {
		long stepX = diffX_x2 - (diffY_x2 >> 1);
		while (true) {
			if (!--stepCounter) {
				long tile = fo_tile_num(target_X, target_Y);
				//fo_debug_printf("\ntile_num: %d [x:%d y:%d]", tile, target_X, target_Y);
				if (tile != lastTile) {
					//if (!TileExists(tile)) {
						long dist = fo_tile_dist(targetTile, tile);
						if ((dist + currentRange) >= maxRange || fo_tile_on_edge(tile)) return tile;
					//	buildLineTiles.push_back(tile);
					//}
					lastTile = tile;
				}
				stepCounter = step;
			}
			if (stepX >= 0) {
				stepX -= diffY_x2;
				target_X += addToX;
			}
			stepX += diffX_x2;
			target_Y += addToY;
		}
	}
}

static void __declspec(naked) tile_num_beyond_hack() {
	__asm { //push ecx;
		push ebx;
		mov  ecx, eax;
		call sfgame_tile_num_beyond;
		pop  ecx;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

void InitReplacementHacks() {
	// Replace ai_check_drugs_ function for code fixes and checking healing items
	MakeJump(ai_check_drugs_, ai_check_drugs_hack); // 0x428480

	// Change ai_can_use_drug_ function code to check healing items
	MakeCall(0x429BDE, ai_can_use_drug_hack, 6);
	SafeWrite8(0x429BE9, CODETYPE_JumpNZ);    // jz > jnz
	SafeWrite8(0x429BF1, CODETYPE_JumpShort); // jnz > jmp

	drugUsePerfFixMode = GetConfigInt("Misc", "AIDrugUsePerfFix", 0);
	if (drugUsePerfFixMode > 0) dlogr("Applying AI drug use preference fix.", DL_FIX);

	// Replace adjust_fid_ function
	MakeJump(adjust_fid_, adjust_fid_hack); // 0x4716E8

	// Replace the item_w_primary_mp_cost_ function with the sfall implementation
	HookCall(0x429A08, ai_search_inven_weap_hook);

	int fastShotFix = GetConfigInt("Misc", "FastShotFix", 0);
	fastShotTweak = (fastShotFix > 0 && fastShotFix <= 3);

	// Replace the srcCopy_ function with a pure MMX implementation
	MakeJump(buf_to_buf_, fo_buf_to_buf); // 0x4D36D4
	// Replace the transSrcCopy_ function
	MakeJump(trans_buf_to_buf_, fo_trans_buf_to_buf); // 0x4D3704

	// Custom implementation of the GNW_win_refresh function
	MakeJump(0x4D6FD9, GNW_win_refresh_hack, 1);
	// Replace _screendump_buf with _screen_buffer for creating screenshots
	const DWORD scrdumpBufAddr[] = {0x4C8FD1, 0x4C900D};
	SafeWriteBatch<DWORD>(FO_VAR_screen_buffer, scrdumpBufAddr);

	// Replace trait_adjust_*_ functions
	MakeJump(trait_adjust_skill_, trait_adjust_skill_hack); // 0x4B40FC
	MakeJump(trait_adjust_stat_, trait_adjust_stat_hack);   // 0x4B3C7C

	// Fix the carry weight penalty of the Small Frame trait not being applied to bonus Strength points
	smallFrameTraitFix = (GetConfigInt("Misc", "SmallFrameFix", 0) != 0);

	// Replace tile_num_beyond_ function
	MakeJump(tile_num_beyond_ + 1, tile_num_beyond_hack); // 0x4B1B84
}
