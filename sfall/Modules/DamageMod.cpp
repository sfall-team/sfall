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

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "..\Logging.h"
#include "..\Translate.h"
#include "HookScripts.h"
#include "Unarmed.h"

#include "..\Game\stats.h"

#include "DamageMod.h"

namespace sfall
{

int DamageMod::formula;

static char ammoInfoFmt[32];

// Integer division w/ round half to even for Glovz's damage formula
// Prerequisite: divisor must be a positive integer (should already be handled in the main function)
// if dividend is negative, the result will be 0 or 1 (for values in range of -1073741825 to -2147483646)
static long DivRound(long dividend, long divisor) {
	if (dividend <= divisor) {
		// if equal then return 1
		return (dividend != divisor && (dividend << 1) <= divisor) ? 0 : 1;
	}

	long quotient = dividend / divisor;
	dividend %= divisor; // get the remainder

	// check the remainder
	if (dividend == 0) return quotient;

	dividend <<= 1; // multiply by 2

	// if equal then round to even
	return (dividend > divisor || (dividend == divisor && (quotient & 1))) ? ++quotient : quotient;
}

// Damage Fix v5 (with v5.1 Damage Multiplier tweak) by Glovz 2014.04.16.xx.xx
void DamageMod::DamageGlovz(fo::ComputeAttackResult &ctd, DWORD &accumulatedDamage, long rounds, long armorDT, long armorDR, long bonusRangedDamage, long multiplyDamage, long difficulty) {
	if (rounds <= 0) return;                                // check the number of hits

	long ammoY = fo::func::item_w_dam_div(ctd.weapon);      // ammoY value (divisor)
	if (ammoY <= 0) ammoY = 1;

	long ammoX = fo::func::item_w_dam_mult(ctd.weapon);     // ammoX value
	if (ammoX <= 0) ammoX = 1;

	long ammoDRM = fo::func::item_w_dr_adjust(ctd.weapon);  // ammoDRM value
	if (ammoDRM > 0) ammoDRM = -ammoDRM;                    // to negative

	long calcDT = (armorDT > 0) ? DivRound(armorDT, ammoY) : armorDT;

	long calcDR = armorDR;
	if (armorDR > 0) {
		if (difficulty > 100) {                             // if the CD value is greater than 100
			calcDR -= 20;                                   // subtract 20 from the armorDR value
		} else if (difficulty < 100) {                      // if the CD value is less than 100
			calcDR += 20;                                   // add 20 to the armorDR value
		}
		calcDR += ammoDRM;                                  // add the ammoDRM value to the armorDR value
		calcDR = DivRound(calcDR, ammoX);                   // goto divTwo
		if (calcDR >= 100) return;                          // if armorDR >= 100, skip damage calculation
	}

	// start of damage calculation loop
	for (long i = 0; i < rounds; i++) {
		long rawDamage = fo::func::item_w_damage(ctd.attacker, ctd.hitMode); // get the raw damage value
		rawDamage += bonusRangedDamage;                     // add the bonus ranged damage value to the RD value
		if (rawDamage <= 0) continue;                       // if raw damage <= 0, skip damage calculation and go to bottom of loop

		if (armorDT > 0) {                                  // compare the armorDT value to 0
			rawDamage -= calcDT;                            // subtract the new armorDT value from the RD value
			if (rawDamage <= 0) continue;                   // if raw damage <= 0, skip damage calculation and go to bottom of loop
		}

		if (armorDR > 0) {                                  // compare the armorDR value to 0
			long resistedDamage = calcDR * rawDamage;
			resistedDamage = DivRound(resistedDamage, 100); // goto divThree
			rawDamage -= resistedDamage;                    // subtract the damage resisted value from the RD value
			if (rawDamage <= 0) continue;                   // if raw damage <= 0, skip damage calculation and go to bottom of loop
		}

		// bonus damage to unarmored target
		if (armorDT <= 0 && armorDR <= 0) {
			if (ammoX > 1 && ammoY > 1) {                   // FMJ/high-end
				rawDamage += DivRound(rawDamage * 15, 100); // goto divFour
			} else if (ammoX > 1) {                         // JHP
				rawDamage += DivRound(rawDamage * 20, 100); // goto divFive
			} else if (ammoY > 1) {                         // AP
				rawDamage += DivRound(rawDamage * 10, 100); // goto divSix
			}
		}

		if (formula == 2) { // v5.1 tweak
			rawDamage += DivRound(rawDamage * multiplyDamage * 25, 100); // goto divSeven
		} else {
			rawDamage = (rawDamage * multiplyDamage) >> 1;  // divide the result by 2
		}
		if (rawDamage > 0) accumulatedDamage += rawDamage;  // accumulate damage (make sure the result > 0 before adding)
	}
}

static __declspec(naked) void AmmoInfoPrintGlovz() {
	__asm {
		lea  edi, ammoInfoFmt;
		retn;
	}
}

// YAAM v1.1a by Haenlomal 2010.05.13
void DamageMod::DamageYAAM(fo::ComputeAttackResult &ctd, DWORD &accumulatedDamage, long rounds, long armorDT, long armorDR, long bonusRangedDamage, long multiplyDamage, long difficulty) {
	if (rounds <= 0) return;                                // Check number of hits

	long ammoDiv = fo::func::item_w_dam_div(ctd.weapon);    // Retrieve Ammo Divisor
	long ammoMult = fo::func::item_w_dam_mult(ctd.weapon);  // Retrieve Ammo Dividend

	multiplyDamage *= ammoMult;                             // Damage Multipler = Critical Multipler * Ammo Dividend

	long ammoDT = fo::func::item_w_dr_adjust(ctd.weapon);   // Retrieve ammo DT (well, it's really Retrieve ammo DR, but since we're treating ammo DR as ammo DT...)

	long calcDT = armorDT - ammoDT;                         // DT = armor DT - ammo DT
	long _calcDT = calcDT;

	if (calcDT >= 0) {                                      // Is DT >= 0?
		_calcDT = 0;                                        // If yes, set DT = 0
	} else {
		_calcDT *= 10;                                      // Otherwise, DT = DT * 10 (note that this should be a negative value)
		calcDT = 0;
	}

	long calcDR = armorDR + _calcDT;                        // DR = armor DR + DT (note that DT should be less than or equal to zero)
	if (calcDR < 0) {                                       // Is DR >= 0?
		calcDR = 0;                                         // If no, set DR = 0
	} else if (calcDR >= 100) {                             // Is DR >= 100?
		return;                                             // If yes, damage will be zero, so stop calculating
	}

	// Start of damage calculation loop
	for (long i = 0; i < rounds; i++) {
		long rawDamage = fo::func::item_w_damage(ctd.attacker, ctd.hitMode); // Retrieve Raw Damage
		rawDamage += bonusRangedDamage;                     // Raw Damage = Raw Damage + Bonus Ranged Damage

		rawDamage -= calcDT;                                // Raw Damage = Raw Damage - DT
		if (rawDamage <= 0) continue;                       // Is Raw Damage <= 0? If yes, skip damage calculation and go to bottom of loop

		rawDamage *= multiplyDamage;                        // Raw Damage = Raw Damage * Damage Multiplier
		if (ammoDiv != 0) {                                 // avoid divide by zero error
			rawDamage /= ammoDiv;                           // Raw Damage = Raw Damage / Ammo Divisor
		}
		rawDamage /= 2;                                     // Raw Damage = Raw Damage / 2 (related to critical hit damage multiplier bonus)
		rawDamage *= difficulty;                            // Raw Damage = Raw Damage * combat difficulty setting (75 if wimpy, 100 if normal or if attacker is player, 125 if rough)
		rawDamage /= 100;                                   // Raw Damage = Raw Damage / 100

		long resistedDamage = calcDR * rawDamage;           // Otherwise, Resisted Damage = DR * Raw Damage
		resistedDamage /= 100;                              // Resisted Damage = Resisted Damage / 100
		rawDamage -= resistedDamage;                        // Raw Damage = Raw Damage - Resisted Damage

		if (rawDamage > 0) accumulatedDamage += rawDamage;  // Accumulated Damage = Accumulated Damage + Raw Damage
	}
}

static __declspec(naked) void AmmoInfoPrintYAAM() {
	__asm {
		lea  ecx, ammoInfoFmt;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

// Display melee damage w/o PERK_bonus_hth_damage bonus
static __declspec(naked) void MeleeDmgDisplayPrintFix_hook() {
	using namespace fo;
	__asm {
		mov  ecx, eax;                                 // Store pointer to critter
		call fo::funcoffs::stat_level_;                // Get Melee Damage
		xchg ecx, eax;                                 // Store Melee Damage value
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		call fo::funcoffs::perk_level_;                // Get rank of Bonus HtH Damage
		shl  eax, 1;                                   // Multiply by 2
		sub  ecx, eax;                                 // Subtract from Melee Damage
		mov  edx, STAT_melee_dmg;
		mov  eax, ds:[FO_VAR_obj_dude];                // Get pointer to PC
		call fo::funcoffs::stat_get_base_;             // Get Melee Damage w/o bonuses
		cmp  ecx, eax;                                 // HtH Damage vs Base Melee Damage
		cmovg eax, ecx;                                // Move back to eax in preparation of push
		retn;
	}
}

// Display max melee damage w/o PERK_bonus_hth_damage bonus
static __declspec(naked) void CommonDmgRngDispFix_hook() {
	using namespace fo;
	__asm {
		mov  ebx, eax;                                 // Store pointer to critter
		call fo::funcoffs::stat_level_;                // Get Melee Damage
		xchg ebx, eax;                                 // Store Melee Damage value
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		call fo::funcoffs::perk_level_;                // Get rank of Bonus HtH Damage
		shl  eax, 1;                                   // Multiply by 2
		sub  ebx, eax;                                 // Subtract from Melee Damage
		mov  edx, STAT_melee_dmg;
		mov  eax, ds:[FO_VAR_stack];
		call fo::funcoffs::stat_get_base_;             // Get Melee Damage w/o bonuses
		cmp  ebx, eax;                                 // HtH Damage vs Base Melee Damage
		cmovg eax, ebx;                                // Move back to eax in preparation of push
		retn;
	}
}
/*
static __declspec(naked) void HtHDamageFix1a_hack() {
	using namespace fo;
	__asm {
		cmp  ecx, dword ptr ds:[FO_VAR_obj_dude];      // Is the critter == PC?
		je   fix;                                      // Skip if no
		mov  edx, 1;                                   // Min_Damage = 1
		retn;
fix:
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		mov  eax, ecx;                                 // pointer to PC
		call fo::funcoffs::perk_level_;                // Return Rank_of_Bonus_HtH_Damage_perk
		shl  eax, 1;                                   // Rank_of_Bonus_HtH_Damage_perk *= 2
		lea  edx, [eax + 1];                           // Min_Damage = 1 + Rank_of_Bonus_HtH_Damage_perk
		retn;
	}
}
*/
static __declspec(naked) void HtHDamageFix1b_hook() {
	using namespace fo;
	__asm {
		call fo::funcoffs::stat_level_;                // Get Total_Melee_Damage
		cmp  ecx, dword ptr ds:[FO_VAR_obj_dude];      // Is the critter == PC?
		je   fix;                                      // Skip to exit if no
		retn;
fix:
		push eax;
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		mov  eax, ecx;                                 // pointer to PC
		call fo::funcoffs::perk_level_;                // Return Rank_of_Bonus_HtH_Damage_perk
		shl  eax, 1;                                   // Rank_of_Bonus_HtH_Damage_perk *= 2
		add  dword ptr [esp + 0x24 - 0x20 + 8], eax;   // Min_Damage += Rank_of_Bonus_HtH_Damage_perk
		pop  eax;
		retn;
	}
}

static __declspec(naked) void DisplayBonusRangedDmg_hook() {
	using namespace fo;
	__asm {
		mov  edx, PERK_bonus_ranged_damage;
		mov  eax, dword ptr ds:[FO_VAR_stack];
		call fo::funcoffs::perk_level_;
		shl  eax, 1;                                   // Multiply by 2
		add  dword ptr [esp + 4 * 4], eax;             // min_dmg + perk bonus
		add  dword ptr [esp + 4 * 5], eax;             // max_dmg + perk bonus
		jmp  fo::funcoffs::sprintf_;
	}
}

static __declspec(naked) void DisplayBonusHtHDmg1_hook() {
	using namespace fo;
	__asm {
		mov  edx, PERK_bonus_hth_damage;
		mov  eax, dword ptr ds:[FO_VAR_stack];
		call fo::funcoffs::perk_level_;
		shl  eax, 1;                                   // Multiply by 2
		add  dword ptr [esp + 4 * 4], eax;             // min_dmg + perk bonus
		jmp  fo::funcoffs::sprintf_;
	}
}

static bool bonusHtHDamageFix = true;
static bool displayBonusDamage = false;

static long __fastcall GetHtHDamage(fo::GameObject* source, long &meleeDmg, long handOffset) {
	long min, max;

	fo::AttackType hit = Unarmed::GetStoredHitMode((handOffset == 0) ? fo::HandSlot::Left : fo::HandSlot::Right);
	long bonus = Unarmed::GetDamage(hit, min, max);
	meleeDmg += max + bonus;

	long perkBonus = game::Stats::perk_level(source, fo::Perk::PERK_bonus_hth_damage) << 1;
	if (!displayBonusDamage) meleeDmg -= perkBonus;
	if (displayBonusDamage && bonusHtHDamageFix) min += perkBonus;

	return min + bonus;
}

static const char* __fastcall GetHtHName(long handOffset) {
	fo::AttackType hit = Unarmed::GetStoredHitMode((handOffset == 0) ? fo::HandSlot::Left : fo::HandSlot::Right);
	return Unarmed::GetName(hit);
}

static __declspec(naked) void DisplayBonusHtHDmg2_hack() {
	static const DWORD DisplayBonusHtHDmg2Exit = 0x47254F;
	static const DWORD DisplayBonusHtHDmg2Exit2 = 0x472556;
	__asm {
		mov  ecx, eax;
		call fo::funcoffs::stat_level_; // get STAT_melee_dmg
		push eax;                       // max dmg (meleeDmg)
		mov  edx, esp;                  // meleeDmg ref
		push edi;                       // handOffset
		call GetHtHDamage;
		push eax;                       // min dmg
		mov  ecx, edi;
		call GetHtHName;
		test eax, eax;
		jnz  customName;
		jmp  DisplayBonusHtHDmg2Exit;
customName:
		jmp  DisplayBonusHtHDmg2Exit2;
	}
}

long DamageMod::GetHtHMinDamageBonus(fo::GameObject* source) {
	return (bonusHtHDamageFix)
	       ? game::Stats::perk_level(source, fo::Perk::PERK_bonus_hth_damage) << 1 // Multiply by 2
	       : 0;
}

void DamageMod::init() {
	if (formula = IniReader::GetConfigInt("Misc", "DamageFormula", 0)) {
		switch (formula) {
		case 1:
		case 2:
			HookScripts::InjectingHook(HOOK_SUBCOMBATDAMAGE);
			MakeCall(0x49B54A, AmmoInfoPrintGlovz, 2); // Dmg Mod (obj_examine_func_)
			Translate::Get("sfall", "AmmoInfoGlovz", "Div: DR/%d, DT/%d", ammoInfoFmt, 32);
			break;
		case 5:
			HookScripts::InjectingHook(HOOK_SUBCOMBATDAMAGE);
			MakeCall(0x49B4EB, AmmoInfoPrintYAAM, 2); // DR Mod (obj_examine_func_)
			Translate::Get("sfall", "AmmoInfoYAAM", "DT Mod: %d", ammoInfoFmt, 32);
			break;
		default:
			formula = 0;
		}
	}

	bonusHtHDamageFix = IniReader::GetConfigInt("Misc", "BonusHtHDamageFix", 1) != 0;
	displayBonusDamage = IniReader::GetConfigInt("Misc", "DisplayBonusDamage", 0) != 0;

	if (bonusHtHDamageFix) {
		dlogr("Applying Bonus HtH Damage Perk fix.", DL_INIT);
		// Subtract damage from perk bonus (vanilla displaying)
		if (!displayBonusDamage) {
			HookCalls(MeleeDmgDisplayPrintFix_hook, {
				0x435C0C,                                     // DisplayFix (ListDrvdStats_)
				0x439921                                      // PrintFix   (Save_as_ASCII_)
			});
			HookCalls(CommonDmgRngDispFix_hook, {
				0x472266,                                     // MeleeWeap  (display_stats_)
				//0x472546                                    // Unarmed    (display_stats_)
			});
		}
		//MakeCall(0x478492, HtHDamageFix1a_hack);            // Unarmed    (item_w_damage_)
		HookCall(0x47854C, HtHDamageFix1b_hook);              // MeleeWeap  (item_w_damage_)
	}

	if (displayBonusDamage) {
		dlogr("Applying Display Bonus Damage patch.", DL_INIT);
		HookCall(0x4722DD, DisplayBonusRangedDmg_hook);       // display_stats_
		if (bonusHtHDamageFix) {
			HookCall(0x472309, DisplayBonusHtHDmg1_hook);     // MeleeWeap (display_stats_)
		}
	}

	// Display the actual damage values of unarmed attacks (display_stats_ hacks)
	MakeJump(0x472546, DisplayBonusHtHDmg2_hack);
	SafeWrite32(0x472558, 0x509EDC); // fmt: '%s %d-%d'
	SafeWrite8(0x472552, 0x98 + 4);
	SafeWrite8(0x47255F, 0x0C + 4);
	SafeWrite8(0x472568, 0x10 + 4);
}

}
