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

#include <algorithm>
#include <unordered_map>
#include <math.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"
#include "SpeedPatch.h"

#include "..\HRP\viewmap\ViewMap.h"

#include "Worldmap.h"

namespace sfall
{

static Delegate<> onWorldmapLoop;

static DWORD AutomapPipboyList[AUTOMAP_MAX];

static DWORD ViewportX;
static DWORD ViewportY;

#pragma pack(push, 1)
struct levelRest {
	char level[4];
};
#pragma pack(pop)

static std::unordered_map<int, levelRest> mapRestInfo;

static std::vector<std::pair<long, std::string>> wmTerrainTypeNames; // pair first: x + y * number of horizontal sub-tiles
static std::unordered_map<long, std::string> wmAreaHotSpotTitle;

static bool restMap;
static bool restMode;
static bool restTime;

static bool worldMapLongDelay = false;
static DWORD worldMapDelay;
static DWORD worldMapTicks;

static DWORD WorldMapEncounterRate;

static constexpr long WorldMapHealingDefaultInterval = 180 * 60 * 10; // 3 hrs
// Healing interval in game time
static long worldMapHealingInterval = WorldMapHealingDefaultInterval;
static DWORD worldMapLastHealTime;

static double tickFract = 0.0;
static double mapMultiMod = 1.0;
static float scriptMapMulti = 1.0f;

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
	fo::Queue* queue = fo::var::queue;
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

static void WorldMapFPS() {
	DWORD prevTicks = worldMapTicks; // previous ticks
	do {
		WorldmapLoopHook();
		if (worldMapLongDelay) {
			__asm call fo::funcoffs::process_bk_;
		}

		DWORD tick; // current ticks
		do {
			tick = SpeedPatch::getTickCount();
		} while (tick == prevTicks); // delay ~15 ms
		prevTicks = tick;

		// get elapsed time
	} while ((prevTicks - worldMapTicks) < worldMapDelay);
	worldMapTicks = prevTicks;
}

static __declspec(naked) void wmWorldMap_hook_patch1() {
	__asm {
		push dword ptr ds:[FO_VAR_last_buttons];
		push dword ptr ds:[FO_VAR_mouse_buttons];
		call WorldMapFPS;
		pop  dword ptr ds:[FO_VAR_mouse_buttons];
		pop  dword ptr ds:[FO_VAR_last_buttons];
		jmp  fo::funcoffs::get_input_;
	}
}

static __declspec(naked) void wmWorldMap_hook_patch2() {
	__asm {
		call WorldMapFPS;
		jmp  fo::funcoffs::get_input_;
	}
}

// Only used if the world map speed patch is disabled, so that world map scripts are still run
static __declspec(naked) void wmWorldMap_hook() {
	__asm {
		call ds:[SpeedPatch::getTickCountOffs]; // current ticks
		cmp  eax, worldMapTicks;
		je   skipHook;
		mov  worldMapTicks, eax;
		call WorldmapLoopHook; // hook is called every ~15 ms (GetTickCount returns a difference of 14-16 ms)
skipHook:
		jmp  fo::funcoffs::get_input_;
	}
}

static __declspec(naked) void wmWorldMapFunc_hook() {
	__asm {
		inc  dword ptr ds:[FO_VAR_wmLastRndTime];
		jmp  fo::funcoffs::wmPartyWalkingStep_;
	}
}

static __declspec(naked) void wmRndEncounterOccurred_hack() {
	__asm { // edx - _wmLastRndTime
		xor  ecx, ecx;
		cmp  edx, WorldMapEncounterRate;
		retn;
	}
}

static __declspec(naked) void ViewportHook() {
	__asm {
		call fo::funcoffs::wmWorldMapLoadTempData_;
		mov  eax, ViewportX;
		mov  ds:[FO_VAR_wmWorldOffsetX], eax;
		mov  eax, ViewportY;
		mov  ds:[FO_VAR_wmWorldOffsetY], eax;
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
	tickFract = std::modf(multi, &multi);

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

static __declspec(naked) void critter_can_obj_dude_rest_hook() {
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

static __declspec(naked) void wmMapInit_hack() {
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

static __declspec(naked) void wmRndEncounterOccurred_hook() {
	__asm {
		push eax;
		mov  edx, 1;
		mov  dword ptr ds:[FO_VAR_wmRndCursorFid], 0;
		mov  ds:[FO_VAR_wmEncounterIconShow], edx;
		mov  ecx, 7;
jLoop:
		mov  eax, edx;
		sub  eax, ds:[FO_VAR_wmRndCursorFid];
		mov  ds:[FO_VAR_wmRndCursorFid], eax;
		call fo::funcoffs::wmInterfaceRefresh_;
		mov  eax, 200;
		call fo::funcoffs::block_for_tocks_;
		dec  ecx;
		jnz  jLoop;
		mov  ds:[FO_VAR_wmEncounterIconShow], ecx;
		pop  eax; // map id
		jmp  fo::funcoffs::map_load_idx_;
	}
}

// Fallout 1 behavior: No radius for uncovered locations on the world map
// for the mark_area_known script function when the mark_state argument of the function is set to 3
__declspec(naked) long Worldmap::AreaMarkStateIsNoRadius() {
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

static bool customPosition = false;

static __declspec(naked) void main_load_new_hook() {
	__asm {
		call fo::funcoffs::map_load_;
		push edx;
		sub  esp, 4; // buff outAreaID
		mov  edx, esp;
		mov  eax, dword ptr ds:[FO_VAR_map_number];
		call fo::funcoffs::wmMatchAreaContainingMapIdx_;
		pop  eax; // area ID
		cmp  customPosition, 0;
		jnz  skip;
		call fo::funcoffs::wmTeleportToArea_;
		pop  edx;
		retn;
skip:
		pop  edx;
		test eax, eax;
		js   end;
		cmp  eax, dword ptr ds:[FO_VAR_wmMaxAreaNum];
		jge  end;
		mov  dword ptr ds:[FO_VAR_WorldMapCurrArea], eax;
end:
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

static void MapLimitsPatches() {
	// This has priority over the SCROLL_DIST_X/Y options in f2_res.ini
	long data = IniReader::GetConfigInt("Misc", "LocalMapXLimit", 0);
	if (data > 0) {
		dlogr("Applying local map x limit patch.", DL_INIT);
		SafeWrite32(0x4B13B9, data);
		HRP::ViewMap::SCROLL_DIST_X = data;
	}
	data = IniReader::GetConfigInt("Misc", "LocalMapYLimit", 0);
	if (data > 0) {
		dlogr("Applying local map y limit patch.", DL_INIT);
		SafeWrite32(0x4B13C7, data);
		HRP::ViewMap::SCROLL_DIST_Y = data;
	}

	//if (IniReader::GetConfigInt("Misc", "CitiesLimitFix", 0)) {
		dlogr("Applying cities limit patch.", DL_INIT);
		if (*((BYTE*)0x4BF3BB) != CodeType::JumpShort) {
			SafeWrite8(0x4BF3BB, CodeType::JumpShort); // wmAreaInit_
		}
	//}
}

static void TimeLimitPatch() {
	int limit = IniReader::GetConfigInt("Misc", "TimeLimit", 13);
	if (limit == -2 || limit == -3) {
		addYear = true;
		MakeCall(0x4A33B8, TimeDateFix, 1); // game_time_date_
		limit = -1; // also reset time
	}
	if (limit >= -1 && limit < 13) {
		dlogr("Applying time limit patch.", DL_INIT);
		if (limit == -1) {
			MakeCall(0x4A3DF5, script_chk_timed_events_hack, 1);
			MakeCall(0x4A3488, set_game_time_hack);
			MakeCalls(TimerReset, {
				0x4A34EF, // inc_game_time_
				0x4A3547  // inc_game_time_in_seconds_
			});
			SafeMemSet(0x4A34F4, CodeType::Nop, 16);
			SafeMemSet(0x4A354C, CodeType::Nop, 16);
		} else {
			SafeWrite8(0x4A34EC, limit);
			SafeWrite8(0x4A3544, limit);
		}
	}
}

static void WorldmapFpsPatch() {
	bool fpsPatchOK = (*(DWORD*)0x4BFE5E == 0x8D16);
	if (IniReader::GetConfigInt("Misc", "WorldMapFPSPatch", 0)) {
		dlog("Applying world map fps patch.", DL_INIT);
		if (fpsPatchOK) {
			int delay = IniReader::GetConfigInt("Misc", "WorldMapDelay2", 66);
			worldMapDelay = max(1, delay);
			dlogr(" Done", DL_INIT);
		} else {
			dlogr(" Failed", DL_INIT);
		}
	}
	if (fpsPatchOK) {
		void* func;
		if (worldMapDelay == 0) {
			func = wmWorldMap_hook; // only WorldmapLoop hook
		} else if (worldMapDelay > 25) {
			worldMapLongDelay = true;
			func = wmWorldMap_hook_patch1;
		} else {
			func = wmWorldMap_hook_patch2;
		}
		HookCall(0x4BFE5D, func); // wmWorldMap_
		::sfall::availableGlobalScriptTypes |= 2;
	}

	if (IniReader::GetConfigInt("Misc", "WorldMapEncounterFix", 0)) {
		dlogr("Applying world map encounter patch.", DL_INIT);
		WorldMapEncounterRate = IniReader::GetConfigInt("Misc", "WorldMapEncounterRate", 5);
		SafeWrite32(0x4C232D, 0xB8); // mov eax, 0; (wmInterfaceInit_)
		HookCall(0x4BFEE0, wmWorldMapFunc_hook);
		MakeCall(0x4C0667, wmRndEncounterOccurred_hack);
	}
}

// Original function checks how many system Ticks (not game ticks) passed since last WM loop iteration and if more than 1000 ms,
// applies one healing to the whole party according to their healing rate.
// So we hijack it and return number bigger than 1000 to force the event, or 0 to skip it.
static DWORD __fastcall wmWorldMap_HealingTimeElapsed(DWORD elapsedTocks) {
	if (worldMapHealingInterval == 0) return elapsedTocks;
	if (worldMapHealingInterval > 0 && (fo::var::fallout_game_time - worldMapLastHealTime > (DWORD)worldMapHealingInterval)) {
		worldMapLastHealTime = fo::var::fallout_game_time;
		return 10000; // force healing
	}
	return 0; // skip healing
}

static __declspec(naked) void wmWorldMap_elapsed_tocks_hook() {
	__asm {
		push ecx;
		call fo::funcoffs::elapsed_tocks_;
		mov  ecx, eax;
		call wmWorldMap_HealingTimeElapsed;
		pop  ecx;
		retn;
	}
}

static void WorldmapHealingRatePatch() {
	dlogr("Applying world map healing rate patch.", DL_INIT);
	HookCall(0x4C0085, wmWorldMap_elapsed_tocks_hook);
	LoadGameHook::OnGameModeChange() += [](DWORD state) {
		if (InWorldMap()) worldMapLastHealTime = fo::var::fallout_game_time;
	};
	LoadGameHook::OnGameReset() += []() {
		worldMapHealingInterval = WorldMapHealingDefaultInterval;
	};
}

static void PathfinderFixInit() {
	//if (IniReader::GetConfigInt("Misc", "PathfinderFix", 0)) {
		dlogr("Applying Pathfinder patch.", DL_INIT);
		SafeWrite16(0x4C1FF6, 0x9090);     // wmPartyWalkingStep_
		HookCall(0x4C1C78, PathfinderFix); // wmGameTimeIncrement_
		mapMultiMod = (double)IniReader::GetConfigInt("Misc", "WorldMapTimeMod", 100) / 100.0;
	//}
}

static void StartingStatePatches() {
	int date = IniReader::GetConfigInt("Misc", "StartYear", -1);
	if (date >= 0) {
		dlogr("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
	}
	int month = IniReader::GetConfigInt("Misc", "StartMonth", -1);
	if (month >= 0) {
		if (month > 11) month = 11;
		dlogr("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, month);
	}
	date = IniReader::GetConfigInt("Misc", "StartDay", -1);
	if (date >= 0) {
		if (month == 1 && date > 28) { // for February
			date = 28; // set 29th day
		} else if (date > 30) {
			date = 30; // set 31st day
		}
		dlogr("Applying starting day patch.", DL_INIT);
		SafeWrite8(0x4A3356, static_cast<BYTE>(date));
	}

	long xPos = IniReader::GetConfigInt("Misc", "StartXPos", -1);
	if (xPos != -1) {
		if (xPos < 0) xPos = 0;
		dlogr("Applying starting x position patch.", DL_INIT);
		SafeWrite32(0x4BC990, xPos);
		SafeWrite32(0x4BCC08, xPos);
		customPosition = true;
	}
	long yPos = IniReader::GetConfigInt("Misc", "StartYPos", -1);
	if (yPos != -1) {
		if (yPos < 0) yPos = 0;
		dlogr("Applying starting y position patch.", DL_INIT);
		SafeWrite32(0x4BC995, yPos);
		SafeWrite32(0x4BCC0D, yPos);
		customPosition = true;
	}

	// Fix the starting position of the player's marker on the world map when starting a new game
	// also initialize the value of the current area for METARULE_CURRENT_TOWN metarule function
	HookCall(0x480DC0, main_load_new_hook);

	xPos = IniReader::GetConfigInt("Misc", "ViewXPos", -1);
	if (xPos != -1) {
		if (xPos < 0) xPos = 0;
		ViewportX = xPos;
		dlogr("Applying starting x view patch.", DL_INIT);
		SafeWrite32(FO_VAR_wmWorldOffsetX, ViewportX);
	}
	yPos = IniReader::GetConfigInt("Misc", "ViewYPos", -1);
	if (yPos != -1) {
		if (yPos < 0) yPos = 0;
		ViewportY = yPos;
		dlogr("Applying starting y view patch.", DL_INIT);
		SafeWrite32(FO_VAR_wmWorldOffsetY, ViewportY);
	}
	if (xPos != -1 || yPos != -1) HookCall(0x4BCF07, ViewportHook); // game_reset_
}

static void PipBoyAutomapsPatch() {
	dlogr("Applying Pip-Boy automaps patch.", DL_INIT);
	MakeCall(0x4BF931, wmMapInit_hack, 2);
	SafeWrite32(0x41B8B7, (DWORD)AutomapPipboyList);
	memcpy(AutomapPipboyList, (void*)FO_VAR_displayMapList, sizeof(AutomapPipboyList)); // copy vanilla data
}

void Worldmap::SaveData(HANDLE file) {
	DWORD sizeWrite, count = mapRestInfo.size();
	WriteFile(file, &count, 4, &sizeWrite, 0);
	std::unordered_map<int, levelRest>::iterator it;
	for (it = mapRestInfo.begin(); it != mapRestInfo.end(); ++it) {
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
		mapRestInfo.emplace(mID, elevData);
	}
	if (count && !restMap) {
		HookCall(0x42E57A, critter_can_obj_dude_rest_hook);
		restMap = true;
	}
	return false;
}

void Worldmap::SetMapMulti(float value) {
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

void Worldmap::SetRestHealTime(long minutes) {
	if (minutes > 0) {
		SafeWrite32(0x499FDE, minutes);
		restTime = (minutes != 180);
	}
}

void Worldmap::SetWorldMapHealTime(long minutes) {
	worldMapHealingInterval = (minutes >= 0)
	                        ? minutes * 60 * 10
	                        : -1;
	worldMapLastHealTime = fo::var::fallout_game_time;
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
		SafeWrite16(0x499FD4, 0x9090); // Check4Health_
		SafeWrite16(0x499E93, 0x8FEB); // TimedRest_: jmp  0x499E24
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
		mapRestInfo.emplace(mapId, elevData);
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
		[=](const std::pair<long, std::string> &el) { return el.first == subTileID; }
	);
	return (it != wmTerrainTypeNames.crend()) ? it->second.c_str() : nullptr;
}

// x, y - position of the sub-tile on the world map
void Worldmap::SetTerrainTypeName(long x, long y, const char* name) {
	long subTileID = x + y * (fo::var::wmNumHorizontalTiles * 7);
	wmTerrainTypeNames.push_back(std::make_pair(subTileID, name));
}

// Returns the name of the terrain type at the specified coordinates on the world map
const char* Worldmap::GetTerrainTypeName(long x, long y) {
	const char* name = GetOverrideTerrainName(x, y);
	return (name) ? name : fo::util::GetMessageStr(&fo::var::wmMsgFile, 1000 + fo::util::wmGetTerrainType(x * 50, y * 50));
}

// Returns the name of the terrain type at the position of the player's marker on the world map
const char* Worldmap::GetCurrentTerrainName() {
	const char* name = GetOverrideTerrainName(fo::var::world_xpos / 50, fo::var::world_ypos / 50);
	return (name) ? name : fo::util::GetMessageStr(&fo::var::wmMsgFile, 1000 + fo::util::wmGetCurrentTerrainType());
}

bool Worldmap::AreaTitlesIsEmpty() {
	return wmAreaHotSpotTitle.empty();
}

void Worldmap::SetCustomAreaTitle(long areaID, const char* msg) {
	wmAreaHotSpotTitle[areaID] = msg;
}

const char* Worldmap::GetCustomAreaTitle(long areaID) {
	if (AreaTitlesIsEmpty()) return nullptr;
	const auto &it = wmAreaHotSpotTitle.find(areaID);
	return (it != wmAreaHotSpotTitle.cend()) ? it->second.c_str() : nullptr;
}

void Worldmap::init() {
	PathfinderFixInit();
	StartingStatePatches();
	TimeLimitPatch();
	MapLimitsPatches();
	WorldmapFpsPatch();
	PipBoyAutomapsPatch();
	WorldmapHealingRatePatch();

	// Add a flashing icon to the Horrigan encounter
	HookCall(0x4C071C, wmRndEncounterOccurred_hook);

	LoadGameHook::OnGameReset() += []() {
		SetCarInterfaceArt(433); // set index
		if (restTime) SetRestHealTime(180);
		RestRestore();
		mapRestInfo.clear();
		wmTerrainTypeNames.clear();
		wmAreaHotSpotTitle.clear();
	};
}

Delegate<>& Worldmap::OnWorldmapLoop() {
	return onWorldmapLoop;
}

}
