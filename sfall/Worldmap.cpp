/*
 *    sfall
 *    Copyright (C) 2008-2019  The sfall team
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

#include "main.h"

#include "Define.h"
#include "FalloutEngine.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"
#include "SpeedPatch.h"

static DWORD AutomapPipboyList[AUTOMAP_MAX];

static DWORD ViewportX;
static DWORD ViewportY;

static int mapSlotsScrollMax = 27 * (17 - 7);
static int mapSlotsScrollLimit = 0;

static DWORD worldMapDelay;
static DWORD worldMapTicks;

static DWORD WorldMapEncounterRate;

static double tickFract = 0.0;
static double mapMultiMod = 1.0;
static float scriptMapMulti = 1.0;

static bool addYear = false; // used as additional years indicator
static DWORD addedYears = 0;

static const DWORD ScrollCityListAddr[] = {
	0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D,
};

static __declspec(naked) void TimeDateFix() {
	__asm {
		test edi, edi; // year buf
		jz   end;
		add  esi, addedYears;
		mov  [edi], esi;
end:
		retn;
	}
}

static void TimerReset() {
	__asm push ecx;

	const DWORD time = ONE_GAME_YEAR * 13;
	*ptr_fallout_game_time -= time;
	addedYears += 13;

	// fix queue time
	Queue* queue = (Queue*)(*ptr_queue);
	while (queue) {
		if (queue->time > time) {
			queue->time -= time;
		} else {
			queue->time = 0;
		}
		queue = queue->next;
	}
	__asm pop ecx;
}

static __declspec(naked) void script_chk_timed_events_hack() {
	__asm {
		mov  eax, dword ptr ds:[_fallout_game_time];
		inc  eax;
		mov  dword ptr ds:[_fallout_game_time], eax;
		cmp  eax, ONE_GAME_YEAR * 13;
		jae  reset;
		retn;
reset:
		jmp  TimerReset;
	}
}

static __declspec(naked) void set_game_time_hack() {
	__asm {
		mov  dword ptr ds:[_fallout_game_time], eax;
		mov  edx, eax;
		call IsMapLoaded;
		test al, al;
		jz   end;
		cmp  edx, ONE_GAME_YEAR * 13;
		jb   end;
		call TimerReset;
end:
		xor  edx, edx;
		retn;
	}
}

static __declspec(naked) void ScrollCityListFix() {
	__asm {
		push ebx;
		mov  ebx, ds:[0x672F10]; // _wmLastTabsYOffset
		test eax, eax;
		jl   up;
		cmp  ebx, mapSlotsScrollMax;
		pop  ebx;
		jl   run;
		retn;
up:
		test ebx, ebx;
		pop  ebx;
		jnz  run;
		retn;
run:
		jmp  wmInterfaceScrollTabsStart_;
	}
}

static void __declspec(naked) WorldMapFpsPatch() {
	__asm {
		push dword ptr ds:[_last_buttons];
		push dword ptr ds:[0x6AC7B0]; // _mouse_buttons
		mov  esi, worldMapTicks;
		mov  ebx, esi;                // previous ticks
loopDelay:
		call RunGlobalScripts3;
		call process_bk_;
subLoop:
		call ds:[sf_GetTickCount]; // current ticks
		mov  edx, eax;
		sub  eax, ebx;     // get elapsed time (cur.ticks - prev.ticks)
		cmp  eax, 10;      // delay - GetTickCount returns minimum difference of 15 units
		jb   subLoop;      // elapsed < invoke delay
		mov  ebx, edx;
		sub  edx, esi;     // get elapsed time (cur.ticks - worldMapTicks)
		cmp  edx, worldMapDelay;
		jb   loopDelay;    // elapsed < worldMapDelay

		pop  dword ptr ds:[0x6AC7B0]; // _mouse_buttons
		pop  dword ptr ds:[_last_buttons];
		call ds:[sf_GetTickCount];
		mov  worldMapTicks, eax;
		jmp  get_input_;
	}
}

static void __declspec(naked) WorldMapFpsPatch2() {
	__asm {
		mov  esi, worldMapTicks;
		mov  ebx, esi;
loopDelay:
		call RunGlobalScripts3;
subLoop:
		call ds:[sf_GetTickCount]; // current ticks
		mov  edx, eax;
		sub  eax, ebx;     // get elapsed time
		jz   subLoop;
		mov  ebx, edx;
		sub  edx, esi;     // get elapsed time
		cmp  edx, worldMapDelay;
		jb   loopDelay;    // elapsed < worldMapDelay

		call ds:[sf_GetTickCount];
		mov  worldMapTicks, eax;
		jmp  get_input_;
	}
}

// Only used if the world map speed patch is disabled, so that world map scripts are still run
static void __declspec(naked) wmWorldMap_hook() {
	__asm {
		pushadc;
		call RunGlobalScripts3;
		popadc;
		jmp  get_input_;
	}
}

static void __declspec(naked) wmWorldMapFunc_hook() {
	__asm {
		inc  dword ptr ds:[_wmLastRndTime];
		jmp  wmPartyWalkingStep_;
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
		call wmWorldMapLoadTempData_;
		mov  eax, ViewportX;
		mov  ds:[_wmWorldOffsetX], eax;
		mov  eax, ViewportY;
		mov  ds:[_wmWorldOffsetY], eax;
		retn;
	}
}

static void __declspec(naked) wmTownMapFunc_hack() {
	__asm {
		cmp  dword ptr [edi][eax * 4 + 0], 0;  // Visited
		je   end;
		cmp  dword ptr [edi][eax * 4 + 4], -1; // Xpos
		je   end;
		cmp  dword ptr [edi][eax * 4 + 8], -1; // Ypos
		je   end;
		// engine code
		mov  edx, [edi][eax * 4 + 0xC];
		mov  [esi], edx
		retn;
end:
		add  esp, 4;                           // destroy the return address
		mov  eax, 0x4C4976;
		jmp  eax;
	}
}

static DWORD _stdcall PathfinderCalc(DWORD perkLevel, DWORD ticks) {
	double multi = mapMultiMod * scriptMapMulti;

	switch (perkLevel) {
	case 1:
		multi *= 0.75;
		break;
	case 2:
		multi *= 0.5;
		break;
	case 3:
		multi *= 0.25;
		break;
	}
	multi = ((double)ticks) * multi + tickFract;
	tickFract = modf(multi, &multi);

	return static_cast<DWORD>(multi);
}

static __declspec(naked) void PathfinderFix() {
	__asm {
		push eax; // ticks
		mov  eax, ds:[_obj_dude];
		mov  edx, PERK_pathfinder;
		call perk_level_;
		push eax;
		call PathfinderCalc;
		jmp  inc_game_time_;
	}
}

static void __declspec(naked) wmInterfaceInit_text_font_hook() {
	__asm {
		mov  eax, 0x65; // normal text font
		jmp  text_font_;
	}
}

static const char* automap = "automap"; // no/yes overrides the value in the table to display the automap in pipboy
static void __declspec(naked) wmMapInit_hack() {
	__asm {
		mov  esi, [esp + 0xA0 - 0x20 + 4];       // curent map number
		cmp  esi, AUTOMAP_MAX;
		jge  end;
		lea  eax, [esp + 4];                     // file
		lea  edx, [esp + 0xA0 - 0x50 + 4];       // section
		mov  ebx, automap;                       // key
		lea  ecx, [esp + 0xA0 - 0x24 + 4];       // value buf
		call config_get_string_;
		test eax, eax;
		jz   end;
		mov  ecx, 2;                             // max index
		mov  ebx, _wmYesNoStrs;
		lea  eax, [esp + 0xA0 - 0x24 + 4];       // key value
		sub  esp, 4;
		mov  edx, esp;                           // index buf
		call strParseStrFromList_;
		cmp  eax, -1;
		jz   skip;
		mov  edx, [esp];                         // value index
		dec  edx;
		mov  [AutomapPipboyList][esi * 4], edx;  // no = -1, yes = 0
skip:
		add  esp, 4;
end:
		inc  esi;
		mov  [esp + 0xA0 - 0x20 + 4], esi;
		retn;
	}
}

void WorldLimitsPatches() {
	DWORD data = GetPrivateProfileIntA("Misc", "LocalMapXLimit", 0, ini);
	if (data) {
		dlog("Applying local map x limit patch.", DL_INIT);
		SafeWrite32(0x4B13B9, data);
		dlogr(" Done", DL_INIT);
	}
	data = GetPrivateProfileIntA("Misc", "LocalMapYLimit", 0, ini);
	if (data) {
		dlog("Applying local map y limit patch.", DL_INIT);
		SafeWrite32(0x4B13C7, data);
		dlogr(" Done", DL_INIT);
	}

	//if (GetPrivateProfileIntA("Misc", "WorldMapCitiesListFix", 0, ini)) {
		dlog("Applying world map cities list patch.", DL_INIT);
		for (int i = 0; i < sizeof(ScrollCityListAddr) / 4; i++) {
			HookCall(ScrollCityListAddr[i], ScrollCityListFix);
		}
		dlogr(" Done", DL_INIT);
	//}

	//if (GetPrivateProfileIntA("Misc", "CitiesLimitFix", 0, ini)) {
		dlog("Applying cities limit patch.", DL_INIT);
		if (*((BYTE*)0x4BF3BB) != 0xEB) {
			SafeWrite8(0x4BF3BB, 0xEB);
		}
		dlogr(" Done", DL_INIT);
	//}

	DWORD wmSlots = GetPrivateProfileIntA("Misc", "WorldMapSlots", 0, ini);
	if (wmSlots && wmSlots < 128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27; // height value after which scrolling is not possible
		mapSlotsScrollLimit = wmSlots * 27;
		SafeWrite32(0x4C21FD, 189); // 27 * 7
		SafeWrite32(0x4C21F1, (DWORD)&mapSlotsScrollLimit);
		dlogr(" Done", DL_INIT);
	}
}

void TimeLimitPatch() {
	int limit = GetPrivateProfileIntA("Misc", "TimeLimit", 13, ini);
	if (limit == -2 || limit == -3) {
		addYear = true;
		MakeCall(0x4A33B8, TimeDateFix, 1); // game_time_date_
		limit = -1; // also reset time
	}
	if (limit >= -1 && limit < 13) {
		dlog("Applying time limit patch.", DL_INIT);
		if (limit == -1) {
			MakeCall(0x4A3DF5, script_chk_timed_events_hack, 1);
			MakeCall(0x4A3488, set_game_time_hack);
			MakeCall(0x4A34EF, TimerReset); // inc_game_time_
			MakeCall(0x4A3547, TimerReset); // inc_game_time_in_seconds_
			SafeMemSet(0x4A34F4, 0x90, 16);
			SafeMemSet(0x4A354C, 0x90, 16);
		} else {
			SafeWrite8(0x4A34EC, limit);
			SafeWrite8(0x4A3544, limit);
		}
		dlogr(" Done", DL_INIT);
	}
}

void TownMapsHotkeyFix() {
	if (GetPrivateProfileIntA("Misc", "TownMapHotkeysFix", 1, ini)) {
		dlog("Applying town map hotkeys patch.", DL_INIT);
		MakeCall(0x4C495A, wmTownMapFunc_hack, 1);
		dlogr(" Done", DL_INIT);
	}
}

void WorldmapFpsPatch() {
	bool fpsPatchOK = (*(DWORD*)0x4BFE5E == 0x8D16);
	if (GetPrivateProfileIntA("Misc", "WorldMapFPSPatch", 0, ini)) {
		dlog("Applying world map fps patch.", DL_INIT);
		if (!fpsPatchOK) {
			dlogr(" Failed", DL_INIT);
		} else {
			int delay = GetPrivateProfileIntA("Misc", "WorldMapDelay2", 66, ini);
			worldMapDelay = max(1, delay);
			dlogr(" Done", DL_INIT);
		}
	}
	if (fpsPatchOK) {
		void* func;
		if (worldMapDelay == 0) {
			func = wmWorldMap_hook;
		} else if (worldMapDelay > 25) {
			func = WorldMapFpsPatch;
		} else {
			func = WorldMapFpsPatch2;
		}
		HookCall(0x4BFE5D, func);
		AvailableGlobalScriptTypes |= 2;
	}

	if (GetPrivateProfileIntA("Misc", "WorldMapEncounterFix", 0, ini)) {
		dlog("Applying world map encounter patch.", DL_INIT);
		WorldMapEncounterRate = GetPrivateProfileIntA("Misc", "WorldMapEncounterRate", 5, ini);
		SafeWrite32(0x4C232D, 0x01EBC031); // xor eax, eax; jmps 0x4C2332 (wmInterfaceInit_)
		HookCall(0x4BFEE0, wmWorldMapFunc_hook);
		MakeCall(0x4C0667, wmRndEncounterOccurred_hack);
		dlogr(" Done", DL_INIT);
	}
}

void PathfinderFixInit() {
	//if (GetPrivateProfileIntA("Misc", "PathfinderFix", 0, ini)) {
		dlog("Applying Pathfinder patch.", DL_INIT);
		SafeWrite16(0x4C1FF6, 0x9090);     // wmPartyWalkingStep_
		HookCall(0x4C1C78, PathfinderFix); // wmGameTimeIncrement_
		mapMultiMod = (double)GetPrivateProfileIntA("Misc", "WorldMapTimeMod", 100, ini) / 100.0;
		dlogr(" Done", DL_INIT);
	//}
}

void StartingStatePatches() {
	int date = GetPrivateProfileIntA("Misc", "StartYear", -1, ini);
	if (date >= 0) {
		dlog("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileIntA("Misc", "StartMonth", -1, ini);
	if (date >= 0 && date < 12) {
		dlog("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileIntA("Misc", "StartDay", -1, ini);
	if (date >= 0 && date < 31) {
		dlog("Applying starting day patch.", DL_INIT);
		SafeWrite8(0x4A3356, date);
		dlogr(" Done", DL_INIT);
	}

	date = GetPrivateProfileIntA("Misc", "StartXPos", -1, ini);
	if (date != -1) {
		dlog("Applying starting x position patch.", DL_INIT);
		SafeWrite32(0x4BC990, date);
		SafeWrite32(0x4BCC08, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileIntA("Misc", "StartYPos", -1, ini);
	if (date != -1) {
		dlog("Applying starting y position patch.", DL_INIT);
		SafeWrite32(0x4BC995, date);
		SafeWrite32(0x4BCC0D, date);
		dlogr(" Done", DL_INIT);
	}

	ViewportX = GetPrivateProfileIntA("Misc", "ViewXPos", -1, ini);
	if (ViewportX != -1) {
		dlog("Applying starting x view patch.", DL_INIT);
		SafeWrite32(_wmWorldOffsetX, ViewportX);
		dlogr(" Done", DL_INIT);
	}
	ViewportY = GetPrivateProfileIntA("Misc", "ViewYPos", -1, ini);
	if (ViewportY != -1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(_wmWorldOffsetY, ViewportY);
		dlogr(" Done", DL_INIT);
	}
	if (ViewportX != -1 || ViewportY != -1) HookCall(0x4BCF07, ViewportHook); // game_reset_
}

void WorldMapFontPatch() {
	if (GetPrivateProfileIntA("Misc", "WorldMapFontPatch", 0, ini)) {
		dlog("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
		dlogr(" Done", DL_INIT);
	}
}

void PipBoyAutomapsPatch() {
	dlog("Applying Pip-Boy automaps patch.", DL_INIT);
	MakeCall(0x4BF931, wmMapInit_hack, 2);
	SafeWrite32(0x41B8B7, (DWORD)AutomapPipboyList);
	memcpy(AutomapPipboyList, (void*)_displayMapList, sizeof(AutomapPipboyList)); // copy vanilla data
	dlogr(" Done", DL_INIT);
}

void _stdcall SetMapMulti(float value) {
	scriptMapMulti = value;
}

void SetAddedYears(DWORD years) {
	addedYears = years;
}

DWORD GetAddedYears(bool isCheck) {
	return (isCheck && !addYear) ? 0 : addedYears;
}

void WorldmapViewportPatch() {
	// Fix images for up/down buttons
	SafeWrite32(0x4C2C0A, 199); // index of UPARWOFF.FRM
	SafeWrite8(0x4C2C7C, 0x43); // dec ebx > inc ebx
	SafeWrite32(0x4C2C92, 181); // index of DNARWOFF.FRM
	SafeWrite8(0x4C2D04, 0x46); // dec esi > inc esi
}

void WorldmapInit() {
	PathfinderFixInit();
	StartingStatePatches();
	TimeLimitPatch();
	TownMapsHotkeyFix();
	WorldLimitsPatches();
	WorldmapFpsPatch();
	WorldMapFontPatch();
	PipBoyAutomapsPatch();
	WorldmapViewportPatch(); // must be located after WorldMapSlots patch
}
