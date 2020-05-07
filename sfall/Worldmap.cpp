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
#include "FalloutEngine.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"
#include "SpeedPatch.h"

static DWORD AutomapPipboyList[AUTOMAP_MAX];

static DWORD ViewportX;
static DWORD ViewportY;

std::vector<std::pair<long, std::string>> wmTerrainTypeNames; // pair first: x + y * number of horizontal sub-tiles
std::tr1::unordered_map<long, std::string> wmAreaHotSpotTitle;

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
		call IsGameLoaded;
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
		//pushadc;
		call RunGlobalScripts3;
		//popadc;
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

static DWORD __stdcall PathfinderCalc(DWORD perkLevel, DWORD ticks) {
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

static void __declspec(naked) wmRndEncounterOccurred_hook() {
	__asm {
		push eax;
		mov  edx, 1;
		mov  dword ptr ds:[_wmRndCursorFid], 0;
		mov  ds:[_wmEncounterIconShow], edx;
		mov  ecx, 7;
jLoop:
		mov  eax, edx;
		sub  eax, ds:[_wmRndCursorFid];
		mov  ds:[_wmRndCursorFid], eax;
		call wmInterfaceRefresh_;
		mov  eax, 200;
		call block_for_tocks_;
		dec  ecx;
		jnz  jLoop;
		mov  ds:[_wmEncounterIconShow], ecx;
		pop  eax; // map id
		jmp  map_load_idx_;
	}
}

// Fallout 1 behavior: No radius for uncovered locations on the world map
// for the mark_area_known script function when the mark_state argument of the function is set to 3
long __declspec(naked) Wmap_AreaMarkStateIsNoRadius() {
	__asm {
		xor  eax, eax;
		cmp  esi, 3; // esi - mark_state value
		jne  skip;
		mov  esi, 1; // revert value to town known state
skip:
		cmove eax, esi; // eax: 1 for Fallout 1 behavior
		retn;
	}
}

static void WorldLimitsPatches() {
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

	//if (GetConfigInt("Misc", "CitiesLimitFix", 0)) {
		dlog("Applying cities limit patch.", DL_INIT);
		if (*((BYTE*)0x4BF3BB) != 0xEB) {
			SafeWrite8(0x4BF3BB, 0xEB);
		}
		dlogr(" Done", DL_INIT);
	//}
}

static void TimeLimitPatch() {
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
			const DWORD timerResetAddr[] = {
				0x4A34EF, // inc_game_time_
				0x4A3547  // inc_game_time_in_seconds_
			};
			MakeCalls(TimerReset, timerResetAddr);
			SafeMemSet(0x4A34F4, 0x90, 16);
			SafeMemSet(0x4A354C, 0x90, 16);
		} else {
			SafeWrite8(0x4A34EC, limit);
			SafeWrite8(0x4A3544, limit);
		}
		dlogr(" Done", DL_INIT);
	}
}

static void WorldmapFpsPatch() {
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
		availableGlobalScriptTypes |= 2;
	}

	if (GetConfigInt("Misc", "WorldMapEncounterFix", 0)) {
		dlog("Applying world map encounter patch.", DL_INIT);
		WorldMapEncounterRate = GetConfigInt("Misc", "WorldMapEncounterRate", 5);
		SafeWrite32(0x4C232D, 0xB8); // mov eax, 0; (wmInterfaceInit_)
		HookCall(0x4BFEE0, wmWorldMapFunc_hook);
		MakeCall(0x4C0667, wmRndEncounterOccurred_hack);
		dlogr(" Done", DL_INIT);
	}
}

static void PathfinderFixInit() {
	//if (GetConfigInt("Misc", "PathfinderFix", 0)) {
		dlog("Applying Pathfinder patch.", DL_INIT);
		SafeWrite16(0x4C1FF6, 0x9090);     // wmPartyWalkingStep_
		HookCall(0x4C1C78, PathfinderFix); // wmGameTimeIncrement_
		mapMultiMod = (double)GetConfigInt("Misc", "WorldMapTimeMod", 100) / 100.0;
		dlogr(" Done", DL_INIT);
	//}
}

static void StartingStatePatches() {
	int date = GetConfigInt("Misc", "StartYear", -1);
	if (date >= 0) {
		dlog("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
		dlogr(" Done", DL_INIT);
	}
	int month = GetConfigInt("Misc", "StartMonth", -1);
	if (month >= 0) {
		if (month > 11) month = 11;
		dlog("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, month);
		dlogr(" Done", DL_INIT);
	}
	date = GetConfigInt("Misc", "StartDay", -1);
	if (date >= 0) {
		if (month == 1 && date > 28) { // for February
			date = 28; // set 29th day
		} else if (date > 30) {
			date = 30; // set 31st day
		}
		dlog("Applying starting day patch.", DL_INIT);
		SafeWrite8(0x4A3356, static_cast<BYTE>(date));
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
		SafeWrite32(_wmWorldOffsetX, ViewportX);
		dlogr(" Done", DL_INIT);
	}
	ViewportY = GetConfigInt("Misc", "ViewYPos", -1);
	if (ViewportY != -1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(_wmWorldOffsetY, ViewportY);
		dlogr(" Done", DL_INIT);
	}
	if (ViewportX != -1 || ViewportY != -1) HookCall(0x4BCF07, ViewportHook); // game_reset_
}

static void PipBoyAutomapsPatch() {
	dlog("Applying Pip-Boy automaps patch.", DL_INIT);
	MakeCall(0x4BF931, wmMapInit_hack, 2);
	SafeWrite32(0x41B8B7, (DWORD)AutomapPipboyList);
	memcpy(AutomapPipboyList, (void*)_displayMapList, sizeof(AutomapPipboyList)); // copy vanilla data
	dlogr(" Done", DL_INIT);
}

void __stdcall SetMapMulti(float value) {
	scriptMapMulti = value;
}

void SetAddedYears(DWORD years) {
	addedYears = years;
}

DWORD GetAddedYears(bool isCheck) {
	return (isCheck && !addYear) ? 0 : addedYears;
}

static const char* GetOverrideTerrainName(long x, long y) {
	if (wmTerrainTypeNames.empty()) return nullptr;

	long subTileID = x + y * (*ptr_wmNumHorizontalTiles * 7);
	auto it = std::find_if(wmTerrainTypeNames.crbegin(), wmTerrainTypeNames.crend(),
						  [=](const std::pair<long, std::string> &el)
						  { return el.first == subTileID; }
	);
	return (it != wmTerrainTypeNames.crend()) ? it->second.c_str() : nullptr;
}

// x, y - position of the sub-tile on the world map
void Wmap_SetTerrainTypeName(long x, long y, const char* name) {
	long subTileID = x + y * (*ptr_wmNumHorizontalTiles * 7);
	wmTerrainTypeNames.push_back(std::make_pair(subTileID, name));
}

// TODO: someone might need to know the name of a terrain type?
/*const char* Wmap_GetTerrainTypeName(long x, long y) {
	const char* name = GetOverrideTerrainName(x, y);
	return (name) ? name : fo::GetMessageStr(&fo::var::wmMsgFile, 1000 + fo::wmGetTerrainType(x, y));
}*/

// Returns the name of the terrain type in the position of the player's marker on the world map
const char* Wmap_GetCurrentTerrainName() {
	const char* name = GetOverrideTerrainName(*ptr_world_xpos / 50, *ptr_world_ypos / 50);
	return (name) ? name : GetMessageStr(ptr_wmMsgFile, 1000 + wmGetCurrentTerrainType());
}

bool Wmap_AreaTitlesIsEmpty() {
	return wmAreaHotSpotTitle.empty();
}

void Wmap_SetCustomAreaTitle(long areaID, const char* msg) {
	wmAreaHotSpotTitle[areaID] = msg;
}

const char* Wmap_GetCustomAreaTitle(long areaID) {
	if (Wmap_AreaTitlesIsEmpty()) return nullptr;
	const auto &it = wmAreaHotSpotTitle.find(areaID);
	return (it != wmAreaHotSpotTitle.cend()) ? it->second.c_str() : nullptr;
}

void Worldmap_OnGameLoad() {
	wmTerrainTypeNames.clear();
	wmAreaHotSpotTitle.clear();
}

void WorldmapInit() {
	PathfinderFixInit();
	StartingStatePatches();
	TimeLimitPatch();
	WorldLimitsPatches();
	WorldmapFpsPatch();
	PipBoyAutomapsPatch();

	// Add a flashing icon to the Horrigan encounter
	HookCall(0x4C071C, wmRndEncounterOccurred_hook);
}
