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

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "..\Logging.h"
#include "..\Translate.h"
#include "Unarmed.h"

#include "..\Game\stats.h"

#include "DamageMod.h"

namespace sfall
{

static int formula;

static char ammoInfoFmt[32];

static const DWORD DamageFunctionReturn = 0x424A63;

// Damage Fix v5 (with v5.1 Damage Multiplier tweak) by Glovz 2014.04.16.xx.xx
static __declspec(naked) void DamageGlovz() {
	static long ammoY, ammoX, calcDT, calcDR;
	__asm {
		mov  ebx, dword ptr ss:[esp + 0x1C];  // get the number of hits
		xor  ecx, ecx;                        // set the loop counter to 0
		test ebx, ebx;                        // compare the number of hits to 0
		jle  end;                             // exit
		mov  dword ptr ss:[esp + 0x30], ecx;  // clear value
		mov  eax, dword ptr ds:[esi + 0x8];   // get pointer to critter's weapon
		call fo::funcoffs::item_w_dam_div_;   // get the ammoY value
		test eax, eax;                        // compare the ammoY value to 0
		jg   getAmX;                          // if the ammoY value is greater than 0 then goto getAmX
		mov  eax, 1;                          // set the ammoY value to 1
getAmX:
		mov  ammoY, eax;                      // store the ammoY value
		mov  eax, dword ptr ds:[esi + 0x8];   // get pointer to critter's weapon
		call fo::funcoffs::item_w_dam_mult_;  // get the ammoX value
		test eax, eax;                        // compare the ammoX value to 0
		jg   getAmDRM;                        // if the ammoX value is greater than 0 then goto getAmDRM
		mov  eax, 1;                          // set the ammoX value to 1
getAmDRM:
		mov  ammoX, eax;                      // store the ammoX value
		mov  eax, dword ptr ds:[esi + 0x8];   // get pointer to critter's weapon
		call fo::funcoffs::item_w_dr_adjust_; // get the ammoDRM value
		test eax, eax;                        // compare the ammoDRM value to 0
		jle  calcADT;                         // if the ammoDRM value is less than or equal to 0 then goto calcADT
		neg  eax;                             // subtract ammoDRM value from 0
calcADT:
		mov  ebx, eax;                        // store the ammoDRM value to temp (available before "begin" label)
		mov  edx, dword ptr ss:[esp + 0x28];  // get the armorDT value
		test edx, edx;                        // compare the armorDT value to 0
		jle  calcADR;                         // if the armorDT value is less than or equal to 0 then goto calcADR
		mov  eax, ammoY;                      // get the ammoY value
aJmp:
		xor  ebp, ebp;                        // clear value
		cmp  edx, eax;                        // compare the dividend value to the divisor value
		jl   lrThan;                          // the dividend is less than the divisor then goto lrThan
		jg   grThan;                          // the dividend is greater than the divisor then goto grThan
		mov  eax, 1;                          // if the two values are equal then set the quotient value to 1
		jmp  endDiv;                          // goto endDiv
lrThan:
		shl  edx, 1;                          // multiply dividend value by 2
		cmp  edx, eax;                        // compare the dividend value to the divisor value
		jle  setZero;                         // if the dividend is less than or equal to the divisor then goto setZero
		mov  eax, 1;                          // if the dividend is greater than the divisor then set the quotient value to 1
		jmp  endDiv;                          // goto endDiv
setZero:
		xor  eax, eax;                        // clear value
		jmp  endDiv;                          // goto endDiv
grThan:
		mov  ebp, eax;                        // assign the divisor value
		mov  eax, edx;                        // assign the dividend value
		cdq;                                  // prepare for signed division
		idiv ebp;                             // perform signed integer division
		test edx, edx;                        // compare the remainder value to 0
		jz   endDiv;                          // if the remainder value is equal to 0 then goto endDiv
		shl  edx, 1;                          // multiply the remainder value by 2
		cmp  edx, ebp;                        // compare the remainder value to the divisor value
		jl   endDiv;                          // if the remainder is less than the divisor then goto endDiv
		jg   addOne;                          // if the remainder is greater than the divisor then goto addOne
		test al, 1;                           // check if the quotient value is even or odd
		jz   endDiv;                          // if the quotient value is even goto endDiv
addOne:
		inc  eax;                             // add 1 to the quotient value
endDiv:
		cmp  dword ptr ss:[esp + 0x30], 2;    // compare value to 2
		je   divTwo;                          // goto divTwo
		cmp  dword ptr ss:[esp + 0x30], 3;    // compare value to 3
		je   divThree;                        // goto divThree
		cmp  dword ptr ss:[esp + 0x30], 4;    // compare value to 4
		je   divFour;                         // goto divFour
		cmp  dword ptr ss:[esp + 0x30], 5;    // compare value to 5
		je   divFive;                         // goto divFive
		cmp  dword ptr ss:[esp + 0x30], 6;    // compare value to 6
		je   divSix;                          // goto divSix
		cmp  dword ptr ss:[esp + 0x30], 7;    // compare value to 7 (added for v5.1 tweak)
		je   divSeven;                        // goto divSeven
		mov  edx, eax;                        // set the new armorDT value
calcADR:
		mov  calcDT, edx;                     // store the armorDT value
		mov  edx, dword ptr ss:[esp + 0x2C];  // get the armorDR value
		mov  calcDR, edx;                     // store the armorDR value
		test edx, edx;                        // compare the armorDR value to 0
		jle  begin;                           // if the armorDR value is less than or equal to 0 then goto begin
		mov  eax, dword ptr ss:[esp + 0x20];  // get the Combat Difficulty (CD) value
		cmp  eax, 100;                        // compare the CD value to 100
		jg   sdrJmp;                          // if the CD value is greater than 100 then goto sdrJmp
		je   drDivJmp;                        // if the CD value is equal to 100 then goto drDivJmp
		add  edx, 20;                         // add 20 to the armorDR value
		jmp  drDivJmp;                        // goto drDivJmp
sdrJmp:
		sub  edx, 20;                         // subtract 20 from the armorDR value
drDivJmp:
		add  edx, ebx;                        // add the ammoDRM value to the armorDR value
		mov  eax, ammoX;                      // get the ammoX value
		mov  dword ptr ss:[esp + 0x30], 2;    // set value to 2
		jmp  aJmp;                            // goto aJmp
divTwo:
		mov  calcDR, eax;                     // store the new armorDR value
		cmp  eax, 100;                        // compare the armorDR value to 100
		jge  end;                             // if the armorDR value is greater than or equal to 100 then exit
begin:
		mov  edx, dword ptr ds:[esi + 0x4];   // get the hit mode of weapon being used by the attacker
		mov  eax, dword ptr ds:[esi];         // get pointer to critter attacking
		call fo::funcoffs::item_w_damage_;    // get the raw damage value
		add  eax, dword ptr ss:[esp + 0x18];  // add the bonus ranged damage value to the RD value
		test eax, eax;                        // compare the new damage value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		mov  ebx, eax;                        // set the ND value
		mov  edx, dword ptr ss:[esp + 0x28];  // get the armorDT value
		test edx, edx;                        // compare the armorDT value to 0
		jle  bJmp;                            // if the armorDT value is less than or equal to 0 then goto bJmp
		sub  ebx, calcDT;                     // subtract the new armorDT value from the ND value
		test ebx, ebx;                        // compare the ND value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		jmp  cJmp;                            // goto cJmp
bJmp:
		mov  edx, dword ptr ss:[esp + 0x2C];  // get the armorDR value
		test edx, edx;                        // compare the armorDR value to 0
		jle  dJmp;                            // if the armorDR value is less than or equal to 0 then goto dJmp
		jmp  cAltJmp;                         // goto cAltJmp (skip duplicate instructions)
cJmp:
		mov  edx, dword ptr ss:[esp + 0x2C];  // get the armorDR value
		test edx, edx;                        // compare the armorDR value to 0
		jle  eJmp;                            // if the armorDR value is less than or equal to 0 then goto eJmp
cAltJmp:
		mov  edx, ebx;                        // set temp value
		imul edx, calcDR;                     // multiply the ND value by the armorDR value
		mov  eax, 100;                        // set divisor value to 100
		mov  dword ptr ss:[esp + 0x30], 3;    // set value to 3
		jmp  aJmp;                            // goto aJmp
divThree:
		sub  ebx, eax;                        // subtract the damage resisted value from the ND value
		test ebx, ebx;                        // compare the ND value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		jmp  eJmp;                            // goto eJmp
dJmp:
		mov  eax, ammoX;                      // get the ammoX value
		cmp  eax, 1;                          // compare the ammoX value to 1
		jle  bSubDJmp;                        // if the ammoX value is less than or equal to 1 then goto bSubDJmp;
		mov  eax, ammoY;                      // get the ammoY value
		cmp  eax, 1;                          // compare the ammoY value to 1
		jle  aSubDJmp;                        // if the ammoY value is less than or equal to 1 then goto aSubDJmp
		mov  edx, ebx;                        // set temp value
		imul edx, 15;                         // multiply the ND value by 15
		mov  eax, 100;                        // set divisor value to 100
		mov  dword ptr ss:[esp + 0x30], 4;    // set value to 4
		jmp  aJmp;                            // goto aJmp
divFour:
		add  ebx, eax;                        // add the quotient value to the ND value
		jmp  eJmp;                            // goto eJmp
aSubDJmp:
		mov  edx, ebx;                        // set temp value
		imul edx, 20;                         // multiply the ND value by 20
		mov  eax, 100;                        // set divisor value to 100
		mov  dword ptr ss:[esp + 0x30], 5;    // set value to 5
		jmp  aJmp;                            // goto aJmp
divFive:
		add  ebx, eax;                        // add the quotient value to the ND value
		jmp  eJmp;                            // goto eJmp
bSubDJmp:
		mov  eax, ammoY;                      // get the ammoY value
		cmp  eax, 1;                          // compare the ammoY value to 1
		jle  eJmp;                            // goto eJmp
		mov  edx, ebx;                        // set temp value
		imul edx, 10;                         // multiply the ND value by 10
		mov  eax, 100;                        // set divisor value to 100
		mov  dword ptr ss:[esp + 0x30], 6;    // set value to 6
		jmp  aJmp;                            // goto aJmp
divSix:
		add  ebx, eax;                        // add the quotient value to the ND value
eJmp:
		mov  eax, dword ptr ss:[esp + 0x24];  // get the Critical Multiplier (CM) value
		cmp  eax, 2;                          // compare the CM value to 2
		jle  addNDJmp;                        // if the CM value is less than or equal to 2 then goto addNDJmp
		cmp  formula, 2;                      // check selected damage formula (added for v5.1 tweak)
		je   tweak;
		imul ebx, eax;                        // multiply the ND value by the CM value
		sar  ebx, 1;                          // divide the result by 2
		jmp  addNDJmp;
//////// begin v5.1 tweak ////////
tweak:
		mov  edx, ebx;                        // set temp ND value
		imul edx, eax;                        // multiply the temp ND value by the CM value
		imul edx, 25;                         // multiply the temp ND value by 25
		mov  eax, 100;                        // set divisor value to 100
		mov  dword ptr ss:[esp + 0x30], 7;    // set value to 7
		jmp  aJmp;                            // goto aJmp
divSeven:
		add  ebx, eax;                        // add the critical damage value to the ND value
//////// end v5.1 tweak //////////
addNDJmp:
		test ebx, ebx;                        // compare the ND value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		add  dword ptr ds:[edi], ebx;         // accumulate damage
noDamageJmp:
		mov  eax, dword ptr ss:[esp + 0x1C];  // get the number of hits
		inc  ecx;                             // the counter value increments by one
		cmp  ecx, eax;                        // is the counter value less than the number of hits
		jl   begin;                           // goto begin
end:
		jmp  DamageFunctionReturn;            // exit
	}
}

static __declspec(naked) void AmmoInfoPrintGlovz() {
	__asm {
		lea  edi, ammoInfoFmt;
		retn;
	}
}

// YAAM v1.1a by Haenlomal 2010.05.13
static __declspec(naked) void DamageYAAM() {
	static long calcDT, calcDR;
	__asm {
		mov  ebx, dword ptr ss:[esp + 0x1C];  // Get number of hits
		xor  ecx, ecx;                        // Set loop counter to zero
		test ebx, ebx;                        // Is number of hits <= 0?
		jle  end;                             // If yes, jump beyond damage calculation loop
		mov  eax, dword ptr ds:[esi + 0x8];   // Get pointer to critter's weapon
		mov  edx, dword ptr ss:[esp + 0x24];  // Get Critical Multiplier (passed in as argument to function)
		call fo::funcoffs::item_w_dam_mult_;  // Retrieve Ammo Dividend
		imul edx, eax;                        // Damage Multipler = Critical Multipler * Ammo Dividend
		mov  dword ptr ss:[esp + 0x24], edx;  // Store Damage Multiplier
		mov  eax, dword ptr ds:[esi + 0x8];   // Get pointer to critter's weapon
		call fo::funcoffs::item_w_dam_div_;   // Retrieve Ammo Divisor
		imul ebp, eax;                        // Ammo Divisor = 1 * Ammo Divisor (ebp set to 1 earlier in function)
		mov  eax, dword ptr ds:[esi + 0x8];   // Get pointer to critter's weapon
		call fo::funcoffs::item_w_dr_adjust_; // Retrieve ammo DT (well, it's really Retrieve ammo DR, but since we're treating ammo DR as ammo DT...)
		mov  edx, dword ptr ss:[esp + 0x28];  // Get armor DT
		sub  edx, eax;                        // DT = armor DT - ammo DT
		test edx, edx;                        // Is DT >= 0?
		jns  ajmp;                            // If yes, skip next instructions
		mov  eax, 10;
		imul eax, edx;                        // Otherwise, DT = DT * 10 (note that this should be a negative value)
		mov  calcDT, 0;                       // Store new DT (zero)
		jmp  bjmp;
ajmp:
		mov  calcDT, edx;                     // Store calculated DT
		xor  eax, eax;                        // Set DT to zero for DR calculation
bjmp:
		mov  edx, dword ptr ss:[esp + 0x2C];  // Get armor DR
		add  edx, eax;                        // DR = armor DR + DT (note that DT should be less than or equal to zero)
		test edx, edx;                        // Is DR >= 0?
		jns  cjmp;
		mov  calcDR, 0;                       // If no, set DR = 0
		jmp  djmp;
cjmp:
		cmp  edx, 100;                        // Is DR >= 100?
		jge  end;                             // If yes, damage will be zero, so stop calculating
		mov  calcDR, edx;                     // Store DR
djmp:                                         // Start of damage calculation loop
		mov  edx, dword ptr ds:[esi + 0x4];   // Get hit mode of weapon being used by attacker
		mov  eax, dword ptr ds:[esi];         // Get pointer to critter
		mov  ebx, dword ptr ss:[esp + 0x18];  // Get Bonus Ranged Damage
		call fo::funcoffs::item_w_damage_;    // Retrieve Raw Damage
		add  ebx, eax;                        // Raw Damage = Raw Damage + Bonus Ranged Damage
		sub  ebx, calcDT;                     // Raw Damage = Raw Damage - DT
		test ebx, ebx;                        // Is Raw Damage <= 0?
		jle  fjmp;                            // If yes, skip damage calculation and go to bottom of loop
		imul ebx, dword ptr ss:[esp + 0x24];  // Otherwise, Raw Damage = Raw Damage * Damage Multiplier
		test ebp, ebp;                        // Is Ammo Divisor == 0?
		jz   ejmp;                            // If yes, avoid divide by zero error
		mov  eax, ebx;
		cdq;
		idiv ebp;
		mov  ebx, eax;                        // Otherwise, Raw Damage = Raw Damage / Ammo Divisor
ejmp:
		mov  eax, ebx;
		cdq;
		sub  eax, edx;
		sar  eax, 1;                          // Raw Damage = Raw Damage / 2 (related to critical hit damage multiplier bonus)
		mov  edx, dword ptr ss:[esp + 0x20];  // Get combat difficulty setting (75 if wimpy, 100 if normal or if attacker is player, 125 if rough)
		imul edx, eax;                        // Raw Damage = Raw Damage * combat difficulty setting
		mov  ebx, 100;
		mov  eax, edx;
		cdq;
		idiv ebx;
		mov  ebx, eax;                        // Raw Damage = Raw Damage / 100
		mov  edx, calcDR;                     // Get calculated DR (note that this should be a nonnegative integer less than 100)
		imul edx, ebx;                        // Otherwise, Resisted Damage = DR * Raw Damage
		mov  dword ptr ss:[esp + 0x30], 100;
		mov  eax, edx;
		cdq;
		idiv dword ptr ss:[esp + 0x30];       // Resisted Damage = Resisted Damage / 100
		sub  ebx, eax;                        // Raw Damage = Raw Damage - Resisted Damage
		test ebx, ebx;                        // Is Raw Damage <= 0?
		jle  fjmp;                            // If yes, don't accumulate damage
		add  dword ptr ds:[edi], ebx;         // Otherwise, Accumulated Damage = Accumulated Damage + Raw Damage
fjmp:
		mov  eax, dword ptr ss:[esp + 0x1C];  // Get number of hits
		inc  ecx;                             // counter += 1
		cmp  ecx, eax;                        // Is counter < number of hits?
		jl   djmp;                            // If yes, go back to start of damage calcuation loop (calculate damage for next hit)
end:
		jmp  DamageFunctionReturn;            // Otherwise, exit loop
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

static void __declspec(naked) DisplayBonusRangedDmg_hook() {
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

static void __declspec(naked) DisplayBonusHtHDmg1_hook() {
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

static void __declspec(naked) DisplayBonusHtHDmg2_hack() {
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
			MakeJump(0x424995, DamageGlovz, 2);
			MakeCall(0x49B54A, AmmoInfoPrintGlovz, 2); // Dmg Mod (obj_examine_func_)
			Translate::Get("sfall", "AmmoInfoGlovz", "Div: DR/%d, DT/%d", ammoInfoFmt, 32);
			break;
		case 5:
			MakeJump(0x424995, DamageYAAM, 2);
			MakeCall(0x49B4EB, AmmoInfoPrintYAAM, 2); // DR Mod (obj_examine_func_)
			Translate::Get("sfall", "AmmoInfoYAAM", "DT Mod: %d", ammoInfoFmt, 32);
			break;
		}
	}

	bonusHtHDamageFix = IniReader::GetConfigInt("Misc", "BonusHtHDamageFix", 1) != 0;
	displayBonusDamage = IniReader::GetConfigInt("Misc", "DisplayBonusDamage", 0) != 0;

	if (bonusHtHDamageFix) {
		dlogr("Applying Bonus HtH Damage Perk fix.", DL_INIT);
		// Subtract damage from perk bonus (vanilla displaying)
		if (!displayBonusDamage) {
			const DWORD meleeDmgDispPrtAddr[] = {
				0x435C0C,                                     // DisplayFix (ListDrvdStats_)
				0x439921                                      // PrintFix   (Save_as_ASCII_)
			};
			HookCalls(MeleeDmgDisplayPrintFix_hook, meleeDmgDispPrtAddr);
			const DWORD commonDmgRngDispAddr[] = {
				0x472266,                                     // MeleeWeap  (display_stats_)
				//0x472546                                    // Unarmed    (display_stats_)
			};
			HookCalls(CommonDmgRngDispFix_hook, commonDmgRngDispAddr);
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
