/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2012  The sfall team
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

void PerksInit();
void PerksReset();
void PerksSave(HANDLE file);
bool PerksLoad(HANDLE file);

void PerksEnterCharScreen();
void PerksCancelCharScreen();
void PerksAcceptCharScreen();

void _stdcall ApplyHeaveHoFix();

void _stdcall SetPerkValue(int id, int value, DWORD offset);
void _stdcall SetPerkName(int id, char* value);
void _stdcall SetPerkDesc(int id, char* value);

void _stdcall SetPerkboxTitle(char* title);
void _stdcall SetSelectablePerk(const char* name, int active, int image, const char* desc);
void _stdcall SetFakePerk(const char* name, int level, int image, const char* desc);
void _stdcall SetFakeTrait(const char* name, int active, int image, const char* desc);
void _stdcall IgnoreDefaultPerks();
void _stdcall RestoreDefaultPerks();

void _stdcall AddPerkMode(DWORD mode);
DWORD _stdcall HasFakePerk(const char* name);
DWORD _stdcall HasFakeTrait(const char* name);
void _stdcall ClearSelectablePerks();

void _stdcall SetPerkFreq(int i);

extern long PerkLevelMod;
