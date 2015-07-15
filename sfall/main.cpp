/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2011, 2012  The sfall team
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

#include "main.h"

#include <math.h>
#include <stdio.h>

#include "AI.h"
#include "AmmoMod.h"
#include "AnimationsAtOnceLimit.h"
#include "BarBoxes.h"
#include "Books.h"
#include "BurstMods.h"
#include "CRC.h"
#include "Credits.h"
#include "Criticals.h"
#include "Define.h"
#include "Elevators.h"
#include "Explosions.h"
#include "FalloutEngine.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "Inventory.h"
#include "KillCounter.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "MainMenu.h"
#include "PartyControl.h"
#include "Premade.h"
#include "QuestList.h"
#include "Reputations.h"
#include "ScriptExtender.h"
#include "SuperSave.h"
#include "Tiles.h"
#include "console.h"
#include "knockback.h"
#include "movies.h"
#include "perks.h"
#include "skills.h"
#include "sound.h"
#include "stats.h"
#include "timer.h"
#include "version.h"

char ini[65];
char translationIni[65];

static char mapName[65];
static char configName[65];
static char patchName[65];
static char versionString[65];

static char smModelName[65];
char dmModelName[65];
static char sfModelName[65];
char dfModelName[65];

static const char* debugLog="LOG";
static const char* debugGnw="GNW";

static const char* musicOverridePath="data\\sound\\music\\";

bool npcautolevel;

static int* scriptDialog;

//GetTickCount calls
static const DWORD offsetsA[] = {
	0x004c8d34,
	0x004c9375,
	0x004c9384,
	0x004c93c0,
	0x004c93e8,
	0x004c9d2e,
	0x004fe01e,
};

//Delayed GetTickCount calls
static const DWORD offsetsB[] = {
	0x004fdf64,
};

//timeGetTime calls
static const DWORD offsetsC[] = {
	0x004a3179,
	0x004a325d,
	0x004f482b,
	0x004f4e53,
	0x004f5542,
	0x004f56cc,
	0x004f59c6,
	0x004fe036,
};

static const DWORD origFuncPosA=0x006c0200;
static const DWORD origFuncPosB=0x006c03b0;
static const DWORD origFuncPosC=0x006c0164;

static const DWORD getLocalTimePos=0x004fdf58;

static const DWORD dinputPos=0x0050FB70;
static const DWORD DDrawPos=0x0050FB5C;

static DWORD AddrGetTickCount;
static DWORD AddrGetLocalTime;

static DWORD ViewportX;
static DWORD ViewportY;

static __declspec(naked) void GetDateWrapper() {
	__asm {
		push ecx;
		push esi;
		push ebx;
		mov ecx, 0x004A3338;
		call ecx;
		mov ecx, ds:[0x51C3BC];
		pop esi;
		test esi, esi;
		jz end;
		add ecx, [esi];
		mov [esi], ecx;
end:
		pop esi;
		pop ecx;
		retn;
	}
}
static void TimerReset() {
	*((DWORD*)0x51C720)=0;
	*((DWORD*)0x51C3BC)+=13;
}

static double TickFrac=0;
static double MapMulti=1;
static double MapMulti2=1;
void _stdcall SetMapMulti(float d) { MapMulti2=d; }
static __declspec(naked) void PathfinderFix3() {
	__asm {
		xor eax, eax;
		retn;
	}
}
static DWORD _stdcall PathfinderFix2(DWORD perkLevel, DWORD ticks) {
	double d = MapMulti*MapMulti2;
	if(perkLevel==1) d*=0.75;
	else if(perkLevel==2) d*=0.5;
	else if(perkLevel==3) d*=0.25;
	d=((double)ticks)*d + TickFrac;
	TickFrac = modf(d, &d);
	return (DWORD)d;
}
static __declspec(naked) void PathfinderFix() {
	__asm {
		push ebx;
		push eax;
		mov eax, ds:[0x006610B8];
		mov edx, 0x2b;
		mov ebx, 0x00496B78;
		call ebx;
		push eax;
		call PathfinderFix2;
		mov ebx, 0x004A34CC;
		call ebx;
		pop ebx;
		retn;
	}
}

static double FadeMulti;
static __declspec(naked) void FadeHook() {
	__asm {
		push ecx;
		pushf;
		push ebx;
		fild [esp];
		fmul FadeMulti;
		fistp [esp];
		pop ebx;
		popf;
		mov ecx, 0x004C7320;
		call ecx;
		pop ecx;
		retn;
	}
}

static int mapSlotsScrollMax=27 * (17 - 7);

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
		mov ebx, 0x004C219C;
		call ebx;
end:
		pop ebx;
		retn;
	}
}

static const DWORD wp_function_timer=0x4c9370;
static const DWORD wp_function_difference=0x4c93e0;
static const DWORD wp_function_event=0x4c8b78;
static DWORD wp_delay;
static void __declspec(naked) worldmap_patch() {
	__asm {
		pushad;
		call RunGlobalScripts3;
		mov ecx, wp_delay;
tck:
		mov eax,ds:[0x50fb08];
		call wp_function_difference;
		cmp eax,ecx;
		jl tck;
		call wp_function_timer;
		mov ds:[0x50fb08],eax;
		popad;
		jmp wp_function_event;
	}
}
static const DWORD WorldMapMoveFunc=0x004C1F90;
static void __declspec(naked) WorldMapEncPatch1() {
	__asm {
		inc dword ptr ds:[0x51dea0]
		call WorldMapMoveFunc;
		retn;
	}
}
static void __declspec(naked) WorldMapEncPatch2() {
	__asm {
		mov dword ptr ds:[0x51dea0],0;
		retn;
	}
}
static void __declspec(naked) WorldMapEncPatch3() {
	__asm {
		mov eax,ds:[0x51dea0];
		retn;
	}
}
static const DWORD Combat_p_procFixFunc0=0x4AEF48; //&GetCurrentStat(void* critter, int statID)
static const DWORD Combat_p_procFixFunc1=0x4a3b0c;
static const DWORD Combat_p_procFixFunc2=0x4a3b34;
static const DWORD Combat_p_procFixFunc3=0x4a4810;
static void __declspec(naked) Combat_p_procFix() {
	__asm {
		push eax;

		mov eax,dword ptr ds:[0x510944];
		cmp eax,3;
		jnz end_cppf;

		push esi;
		push ebx;
		push edx;

		mov esi,0x56d2b0;
		mov eax,[esi];
		mov ebx,[esi+0x20];
		xor edx,edx;
		mov eax,[eax+0x78];
		call Combat_p_procFixFunc1;
		mov eax,[esi];

		cmp dword ptr ds:[esi+0x2c],+0x0;
		jng jmp1;

		test byte ptr ds:[esi+0x15],0x1;
		jz jmp1;
		mov edx,0x2;
		jmp jmp2;
jmp1:
		mov edx,0x1;
jmp2:
		mov eax,[eax+0x78];
		call Combat_p_procFixFunc2;
		mov eax,[esi];
		mov edx,0xd;
		mov eax,[eax+0x78];
		call Combat_p_procFixFunc3;
		pop edx;
		pop ebx;
		pop esi;

end_cppf:
		pop eax;
		call Combat_p_procFixFunc0;

		retn;
	}
}
static double wm_nexttick;
static double wm_wait;
static bool wm_usingperf;
static __int64 wm_perfadd;
static __int64 wm_perfnext;
static DWORD WorldMapLoopCount;
static const DWORD WorldMapRealFunc=0x004C8B78;
static void WorldMapSpeedPatch3() {
	RunGlobalScripts3();
	if(wm_usingperf) {
		__int64 timer;
		while(true) {
			QueryPerformanceCounter((LARGE_INTEGER*)&timer);
			if(timer>wm_perfnext) break;
			Sleep(0);
		}
		if(wm_perfnext+wm_perfadd < timer) wm_perfnext = timer+wm_perfadd;
		else wm_perfnext+=wm_perfadd;
	} else {
		DWORD tick;
		DWORD nexttick=(DWORD)wm_nexttick;
		while((tick=GetTickCount())<nexttick) Sleep(0);
		if(nexttick+wm_wait < tick) wm_nexttick = tick + wm_wait;
		else wm_nexttick+=wm_wait;
	}
}
static void __declspec(naked) WorldMapSpeedPatch2() {
	__asm {
		pushad;
		call WorldMapSpeedPatch3;
		popad;
		call WorldMapRealFunc;
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
		call WorldMapRealFunc;
		retn;
	}
}
//Only used if the world map speed patch is disabled, so that world map scripts are still run
static void WorldMapHook() {
	__asm {
		pushad;
		call RunGlobalScripts3;
		popad;
		call WorldMapRealFunc;
		retn;
	}
}

static void __declspec(naked) ViewportHook() {
	__asm {
		mov eax, 0x004BD6B4;
		call eax;
		mov eax, ViewportX;
		mov ds:[0x51DE2C], eax
		mov eax, ViewportY;
		mov ds:[0x51DE30], eax;
		retn;
	}
}
static void __declspec(naked) BlackSkilldexFix() {
	__asm {
		push ecx;
		mov ecx, 0x004998C0;
		call ecx;
		xor ecx, ecx;
		mov dword ptr ds:[0x006644F8], ecx;
		pop ecx;
		retn;
	}
}
static const DWORD AlcoholFixJmp=0x43186C;
static void __declspec(naked) AlcoholFix() {
	__asm {
		test eax, eax;
		jz end;
		jmp AlcoholFixJmp;
end:
		retn;
	}
}
HANDLE _stdcall FakeFindFirstFile(const char* str, WIN32_FIND_DATAA* data) {
	HANDLE h=FindFirstFileA(str,data);
	if(h==INVALID_HANDLE_VALUE) return h;
	while(strlen(data->cFileName)>12) {
		int i=FindNextFileA(h, data);
		if(i==0) {
			FindClose(h);
			return INVALID_HANDLE_VALUE;
		}
	}
	return h;
}
int _stdcall FakeFindNextFile(HANDLE h, WIN32_FIND_DATAA* data) {
	int i=FindNextFileA(h, data);
	while(strlen(data->cFileName)>12&&i) {
		i=FindNextFileA(h, data);
	}
	return i;
}
static const DWORD WeaponAnimAddr=0x419314;
static void __declspec(naked) WeaponAnimHook() {
	__asm {
		cmp edx, 11;
		je c11;
		cmp edx, 15;
		je c15;
		jmp WeaponAnimAddr;
c11:
		mov edx, 16;
		jmp WeaponAnimAddr;
c15:
		mov edx, 17;
		jmp WeaponAnimAddr;
	}
}
static const DWORD SetGlobalVarAddr=0x443C98;
static const DWORD GetGlobalVarAddr=0x443C68;
static char KarmaGainMsg[128];
static char KarmaLossMsg[128];
static void _stdcall SetKarma(int value) {
	int old;
	__asm {
		xor eax, eax;
		call GetGlobalVarAddr;
		mov old, eax;
	}
	old=value-old;
	char buf[64];
	if(old==0) return;
	if(old>0) {
		sprintf_s(buf, KarmaGainMsg, old);
	} else {
		sprintf_s(buf, KarmaLossMsg, -old);
	}
	DisplayConsoleMessage(buf);
}
static void __declspec(naked) SetGlobalVarWrapper() {
	__asm {
		test eax, eax;
		jnz end;
		pushad;
		push edx;
		call SetKarma;
		popad;
end:
		jmp SetGlobalVarAddr;
	}
}
static const DWORD PlaySfxInternal=0x4519A8;
static const DWORD AnimClear=0x413C4C;
static const DWORD AnimBegin=0x413AF4;
static const DWORD AnimEnd=0x413CCC;
static const DWORD RegAnim=0x4149D0;
static void __declspec(naked) ReloadHook() {
	__asm {
		push eax;
		push ebx;
		push edx;
		mov eax, dword ptr ds:[0x6610B8];
		call AnimClear;
		xor eax, eax;
		inc eax;
		call AnimBegin;
		xor edx, edx;
		xor ebx, ebx;
		mov eax, dword ptr ds:[0x6610B8];
		dec ebx;
		call RegAnim;
		call AnimEnd;
		pop edx;
		pop ebx;
		pop eax;
		jmp PlaySfxInternal;
	}
}
static const DWORD CorpseHitFix2_continue_loop1 = 0x48B99B;
static void __declspec(naked) CorpseHitFix2() {
	__asm {
		push eax;
		mov eax, [eax];
		call critter_is_dead_; // found some object, check if it's a dead critter
		test eax, eax;
		pop eax;
		jz really_end; // if not, allow breaking the loop (will return this object)
		jmp CorpseHitFix2_continue_loop1; // otherwise continue searching

really_end:
		mov     eax, [eax];
		pop     ebp;
		pop     edi;
		pop     esi;
		pop     ecx;
		retn;
	}
}
static const DWORD CorpseHitFix2_continue_loop2 = 0x48BA0B; 
// same logic as above, for different loop
static void __declspec(naked) CorpseHitFix2b() {
	__asm {
		mov eax, [edx];
		call critter_is_dead_;
		test eax, eax;
		jz really_end; 
		jmp CorpseHitFix2_continue_loop2;

really_end:
		mov     eax, [edx];
		pop     ebp;
		pop     edi;
		pop     esi;
		pop     ecx;
		retn;
	}
}

static const DWORD _combat_ai=0x42B130;
static const DWORD _process_bk=0x4C8BDC;
static const DWORD RetryCombatRet=0x422BA9;
static DWORD RetryCombatLastAP;
static DWORD RetryCombatMinAP;
static void __declspec(naked) RetryCombatHook() {
	__asm {
		mov RetryCombatLastAP, 0;
retry:
		mov eax, esi;
		xor edx, edx;
		call _combat_ai;
process:
		cmp dword ptr ds:[0x51093C], 0;
		jle next;
		call _process_bk;
		jmp process;
next:
		mov eax, [esi+0x40];
		cmp eax, RetryCombatMinAP;
		jl end;
		cmp eax, RetryCombatLastAP;
		je end;
		mov RetryCombatLastAP, eax;
		jmp retry;
end:
		retn;
	}
}
static const DWORD IntfaceRotateNumbersRet=0x460BA0;
static void __declspec(naked) IntfaceRotateNumbersHook() {
	__asm {
		cmp ebx, ecx;
		je end;
		jg greater;
		mov ebx, ecx;
		inc ebx;
		jmp end;
greater:
		cmp ebx, 0;
		jg skip;
		xor ebx, ebx;
		inc ebx;
skip:
		dec ebx;
end:
		jmp IntfaceRotateNumbersRet;
	}
}

static DWORD KarmaFrmCount;
static DWORD* KarmaFrms;
static int* KarmaPoints;
static const DWORD DrawCardAddr=0x43AAEC;
static DWORD _stdcall DrawCardHook2() {
	int rep=**(int**)0x5186C0;
	for(DWORD i=0;i<KarmaFrmCount-1;i++) {
		if(rep < KarmaPoints[i]) return KarmaFrms[i];
	}
	return KarmaFrms[KarmaFrmCount-1];
}
static void __declspec(naked) DrawCardHook() {
	__asm {
		cmp ds:[0x5707D0], 10;
		jne skip;
		cmp eax, 0x30;
		jne skip;
		push ecx;
		push edx;
		call DrawCardHook2;
		pop edx;
		pop ecx;
skip:
		jmp DrawCardAddr;
	}
}

static const DWORD critter_kill_count_type=0x42D920;
static void __declspec(naked) ScienceCritterCheckHook() {
	__asm {
		cmp esi, ds:[0x6610B8];
		jne end;
		mov eax, 10;
		retn;
end:
		jmp critter_kill_count_type;
	}
}

static const DWORD inven_worn = 0x471C08;
static const DWORD WieldObjCritterFixEnd = 0x456926;
static void __declspec(naked) WieldObjCritterFix() {
	__asm {
		xor ebp,ebp;				// Set ebp=0 to skip adjust_ac() processing (assume not armor)
		test eax,eax;				// Did item_get_type() return item_type_armor?
		jnz lexit;				// Skip ahead if no
		mov eax,dword ptr ds:[0x6610b8];
		call inven_worn;			// Otherwise, get stats of armor worn
		mov dword ptr ss:[esp+0x14],eax;	// Store pointer to armor worn (for adjust_ac())
		inc ebp;				// Toggle flag to process adjust_ac() later in function
lexit:
		jmp WieldObjCritterFixEnd;
	}
}

static const DWORD _adjust_ac = 0x4715F8;
static const DWORD _intface_update_ac = 0x45EDA8;
static void __declspec(naked) WieldObjCritterFix2() {
	__asm {
		call _adjust_ac;
		xor ecx, ecx;
		call _intface_update_ac;
		retn;
	}
}

static const DWORD JetAntidoteFixEnd=0x47A01A;
static void __declspec(naked) JetAntidoteFix() {
	__asm {
		inc eax;			// set return value to 1 instead of 0
		add esp,0x14;			// in order to trigger call to function
		pop ebp;			// item_remove_mult in callee procedure
		pop edi;
		jmp JetAntidoteFixEnd;
	}
}

static const DWORD mem_malloc = 0x4C5AD0;
static const DWORD NPCStage6Fix1End = 0x493D16;
static const DWORD NPCStage6Fix2End = 0x49423A;
static void __declspec(naked) NPCStage6Fix1() {
	__asm {
		mov eax,0xcc;				// set record size to 204 bytes
		imul eax,edx;				// multiply by number of NPC records in party.txt
		call mem_malloc;			// malloc the necessary memory
		mov edx,dword ptr ds:[0x519d9c];	// retrieve number of NPC records in party.txt
		mov ebx,0xcc;				// set record size to 204 bytes
		imul ebx,edx;				// multiply by number of NPC records in party.txt
		jmp NPCStage6Fix1End;			// call memset to set all malloc'ed memory to 0
	}
}

static void __declspec(naked) NPCStage6Fix2() {
	__asm {
		mov eax,0xcc;				// record size is 204 bytes
		imul edx,eax;				// multiply by NPC number as listed in party.txt
		mov eax,dword ptr ds:[0x519db8];	// get starting offset of internal NPC table
		jmp NPCStage6Fix2End;			// eax+edx = offset of specific NPC record
	}
}

static const DWORD MultiHexFix1End = 0x429024;
static const DWORD MultiHexFix2End = 0x429175;
static void __declspec(naked) MultiHexFix1() {
	__asm {
		xor ecx,ecx;				// argument value for make_path_func: ecx=0 (unknown arg)
		test byte ptr ds:[ebx+0x25],0x08;	// is target multihex?
		mov ebx,dword ptr ds:[ebx+0x4];		// argument value for make_path_func: target's tilenum (end_tile)
		je end;					// skip if not multihex
		inc ebx;				// otherwise, increase tilenum by 1
end:
		jmp MultiHexFix1End;			// call make_path_func
	}
}

static void __declspec(naked) MultiHexFix2() {
	__asm {
		xor ecx,ecx;				// argument for make_path_func: ecx=0 (unknown arg)
		test byte ptr ds:[ebx+0x25],0x08;	// is target multihex?
		mov ebx,dword ptr ds:[ebx+0x4];		// argument for make_path_func: target's tilenum (end_tile)
		je end;					// skip if not multihex
		inc ebx;				// otherwise, increase tilenum by 1
end:
		jmp MultiHexFix2End;			// call make_path_func
	}
}

static const DWORD item_w_range = 0x478A1C;
static const DWORD FastShotTraitFixEnd1 = 0x478E7F;
static const DWORD FastShotTraitFixEnd2 = 0x478E7B;
static void __declspec(naked) FastShotTraitFix() {
	__asm {
		test eax,eax;				// does player have Fast Shot trait?
		je ajmp;				// skip ahead if no
		mov edx,ecx;				// argument for item_w_range: hit_mode
		mov eax,ebx;				// argument for item_w_range: pointer to source_obj (always dude_obj due to code path)
		call item_w_range;			// get weapon's range
		cmp eax,0x2;				// is weapon range less than or equal 2 (i.e. melee/unarmed attack)?
		jle ajmp;				// skip ahead if yes
		xor eax,eax;				// otherwise, disallow called shot attempt
		jmp bjmp;
ajmp:
		jmp FastShotTraitFixEnd1;		// continue processing called shot attempt
bjmp:
		jmp FastShotTraitFixEnd2;		// clean up and exit function item_w_called_shot
	}
}


static void __declspec(naked) DodgyDoorsFix() {//checks if an attacked object is a critter before attempting dodge animation
	__asm {
		mov eax, dword ptr ss:[EBP+0x20] //(original code) objStruct ptr
		mov ebx, dword ptr ss:[EAX+0x20] //objStruct->FID
		and ebx, 0x0F000000
		sar ebx, 0x18
		cmp ebx, 1						//check if object FID type flag is set to critter
		jne EndFunc						//if object not a critter leave jump condition flags set to skip dodge animation
		test byte ptr ds:[eax+0x44], 0x03//(original code) check some flag?
EndFunc:
		ret
	}
}

static const DWORD ScannerHookRet=0x41BC1D;
static const DWORD ScannerHookFail=0x41BC65;
static const DWORD _inven_pid_is_carried_ptr=0x471CA0;
static void __declspec(naked) ScannerAutomapHook() {
	__asm {
		mov eax, ds:[0x6610B8];
		mov edx, 59;
		call _inven_pid_is_carried_ptr;
		test eax, eax;
		jz fail;
		mov edx, eax;
		jmp ScannerHookRet;
fail:
		jmp ScannerHookFail;
	}
}

static const DWORD obj_find_first_at_tile=0x48B5A8;
static const DWORD obj_find_next_at_tile=0x48B608;
static void _stdcall explosion_crash_fix_hook2() {
	if(InCombat()) return;
	for(int elv=0;elv<3;elv++) {
		for(int tile=0;tile<40000;tile++) {
			DWORD* obj;
			__asm {
				mov edx, tile;
				mov eax, elv;
				call obj_find_first_at_tile;
				mov obj, eax;
			}
			while(obj) {
				DWORD otype = obj[25];
				otype = (otype&0xff000000) >> 24;
				if(otype==1) {
					obj[0x12]=0;
					obj[0x15]=0;
					obj[0x10]=0;
				}
				__asm {
					call obj_find_next_at_tile;
					mov obj, eax;
				}
			}
		}
	}
}
static const DWORD realfunc=0x413144;
static void __declspec(naked) explosion_crash_fix_hook() {
	__asm {
		pushad;
		call explosion_crash_fix_hook2;
		popad;
		jmp realfunc;
	}
}

static const DWORD lineOfSightSearch_func = 0x4163C8;
///static const DWORD checkHexObjsIsBlocker_func=0x48B848; //(EAX *obj, EDX hexNum, EBX level)
static const DWORD checkHexObjsIsBlockerShootThru_func = 0x48B930; //(EAX *obj, EDX hexNum, EBX level)
static void __declspec(naked) objCanSeeObj_ShootThru_Fix() {//(EAX *objStruct, EDX hexNum1, EBX hexNum2, ECX ?, stack1 **ret_objStruct, stack2 flags)
	__asm {
			push esi
			push edi

			push checkHexObjsIsBlockerShootThru_func //arg3 check hex objects func pointer
			mov esi, 0x20//arg2 flags, 0x20 = check shootthru
			push esi
			mov edi, dword ptr ss : [esp + 0x14] //arg1 **ret_objStruct
			push edi
			call lineOfSightSearch_func;//(EAX *objStruct, EDX hexNum1, EBX hexNum2, ECX ?, stack1 **ret_objStruct, stack2 flags, stack3 *check_hex_objs_func)

			pop edi
			pop esi
			ret 0x8
	}
}


static void __declspec(naked) SharpshooterFix() {
	__asm {
		call    stat_level_ // Perception
		cmp     edi, ds:[0x6610B8] // _obj_dude
		jne     end
		xchg    ecx, eax
		mov     eax, edi // _obj_dude
		mov     edx, 14 // PERK_sharpshooter
		call    perk_level_ // Sharpshooter
		shl     eax, 1
		add     eax, ecx
end:
		retn
	}
}


static void DllMain2() {
	//SafeWrite8(0x4B15E8, 0xc3);
	//SafeWrite8(0x4B30C4, 0xc3); //this is the one I need to override for bigger tiles
	DWORD tmp;
	dlogr("In DllMain2", DL_MAIN);

	//BlockCall(0x4123BC);

	if(GetPrivateProfileIntA("Speed", "Enable", 0, ini)) {
		dlog("Applying speed patch.", DL_INIT);
		AddrGetTickCount = (DWORD)&FakeGetTickCount;
		AddrGetLocalTime = (DWORD)&FakeGetLocalTime;

		for(int i=0;i<sizeof(offsetsA)/4;i++) {
			SafeWrite32(offsetsA[i], (DWORD)&AddrGetTickCount);
		}
		dlog(".", DL_INIT);
		for(int i=0;i<sizeof(offsetsB)/4;i++) {
			SafeWrite32(offsetsB[i], (DWORD)&AddrGetTickCount);
		}
		dlog(".", DL_INIT);
		for(int i=0;i<sizeof(offsetsC)/4;i++) {
			SafeWrite32(offsetsC[i], (DWORD)&AddrGetTickCount);
		}
		dlog(".", DL_INIT);

		SafeWrite32(getLocalTimePos, (DWORD)&AddrGetLocalTime);
		TimerInit();
		dlogr(" Done", DL_INIT);
	}

	//if(GetPrivateProfileIntA("Input", "Enable", 0, ini)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(dinputPos, "ddraw.dll");
		AvailableGlobalScriptTypes|=1;
		dlogr(" Done", DL_INIT);
	//}

	GraphicsMode=GetPrivateProfileIntA("Graphics", "Mode", 0, ini);
	if(GraphicsMode!=4&&GraphicsMode!=5) GraphicsMode=0;
	if(GraphicsMode==4||GraphicsMode==5) {
		dlog("Applying dx9 graphics patch.", DL_INIT);
#ifdef WIN2K
		HMODULE h=LoadLibraryEx("d3dx9_42.dll", 0, LOAD_LIBRARY_AS_DATAFILE);
		if(!h) {
			MessageBoxA(0, "You have selected graphics mode 4 or 5, but d3dx9_42.dll is missing\nSwitch back to mode 0, or install an up to date version of DirectX", "Error", 0);
#else
		HMODULE h=LoadLibraryEx("d3dx9_43.dll", 0, LOAD_LIBRARY_AS_DATAFILE);
		if(!h) {
			MessageBoxA(0, "You have selected graphics mode 4 or 5, but d3dx9_43.dll is missing\nSwitch back to mode 0, or install an up to date version of DirectX", "Error", 0);
#endif
			ExitProcess(-1);
		} else {
			FreeLibrary(h);
		}
		SafeWrite8(0x0050FB6B, '2');
		dlogr(" Done", DL_INIT);
	}
	tmp=GetPrivateProfileIntA("Graphics", "FadeMultiplier", 100, ini);
	if(tmp!=100) {
		dlog("Applying fade patch.", DL_INIT);
		SafeWrite32(0x00493B17, ((DWORD)&FadeHook) - 0x00493B1b);
		FadeMulti=((double)tmp)/100.0;
		dlogr(" Done", DL_INIT);
	}

	AmmoModInit();
	MoviesInit();

	mapName[64]=0;
	if(GetPrivateProfileString("Misc", "StartingMap", "", mapName, 64, ini)) {
		dlog("Applying starting map patch.", DL_INIT);
		SafeWrite32(0x00480AAA, (DWORD)&mapName);
		dlogr(" Done", DL_INIT);
	}

	versionString[64]=0;
	if(GetPrivateProfileString("Misc", "VersionString", "", versionString, 64, ini)) {
		dlog("Applying version string patch.", DL_INIT);
		SafeWrite32(0x004B4588, (DWORD)&versionString);
		dlogr(" Done", DL_INIT);
	}

	configName[64]=0;
	if(GetPrivateProfileString("Misc", "ConfigFile", "", configName, 64, ini)) {
		dlog("Applying config file patch.", DL_INIT);
		SafeWrite32(0x00444BA5, (DWORD)&configName);
		SafeWrite32(0x00444BCA, (DWORD)&configName);
		dlogr(" Done", DL_INIT);
	}

	patchName[64]=0;
	if(GetPrivateProfileString("Misc", "PatchFile", "", patchName, 64, ini)) {
		dlog("Applying patch file patch.", DL_INIT);
		SafeWrite32(0x00444323, (DWORD)&patchName);
		dlogr(" Done", DL_INIT);
	}

	smModelName[64]=0;
	if(GetPrivateProfileString("Misc", "MaleStartModel", "", smModelName, 64, ini)) {
		dlog("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x00418B88, (DWORD)&smModelName);
		dlogr(" Done", DL_INIT);
	}

	sfModelName[64]=0;
	if(GetPrivateProfileString("Misc", "FemaleStartModel", "", sfModelName, 64, ini)) {
		dlog("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x00418BAB, (DWORD)&sfModelName);
		dlogr(" Done", DL_INIT);
	}

	dmModelName[64]=0;
	GetPrivateProfileString("Misc", "MaleDefaultModel", "hmjmps", dmModelName, 64, ini);
	dlog("Applying male model patch.", DL_INIT);
	SafeWrite32(0x00418B50, (DWORD)&dmModelName);
	dlogr(" Done", DL_INIT);

	dfModelName[64]=0;
	GetPrivateProfileString("Misc", "FemaleDefaultModel", "hfjmps", dfModelName, 64, ini);
	dlog("Applying female model patch.", DL_INIT);
	SafeWrite32(0x00418B6D, (DWORD)&dfModelName);
	dlogr(" Done", DL_INIT);

	int date=GetPrivateProfileInt("Misc", "StartYear", -1, ini);
	if(date!=-1) {
		dlog("Applying starting year patch.", DL_INIT);
		SafeWrite32(0x4A336C, date);
		dlogr(" Done", DL_INIT);
	}
	date=GetPrivateProfileInt("Misc", "StartMonth", -1, ini);
	if(date!=-1) {
		dlog("Applying starting month patch.", DL_INIT);
		SafeWrite32(0x4A3382, date);
		dlogr(" Done", DL_INIT);
	}
	date=GetPrivateProfileInt("Misc", "StartDay", -1, ini);
	if(date!=-1) {
		dlog("Applying starting day patch.", DL_INIT);
		SafeWrite8(0x4A3356, date);
		dlogr(" Done", DL_INIT);
	}

	date=GetPrivateProfileInt("Misc", "LocalMapXLimit", 0, ini);
	if(date) {
		dlog("Applying local map x limit patch.", DL_INIT);
		SafeWrite32(0x004B13B9, date);
		dlogr(" Done", DL_INIT);
	}
	date=GetPrivateProfileInt("Misc", "LocalMapYLimit", 0, ini);
	if(date) {
		dlog("Applying local map y limit patch.", DL_INIT);
		SafeWrite32(0x004B13C7, date);
		dlogr(" Done", DL_INIT);
	}

	date=GetPrivateProfileInt("Misc", "StartXPos", -1, ini);
	if(date!=-1) {
		dlog("Applying starting x position patch.", DL_INIT);
		SafeWrite32(0x4BC990, date);
		SafeWrite32(0x4BCC08, date);
		dlogr(" Done", DL_INIT);
	}
	date=GetPrivateProfileInt("Misc", "StartYPos", -1, ini);
	if(date!=-1) {
		dlog("Applying starting y position patch.", DL_INIT);
		SafeWrite32(0x4BC995, date);
		SafeWrite32(0x4BCC0D, date);
		dlogr(" Done", DL_INIT);
	}
	ViewportX=GetPrivateProfileInt("Misc", "ViewXPos", -1, ini);
	if(ViewportX!=-1) {
		dlog("Applying starting x view patch.", DL_INIT);
		SafeWrite32(0x51DE2C, ViewportX);
		SafeWrite32(0x004BCF08, (DWORD)&ViewportHook - 0x4BCF0C);
		dlogr(" Done", DL_INIT);
	}
	ViewportY=GetPrivateProfileInt("Misc", "ViewYPos", -1, ini);
	if(ViewportY!=-1) {
		dlog("Applying starting y view patch.", DL_INIT);
		SafeWrite32(0x51DE30, ViewportY);
		SafeWrite32(0x004BCF08, (DWORD)&ViewportHook - 0x4BCF0C);
		dlogr(" Done", DL_INIT);
	}

	//if(GetPrivateProfileIntA("Misc", "SharpshooterFix", 0, ini)) {
		dlog("Applying sharpshooter patch.", DL_INIT);
		HookCall(0x4244AB, &SharpshooterFix);
		SafeWrite8(0x424527, 0xEB);
		dlogr(" Done", DL_INIT);
	//}

	//if(GetPrivateProfileIntA("Misc", "PathfinderFix", 0, ini)) {
		dlog("Applying pathfinder patch.", DL_INIT);
		SafeWrite32(0x004C1FF2, ((DWORD)&PathfinderFix3) - 0x004c1ff6);
		SafeWrite32(0x004C1C79, ((DWORD)&PathfinderFix) - 0x004c1c7d);
		MapMulti=(double)GetPrivateProfileIntA("Misc", "WorldMapTimeMod", 100, ini)/100.0;
		dlogr(" Done", DL_INIT);
	//}

	if(GetPrivateProfileInt("Misc", "WorldMapFPSPatch", 0, ini)) {
		dlog("Applying world map fps patch.", DL_INIT);
		if(*(DWORD*)0x004BFE5E != 0x8d16) {
			dlogr(" Failed", DL_INIT);
		} else {
			wp_delay=GetPrivateProfileInt("Misc", "WorldMapDelay2", 66, ini);
			HookCall(0x004BFE5D, worldmap_patch);
			dlogr(" Done", DL_INIT);
		}
	} else {
		tmp=GetPrivateProfileIntA("Misc", "WorldMapFPS", 0, ini);
		if(tmp) {
			dlog("Applying world map fps patch.", DL_INIT);
			if(*((WORD*)0x004CAFB9)==0x0000) {
				AvailableGlobalScriptTypes|=2;
				SafeWrite32(0x004BFE5E, ((DWORD)&WorldMapSpeedPatch2) - 0x004BFE62);
				if(GetPrivateProfileIntA("Misc", "ForceLowResolutionTimer", 0, ini)||!QueryPerformanceFrequency((LARGE_INTEGER*)&wm_perfadd)||wm_perfadd<=1000) {
					wm_wait=1000.0/(double)tmp;
					wm_nexttick=GetTickCount();
					wm_usingperf=false;
				} else {
					wm_usingperf=true;
					wm_perfadd/=tmp;
					wm_perfnext=0;
				}
			}
			dlogr(" Done", DL_INIT);
		} else {
			tmp=GetPrivateProfileIntA("Misc", "WorldMapDelay", 0, ini);
			if(tmp) {
				if(*((WORD*)0x004CAFB9)==0x3d40)
					SafeWrite32(0x004CAFBB, tmp);
				else if(*((WORD*)0x004CAFB9)==0x0000) {
					SafeWrite32(0x004BFE5E, ((DWORD)&WorldMapSpeedPatch) - 0x004BFE62);
					WorldMapLoopCount=tmp;
				}
			} else {
				if(*(DWORD*)0x004BFE5E==0x0000d816) {
					SafeWrite32(0x004BFE5E, ((DWORD)&WorldMapHook) - 0x004BFE62);
				}
			}
		}
		if(GetPrivateProfileIntA("Misc", "WorldMapEncounterFix", 0, ini)) {
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

	if(GetPrivateProfileIntA("Misc", "DialogueFix", 1, ini)) {
		dlog("Applying dialogue patch.", DL_INIT);
		SafeWrite8(0x00446848, 0x31);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "ExtraKillTypes", 0, ini)) {
		dlog("Applying extra kill types patch.", DL_INIT);
		KillCounterInit(true);
		dlogr(" Done", DL_INIT);
	} else KillCounterInit(false);

	//if(GetPrivateProfileIntA("Misc", "ScriptExtender", 0, ini)) {
		dlog("Applying script extender patch.", DL_INIT);
		StatsInit();
		dlog(".", DL_INIT);
		ScriptExtenderSetup();
		dlog(".", DL_INIT);
		LoadGameHookInit();
		dlog(".", DL_INIT);
		PerksInit();
		dlog(".", DL_INIT);
		KnockbackInit();
		dlog(".", DL_INIT);
		SkillsInit();
		dlog(".", DL_INIT);

		//Ray's combat_p_proc fix
		SafeWrite32(0x0425253, ((DWORD)&Combat_p_procFix) - 0x0425257);
		SafeWrite8(0x0424dbc, 0xE9);
		SafeWrite32(0x0424dbd, 0x00000034);
		dlogr(" Done", DL_INIT);
	//}

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
		if(*((BYTE*)0x004BF3BB)!=0xeb) {
			SafeWrite8(0x004BF3BB, 0xeb);
		}
		dlogr(" Done", DL_INIT);
	//}

	tmp=GetPrivateProfileIntA("Misc", "WorldMapSlots", 0, ini);
	if(tmp&&tmp<128) {
		dlog("Applying world map slots patch.", DL_INIT);
		if(tmp<7) tmp=7;
		mapSlotsScrollMax = (tmp-7)*27;
		if(tmp<25) SafeWrite32(0x004C21FD, 230 - (tmp-17)*27);
		else {
			SafeWrite8(0x004C21FC, 0xC2);
			SafeWrite32(0x004C21FD, 2 + 27*(tmp-26));
		}
		dlogr(" Done", DL_INIT);
	}

	int limit=GetPrivateProfileIntA("Misc", "TimeLimit", 13, ini);
	if(limit==-2) limit=14;
	if(limit==-3) {
		dlog("Applying time limit patch (-3).", DL_INIT);
		limit=-1;
		AddUnarmedStatToGetYear=1;

		SafeWrite32(0x004392F9, ((DWORD)&GetDateWrapper) - 0x004392Fd);
		SafeWrite32(0x00443809, ((DWORD)&GetDateWrapper) - 0x0044380d);
		SafeWrite32(0x0047E128, ((DWORD)&GetDateWrapper) - 0x0047E12c);
		SafeWrite32(0x004975A3, ((DWORD)&GetDateWrapper) - 0x004975A7);
		SafeWrite32(0x00497713, ((DWORD)&GetDateWrapper) - 0x00497717);
		SafeWrite32(0x004979Ca, ((DWORD)&GetDateWrapper) - 0x004979Ce);
		SafeWrite32(0x004C3CB6, ((DWORD)&GetDateWrapper) - 0x004C3CBa);
		dlogr(" Done", DL_INIT);
	}
	if(limit<=14&&limit>=-1&&limit!=13) {
		dlog("Applying time limit patch.", DL_INIT);
		if(limit==-1) {
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

#ifdef TRACE
	tmp=GetPrivateProfileIntA("Debugging", "DebugMode", 0, ini);
	if(tmp) {
		dlog("Applying debugmode patch.", DL_INIT);
		//If the player is using an exe with the debug patch already applied, just skip this block without erroring
		if(*((DWORD*)0x00444A64)!=0x082327e8) {
			SafeWrite32(0x00444A64, 0x082327e8);
			SafeWrite32(0x00444A68, 0x0120e900);
			SafeWrite8(0x00444A6D, 0);
			SafeWrite32(0x00444A6E, 0x90909090);
		}
		SafeWrite8(0x004C6D9B, 0xb8);
		if(tmp==1) SafeWrite32(0x004C6D9C, (DWORD)debugGnw);
		else SafeWrite32(0x004C6D9C, (DWORD)debugLog);
		dlogr(" Done", DL_INIT);
	}
#endif

	npcautolevel=GetPrivateProfileIntA("Misc", "NPCAutoLevel", 0, ini)!=0;
	if(npcautolevel) {
		dlog("Applying npc autolevel patch.", DL_INIT);
		SafeWrite16(0x00495D22, 0x9090);
		SafeWrite32(0x00495D24, 0x90909090);
		dlogr(" Done", DL_INIT);
	}
	//if(GetPrivateProfileIntA("Misc", "NPCLevelFix", 0, ini)) {
		dlog("Applying NPCLevelFix.", DL_INIT);
		HookCall(0x495BC9,  (void*)0x495E51);
		dlogr(" Done", DL_INIT);
	//}

	if(GetPrivateProfileIntA("Misc", "SingleCore", 1, ini)) {
		dlog("Applying single core patch.", DL_INIT);
		HANDLE process=GetCurrentProcess();
		SetProcessAffinityMask(process, 1);
		CloseHandle(process);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "OverrideArtCacheSize", 0, ini)) {
		dlog("Applying override art cache size patch.", DL_INIT);
		SafeWrite32(0x00418867, 0x90909090);
		SafeWrite32(0x00418872, 256);
		dlogr(" Done", DL_INIT);
	}

	char elevPath[MAX_PATH];
	GetPrivateProfileString("Misc", "ElevatorsFile", "", elevPath, MAX_PATH, ini);
	if(strlen(elevPath)>0) {
		dlog("Applying elevator patch.", DL_INIT);
		ElevatorsInit(elevPath);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "UseFileSystemOverride", 0, ini)) {
		FileSystemInit();
	}

	/*DWORD horrigan=GetPrivateProfileIntA("Misc", "DisableHorrigan", 0, ini);
	if(horrigan) {
		if(*((BYTE*)0x004C06D8)!=0x75) return false;
		SafeWrite8(0x004C06D8, 0xeb);
	}*/

	//if(GetPrivateProfileIntA("Misc", "BlackSkilldexFix", 1, ini)) {
		dlog("Applying black skilldex patch.", DL_INIT);
		HookCall(0x00497D0F, BlackSkilldexFix);
		dlogr(" Done", DL_INIT);
	//}

	//if(GetPrivateProfileIntA("Misc", "PrintToFileFix", 0, ini)) {
		dlog("Applying print to file patch.", DL_INIT);
		SafeWrite32(0x006C0364, (DWORD)&FakeFindFirstFile);
		SafeWrite32(0x006C0368, (DWORD)&FakeFindNextFile);
		dlogr(" Done", DL_INIT);
	//}

	if(GetPrivateProfileIntA("Misc", "AdditionalWeaponAnims", 0, ini)) {
		dlog("Applying additional weapon animations patch.", DL_INIT);
		SafeWrite8(0x419320, 0x12);
		HookCall(0x4194CC, WeaponAnimHook);
		HookCall(0x451648, WeaponAnimHook);
		HookCall(0x451671, WeaponAnimHook);
		dlogr(" Done", DL_INIT);
	}

#ifdef TRACE
	if(GetPrivateProfileIntA("Debugging", "DontDeleteProtos", 0, ini)) {
		dlog("Applying permanent protos patch.", DL_INIT);
		SafeWrite8(0x48007E, 0xeb);
		dlogr(" Done", DL_INIT);
	}
#endif

	//if(GetPrivateProfileIntA("Misc", "FixWithdrawalPerkDescCrash", 0, ini)) {
		dlog("Applying withdrawal perk description crash fix. ", DL_INIT);
		HookCall(0x47A501, AlcoholFix);
		dlogr(" Done", DL_INIT);
	//}

	CritInit();

	int number_patch_loop=GetPrivateProfileInt("Misc", "NumberPatchLoop", -1, ini);
	if(number_patch_loop>-1) {
		dlog("Applying load multiple patches patch. ", DL_INIT);
		// Disable check
		SafeWrite8(0x0444363, 0xE9);
		SafeWrite32(0x0444364, 0xFFFFFFB9);
		// New loop count
		SafeWrite32(0x0444357, number_patch_loop);
		// Change step from 2 to 1
		SafeWrite8(0x0444354, 0x90);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileInt("Misc", "DisplayKarmaChanges", 0, ini)) {
		dlog("Applying display karma changes patch. ", DL_INIT);
		GetPrivateProfileString("sfall", "KarmaGain", "You gained %d karma.", KarmaGainMsg, 128, translationIni);
		GetPrivateProfileString("sfall", "KarmaLoss", "You lost %d karma.", KarmaLossMsg, 128, translationIni);
		HookCall(0x455A6D, SetGlobalVarWrapper);
		dlogr(" Done", DL_INIT);
	}

	//if(GetPrivateProfileInt("Misc", "ShivPatch", 0, ini)) {
		dlog("Applying shiv patch. ", DL_INIT);
		SafeWrite8(0x477B2B, 0xeb);
		dlogr(" Done", DL_INIT);
	//}

	//if(GetPrivateProfileInt("Misc", "ImportedProcedureFix", 0, ini)) {
		dlog("Applying imported procedure patch. ", DL_INIT);
		SafeWrite16(0x46B35B, 0x1c60);
		SafeWrite32(0x46B35D, 0x90909090);
		SafeWrite8(0x46DBF1, 0xeb);
		SafeWrite8(0x46DDC4, 0xeb);
		SafeWrite8(0x4415CC, 0x00);
		dlogr(" Done", DL_INIT);
	//}

	if(GetPrivateProfileInt("Misc", "AlwaysReloadMsgs", 0, ini)) {
		dlog("Applying always reload messages patch. ", DL_INIT);
		SafeWrite8(0x4A6B8A, 0xff);
		SafeWrite32(0x4A6B8B, 0x02eb0074);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileInt("Misc", "PlayIdleAnimOnReload", 0, ini)) {
		dlog("Applying idle anim on reload patch. ", DL_INIT);
		HookCall(0x460B8C, ReloadHook);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "CorpseLineOfFireFix", 0, ini)) {
		dlog("Applying corpse line of fire patch. ", DL_INIT);

		MakeCall(0x48B994, CorpseHitFix2, true);
		MakeCall(0x48BA04, CorpseHitFix2b, true);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "EnableHeroAppearanceMod", 0, ini)) {
		dlog("Setting up Appearance Char Screen buttons. ", DL_INIT);
		EnableHeroAppearanceMod();
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "SkipOpeningMovies", 0, ini)) {
		dlog("Blocking opening movies. ", DL_INIT);
		BlockCall(0x4809CB);
		BlockCall(0x4809D4);
		BlockCall(0x4809E0);
		dlogr(" Done", DL_INIT);
	}

	RetryCombatMinAP=GetPrivateProfileIntA("Misc", "NPCsTryToSpendExtraAP", 0, ini);
	if(RetryCombatMinAP) {
		dlog("Applying retry combat patch. ", DL_INIT);
		HookCall(0x422B94, &RetryCombatHook);
		dlogr(" Done", DL_INIT);
	}

	dlog("Checking for changed skilldex images. ", DL_INIT);
	tmp=GetPrivateProfileIntA("Misc", "Lockpick", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D54, tmp);
	tmp=GetPrivateProfileIntA("Misc", "Steal", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D58, tmp);
	tmp=GetPrivateProfileIntA("Misc", "Traps", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D5C, tmp);
	tmp=GetPrivateProfileIntA("Misc", "FirstAid", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D4C, tmp);
	tmp=GetPrivateProfileIntA("Misc", "Doctor", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D50, tmp);
	tmp=GetPrivateProfileIntA("Misc", "Science", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D60, tmp);
	tmp=GetPrivateProfileIntA("Misc", "Repair", 293, ini);
	if(tmp!=293) SafeWrite32(0x00518D64, tmp);
	dlogr(" Done", DL_INIT);

	if(GetPrivateProfileIntA("Misc", "RemoveWindowRounding", 0, ini)) {
		SafeWrite32(0x4B8090, 0x90909090);
		SafeWrite16(0x4B8094, 0x9090);
	}

	dlogr("Running TilesInit().", DL_INIT);
	TilesInit();

	dlogr("Applying main menu text patch", DL_INIT);
	CreditsInit();

	if(GetPrivateProfileIntA("Misc", "UseScrollingQuestsList", 0, ini)) {
		dlog("Applying quests list patch ", DL_INIT);
		QuestListInit();
		dlogr(" Done", DL_INIT);
	}

	dlog("Applying premade characters patch", DL_INIT);
	PremadeInit();

	dlogr("Running SoundInit().", DL_INIT);
	SoundInit();

	dlogr("Running ReputationsInit().", DL_INIT);
	ReputationsInit();

	dlogr("Running ConsoleInit().", DL_INIT);
	ConsoleInit();

	if(GetPrivateProfileIntA("Misc", "ExtraSaveSlots", 0, ini)) {
		dlog("Running EnableSuperSaving()", DL_INIT);
		EnableSuperSaving();
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "SpeedInterfaceCounterAnims", 0, ini)) {
		dlog("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
		HookCall(0x45ED63, IntfaceRotateNumbersHook);
		HookCall(0x45ED86, IntfaceRotateNumbersHook);
		HookCall(0x45EDFA, IntfaceRotateNumbersHook);
		dlogr(" Done", DL_INIT);
	}

	KarmaFrmCount=GetPrivateProfileIntA("Misc", "KarmaFRMsCount", 0, ini);
	if(KarmaFrmCount) {
		KarmaFrms=new DWORD[KarmaFrmCount];
		KarmaPoints=new int[KarmaFrmCount-1];
		dlog("Applying karma frm patch.", DL_INIT);
		char buf[512];
		GetPrivateProfileStringA("Misc", "KarmaFRMs", "", buf, 512, ini);
		char *ptr=buf, *ptr2;
		for(DWORD i=0;i<KarmaFrmCount-1;i++) {
			ptr2=strchr(ptr, ',');
			*ptr2='\0';
			KarmaFrms[i]=atoi(ptr);
			ptr=ptr2+1;
		}
		KarmaFrms[KarmaFrmCount-1]=atoi(ptr);
		GetPrivateProfileStringA("Misc", "KarmaPoints", "", buf, 512, ini);
		ptr=buf;
		for(DWORD i=0;i<KarmaFrmCount-2;i++) {
			ptr2=strchr(ptr, ',');
			*ptr2='\0';
			KarmaPoints[i]=atoi(ptr);
			ptr=ptr2+1;
		}
		KarmaPoints[KarmaFrmCount-2]=atoi(ptr);
		HookCall(0x4367A9, DrawCardHook);
		dlogr(" Done", DL_INIT);
	}

	switch(GetPrivateProfileIntA("Misc", "ScienceOnCritters", 0, ini)) {
	case 1:
		HookCall(0x41276E, ScienceCritterCheckHook);
		break;
	case 2:
		SafeWrite8(0x41276A, 0xeb);
		break;
	}

	tmp=GetPrivateProfileIntA("Misc", "SpeedInventoryPCRotation", 166, ini);
	if(tmp!=166 && tmp<=1000) {
		dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
		SafeWrite32(0x47066B, tmp);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running BarBoxesInit().", DL_INIT);
	BarBoxesInit();

	dlogr("Patching out ereg call.", DL_INIT);
	BlockCall(0x4425E6);


	tmp=GetPrivateProfileIntA("Misc", "AnimationsAtOnceLimit", 32, ini);
	if((signed char)tmp>32) {
		dlog("Applying AnimationsAtOnceLimit patch.", DL_INIT);
		AnimationsAtOnceInit((signed char)tmp);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "WieldObjCritterFix", 1, ini)) {
		dlog("Applying wield_obj_critter fix.", DL_INIT);
		MakeCall(0x45690F, &WieldObjCritterFix, true);
		HookCall(0x45697F, &WieldObjCritterFix2);
		dlogr(" Done", DL_INIT);
	}

	//if(GetPrivateProfileIntA("Misc", "JetAntidoteFix", 1, ini)) {
		dlog("Applying Jet Antidote fix.", DL_INIT);
		MakeCall(0x47A015, &JetAntidoteFix, true);
		dlogr(" Done", DL_INIT);
	//}

	if(GetPrivateProfileIntA("Misc", "RemoveCriticalTimelimits", 0, ini)) {
		dlog("Removing critical time limits.", DL_INIT);
		SafeWrite8(0x42412B, 0x90);
		BlockCall(0x42412C);
		SafeWrite16(0x4A3052, 0x9090);
		SafeWrite16(0x4A3093, 0x9090);
		dlogr(" Done", DL_INIT);
	}

	if(tmp=GetPrivateProfileIntA("Sound", "OverrideMusicDir", 0, ini)) {
		SafeWrite32(0x4449C2, (DWORD)&musicOverridePath);
		SafeWrite32(0x4449DB, (DWORD)&musicOverridePath);
		if(tmp==2) {
			SafeWrite32(0x518E78, (DWORD)&musicOverridePath);
			SafeWrite32(0x518E7C, (DWORD)&musicOverridePath);
		}
	}

	if(GetPrivateProfileIntA("Misc", "NPCStage6Fix", 0, ini)) {
		dlog("Applying NPC Stage 6 Fix.", DL_INIT);
		MakeCall(0x493CE9, &NPCStage6Fix1, true);
		SafeWrite8(0x494063, 0x06);		// loop should look for a potential 6th stage
		SafeWrite8(0x4940BB, 0xCC);		// move pointer by 204 bytes instead of 200
		MakeCall(0x494224, &NPCStage6Fix2, true);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "MultiHexPathingFix", 1, ini)) {
		dlog("Applying MultiHex Pathing Fix.", DL_INIT);
		MakeCall(0x42901F, &MultiHexFix1, true);
		MakeCall(0x429170, &MultiHexFix2, true);
		dlogr(" Done", DL_INIT);
	}

	switch(GetPrivateProfileIntA("Misc", "FastShotFix", 1, ini)) {
	case 1:
		dlog("Applying Fast Shot Trait Fix.", DL_INIT);
		MakeCall(0x478E75, &FastShotTraitFix, true);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying Fast Shot Trait Fix. (Fallout 1 version)", DL_INIT);
		SafeWrite16(0x478C9F, 0x9090);
#define hook(a) SafeWrite32(a+1, 0x478C7D - (a+5))
		hook(0x478C2f);
		hook(0x478C08);
		hook(0x478BF9);
		hook(0x478BEA);
		hook(0x478BD6);
		hook(0x478BC7);
		hook(0x478BB8);
#undef hook
		dlogr(" Done", DL_INIT);
		break;
	}

	//if(GetPrivateProfileIntA("Misc", "DodgyDoorsFix", 1, ini)) {
		dlog("Applying Dodgy Door Fix.", DL_INIT);
		SafeWrite16(0x4113D3, 0x9090);
		MakeCall(0x4113D5, &DodgyDoorsFix, false);
		dlogr(" Done", DL_INIT);
	//}

	if(GetPrivateProfileIntA("Misc", "BoostScriptDialogLimit", 0, ini)) {
		const int scriptDialogCount=10000;
		dlog("Applying script dialog limit patch.", DL_INIT);
		scriptDialog=new int[scriptDialogCount*2]; //Because the msg structure is 8 bytes, not 4.
		//scr_init
		SafeWrite32(0x4A50C2, (DWORD)scriptDialog);
		SafeWrite32(0x4A50E3, scriptDialogCount);
		//scr_game_init
		SafeWrite32(0x4A5169, (DWORD)scriptDialog);
		SafeWrite32(0x4A519F, scriptDialogCount);
		//scr_message_free
		SafeWrite32(0x4A52FA, (DWORD)scriptDialog);
		SafeWrite32(0x4A5302, (DWORD)scriptDialog);
		SafeWrite32(0x4A534F, scriptDialogCount*8);
		//scr_get_dialog_msg_file
		SafeWrite32(0x4A6B86, (DWORD)scriptDialog);
		SafeWrite32(0x4A6BE0, (DWORD)scriptDialog);
		SafeWrite32(0x4A6C37, (DWORD)scriptDialog);
		dlogr(" Done", DL_INIT);
	}

	dlog("Running InventoryInit.", DL_INIT);
	InventoryInit();
	dlogr(" Done", DL_INIT);

	if(tmp=GetPrivateProfileIntA("Misc", "MotionScannerFlags", 1, ini)) {
		dlog("Applying MotionScannerFlags patch.", DL_INIT);
		if(tmp&1) MakeCall(0x41BBE9, &ScannerAutomapHook, true);
		if(tmp&2) BlockCall(0x41BC3C);
		dlogr(" Done", DL_INIT);
	}

	if(tmp=GetPrivateProfileIntA("Misc", "EncounterTableSize", 0, ini) && tmp<=127) {
		dlog("Applying EncounterTableSize patch.", DL_INIT);
		DWORD nsize=(tmp+1)*180+0x50;
		SafeWrite32(0x4BD1A3, nsize);
		SafeWrite32(0x4BD1D9, nsize);
		SafeWrite32(0x4BD270, nsize);
		SafeWrite32(0x4BD604, nsize);
		SafeWrite32(0x4BDA14, nsize);
		SafeWrite32(0x4BDA44, nsize);
		SafeWrite32(0x4BE707, nsize);
		SafeWrite32(0x4C0815, nsize);
		SafeWrite32(0x4C0D4a, nsize);
		SafeWrite32(0x4C0FD4, nsize);
		SafeWrite8(0x4BDB17, (BYTE)tmp);
		dlogr(" Done", DL_INIT);
	}

	dlog("Initing main menu patches.", DL_INIT);
	MainMenuInit();
	dlogr(" Done", DL_INIT);

	if(GetPrivateProfileIntA("Misc", "DisablePipboyAlarm", 0, ini)) {
		SafeWrite8(0x499518, 0xc3);
	}

	dlog("Initing AI patches.", DL_INIT);
	AIInit();
	dlogr(" Done", DL_INIT);

	dlog("Initing AI control.", DL_INIT);
	PartyControlInit();
	dlogr(" Done", DL_INIT);

	HookCall(0x413105, explosion_crash_fix_hook);//test for explosives
	SafeWrite32(0x413034, (DWORD)&explosion_crash_fix_hook);

	if (GetPrivateProfileIntA("Misc", "ObjCanSeeObj_ShootThru_Fix", 0, ini)) {
		dlog("Applying ObjCanSeeObj ShootThru Fix.", DL_INIT);
		SafeWrite32(0x456BC7, (DWORD)&objCanSeeObj_ShootThru_Fix - 0x456BCB);
		dlogr(" Done", DL_INIT);
	}

	// phobos2077:
	ComputeSprayModInit();
	ExplosionLightingInit();
	tmp = SimplePatch<DWORD>(0x4A2873, "Misc", "Dynamite_DmgMax", 50, 0, 9999);
	SimplePatch<DWORD>(0x4A2878, "Misc", "Dynamite_DmgMin", 30, 0, tmp);
	tmp = SimplePatch<DWORD>(0x4A287F, "Misc", "PlasticExplosive_DmgMax", 80, 0, 9999);
	SimplePatch<DWORD>(0x4A2884, "Misc", "PlasticExplosive_DmgMin", 40, 0, tmp);
	BooksInit();
	DWORD addrs[2] = {0x45F9DE, 0x45FB33};
	SimplePatch<WORD>(addrs, 2, "Misc", "CombatPanelAnimDelay", 1000, 0, 65535);
	addrs[0] = 0x447DF4; addrs[1] = 0x447EB6;
	SimplePatch<BYTE>(addrs, 2, "Misc", "DialogPanelAnimDelay", 33, 0, 255);
	dlogr("Leave DllMain2", DL_MAIN);  
}

static void _stdcall OnExit() {
	ConsoleExit();
	AnimationsAtOnceExit();
	HeroAppearanceModExit();
	//SoundExit();
}

static const DWORD OnExitRet=0x4E3D3C;
static void __declspec(naked) OnExitFunc() {
	__asm {
		pushad;
		call OnExit;
		popad;
		jmp OnExitRet;
	}
}

static void CompatModeCheck(HKEY root, const char* filepath, int extra) {
	HKEY key;
	char buf[MAX_PATH];
	DWORD size=MAX_PATH;
	DWORD type;
	if(!(type=RegOpenKeyEx(root, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, extra|STANDARD_RIGHTS_READ|KEY_QUERY_VALUE, &key))) {
		if(!RegQueryValueEx(key, filepath, 0, &type, (BYTE*)buf, &size)) {
			if(size&&(type==REG_EXPAND_SZ||type==REG_MULTI_SZ||type==REG_SZ)) {
				if(strstr(buf, "256COLOR")||strstr(buf, "640X480")||strstr(buf, "WIN")) {
					RegCloseKey(key);
					/*if(!RegOpenKeyEx(root, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, extra|KEY_READ|KEY_WRITE, &key)) {
						if((type=RegDeleteValueA(key, filepath))==ERROR_SUCCESS) {
							MessageBoxA(0, "Fallout was set to run in compatibility mode.\n"
								"Please restart fallout to ensure it runs correctly.", "Error", 0);
							RegCloseKey(key);
							ExitProcess(-1);
						} else {
							//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, type, 0, buf, 260, 0);
							//MessageBoxA(0, buf, "", 0);
						}
					}*/

					MessageBoxA(0, "Fallout appears to be running in compatibility mode.\n" //, and sfall was not able to disable it.\n"
						"Please check the compatibility tab of fallout2.exe, and ensure that the following settings are unchecked.\n"
						"Run this program in compatibility mode for..., run in 256 colours, and run in 640x480 resolution.\n"
						"If these options are disabled, click the 'change settings for all users' button and see if that enables them.", "Error", 0);
					//RegCloseKey(key);
					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	} else {
		//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, type, 0, buf, 260, 0);
		//MessageBoxA(0, buf, "", 0);
	}
}

bool _stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID  lpreserved) {
	if(dwReason==DLL_PROCESS_ATTACH) {
#ifdef TRACE
		LoggingInit();
#endif

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

#ifdef TRACE
		if(!GetPrivateProfileIntA("Debugging", "SkipCompatModeCheck", 0, ".\\ddraw.ini")) {
#else
		if(1) {
#endif
			int is64bit;
			typedef int (_stdcall *chk64bitproc)(HANDLE, int*);
			HMODULE h=LoadLibrary("Kernel32.dll");
			chk64bitproc proc = (chk64bitproc)GetProcAddress(h, "IsWow64Process");
			if(proc) proc(GetCurrentProcess(), &is64bit);
			else is64bit=0;
			FreeLibrary(h);

			CompatModeCheck(HKEY_CURRENT_USER, filepath, is64bit?KEY_WOW64_64KEY:0);
			CompatModeCheck(HKEY_LOCAL_MACHINE, filepath, is64bit?KEY_WOW64_64KEY:0);
		}


		bool cmdlineexists=false;
		char* cmdline=GetCommandLineA();
		if(GetPrivateProfileIntA("Main", "UseCommandLine", 0, ".\\ddraw.ini")) {
			while(cmdline[0]==' ') cmdline++;
			bool InQuote=false;
			int count=-1;

			while(true) {
				count++;
				if(cmdline[count]==0) break;;
				if(cmdline[count]==' '&&!InQuote) break;
				if(cmdline[count]=='"') {
					InQuote=!InQuote;
					if(!InQuote) break;
				}
			}
			if(cmdline[count]!=0) {
				count++;
				while(cmdline[count]==' ') count++;
				cmdline=&cmdline[count];
				cmdlineexists=true;
			}
		}

		if(cmdlineexists&&strlen(cmdline)) {
			strcpy_s(ini, ".\\");
			strcat_s(ini, cmdline);
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if(h!=INVALID_HANDLE_VALUE) CloseHandle(h);
			else {
				MessageBox(0, "You gave a command line argument to fallout, but it couldn't be matched to a file\n" \
					"Using default ddraw.ini instead", "Warning", MB_TASKMODAL);
				strcpy_s(ini, ".\\ddraw.ini");
			}
		} else strcpy_s(ini, ".\\ddraw.ini");

		GetPrivateProfileStringA("Main", "TranslationsINI", "./Translations.ini", translationIni, 65, ini);

		DllMain2();
	}
	return true;
}
