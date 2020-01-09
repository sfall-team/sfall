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

void AIInit();
void _stdcall AICombatStart();
void _stdcall AICombatEnd();

TGameObj* _stdcall AIGetLastAttacker(TGameObj* target);
TGameObj* _stdcall AIGetLastTarget(TGameObj* source);

void _stdcall AIBlockCombat(DWORD i);
