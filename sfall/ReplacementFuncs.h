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

void InitReplacementHacks();

// Custom implementation of correctFidForRemovedItem_ engine function with the HOOK_INVENWIELD hook
long __stdcall sfgame_correctFidForRemovedItem(TGameObj* critter, TGameObj* item, long flags);

// This function returns the size of the occupied inventory space for the object or critter
// - difference from the item_c_curr_size_ function: includes the size of equipped items for the critter
// - does not return the size of nested items
DWORD __stdcall sfgame_item_total_size(TGameObj* critter);

// Reimplementation of adjust_fid engine function
// Differences from vanilla:
// - doesn't use art_vault_guy_num as default art, uses current critter FID instead
// - calls AdjustFidHook that allows to hook into FID calculation
DWORD __stdcall sfgame_adjust_fid();

long __stdcall sfgame_item_weapon_range(TGameObj* source, TGameObj* weapon, long hitMode);

// Implementation of item_w_primary_mp_cost_ and item_w_secondary_mp_cost_ engine functions in a single function with the HOOK_CALCAPCOST hook
long __stdcall sfgame_item_weapon_mp_cost(TGameObj* source, TGameObj* weapon, long hitMode, long isCalled);

// Implementation of item_w_mp_cost_ engine function with the HOOK_CALCAPCOST hook
long __fastcall sfgame_item_w_mp_cost(TGameObj* source, long hitMode, long isCalled);

// Implementation of is_within_perception_ engine function with the HOOK_WITHINPERCEPTION hook
long __stdcall sfgame_is_within_perception(TGameObj* watcher, TGameObj* target, long hookType);

void __fastcall sfgame_GNW_win_refresh(WINinfo* win, RECT* updateRect, BYTE* toBuffer);

int __stdcall sfgame_trait_adjust_skill(DWORD skillID);

int __stdcall sfgame_trait_level(DWORD traitID);

int __stdcall sfgame_trait_adjust_stat(DWORD statID);
