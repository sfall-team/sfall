/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Perks.h"

namespace sfall
{

long Perks::PerkLevelMod = 0;

static const int maxNameLen = 64;   // don't change size
static const int maxDescLen = 512;  // don't change size
static const int descLen = 256;     // maximum text length for interface

static char perksFile[MAX_PATH] = {0};

static char Name[maxNameLen * fo::Perk::PERK_count] = {0};
static char Desc[descLen * fo::Perk::PERK_count] = {0};
static char tName[maxNameLen * fo::Trait::TRAIT_count] = {0};
static char tDesc[descLen * fo::Trait::TRAIT_count] = {0};
static char PerkBoxTitle[33];

static DWORD addPerkMode = 2;

static bool perksReInit = false;
static int perksEnable = 0;
static int traitsEnable = 0;

static fo::PerkInfo perks[fo::Perk::PERK_count];
static fo::TraitInfo traits[fo::Trait::TRAIT_count];

#pragma pack(push, 1)
struct FakePerk {
	int Level; // current level (max 100)
	int Image;
	char Name[maxNameLen];
	char Desc[maxDescLen];
	char reserve[512]; // empty block
};
#pragma pack(pop)

std::vector<FakePerk> fakeTraits;
std::vector<FakePerk> fakePerks;
std::vector<FakePerk> fakeSelectablePerks; // available perks for selection in the perk selection list

static long RemoveTraitID = -1;
static std::list<int> RemovePerkID;
static std::list<int> RemoveSelectableID;

static DWORD traitSkillBonuses[fo::Trait::TRAIT_count * 18] = {0};
static DWORD traitStatBonuses[fo::Trait::TRAIT_count * (fo::STAT_max_derived + 1)] = {0};

static bool disableTraits[fo::Trait::TRAIT_count];
static DWORD IgnoringDefaultPerks = 0;

static DWORD PerkFreqOverride = 0;

static const DWORD GainStatPerks[7][2] = {
	{0x4AF122, 0xC9}, // Strength     // mov  ecx, ecx
	{0x4AF184, 0xC9}, // Perception
	{0x4AF19F, 0x90}, // Endurance    // nop
	{0x4AF1C0, 0xC9}, // Charisma
	{0x4AF217, 0xC9}, // Intelligance
	{0x4AF232, 0x90}, // Agility
	{0x4AF24D, 0x90}, // Luck
};

void __stdcall SetPerkFreq(int i) {
	PerkFreqOverride = i;
}

static void __declspec(naked) LevelUpHack() {
	using namespace fo;
	__asm {
		push ecx;
		mov  ecx, PerkFreqOverride;
		test ecx, ecx;
		jnz  afterSkilled;
		push TRAIT_skilled;
		call Perks::IsTraitDisabled;
		test al, al;
		jnz  notSkilled;
		mov  eax, TRAIT_skilled;
		call fo::funcoffs::trait_level_; // Check if the player has the skilled trait
		test eax, eax;
		jz   notSkilled;
		mov  ecx, 4;
		jmp  afterSkilled;
notSkilled:
		mov  ecx, 3;
afterSkilled:
		mov  eax, ds:[FO_VAR_Level_pc]; // Get player's level
		inc  eax;
		xor  edx, edx;
		div  ecx;
		test edx, edx;
		jnz  end;
		inc  byte ptr ds:[FO_VAR_free_perk]; // Increment the number of perks owed
end:
		pop  ecx;
		mov  edx, ds:[FO_VAR_Level_pc];
		retn;
	}
}

static void __declspec(naked) GetPerkBoxTitleHook() {
	__asm {
		lea  eax, PerkBoxTitle;
		retn;
	}
}

void __stdcall IgnoreDefaultPerks() {
	IgnoringDefaultPerks = 1;
}

void __stdcall RestoreDefaultPerks() {
	IgnoringDefaultPerks = 0;
}

void __fastcall Perks::SetPerkboxTitle(const char* name) {
	if (name[0] == '\0') {
		PerkBoxTitle[0] = 0;
		SafeWrite32(0x43C77D, 0x488CB);
	} else {
		strncpy_s(PerkBoxTitle, name, _TRUNCATE);
		HookCall(0x43C77C, GetPerkBoxTitleHook);
	}
}

void Perks::SetSelectablePerk(const char* name, int active, int image, const char* desc) {
	if (active < 0) return;
	if (active > 1) active = 1;
	size_t size = fakeSelectablePerks.size();
	if (active == 0) {
		for (size_t i = 0; i < size; i++) {
			if (!strcmp(name, fakeSelectablePerks[i].Name)) {
				fakeSelectablePerks.erase(fakeSelectablePerks.begin() + i);
				return;
			}
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			if (!strcmp(name, fakeSelectablePerks[i].Name)) {
				fakeSelectablePerks[i].Level = active;
				fakeSelectablePerks[i].Image = image;
				strncpy(fakeSelectablePerks[i].Desc, desc, descLen - 1);
				fakeSelectablePerks[i].Desc[descLen - 1] = 0;
				return;
			}
		}
		FakePerk fp;
		memset(&fp, 0, sizeof(FakePerk));
		fp.Level = active;
		fp.Image = image;
		strncpy_s(fp.Name, name, _TRUNCATE);
		strncpy_s(fp.Desc, desc, _TRUNCATE);
		fakeSelectablePerks.push_back(fp);
	}
}

void Perks::SetFakePerk(const char* name, int level, int image, const char* desc) {
	if (level < 0) return;
	if (level > 100) level = 100;
	size_t size = fakePerks.size();
	if (level == 0) { // remove perk from fakePerks
		for (size_t i = 0; i < size; i++) {
			if (!strcmp(name, fakePerks[i].Name)) {
				fakePerks.erase(fakePerks.begin() + i);
				return;
			}
		}
	} else { // add or change the existing fake perk in fakePerks
		for (size_t i = 0; i < size; i++) {
			if (!strcmp(name, fakePerks[i].Name)) {
				fakePerks[i].Level = level;
				fakePerks[i].Image = image;
				strncpy(fakePerks[i].Desc, desc, descLen - 1);
				fakePerks[i].Desc[descLen - 1] = 0;
				return;
			}
		}
		FakePerk fp;
		memset(&fp, 0, sizeof(FakePerk));
		fp.Level = level;
		fp.Image = image;
		strncpy_s(fp.Name, name, _TRUNCATE);
		strncpy_s(fp.Desc, desc, _TRUNCATE);
		fakePerks.push_back(fp);
	}
}

void Perks::SetFakeTrait(const char* name, int active, int image, const char* desc) {
	if (active < 0) return;
	if (active > 1) active = 1;
	size_t size = fakeTraits.size();
	if (active == 0) {
		for (size_t i = 0; i < size; i++) {
			if (!strcmp(name, fakeTraits[i].Name)) {
				fakeTraits.erase(fakeTraits.begin() + i);
				return;
			}
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			if (!strcmp(name, fakeTraits[i].Name)) {
				fakeTraits[i].Level = active;
				fakeTraits[i].Image = image;
				strncpy(fakeTraits[i].Desc, desc, descLen - 1);
				fakeTraits[i].Desc[descLen - 1] = 0;
				return;
			}
		}
		FakePerk fp;
		memset(&fp, 0, sizeof(FakePerk));
		fp.Level = active;
		fp.Image = image;
		strncpy_s(fp.Name, name, _TRUNCATE);
		strncpy_s(fp.Desc, desc, _TRUNCATE);
		fakeTraits.push_back(fp);
	}
}

static DWORD __stdcall HaveFakeTraits2() {
	return fakeTraits.size();
}

static void __declspec(naked) HaveFakeTraits() {
	__asm {
		push ecx;
		push edx;
		call HaveFakeTraits2;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static DWORD __stdcall HaveFakePerks2() {
	return fakePerks.size();
}

static void __declspec(naked) HaveFakePerks() {
	__asm {
		push ecx;
		push edx;
		call HaveFakePerks2;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static FakePerk* __stdcall GetFakePerk2(int id) {
	return &fakePerks[id - fo::Perk::PERK_count];
}

static void __declspec(naked) GetFakePerk() {
	__asm {
		mov  eax, [esp + 4];
		push ecx;
		push edx;
		push eax;
		call GetFakePerk2;
		pop  edx;
		pop  ecx;
		retn 4;
	}
}

static FakePerk* __stdcall GetFakeSPerk2(int id) {
	return &fakeSelectablePerks[id - fo::Perk::PERK_count];
}

static void __declspec(naked) GetFakeSPerk() {
	__asm {
		mov  eax, [esp + 4];
		push ecx;
		push edx;
		push eax;
		call GetFakeSPerk2;
		pop  edx;
		pop  ecx;
		retn 4;
	}
}

static DWORD __stdcall GetFakeSPerkLevel2(int id) {
	char* name = fakeSelectablePerks[id - fo::Perk::PERK_count].Name;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (!strcmp(name, fakePerks[i].Name)) return fakePerks[i].Level;
	}
	return 0;
}

static void __declspec(naked) GetFakeSPerkLevel() {
	__asm {
		mov  eax, [esp + 4];
		push ecx;
		push edx;
		push eax;
		call GetFakeSPerkLevel2;
		pop  edx;
		pop  ecx;
		retn 4;
	}
}

static DWORD __stdcall HandleFakeTraits(int isSelect) {
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (fo::func::folder_print_line(fakeTraits[i].Name) && !isSelect) {
			isSelect = 1;
			*fo::ptr::folder_card_fid = fakeTraits[i].Image;
			*fo::ptr::folder_card_title = (DWORD)fakeTraits[i].Name;
			*fo::ptr::folder_card_title2 = 0;
			*fo::ptr::folder_card_desc = (DWORD)fakeTraits[i].Desc;
		}
	}
	return isSelect;
}

static void __declspec(naked) PlayerHasPerkHack() {
	using namespace fo;
	__asm {
		push ecx;
		call HandleFakeTraits;
		mov  ecx, eax;
		xor  ebx, ebx;
oloop:
		mov  eax, ds:[FO_VAR_obj_dude];
		mov  edx, ebx;
		call fo::funcoffs::perk_level_;
		test eax, eax;
		jnz  win;
		inc  ebx;
		cmp  ebx, PERK_count;
		jl   oloop;
		call HaveFakePerks;
		test eax, eax;
		jnz  win;
		mov  eax, 0x434446; // skip print perks
		jmp  eax;
win:
		mov  eax, 0x43438A; // print perks
		jmp  eax;
	}
}

static void __declspec(naked) PlayerHasTraitHook() {
	__asm {
		call HaveFakeTraits;
		test eax, eax;
		jz   end;
		mov  eax, 0x43425B; // print traits
		jmp  eax;
end:
		jmp  PlayerHasPerkHack;
	}
}

static void __declspec(naked) GetPerkLevelHook() {
	using namespace fo;
	__asm {
		cmp  edx, PERK_count;
		jl   end;
		push edx;
		call GetFakePerk;
		mov  eax, ds:[eax];
		retn;
end:
		jmp  fo::funcoffs::perk_level_;
	}
}

static void __declspec(naked) GetPerkImageHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakePerk;
		mov  eax, ds:[eax + 4];
		retn;
end:
		jmp  fo::funcoffs::perk_skilldex_fid_;
	}
}

static void __declspec(naked) GetPerkNameHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakePerk;
		lea  eax, ds:[eax + 8];
		retn;
end:
		jmp  fo::funcoffs::perk_name_;
	}
}

static void __declspec(naked) GetPerkDescHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakePerk;
		lea  eax, ds:[eax + 72];
		retn;
end:
		jmp  fo::funcoffs::perk_description_
	}
}

// Search all available perks for the player to display them in the character screen
static void __declspec(naked) EndPerkLoopHack() {
	static const DWORD EndPerkLoopExit = 0x434446;
	static const DWORD EndPerkLoopCont = 0x4343A5;
	using namespace fo;
	__asm {
		jl   cLoop;           // if ebx < 119
		call HaveFakePerks;   // return perks count
		add  eax, PERK_count; // total = perks count + vanilla count
		cmp  ebx, eax;        // if perkId < total then continue
		jl   cLoop;
		jmp  EndPerkLoopExit; // exit loop
cLoop:
		jmp  EndPerkLoopCont; // continue loop
	}
}

// Build a table of perks ID numbers available for selection, data buffer has limited size for 119 perks
static DWORD __stdcall HandleExtraSelectablePerks(DWORD available, DWORD* data) {
	for (size_t i = 0; i < fakeSelectablePerks.size(); i++) {
		if (available >= 119) break; // exit if the buffer is overfull
		data[available++] = fo::Perk::PERK_count + i;
	}
	return available; // total number of perks available for selection
}

static void __declspec(naked) GetAvailablePerksHook() {
	__asm {
		push ecx;
		push edx; // arg data
		cmp  IgnoringDefaultPerks, 0;
		jnz  skipDefaults;
		call fo::funcoffs::perk_make_list_; // return available count
		jmp  next;
skipDefaults:
		xor  eax, eax;
next:
		push eax; // arg available
		call HandleExtraSelectablePerks;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) GetPerkSLevelHook() {
	using namespace fo;
	__asm {
		cmp  edx, PERK_count;
		jl   end;
		push edx;
		call GetFakeSPerkLevel;
		retn;
end:
		jmp  fo::funcoffs::perk_level_;
	}
}

static void __declspec(naked) GetPerkSImageHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakeSPerk;
		mov  eax, ds:[eax + 4];
		retn;
end:
		jmp  fo::funcoffs::perk_skilldex_fid_;
	}
}

static void __declspec(naked) GetPerkSNameHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakeSPerk;
		lea  eax, ds:[eax + 8]; // Name
		retn;
end:
		jmp  fo::funcoffs::perk_name_;
	}
}

static void __declspec(naked) GetPerkSDescHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakeSPerk;
		lea  eax, ds:[eax + 72]; // Desc
		retn;
end:
		jmp  fo::funcoffs::perk_description_;
	}
}

// Adds the selected perk to the player
static long __stdcall AddFakePerk(DWORD perkID) {
	size_t count;
	bool matched = false;
	// behavior for fake perk/trait
	perkID -= fo::Perk::PERK_count;
	if (addPerkMode & 1) { // add perk to trait
		count = fakeTraits.size();
		for (size_t d = 0; d < count; d++) {
			if (!strcmp(fakeTraits[d].Name, fakeSelectablePerks[perkID].Name)) {
				matched = true;
				break;
			}
		}
		if (!matched) {
			if (RemoveTraitID == -1) RemoveTraitID = count; // index of the added trait
			fakeTraits.push_back(fakeSelectablePerks[perkID]);
		}
	}
	if (addPerkMode & 2) { // default mode
		matched = false;
		count = fakePerks.size();
		for (size_t d = 0; d < count; d++) {
			if (!strcmp(fakePerks[d].Name, fakeSelectablePerks[perkID].Name)) {
				RemovePerkID.push_back(d);
				fakePerks[d].Level++;
				matched = true;
				break;
			}
		}
		if (!matched) { // add to fakePerks
			RemovePerkID.push_back(count); // index of the added perk
			fakePerks.push_back(fakeSelectablePerks[perkID]);
		}
	}
	if (addPerkMode & 4) { // delete from selectable perks
		RemoveSelectableID.push_back(perkID); //fakeSelectablePerks.remove_at(perkID);
	}
	return 0;
}

// Adds perk from selection window to player
static void __declspec(naked) AddPerkHook() {
	using namespace fo;
	__asm {
		cmp  edx, PERK_count;
		jl   normalPerk;
		push ecx;
		push edx;
		call AddFakePerk;
		pop  ecx;
		retn;
normalPerk:
		push edx;
		call fo::funcoffs::perk_add_;
		pop  edx;
		test eax, eax;
		jnz  end;
		// fix gain perks (add to base stats instead of bonus stats)
		cmp  edx, PERK_gain_strength_perk;
		jl   end;
		cmp  edx, PERK_gain_luck_perk;
		jg   end;
		inc  ds:[edx * 4 + (FO_VAR_pc_proto + 0x24 - PERK_gain_strength_perk * 4)]; // base_stat_srength
end:
		retn;
	}
}

static void __declspec(naked) HeaveHoHook() {
	using namespace fo;
	__asm {
		xor  edx, edx;
		mov  eax, ecx;
		call fo::funcoffs::stat_level_;
		lea  ebx, [0 + eax * 4];
		sub  ebx, eax;      // ST * 3
		cmp  ebx, esi;      // ebx = dist (3*ST), esi = max dist weapon
		cmovg ebx, esi;     // if dist > max then dist = max
		mov  eax, ecx;
		mov  edx, PERK_heave_ho;
		call fo::funcoffs::perk_level_;
		lea  ecx, [0 + eax * 8];
		sub  ecx, eax;
		sub  ecx, eax;
		mov  eax, ecx;
		add  eax, ebx;      // distance = dist + (PERK_heave_ho * 6)
		push 0x478AFC;
		retn;
	}
}

bool Perks::perkHeaveHoModTweak = false;

void __stdcall Perks::ApplyHeaveHoFix() { // not really a fix
	MakeJump(0x478AC4, HeaveHoHook);
	perks[fo::Perk::PERK_heave_ho].strengthMin = 0;
	perkHeaveHoModTweak = true;
}

//////////////////////////////// ENGINE PERKS /////////////////////////////////

static class EnginePerkBonus {
public:
	long WeaponScopeRangePenalty;
	long WeaponScopeRangeBonus;
	long WeaponLongRangeBonus;
	long WeaponAccurateBonus;
	long WeaponHandlingBonus;

	float MasterTraderBonus;
	long SalesmanBonus;

	long LivingAnatomyBonus;
	long PyromaniacBonus;

	long StonewallPercent;

	long DemolitionExpertBonus;

	long VaultCityInoculationsPoisonBonus;
	long VaultCityInoculationsRadBonus;

	EnginePerkBonus() {
		WeaponScopeRangePenalty = 8;
		WeaponScopeRangeBonus   = 5;
		WeaponLongRangeBonus    = 4;
		WeaponAccurateBonus     = 20;
		WeaponHandlingBonus     = 3;

		MasterTraderBonus       = 25;
		SalesmanBonus           = 20;

		LivingAnatomyBonus      = 5;
		PyromaniacBonus         = 5;

		StonewallPercent        = 50;

		DemolitionExpertBonus   = 10;

		VaultCityInoculationsPoisonBonus = 10;
		VaultCityInoculationsRadBonus    = 10;
	}

	///////////////////////////////////////////

	void setWeaponScopeRangePenalty(long value) {
		if (value < 0) return;
		WeaponScopeRangePenalty = value;
		SafeWrite32(0x42448E, value);
	}

	void setWeaponScopeRangeBonus(long value) {
		if (value < 2) return;
		WeaponScopeRangeBonus = value;
		SafeWrite32(0x424489, value);
	}

	void setWeaponLongRangeBonus(long value) {
		if (value < 2) return;
		WeaponLongRangeBonus = value;
		SafeWrite32(0x424474, value);
	}

	void setWeaponAccurateBonus(long value) {
		if (value < 0) return;
		WeaponAccurateBonus = value;
		if (WeaponAccurateBonus > 125) WeaponAccurateBonus = 125;
		SafeWrite8(0x42465D, static_cast<BYTE>(WeaponAccurateBonus));
	}

	void setWeaponHandlingBonus(long value) {
		if (value < 0) return;
		WeaponHandlingBonus = value;
		if (WeaponHandlingBonus > 10) WeaponHandlingBonus = 10;
		SafeWrite8(0x424636, static_cast<char>(WeaponHandlingBonus));
		SafeWrite8(0x4251CE, static_cast<signed char>(-WeaponHandlingBonus));
	}

	void setMasterTraderBonus(long value) {
		if (value < 0) return;
		MasterTraderBonus = static_cast<float>(value);
		SafeWrite32(0x474BB3, *(DWORD*)&MasterTraderBonus); // write float data
	}

	void setSalesmanBonus(long value) {
		if (value < 0) return;
		SalesmanBonus = value;
		if (SalesmanBonus > 999) SalesmanBonus = 999;
	}

	void setLivingAnatomyBonus(long value) {
		if (value < 0) return;
		LivingAnatomyBonus = value;
		if (LivingAnatomyBonus > 125) LivingAnatomyBonus = 125;
		SafeWrite8(0x424A91, static_cast<BYTE>(LivingAnatomyBonus));
	}

	void setPyromaniacBonus(long value) {
		if (value < 0) return;
		PyromaniacBonus = value;
		if (PyromaniacBonus > 125) PyromaniacBonus = 125;
		SafeWrite8(0x424AB6, static_cast<BYTE>(PyromaniacBonus));
	}

	void setStonewallPercent(long value) {
		if (value < 0) return;
		StonewallPercent = value;
		if (StonewallPercent > 100) StonewallPercent = 100;
		SafeWrite8(0x424B50, static_cast<BYTE>(StonewallPercent));
	}

	void setDemolitionExpertBonus(long value) {
		if (value < 0) return;
		DemolitionExpertBonus = value;
		if (DemolitionExpertBonus > 999) DemolitionExpertBonus = 999;
	}

	void setVaultCityInoculationsPoisonBonus(long value) {
		if (value < -100) value = -100;
		if (value > 100) value = 100;
		VaultCityInoculationsPoisonBonus = value;
		SafeWrite8(0x4AF26A, static_cast<signed char>(VaultCityInoculationsPoisonBonus));
	}

	void setVaultCityInoculationsRadBonus(long value) {
		if (value < -100) value = -100;
		if (value > 100) value = 100;
		VaultCityInoculationsRadBonus = value;
		SafeWrite8(0x4AF287, static_cast<signed char>(VaultCityInoculationsRadBonus));
	}

} enginePerks;

static void __declspec(naked) perk_adjust_skill_hack_salesman() {
	__asm {
		imul eax, [enginePerks.SalesmanBonus];
		add  ecx, eax; // barter_skill + (perkLevel * SalesmanBonus)
		mov  eax, ecx
		retn;
	}
}

static void __declspec(naked) queue_explode_exit_hack_demolition_expert() {
	__asm {
		imul eax, [enginePerks.DemolitionExpertBonus];
		add  ecx, eax; // maxBaseDmg + (perkLevel * DemolitionExpertBonus)
		add  ebx, eax  // minBaseDmg + (perkLevel * DemolitionExpertBonus)
		retn;
	}
}

static void EnginePerkBonusInit() {
	// Allows the current perk level to affect the calculation of its bonus value
	MakeCall(0x496F5E, perk_adjust_skill_hack_salesman);
	MakeCall(0x4A289C, queue_explode_exit_hack_demolition_expert, 1);
}

static void ReadPerksBonuses(const char* perksFile) {
	int wScopeRangeMod = IniReader::GetInt("PerksTweak", "WeaponScopeRangePenalty", 8, perksFile);
	if (wScopeRangeMod != 8) enginePerks.setWeaponScopeRangePenalty(wScopeRangeMod);
	wScopeRangeMod = IniReader::GetInt("PerksTweak", "WeaponScopeRangeBonus", 5, perksFile);
	if (wScopeRangeMod != 5) enginePerks.setWeaponScopeRangeBonus(wScopeRangeMod);

	int wLongRangeBonus = IniReader::GetInt("PerksTweak", "WeaponLongRangeBonus", 4, perksFile);
	if (wLongRangeBonus != 4) enginePerks.setWeaponLongRangeBonus(wLongRangeBonus);

	int wAccurateBonus = IniReader::GetInt("PerksTweak", "WeaponAccurateBonus", 20, perksFile);
	if (wAccurateBonus != 20) enginePerks.setWeaponAccurateBonus(wAccurateBonus);

	int wHandlingBonus = IniReader::GetInt("PerksTweak", "WeaponHandlingBonus", 3, perksFile);
	if (wHandlingBonus != 3) enginePerks.setWeaponHandlingBonus(wHandlingBonus);

	int masterTraderBonus = IniReader::GetInt("PerksTweak", "MasterTraderBonus", 25, perksFile);
	if (masterTraderBonus != 25) enginePerks.setMasterTraderBonus(masterTraderBonus);

	int salesmanBonus = IniReader::GetInt("PerksTweak", "SalesmanBonus", 20, perksFile);
	if (salesmanBonus != 20) enginePerks.setSalesmanBonus(salesmanBonus);

	int livingAnatomyBonus = IniReader::GetInt("PerksTweak", "LivingAnatomyBonus", 5, perksFile);
	if (livingAnatomyBonus != 5) enginePerks.setLivingAnatomyBonus(livingAnatomyBonus);

	int pyromaniacBonus = IniReader::GetInt("PerksTweak", "PyromaniacBonus", 5, perksFile);
	if (pyromaniacBonus != 5) enginePerks.setPyromaniacBonus(pyromaniacBonus);

	int stonewallPercent = IniReader::GetInt("PerksTweak", "StonewallPercent", 50, perksFile);
	if (stonewallPercent != 50) enginePerks.setStonewallPercent(stonewallPercent);

	int demolitionExpertBonus = IniReader::GetInt("PerksTweak", "DemolitionExpertBonus", 10, perksFile);
	if (demolitionExpertBonus != 10) enginePerks.setDemolitionExpertBonus(demolitionExpertBonus);

	int vaultCityInoculationsBonus = IniReader::GetInt("PerksTweak", "VaultCityInoculationsPoisonBonus", 10, perksFile);
	if (vaultCityInoculationsBonus != 10) enginePerks.setVaultCityInoculationsPoisonBonus(vaultCityInoculationsBonus);
	vaultCityInoculationsBonus = IniReader::GetInt("PerksTweak", "VaultCityInoculationsRadBonus", 10, perksFile);
	if (vaultCityInoculationsBonus != 10) enginePerks.setVaultCityInoculationsRadBonus(vaultCityInoculationsBonus);
}

///////////////////////////////////////////////////////////////////////////////

static void PerkEngineInit() {
	EnginePerkBonusInit();

	// Character screen (list_perks_)
	HookCall(0x434256, PlayerHasTraitHook); // jz func
	MakeJump(0x43436B, PlayerHasPerkHack);
	HookCall(0x4343AC, GetPerkLevelHook);
	HookCall(0x43440D, GetPerkImageHook);
	HookCall(0x434432, GetPerkDescHook);
	MakeJump(0x434440, EndPerkLoopHack, 1);
	const DWORD getPerkNameAddr[] = {0x4343C1, 0x4343DF, 0x43441B};
	HookCalls(GetPerkNameHook, getPerkNameAddr);

	// GetPlayerAvailablePerks (ListDPerks_)
	HookCall(0x43D127, GetAvailablePerksHook);

	// ShowPerkBox (perks_dialog_)
	HookCall(0x43C952, AddPerkHook);

	// PerkboxSwitchPerk (RedrwDPrks_)

	// GetPerkS hooks
	const DWORD getPerkSLevelAddr[] = {
		0x43D25E, 0x43D275, // ListDPerks_
		0x43C82E, 0x43C85B, // perks_dialog_
		0x43C3F1, 0x43C41E  // RedrwDPrks_
	};
	HookCalls(GetPerkSLevelHook, getPerkSLevelAddr);
	const DWORD getPerkSNameAddr[] = {
		0x43D17D,           // ListDPerks_
		0x43C8A6, 0x43C8EF, // perks_dialog_
		0x43C469, 0x43C4B2  // RedrwDPrks_
	};
	HookCalls(GetPerkSNameHook, getPerkSNameAddr);
	const DWORD getPerkSDescAddr[] = {
		0x43C888, 0x43C8D1, // perks_dialog_
		0x43C44B, 0x43C494  // RedrwDPrks_
	};
	HookCalls(GetPerkSDescHook, getPerkSDescAddr);
	const DWORD getPerkSImageAddr[] = {
		0x43C90F,           // perks_dialog_
		0x43C4D2            // RedrwDPrks_
	};
	HookCalls(GetPerkSImageHook, getPerkSImageAddr);

	// perk_owed hooks
	MakeCall(0x4AFB2F, LevelUpHack, 1); // replaces 'mov edx, ds:[PlayerLevel]'
	SafeWrite8(0x43C2EC, CodeType::JumpShort); // skip the block of code which checks if the player has gained a perk (now handled in level up code)
}

static void PerkSetup() {
	if (!perksReInit) {
		// _perk_data
		const DWORD perkDataAddr[] = {0x496669, 0x496837, 0x496BAD, 0x496C41, 0x496D25};
		SafeWriteBatch<DWORD>((DWORD)perks, perkDataAddr);
		const DWORD perkDataDescAddr[] = {0x496696, 0x496BD1};
		SafeWriteBatch<DWORD>((DWORD)&perks[0].description, perkDataDescAddr);
		SafeWrite32(0x496BF5, (DWORD)&perks[0].image);
		SafeWrite32(0x496AD4, (DWORD)&perks[0].ranks);
	}
	std::memcpy(perks, (void*)FO_VAR_perk_data, sizeof(fo::PerkInfo) * fo::Perk::PERK_count); // copy vanilla data

	if (perksEnable) {
		char num[4];
		for (int i = 0; i < fo::Perk::PERK_count; i++) {
			_itoa(i, num, 10);
			if (IniReader::GetString(num, "Name", "", &Name[i * maxNameLen], maxNameLen - 1, perksFile)) {
				perks[i].name = &Name[i * maxNameLen];
			}
			if (IniReader::GetString(num, "Desc", "", &Desc[i * descLen], descLen - 1, perksFile)) {
				perks[i].description = &Desc[i * descLen];
			}
			int value;
			value = IniReader::GetInt(num, "Image", -99999, perksFile);
			if (value != -99999) perks[i].image = value;
			value = IniReader::GetInt(num, "Ranks", -99999, perksFile);
			if (value != -99999) perks[i].ranks = value;
			value = IniReader::GetInt(num, "Level", -99999, perksFile);
			if (value != -99999) perks[i].levelMin = value;
			value = IniReader::GetInt(num, "Stat", -99999, perksFile);
			if (value != -99999) perks[i].stat = value;
			value = IniReader::GetInt(num, "StatMag", -99999, perksFile);
			if (value != -99999) perks[i].statMod = value;
			value = IniReader::GetInt(num, "Skill1", -99999, perksFile);
			if (value != -99999) perks[i].skill1 = value;
			value = IniReader::GetInt(num, "Skill1Mag", -99999, perksFile);
			if (value != -99999) perks[i].skill1Min = value;
			value = IniReader::GetInt(num, "Type", -99999, perksFile);
			if (value != -99999) perks[i].skillOperator = value;
			value = IniReader::GetInt(num, "Skill2", -99999, perksFile);
			if (value != -99999) perks[i].skill2 = value;
			value = IniReader::GetInt(num, "Skill2Mag", -99999, perksFile);
			if (value != -99999) perks[i].skill2Min = value;
			value = IniReader::GetInt(num, "STR", -99999, perksFile);
			if (value != -99999) perks[i].strengthMin = value;
			value = IniReader::GetInt(num, "PER", -99999, perksFile);
			if (value != -99999) perks[i].perceptionMin = value;
			value = IniReader::GetInt(num, "END", -99999, perksFile);
			if (value != -99999) perks[i].enduranceMin = value;
			value = IniReader::GetInt(num, "CHR", -99999, perksFile);
			if (value != -99999) perks[i].charismaMin = value;
			value = IniReader::GetInt(num, "INT", -99999, perksFile);
			if (value != -99999) perks[i].intelligenceMin = value;
			value = IniReader::GetInt(num, "AGL", -99999, perksFile);
			if (value != -99999) perks[i].agilityMin = value;
			value = IniReader::GetInt(num, "LCK", -99999, perksFile);
			if (value != -99999) perks[i].luckMin = value;
		}
	}
	perksReInit = false;
}

/////////////////////////// TRAIT FUNCTIONS ///////////////////////////////////

int Perks::TraitsModEnable() {
	return traitsEnable;
}

bool Perks::IsTraitDisabled(int traitID) {
	return disableTraits[traitID];
}

DWORD Perks::GetTraitStatBonus(int statID, int traitIndex) {
	return traitStatBonuses[statID * fo::Trait::TRAIT_count + fo::ptr::pc_trait[traitIndex]];
}

DWORD Perks::GetTraitSkillBonus(int skillID, int traitIndex) {
	return traitSkillBonuses[skillID * fo::Trait::TRAIT_count + fo::ptr::pc_trait[traitIndex]];
}

static void __declspec(naked) BlockedTrait() {
	__asm {
		xor  eax, eax;
		retn;
	}
}

static void PerkAndTraitSetup() {
	PerkSetup();

	if (!traitsEnable) return;

	std::memcpy(traits, (void*)FO_VAR_trait_data, sizeof(fo::TraitInfo) * fo::Trait::TRAIT_count);

	// _trait_data
	const DWORD traitDataAddr[] = {0x4B3A81, 0x4B3B80};
	SafeWriteBatch<DWORD>((DWORD)traits, traitDataAddr);
	const DWORD traitDataDescAddr[] = {0x4B3AAE, 0x4B3BA0};
	SafeWriteBatch<DWORD>((DWORD)&traits[0].description, traitDataDescAddr);
	SafeWrite32(0x4B3BC0, (DWORD)&traits[0].image);

	char buf[512], num[5] = {'t'};
	char* num2 = &num[1];
	for (int i = 0; i < fo::Trait::TRAIT_count; i++) {
		_itoa_s(i, num2, 4, 10);
		if (IniReader::GetString(num, "Name", "", &tName[i * maxNameLen], maxNameLen - 1, perksFile)) {
			traits[i].name = &tName[i * maxNameLen];
		}
		if (IniReader::GetString(num, "Desc", "", &tDesc[i * descLen], descLen - 1, perksFile)) {
			traits[i].description = &tDesc[i * descLen];
		}
		int value;
		value = IniReader::GetInt(num, "Image", -99999, perksFile);
		if (value != -99999) traits[i].image = value;

		if (IniReader::GetString(num, "StatMod", "", buf, 512, perksFile) > 0) {
			char *stat, *mod;
			stat = strtok(buf, "|");
			mod = strtok(0, "|");
			while (stat&&mod) {
				int _stat = atoi(stat), _mod = atoi(mod);
				if (_stat >= 0 && _stat <= fo::STAT_max_derived) traitStatBonuses[_stat * fo::Trait::TRAIT_count + i] = _mod;
				stat = strtok(0, "|");
				mod = strtok(0, "|");
			}
		}

		if (IniReader::GetString(num, "SkillMod", "", buf, 512, perksFile) > 0) {
			char *stat, *mod;
			stat = strtok(buf, "|");
			mod = strtok(0, "|");
			while (stat&&mod) {
				int _stat = atoi(stat), _mod = atoi(mod);
				if (_stat >= 0 && _stat < 18) traitSkillBonuses[_stat * fo::Trait::TRAIT_count + i] = _mod;
				stat = strtok(0, "|");
				mod = strtok(0, "|");
			}
		}

		if (IniReader::GetInt(num, "NoHardcode", 0, perksFile)) {
			disableTraits[i] = true;
			switch (i) {
			case fo::Trait::TRAIT_one_hander:
				HookCall(0x4245E0, BlockedTrait); // determine_to_hit_func_
				break;
			case fo::Trait::TRAIT_finesse:
				HookCall(0x4248F9, BlockedTrait); // compute_damage_
				break;
			case fo::Trait::TRAIT_fast_shot:
				//HookCall(0x478C8A, BlockedTrait); // item_w_mp_cost_ (obsolete)
				HookCall(0x478E70, BlockedTrait); // item_w_called_shot_
				break;
			case fo::Trait::TRAIT_bloody_mess:
				HookCall(0x410707, BlockedTrait); // pick_death_
				break;
			case fo::Trait::TRAIT_jinxed:
				HookCall(0x42389F, BlockedTrait); // compute_attack_
				break;
			case fo::Trait::TRAIT_drug_addict:
				HookCall(0x47A0CD, BlockedTrait); // item_d_take_drug_
				HookCall(0x47A51A, BlockedTrait); // perform_withdrawal_start_
				break;
			case fo::Trait::TRAIT_drug_resistant:
				HookCall(0x479BE1, BlockedTrait); // insert_drug_effect_
				HookCall(0x47A0DD, BlockedTrait); // item_d_take_drug_
				break;
			case fo::Trait::TRAIT_skilled:
				HookCall(0x43C295, BlockedTrait); // UpdateLevel_
				HookCall(0x43C2F3, BlockedTrait); // UpdateLevel_
				break;
			case fo::Trait::TRAIT_gifted:
				HookCall(0x43C2A4, BlockedTrait); // UpdateLevel_
				break;
			}
		}
	}
}

static __declspec(naked) void game_init_hook() {
	__asm {
		call fo::funcoffs::trait_init_;
		jmp  PerkAndTraitSetup;
	}
}

static void __declspec(naked) perks_dialog_hook() {
	static const DWORD perks_dialog_Ret = 0x43C92F;
	__asm {
		call fo::funcoffs::ListDPerks_;
		test eax, eax;
		jz   dlgExit;
		retn;
dlgExit:
		add  esp, 4;
		jmp  perks_dialog_Ret;
	}
}
/*
static void __declspec(naked) item_w_mp_cost_hook() {
	__asm {
		call fo::funcoffs::item_w_range_;
		cmp  eax, 2;
		jge  checkType;                     // is weapon range less than 2?
		retn;                               // yes, skip -1 AP cost (0x478CA2)
checkType:
		mov  eax, edi;                      // source
		mov  edx, ecx;                      // hit_mode
		call fo::funcoffs::item_hit_with_;  // get pointer to weapon
		mov  edx, ecx;                      // hit_mode
		jmp  fo::funcoffs::item_w_subtype_; // eax - item
	}
}
*/
// Haenlomal's tweak
static void __declspec(naked) item_w_called_shot_hack() {
	static const DWORD FastShotTraitFix_End = 0x478E7F;
	using namespace fo;
	__asm {
		mov  edx, ecx;                     // argument for item_hit_with_: hit_mode
		mov  eax, ebx;                     // argument for item_hit_with_: pointer to source_obj (always dude_obj due to code path)
		call fo::funcoffs::item_hit_with_; // get pointer to weapon
		mov  edx, ecx;
		call fo::funcoffs::item_w_subtype_;
		cmp  eax, THROWING;                // is weapon type GUNS or THROWING?
		jge  checkRange;                   // yes
		jmp  FastShotTraitFix_End;         // continue processing called shot attempt
checkRange:
		mov  edx, ecx;                     // argument for item_w_range_: hit_mode
		mov  eax, ebx;                     // argument for item_w_range_: pointer to source_obj (always dude_obj due to code path)
		call fo::funcoffs::item_w_range_;  // get weapon's range
		cmp  eax, 2;                       // is weapon range greater than or equal to 2 (i.e. ranged attack)?
		jge  cantUse;                      // yes, disallow called shot attempt
		jmp  FastShotTraitFix_End;         // continue processing called shot attempt
cantUse:
		xor  eax, eax;                     // clean up and exit function item_w_called_shot
		pop  esi;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

int Perks::fastShotTweak;

static void FastShotTraitFix() {
	Perks::fastShotTweak = IniReader::GetConfigInt("Misc", "FastShotFix", 0);
	switch (Perks::fastShotTweak) {
	case 1:
		dlog("Applying Fast Shot trait patch (Haenlomal's tweak).", DL_INIT);
		MakeJump(0x478E79, item_w_called_shot_hack);
		goto fix;
	case 2: {
		dlog("Applying Fast Shot trait patch (Alternative behavior).", DL_INIT);
		/* Implemented in sfall item_w_mp_cost function */
		//SafeWrite16(0x478C9F, 0x9090); // item_w_mp_cost_
		//const DWORD fastShotFixF1[] = {0x478BB8, 0x478BC7, 0x478BD6, 0x478BEA, 0x478BF9, 0x478C08, 0x478C2F};
		//HookCalls((void*)0x478C7D, fastShotFixF1); // jmp 0x478C7D
		goto done;
	}
	case 3:
		dlog("Applying Fast Shot trait patch (Fallout 1 behavior).", DL_INIT);
		/* Implemented in sfall item_w_mp_cost function */
		//HookCall(0x478C97, (void*)fo::funcoffs::item_hit_with_);
		//SafeWrite16(0x478C9E, CodeType::JumpZ << 8); // ignore all unarmed attacks (cmp eax, 0; jz)
		goto done;
	default:
		dlog("Applying Fast Shot trait fix.", DL_INIT);
	fix:
		//HookCall(0x478C97, item_w_mp_cost_hook); - Fix implemented in sfall item_w_mp_cost function
	done:
		dlogr(" Done", DL_INIT);
	}
}

///////////////////////////////////////////////////////////////////////////////

void __fastcall Perks::SetPerkValue(int id, int param, int value) {
	if (id < 0 || id >= fo::Perk::PERK_count) return;
	*(DWORD*)((DWORD)(&perks[id]) + param) = value;
	perksReInit = true;
}

void Perks::SetPerkName(int id, const char* value) {
	if (id < 0 || id >= fo::Perk::PERK_count) return;
	strncpy_s(&Name[id * maxNameLen], maxNameLen, value, _TRUNCATE);
	perks[id].name = &Name[maxNameLen * id];
	perksReInit = true;
}

void Perks::SetPerkDesc(int id, const char* value) {
	if (id < 0 || id >= fo::Perk::PERK_count) return;
	strncpy_s(&Desc[id * descLen], descLen, value, _TRUNCATE);
	perks[id].description = &Desc[descLen * id];
	perksReInit = true;
}

void Perks::Reset() {
	fakeTraits.clear();
	fakePerks.clear();
	fakeSelectablePerks.clear();
	IgnoringDefaultPerks = 0;
	addPerkMode = 2;
	PerkFreqOverride = 0;

	if (PerkBoxTitle[0] != 0) {
		PerkBoxTitle[0] = 0;
		SafeWrite32(0x43C77D, 0x488CB);
	}

	// Reset some settable game values back to the defaults
	// Perk level mod
	Perks::PerkLevelMod = 0;
	// Pyromaniac bonus
	SafeWrite8(0x424AB6, 5);
	// Swift Learner bonus
	SafeWrite32(0x4AFAE2, 100);
	// Restore 'Heave Ho' modify fix
	if (Perks::perkHeaveHoModTweak) {
		SafeWrite8(0x478AC4, 0xBA);
		SafeWrite32(0x478AC5, 0x23);
		Perks::perkHeaveHoModTweak = false;
	}
	if (perksReInit) PerkSetup(); // restore perk data
}

void Perks::Save(HANDLE file) {
	DWORD unused, count = fakeTraits.size();
	WriteFile(file, &count, 4, &unused, 0);
	for (DWORD i = 0; i < count; i++) {
		WriteFile(file, &fakeTraits[i], sizeof(FakePerk), &unused, 0);
	}
	count = fakePerks.size();
	WriteFile(file, &count, 4, &unused, 0);
	for (DWORD i = 0; i < count; i++) {
		WriteFile(file, &fakePerks[i], sizeof(FakePerk), &unused, 0);
	}
	count = fakeSelectablePerks.size();
	WriteFile(file, &count, 4, &unused, 0);
	for (DWORD i = 0; i < count; i++) {
		WriteFile(file, &fakeSelectablePerks[i], sizeof(FakePerk), &unused, 0);
	}
}

bool Perks::Load(HANDLE file) {
	DWORD count, size;
	ReadFile(file, &count, 4, &size, 0);
	if (size != 4) return false;
	for (DWORD i = 0; i < count; i++) {
		FakePerk fp;
		ReadFile(file, &fp, sizeof(FakePerk), &size, 0);
		fakeTraits.push_back(fp);
	}
	ReadFile(file, &count, 4, &size, 0);
	if (size != 4) return false;
	for (DWORD i = 0; i < count; i++) {
		FakePerk fp;
		ReadFile(file, &fp, sizeof(FakePerk), &size, 0);
		fakePerks.push_back(fp);
	}
	ReadFile(file, &count, 4, &size, 0);
	if (size != 4) return false;
	for (DWORD i = 0; i < count; i++) {
		FakePerk fp;
		ReadFile(file, &fp, sizeof(FakePerk), &size, 0);
		fakeSelectablePerks.push_back(fp);
	}
	return true;
}

void __stdcall AddPerkMode(DWORD mode) {
	addPerkMode = mode;
}

DWORD Perks::HasFakePerk(const char* name) {
	if (name[0] == 0) return 0;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (!strcmp(name, fakePerks[i].Name)) {
			return fakePerks[i].Level;
		}
	}
	return 0;
}

DWORD Perks::HasFakeTrait(const char* name) {
	if (name[0] == 0) return 0;
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (!strcmp(name, fakeTraits[i].Name)) {
			return 1;
		}
	}
	return 0;
}

void __stdcall ClearSelectablePerks() {
	fakeSelectablePerks.clear();
	addPerkMode = 2;
	IgnoringDefaultPerks = 0;
	SafeWrite32(0x43C77D, 0x488CB);
}

void PerksEnterCharScreen() {
	RemoveTraitID = -1;
	RemovePerkID.clear();
	RemoveSelectableID.clear();
}

void PerksCancelCharScreen() {
	if (RemoveTraitID != -1) {
		fakeTraits.erase(fakeTraits.begin() + RemoveTraitID, fakeTraits.end());
	}
	if (RemovePerkID.size() > 1) RemovePerkID.sort(); // sorting to correctly remove from the end
	while (!RemovePerkID.empty()) {
		int index = RemovePerkID.back();
		if (!--fakePerks[index].Level) fakePerks.erase(fakePerks.begin() + index);
		RemovePerkID.pop_back();
	}
}

void PerksAcceptCharScreen() {
	if (RemoveSelectableID.size() > 1) {
		RemoveSelectableID.sort();
		RemoveSelectableID.unique();
	}
	while (!RemoveSelectableID.empty()) {
		fakeSelectablePerks.erase(fakeSelectablePerks.begin() + RemoveSelectableID.back());
		RemoveSelectableID.pop_back();
	}
}

void Perks::init() {
	FastShotTraitFix();

	// Disable losing unused perks
	SafeWrite16(0x43C369, 0x0DFE); // mov ds:[_free_perk], dh > dec ds:[_free_perk]
	// If there are unused perks, then call the perk selection window
	SafeWrite8(0x43C370, 0xB1);    // jmp 0x43C322

	// Don't show an empty perk selection window
	HookCall(0x43C80B, perks_dialog_hook);

	// Disable gain perks for bonus stats
	for (int i = fo::Stat::STAT_st; i <= fo::Stat::STAT_lu; i++) {
		SafeWrite8(GainStatPerks[i][0], (BYTE)GainStatPerks[i][1]);
	}

	PerkEngineInit();
	// Perk and Trait init
	HookCall(0x44272E, game_init_hook);

	if (IniReader::GetConfigString("Misc", "PerksFile", "", &perksFile[2], MAX_PATH - 3)) {
		perksFile[0] = '.';
		perksFile[1] = '\\';
		if (GetFileAttributes(perksFile) == INVALID_FILE_ATTRIBUTES) return;

		perksEnable = IniReader::GetInt("Perks", "Enable", 1, perksFile);
		traitsEnable = IniReader::GetInt("Traits", "Enable", 1, perksFile);

		// Engine perks settings
		ReadPerksBonuses(perksFile);
	}
}

}
