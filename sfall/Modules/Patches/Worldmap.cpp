/*
*    sfall
*    Copyright (C) 2008-2016  The sfall team
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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\ScriptExtender.h"

#include "Worldmap.h"

static DWORD ViewportX;
static DWORD ViewportY;

static __declspec(naked) void GetDateWrapper() {
	__asm {
		push ecx;
		push esi;
		push ebx;
		call FuncOffs::game_time_date_;
		mov ecx, ds:[VARPTR_pc_proto + 0x4C];
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
	VarPtr::fallout_game_time = 0;
	// used as additional years indicator
	VarPtr::pc_proto.base_stat_unarmed_damage += 13;
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
		call FuncOffs::wmInterfaceScrollTabsStart_;
end:
		pop ebx;
		retn;
	}
}

static DWORD wp_delay;
static void __declspec(naked) worldmap_patch() {
	__asm {
		pushad;
		call RunGlobalScripts3;
		mov ecx, wp_delay;
tck:
		mov eax, ds : [0x50fb08];
		call FuncOffs::elapsed_time_;
		cmp eax, ecx;
		jl tck;
		call FuncOffs::get_time_;
		mov ds : [0x50fb08], eax;
		popad;
		jmp FuncOffs::get_input_;
	}
}

static void __declspec(naked) WorldMapEncPatch1() {
	__asm {
		inc dword ptr ds : [VARPTR_wmLastRndTime]
		call FuncOffs::wmPartyWalkingStep_;
		retn;
	}
}

static void __declspec(naked) WorldMapEncPatch2() {
	__asm {
		mov dword ptr ds : [VARPTR_wmLastRndTime], 0;
		retn;
	}
}

static void __declspec(naked) WorldMapEncPatch3() {
	__asm {
		mov eax, ds:[VARPTR_wmLastRndTime];
		retn;
	}
}

static DWORD WorldMapEncounterRate;
static void __declspec(naked) wmWorldMapFunc_hook() {
	__asm {
		inc  dword ptr ds:[VARPTR_wmLastRndTime];
		jmp  FuncOffs::wmPartyWalkingStep_;
	}
}

static void __declspec(naked) wmRndEncounterOccurred_hack() {
	__asm {
		xor  ecx, ecx;
		cmp  edx, WorldMapEncounterRate;
		retn;
	}
}

static double wm_nexttick;
static double wm_wait;
static bool wm_usingperf;
static __int64 wm_perfadd;
static __int64 wm_perfnext;
static DWORD WorldMapLoopCount;
static void WorldMapSpeedPatch3() {
	RunGlobalScripts3();
	if (wm_usingperf) {
		__int64 timer;
		while (true) {
			QueryPerformanceCounter((LARGE_INTEGER*)&timer);
			if (timer > wm_perfnext) break;
			Sleep(0);
		}
		if (wm_perfnext + wm_perfadd < timer) wm_perfnext = timer + wm_perfadd;
		else wm_perfnext += wm_perfadd;
	} else {
		DWORD tick;
		DWORD nexttick = (DWORD)wm_nexttick;
		while ((tick = GetTickCount()) < nexttick) Sleep(0);
		if (nexttick + wm_wait < tick) wm_nexttick = tick + wm_wait;
		else wm_nexttick += wm_wait;
	}
}

static void __declspec(naked) WorldMapSpeedPatch2() {
	__asm {
		pushad;
		call WorldMapSpeedPatch3;
		popad;
		call FuncOffs::get_input_;
		retn;
	}
}

static void __declspec(naked) WorldMapSpeedPatch() {
	__asm {
		pushad;
		call RunGlobalScripts3;
		popad;
		push ecx;
		mov ecx, WorldMapLoopCount;
ls:
		mov eax, eax;
		loop ls;
		pop ecx;
		call FuncOffs::get_input_;
		retn;
	}
}

//Only used if the world map speed patch is disabled, so that world map scripts are still run
static void WorldMapHook() {
	__asm {
		pushad;
		call RunGlobalScripts3;
		popad;
		call FuncOffs::get_input_;
		retn;
	}
}

static void __declspec(naked) ViewportHook() {
	__asm {
		call FuncOffs::wmWorldMapLoadTempData_;
		mov eax, ViewportX;
		mov ds : [VARPTR_wmWorldOffsetX], eax
		mov eax, ViewportY;
		mov ds : [VARPTR_wmWorldOffsetY], eax;
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
	__asm {
		push eax;
		mov eax, ds:[VARPTR_obj_dude];
		mov edx, PERK_pathfinder;
		call FuncOffs::perk_level_;
		push eax;
		call PathfinderFix2;
		call FuncOffs::inc_game_time_;
		retn;
	}
}

void ApplyWorldLimitsPatches() {
	DWORD date = GetPrivateProfileInt("Misc", "LocalMapXLimit", 0, ini);
	if (date) {
		dlog("Applying local map x limit patch.", DL_INIT);
		SafeWrite32(0x004B13B9, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileInt("Misc", "LocalMapYLimit", 0, ini);
	if (date) {
		dlog("Applying local map y limit patch.", DL_INIT);
		SafeWrite32(0x004B13C7, date);
		dlogr(" Done", DL_INIT);
	}

	//if(GetPrivateProfileIntA("Misc", "WorldMapCitiesListFix", 0, ini)) {
	dlog("Applying world map cities list patch.", DL_INIT);

	SafeWrite32(0x004C04BA, ((DWORD)&ScrollCityListHook) - 0x004C04BE);
	SafeWrite32(0x004C04C9, ((DWORD)&ScrollCityListHook) - 0x004C04CD);
	SafeWrite32(0x004C4A35, ((DWORD)&ScrollCityListHook) - 0x004C4A39);
	SafeWrite32(0x004C4A3E, ((DWORD)&ScrollCityListHook) - 0x004C4A42);
	dlogr(" Done", DL_INIT);
	//}

	//if(GetPrivateProfileIntA("Misc", "CitiesLimitFix", 0, ini)) {
	dlog("Applying cities limit patch.", DL_INIT);
	if (*((BYTE*)0x004BF3BB) != 0xeb) {
		SafeWrite8(0x004BF3BB, 0xeb);
	}
	dlogr(" Done", DL_INIT);
	//}

	DWORD wmSlots = GetPrivateProfileIntA("Misc", "WorldMapSlots", 0, ini);
	if (wmSlots && wmSlots < 128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if (wmSlots < 7) wmSlots = 7;
		mapSlotsScrollMax = (wmSlots - 7) * 27;
		if (wmSlots < 25) SafeWrite32(0x004C21FD, 230 - (wmSlots - 17) * 27);
		else {
			SafeWrite8(0x004C21FC, 0xC2);
			SafeWrite32(0x004C21FD, 2 + 27 * (wmSlots - 26));
		}
		dlogr(" Done", DL_INIT);
	}
}

void ApplyTimeLimitPatch() {
	int limit = GetPrivateProfileIntA("Misc", "TimeLimit", 13, ini);
	if (limit == -2) {
		limit = 14;
	}
	if (limit == -3) {
		dlog("Applying time limit patch (-3).", DL_INIT);
		limit = -1;
		AddUnarmedStatToGetYear = 1;

		SafeWrite32(0x004392F9, ((DWORD)&GetDateWrapper) - 0x004392Fd);
		SafeWrite32(0x00443809, ((DWORD)&GetDateWrapper) - 0x0044380d);
		SafeWrite32(0x0047E128, ((DWORD)&GetDateWrapper) - 0x0047E12c);
		SafeWrite32(0x004975A3, ((DWORD)&GetDateWrapper) - 0x004975A7);
		SafeWrite32(0x00497713, ((DWORD)&GetDateWrapper) - 0x00497717);
		SafeWrite32(0x004979Ca, ((DWORD)&GetDateWrapper) - 0x004979Ce);
		SafeWrite32(0x004C3CB6, ((DWORD)&GetDateWrapper) - 0x004C3CBa);
		dlogr(" Done", DL_INIT);
	}

	if (limit <= 14 && limit >= -1 && limit != 13) {
		dlog("Applying time limit patch.", DL_INIT);
		if (limit == -1) {
			SafeWrite32(0x004A34Fa, ((DWORD)&TimerReset) - 0x004A34Fe);
			SafeWrite32(0x004A3552, ((DWORD)&TimerReset) - 0x004A3556);

			SafeWrite32(0x004A34EF, 0x90909090);
			SafeWrite32(0x004A34f3, 0x90909090);
			SafeWrite16(0x004A34f7, 0x9090);
			SafeWrite32(0x004A34FE, 0x90909090);
			SafeWrite16(0x004A3502, 0x9090);

			SafeWrite32(0x004A3547, 0x90909090);
			SafeWrite32(0x004A354b, 0x90909090);
			SafeWrite16(0x004A354f, 0x9090);
			SafeWrite32(0x004A3556, 0x90909090);
			SafeWrite16(0x004A355a, 0x9090);
		} else {
			SafeWrite8(0x004A34EC, limit);
			SafeWrite8(0x004A3544, limit);
		}
		dlogr(" Done", DL_INIT);
	}
}

void ApplyTownMapsHotkeyFix() {
	if (GetPrivateProfileIntA("Misc", "TownMapHotkeysFix", 1, ini)) {
		dlog("Applying town map hotkeys patch.", DL_INIT);
		MakeCall(0x4C4945, &wmTownMapFunc_hack, false);
		dlogr(" Done", DL_INIT);
	}
}

void ApplyWorldmapFpsPatch() {
	DWORD tmp;
	if (GetPrivateProfileInt("Misc", "WorldMapFPSPatch", 0, ini)) {
		dlog("Applying world map fps patch.", DL_INIT);
		if (*(DWORD*)0x004BFE5E != 0x8d16) {
			dlogr(" Failed", DL_INIT);
		} else {
			wp_delay = GetPrivateProfileInt("Misc", "WorldMapDelay2", 66, ini);
			HookCall(0x004BFE5D, worldmap_patch);
			dlogr(" Done", DL_INIT);
		}
	} else {
		tmp = GetPrivateProfileIntA("Misc", "WorldMapFPS", 0, ini);
		if (tmp) {
			dlog("Applying world map fps patch.", DL_INIT);
			if (*((WORD*)0x004CAFB9) == 0x0000) {
				AvailableGlobalScriptTypes |= 2;
				SafeWrite32(0x004BFE5E, ((DWORD)&WorldMapSpeedPatch2) - 0x004BFE62);
				if (GetPrivateProfileIntA("Misc", "ForceLowResolutionTimer", 0, ini) || !QueryPerformanceFrequency((LARGE_INTEGER*)&wm_perfadd) || wm_perfadd <= 1000) {
					wm_wait = 1000.0 / (double)tmp;
					wm_nexttick = GetTickCount();
					wm_usingperf = false;
				} else {
					wm_usingperf = true;
					wm_perfadd /= tmp;
					wm_perfnext = 0;
				}
			}
			dlogr(" Done", DL_INIT);
		} else {
			tmp = GetPrivateProfileIntA("Misc", "WorldMapDelay", 0, ini);
			if (tmp) {
				if (*((WORD*)0x004CAFB9) == 0x3d40)
					SafeWrite32(0x004CAFBB, tmp);
				else if (*((WORD*)0x004CAFB9) == 0x0000) {
					SafeWrite32(0x004BFE5E, ((DWORD)&WorldMapSpeedPatch) - 0x004BFE62);
					WorldMapLoopCount = tmp;
				}
			} else {
				if (*(DWORD*)0x004BFE5E == 0x0000d816) {
					SafeWrite32(0x004BFE5E, ((DWORD)&WorldMapHook) - 0x004BFE62);
				}
			}
		}
		if (GetPrivateProfileIntA("Misc", "WorldMapEncounterFix", 0, ini)) {
			dlog("Applying world map encounter patch.", DL_INIT);
			WorldMapEncounterRate = GetPrivateProfileIntA("Misc", "WorldMapEncounterRate", 5, ini);
			SafeWrite32(0x4C232D, 0x01EBC031);        // xor eax, eax; jmps 0x4C2332
			HookCall(0x4BFEE0, &wmWorldMapFunc_hook);
			MakeCall(0x4C0667, &wmRndEncounterOccurred_hack, false);
			dlogr(" Done", DL_INIT);
		}
	}
}

void ApplyPathfinderFix() {
	//if(GetPrivateProfileIntA("Misc", "PathfinderFix", 0, ini)) {
	dlog("Applying pathfinder patch.", DL_INIT);
	SafeWrite32(0x004C1FF2, ((DWORD)&PathfinderFix3) - 0x004c1ff6);
	SafeWrite32(0x004C1C79, ((DWORD)&PathfinderFix) - 0x004c1c7d);
	MapMulti = (double)GetPrivateProfileIntA("Misc", "WorldMapTimeMod", 100, ini) / 100.0;
	dlogr(" Done", DL_INIT);
//}
}

void ApplyStartingStatePatches() {
	int date = GetPrivateProfileInt("Misc", "StartYear", -1, ini);
	if (date > 0) {
		dlog("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileInt("Misc", "StartMonth", -1, ini);
	if (date >= 0 && date < 12) {
		dlog("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileInt("Misc", "StartDay", -1, ini);
	if (date >= 0 && date < 31) {
		dlog("Applying starting day patch.", DL_INIT);
		SafeWrite8(0x4A3356, date);
		dlogr(" Done", DL_INIT);
	}

	date = GetPrivateProfileInt("Misc", "StartXPos", -1, ini);
	if (date != -1) {
		dlog("Applying starting x position patch.", DL_INIT);
		SafeWrite32(0x4BC990, date);
		SafeWrite32(0x4BCC08, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileInt("Misc", "StartYPos", -1, ini);
	if (date != -1) {
		dlog("Applying starting y position patch.", DL_INIT);
		SafeWrite32(0x4BC995, date);
		SafeWrite32(0x4BCC0D, date);
		dlogr(" Done", DL_INIT);
	}

	ViewportX = GetPrivateProfileInt("Misc", "ViewXPos", -1, ini);
	if (ViewportX != -1) {
		dlog("Applying starting x view patch.", DL_INIT);
		SafeWrite32(VARPTR_wmWorldOffsetX, ViewportX);
		HookCall(0x4BCF07, &ViewportHook);
		dlogr(" Done", DL_INIT);
	}
	ViewportY = GetPrivateProfileInt("Misc", "ViewYPos", -1, ini);
	if (ViewportY != -1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(VARPTR_wmWorldOffsetY, ViewportY);
		HookCall(0x4BCF07, &ViewportHook);
		dlogr(" Done", DL_INIT);
	}
}