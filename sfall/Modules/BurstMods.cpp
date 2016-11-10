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

#include "..\main.h"

#include "..\Logging.h"

static DWORD compute_spray_center_mult;
static DWORD compute_spray_center_div;
static DWORD compute_spray_target_mult;
static DWORD compute_spray_target_div;

static const DWORD compute_spray_rounds_back = 0x42353A;
static void __declspec(naked) compute_spray_rounds_distribution() {
	__asm {
		// ebp - totalRounds
		mov     eax, ebp; // roundsCenter = totalRounds * mult / div
		mov     ebx, compute_spray_center_mult;
		imul    ebx; // multiply eax by ebx and store result in edx:eax
		mov     ebx, compute_spray_center_div; // divisor
		idiv    ebx; // divide edx:eax by ebx and store result in eax (edx)
		test    edx, edx; // if remainder (edx) is not 0
		jz      divEnd1;
		inc     eax; // round up
divEnd1:
		mov     [esp+16], eax; // roundsCenter
		test    eax, eax; // if (roundsCenter == 0)
		jnz     loc_42350F;
		mov     [esp+16], 1; // roundsCenter = 1;
loc_42350F:
		mov     eax, ebp; // roundsLeft = (totalRounds - roundsCenter) / 2
		sub     eax, [esp+16]; // - roundsCenter
		sar     eax, 1; // /2
		mov     [esp+8], eax; // roundsLeft
		mov     eax, ebp; // roundsRight = totalRounds - roundsCenter - roundsLeft
		sub     eax, [esp+16];
		sub     eax, [esp+8];
		mov     [esp+4], eax; // roundsRight
		mov     eax, [esp+16]; // roundsMainTarget = roundsCenter * mult / div
		mov     ebx, compute_spray_target_mult;
		imul    ebx;
		mov     ebx, compute_spray_target_div;
		idiv    ebx;
		test    edx, edx; // if remainder (edx) is not 0
		jz      divEnd2;
		inc     eax; // round up
divEnd2:
		mov     ebp, eax;
		mov     ebx, [esp+16];
		// at this point, eax should contain the same value as ebp (roundsMainTarget); ebx should contain value of roundsCenter
		jmp     compute_spray_rounds_back;
	}
}


void ComputeSprayModInit() {
	if (GetPrivateProfileIntA("Misc", "ComputeSprayMod", 0, ini)) {
		dlog("Applying ComputeSpray changes.", DL_INIT);
		compute_spray_center_mult = GetPrivateProfileIntA("Misc", "ComputeSpray_CenterMult", 1, ini);
		compute_spray_center_div  = GetPrivateProfileIntA("Misc", "ComputeSpray_CenterDiv", 3, ini);
		if (compute_spray_center_div < 1) {
			compute_spray_center_div = 1;
		}
		if (compute_spray_center_mult > compute_spray_center_div) {
			compute_spray_center_mult = compute_spray_center_div;
		}
		compute_spray_target_mult = GetPrivateProfileIntA("Misc", "ComputeSpray_TargetMult", 1, ini);
		compute_spray_target_div  = GetPrivateProfileIntA("Misc", "ComputeSpray_TargetDiv", 2, ini);
		if (compute_spray_target_div < 1) {
			compute_spray_target_div = 1;
		}
		if (compute_spray_target_mult > compute_spray_target_div) {
			compute_spray_target_mult = compute_spray_target_div;
		}
		MakeCall(0x4234F1, &compute_spray_rounds_distribution, true);
		dlogr(" Done", DL_INIT);
	}
}
