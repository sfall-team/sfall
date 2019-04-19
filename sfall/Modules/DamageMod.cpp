/*
*    sfall
*    Copyright (C) 2008, 2009, 2010, 2013, 2014  The sfall team
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
#include "HookScripts.h"

#include "DamageMod.h"

namespace sfall
{
using namespace fo;

int DamageMod::formula;

// Damage Fix v5 (with v5.1 Damage Multiplier tweak) by Glovz 2014.04.16.xx.xx
// TODO: rewrite in C++
void DamageMod::DamageGlovz(fo::ComputeAttackResult &ctd, DWORD* accumulatedDamage, int rounds, int armorDT, int armorDR, int bonusRangedDamage, int multiplyDamage, int difficulty) {
	if (rounds <= 0) return;

	int ammoY   = fo::func::item_w_dam_div(ctd.weapon);   // ammoY value (divisor)
	int ammoX   = fo::func::item_w_dam_mult(ctd.weapon);  // ammoX value
	int ammoDRM = fo::func::item_w_dr_adjust(ctd.weapon); // ammoDRM value
	int mValue;

	__asm {
		mov esi, ctd;
		mov edi, accumulatedDamage;
begin:
		mov  mValue, 0;                       // clear value
		mov  edx, dword ptr ds:[esi + 4];     // get the hit mode of weapon being used by an attacker
		mov  eax, dword ptr ds:[esi];         // get pointer to critter attacking
		call fo::funcoffs::item_w_damage_;    // get the raw damage value
		mov  ebx, bonusRangedDamage;          // get the bonus ranged damage value
		cmp  ebx, 0;                          // compare the range bonus damage value to 0
		jle  rdJmp;                           // if the RB value is less than or equal to 0 then goto rdJmp
		add  eax, ebx;                        // add the RB value to the RD value
rdJmp:
		cmp  eax, 0;                          // compare the new damage value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		mov  ebx, eax;                        // set the ND value
		mov  edx, armorDT;                    // get the armorDT value
		cmp  edx, 0;                          // compare the armorDT value to 0
		jle  bJmp;                            // if the armorDT value is less than or equal to 0 then goto bJmp
		mov  eax, dword ptr ammoY;            // get the ammoY value
		cmp  eax, 0;                          // compare the ammoY value to 0
		jg   aJmp;                            // if the ammoY value is greater than 0 then goto aJmp
		mov  eax, 1;                          // set the ammoY value to 1
aJmp:
		xor  ecx, ecx;                        // clear value (old ebp)
		cmp  edx, eax;                        // compare the dividend with the divisor
		jl   lrThan;                          // the dividend is less than the divisor then goto lrThan
		jg   grThan;                          // the dividend is greater than the divisor then goto grThan
		jmp  setOne;                          // if the two values are equal then goto setOne
lrThan:
		mov  ecx, edx;                        // store the dividend value temporarily
		imul edx, 2;                          // multiply dividend value by 2
		sub  edx, eax;                        // subtract divisor value from the dividend value
		cmp  edx, 0;                          // compare the result to 0
		jl   setZero;                         // if the result is less than 0 then goto setZero
		jg   setOne;                          // if the result is greater than 0 then goto setOne
		mov  edx, ecx;                        // restore dividend value
		and  edx, 1;                          // if true (1) then odd if false (0) then even
		jz   setZero;                         // if the result is equal to 0 then setZero
		jmp  setOne;                          // if the result is not 0 then goto setOne
grThan:
		mov  ecx, eax;                        // assign the divisor value
		xor  eax, eax;                        // clear value
bbbJmp:
		inc  eax;                             // add 1 to the quotient value
		sub  edx, ecx;                        // subtract the divisor value from the dividend value
		cmp  edx, ecx;                        // compare the remainder value to the divisor value
		jge  bbbJmp;                          // if the remainder value is greater or equal to the divisor value then goto bbbJmp
		jz   endDiv;                          // if the remainder value is equal to 0 then goto endDiv
		imul edx, 2;                          // multiply temp remainder value by 2
		sub  edx, ecx;                        // subtract the divisor value from the temp remainder
		cmp  edx, 0;                          // compare the result to 0
		jl   endDiv;                          // if the result is less than 0 then goto endDiv
		jg   addOne;                          // if the result is greater than 0 then goto addOne
		mov  ecx, eax;                        // assign the quotient value
		and  ecx, 1;                          // if true (1) then odd if false (0) then even
		jz   endDiv;                          // if the result is equal to zero goto endDiv
addOne:
		inc  eax;                             // add 1 to the quotient value
		jmp  endDiv;                          // goto endDiv
setOne:
		mov  eax, 1;                          // set the quotient value to 1
		jmp  endDiv;                          // goto endDiv
setZero:
		xor  eax, eax;                        // clear value
endDiv:
		cmp  mValue, 2;                       // compare value to 2
		je   divTwo;                          // goto divTwo
		cmp  mValue, 3;                       // compare value to 3
		je   divThree;                        // goto divThree
		cmp  mValue, 4;                       // compare value to 4
		je   divFour;                         // goto divFour
		cmp  mValue, 5;                       // compare value to 5
		je   divFive;                         // goto divFive
		cmp  mValue, 6;                       // compare value to 6
		je   divSix;                          // goto divSix
		cmp  mValue, 7;                       // compare value to 7 (added for v5.1 tweak)
		je   divSeven;                        // goto divSeven
		sub  ebx, eax;                        // subtract the new armorDT value from the RD value
		jmp  cJmp;                            // goto cJmp
bJmp:
		mov  edx, armorDR;                    // get the armorDR value
		cmp  edx, 0;                          // compare the armorDR value to 0
		jle  dJmp;                            // if the armorDR value is less than or equal to 0 then goto dJmp
cJmp:
		cmp  ebx, 0;                          // compare the new damage value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		mov  edx, armorDR;                    // get the armorDR value
		cmp  edx, 0;                          // compare the armorDR value to 0
		jle  eJmp;                            // if the armorDR value is less than or equal to 0 then goto eJmp
		mov  eax, difficulty;                 // get the CD value
		cmp  eax, 100;                        // compare the CD value to 100
		jg   sdrJmp;                          // if the CD value is greater than 100 then goto sdrJmp
		je   aSubCJmp;                        // if the CD value is equal to 100 then goto aSubCJmp
		add  edx, 20;                         // add 20 to the armorDR value
		jmp  aSubCJmp;                        // goto aSubCJmp
sdrJmp:
		sub  edx, 20;                         // subtract 20 from the armorDR value
aSubCJmp:
		mov  eax, dword ptr ammoDRM;          // get the ammoDRM value
		cmp  eax, 0;                          // compare the ammoDRM value to 0
		jl   adrJmp;                          // if the ammoDRM value is less than 0 then goto adrJmp
		je   bSubCJmp;                        // if the ammoDRM value is equal to 0 then goto bSubCJmp
		xor  ecx, ecx;                        // clear value
		sub  ecx, eax;                        // subtract ammoDRM value from 0
		mov  eax, ecx;                        // set new ammoDRM value
adrJmp:
		add  edx, eax;                        // add the ammoDRM value to the armorDR value
bSubCJmp:
		mov  eax, dword ptr ammoX;            // get the ammoX value
		cmp  eax, 0;                          // compare the ammoX value to 0
		jg   cSubCJmp;                        // if the ammoX value is greater than 0 then goto cSubCJmp;
		mov  eax, 1;                          // set the ammoX value to 1
cSubCJmp:
		mov  mValue, 2;                       // set value to 2
		jmp  aJmp;                            // goto aJmp
divTwo:
		mov  edx, ebx;                        // set temp value
		imul edx, eax;                        // multiply the ND value by the armorDR value
		mov  eax, 100;                        // set divisor value to 100
		mov  mValue, 3;                       // set value to 3
		jmp  aJmp;                            // goto aJmp
divThree:
		sub  ebx, eax;                        // subtract the damage resisted value from the ND value
		jmp  eJmp;                            // goto eJmp
dJmp:
		mov  eax, dword ptr ammoX;            // get the ammoX value
		cmp  eax, 1;                          // compare the ammoX value to 1
		jle  bSubDJmp;                        // if the ammoX value is less than or equal to 1 then goto bSubDJmp;
		mov  eax, dword ptr ammoY;            // get the ammoY value
		cmp  eax, 1;                          // compare the ammoY value to 1
		jle  aSubDJmp;                        // if the ammoY value is less than or equal to 1 then goto aSubDJmp
		mov  edx, ebx;                        // set temp value
		imul edx, 15;                         // multiply the ND value by 15
		mov  eax, 100;                        // set divisor value to 100
		mov  mValue, 4;                       // set value to 4
		jmp  aJmp;                            // goto aJmp
divFour:
		add  ebx, eax;                        // add the quotient value to the ND value
		jmp  eJmp;                            // goto eJmp
aSubDJmp:
		mov  edx, ebx;                        // set temp value
		imul edx, 20;                         // multiply the ND value by 20
		mov  eax, 100;                        // set divisor value to 100
		mov  mValue, 5;                       // set value to 5
		jmp  aJmp;                            // goto aJmp
divFive:
		add  ebx, eax;                        // add the quotient value to the ND value
		jmp  eJmp;                            // goto eJmp
bSubDJmp:
		mov  eax, dword ptr ammoY;            // get the ammoY value
		cmp  eax, 1;                          // compare the ammoY value to 1
		jle  eJmp;                            // goto eJmp
		mov  edx, ebx;                        // set temp value
		imul edx, 10;                         // multiply the ND value by 10
		mov  eax, 100;                        // set divisor value to 100
		mov  mValue, 6;                       // set value to 6
		jmp  aJmp;                            // goto aJmp
divSix:
		add  ebx, eax;                        // add the quotient value to the ND value
eJmp:
		cmp  ebx, 0;                          // compare the new damage value to 0
		jle  noDamageJmp;                     // goto noDamageJmp
		mov  eax, multiplyDamage;             // get the Critical Multiplier (CM) value
		cmp  eax, 2;                          // compare the CM value to 2
		jle  addNDJmp;                        // if the CM value is less than or equal to 2 then goto addNDJmp
		cmp  DamageMod::formula, 2;           // check selected damage formula (added for v5.1 tweak)
		jz   tweak;
		imul ebx, eax;                        // multiply the ND value by the CM value
		sar  ebx, 1;                          // divide the result by 2
		jmp  addNDJmp;
//// begin v5.1 tweak ////
tweak:
		mov  edx, ebx;                        // set temp ND value
		imul edx, eax;                        // multiply the temp ND value by the CM value
		imul edx, 25;                         // multiply the temp ND value by 25
		mov  eax, 100;                        // set divisor value to 100
		mov  mValue, 7;                       // set value to 7
		jmp  aJmp;                            // goto aJmp
divSeven:
		add  ebx, eax;                        // add the critical damage value to the ND value
////  end v5.1 tweak  ////
addNDJmp:
		add  dword ptr ds:[edi], ebx;         // accumulate damage
noDamageJmp:
		dec  dword ptr rounds;                // decrease the hit counter value by one
		jnz  begin;                           // compare the number of hits to 0
	}
}

// YAAM
void DamageMod::DamageYAAM(fo::ComputeAttackResult &ctd, DWORD* accumulatedDamage, int rounds, int armorDT, int armorDR, int bonusRangedDamage,int multiplyDamage, int difficulty) {
	int ammoDiv = fo::func::item_w_dam_div(ctd.weapon);     // Retrieve Ammo Divisor
	int ammoMult = fo::func::item_w_dam_mult(ctd.weapon);   // Retrieve Ammo Dividend

	multiplyDamage *= ammoMult;                             // Damage Multipler = Critical Multipler * Ammo Dividend
	int ammoDivisor = 1 * ammoDiv;                          // Ammo Divisor = 1 * Ammo Divisor

	int ammoDT = fo::func::item_w_dr_adjust(ctd.weapon);    // Retrieve ammo DT (well, it's really Retrieve ammo DR, but since we're treating ammo DR as ammo DT...)

	// Start of damage calculation loop
	for (int i = 0; i < rounds; i++) {                      // Check number of hits
		int rawDamage = fo::func::item_w_damage(ctd.attacker, ctd.hitMode); // Retrieve Raw Damage
		rawDamage += bonusRangedDamage;                     // Raw Damage = Raw Damage + Bonus Ranged Damage

		int calcDT = armorDT - ammoDT;                      // DT = armor DT - ammo DT
		if (calcDT < 0) calcDT = 0;

		rawDamage -= calcDT;                                // Raw Damage = Raw Damage - DT
		if (rawDamage <= 0) continue;                       // Is Raw Damage <= 0? If yes, skip damage calculation and go to bottom of loop

		rawDamage *= multiplyDamage;                        // Raw Damage = Raw Damage * Damage Multiplier
		if (ammoDivisor != 0) {                             // avoid divide by zero error
			rawDamage /= ammoDivisor;                       // Raw Damage = Raw Damage / Ammo Divisor
		}
		rawDamage /= 2;                                     // Raw Damage = Raw Damage / 2 (related to critical hit damage multiplier bonus)
		rawDamage *= difficulty;                            // Raw Damage = Raw Damage * combat difficulty setting (75 if wimpy, 100 if normal or if attacker is player, 125 if rough)
		rawDamage /= 100;                                   // Raw Damage = Raw Damage / 100

		calcDT = armorDT - ammoDT;                          // DT = armor DT - ammo DT
		if (calcDT >= 0) {                                  // Is DT >= 0?
			calcDT = 0;                                     // If yes, set DT = 0
		} else {
			calcDT *= 10;                                   // Otherwise, DT = DT * 10 (note that this should be a negative value)
		}

		int calcDR = armorDR + calcDT;                      // DR = armor DR + DT (note that DT should be less than or equal to zero)
		if (calcDR >= 100) {                                // Is DR >= 100?
			continue;                                       // If yes, damage will be zero, so stop calculating and go to bottom of loop
		} else if (calcDR < 0) {                            // Is DR >= 0?
			calcDR = 0;                                     // If no, set DR = 0
		}

		int resistedDamage =  calcDR * rawDamage;           // Otherwise, Resisted Damage = DR * Raw Damage
		resistedDamage /= 100;                              // Resisted Damage = Resisted Damage / 100
		rawDamage -= resistedDamage;                        // Raw Damage = Raw Damage - Resisted Damage

		if (rawDamage > 0) *accumulatedDamage += rawDamage; // Accumulated Damage = Accumulated Damage + Raw Damage
	}
}
////////////////////////////////////////////////////////////////////////////////

static __declspec(naked) void MeleeDmgDisplayPrintFix_hook() {
	__asm {
		call fo::funcoffs::stat_level_;                // Get Melee Damage
		mov  ecx, eax;                                 // Store value
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		mov  eax, dword ptr ds:[FO_VAR_obj_dude];      // Get pointer to PC
		call fo::funcoffs::perk_level_;                // Get rank of Bonus HtH Damage
		shl  eax, 1;                                   // Multiply by 2
		sub  ecx, eax;                                 // Subtract from Melee Damage
		mov  eax, ecx;                                 // Move back to eax in preparation of push
		retn;
	}
}

static __declspec(naked) void CommonDmgRngDispFix_hook() {
	__asm {
		mov  ebx, eax;                                 // Store pointer to critter
		call fo::funcoffs::stat_level_;                // Get Melee Damage
		xchg ebx, eax;                                 // Store Melee Damage value
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		call fo::funcoffs::perk_level_;                // Get rank of Bonus HtH Damage
		shl  eax, 1;                                   // Multiply by 2
		sub  ebx, eax;                                 // Subtract from Melee Damage
		mov  eax, ebx;                                 // Move back to eax in preparation of push
		retn;
	}
}

static __declspec(naked) void HtHDamageFix1a_hack() {
	__asm {
		xor  edx, edx;
		cmp  ecx, dword ptr ds:[FO_VAR_obj_dude];      // Is the critter == PC?
		jnz  skip;                                     // Skip if no
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		mov  eax, ecx;                                 // pointer to PC
		call fo::funcoffs::perk_level_;                // Return Rank_of_Bonus_HtH_Damage_perk
		shl  eax, 1;                                   // Rank_of_Bonus_HtH_Damage_perk *= 2
		mov  edx, eax;                                 // Min_Damage = Rank_of_Bonus_HtH_Damage_perk
skip:
		add  edx, 1;                                   // Min_Damage += 1
		retn;
	}
}

static __declspec(naked) void HtHDamageFix1b_hook() {
	__asm {
		call fo::funcoffs::stat_level_;                // Get Total_Melee_Damage
		cmp  ecx, dword ptr ds:[FO_VAR_obj_dude];      // Is the critter == PC?
		jnz  end;                                      // Skip to exit if no
		push eax;
		mov  edx, PERK_bonus_hth_damage;               // perk_level_ argument: PERK_bonus_hth_damage
		mov  eax, ecx;                                 // pointer to PC
		call fo::funcoffs::perk_level_;                // Return Rank_of_Bonus_HtH_Damage_perk
		shl  eax, 1;                                   // Rank_of_Bonus_HtH_Damage_perk *= 2
		add  dword ptr [esp + 0x24 - 0x20 + 8], eax;   // Min_Damage += Rank_of_Bonus_HtH_Damage_perk
		pop  eax;
end:
		retn;
	}
}

static void __declspec(naked) DisplayBonusRangedDmg_hook() {
	__asm {
		mov  edx, PERK_bonus_ranged_damage;
		mov  eax, dword ptr ds:[FO_VAR_stack];
		call fo::funcoffs::perk_level_;
		shl  eax, 1;
		add  dword ptr [esp + 4 * 4], eax;             // min_dmg + perk bonus
		add  dword ptr [esp + 4 * 5], eax;             // max_dmg + perk bonus
		jmp  fo::funcoffs::sprintf_;
	}
}

static void __declspec(naked) DisplayBonusHtHDmg1_hook() {
	__asm {
		mov  edx, PERK_bonus_hth_damage;
		mov  eax, dword ptr ds:[FO_VAR_stack];
		call fo::funcoffs::perk_level_;
		shl  eax, 1;
		add  dword ptr [esp + 4 * 4], eax;             // min_dmg + perk bonus
		jmp  fo::funcoffs::sprintf_;
	}
}

static const DWORD DisplayBonusHtHDmg2Exit = 0x472569;
static void __declspec(naked) DisplayBonusHtHDmg2_hack() {
	__asm {
		mov  ecx, eax;
		call fo::funcoffs::stat_level_;
		add  eax, 2;
		push eax;                                      // max dmg
		mov  edx, PERK_bonus_hth_damage;
		mov  eax, ecx;
		call fo::funcoffs::perk_level_;
		shl  eax, 1;
		add  eax, 1;
		push eax;                                      // min dmg + bonus
		mov  ecx, dword ptr[esp + 0x98 + 0x4];
		push ecx;                                      // message
		push 0x509EDC;                                 // '%s %d-%d'
		lea  eax, [esp + 0x0C + 0x4];
		push eax;                                      // buf
		call fo::funcoffs::sprintf_;
		add  esp, 0x10 + 0x4;
		jmp  DisplayBonusHtHDmg2Exit;
	}
}

void DamageMod::init() {
	if (formula = GetConfigInt("Misc", "DamageFormula", 0)) {
		switch (formula) {
		case 1:
		case 2:
		case 5:
			if (!HookScripts::IsInjectHook(HOOK_SUBCOMBATDAMAGE)) {
				HookScripts::InjectingHook(HOOK_SUBCOMBATDAMAGE);
			}
			break;
		default:
			formula = 0;
		}
	}

	int BonusHtHDmgFix = GetConfigInt("Misc", "BonusHtHDamageFix", 1);
	int DisplayBonusDmg = GetConfigInt("Misc", "DisplayBonusDamage", 0);
	if (BonusHtHDmgFix) {
		dlog("Applying Bonus HtH Damage Perk fix.", DL_INIT);
		if (!DisplayBonusDmg) {                               // Subtract damage from perk bonus (vanilla displaying)
			HookCalls(MeleeDmgDisplayPrintFix_hook, {
				0x435C0C,                                     // DisplayFix (ListDrvdStats_)
				0x439921                                      // PrintFix   (Save_as_ASCII_)
			});
			HookCalls(CommonDmgRngDispFix_hook, {
				0x472266,                                     // MeleeWeap  (display_stats_)
				0x472546                                      // Unarmed    (display_stats_)
			});
		}
		MakeCall(0x478492, HtHDamageFix1a_hack);              // Unarmed    (item_w_damage_)
		HookCall(0x47854C, HtHDamageFix1b_hook);              // MeleeWeap  (item_w_damage_)
		dlogr(" Done", DL_INIT);
	}

	if (DisplayBonusDmg) {
		dlog("Applying Display Bonus Damage patch.", DL_INIT);
		HookCall(0x4722DD, DisplayBonusRangedDmg_hook);       // display_stats_
		if (BonusHtHDmgFix) {
			HookCall(0x472309, DisplayBonusHtHDmg1_hook);     // display_stats_
			MakeJump(0x472546, DisplayBonusHtHDmg2_hack);     // display_stats_
		}
		dlogr(" Done", DL_INIT);
	}
}

}
