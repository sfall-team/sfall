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
#include <stdio.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "Graphics.h"
#include "ScriptExtender.h"
#include "Timer.h"

#include "Misc.h"


static const char* debugLog = "LOG";
static const char* debugGnw = "GNW";

static int* scriptDialog = nullptr;

//GetTickCount calls
static const DWORD offsetsA[] = {
	0x4C8D34, 0x4C9375, 0x4C9384, 0x4C93C0, 0x4C93E8, 0x4C9D2E, 0x4FE01E,
};

//Delayed GetTickCount calls
static const DWORD offsetsB[] = {
	0x4FDF64,
};

//timeGetTime calls
static const DWORD offsetsC[] = {
	0x4A3179, 0x4A325D, 0x4F482B, 0x4F4E53, 0x4F5542, 0x4F56CC, 0x4F59C6,
	0x4FE036,
};

static const DWORD PutAwayWeapon[] = {
	0x411EA2, // action_climb_ladder_
	0x412046, // action_use_an_item_on_object_
	0x41224A, // action_get_an_object_
	0x4606A5, // intface_change_fid_animate_
	0x472996, // invenWieldFunc_
};

static const DWORD origFuncPosA = 0x6C0200;
static const DWORD origFuncPosB = 0x6C03B0;
static const DWORD origFuncPosC = 0x6C0164;

static const DWORD getLocalTimePos = 0x4FDF58;

static const DWORD dinputPos = 0x50FB70;
static const DWORD DDrawPos = 0x50FB5C;

static DWORD AddrGetTickCount;
static DWORD AddrGetLocalTime;

static DWORD ViewportX;
static DWORD ViewportY;

static DWORD KarmaFrmCount;
static DWORD* KarmaFrms;
static int* KarmaPoints;

static const DWORD FastShotFixF1[] = {
	0x478BB8, 0x478BC7, 0x478BD6, 0x478BEA, 0x478BF9, 0x478C08, 0x478C2F,
};

static const DWORD script_dialog_msgs[] = {
	0x4A50C2, 0x4A5169, 0x4A52FA, 0x4A5302, 0x4A6B86, 0x4A6BE0, 0x4A6C37,
};


static double FadeMulti;
static __declspec(naked) void FadeHook() {
	__asm {
		pushf;
		push ebx;
		fild[esp];
		fmul FadeMulti;
		fistp[esp];
		pop ebx;
		popf;
		call FuncOffs::fadeSystemPalette_;
		retn;
	}
}

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

static void __declspec(naked) Combat_p_procFix() {
	__asm {
		push eax;

		mov eax, dword ptr ds : [VARPTR_combat_state];
		cmp eax, 3;
		jnz end_cppf;

		push esi;
		push ebx;
		push edx;

		mov esi, VARPTR_main_ctd;
		mov eax, [esi];
		mov ebx, [esi + 0x20];
		xor edx, edx;
		mov eax, [eax + 0x78];
		call FuncOffs::scr_set_objs_;
		mov eax, [esi];

		cmp dword ptr ds : [esi + 0x2c], +0x0;
		jng jmp1;

		test byte ptr ds : [esi + 0x15], 0x1;
		jz jmp1;
		mov edx, 0x2;
		jmp jmp2;
jmp1:
		mov edx, 0x1;
jmp2:
		mov eax, [eax + 0x78];
		call FuncOffs::scr_set_ext_param_;
		mov eax, [esi];
		mov edx, 0xd;
		mov eax, [eax + 0x78];
		call FuncOffs::exec_script_proc_;
		pop edx;
		pop ebx;
		pop esi;

end_cppf:
		pop eax;
		call FuncOffs::stat_level_;

		retn;
	}
}

static void __declspec(naked) WeaponAnimHook() {
	__asm {
		cmp edx, 11;
		je c11;
		cmp edx, 15;
		je c15;
		jmp FuncOffs::art_get_code_;
c11:
		mov edx, 16;
		jmp FuncOffs::art_get_code_;
c15:
		mov edx, 17;
		jmp FuncOffs::art_get_code_;
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

static void __declspec(naked) intface_rotate_numbers_hack() {
	__asm {
		push edi
		push ebp
		sub  esp, 0x54
		// ebx=old value, ecx=new value
		cmp  ebx, ecx
		je   end
		mov  ebx, ecx
		jg   decrease
		dec  ebx
		jmp  end
decrease:
		test ecx, ecx
		jl   negative
		inc  ebx
		jmp  end
negative:
		xor  ebx, ebx
end:
		push 0x460BA6
		retn
	}
}

static void __declspec(naked) register_object_take_out_hack() {
	__asm {
		push ecx
		push eax
		mov  ecx, edx                             // ID1
		mov  edx, [eax + 0x1C]                      // cur_rot
		inc  edx
		push edx                                  // ID3
		xor  ebx, ebx                             // ID2
		mov  edx, [eax + 0x20]                      // fid
		and edx, 0xFFF                           // Index
		xor eax, eax
		inc  eax                                  // Obj_Type
		call FuncOffs::art_id_
		xor  ebx, ebx
		dec  ebx
		xchg edx, eax
		pop  eax
		call FuncOffs::register_object_change_fid_
		pop  ecx
		xor  eax, eax
		retn
	}
}

static void __declspec(naked) gdAddOptionStr_hack() {
	__asm {
		mov  ecx, ds:[VARPTR_gdNumOptions]
		add  ecx, '1'
		push ecx
		push 0x4458FA
		retn
	}
}

static DWORD _stdcall DrawCardHook2() {
	int reputation = VarPtr::game_global_vars[GVAR_PLAYER_REPUTATION];
	for (DWORD i = 0; i < KarmaFrmCount - 1; i++) {
		if (reputation < KarmaPoints[i]) return KarmaFrms[i];
	}
	return KarmaFrms[KarmaFrmCount - 1];
}

static void __declspec(naked) DrawCardHook() {
	__asm {
		cmp ds : [VARPTR_info_line], 10;
		jne skip;
		cmp eax, 0x30;
		jne skip;
		push ecx;
		push edx;
		call DrawCardHook2;
		pop edx;
		pop ecx;
skip:
		jmp FuncOffs::DrawCard_;
	}
}

static void __declspec(naked) ScienceCritterCheckHook() {
	__asm {
		cmp esi, ds:[VARPTR_obj_dude];
		jne end;
		mov eax, 10;
		retn;
end:
		jmp FuncOffs::critter_kill_count_type_;
	}
}

static const DWORD FastShotTraitFixEnd1 = 0x478E7F;
static const DWORD FastShotTraitFixEnd2 = 0x478E7B;
static void __declspec(naked) FastShotTraitFix() {
	__asm {
		test eax, eax;				// does player have Fast Shot trait?
		je ajmp;				// skip ahead if no
		mov edx, ecx;				// argument for item_w_range_: hit_mode
		mov eax, ebx;				// argument for item_w_range_: pointer to source_obj (always dude_obj due to code path)
		call FuncOffs::item_w_range_;			// get weapon's range
		cmp eax, 0x2;				// is weapon range less than or equal 2 (i.e. melee/unarmed attack)?
		jle ajmp;				// skip ahead if yes
		xor eax, eax;				// otherwise, disallow called shot attempt
		jmp bjmp;
ajmp:
		jmp FastShotTraitFixEnd1;		// continue processing called shot attempt
bjmp:
		jmp FastShotTraitFixEnd2;		// clean up and exit function item_w_called_shot
	}
}

void ApplySpeedPatch() {
	if (GetPrivateProfileIntA("Speed", "Enable", 0, ini)) {
		dlog("Applying speed patch.", DL_INIT);
		AddrGetTickCount = (DWORD)&FakeGetTickCount;
		AddrGetLocalTime = (DWORD)&FakeGetLocalTime;

		for (int i = 0; i < sizeof(offsetsA) / 4; i++) {
			SafeWrite32(offsetsA[i], (DWORD)&AddrGetTickCount);
		}
		dlog(".", DL_INIT);
		for (int i = 0; i < sizeof(offsetsB) / 4; i++) {
			SafeWrite32(offsetsB[i], (DWORD)&AddrGetTickCount);
		}
		dlog(".", DL_INIT);
		for (int i = 0; i < sizeof(offsetsC) / 4; i++) {
			SafeWrite32(offsetsC[i], (DWORD)&AddrGetTickCount);
		}
		dlog(".", DL_INIT);

		SafeWrite32(getLocalTimePos, (DWORD)&AddrGetLocalTime);
		TimerInit();
		dlogr(" Done", DL_INIT);
	}
}

void ApplyInputPatch() {
	//if(GetPrivateProfileIntA("Input", "Enable", 0, ini)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(dinputPos, "ddraw.dll");
		AvailableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}
}

void ApplyGraphicsPatch() {
	DWORD fadeMulti;
	DWORD GraphicsMode = GetPrivateProfileIntA("Graphics", "Mode", 0, ini);
	if (GraphicsMode != 4 && GraphicsMode != 5) {
		GraphicsMode = 0;
	}
	if (GraphicsMode == 4 || GraphicsMode == 5) {
		dlog("Applying dx9 graphics patch.", DL_INIT);
#ifdef WIN2K
#define _DLL_NAME "d3dx9_42.dll"
#else
#define _DLL_NAME "d3dx9_43.dll"
#endif
		HMODULE h = LoadLibraryEx(_DLL_NAME, 0, LOAD_LIBRARY_AS_DATAFILE);
		if (!h) {
			MessageBoxA(0, "You have selected graphics mode 4 or 5, but " _DLL_NAME " is missing\nSwitch back to mode 0, or install an up to date version of DirectX", "Error", 0);
			ExitProcess(-1);
		} else {
			FreeLibrary(h);
		}
		SafeWrite8(0x0050FB6B, '2');
		dlogr(" Done", DL_INIT);
#undef _DLL_NAME
	}
	fadeMulti = GetPrivateProfileIntA("Graphics", "FadeMultiplier", 100, ini);
	if (fadeMulti != 100) {
		dlog("Applying fade patch.", DL_INIT);
		SafeWrite32(0x00493B17, ((DWORD)&FadeHook) - 0x00493B1b);
		FadeMulti = ((double)fadeMulti) / 100.0;
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
			SafeWrite32(0x004bfee1, ((DWORD)&WorldMapEncPatch1) - 0x004bfee5);
			SafeWrite32(0x004c0663, ((DWORD)&WorldMapEncPatch3) - 0x004c0667);//replaces 'call Difference(GetTickCount(), TicksSinceLastEncounter)'
			SafeWrite16(0x004c0668, GetPrivateProfileIntA("Misc", "WorldMapEncounterRate", 5, ini)); //replaces cmp eax, 0x5dc with cmp eax, <rate>
			SafeWrite16(0x004c0677, 0xE890); //nop, call relative. Replaces 'mov TicksSinceLastEncounter, ecx'
			SafeWrite32(0x004c0679, ((DWORD)&WorldMapEncPatch2) - 0x004c067D);
			SafeWrite8(0x004c232d, 0xb8);	//'mov eax, 0', replaces 'mov eax, GetTickCount()'
			SafeWrite32(0x004c232e, 0);
			dlogr(" Done", DL_INIT);
		}
	}
}

void ApplyStartingStatePatches() {
	int date = GetPrivateProfileInt("Misc", "StartYear", -1, ini);
	if (date != -1) {
		dlog("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileInt("Misc", "StartMonth", -1, ini);
	if (date != -1) {
		dlog("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, date);
		dlogr(" Done", DL_INIT);
	}
	date = GetPrivateProfileInt("Misc", "StartDay", -1, ini);
	if (date != -1) {
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
		SafeWrite32(0x51DE2C, ViewportX);
		SafeWrite32(0x004BCF08, (DWORD)&ViewportHook - 0x4BCF0C);
		dlogr(" Done", DL_INIT);
	}
	ViewportY = GetPrivateProfileInt("Misc", "ViewYPos", -1, ini);
	if (ViewportY != -1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(0x51DE30, ViewportY);
		SafeWrite32(0x004BCF08, (DWORD)&ViewportHook - 0x4BCF0C);
		dlogr(" Done", DL_INIT);
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

void ApplyDebugModePatch() {
	if (IsDebug) {
		DWORD dbgMode = GetPrivateProfileIntA("Debugging", "DebugMode", 0, ".\\ddraw.ini");
		if (dbgMode) {
			dlog("Applying debugmode patch.", DL_INIT);
			//If the player is using an exe with the debug patch already applied, just skip this block without erroring
			if (*((DWORD*)0x00444A64) != 0x082327e8) {
				SafeWrite32(0x00444A64, 0x082327e8);
				SafeWrite32(0x00444A68, 0x0120e900);
				SafeWrite8(0x00444A6D, 0);
				SafeWrite32(0x00444A6E, 0x90909090);
			}
			SafeWrite8(0x004C6D9B, 0xb8);
			if (dbgMode == 1) {
				SafeWrite32(0x004C6D9C, (DWORD)debugGnw);
			}
			else {
				SafeWrite32(0x004C6D9C, (DWORD)debugLog);
			}
			dlogr(" Done", DL_INIT);
		}
	}
}

void ApplyNPCAutoLevelPatch() {
	npcautolevel = GetPrivateProfileIntA("Misc", "NPCAutoLevel", 0, ini) != 0;
	if (npcautolevel) {
		dlog("Applying npc autolevel patch.", DL_INIT);
		SafeWrite16(0x00495D22, 0x9090);
		SafeWrite32(0x00495D24, 0x90909090);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "SingleCore", 1, ini)) {
		dlog("Applying single core patch.", DL_INIT);
		HANDLE process = GetCurrentProcess();
		SetProcessAffinityMask(process, 1);
		CloseHandle(process);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "OverrideArtCacheSize", 0, ini)) {
		dlog("Applying override art cache size patch.", DL_INIT);
		SafeWrite32(0x00418867, 0x90909090);
		SafeWrite32(0x00418872, 256);
		dlogr(" Done", DL_INIT);
	}
}

void ApplyAdditionalWeaponAnimsPatch() {
	if (GetPrivateProfileIntA("Misc", "AdditionalWeaponAnims", 0, ini)) {
		dlog("Applying additional weapon animations patch.", DL_INIT);
		SafeWrite8(0x419320, 0x12);
		HookCall(0x4194CC, WeaponAnimHook);
		HookCall(0x451648, WeaponAnimHook);
		HookCall(0x451671, WeaponAnimHook);
		dlogr(" Done", DL_INIT);
	}
}

void ApplySkilldexImagesPatch() {
	DWORD tmp;
	dlog("Checking for changed skilldex images.", DL_INIT);
	tmp = GetPrivateProfileIntA("Misc", "Lockpick", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D54, tmp);
	}
	tmp = GetPrivateProfileIntA("Misc", "Steal", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D58, tmp);
	}
	tmp = GetPrivateProfileIntA("Misc", "Traps", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D5C, tmp);
	}
	tmp = GetPrivateProfileIntA("Misc", "FirstAid", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D4C, tmp);
	}
	tmp = GetPrivateProfileIntA("Misc", "Doctor", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D50, tmp);
	}
	tmp = GetPrivateProfileIntA("Misc", "Science", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D60, tmp);
	}
	tmp = GetPrivateProfileIntA("Misc", "Repair", 293, ini);
	if (tmp != 293) {
		SafeWrite32(0x00518D64, tmp);
	}
	dlogr(" Done", DL_INIT);
}

void ApplyKarmaFRMsPatch() {
	KarmaFrmCount = GetPrivateProfileIntA("Misc", "KarmaFRMsCount", 0, ini);
	if (KarmaFrmCount) {
		KarmaFrms = new DWORD[KarmaFrmCount];
		KarmaPoints = new int[KarmaFrmCount - 1];
		dlog("Applying karma frm patch.", DL_INIT);
		char buf[512];
		GetPrivateProfileStringA("Misc", "KarmaFRMs", "", buf, 512, ini);
		char *ptr = buf, *ptr2;
		for (DWORD i = 0; i < KarmaFrmCount - 1; i++) {
			ptr2 = strchr(ptr, ',');
			*ptr2 = '\0';
			KarmaFrms[i] = atoi(ptr);
			ptr = ptr2 + 1;
		}
		KarmaFrms[KarmaFrmCount - 1] = atoi(ptr);
		GetPrivateProfileStringA("Misc", "KarmaPoints", "", buf, 512, ini);
		ptr = buf;
		for (DWORD i = 0; i < KarmaFrmCount - 2; i++) {
			ptr2 = strchr(ptr, ',');
			*ptr2 = '\0';
			KarmaPoints[i] = atoi(ptr);
			ptr = ptr2 + 1;
		}
		KarmaPoints[KarmaFrmCount - 2] = atoi(ptr);
		HookCall(0x4367A9, DrawCardHook);
		dlogr(" Done", DL_INIT);
	}
}

void ApplySpeedInterfaceCounterAnimsPatch() {
	switch (GetPrivateProfileIntA("Misc", "SpeedInterfaceCounterAnims", 0, ini)) {
	case 1:
		dlog("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
		MakeCall(0x460BA1, &intface_rotate_numbers_hack, true);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying SpeedInterfaceCounterAnims patch. (Instant)", DL_INIT);
		SafeWrite32(0x460BB6, 0x90DB3190);
		dlogr(" Done", DL_INIT);
		break;
	}
}

void ApplyScienceOnCrittersPatch() {
	switch (GetPrivateProfileIntA("Misc", "ScienceOnCritters", 0, ini)) {
	case 1:
		HookCall(0x41276E, ScienceCritterCheckHook);
		break;
	case 2:
		SafeWrite8(0x41276A, 0xeb);
		break;
	}
}

void ApplyFashShotTraitFix() {
	switch (GetPrivateProfileIntA("Misc", "FastShotFix", 1, ini)) {
	case 1:
		dlog("Applying Fast Shot Trait Fix.", DL_INIT);
		MakeCall(0x478E75, &FastShotTraitFix, true);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying Fast Shot Trait Fix. (Fallout 1 version)", DL_INIT);
		SafeWrite16(0x478C9F, 0x9090);
		for (int i = 0; i < sizeof(FastShotFixF1) / 4; i++) {
			HookCall(FastShotFixF1[i], (void*)0x478C7D);
		}
		dlogr(" Done", DL_INIT);
		break;
	}
}

void ApplyBoostScriptDialogLimitPatch() {
	if (GetPrivateProfileIntA("Misc", "BoostScriptDialogLimit", 0, ini)) {
		const int scriptDialogCount = 10000;
		dlog("Applying script dialog limit patch.", DL_INIT);
		scriptDialog = new int[scriptDialogCount * 2]; // Because the msg structure is 8 bytes, not 4.
		SafeWrite32(0x4A50E3, scriptDialogCount); // scr_init
		SafeWrite32(0x4A519F, scriptDialogCount); // scr_game_init
		SafeWrite32(0x4A534F, scriptDialogCount * 8); // scr_message_free
		for (int i = 0; i < sizeof(script_dialog_msgs) / 4; i++) {
			SafeWrite32(script_dialog_msgs[i], (DWORD)scriptDialog); // scr_get_dialog_msg_file
		}
		dlogr(" Done", DL_INIT);
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

void ApplyNumbersInDialoguePatch() {
	if (GetPrivateProfileIntA("Misc", "NumbersInDialogue", 0, ini)) {
		dlog("Applying numbers in dialogue patch.", DL_INIT);
		SafeWrite32(0x502C32, 0x2000202E);
		SafeWrite8(0x446F3B, 0x35);
		SafeWrite32(0x5029E2, 0x7325202E);
		SafeWrite32(0x446F03, 0x2424448B);        // mov  eax, [esp+0x24]
		SafeWrite8(0x446F07, 0x50);               // push eax
		SafeWrite32(0x446FE0, 0x2824448B);        // mov  eax, [esp+0x28]
		SafeWrite8(0x446FE4, 0x50);               // push eax
		MakeCall(0x4458F5, &gdAddOptionStr_hack, true);
		dlogr(" Done", DL_INIT);
	}
}

void ApplyInstantWeaponEquipPatch() {
	if (GetPrivateProfileIntA("Misc", "InstantWeaponEquip", 0, ini)) {
		//Skip weapon equip/unequip animations
		dlog("Applying instant weapon equip patch.", DL_INIT);
		for (int i = 0; i < sizeof(PutAwayWeapon) / 4; i++) {
			SafeWrite8(PutAwayWeapon[i], 0xEB);   // jmps
		}
		BlockCall(0x472AD5);                      //
		BlockCall(0x472AE0);                      // invenUnwieldFunc_
		BlockCall(0x472AF0);                      //
		MakeCall(0x415238, &register_object_take_out_hack, true);
		dlogr(" Done", DL_INIT);
	}
}

void ApplyCombatProcFix() {
	//Ray's combat_p_proc fix
		SafeWrite32(0x0425253, ((DWORD)&Combat_p_procFix) - 0x0425257);
		SafeWrite8(0x0424dbc, 0xE9);
		SafeWrite32(0x0424dbd, 0x00000034);
		dlogr(" Done", DL_INIT);
	//}
}

void MiscReset() {
	if (scriptDialog != nullptr) {
		delete[] scriptDialog;
	}
}
