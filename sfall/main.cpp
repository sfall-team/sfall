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

#include "main.h"

#include <math.h>
#include <stdio.h>

#include "FalloutEngine\Fallout2.h"
#include "Modules\AI.h"
#include "Modules\AmmoMod.h"
#include "Modules\AnimationsAtOnceLimit.h"
#include "Modules\BarBoxes.h"
#include "Modules\Books.h"
#include "Modules\Bugs.h"
#include "Modules\BurstMods.h"
#include "Modules\Console.h"
#include "Modules\CRC.h"
#include "Modules\Credits.h"
#include "Modules\Criticals.h"
#include "Modules\Elevators.h"
#include "Modules\Explosions.h"
#include "Modules\FileSystem.h"
#include "Modules\Graphics.h"
#include "Modules\HeroAppearance.h"
#include "Modules\Inventory.h"
#include "Modules\KillCounter.h"
#include "Modules\knockback.h"
#include "Modules\LoadGameHook.h"
#include "Modules\MainMenu.h"
#include "Modules\Message.h"
#include "Modules\Movies.h"
#include "Modules\PartyControl.h"
#include "Modules\Perks.h"
#include "Modules\Premade.h"
#include "Modules\QuestList.h"
#include "Modules\Reputations.h"
#include "Modules\ScriptExtender.h"
#include "Modules\Skills.h"
#include "Modules\Sound.h"
#include "Modules\Stats.h"
#include "Modules\SuperSave.h"
#include "Modules\Tiles.h"
#include "Modules\Timer.h"

#include "Logging.h"
#include "Version.h"
#if (_MSC_VER < 1600)
#include "Cpp11_emu.h"
#endif

bool IsDebug = false;

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

static const DWORD FastShotFixF1[] = {
	0x478BB8, 0x478BC7, 0x478BD6, 0x478BEA, 0x478BF9, 0x478C08, 0x478C2F,
};

static const DWORD script_dialog_msgs[] = {
	0x4A50C2, 0x4A5169, 0x4A52FA, 0x4A5302, 0x4A6B86, 0x4A6BE0, 0x4A6C37,
};

static const DWORD EncounterTableSize[] = {
	0x4BD1A3, 0x4BD1D9, 0x4BD270, 0x4BD604, 0x4BDA14, 0x4BDA44, 0x4BE707,
	0x4C0815, 0x4C0D4A, 0x4C0FD4,
};

static const DWORD PutAwayWeapon[] = {
	0x411EA2, // action_climb_ladder_
	0x412046, // action_use_an_item_on_object_
	0x41224A, // action_get_an_object_
	0x4606A5, // intface_change_fid_animate_
	0x472996, // invenWieldFunc_
};

static __declspec(naked) void GetDateWrapper() {
	__asm {
		push ecx;
		push esi;
		push ebx;
		call FuncOffs::game_time_date_;
		mov ecx, ds:[_pc_proto + 0x4C];
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
	*((DWORD*)_fallout_game_time)=0;
	*((DWORD*)_pc_proto + 0x4C)+=13;
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
		push eax;
		mov eax, ds:[_obj_dude];
		mov edx, PERK_pathfinder;
		call FuncOffs::perk_level_;
		push eax;
		call PathfinderFix2;
		call FuncOffs::inc_game_time_;
		retn;
	}
}

static double FadeMulti;
static __declspec(naked) void FadeHook() {
	__asm {
		pushf;
		push ebx;
		fild [esp];
		fmul FadeMulti;
		fistp [esp];
		pop ebx;
		popf;
		call FuncOffs::fadeSystemPalette_;
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
		mov eax, ds:[0x50fb08];
		call FuncOffs::elapsed_time_;
		cmp eax, ecx;
		jl tck;
		call FuncOffs::get_time_;
		mov ds:[0x50fb08], eax;
		popad;
		jmp FuncOffs::get_input_;
	}
}
static void __declspec(naked) WorldMapEncPatch1() {
	__asm {
		inc dword ptr ds:[_wmLastRndTime]
		call FuncOffs::wmPartyWalkingStep_;
		retn;
	}
}
static void __declspec(naked) WorldMapEncPatch2() {
	__asm {
		mov dword ptr ds:[_wmLastRndTime], 0;
		retn;
	}
}
static void __declspec(naked) WorldMapEncPatch3() {
	__asm {
		mov eax,ds:[_wmLastRndTime];
		retn;
	}
}
static void __declspec(naked) Combat_p_procFix() {
	__asm {
		push eax;

		mov eax,dword ptr ds:[_combat_state];
		cmp eax,3;
		jnz end_cppf;

		push esi;
		push ebx;
		push edx;

		mov esi, _main_ctd;
		mov eax, [esi];
		mov ebx, [esi+0x20];
		xor edx, edx;
		mov eax, [eax+0x78];
		call FuncOffs::scr_set_objs_;
		mov eax, [esi];

		cmp dword ptr ds:[esi+0x2c], +0x0;
		jng jmp1;

		test byte ptr ds:[esi+0x15], 0x1;
		jz jmp1;
		mov edx, 0x2;
		jmp jmp2;
jmp1:
		mov edx, 0x1;
jmp2:
		mov eax, [eax+0x78];
		call FuncOffs::scr_set_ext_param_;
		mov eax, [esi];
		mov edx, 0xd;
		mov eax, [eax+0x78];
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
static double wm_nexttick;
static double wm_wait;
static bool wm_usingperf;
static __int64 wm_perfadd;
static __int64 wm_perfnext;
static DWORD WorldMapLoopCount;
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
		mov ds:[_wmWorldOffsetX], eax
		mov eax, ViewportY;
		mov ds:[_wmWorldOffsetY], eax;
		retn;
	}
}

/*HANDLE _stdcall FakeFindFirstFile(const char* str, WIN32_FIND_DATAA* data) {
	HANDLE h = FindFirstFileA(str,data);
	if (h == INVALID_HANDLE_VALUE) return h;
	while (strlen(data->cFileName) > 12) {
		int i = FindNextFileA(h, data);
		if(i == 0) {
			FindClose(h);
			return INVALID_HANDLE_VALUE;
		}
	}
	return h;
}
int _stdcall FakeFindNextFile(HANDLE h, WIN32_FIND_DATAA* data) {
	int i = FindNextFileA(h, data);
	while (strlen(data->cFileName) > 12 && i) {
		i = FindNextFileA(h, data);
	}
	return i;
}*/

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

static void __declspec(naked) removeDatabase() {
	__asm {
		cmp  eax, -1
		je   end
		mov  ebx, ds:[_paths]
		mov  ecx, ebx
nextPath:
		mov  edx, [esp+0x104+4+4]                 // path_patches
		mov  eax, [ebx]                           // database.path
		call FuncOffs::stricmp_
		test eax, eax                             // found path?
		jz   skip                                 // Yes
		mov  ecx, ebx
		mov  ebx, [ebx+0xC]                       // database.next
		jmp  nextPath
skip:
		mov  eax, [ebx+0xC]                       // database.next
		mov  [ecx+0xC], eax                       // database.next
		xchg ebx, eax
		cmp  eax, ecx
		jne  end
		mov  ds:[_paths], ebx
end:
		retn
	}
}

static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		call removeDatabase
		mov  ds:[_master_db_handle], eax
		retn
	}
}

static void __declspec(naked) game_init_databases_hack2() {
	__asm {
		cmp  eax, -1
		je   end
		mov  eax, ds:[_master_db_handle]
		mov  eax, [eax]                           // eax = master_patches.path
		call FuncOffs::xremovepath_
		dec  eax                                  // remove path (critter_patches == master_patches)?
		jz   end                                  // Yes
		inc  eax
		call removeDatabase
end:
		mov  ds:[_critter_db_handle], eax
		retn
	}
}

static void __declspec(naked) game_init_databases_hook() {
// eax = _master_db_handle
	__asm {
		mov  ecx, ds:[_critter_db_handle]
		mov  edx, ds:[_paths]
		jecxz skip
		mov  [ecx+0xC], edx                       // critter_patches.next->_paths
		mov  edx, ecx
skip:
		mov  [eax+0xC], edx                       // master_patches.next
		mov  ds:[_paths], eax
		retn
	}
}

static char KarmaGainMsg[128];
static char KarmaLossMsg[128];
static void _stdcall SetKarma(int value) {
	int old;
	__asm {
		xor eax, eax;
		call FuncOffs::game_get_global_var_;
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
	Wrapper::display_print(buf);
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
		jmp FuncOffs::game_set_global_var_;
	}
}
static void __declspec(naked) ReloadHook() {
	__asm {
		push eax;
		push ebx;
		push edx;
		mov eax, dword ptr ds:[_obj_dude];
		call FuncOffs::register_clear_;
		xor eax, eax;
		inc eax;
		call FuncOffs::register_begin_;
		xor edx, edx;
		xor ebx, ebx;
		mov eax, dword ptr ds:[_obj_dude];
		dec ebx;
		call FuncOffs::register_object_animate_;
		call FuncOffs::register_end_;
		pop edx;
		pop ebx;
		pop eax;
		jmp FuncOffs::gsound_play_sfx_file_;
	}
}
static const DWORD CorpseHitFix2_continue_loop1 = 0x48B99B;
static void __declspec(naked) CorpseHitFix2() {
	__asm {
		push eax;
		mov eax, [eax];
		call FuncOffs::critter_is_dead_; // found some object, check if it's a dead critter
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
		call FuncOffs::critter_is_dead_;
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

static const DWORD RetryCombatRet=0x422BA9;
static DWORD RetryCombatLastAP;
static DWORD RetryCombatMinAP;
static void __declspec(naked) RetryCombatHook() {
	__asm {
		mov RetryCombatLastAP, 0;
retry:
		mov eax, esi;
		xor edx, edx;
		call FuncOffs::combat_ai_;
process:
		cmp dword ptr ds:[_combat_turn_running], 0;
		jle next;
		call FuncOffs::process_bk_;
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

static DWORD KarmaFrmCount;
static DWORD* KarmaFrms;
static int* KarmaPoints;
static DWORD _stdcall DrawCardHook2() {
	int rep=**(int**)_game_global_vars;
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
		jmp FuncOffs::DrawCard_;
	}
}

static void __declspec(naked) ScienceCritterCheckHook() {
	__asm {
		cmp esi, ds:[_obj_dude];
		jne end;
		mov eax, 10;
		retn;
end:
		jmp FuncOffs::critter_kill_count_type_;
	}
}

static const DWORD NPCStage6Fix1End = 0x493D16;
static const DWORD NPCStage6Fix2End = 0x49423A;
static void __declspec(naked) NPCStage6Fix1() {
	__asm {
		mov eax,0xcc;				// set record size to 204 bytes
		imul eax,edx;				// multiply by number of NPC records in party.txt
		call FuncOffs::mem_malloc_;			// malloc the necessary memory
		mov edx,dword ptr ds:[_partyMemberMaxCount];	// retrieve number of NPC records in party.txt
		mov ebx,0xcc;				// set record size to 204 bytes
		imul ebx,edx;				// multiply by number of NPC records in party.txt
		jmp NPCStage6Fix1End;			// call memset to set all malloc'ed memory to 0
	}
}

static void __declspec(naked) NPCStage6Fix2() {
	__asm {
		mov eax,0xcc;				// record size is 204 bytes
		imul edx,eax;				// multiply by NPC number as listed in party.txt
		mov eax,dword ptr ds:[_partyMemberAIOptions];	// get starting offset of internal NPC table
		jmp NPCStage6Fix2End;			// eax+edx = offset of specific NPC record
	}
}

static const DWORD FastShotTraitFixEnd1 = 0x478E7F;
static const DWORD FastShotTraitFixEnd2 = 0x478E7B;
static void __declspec(naked) FastShotTraitFix() {
	__asm {
		test eax,eax;				// does player have Fast Shot trait?
		je ajmp;				// skip ahead if no
		mov edx,ecx;				// argument for item_w_range_: hit_mode
		mov eax,ebx;				// argument for item_w_range_: pointer to source_obj (always dude_obj due to code path)
		call FuncOffs::item_w_range_;			// get weapon's range
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

static const DWORD ScannerHookRet=0x41BC1D;
static const DWORD ScannerHookFail=0x41BC65;
static void __declspec(naked) ScannerAutomapHook() {
	__asm {
		mov eax, ds:[_obj_dude];
		mov edx, 59;
		call FuncOffs::inven_pid_is_carried_ptr_;
		test eax, eax;
		jz fail;
		mov edx, eax;
		jmp ScannerHookRet;
fail:
		jmp ScannerHookFail;
	}
}

/*static void _stdcall explosion_crash_fix_hook2() {
	if (InCombat()) return;
	for (int elv = 0; elv < 3; elv++) {
		for (int tile = 0; tile < 40000; tile++) {
			DWORD* obj;
			__asm {
				mov  edx, tile;
				mov  eax, elv;
				call FuncOffs::obj_find_first_at_tile_;
				mov  obj, eax;
			}
			while (obj) {
				DWORD otype = obj[25];
				otype = (otype&0xff000000) >> 24;
				if (otype == 1) {
					obj[0x12] = 0;
					obj[0x15] = 0;
					obj[0x10] = 0;
				}
				__asm {
					call FuncOffs::obj_find_next_at_tile_;
					mov  obj, eax;
				}
			}
		}
	}
}

static void __declspec(naked) explosion_crash_fix_hook() {
	__asm {
		pushad;
		call explosion_crash_fix_hook2;
		popad;
		jmp  report_explosion_;
	}
}*/

static void __declspec(naked) objCanSeeObj_ShootThru_Fix() {//(EAX *objStruct, EDX hexNum1, EBX hexNum2, ECX ?, stack1 **ret_objStruct, stack2 flags)
	__asm {
		push esi
		push edi

		push FuncOffs::obj_shoot_blocking_at_ //arg3 check hex objects func pointer
		mov esi, 0x20//arg2 flags, 0x20 = check shootthru
		push esi
		mov edi, dword ptr ss : [esp + 0x14] //arg1 **ret_objStruct
		push edi
		call FuncOffs::make_straight_path_func_;//(EAX *objStruct, EDX hexNum1, EBX hexNum2, ECX ?, stack1 **ret_objStruct, stack2 flags, stack3 *check_hex_objs_func)

		pop edi
		pop esi
		ret 0x8
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

static void __declspec(naked) register_object_take_out_hack() {
	__asm {
		push ecx
		push eax
		mov  ecx, edx                             // ID1
		mov  edx, [eax+0x1C]                      // cur_rot
		inc  edx
		push edx                                  // ID3
		xor  ebx, ebx                             // ID2
		mov  edx, [eax+0x20]                      // fid
		and  edx, 0xFFF                           // Index
		xor  eax, eax
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
		mov  ecx, ds:[_gdNumOptions]
		add  ecx, '1'
		push ecx
		push 0x4458FA
		retn
	}
}

static void DllMain2() {
	//SafeWrite8(0x4B15E8, 0xc3);
	//SafeWrite8(0x4B30C4, 0xc3); //this is the one I need to override for bigger tiles
	DWORD tmp;
	dlogr("In DllMain2", DL_MAIN);

	dlogr("Running BugsInit.", DL_INIT);
	BugsInit();
	dlogr(" Done", DL_INIT);

	if (GetPrivateProfileIntA("Speed", "Enable", 0, ini)) {
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

	if (IsDebug) {
		tmp = GetPrivateProfileIntA("Debugging", "DebugMode", 0, ".\\ddraw.ini");
		if (tmp) {
			dlog("Applying debugmode patch.", DL_INIT);
			//If the player is using an exe with the debug patch already applied, just skip this block without erroring
			if(*((DWORD*)0x00444A64)!=0x082327e8) {
				SafeWrite32(0x00444A64, 0x082327e8);
				SafeWrite32(0x00444A68, 0x0120e900);
				SafeWrite8(0x00444A6D, 0);
				SafeWrite32(0x00444A6E, 0x90909090);
			}
			SafeWrite8(0x004C6D9B, 0xb8);
			if (tmp==1) SafeWrite32(0x004C6D9C, (DWORD)debugGnw);
			else SafeWrite32(0x004C6D9C, (DWORD)debugLog);
			dlogr(" Done", DL_INIT);
		}
	}

	npcautolevel=GetPrivateProfileIntA("Misc", "NPCAutoLevel", 0, ini)!=0;
	if(npcautolevel) {
		dlog("Applying npc autolevel patch.", DL_INIT);
		SafeWrite16(0x00495D22, 0x9090);
		SafeWrite32(0x00495D24, 0x90909090);
		dlogr(" Done", DL_INIT);
	}

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

	/*if (GetPrivateProfileIntA("Misc", "PrintToFileFix", 0, ini)) {
		dlog("Applying print to file patch.", DL_INIT);
		SafeWrite32(0x6C0364, (DWORD)&FakeFindFirstFile);
		SafeWrite32(0x6C0368, (DWORD)&FakeFindNextFile);
		dlogr(" Done", DL_INIT);
	}*/

	if(GetPrivateProfileIntA("Misc", "AdditionalWeaponAnims", 0, ini)) {
		dlog("Applying additional weapon animations patch.", DL_INIT);
		SafeWrite8(0x419320, 0x12);
		HookCall(0x4194CC, WeaponAnimHook);
		HookCall(0x451648, WeaponAnimHook);
		HookCall(0x451671, WeaponAnimHook);
		dlogr(" Done", DL_INIT);
	}

	if (IsDebug && GetPrivateProfileIntA("Debugging", "DontDeleteProtos", 0, ".\\ddraw.ini")) {
		dlog("Applying permanent protos patch.", DL_INIT);
		SafeWrite8(0x48007E, 0xeb);
		dlogr(" Done", DL_INIT);
	}

	CritInit();

	if (GetPrivateProfileIntA("Misc", "MultiPatches", 0, ini)) {
		dlog("Applying load multiple patches patch.", DL_INIT);
		SafeWrite8(0x444354, 0x90); // Change step from 2 to 1
		SafeWrite8(0x44435C, 0xC4); // Disable check
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "DataLoadOrderPatch", 0, ini)) {
		dlog("Applying data load order patch.", DL_INIT);
		MakeCall(0x444259, &game_init_databases_hack1, false);
		MakeCall(0x4442F1, &game_init_databases_hack2, false);
		HookCall(0x44436D, &game_init_databases_hook);
		SafeWrite8(0x4DFAEC, 0x1D); // error correction
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileInt("Misc", "DisplayKarmaChanges", 0, ini)) {
		dlog("Applying display karma changes patch.", DL_INIT);
		GetPrivateProfileString("sfall", "KarmaGain", "You gained %d karma.", KarmaGainMsg, 128, translationIni);
		GetPrivateProfileString("sfall", "KarmaLoss", "You lost %d karma.", KarmaLossMsg, 128, translationIni);
		HookCall(0x455A6D, SetGlobalVarWrapper);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileInt("Misc", "AlwaysReloadMsgs", 0, ini)) {
		dlog("Applying always reload messages patch.", DL_INIT);
		SafeWrite8(0x4A6B8A, 0xff);
		SafeWrite32(0x4A6B8B, 0x02eb0074);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileInt("Misc", "PlayIdleAnimOnReload", 0, ini)) {
		dlog("Applying idle anim on reload patch.", DL_INIT);
		HookCall(0x460B8C, ReloadHook);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "CorpseLineOfFireFix", 0, ini)) {
		dlog("Applying corpse line of fire patch.", DL_INIT);
		MakeCall(0x48B994, CorpseHitFix2, true);
		MakeCall(0x48BA04, CorpseHitFix2b, true);
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "EnableHeroAppearanceMod", 0, ini)) {
		dlog("Setting up Appearance Char Screen buttons.", DL_INIT);
		EnableHeroAppearanceMod();
		dlogr(" Done", DL_INIT);
	}

	if(GetPrivateProfileIntA("Misc", "SkipOpeningMovies", 0, ini)) {
		dlog("Blocking opening movies.", DL_INIT);
		BlockCall(0x4809CB);
		BlockCall(0x4809D4);
		BlockCall(0x4809E0);
		dlogr(" Done", DL_INIT);
	}

	RetryCombatMinAP=GetPrivateProfileIntA("Misc", "NPCsTryToSpendExtraAP", 0, ini);
	if(RetryCombatMinAP) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, &RetryCombatHook);
		dlogr(" Done", DL_INIT);
	}

	dlog("Checking for changed skilldex images.", DL_INIT);
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

	switch(GetPrivateProfileIntA("Misc", "FastShotFix", 1, ini)) {
	case 1:
		dlog("Applying Fast Shot Trait Fix.", DL_INIT);
		MakeCall(0x478E75, &FastShotTraitFix, true);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying Fast Shot Trait Fix. (Fallout 1 version)", DL_INIT);
		SafeWrite16(0x478C9F, 0x9090);
		for (int i = 0; i < sizeof(FastShotFixF1)/4; i++) {
			HookCall(FastShotFixF1[i], (void*)0x478C7D);
		}
		dlogr(" Done", DL_INIT);
		break;
	}

	if(GetPrivateProfileIntA("Misc", "BoostScriptDialogLimit", 0, ini)) {
		const int scriptDialogCount=10000;
		dlog("Applying script dialog limit patch.", DL_INIT);
		scriptDialog = new int[scriptDialogCount*2]; // Because the msg structure is 8 bytes, not 4.
		SafeWrite32(0x4A50E3, scriptDialogCount); // scr_init
		SafeWrite32(0x4A519F, scriptDialogCount); // scr_game_init
		SafeWrite32(0x4A534F, scriptDialogCount*8); // scr_message_free
		for (int i = 0; i < sizeof(script_dialog_msgs)/4; i++) {
			SafeWrite32(script_dialog_msgs[i], (DWORD)scriptDialog); // scr_get_dialog_msg_file
		}
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

	if (tmp = GetPrivateProfileIntA("Misc", "EncounterTableSize", 0, ini) && tmp <= 127) {
		dlog("Applying EncounterTableSize patch.", DL_INIT);
		DWORD nsize = (tmp + 1) * 180 + 0x50;
		for (int i = 0; i < sizeof(EncounterTableSize)/4; i++) {
			SafeWrite32(EncounterTableSize[i], nsize);
		}
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

	//HookCall(0x413105, explosion_crash_fix_hook);//test for explosives
	//SafeWrite32(0x413034, (DWORD)&explosion_crash_fix_hook);

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
	addrs[0] = 0x499B99; addrs[1] = 0x499DA8;
	SimplePatch<BYTE>(addrs, 2, "Misc", "PipboyTimeAnimDelay", 50, 0, 127);

	if (GetPrivateProfileIntA("Misc", "EnableMusicInDialogue", 0, ini)) {
		dlog("Applying music in dialogue patch.", DL_INIT);
		SafeWrite8(0x44525B, 0x0);
		//BlockCall(0x450627);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "TownMapHotkeysFix", 1, ini)) {
		dlog("Applying town map hotkeys patch.", DL_INIT);
		MakeCall(0x4C4945, &wmTownMapFunc_hack, false);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "InstantWeaponEquip", 0, ini)) {
	//Skip weapon equip/unequip animations
		dlog("Applying instant weapon equip patch.", DL_INIT);
		for (int i = 0; i < sizeof(PutAwayWeapon)/4; i++) {
			SafeWrite8(PutAwayWeapon[i], 0xEB);   // jmps
		}
		BlockCall(0x472AD5);                      //
		BlockCall(0x472AE0);                      // invenUnwieldFunc_
		BlockCall(0x472AF0);                      //
		MakeCall(0x415238, &register_object_take_out_hack, true);
		dlogr(" Done", DL_INIT);
	}

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

	dlogr("Leave DllMain2", DL_MAIN);
}

static void _stdcall OnExit() {
	if (scriptDialog != nullptr) {
		delete[] scriptDialog;
	}
	ClearReadExtraGameMsgFiles();
	ConsoleExit();
	AnimationsAtOnceExit();
	HeroAppearanceModExit();
	//SoundExit();
}

static void __declspec(naked) OnExitFunc() {
	__asm {
		pushad;
		call OnExit;
		popad;
		jmp FuncOffs::DOSCmdLineDestroy_;
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
	if (dwReason == DLL_PROCESS_ATTACH) {
		// enabling debugging features
		IsDebug = (GetPrivateProfileIntA("Debugging", "Enable", 0, ".\\ddraw.ini") != 0);
		if (IsDebug) {
			LoggingInit();
		}

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

		if (!IsDebug || !GetPrivateProfileIntA("Debugging", "SkipCompatModeCheck", 0, ".\\ddraw.ini")) {
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

		// ini file override
		bool cmdlineexists = false;
		char* cmdline = GetCommandLineA();
		if (GetPrivateProfileIntA("Main", "UseCommandLine", 0, ".\\ddraw.ini")) {
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

		if (cmdlineexists && strlen(cmdline)) {
			strcpy_s(ini, ".\\");
			strcat_s(ini, cmdline);
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (h!=INVALID_HANDLE_VALUE) CloseHandle(h);
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
