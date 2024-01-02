/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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
private:
	static bool IsTraitDisabled(int traitID);

public:
	const char* name() { return "Perks"; }
	void init();

	// Enable the modification of the stats/skills for traits by using the perks ini file
	static int TraitsModEnable();
	static DWORD GetTraitStatBonus(int statID, int traitIndex);
	static DWORD GetTraitSkillBonus(int skillID, int traitIndex);

	static void Save(HANDLE file);
	static bool Load(HANDLE file);

	static void SetSelectablePerk(const char* name, int active, int image, const char* desc, int npcID = 0);
	static void SetFakePerk(const char* name, int level, int image, const char* desc, int npcID = 0);
	static void SetFakeTrait(const char* name, int active, int image, const char* desc, int npcID = 0);

	static DWORD HasFakePerk(const char* name, long perkId);
	static DWORD HasFakeTrait(const char* name);
	static DWORD HasFakePerkOwner(const char* name, long objId);
	static DWORD HasFakeTraitOwner(const char* name, long objId);

	static long PerkLevelMod;

	static void __fastcall SetPerkValue(int id, int param, int value);
	static void __fastcall SetPerkboxTitle(const char* name);
	static void SetPerkName(int id, const char* value);
	static void SetPerkDesc(int id, const char* value);

	static __forceinline bool DudeHasTrait(DWORD traitID) {
		return (!IsTraitDisabled(traitID) && (fo::var::pc_trait[0] == traitID || fo::var::pc_trait[1] == traitID));
	}

	static bool perkHeaveHoModTweak;
	static void __stdcall ApplyHeaveHoFix();

	static int fastShotTweak;
};

void PerksEnterCharScreen();
void PerksCancelCharScreen();
void PerksAcceptCharScreen();

void __stdcall IgnoreDefaultPerks();
void __stdcall RestoreDefaultPerks();
void __stdcall AddPerkMode(DWORD mode);
void __stdcall ClearSelectablePerks();

void __stdcall SetPerkFreq(int i);

}
