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

#include <unordered_map>
#include <math.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
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
std::unordered_map<int, levelRest> mapRestInfo;

std::vector<std::pair<long, std::string>> wmTerrainTypeNames; // pair first: x + y * number of horizontal sub-tiles

static bool restMap;
static bool restMode;
static bool restTime;

static DWORD worldMapDelay;
static DWORD worldMapTicks;

static DWORD WorldMapEncounterRate;

static double tickFract = 0.0;
static double mapMultiMod = 1.0;
static float scriptMapMulti = 1.0;

static bool addYear = false; // used as additional years indicator
static DWORD addedYears = 0;

static void __stdcall WorldmapLoopHook() {
	onWorldmapLoop.invoke();
}

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
		//pushadc;
		call WorldmapLoopHook;
		//popadc;
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
		add  esp, 4; // destroy the return address
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

	//if (GetConfigInt("Misc", "CitiesLimitFix", 0)) {
		dlog("Applying cities limit patch.", DL_INIT);
		if (*((BYTE*)0x4BF3BB) != 0xEB) {
			SafeWrite8(0x4BF3BB, 0xEB);
		}
		dlogr(" Done", DL_INIT);
	//}
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
		SafeWrite32(0x4C232D, 0xB8); // mov eax, 0; (wmInterfaceInit_)
		HookCall(0x4BFEE0, wmWorldMapFunc_hook);
		MakeCall(0x4C0667, wmRndEncounterOccurred_hack);
		dlogr(" Done", DL_INIT);
	}
}

void PathfinderFixInit() {
	//if (GetConfigInt("Misc", "PathfinderFix", 0)) {
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
	std::unordered_map<int, levelRest>::iterator it;
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

static const char* GetOverrideTerrainName(long x, long y) {
	if (wmTerrainTypeNames.empty()) return nullptr;

	long subTileID = x + y * (fo::var::wmNumHorizontalTiles * 7);
	auto it = std::find_if(wmTerrainTypeNames.crbegin(), wmTerrainTypeNames.crend(),
						  [=](const std::pair<long, std::string> &el)
						  { return el.first == subTileID; }
	);
	return (it != wmTerrainTypeNames.crend()) ? it->second.c_str() : nullptr;
}

// x, y - position of the sub-tile on the world map
void Worldmap::SetTerrainTypeName(long x, long y, const char* name) {
	long subTileID = x + y * (fo::var::wmNumHorizontalTiles * 7);
	wmTerrainTypeNames.push_back(std::make_pair(subTileID, name));
}

// TODO: someone might need to know the name of a terrain type?
const char* Worldmap::GetTerrainTypeName(long x, long y) {
	//const char* name = GetOverrideTerrainName(x, y);
	//return (name) ? name : fo::GetMessageStr(&fo::var::wmMsgFile, 1000 + fo::wmGetTerrainType(x, y));
	return nullptr;
}

// Returns the name of the terrain type in the position of the player's marker on the world map
const char* Worldmap::GetCurrentTerrainName() {
	const char* name = GetOverrideTerrainName(fo::var::world_xpos / 50, fo::var::world_ypos / 50);
	return (name) ? name : fo::GetMessageStr(&fo::var::wmMsgFile, 1000 + fo::wmGetCurrentTerrainType());
}

void Worldmap::init() {
	PathfinderFixInit();
	StartingStatePatches();
	TimeLimitPatch();
	TownMapsHotkeyFix();
	WorldLimitsPatches();
	WorldmapFpsPatch();
	PipBoyAutomapsPatch();

	LoadGameHook::OnGameReset() += []() {
		SetCarInterfaceArt(433); // set index
		if (restTime) SetRestHealTime(180);
		RestRestore();
		mapRestInfo.clear();
		wmTerrainTypeNames.clear();
	};
}

Delegate<>& Worldmap::OnWorldmapLoop() {
	return onWorldmapLoop;
}

}
