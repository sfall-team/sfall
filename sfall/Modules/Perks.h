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

#include "Module.h"

namespace sfall
{

class Perks : public Module {
public:
	const char* name() { return "Perks"; }
	void init();

	static void save(HANDLE file);
	static bool load(HANDLE file);

	static void SetSelectablePerk(const char* name, int active, int image, const char* desc, int npcID = 0);
	static void SetFakePerk(const char* name, int level, int image, const char* desc, int npcID = 0);
	static void SetFakeTrait(const char* name, int active, int image, const char* desc, int npcID = 0);

	static DWORD HasFakePerk(const char* name, long perkId);
	static DWORD HasFakeTrait(const char* name);
	static DWORD HasFakePerkOwner(const char* name, long objId);
	static DWORD HasFakeTraitOwner(const char* name, long objId);

	static long PerkLevelMod;
};

void PerksEnterCharScreen();
void PerksCancelCharScreen();
void PerksAcceptCharScreen();

void _stdcall ApplyHeaveHoFix();

void _stdcall SetPerkValue(int id, int value, DWORD offset);
void _stdcall SetPerkName(int id, char* value);
void _stdcall SetPerkDesc(int id, char* value);

void _stdcall SetPerkboxTitle(char* title);

void _stdcall IgnoreDefaultPerks();
void _stdcall RestoreDefaultPerks();
void _stdcall AddPerkMode(DWORD mode);

void _stdcall ClearSelectablePerks();
void _stdcall SetPerkFreq(int i);

}
