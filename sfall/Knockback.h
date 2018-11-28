/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2011  The sfall team
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

void _stdcall SetPickpocketMax(TGameObj* critter, DWORD maximum, DWORD mod);
void _stdcall SetHitChanceMax(TGameObj* critter, DWORD maximum, DWORD mod);
void _stdcall KnockbackSetMod(TGameObj* id, DWORD type, float val, DWORD on);
void _stdcall KnockbackRemoveMod(TGameObj* id, DWORD on);
void KnockbackInit();
void Knockback_OnGameLoad();
void _stdcall SetNoBurstMode(TGameObj* critter, DWORD on);
void _stdcall DisableAimedShots(DWORD pid);
void _stdcall ForceAimedShots(DWORD pid);
