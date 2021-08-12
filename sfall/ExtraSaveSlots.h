/*
 *    sfall
 *    Copyright (C) 2009  Mash (Matt Wells, mashw at bigpond dot net dot au)
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

void ExtraSaveSlots_Init();
void ExtraSaveSlots_Exit();

long ExtraSaveSlots_GetSaveSlot();
void ExtraSaveSlots_SetSaveSlot(long page, long slot);

long ExtraSaveSlots_GetQuickSavePage();
long ExtraSaveSlots_GetQuickSaveSlot();
void ExtraSaveSlots_SetQuickSaveSlot(long page, long slot, long check);
