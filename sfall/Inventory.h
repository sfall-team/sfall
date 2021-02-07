/*
 *    sfall
 *    Copyright (C) 2011  Timeslip
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

void Inventory_Init();
void InventoryReset();
void InventoryKeyPressedHook(DWORD dxKey, bool pressed);

long __stdcall GetInvenApCost();
void __fastcall SetInvenApCost(int cost);

// This function returns the size of the occupied inventory space for the object or critter
// - difference from the item_c_curr_size_ function: includes the size of equipped items for the critter
// - does not return the size of nested items
DWORD __stdcall sfgame_item_total_size(TGameObj* critter);

// Reimplementation of adjust_fid engine function
// Differences from vanilla:
// - doesn't use art_vault_guy_num as default art, uses current critter FID instead
// - calls AdjustFidHook that allows to hook into FID calculation
DWORD __stdcall sfgame_adjust_fid();
