/*
*    sfall
*    Copyright (C) 2008-2017  The sfall team
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

#include <math.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"

#include "Worldmap.h"

namespace sfall
{

static Delegate<> onWorldmapLoop;
static DWORD ViewportX;
static DWORD ViewportY;

static bool restMode;
static bool restTime;

static __declspec(naked) void GetDateWrapper() {
	__asm {
		push ecx;
		push esi;
		push ebx;
		call fo::funcoffs::game_time_date_;
		mov ecx, ds:[FO_VAR_pc_proto + 0x4C];
		pop esi;
		test esi, esi;
		jz end;
		add ecx, [esi];
		mov[esi], ecx;
end:
		pop esi;
		pop ecx;
		retn;
	}
}

static void TimerReset() {
	fo::var::fallout_game_time = 0;
	// used as additional years indicator
	fo::var::pc_proto.critter.base.unarmedDamage += 13;
}

static int mapSlotsScrollMax = 27 * (17 - 7);
static __declspec(naked) void ScrollCityListHook() {
	__asm {
		push ebx;
		mov ebx, ds:[0x672F10];
		test eax, eax;
		jl up;
		cmp ebx, mapSlotsScrollMax;
		je end;
		jmp run;
up:
		test ebx, ebx;
		jz end;
run:
		call fo::funcoffs::wmInterfaceScrollTabsStart_;
end:
		pop ebx;
		retn;
	}
}

static void __stdcall WorldmapLoopHook() {
	onWorldmapLoop.invoke();
}

static DWORD worldMapDelay;
static void __declspec(naked) WorldMapFpsPatch() {
	__asm {
		pushad;
		call WorldmapLoopHook;
		mov ecx, worldMapDelay;
tck:
		mov eax, ds : [0x50fb08];
		call fo::funcoffs::elapsed_time_;
		cmp eax, ecx;
		jl tck;
		call fo::funcoffs::get_time_;
		mov ds : [0x50fb08], eax;
		popad;
		jmp fo::funcoffs::get_input_;
	}
}

//Only used if the world map speed patch is disabled, so that world map scripts are still run
static void __declspec(naked) WorldMapHook() {
	__asm {
		pushad;
		call WorldmapLoopHook;
		popad;
		jmp fo::funcoffs::get_input_;
	}
}

static void __declspec(naked) WorldMapEncPatch1() {
	__asm {
		inc dword ptr ds : [FO_VAR_wmLastRndTime]
		call fo::funcoffs::wmPartyWalkingStep_;
		retn;
	}
}

static void __declspec(naked) WorldMapEncPatch2() {
	__asm {
		mov dword ptr ds : [FO_VAR_wmLastRndTime], 0;
		retn;
	}
}

static void __declspec(naked) WorldMapEncPatch3() {
	__asm {
		mov eax, ds:[FO_VAR_wmLastRndTime];
		retn;
	}
}

static DWORD WorldMapEncounterRate;
static void __declspec(naked) wmWorldMapFunc_hook() {
	__asm {
		inc  dword ptr ds:[FO_VAR_wmLastRndTime];
		jmp  fo::funcoffs::wmPartyWalkingStep_;
	}
}

static void __declspec(naked) wmRndEncounterOccurred_hack() {
	__asm {
		xor  ecx, ecx;
		cmp  edx, WorldMapEncounterRate;
		retn;
	}
}

static void __declspec(naked) ViewportHook() {
	__asm {
		call fo::funcoffs::wmWorldMapLoadTempData_;
		mov eax, ViewportX;
		mov ds : [FO_VAR_wmWorldOffsetX], eax
		mov eax, ViewportY;
		mov ds : [FO_VAR_wmWorldOffsetY], eax;
		retn;
	}
}

static void __declspec(naked) wmTownMapFunc_hack() {
	__asm {
		cmp  edx, 0x31
		jl   end
		cmp  edx, ecx
		jge  end
		push edx
		sub  edx, 0x31
		lea  eax, ds:0[edx*8]
		sub  eax, edx
		pop  edx
		cmp  dword ptr [edi+eax*4+0x0], 0         // Visited
		je   end
		cmp  dword ptr [edi+eax*4+0x4], -1        // Xpos
		je   end
		cmp  dword ptr [edi+eax*4+0x8], -1        // Ypos
		je   end
		retn
end:
		pop  eax                                  // destroy the return address
		push 0x4C4976
		retn
	}
}

static double TickFrac = 0;
static double MapMulti = 1;
static double MapMulti2 = 1;
void _stdcall SetMapMulti(float d) {
	MapMulti2 = d;
}

static __declspec(naked) void PathfinderFix3() {
	__asm {
		xor eax, eax;
		retn;
	}
}

static DWORD _stdcall PathfinderFix2(DWORD perkLevel, DWORD ticks) {
	double d = MapMulti*MapMulti2;
	if (perkLevel == 1) d *= 0.75;
	else if (perkLevel == 2) d *= 0.5;
	else if (perkLevel == 3) d *= 0.25;
	d = ((double)ticks)*d + TickFrac;
	TickFrac = modf(d, &d);
	return (DWORD)d;
}

static __declspec(naked) void PathfinderFix() {
	using namespace fo;
	__asm {
		push eax;
		mov eax, ds:[FO_VAR_obj_dude];
		mov edx, PERK_pathfinder;
		call fo::funcoffs::perk_level_;
		push eax;
		call PathfinderFix2;
		call fo::funcoffs::inc_game_time_;
		retn;
	}
}

static void __declspec(naked) wmInterfaceInit_text_font_hook() {
	__asm {
		mov  eax, 0x65; // normal text font
		call fo::funcoffs::text_font_;
		retn;
	}
}

static void RestRestore() {
	if (!restMode) return;

	restMode = false;
	SafeWrite8(0x49952C, 0x85);
	SafeWrite8(0x497557, 0x85);
	SafeWrite8(0x42E587, 0xC7);
	SafeWrite32(0x42E588, 0x10C2444);
	SafeWrite16(0x499FD4, 0xC201);
	SafeWrite16(0x499E93, 0x0574);
}

void WorldLimitsPatches() {
	DWORD date = GetConfigInt("Misc", "LocalMapXLimit", 0);
	if (date) {
		dlog("Applying local map x limit patch.", DL_INIT);
		SafeWrite32(0x4B13B9, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetConfigInt("Misc", "LocalMapYLimit", 0);
	if (date) {
		dlog("Applying local map y limit patch.", DL_INIT);
		SafeWrite32(0x4B13C7, date);
		dlogr(" Done", DL_INIT);
	}

	//if(GetConfigInt("Misc", "WorldMapCitiesListFix", 0)) {
	dlog("Applying world map cities list patch.", DL_INIT);

	HookCall(0x4C04B9, &ScrollCityListHook);
	HookCall(0x4C04C8, &ScrollCityListHook);
	HookCall(0x4C4A34, &ScrollCityListHook);
	HookCall(0x4C4A3D, &ScrollCityListHook);
	dlogr(" Done", DL_INIT);
	//}

	//if(GetConfigInt("Misc", "CitiesLimitFix", 0)) {
	dlog("Applying cities limit patch.", DL_INIT);
	if (*((BYTE*)0x4BF3BB) != 0xEB) {
		SafeWrite8(0x4BF3BB, 0xEB);
	}
	dlogr(" Done", DL_INIT);
	//}

	DWORD wmSlots = GetConfigInt("Misc", "WorldMapSlots", 0);
	if (wmSlots && wmSlots < 128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27;
		if (wmSlots < 25) SafeWrite32(0x4C21FD, 230 - (wmSlots - 17) * 27);
		else {
			SafeWrite8(0x4C21FC, 0xC2);
			SafeWrite32(0x4C21FD, 2 + 27 * (wmSlots - 26));
		}
		dlogr(" Done", DL_INIT);
	}
}

void TimeLimitPatch() {
	int limit = GetConfigInt("Misc", "TimeLimit", 13);
	if (limit == -2) {
		limit = 14;
	}
	if (limit == -3) {
		dlog("Applying time limit patch (-3).", DL_INIT);
		limit = -1;
		addUnarmedStatToGetYear = 1;

		HookCalls(GetDateWrapper, {
			0x4392F8,
			0x443808,
			0x47E127,
			0x4975A2,
			0x497712,
			0x4979C9,
			0x4C3CB5
		});
		dlogr(" Done", DL_INIT);
	}

	if (limit <= 14 && limit >= -1 && limit != 13) {
		dlog("Applying time limit patch.", DL_INIT);
		if (limit == -1) {
			HookCall(0x4A34F9, &TimerReset);
			HookCall(0x4A3551, &TimerReset);

			SafeMemSet(0x4A34EF, 0x90, 10);
			SafeMemSet(0x4A34FE, 0x90, 6);

			SafeMemSet(0x4A3547, 0x90, 10);
			SafeMemSet(0x4A3556, 0x90, 6);
		} else {
			SafeWrite8(0x4A34EC, limit);
			SafeWrite8(0x4A3544, limit);
		}
		dlogr(" Done", DL_INIT);
	}
}

void TownMapsHotkeyFix() {
	if (GetConfigInt("Misc", "TownMapHotkeysFix", 1)) {
		dlog("Applying town map hotkeys patch.", DL_INIT);
		MakeCall(0x4C4945, wmTownMapFunc_hack);
		dlogr(" Done", DL_INIT);
	}
}

void WorldmapFpsPatch() {
	if (GetConfigInt("Misc", "WorldMapFPSPatch", 0)) {
		dlog("Applying world map fps patch.", DL_INIT);
		if (*(DWORD*)0x4BFE5E != 0x8D16) {
			dlogr(" Failed", DL_INIT);
		} else {
			worldMapDelay = GetConfigInt("Misc", "WorldMapDelay2", 66);
			HookCall(0x4BFE5D, WorldMapFpsPatch);
			::sfall::availableGlobalScriptTypes |= 2;
			dlogr(" Done", DL_INIT);
		}
	} else {
		if (*(DWORD*)0x4BFE5E == 0x8D16) {
			HookCall(0x4BFE5D, WorldMapHook);
			::sfall::availableGlobalScriptTypes |= 2;
		}
	}

	if (GetConfigInt("Misc", "WorldMapEncounterFix", 0)) {
		dlog("Applying world map encounter patch.", DL_INIT);
		WorldMapEncounterRate = GetConfigInt("Misc", "WorldMapEncounterRate", 5);
		SafeWrite32(0x4C232D, 0x01EBC031);        // xor eax, eax; jmps 0x4C2332
		HookCall(0x4BFEE0, &wmWorldMapFunc_hook);
		MakeCall(0x4C0667, wmRndEncounterOccurred_hack);
		dlogr(" Done", DL_INIT);
	}
}

void PathfinderFixInit() {
	//if(GetConfigInt("Misc", "PathfinderFix", 0)) {
	dlog("Applying pathfinder patch.", DL_INIT);
	HookCall(0x4C1FF1, &PathfinderFix3);
	HookCall(0x4C1C78, &PathfinderFix);
	MapMulti = (double)GetConfigInt("Misc", "WorldMapTimeMod", 100) / 100.0;
	dlogr(" Done", DL_INIT);
//}
}

void StartingStatePatches() {
	int date = GetConfigInt("Misc", "StartYear", -1);
	if (date > 0) {
		dlog("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetConfigInt("Misc", "StartMonth", -1);
	if (date >= 0 && date < 12) {
		dlog("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetConfigInt("Misc", "StartDay", -1);
	if (date >= 0 && date < 31) {
		dlog("Applying starting day patch.", DL_INIT);
		SafeWrite8(0x4A3356, date);
		dlogr(" Done", DL_INIT);
	}

	date = GetConfigInt("Misc", "StartXPos", -1);
	if (date != -1) {
		dlog("Applying starting x position patch.", DL_INIT);
		SafeWrite32(0x4BC990, date);
		SafeWrite32(0x4BCC08, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetConfigInt("Misc", "StartYPos", -1);
	if (date != -1) {
		dlog("Applying starting y position patch.", DL_INIT);
		SafeWrite32(0x4BC995, date);
		SafeWrite32(0x4BCC0D, date);
		dlogr(" Done", DL_INIT);
	}

	ViewportX = GetConfigInt("Misc", "ViewXPos", -1);
	if (ViewportX != -1) {
		dlog("Applying starting x view patch.", DL_INIT);
		SafeWrite32(FO_VAR_wmWorldOffsetX, ViewportX);
		HookCall(0x4BCF07, &ViewportHook);
		dlogr(" Done", DL_INIT);
	}
	ViewportY = GetConfigInt("Misc", "ViewYPos", -1);
	if (ViewportY != -1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(FO_VAR_wmWorldOffsetY, ViewportY);
		HookCall(0x4BCF07, &ViewportHook);
		dlogr(" Done", DL_INIT);
	}
}

void WorldMapFontPatch() {
	if (GetConfigInt("Misc", "WorldMapFont", 0)) {
		dlog("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
		dlogr(" Done", DL_INIT);
	}
}

void Worldmap::init() {
	PathfinderFixInit();
	StartingStatePatches();
	TimeLimitPatch();
	TownMapsHotkeyFix();
	WorldLimitsPatches();
	WorldmapFpsPatch();
	WorldMapFontPatch();

	LoadGameHook::OnGameReset() += []() {
		SetCarInterfaceArt(0x1B1);
		if (restTime) SetRestHealTime(180);
		RestRestore();
	};
}

Delegate<>& Worldmap::OnWorldmapLoop() {
	return onWorldmapLoop;
}

void Worldmap::SetCarInterfaceArt(DWORD artIndex) {
	SafeWrite32(0x4C2D9B, artIndex);
}

void Worldmap::SetRestHealTime(DWORD minutes) {
	if (minutes > 0) {
		SafeWrite32(0x499FDE, minutes);
		restTime = (minutes != 180);
	}
}

void Worldmap::SetRestMode(DWORD mode) {
	RestRestore(); // restore default

	restMode = ((mode & 0x7) > 0);
	if (!restMode) return;

	if (mode & 1) { // bit1 - disable resting on all maps
		SafeWrite8(0x49952C, 0x31); // test to xor
		SafeWrite8(0x497557, 0x31);
		return;
	}
	if (mode & 2) { // bit2 - disable resting on maps with "can_rest_here=No" in Maps.txt, even if there are no other critters
		SafeWrite8(0x42E587, 0xE9);
		SafeWrite32(0x42E588, 0x00000094); // jmp  0x42E620
	}
	if (mode & 4) { // bit3 - disable healing during resting
		SafeWrite16(0x499FD4, 0x9090);
		SafeWrite16(0x499E93, 0x8FEB); // jmp  0x499E24
	}
}

}
