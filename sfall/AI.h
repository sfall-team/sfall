/*
 *    sfall
 *    Copyright (C) 2012  The sfall team
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

#include "FalloutEngine.h"

void AI_Init();

TGameObj* AI_CheckShootAndFriendlyInLineOfFire(TGameObj* object, long targetTile, long team);
TGameObj* AI_CheckFriendlyFire(TGameObj* target, TGameObj* attacker);

void __stdcall AICombatStart();
void __stdcall AICombatEnd();

TGameObj* __stdcall AIGetLastAttacker(TGameObj* target);
TGameObj* __stdcall AIGetLastTarget(TGameObj* source);
