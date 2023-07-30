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

#include "..\..\main.h"
#include "..\..\Utils.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "EnginePerks.h"

namespace sfall
{
namespace perk
{

static long SalesmanBonus;
static long DemolitionExpertBonus;

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

static void __declspec(naked) perk_adjust_skill_hack_salesman() {
	__asm {
		imul eax, [SalesmanBonus];
		add  ecx, eax; // barter_skill + (perkLevel * SalesmanBonus)
		mov  eax, ecx
		retn;
	}
}

static void __declspec(naked) queue_explode_exit_hack_demolition_expert() {
	__asm {
		imul eax, [DemolitionExpertBonus];
		add  ecx, eax; // maxBaseDmg + (perkLevel * DemolitionExpertBonus)
		add  ebx, eax  // minBaseDmg + (perkLevel * DemolitionExpertBonus)
		retn;
	}
}

void EnginePerkBonusInit() {
	
}
void ReadPerksBonuses(const char* perksFile) {
	int value;
	TryPatchValue32("WeaponScopeRangePenalty", 8, 0, 100, 0x42448E, perksFile);
	TryPatchValue32("WeaponScopeRangeBonus", 5, 2, 100, 0x424489, perksFile);
	TryPatchValue32("WeaponLongRangeBonus", 4, 2, 100, 0x424474, perksFile);
	TryPatchSkillBonus8("WeaponAccurateBonus", 20, 0x42465D, perksFile);
	if (TryGetModifiedInt("WeaponHandlingBonus", 3, value, perksFile) && value >= 0) {
		if (value > 10) value = 10;
		SafeWrite8(0x424636, static_cast<char>(value));
		SafeWrite8(0x4251CE, static_cast<signed char>(-value));
	}
	if (TryGetModifiedInt("MasterTraderBonus", 25, value, perksFile) && value >= 0) {
		float floatValue = static_cast<float>(value);
		SafeWrite32(0x474BB3, *(DWORD*)&floatValue); // write float data
	}
	if (TryGetModifiedInt("SalesmanBonus", 20, value, perksFile) && value >= 0) {
		SalesmanBonus = min(value, 999);
		// Allows the current perk level to affect the calculation of its bonus value
		MakeCall(0x496F5E, perk_adjust_skill_hack_salesman);
	}
	TryPatchSkillBonus8("LivingAnatomyBonus", 5, 0x424A91, perksFile);
	TryPatchSkillBonus8("LivingAnatomyDoctorBonus", 10, 0x496E66, perksFile);
	TryPatchSkillBonus8("PyromaniacBonus", 5, 0x424AB6, perksFile);
	TryPatchValue8("StonewallPercent", 50, 0, 100, 0x424B50, perksFile);
	if (TryGetModifiedInt("DemolitionExpertBonus", 10, value, perksFile) && value >= 0) {
		DemolitionExpertBonus = min(value, 999);
		MakeCall(0x4A289C, queue_explode_exit_hack_demolition_expert, 1);
	}
	if (TryGetModifiedInt("VaultCityInoculationsPoisonBonus", 10, value, perksFile)) {
		SafeWrite8(0x4AF26A, static_cast<signed char>(clamp<long>(value, -100, 100)));
	}
	if (TryGetModifiedInt("VaultCityInoculationsRadBonus", 10, value, perksFile)) {
		SafeWrite8(0x4AF287, static_cast<signed char>(clamp<long>(value, -100, 100)));
	}
	TryPatchSkillBonus8("VaultCityTrainingFirstAidBonus", 5, 0x496E35, perksFile);
	TryPatchSkillBonus8("VaultCityTrainingDoctorBonus", 5, 0x496E7F, perksFile);
	TryPatchSkillBonus32("MedicFirstAidBonus", 10, 0x496E19, perksFile);
	TryPatchSkillBonus32("MedicDoctorBonus", 10, 0x496E4E, perksFile);

	TryPatchSkillBonus32("GhostSneakBonus", 20, 0x496EA9, perksFile);
	TryPatchSkillBonus8("ThiefSkillsBonus", 10, 0x496EC1, perksFile);
	TryPatchSkillBonus8("MasterThiefSkillsBonus", 15, 0x496EE0, perksFile);
	TryPatchSkillBonus8("HarmlessStealBonus", 20, 0x496F02, perksFile);
	TryPatchSkillBonus32("SpeakerSpeechBonus", 20, 0x496F1B, perksFile);
	TryPatchSkillBonus8("ExpertExcrementExpeditorSpeechBonus", 5, 0x496F33, perksFile);
	TryPatchSkillBonus8("NegotiatorSkillsBonus", 10, 0x496F48, perksFile);
	TryPatchSkillBonus8("SalesmanBarterBonus", 20, 0x496F60, perksFile);
	TryPatchSkillBonus32("GamblerGamblingBonus", 20, 0x496F79, perksFile);
	TryPatchSkillBonus32("RangerOutdoorsmanBonus", 15, 0x496F95, perksFile);
	TryPatchSkillBonus8("SurvivalistOutdoorsmanBonus", 25, 0x496FAB, perksFile);
	TryPatchSkillBonus8("MrFixitSkillsBonus", 10, 0x496E00, perksFile);
}

}
}
