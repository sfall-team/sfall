/*
 *    sfall
 *    Copyright (C) 2008-2025  The sfall team
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

#include "..\..\main.h"
#include "..\..\Utils.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "EnginePerks.h"

namespace sfall
{
namespace perk
{

static long SalesmanBonus = 20;
static long DemolitionExpertBonus = 10;
static long NightVisionBonus = 13107; // 20% of max light
static long ComprehensionBonus = 150; // +50% of earned skill points
static long EducatedBonus = 2;
static long HealerMinBonus = 4;
static long HealerMaxBonus = 10;
static long LifegiverBonus = 4;

static bool TryGetModifiedInt(const char* key, int defaultValue, int& outValue, const char* perksFile) {
	outValue = IniReader::GetInt("PerksTweak", key, defaultValue, perksFile);
	return outValue != defaultValue;
}

static void TryPatchValue8(const char* key, int defaultValue, int minValue, int maxValue, DWORD addr, const char* perksFile) {
	int value;
	if (TryGetModifiedInt(key, defaultValue, value, perksFile) && value >= minValue) {
		SafeWrite8(addr, static_cast<BYTE>(min(value, maxValue)));
	}
}

static void TryPatchValue32(const char* key, int defaultValue, int minValue, int maxValue, DWORD addr, const char* perksFile) {
	int value;
	if (TryGetModifiedInt(key, defaultValue, value, perksFile) && value >= minValue) {
		SafeWrite32(addr, min(value, maxValue));
	}
}

static void TryPatchSkillBonus8(const char* key, int defaultValue, DWORD addr, const char* perksFile) {
	TryPatchValue8(key, defaultValue, 0, 125, addr, perksFile);
}

static void TryPatchSkillBonus32(const char* key, int defaultValue, DWORD addr, const char* perksFile) {
	TryPatchValue32(key, defaultValue, 0, 125, addr, perksFile);
}

static __declspec(naked) void perk_adjust_skill_hack_salesman() {
	__asm {
		imul eax, [SalesmanBonus];
		add  ecx, eax; // barter_skill + (perkLevel * SalesmanBonus)
		mov  eax, ecx
		retn;
	}
}

static __declspec(naked) void queue_explode_exit_hack_demolition_expert() {
	__asm {
		imul eax, [DemolitionExpertBonus];
		add  ecx, eax; // maxBaseDmg + (perkLevel * DemolitionExpertBonus)
		add  ebx, eax  // minBaseDmg + (perkLevel * DemolitionExpertBonus)
		retn;
	}
}

static __declspec(naked) void light_set_ambient_hack_night_vision() {
	static const DWORD light_set_ambient_night_vision_Ret = 0x47A932;
	__asm {
		imul eax, [NightVisionBonus];
		jmp  light_set_ambient_night_vision_Ret;
	}
}

static __declspec(naked) void obj_use_book_hack_comprehension() {
	static const DWORD obj_use_book_comprehension_Ret = 0x49BADD;
	__asm {
		imul esi, [ComprehensionBonus];
		mov  edx, esi;
		jmp  obj_use_book_comprehension_Ret;
	}
}

static __declspec(naked) void UpdateLevel_hack_educated() {
	__asm {
		call fo::funcoffs::perk_level_;
		imul eax, [EducatedBonus];
		retn;
	}
}

static __declspec(naked) void skill_use_hack_healer() {
	static const DWORD skill_use_healer_Ret = 0x4AADDC;
	__asm {
		mov  edx, eax;
		imul edx, [HealerMinBonus];
		mov  [esp + 0xC8 - 0x34], edx; // minimumHpToHeal
		imul eax, [HealerMaxBonus];
		jmp  skill_use_healer_Ret;
	}
}

static __declspec(naked) void statPcExperience_hack_lifegiver() {
	__asm {
		call fo::funcoffs::perk_level_;
		imul eax, [LifegiverBonus];
		retn;
	}
}

void EnginePerkBonusInit() {
	// Allow the current perk level to affect the calculation of its bonus value
	MakeCall(0x496F5E, perk_adjust_skill_hack_salesman);
	MakeCall(0x4A289C, queue_explode_exit_hack_demolition_expert, 1);
	// Allow configurable bonuses
	MakeJump(0x47A91D, light_set_ambient_hack_night_vision);
	MakeJump(0x49BACD, obj_use_book_hack_comprehension);
	MakeCall(0x43C287, UpdateLevel_hack_educated, 2);
	MakeJump(0x4AADC5, skill_use_hack_healer, 2);
	MakeCall(0x4AFBCC, statPcExperience_hack_lifegiver, 3); // statPCAddExperienceCheckPMs_
	MakeCall(0x4AFCBB, statPcExperience_hack_lifegiver, 3); // statPcResetExperience_
}

void ReadPerksBonuses(const char* perksFile) {
	int value;
	TryPatchValue32("WeaponScopeRangePenalty", 8, 0, 100, 0x42448E, perksFile);
	TryPatchValue32("WeaponScopeRangeBonus", 5, 2, 100, 0x424489, perksFile);
	TryPatchValue32("WeaponLongRangeBonus", 4, 2, 100, 0x424474, perksFile);
	TryPatchSkillBonus8("WeaponAccurateBonus", 20, 0x42465D, perksFile);

	TryPatchSkillBonus8("SurvivalistBonus", 25, 0x496FAB, perksFile);
	TryPatchSkillBonus8("MrFixitBonus", 10, 0x496E00, perksFile);
	TryPatchSkillBonus32("MedicFirstAidBonus", 10, 0x496E19, perksFile);
	TryPatchSkillBonus32("MedicDoctorBonus", 10, 0x496E4E, perksFile);
	TryPatchSkillBonus8("MasterThiefBonus", 15, 0x496EE0, perksFile);
	TryPatchSkillBonus32("SpeakerBonus", 20, 0x496F1B, perksFile);
	TryPatchSkillBonus32("GhostBonus", 20, 0x496EA9, perksFile);
	TryPatchSkillBonus32("RangerOutdoorsmanBonus", 15, 0x496F95, perksFile);
	TryPatchValue8("CautiousNatureBonus", 3, -12, 20, 0x4C1756, perksFile); // -12 - force distance to 0
	TryPatchSkillBonus32("GamblerBonus", 20, 0x496F79, perksFile);
	TryPatchSkillBonus8("HarmlessBonus", 20, 0x496F02, perksFile);
	TryPatchSkillBonus8("LivingAnatomyBonus", 5, 0x424A91, perksFile);
	TryPatchSkillBonus8("LivingAnatomyDoctorBonus", 10, 0x496E66, perksFile);
	TryPatchSkillBonus8("NegotiatorBonus", 10, 0x496F48, perksFile);
	TryPatchSkillBonus8("PyromaniacBonus", 5, 0x424AB6, perksFile);
	TryPatchValue8("StonewallPercent", 50, 0, 100, 0x424B50, perksFile);
	TryPatchSkillBonus8("ThiefBonus", 10, 0x496EC1, perksFile);
	TryPatchSkillBonus8("VaultCityTrainingFirstAidBonus", 5, 0x496E35, perksFile);
	TryPatchSkillBonus8("VaultCityTrainingDoctorBonus", 5, 0x496E7F, perksFile);
	TryPatchSkillBonus8("ExpertExcrementExpeditorBonus", 5, 0x496F33, perksFile);

	if (TryGetModifiedInt("NightVisionBonus", 20, value, perksFile) && value >= 0) {
		if (value > 100) value = 100;
		NightVisionBonus = (65536 * value) / 100;
	}
	if (TryGetModifiedInt("MasterTraderBonus", 25, value, perksFile) && value >= 0) {
		float floatValue = static_cast<float>(value);
		SafeWrite32(0x474BB3, *(DWORD*)&floatValue); // write float data
	}
	if (TryGetModifiedInt("EducatedBonus", EducatedBonus, value, perksFile) && value >= 0) {
		EducatedBonus = min(value, 125);
		SafeWrite8(0x43CA6D, static_cast<signed char>(EducatedBonus));
	}
	if (TryGetModifiedInt("HealerMinBonus", HealerMinBonus, value, perksFile) && value >= 0) {
		HealerMinBonus = min(value, 999);
	}
	if (TryGetModifiedInt("HealerMaxBonus", HealerMaxBonus, value, perksFile) && value >= 0) {
		HealerMaxBonus = min(value, 999);
	}
	if (TryGetModifiedInt("LifegiverBonus", LifegiverBonus, value, perksFile) && value >= 0) {
		LifegiverBonus = min(value, 125);
		SafeWriteBatch<BYTE>(static_cast<signed char>(LifegiverBonus), {0x43CA2C, 0x43CA3D});
	}
	if (TryGetModifiedInt("VaultCityInoculationsPoisonBonus", 10, value, perksFile)) {
		SafeWrite8(0x4AF26A, static_cast<signed char>(clamp<long>(value, -100, 100)));
	}
	if (TryGetModifiedInt("VaultCityInoculationsRadBonus", 10, value, perksFile)) {
		SafeWrite8(0x4AF287, static_cast<signed char>(clamp<long>(value, -100, 100)));
	}
	if (TryGetModifiedInt("ComprehensionBonus", 50, value, perksFile) && value >= 0) {
		ComprehensionBonus = value + 100;
	}
	if (TryGetModifiedInt("DemolitionExpertBonus", DemolitionExpertBonus, value, perksFile) && value >= 0) {
		DemolitionExpertBonus = min(value, 999);
	}
	if (TryGetModifiedInt("SalesmanBonus", SalesmanBonus, value, perksFile) && value >= 0) {
		SalesmanBonus = min(value, 999);
	}
	if (TryGetModifiedInt("WeaponHandlingBonus", 3, value, perksFile) && value >= 0) {
		if (value > 10) value = 10;
		SafeWrite8(0x424636, static_cast<signed char>(value));
		SafeWrite8(0x4251CE, static_cast<signed char>(-value));
	}
}

}
}
