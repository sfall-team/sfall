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

#include "main.h"
#include "FalloutEngine.h"

#include "Graphics.h"
#include "HeroAppearance.h"
#include "HookScripts.h"
#include "PartyControl.h"
#include "Perks.h"

#include "ReplacementFuncs.h"

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
				sProto* armorPro;
				GetProto((*ptr_i_worn)->protoId, &armorPro);
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

long __stdcall sfgame_item_weapon_range(TGameObj* source, TGameObj* weapon, long hitMode) {
	sProto* wProto;
	if (!GetProto(weapon->protoId, &wProto)) return 0;

	long isSecondMode = (hitMode && hitMode != ATKTYPE_RWEAPON_PRIMARY) ? 1 : 0;
	long range = wProto->item.weapon.maxRange[isSecondMode];

	long flagExt = wProto->item.flagsExt;
	if (isSecondMode) flagExt = (flagExt >> 4);
	long type = GetWeaponType(flagExt);

	if (type == ATKSUBTYPE_THROWING) {
		// TODO: add perkHeaveHoModFix from perks.cpp
		long heaveHoMod = fo_perk_level(source, PERK_heave_ho);
		if (heaveHoMod > 0) heaveHoMod *= 2;

		long stRange = (fo_stat_level(source, STAT_st) + heaveHoMod);
		if (stRange > 10) stRange = 10; // fix for Heave Ho!
		stRange *= 3;
		if (stRange < range) range = stRange;
	}
	return range;
}

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

		if (source->id == PLAYER_ID && sfgame_trait_level(TRAIT_fast_shot)) {
			// Fallout 1 behavior and Alternative behavior (allowed for all weapons)
			bool allow = false; // TODO: add FastShotFix variable

						// Fallout 2 behavior (with fix) and Haenlomal's fix
			if (allow || (fo_item_w_range(source, hitMode) >= 2 && type > ATKSUBTYPE_MELEE)) cost--;
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

static std::vector<int> buildLineTiles;

static bool __stdcall TileExists(long tile) {
	return (std::find(buildLineTiles.cbegin(), buildLineTiles.cend(), tile) != buildLineTiles.cend());
}

// Fixed and improved implementation of tile_num_beyond_ engine function
// compared to the original implementation, this function gets the hex (tile) from the constructed line more correctly
long __fastcall sfgame_tile_num_beyond(long sourceTile, long targetTile, long maxRange) {
	if (maxRange <= 0 || sourceTile == targetTile) return sourceTile;

	if (buildLineTiles.empty()) {
		buildLineTiles.reserve(100);
	} else {
		buildLineTiles.clear();
	}

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

	// Shift the starting point depending on the direction of the line building
	// to reduce the inaccuracy when getting the tile from the x/y coordinates
	// TODO: find a better way without having to shift the point
	long direction = (source_X != target_X) ? fo_tile_dir(sourceTile, targetTile) : -1;
	//fo_debug_printf("\ntile_dir: %d", direction);
	switch (direction) {
		case 0:
			target_X += 8;
			target_Y -= 4;
			break;
		case 1:
			target_X += 15;
			break;
		case 2:
			target_X += 8;
			target_Y += 4;
			break;
		case 3:
			target_X -= 8;
			target_Y += 4;
			break;
		case 4:
			target_X -= 15;
			break;
		case 5:
			target_X -= 8;
			target_Y -= 4;
			break;
	}
	const int step = 4;
	long stepCounter = step - 1;

	if (diffX_x2 > diffY_x2) {
		long stepY = diffY_x2 - (diffX_x2 >> 1);
		while (true) {
			if (++stepCounter == step) {
				long tile = fo_tile_num(target_X, target_Y);
				//fo_debug_printf("\ntile_num: %d [x:%d y:%d]", tile, target_X, target_Y);
				if (tile != lastTile) {
					if (!TileExists(tile)) {
						if (++currentRange >= maxRange || fo_tile_on_edge(tile)) return tile;
						buildLineTiles.push_back(tile);
					}
					lastTile = tile;
				}
				stepCounter = 0;
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
			if (++stepCounter == step) {
				long tile = fo_tile_num(target_X, target_Y);
				//fo_debug_printf("\ntile_num: %d [x:%d y:%d]", tile, target_X, target_Y);
				if (tile != lastTile) {
					if (!TileExists(tile)) {
						if (++currentRange >= maxRange || fo_tile_on_edge(tile)) return tile;
						buildLineTiles.push_back(tile);
					}
					lastTile = tile;
				}
				stepCounter = 0;
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
	__asm {
		//push ecx;
		push ebx;
		mov  ecx, eax;
		call sfgame_tile_num_beyond;
		pop  ecx;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

void InitReplacementHacks() {
	// Replace adjust_fid_ function
	MakeJump(adjust_fid_, adjust_fid_hack); // 0x4716E8

	// Replace the item_w_primary_mp_cost_ function with the sfall implementation
	HookCall(0x429A08, ai_search_inven_weap_hook);

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
