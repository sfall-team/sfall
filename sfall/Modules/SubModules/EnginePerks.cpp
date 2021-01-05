/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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
#include "..\..\FalloutEngine\Fallout2.h"

#include "EnginePerks.h"

namespace sfall
{
namespace perk
{

static class EnginePerkBonus {
public:
	long WeaponScopeRangePenalty = 8;
	long WeaponScopeRangeBonus   = 5;
	long WeaponLongRangeBonus    = 4;
	long WeaponAccurateBonus     = 20;
	long WeaponHandlingBonus     = 3;

	float MasterTraderBonus      = 25;
	long SalesmanBonus           = 20;

	long LivingAnatomyBonus      = 5;
	long PyromaniacBonus         = 5;

	long StonewallPercent        = 50;

	long DemolitionExpertBonus   = 10;

	long VaultCityInoculationsPoisonBonus = 10;
	long VaultCityInoculationsRadBonus    = 10;

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
		if (SalesmanBonus > 125) SalesmanBonus = 125;
		SafeWrite8(0x496F60, static_cast<BYTE>(SalesmanBonus));
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

} perks;

static void __declspec(naked) perk_adjust_skill_hack_salesman() {
	__asm {
		imul eax, [perks.SalesmanBonus];
		add  ecx, eax; // barter_skill + (perkLevel * SalesmanBonus)
		mov  eax, ecx
		retn;
	}
}

static void __declspec(naked) queue_explode_exit_hack_demolition_expert() {
	__asm {
		imul eax, [perks.DemolitionExpertBonus];
		add  ecx, eax; // maxBaseDmg + (perkLevel * DemolitionExpertBonus)
		add  ebx, eax  // minBaseDmg + (perkLevel * DemolitionExpertBonus)
		retn;
	}
}

void EnginePerkBonusInit() {
	// Allows the current perk level to affect the calculation of its bonus value
	MakeCall(0x496F5E, perk_adjust_skill_hack_salesman);
	MakeCall(0x4A289C, queue_explode_exit_hack_demolition_expert, 1);
}

void ReadPerksBonuses(const char* perksFile) {
	int wScopeRangeMod = iniGetInt("PerksTweak", "WeaponScopeRangePenalty", 8, perksFile);
	if (wScopeRangeMod != 8) perks.setWeaponScopeRangePenalty(wScopeRangeMod);
	wScopeRangeMod = iniGetInt("PerksTweak", "WeaponScopeRangeBonus", 5, perksFile);
	if (wScopeRangeMod != 5) perks.setWeaponScopeRangeBonus(wScopeRangeMod);

	int enginePerkMod = iniGetInt("PerksTweak", "WeaponLongRangeBonus", 4, perksFile);
	if (enginePerkMod != 4) perks.setWeaponLongRangeBonus(enginePerkMod);

	enginePerkMod = iniGetInt("PerksTweak", "WeaponAccurateBonus", 20, perksFile);
	if (enginePerkMod != 20) perks.setWeaponAccurateBonus(enginePerkMod);

	enginePerkMod = iniGetInt("PerksTweak", "WeaponHandlingBonus", 3, perksFile);
	if (enginePerkMod != 3) perks.setWeaponHandlingBonus(enginePerkMod);

	int masterTraderBonus = iniGetInt("PerksTweak", "MasterTraderBonus", 25, perksFile);
	if (masterTraderBonus != 25) perks.setMasterTraderBonus(masterTraderBonus);

	int salesmanBonus = iniGetInt("PerksTweak", "SalesmanBonus", 20, perksFile);
	if (salesmanBonus != 20) perks.setSalesmanBonus(salesmanBonus);

	int livingAnatomyBonus = iniGetInt("PerksTweak", "LivingAnatomyBonus", 5, perksFile);
	if (livingAnatomyBonus != 5) perks.setLivingAnatomyBonus(livingAnatomyBonus);

	int pyromaniacBonus = iniGetInt("PerksTweak", "PyromaniacBonus", 5, perksFile);
	if (pyromaniacBonus != 5) perks.setPyromaniacBonus(pyromaniacBonus);

	int stonewallPercent = iniGetInt("PerksTweak", "StonewallPercent", 50, perksFile);
	if (stonewallPercent != 50) perks.setStonewallPercent(stonewallPercent);

	int demolitionExpertBonus = iniGetInt("PerksTweak", "DemolitionExpertBonus", 10, perksFile);
	if (demolitionExpertBonus != 10) perks.setDemolitionExpertBonus(demolitionExpertBonus);

	int vaultCityInoculationsBonus = iniGetInt("PerksTweak", "VaultCityInoculationsPoisonBonus", 10, perksFile);
	if (vaultCityInoculationsBonus != 10) perks.setVaultCityInoculationsPoisonBonus(vaultCityInoculationsBonus);
	vaultCityInoculationsBonus = iniGetInt("PerksTweak", "VaultCityInoculationsRadBonus", 10, perksFile);
	if (vaultCityInoculationsBonus != 10) perks.setVaultCityInoculationsRadBonus(vaultCityInoculationsBonus);
}

}
}
