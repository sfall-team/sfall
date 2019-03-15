/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "Perks.h"

namespace sfall
{
using namespace fo;

constexpr int maxNameLen = 64;   // don't change size
constexpr int maxDescLen = 512;  // don't change size
static const int descLen = 256;  // maximum text length for interface

static char perksFile[MAX_PATH] = {0};

static char Name[maxNameLen * PERK_count] = {0};
static char Desc[descLen * PERK_count] = {0};
static char tName[maxNameLen * TRAIT_count] = {0};
static char tDesc[descLen * TRAIT_count] = {0};
static char PerkBoxTitle[33];

#define check_trait(id) !disableTraits[id] && (var::pc_trait[0] == id || var::pc_trait[1] == id)

static const int startFakeID = 256;
static DWORD addPerkMode = 2;
static bool perksReInit = false;

static PerkInfo perks[PERK_count];
static TraitInfo traits[TRAIT_count];

struct PerkInfoExt {
	short id;
	char reserve[6];
	char Name[maxNameLen];
	char Desc[descLen];
	PerkInfo data;
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

struct FakePerk {
	int Level; // current level (max 99)
	int Image;
	char Name[maxNameLen];
	char Desc[maxDescLen];
	char reserve[510]; // empty block
	short id; // use last bytes of the description under the ID value for compatibility

	FakePerk() {}

	FakePerk(char* _name, int _level, int _image, char* _desc) : id(-1), Name {0}, Desc {0}, reserve {0} {
		Level = _level;
		Image = _image;
		strncpy_s(this->Name, _name, _TRUNCATE);
		strncpy_s(this->Desc, _desc, _TRUNCATE);
	}
};

std::vector<FakePerk> fakeTraits;
std::vector<FakePerk> fakePerks;
std::vector<FakePerk> fakeSelectablePerks; // available perks for selection in the perk selection list

static std::list<int> RemoveTraitID;
static std::list<int> RemovePerkID;
static std::list<int> RemoveSelectableID;

static DWORD TraitSkillBonuses[TRAIT_count * 18] = {0};
static DWORD TraitStatBonuses[TRAIT_count * (STAT_max_derived + 1)] = {0};

static bool disableTraits[TRAIT_count];
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

void _stdcall SetPerkFreq(int i) {
	PerkFreqOverride = i;
}

static bool IsTraitDisabled(int id) {
	return disableTraits[id];
}

static long _stdcall LevelUp() {
	int eachLevel = PerkFreqOverride;

	if (!eachLevel) {
		if (!IsTraitDisabled(TRAIT_skilled) && fo::func::trait_level(TRAIT_skilled)) { // Check if the player has the skilled trait
			eachLevel = 4;
		} else {
			eachLevel = 3;
		}
	}

	int level = fo::var::Level_; // Get players level
	if (!((level + 1) % eachLevel)) fo::var::free_perk++; // Increment the number of perks owed
	return level;
}

static void __declspec(naked) LevelUpHack() {
	__asm {
		push ecx;
		call LevelUp;
		mov  edx, eax; // player level
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) GetPerkBoxTitleHook() {
	__asm {
		lea  eax, PerkBoxTitle;
		retn;
	}
}

void _stdcall IgnoreDefaultPerks() {
	IgnoringDefaultPerks = 1;
}

void _stdcall RestoreDefaultPerks() {
	IgnoringDefaultPerks = 0;
}

void _stdcall SetPerkboxTitle(char* name) {
	if (name[0] == '\0') {
		PerkBoxTitle[0] = 0;
		SafeWrite32(0x43C77D, 0x488CB);
	} else {
		strncpy_s(PerkBoxTitle, name, _TRUNCATE);
		HookCall(0x43C77C, GetPerkBoxTitleHook);
	}
}

void _stdcall SetSelectablePerk(char* name, int active, int image, char* desc) {
	if (active < 0 || active > 1) return;
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
		if (size == fakeSelectablePerks.capacity()) fakeSelectablePerks.reserve(size + 10);
		fakeSelectablePerks.push_back(FakePerk(name, active, image, desc));
	}
}

void _stdcall SetFakePerk(char* name, int level, int image, char* desc) {
	if (level < 0 || level > 100) return;
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
		if (size == fakePerks.capacity()) fakePerks.reserve(size + 10);
		fakePerks.push_back(FakePerk(name, level, image, desc));
	}
}

void _stdcall SetFakeTrait(char* name, int active, int image, char* desc) {
	if (active < 0 || active > 1) return;
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
		if (size == fakeTraits.capacity()) fakeTraits.reserve(size + 5);
		fakeTraits.push_back(FakePerk(name, active, image, desc));
	}
}

static DWORD _stdcall HaveFakePerks() {
	return fakePerks.size();
}

static long __fastcall GetFakePerkLevel(int id) {
	return fakePerks[id - PERK_count].Level;
}

static long __fastcall GetFakePerkImage(int id) {
	return fakePerks[id - PERK_count].Image;
}

static FakePerk* __fastcall GetFakePerk(int id) {
	return &fakePerks[id - PERK_count];
}

static DWORD __fastcall GetFakeSelectPerkLevel(int id) {
	if (id < startFakeID) {
		for (DWORD i = 0; i < fakePerks.size(); i++) {
			if (fakePerks[i].id == id) return fakePerks[i].Level;
		}
		return 0;
	}
	char* name = fakeSelectablePerks[id - startFakeID].Name;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (!strcmp(name, fakePerks[i].Name)) return fakePerks[i].Level;
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

static DWORD HandleFakeTraits(int isSelect) {
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (fo::func::folder_print_line(fakeTraits[i].Name) && !isSelect) {
			isSelect = 1;
			var::folder_card_fid = fakeTraits[i].Image;
			var::folder_card_title = (DWORD)fakeTraits[i].Name;
			var::folder_card_title2 = 0;
			var::folder_card_desc = (DWORD)fakeTraits[i].Desc;
		}
	}
	return isSelect;
}

static long __fastcall PlayerHasPerk(int* isSelectPtr) {
	*isSelectPtr = HandleFakeTraits(*isSelectPtr);

	for	(int i = 0; i < PERK_count; i++) {
		if (fo::func::perk_level(fo::var::obj_dude, i)) return 0x43438A; // print perks
	}
	return (!fakePerks.empty()) ? 0x43438A : 0x434446; // skip print perks
}

static DWORD __fastcall HaveFakeTraits(int* isSelectPtr) {
	return (fakeTraits.empty()) ? PlayerHasPerk(isSelectPtr) : 0x43425B;
}

static void __declspec(naked) PlayerHasPerkHack() {
	__asm {
		push ecx;            // isSelect
		mov  ecx, esp;       // ptr to isSelect
		call PlayerHasPerk;
		pop  ecx;            // value from HandleFakeTraits
		jmp  eax;
	}
}

static void __declspec(naked) PlayerHasTraitHook() {
	__asm {
		push ecx;            // isSelect
		mov  ecx, esp;       // ptr to isSelect
		call HaveFakeTraits;
		pop  ecx;            // value from HandleFakeTraits
		jmp  eax;
	}
}

static void __declspec(naked) GetPerkLevelHook() {
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

static void __declspec(naked) GetPerkImageHook() {
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

static void __declspec(naked) GetPerkNameHook() {
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

static void __declspec(naked) GetPerkDescHook() {
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
static const DWORD EndPerkLoopExit = 0x434446;
static const DWORD EndPerkLoopCont = 0x4343A5;
static void __declspec(naked) EndPerkLoopHack() {
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

// Build a table of perks ID numbers available for selection
// data buffer has limited size for 119 perks
static DWORD _stdcall HandleExtraSelectablePerks(DWORD available, DWORD* data) {
	size_t count = extPerks.size();
	for (size_t i = 0; i < count; i++) {
		if (available >= 119) break; // exit if the buffer is overfull
		if (fo::func::perk_can_add(fo::var::obj_dude, extPerks[i].id)) data[available++] = extPerks[i].id;
	}
	count = fakeSelectablePerks.size();
	for (size_t i = 0; i < count; i++) {
		if (available >= 119) break;
		// for fake perks, their ID should start from 256
		data[available++] = startFakeID + i; //*(WORD*)(_name_sort_list + (offset+i)*8)=(WORD)(PERK_count+i);
	}
	return available; // total number of perks available for selection
}

static void __declspec(naked) GetAvailablePerksHook() {
	__asm {
		push ecx;
		push edx; // arg data
		mov  ecx, IgnoringDefaultPerks;
		test ecx, ecx;
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

static void __declspec(naked) GetPerkSImageHook() {
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

static void __declspec(naked) GetPerkSNameHook() {
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

static void __declspec(naked) GetPerkSDescHook() {
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
static void _stdcall AddFakePerk(DWORD perkID) {
	size_t count;
	bool matched = false;
	if (perkID < startFakeID) {
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
			RemovePerkID.push_back(count);    // index of the added perk
			int index = PerkSearchID(perkID);
			FakePerk perk(extPerks[index].Name, 1, extPerks[index].data.image, extPerks[index].Desc);
			perk.id	= extPerks[index].id;     // same as perkID
			fakePerks.emplace_back(perk);
		}
		fo::func::perk_add_effect(fo::var::obj_dude, perkID);
		return;
	}
	// behavior for fake perk/trait
	perkID -= startFakeID;
	if (addPerkMode & 1) { // add perk to trait
		count = fakeTraits.size();
		for (size_t d = 0; d < count; d++) {
			if (!strcmp(fakeTraits[d].Name, fakeSelectablePerks[perkID].Name)) {
				matched = true;
				break;
			}
		}
		if (!matched) {
			RemoveTraitID.push_back(count); // index of the added trait
			fakeTraits.push_back(fakeSelectablePerks[perkID]);
		}
	}
	if (addPerkMode & 2) { // default mode
		matched = false;
		count = fakeTraits.size();
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
		RemoveSelectableID.push_back(perkID);
		//fakeSelectablePerks.remove_at(perkID);
	}
}

// Adds perk from selection window to player
static void __declspec(naked) AddPerkHook() {
	__asm {
		cmp  edx, PERK_count;
		jl   normalPerk;
		push ecx;
		push edx;
		call AddFakePerk;
		pop  ecx;
		xor  eax, eax;
		retn;
normalPerk:
		push edx;
		call fo::funcoffs::perk_add_;
		pop  edx;
		test eax, eax;
		jnz  end;
		// fix gain perks
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
static PerkInfo* __fastcall CanAddPerk(DWORD perkID) {
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

static const DWORD perk_can_add_exit = 0x496A03;
static const DWORD perk_can_add_check = 0x496872;
static void __declspec(naked) perk_can_add_hack() {
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

static PerkInfo* __fastcall PerkData(DWORD perkID, fo::GameObject* critter, long type) {
	int index = PerkSearchID(perkID);
	if (index != -1) {
		ApplyPerkEffect(index, critter, type); // apply ext. perk to critter
		return &extPerks[index].data;
	}
	return 0;
}

static const DWORD perk_add_effect_exit = 0x496CD9;
static const DWORD perk_add_effect_continue = 0x496C4A;
static void __declspec(naked) perk_add_effect_hook() {
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

static const DWORD perk_remove_effect_exit = 0x496D99;
static const DWORD perk_remove_effect_continue = 0x496D2E;
static void __declspec(naked) perk_remove_effect_hook() {
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

static void __declspec(naked) HeaveHoHook() {
	__asm {
		xor  edx, edx;
		mov  eax, ecx;
		call fo::funcoffs::stat_level_;
		lea  ebx, [0 + eax * 4];
		sub  ebx, eax;
		cmp  ebx, esi;      // ebx = dist (3*ST), esi = max dist weapon
		jle  lower;         // jump if dist <= max
		mov  ebx, esi;      // dist = max
lower:
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

static bool perkHeaveHoModFix = false;
void _stdcall ApplyHeaveHoFix() { // not really a fix
	MakeJump(0x478AC4, HeaveHoHook);
	perks[PERK_heave_ho].strengthMin = 0;
	perkHeaveHoModFix = true;
}

static void PerkEngineInit() {
	// Character screen (list_perks_)
	HookCall(0x434256, PlayerHasTraitHook); // jz func
	MakeJump(0x43436B, PlayerHasPerkHack);
	HookCall(0x4343AC, GetPerkLevelHook);
	HookCall(0x43440D, GetPerkImageHook);
	HookCall(0x434432, GetPerkDescHook);
	MakeJump(0x434440, EndPerkLoopHack, 1);
	HookCalls(GetPerkNameHook, {0x4343C1, 0x4343DF, 0x43441B});

	// GetPlayerAvailablePerks (ListDPerks_)
	HookCall(0x43D127, GetAvailablePerksHook);
	HookCall(0x43D17D, GetPerkSNameHook);
	HookCalls(GetPerkSLevelHook, {0x43D25E, 0x43D275});

	// ShowPerkBox (perks_dialog_)
	HookCalls(GetPerkSLevelHook, {0x43C82E, 0x43C85B});
	HookCalls(GetPerkSDescHook, {0x43C888, 0x43C8D1});
	HookCalls(GetPerkSNameHook, {0x43C8A6, 0x43C8EF});
	HookCall(0x43C90F, GetPerkSImageHook);
	HookCall(0x43C952, AddPerkHook);

	// PerkboxSwitchPerk (RedrwDPrks_)
	HookCalls(GetPerkSLevelHook, {0x43C3F1, 0x43C41E});
	HookCalls(GetPerkSDescHook, {0x43C44B, 0x43C494});
	HookCalls(GetPerkSNameHook, {0x43C469, 0x43C4B2});
	HookCall(0x43C4D2, GetPerkSImageHook);

	// perk_owed hooks
	MakeCall(0x4AFB2F, LevelUpHack, 1); // replaces 'mov edx, ds:[PlayerLevel]'
	SafeWrite8(0x43C2EC, 0xEB); // skip the block of code which checks if the player has gained a perk (now handled in level up code)

	// Disable losing unused perks
	SafeWrite16(0x43C369, 0x0DFE); // dec  byte ptr ds:_free_perk
	SafeWrite8(0x43C370, 0xB1);    // jmp  0x43C322
}

static void PerkSetup() {
	memcpy(perks, var::perk_data, sizeof(PerkInfo) * PERK_count); // copy vanilla data

	if (!perksReInit) {
		// _perk_data
		SafeWriteBatch<DWORD>((DWORD)perks, {0x496669, 0x496837, 0x496BAD, 0x496C41, 0x496D25});
		SafeWrite32(0x496696, (DWORD)perks + 4);
		SafeWrite32(0x496BD1, (DWORD)perks + 4);
		SafeWrite32(0x496BF5, (DWORD)perks + 8);
		SafeWrite32(0x496AD4, (DWORD)perks + 12);
	}
	if (perksFile[0] != '\0') {
		char num[4];
		for (int i = 0; i < PERK_count; i++) {
			_itoa_s(i, num, 10);
			if (GetPrivateProfileString(num, "Name", "", &Name[i * maxNameLen], maxNameLen - 1, perksFile)) {
				perks[i].name = &Name[i * maxNameLen];
			}
			if (GetPrivateProfileString(num, "Desc", "", &Desc[i * descLen], descLen - 1, perksFile)) {
				perks[i].description = &Desc[i * descLen];
			}
			int value;
			value = GetPrivateProfileInt(num, "Image", -99999, perksFile);
			if (value != -99999) perks[i].image = value;
			value = GetPrivateProfileInt(num, "Ranks", -99999, perksFile);
			if (value != -99999) perks[i].ranks = value;
			value = GetPrivateProfileInt(num, "Level", -99999, perksFile);
			if (value != -99999) perks[i].levelMin = value;
			value = GetPrivateProfileInt(num, "Stat", -99999, perksFile);
			if (value != -99999) perks[i].stat = value;
			value = GetPrivateProfileInt(num, "StatMag", -99999, perksFile);
			if (value != -99999) perks[i].statMod = value;
			value = GetPrivateProfileInt(num, "Skill1", -99999, perksFile);
			if (value != -99999) perks[i].skill1 = value;
			value = GetPrivateProfileInt(num, "Skill1Mag", -99999, perksFile);
			if (value != -99999) perks[i].skill1Min = value;
			value = GetPrivateProfileInt(num, "Type", -99999, perksFile);
			if (value != -99999) perks[i].skillOperator = value;
			value = GetPrivateProfileInt(num, "Skill2", -99999, perksFile);
			if (value != -99999) perks[i].skill2 = value;
			value = GetPrivateProfileInt(num, "Skill2Mag", -99999, perksFile);
			if (value != -99999) perks[i].skill2Min = value;
			value = GetPrivateProfileInt(num, "STR", -99999, perksFile);
			if (value != -99999) perks[i].strengthMin = value;
			value = GetPrivateProfileInt(num, "PER", -99999, perksFile);
			if (value != -99999) perks[i].perceptionMin = value;
			value = GetPrivateProfileInt(num, "END", -99999, perksFile);
			if (value != -99999) perks[i].enduranceMin = value;
			value = GetPrivateProfileInt(num, "CHR", -99999, perksFile);
			if (value != -99999) perks[i].charismaMin = value;
			value = GetPrivateProfileInt(num, "INT", -99999, perksFile);
			if (value != -99999) perks[i].intelligenceMin = value;
			value = GetPrivateProfileInt(num, "AGL", -99999, perksFile);
			if (value != -99999) perks[i].agilityMin = value;
			value = GetPrivateProfileInt(num, "LCK", -99999, perksFile);
			if (value != -99999) perks[i].luckMin = value;
		}
		if (perksReInit) {
			perksReInit = false;
			return;
		}
		// adding extra perks with IDs from 119 to 255
		extPerks.reserve(startFakeID - PERK_count);
		extPerks.resize(1);
		int n = 0;
		for (int id = PERK_count; id < startFakeID; id++) {
			_itoa(id, num, 10);
			int ranks = GetPrivateProfileInt(num, "Ranks", -1, perksFile);
			if (ranks == -1) continue;
			extPerks[n].data.ranks = ranks;
			extPerks[n].data.image = GetPrivateProfileInt(num, "Image", -1, perksFile);
			extPerks[n].data.levelMin = GetPrivateProfileInt(num, "Level", -1, perksFile);
			extPerks[n].data.stat = GetPrivateProfileInt(num, "Stat", -1, perksFile);
			extPerks[n].data.statMod = GetPrivateProfileInt(num, "StatMag", 0, perksFile);
			extPerks[n].data.skill1 = GetPrivateProfileInt(num, "Skill1", -1, perksFile);
			extPerks[n].data.skill1Min = GetPrivateProfileInt(num, "Skill1Mag", 0, perksFile);
			extPerks[n].data.skillOperator = GetPrivateProfileInt(num, "Type", 0, perksFile);
			extPerks[n].data.skill2 = GetPrivateProfileInt(num, "Skill2", -1, perksFile);
			extPerks[n].data.skill2Min = GetPrivateProfileInt(num, "Skill2Mag", 0, perksFile);
			extPerks[n].data.strengthMin = GetPrivateProfileInt(num, "STR", 0, perksFile);
			extPerks[n].data.perceptionMin = GetPrivateProfileInt(num, "PER", 0, perksFile);
			extPerks[n].data.enduranceMin = GetPrivateProfileInt(num, "END", 0, perksFile);
			extPerks[n].data.charismaMin = GetPrivateProfileInt(num, "CHR", 0, perksFile);
			extPerks[n].data.intelligenceMin = GetPrivateProfileInt(num, "INT", 0, perksFile);
			extPerks[n].data.agilityMin = GetPrivateProfileInt(num, "AGL", 0, perksFile);
			extPerks[n].data.luckMin = GetPrivateProfileInt(num, "LCK", 0, perksFile);

			GetPrivateProfileString(num, "Name", "Error", extPerks[n].Name, maxNameLen - 1, perksFile);
			extPerks[n].data.name = extPerks[n].Name;
			GetPrivateProfileString(num, "Desc", "Error", extPerks[n].Desc, descLen - 1, perksFile);
			extPerks[n].data.description = extPerks[n].Desc;

			extPerks[n].stat1 = GetPrivateProfileInt(num, "Stat1", -1, perksFile);
			extPerks[n].stat1Mod = GetPrivateProfileInt(num, "Stat1Mag", 0, perksFile);
			extPerks[n].stat2 = GetPrivateProfileInt(num, "Stat2", -1, perksFile);
			extPerks[n].stat2Mod = GetPrivateProfileInt(num, "Stat2Mag", 0, perksFile);
			extPerks[n].skill3 = GetPrivateProfileInt(num, "Skill3", -1, perksFile);
			extPerks[n].skill3Mod = GetPrivateProfileInt(num, "Skill3Mod", 0, perksFile);
			extPerks[n].skill4 = GetPrivateProfileInt(num, "Skill4", -1, perksFile);
			extPerks[n].skill4Mod = GetPrivateProfileInt(num, "Skill4Mod", 0, perksFile);
			extPerks[n].skill5 = GetPrivateProfileInt(num, "Skill5", -1, perksFile);
			extPerks[n].skill5Mod = GetPrivateProfileInt(num, "Skill5Mod", 0, perksFile);
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

static __declspec(naked) void PerkInitWrapper() {
	__asm {
		call fo::funcoffs::perk_init_;
		push edx;
		push ecx;
		call PerkSetup;
		pop  ecx;
		pop  edx;
		retn;
	}
}

/////////////////////////// TRAIT FUNCTIONS ///////////////////////////////////

static int stat_get_base_direct(DWORD statID) {
	return fo::func::stat_get_base_direct(fo::var::obj_dude, statID);
}

static int _stdcall trait_adjust_stat_override(DWORD statID) {
	if (statID > STAT_max_derived) return 0;

	int result = 0;
	if (var::pc_trait[0] != -1) result += TraitStatBonuses[statID * TRAIT_count + var::pc_trait[0]];
	if (var::pc_trait[1] != -1) result += TraitStatBonuses[statID * TRAIT_count + var::pc_trait[1]];

	switch (statID) {
	case STAT_st:
		if (check_trait(TRAIT_gifted)) result++;
		if (check_trait(TRAIT_bruiser)) result += 2;
		break;
	case STAT_pe:
		if (check_trait(TRAIT_gifted)) result++;
		break;
	case STAT_en:
		if (check_trait(TRAIT_gifted)) result++;
		break;
	case STAT_ch:
		if (check_trait(TRAIT_gifted)) result++;
		break;
	case STAT_iq:
		if (check_trait(TRAIT_gifted)) result++;
		break;
	case STAT_ag:
		if (check_trait(TRAIT_gifted)) result++;
		if (check_trait(TRAIT_small_frame)) result++;
		break;
	case STAT_lu:
		if (check_trait(TRAIT_gifted)) result++;
		break;
	case STAT_max_move_points:
		if (check_trait(TRAIT_bruiser)) result -= 2;
		break;
	case STAT_ac:
		if (check_trait(TRAIT_kamikaze)) return -stat_get_base_direct(STAT_ac);
		break;
	case STAT_melee_dmg:
		if (check_trait(TRAIT_heavy_handed)) result += 4;
		break;
	case STAT_carry_amt:
		if (check_trait(TRAIT_small_frame)) {
			int str = stat_get_base_direct(STAT_st);
			result -= str * 10;
		}
		break;
	case STAT_sequence:
		if (check_trait(TRAIT_kamikaze)) result += 5;
		break;
	case STAT_heal_rate:
		if (check_trait(TRAIT_fast_metabolism)) result += 2;
		break;
	case STAT_crit_chance:
		if (check_trait(TRAIT_finesse)) result += 10;
		break;
	case STAT_better_crit:
		if (check_trait(TRAIT_heavy_handed)) result -= 30;
		break;
	case STAT_rad_resist:
		if (check_trait(TRAIT_fast_metabolism)) return -stat_get_base_direct(STAT_rad_resist);
		break;
	case STAT_poison_resist:
		if (check_trait(TRAIT_fast_metabolism)) return -stat_get_base_direct(STAT_poison_resist);
		break;
	}
	return result;
}

static void __declspec(naked) TraitAdjustStatHack() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call trait_adjust_stat_override;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static int _stdcall trait_adjust_skill_override(DWORD skillID) {
	int result = 0;
	if (var::pc_trait[0] != -1) {
		result += TraitSkillBonuses[skillID * TRAIT_count + var::pc_trait[0]];
	}
	if (var::pc_trait[1] != -1) {
		result += TraitSkillBonuses[skillID * TRAIT_count + var::pc_trait[1]];
	}
	if (check_trait(TRAIT_gifted)) {
		result -= 10;
	}
	if (check_trait(TRAIT_good_natured)) {
		if (skillID <= SKILL_THROWING) {
			result -= 10;
		} else if (skillID == SKILL_FIRST_AID || skillID == SKILL_DOCTOR || skillID == SKILL_CONVERSANT || skillID == SKILL_BARTER) {
			result += 15;
		}
	}
	return result;
}

static void __declspec(naked) TraitAdjustSkillHack() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call trait_adjust_skill_override;
		pop  ecx;
		pop  edx;
		retn;
	}
}

static void __declspec(naked) BlockedTrait() {
	__asm {
		xor  eax, eax;
		retn;
	}
}

static void TraitSetup() {
	// Replace functions
	MakeJump(0x4B3C7C, TraitAdjustStatHack);  // trait_adjust_stat_
	MakeJump(0x4B40FC, TraitAdjustSkillHack); // trait_adjust_skill_

	memcpy(traits, var::trait_data, sizeof(TraitInfo) * TRAIT_count);

	// _trait_data
	SafeWrite32(0x4B3A81, (DWORD)traits);
	SafeWrite32(0x4B3B80, (DWORD)traits);
	SafeWrite32(0x4B3AAE, (DWORD)traits + 4);
	SafeWrite32(0x4B3BA0, (DWORD)traits + 4);
	SafeWrite32(0x4B3BC0, (DWORD)traits + 8);

	if (perksFile[0] != '\0') {
		char buf[512], num[5] = {'t'};
		char* num2 = &num[1];
		for (int i = 0; i < TRAIT_count; i++) {
			_itoa_s(i, num2, 4, 10);
			if (GetPrivateProfileString(num, "Name", "", &tName[i * maxNameLen], maxNameLen - 1, perksFile)) {
				traits[i].name = &tName[i * maxNameLen];
			}
			if (GetPrivateProfileString(num, "Desc", "", &tDesc[i * descLen], descLen - 1, perksFile)) {
				traits[i].description = &tDesc[i * descLen];
			}
			int value;
			value = GetPrivateProfileInt(num, "Image", -99999, perksFile);
			if (value != -99999) traits[i].image = value;

			if (GetPrivateProfileStringA(num, "StatMod", "", buf, 512, perksFile) > 0) {
				char *stat, *mod;
				stat = strtok(buf, "|");
				mod = strtok(0, "|");
				while (stat&&mod) {
					int _stat = atoi(stat), _mod = atoi(mod);
					if (_stat >= 0 && _stat <= STAT_max_derived) TraitStatBonuses[_stat * TRAIT_count + i] = _mod;
					stat = strtok(0, "|");
					mod = strtok(0, "|");
				}
			}

			if (GetPrivateProfileStringA(num, "SkillMod", "", buf, 512, perksFile) > 0) {
				char *stat, *mod;
				stat = strtok(buf, "|");
				mod = strtok(0, "|");
				while (stat&&mod) {
					int _stat = atoi(stat), _mod = atoi(mod);
					if (_stat >= 0 && _stat < 18) TraitSkillBonuses[_stat * TRAIT_count + i] = _mod;
					stat = strtok(0, "|");
					mod = strtok(0, "|");
				}
			}

			if (GetPrivateProfileInt(num, "NoHardcode", 0, perksFile)) {
				disableTraits[i] = true;
				switch (i) {
				case TRAIT_one_hander:
					HookCall(0x4245E0, BlockedTrait);
					break;
				case TRAIT_finesse:
					HookCall(0x4248F9, BlockedTrait);
					break;
				case TRAIT_fast_shot:
					HookCall(0x478C8A, BlockedTrait); // fast shot
					HookCall(0x478E70, BlockedTrait);
					break;
				case TRAIT_bloody_mess:
					HookCall(0x410707, BlockedTrait);
					break;
				case TRAIT_jinxed:
					HookCall(0x42389F, BlockedTrait);
					break;
				case TRAIT_drug_addict:
					HookCall(0x47A0CD, BlockedTrait);
					HookCall(0x47A51A, BlockedTrait);
					break;
				case TRAIT_drug_resistant:
					HookCall(0x479BE1, BlockedTrait);
					HookCall(0x47A0DD, BlockedTrait);
					break;
				case TRAIT_skilled:
					HookCall(0x43C295, BlockedTrait);
					HookCall(0x43C2F3, BlockedTrait);
					break;
				case TRAIT_gifted:
					HookCall(0x43C2A4, BlockedTrait);
					break;
				}
			}
		}
	}
}

static __declspec(naked) void TraitInitWrapper() {
	__asm {
		call fo::funcoffs::trait_init_;
		push edx;
		push ecx;
		call TraitSetup;
		pop  ecx;
		pop  edx;
		retn;
	}
}

///////////////////////////////////////////////////////////////////////////////

void _stdcall SetPerkValue(int id, int value, DWORD offset) {
	if (id < 0 || id >= PERK_count) return;
	*(DWORD*)((DWORD)(&perks[id]) + offset) = value;
	perksReInit = true;
}

void _stdcall SetPerkName(int id, char* value) {
	if (id < 0 || id >= PERK_count) return;
	strncpy_s(&Name[id * maxNameLen], maxNameLen, value, _TRUNCATE);
	perks[id].name = &Name[maxNameLen * id];
	perksReInit = true;
}

void _stdcall SetPerkDesc(int id, char* value) {
	if (id < 0 || id >= PERK_count) return;
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
	// Pyromaniac bonus
	SafeWrite8(0x424AB6, 5);
	// Perk level mod
	SafeWrite32(0x496880, 0x00019078);
	// Restore 'Heave Ho' modify fix
	if (perkHeaveHoModFix) {
		SafeWrite8(0x478AC4, 0xBA);
		SafeWrite32(0x478AC5, 0x23);
		perkHeaveHoModFix = false;
	}
	if (perksReInit) PerkSetup(); // restore perk data
}

void Perks::save(HANDLE file) {
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

bool Perks::load(HANDLE file) {
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

void _stdcall AddPerkMode(DWORD mode) {
	addPerkMode = mode;
}

DWORD HasFakePerk(const char* name, long id) {
	if (id < PERK_count && name[0] == 0) return 0;
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (id) {
			if (fakePerks[i].id == id) return fakePerks[i].Level;
		} else {
			if (!strcmp(name, fakePerks[i].Name)) {
				return fakePerks[i].Level; // current perk level
			}
		}
	}
	return 0;
}

DWORD _stdcall HasFakeTrait(const char* name) {
	if (name[0] == 0) return 0;
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		if (!strcmp(name, fakeTraits[i].Name)) {
			return 1;
		}
	}
	return 0;
}

void _stdcall ClearSelectablePerks() {
	fakeSelectablePerks.clear();
	addPerkMode = 2;
	IgnoringDefaultPerks = 0;
	SafeWrite32(0x43C77D, 0x488CB);
}

void PerksEnterCharScreen() {
	RemoveTraitID.clear();
	RemovePerkID.clear();
	RemoveSelectableID.clear();
}

void PerksCancelCharScreen() {
	if (RemoveTraitID.size() > 1) RemoveTraitID.sort();
	while (!RemoveTraitID.empty()) {
		fakeTraits.erase(fakeTraits.begin() + RemoveTraitID.back());
		RemoveTraitID.pop_back();
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
	for (int i = STAT_st; i <= STAT_lu; i++) SafeWrite8(GainStatPerks[i][0], (BYTE)GainStatPerks[i][1]);

	PerkEngineInit();
	HookCall(0x442729, PerkInitWrapper);      // game_init_
	if (GetConfigString("Misc", "PerksFile", "", &perksFile[2], MAX_PATH - 3)) {
		perksFile[0] = '.';
		perksFile[1] = '\\';
		HookCall(0x44272E, TraitInitWrapper); // game_init_
	}

	LoadGameHook::OnGameReset() += PerksReset;
}

}
