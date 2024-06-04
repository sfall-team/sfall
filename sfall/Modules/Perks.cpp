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

#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"
#include "PartyControl.h"

#include "SubModules\EnginePerks.h"

#include "Perks.h"

namespace sfall
{

long Perks::PerkLevelMod = 0;

constexpr int maxNameLen = 64;   // don't change size
constexpr int maxDescLen = 512;  // don't change size
static const int descLen = 256;  // maximum text length for interface

static char perksFile[MAX_PATH] = {0};

static char Name[maxNameLen * fo::Perk::PERK_count] = {0};
static char Desc[descLen * fo::Perk::PERK_count] = {0};
static char tName[maxNameLen * fo::Trait::TRAIT_count] = {0};
static char tDesc[descLen * fo::Trait::TRAIT_count] = {0};
static char PerkBoxTitle[33];

static const int startFakeID = 256;
static DWORD addPerkMode = 2;

static bool perksReInit = false;
static int perksEnable = 0;
static int traitsEnable = 0;

static fo::PerkInfo perks[fo::Perk::PERK_count];
static fo::TraitInfo traits[fo::Trait::TRAIT_count];

struct PerkInfoExt {
	short id;
	char reserve[6];
	char Name[maxNameLen];
	char Desc[descLen];
	fo::PerkInfo data;
	// extra modificators
	long stat1;
	long stat1Mod;
	long stat2;
	long stat2Mod;

	long skill3;
	long skill3Mod;
	long skill4;
	long skill4Mod;
	long skill5;
	long skill5Mod;
};
static std::vector<PerkInfoExt> extPerks;

#pragma pack(push, 1)
struct FakePerk {
	int Level; // current level (max 100)
	int Image;
	char Name[maxNameLen];
	char Desc[maxDescLen];
	char reserve[506]; // empty block
	int  ownerId;      // 0 = the player, or ID number of party member NPC
	short id; // perk id (use last bytes of the description under the ID value for compatibility)

	FakePerk() {}

	FakePerk(const char* _name, int _level, int _image, const char* _desc, int npcId, short _id = -1) : ownerId(npcId), id(_id), Name {0}, Desc {0}, reserve {0} {
		Level = _level;
		Image = _image;
		strncpy_s(this->Name, _name, _TRUNCATE);
		strncpy_s(this->Desc, _desc, _TRUNCATE);
	}
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

// Returns the index for the found ID
int PerkSearchID(int id) {
	int left = 0,
		right = extPerks.size();
	if (!--right) return (extPerks[right].id == id) ? right : -1;
	while (left <= right) {
		int mid = (left + right) / 2; // middle index of the segment
		int value = extPerks[mid].id;
		if (value == id) return mid;
		if (value > id) {
			right = mid - 1;
		} else {
			left = mid + 1;
		}
	}
	return -1;
}

void __stdcall SetPerkFreq(int i) {
	PerkFreqOverride = i;
}

static DWORD __stdcall LevelUp() {
	DWORD eachLevel = PerkFreqOverride;

	if (!eachLevel) {
		if (Perks::DudeHasTrait(fo::Trait::TRAIT_skilled)) { // Check if the player has the skilled trait
			eachLevel = 4;
		} else {
			eachLevel = 3;
		}
	}

	DWORD level = fo::var::Level_pc; // Get player's level
	if (!((level + 1) % eachLevel)) fo::var::free_perk++; // Increment the number of perks owed
	return level;
}

static __declspec(naked) void LevelUpHack() {
	__asm {
		push ecx;
		call LevelUp;
		mov  edx, eax; // player level
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkBoxTitleHook() {
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

static bool IsOwnedFake(int ownerId) {
	return ((!ownerId && !PartyControl::IsNpcControlled()) || ownerId == fo::var::obj_dude->id);
}

static bool IsNotOwnedFake(int ownerId1, int ownerId2) {
	return (ownerId1 != ownerId2);
}

void Perks::SetSelectablePerk(const char* name, int active, int image, const char* desc, int npcID) {
	if (active < 0) return;
	if (active > 1) active = 1;
	size_t size = fakeSelectablePerks.size();
	if (active == 0) {
		for (size_t i = 0; i < size; i++) {
			if (IsNotOwnedFake(fakeSelectablePerks[i].ownerId, npcID)) continue;
			if (!strcmp(name, fakeSelectablePerks[i].Name)) {
				fakeSelectablePerks.erase(fakeSelectablePerks.begin() + i);
				return;
			}
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			if (IsNotOwnedFake(fakeSelectablePerks[i].ownerId, npcID)) continue;
			if (!strcmp(name, fakeSelectablePerks[i].Name)) {
				fakeSelectablePerks[i].Level = active;
				fakeSelectablePerks[i].Image = image;
				strncpy(fakeSelectablePerks[i].Desc, desc, descLen - 1);
				fakeSelectablePerks[i].Desc[descLen - 1] = 0;
				return;
			}
		}
		if (size == fakeSelectablePerks.capacity()) fakeSelectablePerks.reserve(size + 10);
		fakeSelectablePerks.emplace_back(name, active, image, desc, npcID);
	}
}

void Perks::SetFakePerk(const char* name, int level, int image, const char* desc, int npcID) {
	if (level < 0) return;
	if (level > 100) level = 100;
	size_t size = fakePerks.size();
	if (level == 0) { // remove perk from fakePerks
		for (size_t i = 0; i < size; i++) {
			if (IsNotOwnedFake(fakePerks[i].ownerId, npcID)) continue;
			if (!strcmp(name, fakePerks[i].Name)) {
				fakePerks.erase(fakePerks.begin() + i);
				return;
			}
		}
	} else { // add or change the existing fake perk in fakePerks
		for (size_t i = 0; i < size; i++) {
			if (IsNotOwnedFake(fakePerks[i].ownerId, npcID)) continue;
			if (!strcmp(name, fakePerks[i].Name)) {
				fakePerks[i].Level = level;
				fakePerks[i].Image = image;
				strncpy(fakePerks[i].Desc, desc, descLen - 1);
				fakePerks[i].Desc[descLen - 1] = 0;
				return;
			}
		}
		if (size == fakePerks.capacity()) fakePerks.reserve(size + 10);
		fakePerks.emplace_back(name, level, image, desc, npcID);
	}
}

void Perks::SetFakeTrait(const char* name, int active, int image, const char* desc, int npcID) {
	if (active < 0) return;
	if (active > 1) active = 1;
	size_t size = fakeTraits.size();
	if (active == 0) {
		for (size_t i = 0; i < size; i++) {
			if (IsNotOwnedFake(fakeTraits[i].ownerId, npcID)) continue;
			if (!strcmp(name, fakeTraits[i].Name)) {
				fakeTraits.erase(fakeTraits.begin() + i);
				return;
			}
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			if (IsNotOwnedFake(fakeTraits[i].ownerId, npcID)) continue;
			if (!strcmp(name, fakeTraits[i].Name)) {
				fakeTraits[i].Level = active;
				fakeTraits[i].Image = image;
				strncpy(fakeTraits[i].Desc, desc, descLen - 1);
				fakeTraits[i].Desc[descLen - 1] = 0;
				return;
			}
		}
		if (size == fakeTraits.capacity()) fakeTraits.reserve(size + 5);
		fakeTraits.emplace_back(name, active, image, desc, npcID);
	}
}

static DWORD __stdcall HaveFakePerks() {
	return fakePerks.size();
}

static long __fastcall GetFakePerkLevel(int id) {
	int i = id - fo::Perk::PERK_count;
	return IsOwnedFake(fakePerks[i].ownerId) ? fakePerks[i].Level : 0;
}

static long __fastcall GetFakePerkImage(int id) {
	return fakePerks[id - fo::Perk::PERK_count].Image;
}

static FakePerk* __fastcall GetFakePerk(int id) {
	return &fakePerks[id - fo::Perk::PERK_count];
}

// Get level of taken perk
static DWORD __fastcall GetFakeSelectPerkLevel(int id) {
	if (id < startFakeID) {
		if (!PartyControl::IsNpcControlled()) { // extra perks are not available for controlled NPC
			for (DWORD i = 0; i < fakePerks.size(); i++) {
				if (fakePerks[i].id == id) return fakePerks[i].Level;
			}
		}
		return 0;
	}
	long n = id - startFakeID;
	const char* name = fakeSelectablePerks[n].Name;
	int ownerID = fakeSelectablePerks[n].ownerId;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (ownerID != fakePerks[i].ownerId) continue;
		if (!strcmp(name, fakePerks[i].Name)) {
			return fakePerks[i].Level;
		}
	}
	return 0;
}

static long __fastcall GetFakeSelectPerkImage(int id) {
	if (id < startFakeID) {
		int i = PerkSearchID(id);
		return (i != -1) ? extPerks[i].data.image : i;
	}
	return fakeSelectablePerks[id - startFakeID].Image;
}

static FakePerk* __fastcall GetFakeSelectPerk(int id) {
	if (id < startFakeID) {
		int i = PerkSearchID(id);
		if (i < 0) i = 0; // if id is not found
		return (FakePerk*)&extPerks[i];
	}
	return &fakeSelectablePerks[id - startFakeID];
}

static __declspec(naked) void CheckTraitHack() {
	__asm {
		mov  edx, ds:[FO_VAR_temp_trait];
		cmp  edx, -1;
		jnz  end;
		mov  edx, ds:[FO_VAR_temp_trait2];
end:
		retn;
	}
}

// Print a list of fake traits
static DWORD HandleFakeTraits(int isSelect) {
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (!IsOwnedFake(fakeTraits[i].ownerId)) continue;
		if (fo::func::folder_print_line(fakeTraits[i].Name) && !isSelect) {
			isSelect = 1;
			fo::var::folder_card_fid = fakeTraits[i].Image;
			fo::var::folder_card_title = (DWORD)fakeTraits[i].Name;
			fo::var::folder_card_title2 = 0;
			fo::var::folder_card_desc = (DWORD)fakeTraits[i].Desc;
		}
	}
	return isSelect;
}

static long __fastcall PlayerHasPerk(int &isSelectPtr) {
	isSelectPtr = HandleFakeTraits(isSelectPtr);

	for (int i = 0; i < fo::Perk::PERK_count; i++) {
		if (fo::func::perk_level(fo::var::obj_dude, i)) return 0x43438A; // print perks
	}
	return (!fakePerks.empty())
	       ? 0x43438A  // print perks
	       : 0x434446; // skip print perks
}

static DWORD __fastcall HaveFakeTraits(int &isSelectPtr) {
	return (fakeTraits.empty()) ? PlayerHasPerk(isSelectPtr) : 0x43425B; // print traits
}

static __declspec(naked) void PlayerHasPerkHack() {
	__asm {
		push ecx;            // isSelect
		mov  ecx, esp;       // ptr to isSelect
		call PlayerHasPerk;
		pop  ecx;            // isSelect value from HandleFakeTraits
		jmp  eax;
	}
}

static __declspec(naked) void PlayerHasTraitHook() {
	__asm {
		push ecx;            // isSelect
		mov  ecx, esp;       // ptr to isSelect
		call HaveFakeTraits;
		pop  ecx;            // isSelect value from HandleFakeTraits
		jmp  eax;
	}
}

static __declspec(naked) void GetPerkLevelHook() {
	using namespace fo;
	__asm {
		cmp  edx, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_level_;
fake:
		push ecx;
		mov  ecx, edx;
		call GetFakePerkLevel;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkImageHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_skilldex_fid_;
fake:
		push ecx;
		mov  ecx, eax;
		call GetFakePerkImage;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkNameHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_name_;
fake:
		push ecx;
		mov  ecx, eax;
		call GetFakePerk;
		lea  eax, ds:[eax + 8];
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkDescHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_description_;
fake:
		push ecx;
		mov  ecx, eax;
		call GetFakePerk;
		lea  eax, ds:[eax + 72];
		pop  ecx;
		retn;
	}
}

// Search all available perks for the player to display them in the character screen
static __declspec(naked) void EndPerkLoopHack() {
	static const DWORD EndPerkLoopExit = 0x434446;
	static const DWORD EndPerkLoopCont = 0x4343A5;
	using namespace fo;
	__asm {
		jl   cLoop;           // if ebx < 119
		push ecx;
		call HaveFakePerks;   // return perks count (fake + ext)
		pop  ecx;
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
	size_t count;
	if (!PartyControl::IsNpcControlled()) { // extra perks are not available for controlled NPC
		count = extPerks.size();
		for (size_t i = 0; i < count; i++) {
			if (available >= 119) return available; // exit if the buffer is overfull
			if (fo::func::perk_can_add(fo::var::obj_dude, extPerks[i].id)) data[available++] = extPerks[i].id;
		}
	}
	count = fakeSelectablePerks.size();
	for (size_t i = 0; i < count; i++) {
		if (IsOwnedFake(fakeSelectablePerks[i].ownerId)) {
			if (available >= 119) break;
			// for fake perks, their ID should start from 256
			data[available++] = startFakeID + i;
		}
	}
	return available; // total number of perks available for selection
}

static __declspec(naked) void GetAvailablePerksHook() {
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

static __declspec(naked) void GetPerkSLevelHook() {
	using namespace fo;
	__asm {
		cmp  edx, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_level_;
fake:
		push ecx;
		mov  ecx, edx;
		call GetFakeSelectPerkLevel;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkSImageHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_skilldex_fid_;
fake:
		push ecx;
		push edx;
		mov  ecx, eax;
		call GetFakeSelectPerkImage;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkSNameHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_name_;
fake:
		push ecx;
		push edx;
		mov  ecx, eax;
		call GetFakeSelectPerk;
		lea  eax, ds:[eax + 8]; // Name
		pop  edx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void GetPerkSDescHook() {
	using namespace fo;
	__asm {
		cmp  eax, PERK_count;
		jge  fake;
		jmp  fo::funcoffs::perk_description_;
fake:
		push ecx;
		push edx;
		mov  ecx, eax;
		call GetFakeSelectPerk;
		lea  eax, ds:[eax + 72]; // Desc
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void PerkSkillMod(fo::GameObject* critter, long skill, long mod, long type) {
	if (mod == 0 || skill >= fo::SKILL_count) return;
	if (mod < 0) {
		type = !type;
		mod = -mod;
	}
	if (fo::func::skill_is_tagged(skill)) mod /= 2;
	do {
		if (type) {
			fo::func::skill_inc_point_force(critter, skill);
		} else {
			fo::func::skill_dec_point_force(critter, skill);
		}
	} while (--mod);
}

static void PerkStatMod(fo::GameObject* critter, long stat, long mod, long type) {
	if (mod == 0 || stat > fo::STAT_poison_resist) return;
	if (!type) mod = -mod;
	int value = fo::func::stat_get_bonus(critter, stat);
	fo::func::stat_set_bonus(critter, stat, value + mod);
}

static void ApplyPerkEffect(long index, fo::GameObject* critter, long type) {
	int stat = extPerks[index].stat1;
	if (stat > -1) PerkStatMod(critter, stat, extPerks[index].stat1Mod, type);
	stat = extPerks[index].stat2;
	if (stat > -1) PerkStatMod(critter, stat, extPerks[index].stat2Mod, type);

	int skill = extPerks[index].skill3;
	if (skill > -1) PerkSkillMod(critter, skill, extPerks[index].skill3Mod, type);
	skill = extPerks[index].skill4;
	if (skill > -1) PerkSkillMod(critter, skill, extPerks[index].skill4Mod, type);
	skill = extPerks[index].skill5;
	if (skill > -1) PerkSkillMod(critter, skill, extPerks[index].skill5Mod, type);
}

// Adds the selected perk to the player
static long __stdcall AddFakePerk(DWORD perkID) {
	size_t count;
	bool matched = false;
	if (perkID < startFakeID) { // extra perk can only be added to the player
		count = fakePerks.size();
		for (size_t d = 0; d < count; d++) {
			if (fakePerks[d].id == perkID) {
				RemovePerkID.push_back(d);
				fakePerks[d].Level++;
				matched = true;
				break;
			}
		}
		if (!matched) { // add to fakePerks
			int index = PerkSearchID(perkID);
			if (index < 0) return -1;
			RemovePerkID.push_back(count); // index of the added perk
			fakePerks.emplace_back(extPerks[index].Name, 1, extPerks[index].data.image, extPerks[index].Desc, 0, extPerks[index].id); // id same as perkID
		}
		fo::func::perk_add_effect(fo::var::obj_dude, perkID);
		return 0;
	}
	// behavior for fake perk/trait
	perkID -= startFakeID;
	if (addPerkMode & 1) { // add perk to trait
		count = fakeTraits.size();
		for (size_t d = 0; d < count; d++) {
			if (IsNotOwnedFake(fakeTraits[d].ownerId, fakeSelectablePerks[perkID].ownerId)) continue;
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
			if (IsNotOwnedFake(fakePerks[d].ownerId, fakeSelectablePerks[perkID].ownerId)) continue;
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
static __declspec(naked) void AddPerkHook() {
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

// Checks player statistics to add perk in selection listing
static fo::PerkInfo* __fastcall CanAddPerk(DWORD perkID) {
	int index = PerkSearchID(perkID);
	if (index != -1) {
		int ranks = extPerks[index].data.ranks;
		if (ranks > 0) {
			for (size_t i = 0; i < fakePerks.size(); i++) {
				if (fakePerks[i].id == perkID) {
					if (fakePerks[i].Level >= ranks) return 0; // not available to add
					break;
				}
			}
			return &extPerks[index].data;
		}
	}
	return 0;
}

static __declspec(naked) void perk_can_add_hack() {
	static const DWORD perk_can_add_exit = 0x496A03;
	static const DWORD perk_can_add_check = 0x496872;
	__asm {
		test edx, edx;
		jz   end;
		push edx;
		mov  ecx, edx;
		call CanAddPerk;
		pop  edx;
		test eax, eax;
		jz   end;
		mov  ecx, eax; // ptr to perk_data
		jmp  perk_can_add_check;
end:
		jmp  perk_can_add_exit;
	}
}

static fo::PerkInfo* __fastcall PerkData(DWORD perkID, fo::GameObject* critter, long type) {
	int index = PerkSearchID(perkID);
	if (index != -1) {
		ApplyPerkEffect(index, critter, type); // apply ext. perk to critter
		return &extPerks[index].data;
	}
	return 0;
}

static __declspec(naked) void perk_add_effect_hook() {
	static const DWORD perk_add_effect_exit = 0x496CD9;
	static const DWORD perk_add_effect_continue = 0x496C4A;
	__asm {
		cmp  edx, startFakeID;
		jge  end;
		push ecx;
		push 1;        // add effect
		mov  edx, esi; // critter
		call PerkData; // ecx - perkID
		pop  ecx;
		test eax, eax;
		jz   end;
		mov  edi, eax; // ptr to perk_data
		jmp  perk_add_effect_continue;
end:
		jmp  perk_add_effect_exit;
	}
}

static __declspec(naked) void perk_remove_effect_hook() {
	static const DWORD perk_remove_effect_exit = 0x496D99;
	static const DWORD perk_remove_effect_continue = 0x496D2E;
	__asm {
		cmp  edx, startFakeID;
		jge  end;
		push ecx;
		push 0;        // remove effect
		mov  edx, esi; // critter
		call PerkData; // ecx - perkID
		pop  ecx;
		test eax, eax;
		jz   end;
		mov  edi, eax; // ptr to perk_data
		jmp  perk_remove_effect_continue;
end:
		jmp  perk_remove_effect_exit;
	}
}

static __declspec(naked) void HeaveHoHook() {
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

static void PerkEngineInit() {
	perk::EnginePerkBonusInit();

	// Character screen (list_perks_)
	MakeCall(0x434246, CheckTraitHack, 1);  // fix for certain cases
	HookCall(0x434256, PlayerHasTraitHook); // jz func
	MakeJump(0x43436B, PlayerHasPerkHack);
	HookCall(0x4343AC, GetPerkLevelHook);
	HookCall(0x43440D, GetPerkImageHook);
	HookCall(0x434432, GetPerkDescHook);
	MakeJump(0x434440, EndPerkLoopHack, 1);
	HookCalls(GetPerkNameHook, {0x4343C1, 0x4343DF, 0x43441B});

	// GetPlayerAvailablePerks (ListDPerks_)
	HookCall(0x43D127, GetAvailablePerksHook);

	// ShowPerkBox (perks_dialog_)
	HookCall(0x43C952, AddPerkHook);

	// PerkboxSwitchPerk (RedrwDPrks_)

	// GetPerkS hooks
	HookCalls(GetPerkSLevelHook, {
		0x43D25E, 0x43D275, // ListDPerks_
		0x43C82E, 0x43C85B, // perks_dialog_
		0x43C3F1, 0x43C41E  // RedrwDPrks_
	});
	HookCalls(GetPerkSNameHook, {
		0x43D17D,           // ListDPerks_
		0x43C8A6, 0x43C8EF, // perks_dialog_
		0x43C469, 0x43C4B2  // RedrwDPrks_
	});
	HookCalls(GetPerkSDescHook, {
		0x43C888, 0x43C8D1, // perks_dialog_
		0x43C44B, 0x43C494  // RedrwDPrks_
	});
	HookCalls(GetPerkSImageHook, {
		0x43C90F,           // perks_dialog_
		0x43C4D2            // RedrwDPrks_
	});

	// perk_owed hooks
	MakeCall(0x4AFB2F, LevelUpHack, 1); // replaces 'mov edx, ds:[PlayerLevel]'
	SafeWrite8(0x43C2EC, CodeType::JumpShort); // skip the block of code which checks if the player has gained a perk (now handled in level up code)
}

static void PerkSetup() {
	if (!perksReInit) {
		// _perk_data
		SafeWriteBatch<DWORD>((DWORD)perks, {0x496669, 0x496837, 0x496BAD, 0x496C41, 0x496D25});
		SafeWriteBatch<DWORD>((DWORD)&perks[0].description, {0x496696, 0x496BD1});
		SafeWrite32(0x496BF5, (DWORD)&perks[0].image);
		SafeWrite32(0x496AD4, (DWORD)&perks[0].ranks);
	}
	std::memcpy(perks, fo::var::perk_data, sizeof(fo::PerkInfo) * fo::Perk::PERK_count); // copy vanilla data

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
		if (perksReInit) {
			perksReInit = false;
			return;
		}
		// adding extra perks with IDs from 119 to 255
		extPerks.reserve(startFakeID - fo::Perk::PERK_count);
		extPerks.resize(1);
		int n = 0;
		for (int id = fo::Perk::PERK_count; id < startFakeID; id++) {
			_itoa(id, num, 10);
			int ranks = IniReader::GetInt(num, "Ranks", -1, perksFile);
			if (ranks == -1) continue;
			extPerks[n].data.ranks = ranks;
			extPerks[n].data.image = IniReader::GetInt(num, "Image", -1, perksFile);
			extPerks[n].data.levelMin = IniReader::GetInt(num, "Level", -1, perksFile);
			extPerks[n].data.stat = IniReader::GetInt(num, "Stat", -1, perksFile);
			extPerks[n].data.statMod = IniReader::GetInt(num, "StatMag", 0, perksFile);
			extPerks[n].data.skill1 = IniReader::GetInt(num, "Skill1", -1, perksFile);
			extPerks[n].data.skill1Min = IniReader::GetInt(num, "Skill1Mag", 0, perksFile);
			extPerks[n].data.skillOperator = IniReader::GetInt(num, "Type", 0, perksFile);
			extPerks[n].data.skill2 = IniReader::GetInt(num, "Skill2", -1, perksFile);
			extPerks[n].data.skill2Min = IniReader::GetInt(num, "Skill2Mag", 0, perksFile);
			extPerks[n].data.strengthMin = IniReader::GetInt(num, "STR", 0, perksFile);
			extPerks[n].data.perceptionMin = IniReader::GetInt(num, "PER", 0, perksFile);
			extPerks[n].data.enduranceMin = IniReader::GetInt(num, "END", 0, perksFile);
			extPerks[n].data.charismaMin = IniReader::GetInt(num, "CHR", 0, perksFile);
			extPerks[n].data.intelligenceMin = IniReader::GetInt(num, "INT", 0, perksFile);
			extPerks[n].data.agilityMin = IniReader::GetInt(num, "AGL", 0, perksFile);
			extPerks[n].data.luckMin = IniReader::GetInt(num, "LCK", 0, perksFile);

			IniReader::GetString(num, "Name", "Error", extPerks[n].Name, maxNameLen - 1, perksFile);
			extPerks[n].data.name = extPerks[n].Name;
			IniReader::GetString(num, "Desc", "Error", extPerks[n].Desc, descLen - 1, perksFile);
			extPerks[n].data.description = extPerks[n].Desc;

			extPerks[n].stat1 = IniReader::GetInt(num, "Stat1", -1, perksFile);
			extPerks[n].stat1Mod = IniReader::GetInt(num, "Stat1Mag", 0, perksFile);
			extPerks[n].stat2 = IniReader::GetInt(num, "Stat2", -1, perksFile);
			extPerks[n].stat2Mod = IniReader::GetInt(num, "Stat2Mag", 0, perksFile);
			extPerks[n].skill3 = IniReader::GetInt(num, "Skill3", -1, perksFile);
			extPerks[n].skill3Mod = IniReader::GetInt(num, "Skill3Mod", 0, perksFile);
			extPerks[n].skill4 = IniReader::GetInt(num, "Skill4", -1, perksFile);
			extPerks[n].skill4Mod = IniReader::GetInt(num, "Skill4Mod", 0, perksFile);
			extPerks[n].skill5 = IniReader::GetInt(num, "Skill5", -1, perksFile);
			extPerks[n].skill5Mod = IniReader::GetInt(num, "Skill5Mod", 0, perksFile);
			extPerks[n].id = id;
			++n;
			extPerks.resize(n + 1); // add next 'empty' perk
		}
		extPerks.pop_back();
		extPerks.shrink_to_fit();

		if (!extPerks.empty()) {
			MakeJump(0x496823, perk_can_add_hack);
			HookCall(0x496C2D, perk_add_effect_hook);    // jge func
			HookCall(0x496D11, perk_remove_effect_hook); // jge func
			dlog_f("Added extra %d perks\n", DL_INIT, n);
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
	return traitStatBonuses[statID * fo::Trait::TRAIT_count + fo::var::pc_trait[traitIndex]];
}

DWORD Perks::GetTraitSkillBonus(int skillID, int traitIndex) {
	return traitSkillBonuses[skillID * fo::Trait::TRAIT_count + fo::var::pc_trait[traitIndex]];
}

static __declspec(naked) void BlockedTrait() {
	__asm {
		xor  eax, eax;
		retn;
	}
}

static void PerkAndTraitSetup() {
	PerkSetup();

	if (!traitsEnable) return;

	std::memcpy(traits, fo::var::trait_data, sizeof(fo::TraitInfo) * fo::Trait::TRAIT_count);

	// _trait_data
	SafeWriteBatch<DWORD>((DWORD)traits, {0x4B3A81, 0x4B3B80});
	SafeWriteBatch<DWORD>((DWORD)&traits[0].description, {0x4B3AAE, 0x4B3BA0});
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

static __declspec(naked) void perks_dialog_hook() {
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
static __declspec(naked) void item_w_mp_cost_hook() {
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
static __declspec(naked) void item_w_called_shot_hack() {
	static const DWORD FastShotTraitFix_End = 0x478E7F;
	using namespace fo;
	__asm {
		mov  edx, ecx;                     // argument for item_hit_with_: hit_mode
		mov  eax, ebx;                     // argument for item_hit_with_: pointer to source_obj (always dude_obj due to code path)
		call fo::funcoffs::item_hit_with_; // get pointer to weapon
		mov  edx, ecx;
		call fo::funcoffs::item_w_subtype_;
		cmp  eax, THROWING;                // is weapon type RANGED or THROWING?
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
		dlogr("Applying Fast Shot trait patch (Haenlomal's tweak).", DL_INIT);
		MakeJump(0x478E79, item_w_called_shot_hack);
		goto fix;
	case 2:
		dlogr("Applying Fast Shot trait patch (Alternative behavior).", DL_INIT);
		/* Implemented in sfall item_w_mp_cost function */
		//SafeWrite16(0x478C9F, 0x9090); // item_w_mp_cost_
		//HookCalls((void*)0x478C7D, {0x478BB8, 0x478BC7, 0x478BD6, 0x478BEA, 0x478BF9, 0x478C08, 0x478C2F}); // jmp 0x478C7D
		break;
	case 3:
		dlogr("Applying Fast Shot trait patch (Fallout 1 behavior).", DL_INIT);
		/* Implemented in sfall item_w_mp_cost function */
		//HookCall(0x478C97, (void*)fo::funcoffs::item_hit_with_);
		//SafeWrite16(0x478C9E, CodeType::JumpZ << 8); // ignore all unarmed attacks (cmp eax, 0; jz)
		break;
	default:
		dlogr("Applying Fast Shot trait fix.", DL_INIT);
	fix:
		//HookCall(0x478C97, item_w_mp_cost_hook); - Fix implemented in sfall item_w_mp_cost function
		break;
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

void PerksReset() {
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
	if (count > 2) fakeTraits.reserve(count);
	for (DWORD i = 0; i < count; i++) {
		FakePerk fp;
		ReadFile(file, &fp, sizeof(FakePerk), &size, 0);
		fakeTraits.push_back(fp);
	}
	ReadFile(file, &count, 4, &size, 0);
	if (size != 4) return false;
	if (count > 2) fakePerks.reserve(count);
	for (DWORD i = 0; i < count; i++) {
		FakePerk fp;
		ReadFile(file, &fp, sizeof(FakePerk), &size, 0);
		if (fp.id > 0) { // update name/desc for non-fake perks
			int index = PerkSearchID(fp.id);
			if (index != -1) {
				strncpy_s(fp.Name, extPerks[index].Name, _TRUNCATE);
				strncpy_s(fp.Desc, extPerks[index].Desc, _TRUNCATE);
			}
		}
		fakePerks.push_back(fp);
	}
	ReadFile(file, &count, 4, &size, 0);
	if (count > 2) fakeSelectablePerks.reserve(count);
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

DWORD Perks::HasFakePerk(const char* name, long perkId) {
	if ((perkId < fo::Perk::PERK_count && name[0] == 0) || (perkId && PartyControl::IsNpcControlled())) return 0;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (perkId) {
			if (fakePerks[i].id == perkId) return fakePerks[i].Level; // current perk level
		} else if (IsOwnedFake(fakePerks[i].ownerId) && !strcmp(name, fakePerks[i].Name)) {
			return fakePerks[i].Level;
		}
	}
	return 0;
}

DWORD Perks::HasFakeTrait(const char* name) {
	if (name[0] == 0) return 0;
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (IsOwnedFake(fakeTraits[i].ownerId) && !strcmp(name, fakeTraits[i].Name)) {
			return 1;
		}
	}
	return 0;
}

DWORD Perks::HasFakePerkOwner(const char* name, long objId) {
	if (name[0] == 0) return 0;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (IsNotOwnedFake(fakePerks[i].ownerId, objId)) continue;
		if (!strcmp(name, fakePerks[i].Name)) {
			return fakePerks[i].Level;
		}
	}
	return 0;
}

DWORD Perks::HasFakeTraitOwner(const char* name, long objId) {
	if (name[0] == 0) return 0;
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (IsNotOwnedFake(fakeTraits[i].ownerId, objId)) continue;
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
	LoadGameHook::OnGameReset() += PerksReset;

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
		if (GetFileAttributesA(perksFile) == INVALID_FILE_ATTRIBUTES) return;

		perksEnable = IniReader::GetInt("Perks", "Enable", 1, perksFile);
		traitsEnable = IniReader::GetInt("Traits", "Enable", 1, perksFile);

		// Engine perks settings
		perk::ReadPerksBonuses(perksFile);
	}
}

}
