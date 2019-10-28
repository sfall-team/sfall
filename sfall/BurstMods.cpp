/*
 *    sfall
 *    Copyright (C) 2008-2014  The sfall team
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

#include "main.h"

#include "Logging.h"

static long compute_spray_center_mult;
static long compute_spray_center_div;
static long compute_spray_target_mult;
static long compute_spray_target_div;

static long __fastcall ComputeSpray(DWORD* roundsLeftOut, DWORD* roundsRightOut, DWORD totalRounds, DWORD* roundsCenterOut) {
	// roundsCenter = totalRounds * mult / div
	long result = totalRounds * compute_spray_center_mult;
	long roundsCenter = result / compute_spray_center_div;
	if (result % compute_spray_center_div) roundsCenter++; // if remainder then round up

	if (roundsCenter == 0) roundsCenter = 1;
	*roundsCenterOut = roundsCenter;

	long roundsLeft = (totalRounds - roundsCenter) / 2;
	*roundsLeftOut = roundsLeft;
	*roundsRightOut = totalRounds - roundsCenter - roundsLeft;

	// roundsMainTarget = roundsCenter * mult / div
	result = roundsCenter * compute_spray_target_mult;
	long roundsMainTarget = result / compute_spray_target_div;

	return (result % compute_spray_target_div) ? ++roundsMainTarget : roundsMainTarget; // if remainder then round up
}

static const DWORD compute_spray_rounds_back = 0x42353A;
static void __declspec(naked) compute_spray_rounds_distribution() {
	__asm {
		push ecx;
		lea  ecx, [esp + 8 + 4];  // roundsLeft - out
		lea  edx, [esp + 4 + 4];  // roundsRight - out
		lea  eax, [esp + 16 + 4];
		push eax;                 // roundsCenter - out
		push ebp;                 // totalRounds
		call ComputeSpray;
		pop  ecx;
		mov  ebp, eax;
		mov  ebx, [esp + 16];
		jmp  compute_spray_rounds_back; // at this point, eax should contain the same value as ebp (roundsMainTarget); ebx should contain value of roundsCenter
	}
}


void BurstModsInit() {
	if (GetConfigInt("Misc", "ComputeSprayMod", 0)) {
		dlog("Applying ComputeSpray changes.", DL_INIT);
		compute_spray_center_mult = GetConfigInt("Misc", "ComputeSpray_CenterMult", 1);
		compute_spray_center_div  = GetConfigInt("Misc", "ComputeSpray_CenterDiv", 3);
		if (compute_spray_center_div < 1) {
			compute_spray_center_div = 1;
		}
		if (compute_spray_center_mult > compute_spray_center_div) {
			compute_spray_center_mult = compute_spray_center_div;
		}
		compute_spray_target_mult = GetConfigInt("Misc", "ComputeSpray_TargetMult", 1);
		compute_spray_target_div  = GetConfigInt("Misc", "ComputeSpray_TargetDiv", 2);
		if (compute_spray_target_div < 1) {
			compute_spray_target_div = 1;
		}
		if (compute_spray_target_mult > compute_spray_target_div) {
			compute_spray_target_mult = compute_spray_target_div;
		}
		MakeJump(0x4234F1, compute_spray_rounds_distribution);
		dlogr(" Done", DL_INIT);
	}
}