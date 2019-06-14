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

#include <map>
#include <math.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"
#include "SpeedPatch.h"

#include "Worldmap.h"

namespace sfall
{

static Delegate<> onWorldmapLoop;

static DWORD AutomapPipboyList[AUTOMAP_MAX];

static DWORD ViewportX;
static DWORD ViewportY;

struct levelRest {
	char level[4];
};
std::map<int, levelRest> mapRestInfo;

static bool restMap;
static bool restMode;
static bool restTime;

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

	const DWORD time = fo::TicksTime::ONE_GAME_YEAR * 13;
	fo::var::fallout_game_time -= time;
	addedYears += 13;

	// fix queue time
	fo::Queue* queue = (fo::Queue*)fo::var::queue;
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
	using namespace fo;
	__asm {
		mov  eax, dword ptr ds:[FO_VAR_fallout_game_time];
		inc  eax;
		mov  dword ptr ds:[FO_VAR_fallout_game_time], eax;
		cmp  eax, ONE_GAME_YEAR * 13;
		jae  reset;
		retn;
reset:
		jmp  TimerReset;
	}
}

static __declspec(naked) void set_game_time_hack() {
	using namespace fo;
	__asm {
		mov  dword ptr ds:[FO_VAR_fallout_game_time], eax;
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
		jmp  fo::funcoffs::wmInterfaceScrollTabsStart_;
	}
}

static void __stdcall WorldmapLoopHook() {
	onWorldmapLoop.invoke();
}

static void __declspec(naked) WorldMapFpsPatch() {
	__asm {
		push dword ptr ds:[FO_VAR_last_buttons];
		push dword ptr ds:[0x6AC7B0]; // _mouse_buttons
		mov  esi, worldMapTicks;
		mov  ebx, esi;                // previous ticks
loopDelay:
		call WorldmapLoopHook;
		call fo::funcoffs::process_bk_;
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
		pop  dword ptr ds:[FO_VAR_last_buttons];
		call ds:[sf_GetTickCount];
		mov  worldMapTicks, eax;
		jmp  fo::funcoffs::get_input_;
	}
}

static void __declspec(naked) WorldMapFpsPatch2() {
	__asm {
		mov  esi, worldMapTicks;
		mov  ebx, esi;
loopDelay:
		call WorldmapLoopHook;
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
		jmp  fo::funcoffs::get_input_;
	}
}

// Only used if the world map speed patch is disabled, so that world map scripts are still run
static void __declspec(naked) wmWorldMap_hook() {
	__asm {
		pushadc;
		call WorldmapLoopHook;
		popadc;
		jmp  fo::funcoffs::get_input_;
	}
}

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
		mov  eax, ViewportX;
		mov  ds:[FO_VAR_wmWorldOffsetX], eax;
		mov  eax, ViewportY;
		mov  ds:[FO_VAR_wmWorldOffsetY], eax;
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
	using namespace fo;
	__asm {
		push eax; // ticks
		mov  eax, ds:[FO_VAR_obj_dude];
		mov  edx, PERK_pathfinder;
		call fo::funcoffs::perk_level_;
		push eax;
		call PathfinderCalc;
		jmp  fo::funcoffs::inc_game_time_;
	}
}

static void __declspec(naked) wmInterfaceInit_text_font_hook() {
	__asm {
		mov  eax, 0x65; // normal text font
		jmp  fo::funcoffs::text_font_;
	}
}

static void __declspec(naked) critter_can_obj_dude_rest_hook() {
	using namespace fo;
	__asm {
		push eax;
		mov  ecx, eax;  // elevation
		mov  edx, dword ptr ds:[FO_VAR_map_number];
		call Worldmap::GetRestMapLevel;
		xor  edx, edx;
		cmp  eax, edx;
		jge  skip;
		pop  eax;
		jmp  fo::funcoffs::wmMapCanRestHere_;
skip:
		add  esp, 4;
		retn;           // overrides
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
		call fo::funcoffs::config_get_string_;
		test eax, eax;
		jz   end;
		mov  ecx, 2;                             // max index
		mov  ebx, FO_VAR_wmYesNoStrs;
		lea  eax, [esp + 0xA0 - 0x24 + 4];       // key value
		sub  esp, 4;
		mov  edx, esp;                           // index buf
		call fo::funcoffs::strParseStrFromList_;
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

static void __declspec(naked) wmWorldMap_hack() {
	__asm {
		mov  ebx, [ebx + 0x34]; // wmAreaInfoList.size
		cmp  ebx, 1;
		jg   largeLoc;
		je   mediumLoc;
//smallLoc:
		sub  eax, 5;
		sub  edx, 5;
mediumLoc:
		sub  eax, 10;
		sub  edx, 10;
largeLoc:
		xor  ebx, ebx;
		jmp  fo::funcoffs::wmPartyInitWalking_;
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
	DWORD data = GetConfigInt("Misc", "LocalMapXLimit", 0);
	if (data) {
		dlog("Applying local map x limit patch.", DL_INIT);
		SafeWrite32(0x4B13B9, data);
		dlogr(" Done", DL_INIT);
	}
	data = GetConfigInt("Misc", "LocalMapYLimit", 0);
	if (data) {
		dlog("Applying local map y limit patch.", DL_INIT);
		SafeWrite32(0x4B13C7, data);
		dlogr(" Done", DL_INIT);
	}

	//if(GetConfigInt("Misc", "WorldMapCitiesListFix", 0)) {
	dlog("Applying world map cities list patch.", DL_INIT);
	HookCalls(ScrollCityListFix, {0x4C04B9, 0x4C04C8, 0x4C4A34, 0x4C4A3D});
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
		mapSlotsScrollMax = (wmSlots - 7) * 27; // height value after which scrolling is not possible
		mapSlotsScrollLimit = wmSlots * 27;
		SafeWrite32(0x4C21FD, 189); // 27 * 7
		SafeWrite32(0x4C21F1, (DWORD)&mapSlotsScrollLimit);
		dlogr(" Done", DL_INIT);
	}
}

void TimeLimitPatch() {
	int limit = GetConfigInt("Misc", "TimeLimit", 13);
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
			MakeCalls(TimerReset, {
				0x4A34EF, // inc_game_time_
				0x4A3547  // inc_game_time_in_seconds_
			});
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
	if (GetConfigInt("Misc", "TownMapHotkeysFix", 1)) {
		dlog("Applying town map hotkeys patch.", DL_INIT);
		MakeCall(0x4C495A, wmTownMapFunc_hack, 1);
		dlogr(" Done", DL_INIT);
	}
}

void WorldmapFpsPatch() {
	bool fpsPatchOK = (*(DWORD*)0x4BFE5E == 0x8D16);
	if (GetConfigInt("Misc", "WorldMapFPSPatch", 0)) {
		dlog("Applying world map fps patch.", DL_INIT);
		if (!fpsPatchOK) {
			dlogr(" Failed", DL_INIT);
		} else {
			int delay = GetConfigInt("Misc", "WorldMapDelay2", 66);
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
		::sfall::availableGlobalScriptTypes |= 2;
	}

	if (GetConfigInt("Misc", "WorldMapEncounterFix", 0)) {
		dlog("Applying world map encounter patch.", DL_INIT);
		WorldMapEncounterRate = GetConfigInt("Misc", "WorldMapEncounterRate", 5);
		SafeWrite32(0x4C232D, 0x01EBC031); // xor eax, eax; jmps 0x4C2332 (wmInterfaceInit_)
		HookCall(0x4BFEE0, wmWorldMapFunc_hook);
		MakeCall(0x4C0667, wmRndEncounterOccurred_hack);
		dlogr(" Done", DL_INIT);
	}
}

void PathfinderFixInit() {
	//if(GetConfigInt("Misc", "PathfinderFix", 0)) {
	dlog("Applying Pathfinder patch.", DL_INIT);
	SafeWrite16(0x4C1FF6, 0x9090);     // wmPartyWalkingStep_
	HookCall(0x4C1C78, PathfinderFix); // wmGameTimeIncrement_
	mapMultiMod = (double)GetConfigInt("Misc", "WorldMapTimeMod", 100) / 100.0;
	dlogr(" Done", DL_INIT);
	//}
}

void StartingStatePatches() {
	int date = GetConfigInt("Misc", "StartYear", -1);
	if (date >= 0) {
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
		dlogr(" Done", DL_INIT);
	}
	ViewportY = GetConfigInt("Misc", "ViewYPos", -1);
	if (ViewportY != -1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(FO_VAR_wmWorldOffsetY, ViewportY);
		dlogr(" Done", DL_INIT);
	}
	if (ViewportX != -1 || ViewportY != -1) HookCall(0x4BCF07, ViewportHook); // game_reset_
}

void WorldMapInterfacePatch() {
	if (GetConfigInt("Misc", "WorldMapFontPatch", 0)) {
		dlog("Applying world map font patch.", DL_INIT);
		HookCall(0x4C2343, wmInterfaceInit_text_font_hook);
		dlogr(" Done", DL_INIT);
	}
	// Fix images for up/down buttons
	SafeWrite32(0x4C2C0A, 199); // index of UPARWOFF.FRM
	SafeWrite8(0x4C2C7C, 0x43); // dec ebx > inc ebx
	SafeWrite32(0x4C2C92, 181); // index of DNARWOFF.FRM
	SafeWrite8(0x4C2D04, 0x46); // dec esi > inc esi

	// Fix the position of the target marker for small/medium location circles
	MakeCall(0x4C03AA, wmWorldMap_hack, 2);
}

void PipBoyAutomapsPatch() {
	dlog("Applying Pip-Boy automaps patch.", DL_INIT);
	MakeCall(0x4BF931, wmMapInit_hack, 2);
	SafeWrite32(0x41B8B7, (DWORD)AutomapPipboyList);
	memcpy(AutomapPipboyList, (void*)FO_VAR_displayMapList, sizeof(AutomapPipboyList)); // copy vanilla data
	dlogr(" Done", DL_INIT);
}

void Worldmap::SaveData(HANDLE file) {
	DWORD sizeWrite, count = mapRestInfo.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	std::map<int, levelRest>::iterator it;
	for (it = mapRestInfo.begin(); it != mapRestInfo.end(); it++) {
		WriteFile(file, &it->first, 4, &sizeWrite, 0);
		WriteFile(file, &it->second, sizeof(levelRest), &sizeWrite, 0);
	}
}

bool Worldmap::LoadData(HANDLE file) {
	DWORD count, sizeRead;

	ReadFile(file, &count, 4, &sizeRead, 0);
	if (sizeRead != 4) return true;

	for (DWORD i = 0; i < count; i++) {
		DWORD mID;
		levelRest elevData;
		ReadFile(file, &mID, 4, &sizeRead, 0);
		ReadFile(file, &elevData, sizeof(levelRest), &sizeRead, 0);
		if (sizeRead != sizeof(levelRest)) return true;
		mapRestInfo.insert(std::make_pair(mID, elevData));
	}
	if (count && !restMap) {
		HookCall(0x42E57A, critter_can_obj_dude_rest_hook);
		restMap = true;
	}
	return false;
}

void _stdcall SetMapMulti(float value) {
	scriptMapMulti = value;
}

void Worldmap::SetAddedYears(DWORD years) {
	addedYears = years;
}

DWORD Worldmap::GetAddedYears(bool isCheck) {
	return (isCheck && !addYear) ? 0 : addedYears;
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
		SafeWrite32(0x42E588, 0x94); // jmp  0x42E620
	}
	if (mode & 4) { // bit3 - disable healing during resting
		SafeWrite16(0x499FD4, 0x9090);
		SafeWrite16(0x499E93, 0x8FEB); // jmp  0x499E24
	}
}

void Worldmap::SetRestMapLevel(int mapId, long elev, bool canRest) {
	auto keyIt = mapRestInfo.find(mapId);
	if (keyIt != mapRestInfo.end()) {
		if (elev == -1) {
			keyIt->second.level[++elev] = canRest;
			keyIt->second.level[++elev] = canRest;
			elev++;
		}
		keyIt->second.level[elev] = canRest;
	} else {
		if (!restMap) {
			HookCall(0x42E57A, critter_can_obj_dude_rest_hook);
			restMap = true;
		}

		levelRest elevData = {-1, -1, -1, -1};
		if (elev == -1) {
			elevData.level[++elev] = canRest;
			elevData.level[++elev] = canRest;
			elev++;
		}
		elevData.level[elev] = canRest;
		mapRestInfo.insert(std::make_pair(mapId, elevData));
	}
}

long __fastcall Worldmap::GetRestMapLevel(long elev, int mapId) {
	if (mapRestInfo.empty()) return -1;

	auto keyIt = mapRestInfo.find(mapId);
	if (keyIt != mapRestInfo.end()) {
		return keyIt->second.level[elev];
	}
	return -1;
}

////////////////////////////// WORLDMAP INTERFACE //////////////////////////////

#define WMAP_WIN_WIDTH    (890)
#define WMAP_WIN_HEIGHT   (720)
#define WMAP_TOWN_BUTTONS (15)

static DWORD wmTownMapSubButtonIds[WMAP_TOWN_BUTTONS + 1]; // replace _wmTownMapSubButtonIds (index 0 - unused element)

// Window width
static const DWORD wmWinWidth[] = {
	// wmInterfaceInit_
	0x4C239E, 0x4C247A,
	// wmInterfaceRefresh_
	0x4C38EA, 0x4C3978,
	// wmInterfaceDrawCircleOverlay_
	0x4C3FCA, 0x4C408E,
	// wmRefreshInterfaceDial_
	0x4C5757,
	// wmInterfaceRefreshDate_
	0x4C3D0A, 0x4C3D79, 0x4C3DBB, 0x4C3E87, 0x4C3E1F,
	// wmRefreshTabs_
	0x4C52BF, 0x4C53F5, 0x4C55A5, 0x4C557E, 0x4C54B2, 0x4C53E8,
	// wmRefreshInterfaceOverlay_
	0x4C50FD, 0x4C51CF, 0x4C51F8, 0x4C517F,
	// wmInterfaceRefreshCarFuel_
	0x4C528B, 0x4C529F, 0x4C52AA,
	// wmInterfaceDrawSubTileList_
	0x4C41C1, 0x4C41D2,
	// wmTownMapRefresh_
	0x4C4BDF,
	// wmDrawCursorStopped_
	0x4C42EE, 0x4C43C8, 0x4C445F,
	// wmTownMapRefresh_
	0x4C4BDF
};

// Right limit of the viewport (450)
static const DWORD wmViewportEndRight[] = {
//	0x4BC91F,                                                   // wmWorldMap_init_
	0x4C3937, 0x4C393E, 0x4C39BB, 0x4C3B2F, 0x4C3B36, 0x4C3C4B, // wmInterfaceRefresh_
	0x4C4288, 0x4C436A, 0x4C4409,                               // wmDrawCursorStopped_
	0x4C44B4,                                                   // wmCursorIsVisible_
};

// Bottom limit of viewport (443)
static const DWORD wmViewportEndBottom[] = {
//	0x4BC947,                                                   // wmWorldMap_init_
	0x4C3963, 0x4C38D7, 0x4C39DA, 0x4C3B62, 0x4C3AE7, 0x4C3C74, // wmInterfaceRefresh_
	0x4C429A, 0x4C4378, 0x4C4413,                               // wmDrawCursorStopped_
	0x4C44BE,                                                   // wmCursorIsVisible_
};

static const DWORD wmInterfaceInit_Ret = 0x4C23A7;
static void __declspec(naked) wmInterfaceInit_hack() {
	__asm {
		push eax;
		mov  eax, 640 - WMAP_WIN_WIDTH;
		mov  edx, 480 - WMAP_WIN_HEIGHT;
		jmp  wmInterfaceInit_Ret;
	}
}

static void __declspec(naked) wmInterfaceDrawSubTileList_hack() {
	__asm {
		mov  edx, [esp + 0x10 - 0x10 + 4];
		imul edx, WMAP_WIN_WIDTH;
		add  edx, ecx;
		retn;
	}
}

static void __declspec(naked) wmInterfaceDrawCircleOverlay_hack() {
	__asm {
		mov  eax, ecx;
		imul eax, WMAP_WIN_WIDTH;
		retn;
	}
}

static void __declspec(naked) wmDrawCursorStopped_hack0() {
	__asm {
		mov  ebx, ecx;
		imul ebx, WMAP_WIN_WIDTH;
		retn;
	}
}

static void __declspec(naked) wmDrawCursorStopped_hack1() {
	__asm {
		mov  ebx, eax;
		imul ebx, WMAP_WIN_WIDTH;
		retn;
	}
}

static void __declspec(naked) wmRefreshTabs_hook() {
	__asm {
		mov  eax, edx;
		imul eax, WMAP_WIN_WIDTH;
		sub  ebp, eax;
		retn;
	}
}

static void __declspec(naked) wmTownMapRefresh_hook() {
	__asm {
		cmp  edx, 700; //_wmTownWidth
		jl   scale;
		cmp  ebx, 682; //_wmTownHeight
		jl   scale;
		jmp  fo::funcoffs::buf_to_buf_;
scale:
		push WMAP_WIN_WIDTH; // to_width
		push 684;            // height
		push 702;            // width
		push eax;            // to_buff
		mov  ecx, edx;       // from_width
		mov  eax, esi;       // from_buff
		call fo::funcoffs::cscale_;
		retn; // don't delete
	}
}

void WorldmapViewportPatch() {
	if (Graphics::GetGameHeightRes() < WMAP_WIN_HEIGHT || Graphics::GetGameWidthRes() < WMAP_WIN_WIDTH) return;
	if (!fo::func::db_access("art\\intrface\\worldmap.frm")) return;
	dlog("Applying world map interface patch.", DL_INIT);

	mapSlotsScrollMax -= 216;
	if (mapSlotsScrollMax < 0) mapSlotsScrollMax = 0;

	fo::var::wmViewportRightScrlLimit = (350 * fo::var::wmNumHorizontalTiles) - (WMAP_WIN_WIDTH - (640 - 450));
	fo::var::wmViewportBottomtScrlLimit = (300 * (fo::var::wmMaxTileNum / fo::var::wmNumHorizontalTiles)) - (WMAP_WIN_HEIGHT - (480 - 443));

	SafeWriteBatch<DWORD>(135, {0x4C23BD, 0x4C2408}); // use unused worldmap.frm for new world map interface (wmInterfaceInit_)

	// x/y axis offset of interface window
	MakeJump(0x4C23A2, wmInterfaceInit_hack);
	// size of the created window/buffer
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH, wmWinWidth); // width
	SafeWrite32(0x4C238B, WMAP_WIN_HEIGHT);            // height (wmInterfaceInit_)

	// Mouse scrolling area (wmMouseBkProc_)
	SafeWrite32(0x4C331D, WMAP_WIN_WIDTH - 1);
	SafeWrite32(0x4C3337, WMAP_WIN_HEIGHT - 1);

	MakeCall(0x4C41A4, wmInterfaceDrawSubTileList_hack);   // 640 * 21
	MakeCall(0x4C4082, wmInterfaceDrawCircleOverlay_hack); // 640 * y
	MakeCall(0x4C4452, wmDrawCursorStopped_hack1);
	MakeCalls(wmDrawCursorStopped_hack0, {0x4C43BB, 0x4C42E1});
	MakeCall(0x4C5325, wmRefreshTabs_hook);
	HookCall(0x4C4BFF, wmTownMapRefresh_hook);

	// up/down buttons of the location list (wmInterfaceInit_)
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 480), { // offset by X (480)
		0x4C2D3C,
		0x4C2D7A
	});

	// town/world button (wmInterfaceInit_)
	SafeWrite32(0x4C2B9B, WMAP_WIN_HEIGHT - (480 - 439)); // offset by Y (439)
	SafeWrite32(0x4C2BAF, WMAP_WIN_WIDTH - (640 - 519));  // offset by X (508)

	// viewport size for mouse click
	SafeWriteBatch<DWORD>(WMAP_WIN_HEIGHT - (480 - 465), { // height/offset by Y (465 - 21 = 444 (443))
		0x4C0154, 0x4C02BA, // wmWorldMap_
		0x4C3A47,           // wmInterfaceRefresh_
	});
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 472), { // width/offset by X (472 - 22 = 450)
		0x4C0159, 0x4C02BF, // wmWorldMap_
		0x4C3A3A,           // wmInterfaceRefresh_
		0x4C417C, 0x4C4184  // wmInterfaceDrawSubTileList_
	});
	SafeWriteBatch<DWORD>(WMAP_WIN_HEIGHT - (480 - 464), { // width/offset by X (464)
		0x4C3FED,           // wmInterfaceDrawCircleOverlay_
		0x4C4157, 0x4C415F, // wmInterfaceDrawSubTileList_
	});
	// right limit of the viewport (450)
	SafeWriteBatch<DWORD>(WMAP_WIN_WIDTH - (640 - 450), wmViewportEndRight);   // 890 - 190 = 700 + 22 = 722
	// bottom limit of the viewport (443)
	SafeWriteBatch<DWORD>(WMAP_WIN_HEIGHT - (480 - 443), wmViewportEndBottom); // 720 - 37 = 683 + 21 = 704

	// Night/Day frm (wmRefreshInterfaceDial_)
	SafeWrite32(0x4C577F, WMAP_WIN_WIDTH - (640 - 532));                 // X offset (532)
	SafeWrite32(0x4C575D, (WMAP_WIN_WIDTH * 49) - ((640 * 49) - 31252)); // start offset in buffer (31252 / 640 = 49)

	// Date/Time frm
	SafeWrite32(0x4C3EC7, WMAP_WIN_WIDTH - (640 - 487));                 // start offset by X (487)
	SafeWrite32(0x4C3ED1, WMAP_WIN_WIDTH - (640 - 630));                 // end offset by X (630)
	SafeWrite32(0x4C3D10, (WMAP_WIN_WIDTH * 13) - ((640 * 13) - 8167));  // 8167 start offset in buffer (12327)
	SafeWrite32(0x4C3DC1, WMAP_WIN_WIDTH + (666 - 640)); // 666

	// WMCARMVE/WMGLOBE/WMSCREEN frms (wmRefreshInterfaceOverlay_)
	SafeWrite32(0x4C51D5, (WMAP_WIN_WIDTH * 577) - ((640 * 337) - 215554)); // start offset for image WMCARMVE
	SafeWrite32(0x4C5184, (WMAP_WIN_WIDTH * 571) - ((640 * 331) - 211695)); // start offset for image WMGLOBE
	SafeWrite32(0x4C51FD, (WMAP_WIN_WIDTH * 571) - ((640 * 331) - 211699)); // start offset for image WMSCREEN
	// Car gas indicator
	SafeWrite32(0x4C527E, (WMAP_WIN_WIDTH * 580) - ((640 * 340) - 217460)); // start offset in buffer (217460 / 640 = 340 + (720 - 480) = 580 Y-axes)

	// WMTABS.frm
	SafeWriteBatch<DWORD>((WMAP_WIN_WIDTH * 136) - ((640 * 136) - 86901), {
		0x4C52C4, 0x4C55AA // wmRefreshTabs_
	});
	SafeWrite32(0x4C52FF, (WMAP_WIN_WIDTH * 139) - ((640 * 139) - 88850)); // wmRefreshTabs_
	SafeWriteBatch<DWORD>((WMAP_WIN_WIDTH * 27), { // start offset in buffer (17280)
		0x4C54DE, 0x4C5424 // wmRefreshTabs_
	});
	SafeWrite32(0x4C52E5, WMAP_WIN_HEIGHT - 480 + 178 - 19); // height (178)

	// Buttons of cities, now 15
	SafeWrite32(0x4C2BD9, WMAP_WIN_WIDTH - (640 - 508)); // offset of the buttons by X (508)
	SafeWriteBatch<BYTE>(WMAP_TOWN_BUTTONS * 4, {0x4C2C01, 0x4C21B6, 0x4C2289}); // number of buttons (28 = 7 * 4)
	int btn = (mapSlotsScrollLimit) ? WMAP_TOWN_BUTTONS : WMAP_TOWN_BUTTONS + 1;
	SafeWrite32(0x4C21FD, 27 * btn); // scroll limit for buttons
	// number of city labels (6) wmRefreshTabs_
	SafeWriteBatch<BYTE>(WMAP_TOWN_BUTTONS - 1, {0x4C54F6, 0x4C542A});
	SafeWrite8(0x4C555E, 0);
	SafeWrite32(0x4C0348, 350 + WMAP_TOWN_BUTTONS); // buttons input code (wmWorldMap_)

	SafeWrite32(0x4C2BFB, (DWORD)&wmTownMapSubButtonIds[0]); // wmInterfaceInit_
	SafeWriteBatch<DWORD>((DWORD)&wmTownMapSubButtonIds[1], {
		0x4C22DD, 0x4C230A, // wmInterfaceScrollTabsUpdate_ (never called)
		0x4C227B,           // wmInterfaceScrollTabsStop_
		0x4C21A8            // wmInterfaceScrollTabsStart_
	});
	// WMTBEDGE.frm (wmRefreshTabs_)
	BlockCall(0x4C55BF);

	// Town map frm images (wmTownMapRefresh_)
	SafeWrite32(0x4C4BE4, (WMAP_WIN_WIDTH * 21) + 22); // start offset for town map image (13462)
	dlogr(" Done", DL_INIT);
}

void Worldmap::init() {
	PathfinderFixInit();
	StartingStatePatches();
	TimeLimitPatch();
	TownMapsHotkeyFix();
	WorldLimitsPatches();
	WorldmapFpsPatch();
	WorldMapInterfacePatch();
	PipBoyAutomapsPatch();

	if (*(DWORD*)0x4E4480 != 0x278805C7 // check if HRP is enabled
		&& GetConfigInt("Misc", "WorldMapInterface", 0))
	{
		LoadGameHook::OnAfterGameInit() += WorldmapViewportPatch; // Note: must be applied after WorldMapSlots patch
	}
	LoadGameHook::OnGameReset() += []() {
		SetCarInterfaceArt(433); // set index
		if (restTime) SetRestHealTime(180);
		RestRestore();
		mapRestInfo.clear();
	};
}

Delegate<>& Worldmap::OnWorldmapLoop() {
	return onWorldmapLoop;
}

}
