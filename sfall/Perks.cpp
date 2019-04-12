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

#include "main.h"

#include "Define.h"
#include "FalloutEngine.h"
#include "Perks.h"

static const int maxNameLen = 64;   // don't change size
static const int maxDescLen = 512;  // don't change size
static const int descLen = 256;     // maximum text length for interface

static char perksFile[MAX_PATH] = {0};

static char Name[maxNameLen * PERK_count] = {0};
static char Desc[descLen * PERK_count] = {0};
static char tName[maxNameLen * TRAIT_count] = {0};
static char tDesc[descLen * TRAIT_count] = {0};
static char PerkBoxTitle[33];

#define check_trait(id) !disableTraits[id] && (pc_trait[0] == id || pc_trait[1] == id)

static DWORD addPerkMode = 2;
static bool perksReInit = false;

struct TraitStruct {
	char* Name;
	char* Desc;
	DWORD Image;
};

struct PerkStruct {
	char* Name;
	char* Desc;
	int Image;
	int Ranks;
	int Level;
	int Stat;
	int StatMag;
	int Skill1;
	int Skill1Mag;
	int Type;
	int Skill2;
	int Skill2Mag;
	int Str;
	int Per;
	int End;
	int Chr;
	int Int;
	int Agl;
	int Lck;
};

//static const PerkStruct BlankPerk={ &Name[PERK_count*64], &Desc[PERK_count*1024], 0x48, 1, 1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 };
static PerkStruct Perks[PERK_count];
static TraitStruct Traits[TRAIT_count];

struct FakePerk {
	int Level;
	int Image;
	char Name[maxNameLen];
	char Desc[maxDescLen];
	char reserve[512]; // empty block
};

std::vector<FakePerk> fakeTraits;
std::vector<FakePerk> fakePerks;
std::vector<FakePerk> fakeSelectablePerks;

static long RemoveTraitID = -1;
static std::list<int> RemovePerkID;
static std::list<int> RemoveSelectableID;

static DWORD TraitSkillBonuses[TRAIT_count * 18] = {0};
static DWORD TraitStatBonuses[TRAIT_count * (STAT_max_derived + 1)] = {0};

static bool disableTraits[TRAIT_count];
static DWORD* pc_trait = (DWORD*)_pc_trait;
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

void _stdcall SetPerkFreq(int i) {
	PerkFreqOverride = i;
}

static bool _stdcall IsTraitDisabled(int id) {
	return disableTraits[id];
}

static void __declspec(naked) LevelUpHack() {
	__asm {
		push ecx;
		mov  ecx, PerkFreqOverride;
		test ecx, ecx;
		jnz  afterSkilled;
		push TRAIT_skilled;
		call IsTraitDisabled;
		test al, al;
		jnz  notSkilled;
		mov  eax, TRAIT_skilled;
		call trait_level_; // Check if the player has the skilled trait
		test eax, eax;
		jz   notSkilled;
		mov  ecx, 4;
		jmp  afterSkilled;
notSkilled:
		mov  ecx, 3;
afterSkilled:
		mov  eax, ds:[_Level_]; // Get players level
		inc  eax;
		xor  edx, edx;
		div  ecx;
		test edx, edx;
		jnz  end;
		inc  byte ptr ds:[_free_perk]; // Increment the number of perks owed
end:
		pop  ecx;
		mov  edx, ds:[_Level_];
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
		FakePerk fp;
		memset(&fp, 0, sizeof(FakePerk));
		fp.Level = active;
		fp.Image = image;
		strncpy_s(fp.Name, name, _TRUNCATE);
		strncpy_s(fp.Desc, desc, _TRUNCATE);
		fakeSelectablePerks.push_back(fp);
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
		FakePerk fp;
		memset(&fp, 0, sizeof(FakePerk));
		fp.Level = level;
		fp.Image = image;
		strncpy_s(fp.Name, name, _TRUNCATE);
		strncpy_s(fp.Desc, desc, _TRUNCATE);
		fakePerks.push_back(fp);
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
		FakePerk fp;
		memset(&fp, 0, sizeof(FakePerk));
		fp.Level = active;
		fp.Image = image;
		strncpy_s(fp.Name, name, _TRUNCATE);
		strncpy_s(fp.Desc, desc, _TRUNCATE);
		fakeTraits.push_back(fp);
	}
}

static DWORD _stdcall HaveFakeTraits2() {
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

static DWORD _stdcall HaveFakePerks2() {
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

static FakePerk* _stdcall GetFakePerk2(int id) {
	return &fakePerks[id - PERK_count];
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

static FakePerk* _stdcall GetFakeSPerk2(int id) {
	return &fakeSelectablePerks[id - PERK_count];
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

static DWORD _stdcall GetFakeSPerkLevel2(int id) {
	char* name = fakeSelectablePerks[id - PERK_count].Name;
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

static DWORD _stdcall HandleFakeTraits(int isSelect) {
	for (DWORD i = 0; i < fakeTraits.size(); i++) {
		DWORD a = (DWORD)fakeTraits[i].Name;
		__asm {
			mov  eax, a;
			call folder_print_line_;
			mov  a, eax;
		}
		if (a && !isSelect) {
			isSelect = 1;
			*(DWORD*)_folder_card_fid = fakeTraits[i].Image;
			*(DWORD*)_folder_card_title = (DWORD)fakeTraits[i].Name;
			*(DWORD*)_folder_card_title2 = 0;
			*(DWORD*)_folder_card_desc = (DWORD)fakeTraits[i].Desc;
		}
	}
	return isSelect;
}

static void __declspec(naked) PlayerHasPerkHack() {
	__asm {
		push ecx;
		call HandleFakeTraits;
		mov  ecx, eax;
		xor  ebx, ebx;
oloop:
		mov  eax, ds:[_obj_dude];
		mov  edx, ebx;
		call perk_level_;
		test eax, eax;
		jnz  win;
		inc  ebx;
		cmp  ebx, PERK_count;
		jl   oloop;
		call HaveFakePerks;
		test eax, eax;
		jnz  win;
		mov  eax, 0x434446;
		jmp  eax;
win:
		mov  eax, 0x43438A;
		jmp  eax;
	}
}

static void __declspec(naked) PlayerHasTraitHook() {
	__asm {
		call HaveFakeTraits;
		test eax, eax;
		jz   end;
		mov  eax, 0x43425B;
		jmp  eax;
end:
		jmp  PlayerHasPerkHack;
	}
}

static void __declspec(naked) GetPerkLevelHook() {
	__asm {
		cmp  edx, PERK_count;
		jl   end;
		push edx;
		call GetFakePerk;
		mov  eax, ds:[eax];
		retn;
end:
		jmp  perk_level_;
	}
}

static void __declspec(naked) GetPerkImageHook() {
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakePerk;
		mov  eax, ds:[eax + 4];
		retn;
end:
		jmp  perk_skilldex_fid_;
	}
}

static void __declspec(naked) GetPerkNameHook() {
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakePerk;
		lea  eax, ds:[eax + 8];
		retn;
end:
		jmp  perk_name_;
	}
}

static void __declspec(naked) GetPerkDescHook() {
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakePerk;
		lea  eax, ds:[eax + 72];
		retn;
end:
		jmp  perk_description_
	}
}

// Search all available perks for the player to display them in the character screen
static const DWORD EndPerkLoopExit = 0x434446;
static const DWORD EndPerkLoopCont = 0x4343A5;
static void __declspec(naked) EndPerkLoopHack() {
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

// Build a table of perks ID numbers available for selection
// data buffer has limited size for 119 perks
static DWORD _stdcall HandleExtraSelectablePerks(DWORD available, DWORD* data) {
	for (DWORD i = 0; i < fakeSelectablePerks.size(); i++) {
		if (available >= 119) break; // exit if the buffer is overfull
		data[available++] = PERK_count + i; //*(WORD*)(_name_sort_list + (offset+i)*8)=(WORD)(PERK_count+i);
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
		call perk_make_list_; // return available count
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
		jl   end;
		push edx;
		call GetFakeSPerkLevel;
		retn;
end:
		jmp  perk_level_;
	}
}

static void __declspec(naked) GetPerkSImageHook() {
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakeSPerk;
		mov  eax, ds:[eax + 4];
		retn;
end:
		jmp  perk_skilldex_fid_;
	}
}

static void __declspec(naked) GetPerkSNameHook() {
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakeSPerk;
		lea  eax, ds:[eax + 8];
		retn;
end:
		jmp  perk_name_;
	}
}

static void __declspec(naked) GetPerkSDescHook() {
	__asm {
		cmp  eax, PERK_count;
		jl   end;
		push eax;
		call GetFakeSPerk;
		lea  eax, ds:[eax + 72];
		retn;
end:
		jmp  perk_description_;
	}
}

// Adds the selected perk to the player
static void _stdcall AddFakePerk(DWORD perkID) {
	size_t count;
	bool matched = false;
	// behavior for fake perk/trait
	perkID -= PERK_count;
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
		call perk_add_;
		pop  edx;
		test eax, eax;
		jnz  end;
		// fix gain perks
		cmp  edx, PERK_gain_strength_perk;
		jl   end;
		cmp  edx, PERK_gain_luck_perk;
		jg   end;
		inc  ds:[edx * 4 + (_pc_proto + 0x24 - PERK_gain_strength_perk * 4)]; // base_stat_srength
end:
		retn;
	}
}

static void __declspec(naked) HeaveHoHook() {
	__asm {
		xor  edx, edx;
		mov  eax, ecx;
		call stat_level_;
		lea  ebx, [0 + eax * 4];
		sub  ebx, eax;
		cmp  ebx, esi;      // ebx = dist (3*ST), esi = max dist weapon
		jle  lower;         // jump if dist <= max
		mov  ebx, esi;      // dist = max
lower:
		mov  eax, ecx;
		mov  edx, PERK_heave_ho;
		call perk_level_;
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
	Perks[PERK_heave_ho].Str = 0;
	perkHeaveHoModFix = true;
}

static void PerkEngineInit() {
	// Character screen (list_perks_)
	HookCall(0x434256, PlayerHasTraitHook); // jz func
	MakeJump(0x43436B, PlayerHasPerkHack);
	HookCall(0x4343AC, GetPerkLevelHook);
	HookCall(0x4343C1, GetPerkNameHook);
	HookCall(0x4343DF, GetPerkNameHook);
	HookCall(0x43440D, GetPerkImageHook);
	HookCall(0x43441B, GetPerkNameHook);
	HookCall(0x434432, GetPerkDescHook);
	MakeJump(0x434440, EndPerkLoopHack, 1);

	// GetPlayerAvailablePerks (ListDPerks_)
	HookCall(0x43D127, GetAvailablePerksHook);
	HookCall(0x43D17D, GetPerkSNameHook);
	HookCall(0x43D25E, GetPerkSLevelHook);
	HookCall(0x43D275, GetPerkSLevelHook);

	// ShowPerkBox (perks_dialog_)
	HookCall(0x43C82E, GetPerkSLevelHook);
	HookCall(0x43C85B, GetPerkSLevelHook);
	HookCall(0x43C888, GetPerkSDescHook);
	HookCall(0x43C8A6, GetPerkSNameHook);
	HookCall(0x43C8D1, GetPerkSDescHook);
	HookCall(0x43C8EF, GetPerkSNameHook);
	HookCall(0x43C90F, GetPerkSImageHook);
	HookCall(0x43C952, AddPerkHook);

	// PerkboxSwitchPerk (RedrwDPrks_)
	HookCall(0x43C3F1, GetPerkSLevelHook);
	HookCall(0x43C41E, GetPerkSLevelHook);
	HookCall(0x43C44B, GetPerkSDescHook);
	HookCall(0x43C469, GetPerkSNameHook);
	HookCall(0x43C494, GetPerkSDescHook);
	HookCall(0x43C4B2, GetPerkSNameHook);
	HookCall(0x43C4D2, GetPerkSImageHook);

	// perk_owed hooks
	MakeCall(0x4AFB2F, LevelUpHack, 1); // replaces 'mov edx, ds:[PlayerLevel]'
	SafeWrite8(0x43C2EC, 0xEB); // skip the block of code which checks if the player has gained a perk (now handled in level up code)

	// Disable losing unused perks
	SafeWrite16(0x43C369, 0x0DFE); // dec  byte ptr ds:_free_perk
	SafeWrite8(0x43C370, 0xB1);    // jmp  0x43C322
}

static void PerkSetup() {
	memcpy(Perks, (void*)_perk_data, sizeof(PerkStruct) * PERK_count); // copy vanilla data

	if (!perksReInit) {
		// _perk_data
		SafeWrite32(0x496669, (DWORD)Perks);
		SafeWrite32(0x496837, (DWORD)Perks);
		SafeWrite32(0x496BAD, (DWORD)Perks);
		SafeWrite32(0x496C41, (DWORD)Perks);
		SafeWrite32(0x496D25, (DWORD)Perks);
		SafeWrite32(0x496696, (DWORD)Perks + 4);
		SafeWrite32(0x496BD1, (DWORD)Perks + 4);
		SafeWrite32(0x496BF5, (DWORD)Perks + 8);
		SafeWrite32(0x496AD4, (DWORD)Perks + 12);
	}
	if (perksFile[0] != '\0') {
		char num[4];
		for (int i = 0; i < PERK_count; i++) {
			_itoa_s(i, num, 10);
			if (GetPrivateProfileString(num, "Name", "", &Name[i * maxNameLen], maxNameLen - 1, perksFile)) {
				Perks[i].Name = &Name[i * maxNameLen];
			}
			if (GetPrivateProfileString(num, "Desc", "", &Desc[i * descLen], descLen - 1, perksFile)) {
				Perks[i].Desc = &Desc[i * descLen];
			}
			int value;
			value = GetPrivateProfileInt(num, "Image", -99999, perksFile);
			if (value != -99999) Perks[i].Image = value;
			value = GetPrivateProfileInt(num, "Ranks", -99999, perksFile);
			if (value != -99999) Perks[i].Ranks = value;
			value = GetPrivateProfileInt(num, "Level", -99999, perksFile);
			if (value != -99999) Perks[i].Level = value;
			value = GetPrivateProfileInt(num, "Stat", -99999, perksFile);
			if (value != -99999) Perks[i].Stat = value;
			value = GetPrivateProfileInt(num, "StatMag", -99999, perksFile);
			if (value != -99999) Perks[i].StatMag = value;
			value = GetPrivateProfileInt(num, "Skill1", -99999, perksFile);
			if (value != -99999) Perks[i].Skill1 = value;
			value = GetPrivateProfileInt(num, "Skill1Mag", -99999, perksFile);
			if (value != -99999) Perks[i].Skill1Mag = value;
			value = GetPrivateProfileInt(num, "Type", -99999, perksFile);
			if (value != -99999) Perks[i].Type = value;
			value = GetPrivateProfileInt(num, "Skill2", -99999, perksFile);
			if (value != -99999) Perks[i].Skill2 = value;
			value = GetPrivateProfileInt(num, "Skill2Mag", -99999, perksFile);
			if (value != -99999) Perks[i].Skill2Mag = value;
			value = GetPrivateProfileInt(num, "STR", -99999, perksFile);
			if (value != -99999) Perks[i].Str = value;
			value = GetPrivateProfileInt(num, "PER", -99999, perksFile);
			if (value != -99999) Perks[i].Per = value;
			value = GetPrivateProfileInt(num, "END", -99999, perksFile);
			if (value != -99999) Perks[i].End = value;
			value = GetPrivateProfileInt(num, "CHR", -99999, perksFile);
			if (value != -99999) Perks[i].Chr = value;
			value = GetPrivateProfileInt(num, "INT", -99999, perksFile);
			if (value != -99999) Perks[i].Int = value;
			value = GetPrivateProfileInt(num, "AGL", -99999, perksFile);
			if (value != -99999) Perks[i].Agl = value;
			value = GetPrivateProfileInt(num, "LCK", -99999, perksFile);
			if (value != -99999) Perks[i].Lck = value;
		}
	}
	perksReInit = false;
}

static __declspec(naked) void PerkInitWrapper() {
	__asm {
		call perk_init_;
		push edx;
		push ecx;
		call PerkSetup;
		pop  ecx;
		pop  edx;
		retn;
	}
}

/////////////////////////// TRAIT FUNCTIONS ///////////////////////////////////

static int _stdcall stat_get_base_direct(DWORD statID) {
	DWORD result;
	__asm {
		mov  edx, statID;
		mov  eax, dword ptr ds:[_obj_dude];
		call stat_get_base_direct_;
		mov  result, eax;
	}
	return result;
}

static int _stdcall trait_adjust_stat_override(DWORD statID) {
	if (statID > STAT_max_derived) return 0;

	int result = 0;
	if (pc_trait[0] != -1) result += TraitStatBonuses[statID * TRAIT_count + pc_trait[0]];
	if (pc_trait[1] != -1) result += TraitStatBonuses[statID * TRAIT_count + pc_trait[1]];

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
	if (pc_trait[0] != -1) {
		result += TraitSkillBonuses[skillID * TRAIT_count + pc_trait[0]];
	}
	if (pc_trait[1] != -1) {
		result += TraitSkillBonuses[skillID * TRAIT_count + pc_trait[1]];
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

	memcpy(Traits, (void*)_trait_data, sizeof(TraitStruct) * TRAIT_count);

	// _trait_data
	SafeWrite32(0x4B3A81, (DWORD)Traits);
	SafeWrite32(0x4B3B80, (DWORD)Traits);
	SafeWrite32(0x4B3AAE, (DWORD)Traits + 4);
	SafeWrite32(0x4B3BA0, (DWORD)Traits + 4);
	SafeWrite32(0x4B3BC0, (DWORD)Traits + 8);

	if (perksFile[0] != '\0') {
		char buf[512], num[5] = {'t'};
		char* num2 = &num[1];
		for (int i = 0; i < TRAIT_count; i++) {
			_itoa_s(i, num2, 4, 10);
			if (GetPrivateProfileString(num, "Name", "", &tName[i * maxNameLen], maxNameLen - 1, perksFile)) {
				Traits[i].Name = &tName[i * maxNameLen];
			}
			if (GetPrivateProfileString(num, "Desc", "", &tDesc[i * descLen], descLen - 1, perksFile)) {
				Traits[i].Desc = &tDesc[i * descLen];
			}
			int value;
			value = GetPrivateProfileInt(num, "Image", -99999, perksFile);
			if (value != -99999) Traits[i].Image = value;

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
		call trait_init_;
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
	*(DWORD*)((DWORD)(&Perks[id]) + offset) = value;
	perksReInit = true;
}

void _stdcall SetPerkName(int id, char* value) {
	if (id < 0 || id >= PERK_count) return;
	strncpy_s(&Name[id * maxNameLen], maxNameLen, value, _TRUNCATE);
	Perks[id].Name = &Name[maxNameLen * id];
	perksReInit = true;
}

void _stdcall SetPerkDesc(int id, char* value) {
	if (id < 0 || id >= PERK_count) return;
	strncpy_s(&Desc[id * descLen], descLen, value, _TRUNCATE);
	Perks[id].Desc = &Desc[descLen * id];
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

void PerksSave(HANDLE file) {
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

bool PerksLoad(HANDLE file) {
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

void _stdcall AddPerkMode(DWORD mode) {
	addPerkMode = mode;
}

DWORD _stdcall HasFakePerk(char* name) {
	for (DWORD i = 0; i < fakePerks.size(); i++) {
		if (!strcmp(name, fakePerks[i].Name)) {
			return fakePerks[i].Level;
		}
	}
	return 0;
}

DWORD _stdcall HasFakeTrait(char* name) {
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

void PerksInit() {
	for (int i = STAT_st; i <= STAT_lu; i++) SafeWrite8(GainStatPerks[i][0], (BYTE)GainStatPerks[i][1]);

	PerkEngineInit();
	HookCall(0x442729, PerkInitWrapper);      // game_init_
	if (GetPrivateProfileString("Misc", "PerksFile", "", &perksFile[2], MAX_PATH - 3, ini)) {
		perksFile[0] = '.';
		perksFile[1] = '\\';
		HookCall(0x44272E, TraitInitWrapper); // game_init_
	}
}
