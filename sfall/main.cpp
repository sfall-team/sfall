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
#include "Modules\Misc.h"
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

bool npcautolevel;

static const char* musicOverridePath = "data\\sound\\music\\";

static const DWORD EncounterTableSize[] = {
	0x4BD1A3, 0x4BD1D9, 0x4BD270, 0x4BD604, 0x4BDA14, 0x4BDA44, 0x4BE707,
	0x4C0815, 0x4C0D4A, 0x4C0FD4,
};

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

static void __declspec(naked) removeDatabase() {
	__asm {
		cmp  eax, -1
		je   end
		mov  ebx, ds:[VARPTR_paths]
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
		mov  ds:[VARPTR_paths], ebx
end:
		retn
	}
}

static void __declspec(naked) game_init_databases_hack1() {
	__asm {
		call removeDatabase
		mov  ds:[VARPTR_master_db_handle], eax
		retn
	}
}

static void __declspec(naked) game_init_databases_hack2() {
	__asm {
		cmp  eax, -1
		je   end
		mov  eax, ds:[VARPTR_master_db_handle]
		mov  eax, [eax]                           // eax = master_patches.path
		call FuncOffs::xremovepath_
		dec  eax                                  // remove path (critter_patches == master_patches)?
		jz   end                                  // Yes
		inc  eax
		call removeDatabase
end:
		mov  ds:[VARPTR_critter_db_handle], eax
		retn
	}
}

static void __declspec(naked) game_init_databases_hook() {
// eax = _master_db_handle
	__asm {
		mov  ecx, ds:[VARPTR_critter_db_handle]
		mov  edx, ds:[VARPTR_paths]
		jecxz skip
		mov  [ecx+0xC], edx                       // critter_patches.next->_paths
		mov  edx, ecx
skip:
		mov  [eax+0xC], edx                       // master_patches.next
		mov  ds:[VARPTR_paths], eax
		retn
	}
}

static char KarmaGainMsg[128];
static char KarmaLossMsg[128];
static void _stdcall SetKarma(int value) {
	int old = VarPtr::game_global_vars[GVAR_PLAYER_REPUTATION];
	old = value - old;
	char buf[64];
	if (old == 0) return;
	if (old > 0) {
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
		mov eax, dword ptr ds:[VARPTR_obj_dude];
		call FuncOffs::register_clear_;
		xor eax, eax;
		inc eax;
		call FuncOffs::register_begin_;
		xor edx, edx;
		xor ebx, ebx;
		mov eax, dword ptr ds:[VARPTR_obj_dude];
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

static const DWORD RetryCombatRet = 0x422BA9;
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
		cmp dword ptr ds:[VARPTR_combat_turn_running], 0;
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

static const DWORD NPCStage6Fix1End = 0x493D16;
static const DWORD NPCStage6Fix2End = 0x49423A;
static void __declspec(naked) NPCStage6Fix1() {
	__asm {
		mov eax,0xcc;				// set record size to 204 bytes
		imul eax,edx;				// multiply by number of NPC records in party.txt
		call FuncOffs::mem_malloc_;			// malloc the necessary memory
		mov edx,dword ptr ds:[VARPTR_partyMemberMaxCount];	// retrieve number of NPC records in party.txt
		mov ebx,0xcc;				// set record size to 204 bytes
		imul ebx,edx;				// multiply by number of NPC records in party.txt
		jmp NPCStage6Fix1End;			// call memset to set all malloc'ed memory to 0
	}
}

static void __declspec(naked) NPCStage6Fix2() {
	__asm {
		mov eax,0xcc;				// record size is 204 bytes
		imul edx,eax;				// multiply by NPC number as listed in party.txt
		mov eax,dword ptr ds:[VARPTR_partyMemberAIOptions];	// get starting offset of internal NPC table
		jmp NPCStage6Fix2End;			// eax+edx = offset of specific NPC record
	}
}

static const DWORD ScannerHookRet=0x41BC1D;
static const DWORD ScannerHookFail=0x41BC65;
static void __declspec(naked) ScannerAutomapHook() {
	__asm {
		mov eax, ds:[VARPTR_obj_dude];
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

void ApplyScriptExtenderPatches() {
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
}

static void DllMain2() {
	//SafeWrite8(0x4B15E8, 0xc3);
	//SafeWrite8(0x4B30C4, 0xc3); //this is the one I need to override for bigger tiles
	dlogr("In DllMain2", DL_MAIN);

	dlogr("Running BugsInit.", DL_INIT);
	BugsInit();
	dlogr(" Done", DL_INIT);

	ApplySpeedPatch();
	ApplyInputPatch();

	ApplyGraphicsPatch();

	AmmoModInit();
	MoviesInit();

	mapName[64] = 0;
	if (GetPrivateProfileString("Misc", "StartingMap", "", mapName, 64, ini)) {
		dlog("Applying starting map patch.", DL_INIT);
		SafeWrite32(0x00480AAA, (DWORD)&mapName);
		dlogr(" Done", DL_INIT);
	}

	versionString[64] = 0;
	if (GetPrivateProfileString("Misc", "VersionString", "", versionString, 64, ini)) {
		dlog("Applying version string patch.", DL_INIT);
		SafeWrite32(0x004B4588, (DWORD)&versionString);
		dlogr(" Done", DL_INIT);
	}

	configName[64] = 0;
	if (GetPrivateProfileString("Misc", "ConfigFile", "", configName, 64, ini)) {
		dlog("Applying config file patch.", DL_INIT);
		SafeWrite32(0x00444BA5, (DWORD)&configName);
		SafeWrite32(0x00444BCA, (DWORD)&configName);
		dlogr(" Done", DL_INIT);
	}

	patchName[64] = 0;
	if (GetPrivateProfileString("Misc", "PatchFile", "", patchName, 64, ini)) {
		dlog("Applying patch file patch.", DL_INIT);
		SafeWrite32(0x00444323, (DWORD)&patchName);
		dlogr(" Done", DL_INIT);
	}

	smModelName[64] = 0;
	if (GetPrivateProfileString("Misc", "MaleStartModel", "", smModelName, 64, ini)) {
		dlog("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x00418B88, (DWORD)&smModelName);
		dlogr(" Done", DL_INIT);
	}

	sfModelName[64] = 0;
	if (GetPrivateProfileString("Misc", "FemaleStartModel", "", sfModelName, 64, ini)) {
		dlog("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x00418BAB, (DWORD)&sfModelName);
		dlogr(" Done", DL_INIT);
	}

	dmModelName[64] = 0;
	GetPrivateProfileString("Misc", "MaleDefaultModel", "hmjmps", dmModelName, 64, ini);
	dlog("Applying male model patch.", DL_INIT);
	SafeWrite32(0x00418B50, (DWORD)&dmModelName);
	dlogr(" Done", DL_INIT);

	dfModelName[64] = 0;
	GetPrivateProfileString("Misc", "FemaleDefaultModel", "hfjmps", dfModelName, 64, ini);
	dlog("Applying female model patch.", DL_INIT);
	SafeWrite32(0x00418B6D, (DWORD)&dfModelName);
	dlogr(" Done", DL_INIT);

	ApplyStartingStatePatches();

	ApplyPathfinderFix();

	ApplyWorldmapFpsPatch();

	if (GetPrivateProfileIntA("Misc", "DialogueFix", 1, ini)) {
		dlog("Applying dialogue patch.", DL_INIT);
		SafeWrite8(0x00446848, 0x31);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "ExtraKillTypes", 0, ini)) {
		dlog("Applying extra kill types patch.", DL_INIT);
		KillCounterInit(true);
		dlogr(" Done", DL_INIT);
	} else {
		KillCounterInit(false);
	}

	ApplyScriptExtenderPatches();

	ApplyCombatProcFix();

	ApplyWorldLimitsPatches();

	ApplyTimeLimitPatch();

	ApplyDebugModePatch();

	ApplyNPCAutoLevelPatch();
	
	char elevPath[MAX_PATH];
	GetPrivateProfileString("Misc", "ElevatorsFile", "", elevPath, MAX_PATH, ini);
	if (strlen(elevPath)>0) {
		dlog("Applying elevator patch.", DL_INIT);
		ElevatorsInit(elevPath);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "UseFileSystemOverride", 0, ini)) {
		FileSystemInit();
	}

	/*if (GetPrivateProfileIntA("Misc", "PrintToFileFix", 0, ini)) {
		dlog("Applying print to file patch.", DL_INIT);
		SafeWrite32(0x6C0364, (DWORD)&FakeFindFirstFile);
		SafeWrite32(0x6C0368, (DWORD)&FakeFindNextFile);
		dlogr(" Done", DL_INIT);
	}*/

	ApplyAdditionalWeaponAnimsPatch();

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

	if (GetPrivateProfileInt("Misc", "DisplayKarmaChanges", 0, ini)) {
		dlog("Applying display karma changes patch.", DL_INIT);
		GetPrivateProfileString("sfall", "KarmaGain", "You gained %d karma.", KarmaGainMsg, 128, translationIni);
		GetPrivateProfileString("sfall", "KarmaLoss", "You lost %d karma.", KarmaLossMsg, 128, translationIni);
		HookCall(0x455A6D, SetGlobalVarWrapper);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "AlwaysReloadMsgs", 0, ini)) {
		dlog("Applying always reload messages patch.", DL_INIT);
		SafeWrite8(0x4A6B8A, 0xff);
		SafeWrite32(0x4A6B8B, 0x02eb0074);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileInt("Misc", "PlayIdleAnimOnReload", 0, ini)) {
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

	if (GetPrivateProfileIntA("Misc", "EnableHeroAppearanceMod", 0, ini)) {
		dlog("Setting up Appearance Char Screen buttons.", DL_INIT);
		EnableHeroAppearanceMod();
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "SkipOpeningMovies", 0, ini)) {
		dlog("Blocking opening movies.", DL_INIT);
		BlockCall(0x4809CB);
		BlockCall(0x4809D4);
		BlockCall(0x4809E0);
		dlogr(" Done", DL_INIT);
	}

	RetryCombatMinAP = GetPrivateProfileIntA("Misc", "NPCsTryToSpendExtraAP", 0, ini);
	if (RetryCombatMinAP) {
		dlog("Applying retry combat patch.", DL_INIT);
		HookCall(0x422B94, &RetryCombatHook);
		dlogr(" Done", DL_INIT);
	}

	ApplySkilldexImagesPatch();

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

	if (GetPrivateProfileIntA("Misc", "ExtraSaveSlots", 0, ini)) {
		dlog("Running EnableSuperSaving()", DL_INIT);
		EnableSuperSaving();
		dlogr(" Done", DL_INIT);
	}

	ApplySpeedInterfaceCounterAnimsPatch();

	ApplyKarmaFRMsPatch();

	ApplyScienceOnCrittersPatch();

	DWORD tmp;
	tmp = GetPrivateProfileIntA("Misc", "SpeedInventoryPCRotation", 166, ini);
	if (tmp != 166 && tmp <= 1000) {
		dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
		SafeWrite32(0x47066B, tmp);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running BarBoxesInit().", DL_INIT);
	BarBoxesInit();

	dlogr("Patching out ereg call.", DL_INIT);
	BlockCall(0x4425E6);

	tmp = GetPrivateProfileIntA("Misc", "AnimationsAtOnceLimit", 32, ini);
	if ((signed char)tmp > 32) {
		dlog("Applying AnimationsAtOnceLimit patch.", DL_INIT);
		AnimationsAtOnceInit((signed char)tmp);
		dlogr(" Done", DL_INIT);
	}

	if (GetPrivateProfileIntA("Misc", "RemoveCriticalTimelimits", 0, ini)) {
		dlog("Removing critical time limits.", DL_INIT);
		SafeWrite8(0x42412B, 0x90);
		BlockCall(0x42412C);
		SafeWrite16(0x4A3052, 0x9090);
		SafeWrite16(0x4A3093, 0x9090);
		dlogr(" Done", DL_INIT);
	}

	if (tmp = GetPrivateProfileIntA("Sound", "OverrideMusicDir", 0, ini)) {
		SafeWrite32(0x4449C2, (DWORD)musicOverridePath);
		SafeWrite32(0x4449DB, (DWORD)musicOverridePath);
		if (tmp == 2) {
			SafeWrite32(0x518E78, (DWORD)musicOverridePath);
			SafeWrite32(0x518E7C, (DWORD)musicOverridePath);
		}
	}

	if (GetPrivateProfileIntA("Misc", "NPCStage6Fix", 0, ini)) {
		dlog("Applying NPC Stage 6 Fix.", DL_INIT);
		MakeCall(0x493CE9, &NPCStage6Fix1, true);
		SafeWrite8(0x494063, 0x06);		// loop should look for a potential 6th stage
		SafeWrite8(0x4940BB, 0xCC);		// move pointer by 204 bytes instead of 200
		MakeCall(0x494224, &NPCStage6Fix2, true);
		dlogr(" Done", DL_INIT);
	}

	ApplyFashShotTraitFix();

	ApplyBoostScriptDialogLimitPatch();

	dlog("Running InventoryInit.", DL_INIT);
	InventoryInit();
	dlogr(" Done", DL_INIT);

	if (tmp = GetPrivateProfileIntA("Misc", "MotionScannerFlags", 1, ini)) {
		dlog("Applying MotionScannerFlags patch.", DL_INIT);
		if (tmp & 1) MakeCall(0x41BBE9, &ScannerAutomapHook, true);
		if (tmp & 2) BlockCall(0x41BC3C);
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

	if (GetPrivateProfileIntA("Misc", "DisablePipboyAlarm", 0, ini)) {
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

	ApplyInstantWeaponEquipPatch();	

	ApplyNumbersInDialoguePatch();

	dlogr("Leave DllMain2", DL_MAIN);
}

static void _stdcall OnExit() {
	MiscReset();
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
	DWORD size = MAX_PATH;
	DWORD type;
	if (!(type = RegOpenKeyEx(root, "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers", 0, extra | STANDARD_RIGHTS_READ | KEY_QUERY_VALUE, &key))) {
		if (!RegQueryValueEx(key, filepath, 0, &type, (BYTE*)buf, &size)) {
			if (size && (type == REG_EXPAND_SZ || type == REG_MULTI_SZ || type == REG_SZ)) {
				if (strstr(buf, "256COLOR") || strstr(buf, "640X480") || strstr(buf, "WIN")) {
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
			while (cmdline[0] == ' ') cmdline++;
			bool InQuote = false;
			int count = -1;

			while (true) {
				count++;
				if (cmdline[count] == 0) break;;
				if (cmdline[count] == ' ' && !InQuote) break;
				if (cmdline[count] == '"') {
					InQuote = !InQuote;
					if (!InQuote) break;
				}
			}
			if (cmdline[count] != 0) {
				count++;
				while (cmdline[count] == ' ') count++;
				cmdline = &cmdline[count];
				cmdlineexists = true;
			}
		}

		if (cmdlineexists && strlen(cmdline)) {
			strcpy_s(ini, ".\\");
			strcat_s(ini, cmdline);
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
			else {
				MessageBox(0, "You gave a command line argument to fallout, but it couldn't be matched to a file\n" \
						   "Using default ddraw.ini instead", "Warning", MB_TASKMODAL);
				strcpy_s(ini, ".\\ddraw.ini");
			}
		} else {
			strcpy_s(ini, ".\\ddraw.ini");
		}

		GetPrivateProfileStringA("Main", "TranslationsINI", "./Translations.ini", translationIni, 65, ini);

		DllMain2();
	}
	return true;
}
