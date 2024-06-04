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

#include "BurstMods.h"
#include "MainLoopHook.h"

namespace sfall
{

static long compute_spray_center_mult;
static long compute_spray_center_div;
static long compute_spray_target_mult;
static long compute_spray_target_div;

// default values
static long compute_spray_center_mult_def;
static long compute_spray_center_div_def;
static long compute_spray_target_mult_def;
static long compute_spray_target_div_def;

static bool computeSpraySettingsReset = false;

static long __fastcall ComputeSpray(DWORD* roundsLeftOut, DWORD* roundsRightOut, DWORD totalRounds, DWORD* roundsCenterOut) {
	// roundsCenter = totalRounds * mult / div
	long result = totalRounds * compute_spray_center_mult;
	long roundsCenter = result / compute_spray_center_div;
	if (result % compute_spray_center_div) roundsCenter++; // if remainder then round up

	if (roundsCenter == 0) roundsCenter = 1;
	*roundsCenterOut = roundsCenter;

	long roundsLeft = (totalRounds - roundsCenter) / 2;         // minimum possible value is 0
	long roundsRight = totalRounds - roundsCenter - roundsLeft; // is either equal to or one more than roundsLeft
	if (roundsLeft != roundsRight && fo::func::roll_random(0, 1)) { // randomize the distribution of one extra bullet
		roundsLeft++;
		roundsRight--;
	}
	*roundsLeftOut = roundsLeft;
	*roundsRightOut = roundsRight;

	// roundsMainTarget = roundsCenter * mult / div
	result = roundsCenter * compute_spray_target_mult;
	long roundsMainTarget = result / compute_spray_target_div;

	return (result % compute_spray_target_div) ? ++roundsMainTarget : roundsMainTarget; // if remainder then round up
}

static __declspec(naked) void compute_spray_rounds_distribution() {
	static const DWORD compute_spray_rounds_back = 0x42353A;
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

void BurstMods::SetComputeSpraySettings(long centerMult, long centerDiv, long targetMult, long targetDiv) {
	compute_spray_center_mult = centerMult;
	compute_spray_center_div  = centerDiv;
	compute_spray_target_mult = targetMult;
	compute_spray_target_div  = targetDiv;
	computeSpraySettingsReset = true;
}

void ResetComputeSpraySettings() {
	if (!computeSpraySettingsReset) return;
	compute_spray_center_mult = compute_spray_center_mult_def;
	compute_spray_center_div  = compute_spray_center_div_def;
	compute_spray_target_mult = compute_spray_target_mult_def;
	compute_spray_target_div  = compute_spray_target_div_def;
	computeSpraySettingsReset = false;
}

void BurstMods::init() {
	//if (IniReader::GetConfigInt("Misc", "ComputeSprayMod", 1)) {
		dlogr("Applying ComputeSpray settings to burst attacks.", DL_INIT);
		compute_spray_center_mult = IniReader::GetConfigInt("Misc", "ComputeSpray_CenterMult", 1);
		compute_spray_center_div  = IniReader::GetConfigInt("Misc", "ComputeSpray_CenterDiv", 3);
		if (compute_spray_center_div < 1) {
			compute_spray_center_div = 1;
		}
		if (compute_spray_center_mult < 1) {
			compute_spray_center_mult = 1;
		} else if (compute_spray_center_mult > compute_spray_center_div) {
			compute_spray_center_mult = compute_spray_center_div;
		}
		compute_spray_center_mult_def = compute_spray_center_mult;
		compute_spray_center_div_def = compute_spray_center_div;

		compute_spray_target_mult = IniReader::GetConfigInt("Misc", "ComputeSpray_TargetMult", 1);
		compute_spray_target_div  = IniReader::GetConfigInt("Misc", "ComputeSpray_TargetDiv", 2);
		if (compute_spray_target_div < 1) {
			compute_spray_target_div = 1;
		}
		if (compute_spray_target_mult < 1) {
			compute_spray_target_mult = 1;
		} else if (compute_spray_target_mult > compute_spray_target_div) {
			compute_spray_target_mult = compute_spray_target_div;
		}
		compute_spray_target_mult_def = compute_spray_target_mult;
		compute_spray_target_div_def = compute_spray_target_div;

		MakeJump(0x4234F1, compute_spray_rounds_distribution);
		// after each combat attack, reset ComputeSpray settings
		MainLoopHook::OnAfterCombatAttack() += ResetComputeSpraySettings;
	//}
}

}
