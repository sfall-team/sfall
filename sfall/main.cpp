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

#include <algorithm>
#include <math.h>
#include <stdio.h>

#include "Define.h"
#include "FalloutEngine.h"
#include "AI.h"
#include "AnimationsAtOnceLimit.h"
#include "BarBoxes.h"
#include "Books.h"
#include "BugFixes.h"
#include "BurstMods.h"
#include "Combat.h"
#include "Console.h"
#include "CRC.h"
#include "Credits.h"
#include "Criticals.h"
#include "DamageMod.h"
#include "DebugEditor.h"
#include "Elevators.h"
#include "Explosions.h"
#include "FileSystem.h"
#include "Graphics.h"
#include "HeroAppearance.h"
#include "Inventory.h"
#include "Karma.h"
#include "KillCounter.h"
#include "LoadGameHook.h"
#include "LoadOrder.h"
#include "Logging.h"
#include "MainMenu.h"
#include "Message.h"
#include "Movies.h"
#include "Objects.h"
#include "PartyControl.h"
#include "Perks.h"
#include "Premade.h"
#include "QuestList.h"
#include "Reputations.h"
#include "ScriptExtender.h"
#include "Skills.h"
#include "Sound.h"
#include "SpeedPatch.h"
#include "Stats.h"
#include "SuperSave.h"
#include "TalkingHeads.h"
#include "Tiles.h"
#include "Utils.h"
#include "version.h"
#include "Worldmap.h"

ddrawDll ddraw;

bool isDebug = false;

bool hrpIsEnabled = false;
bool hrpVersionValid = false; // HRP 4.1.8 version validation

const char ddrawIniDef[] = ".\\ddraw.ini";
static char ini[65] = ".\\";
static char translationIni[65];

DWORD modifiedIni;
DWORD hrpDLLBaseAddr = 0x10000000;

DWORD HRPAddressOffset(DWORD offset) {
	return (hrpDLLBaseAddr + offset);
}

int iniGetInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	return GetPrivateProfileIntA(section, setting, defaultValue, iniFile);
}

size_t iniGetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	return GetPrivateProfileStringA(section, setting, defaultValue, buf, bufSize, iniFile);
}

std::string GetIniString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile) {
	char* buf = new char[bufSize];
	iniGetString(section, setting, defaultValue, buf, bufSize, iniFile);
	std::string str(buf);
	delete[] buf;
	return str;
}

std::vector<std::string> GetIniList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile) {
	auto list = split(GetIniString(section, setting, defaultValue, bufSize, iniFile), delimiter);
	std::transform(list.cbegin(), list.cend(), list.begin(), trim);
	return list;
}

/*
	For ddraw.ini config
*/
unsigned int GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return iniGetInt(section, setting, defaultValue, ini);
}

std::string GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return trim(GetIniString(section, setting, defaultValue, bufSize, ini));
}

size_t GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize) {
	return iniGetString(section, setting, defaultValue, buf, bufSize, ini);
}

std::vector<std::string> GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniList(section, setting, defaultValue, bufSize, ',', ini);
}

std::string Translate(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniString(section, setting, defaultValue, bufSize, translationIni);
}

size_t Translate(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize) {
	return iniGetString(section, setting, defaultValue, buffer, bufSize, translationIni);
}

static char mapName[65];
static char configName[65];
static char patchName[65];
static char versionString[65];

static char startMaleModelName[65];
char defaultMaleModelName[65];
static char startFemaleModelName[65];
char defaultFemaleModelName[65];

static const char* musicOverridePath = "data\\sound\\music\\";

static int* scriptDialog = nullptr;

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

static const DWORD WalkDistanceAddr[] = {
	0x411FF0, 0x4121C4, 0x412475, 0x412906,
};

static void __declspec(naked) WeaponAnimHook() {
	__asm {
		cmp edx, 11;
		je  c11;
		cmp edx, 15;
		je  c15;
		jmp art_get_code_;
c11:
		mov edx, 16;
		jmp art_get_code_;
c15:
		mov edx, 17;
		jmp art_get_code_;
	}
}

static void __declspec(naked) ReloadHook() {
	__asm {
		push eax;
		push ebx;
		push edx;
		mov eax, dword ptr ds:[_obj_dude];
		call register_clear_;
		xor eax, eax;
		inc eax;
		call register_begin_;
		xor edx, edx;
		xor ebx, ebx;
		mov eax, dword ptr ds:[_obj_dude];
		dec ebx;
		call register_object_animate_;
		call register_end_;
		pop edx;
		pop ebx;
		pop eax;
		jmp gsound_play_sfx_file_;
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

static void __declspec(naked) ScienceCritterCheckHook() {
	__asm {
		cmp esi, ds:[_obj_dude];
		jne end;
		mov eax, 10;
		retn;
end:
		jmp critter_kill_count_type_;
	}
}

static const DWORD ScannerHookRet = 0x41BC1D;
static const DWORD ScannerHookFail = 0x41BC65;
static void __declspec(naked) ScannerAutomapHook() {
	__asm {
		mov eax, ds:[_obj_dude];
		mov edx, PID_MOTION_SENSOR;
		call inven_pid_is_carried_ptr_;
		test eax, eax;
		jz fail;
		mov edx, eax;
		jmp ScannerHookRet;
fail:
		jmp ScannerHookFail;
	}
}

static void __declspec(naked) op_obj_can_see_obj_hook() {
	__asm {
		push obj_shoot_blocking_at_;      // check hex objects func pointer
		push 0x20;                        // flags, 0x20 = check ShootThru
		mov  ecx, dword ptr [esp + 0x0C]; // buf **ret_objStruct
		push ecx;
		xor  ecx, ecx;
		call make_straight_path_func_;    // (EAX *objStruct, EDX hexNum1, EBX hexNum2, ECX 0, stack1 **ret_objStruct, stack2 flags, stack3 *check_hex_objs_func)
		retn 8;
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
		call art_id_
		xor  ebx, ebx
		dec  ebx
		xchg edx, eax
		pop  eax
		call register_object_change_fid_
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

static DWORD __fastcall GetWeaponSlotMode(DWORD itemPtr, DWORD mode) {
	int slot = (mode > 0) ? 1 : 0;
	DWORD* itemButton = ptr_itemButtonItems;
	if (itemButton[slot * 6] == itemPtr) {
		int slotMode = itemButton[(slot * 6) + 4];
		if (slotMode == 3 || slotMode == 4) {
			mode++;
		}
	}
	return mode;
}

static void __declspec(naked) display_stats_hook() {
	__asm {
		push eax;
		push ecx;
		mov ecx, ds:[esp + edi + 0xA8 + 0xC];   // get itemPtr
		call GetWeaponSlotMode;                 // ecx - itemPtr, edx - mode;
		mov edx, eax;
		pop ecx;
		pop eax;
		jmp item_w_range_;
	}
}

static void DllMain2() {
	DWORD tmp;
	dlogr("In DllMain2", DL_MAIN);

	dlogr("Running BugFixesInit().", DL_INIT);
	BugFixesInit();

	dlogr("Running GraphicsInit().", DL_INIT);
	GraphicsInit();

	//if (GetConfigInt("Input", "Enable", 0)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(0x50FB70, "ddraw.dll");
		AvailableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}

	dlogr("Running LoadOrderInit().", DL_INIT);
	LoadOrderInit();

	dlogr("Running LoadGameHookInit().", DL_INIT);
	LoadGameHookInit();

	dlogr("Running MoviesInit().", DL_INIT);
	MoviesInit();

	dlog("Applying main menu patches.", DL_INIT);
	MainMenuInit();
	dlogr(" Done", DL_INIT);

	dlogr("Running ObjectsInit().", DL_INIT);
	ObjectsInit();

	dlogr("Running SpeedPatchInit().", DL_INIT);
	SpeedPatchInit();

	startMaleModelName[64] = 0;
	if (GetConfigString("Misc", "MaleStartModel", "", startMaleModelName, 64)) {
		dlog("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x418B88, (DWORD)&startMaleModelName);
		dlogr(" Done", DL_INIT);
	}

	startFemaleModelName[64] = 0;
	if (GetConfigString("Misc", "FemaleStartModel", "", startFemaleModelName, 64)) {
		dlog("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x418BAB, (DWORD)&startFemaleModelName);
		dlogr(" Done", DL_INIT);
	}

	defaultMaleModelName[64] = 0;
	GetConfigString("Misc", "MaleDefaultModel", "hmjmps", defaultMaleModelName, 64);
	dlog("Applying male model patch.", DL_INIT);
	SafeWrite32(0x418B50, (DWORD)&defaultMaleModelName);
	dlogr(" Done", DL_INIT);

	defaultFemaleModelName[64] = 0;
	GetConfigString("Misc", "FemaleDefaultModel", "hfjmps", defaultFemaleModelName, 64);
	dlog("Applying female model patch.", DL_INIT);
	SafeWrite32(0x418B6D, (DWORD)&defaultFemaleModelName);
	dlogr(" Done", DL_INIT);

	dlogr("Running WorldmapInit().", DL_INIT);
	WorldmapInit();

	dlogr("Running StatsInit().", DL_INIT);
	StatsInit();

	dlogr("Running PerksInit().", DL_INIT);
	PerksInit();

	dlogr("Running CombatInit().", DL_INIT);
	CombatInit();

	dlogr("Running SkillsInit().", DL_INIT);
	SkillsInit();

	dlogr("Running FileSystemInit().", DL_INIT);
	FileSystemInit();

	dlogr("Running CriticalsInit().", DL_INIT);
	CriticalsInit();

	dlogr("Running KarmaInit().", DL_INIT);
	KarmaInit();

	dlogr("Running TilesInit().", DL_INIT);
	TilesInit();

	dlogr("Running CreditsInit().", DL_INIT);
	CreditsInit();

	dlogr("Running QuestListInit().", DL_INIT);
	QuestListInit();

	dlogr("Running PremadeInit().", DL_INIT);
	PremadeInit();

	dlogr("Running SoundInit().", DL_INIT);
	SoundInit();

	dlogr("Running ReputationsInit().", DL_INIT);
	ReputationsInit();

	dlogr("Running ConsoleInit().", DL_INIT);
	ConsoleInit();

	if (GetConfigInt("Misc", "ExtraSaveSlots", 0)) {
		dlogr("Running EnableSuperSaving().", DL_INIT);
		EnableSuperSaving();
	}

	dlogr("Running InventoryInit().", DL_INIT);
	InventoryInit();

	dlogr("Initializing party control...", DL_INIT);
	PartyControlInit();

	dlogr("Running ComputeSprayModInit().", DL_INIT);
	ComputeSprayModInit();

	dlogr("Running BooksInit().", DL_INIT);
	BooksInit();

	dlogr("Running ExplosionInit().", DL_INIT);
	ExplosionInit();

	std::string elevPath = GetConfigString("Misc", "ElevatorsFile", "", MAX_PATH);
	if (!elevPath.empty()) {
		dlog("Applying elevator patch.", DL_INIT);
		ElevatorsInit();
		LoadElevators(elevPath.insert(0, ".\\").c_str());
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "ExtraKillTypes", 0)) {
		dlog("Applying extra kill types patch.", DL_INIT);
		KillCounterInit();
		dlogr(" Done", DL_INIT);
	}

	dlogr("Running AIInit().", DL_INIT);
	AIInit();

	dlogr("Running DamageModInit().", DL_INIT);
	DamageModInit();

	dlogr("Running AnimationsAtOnceInit().", DL_INIT);
	AnimationsAtOnceInit();

	dlogr("Running BarBoxesInit().", DL_INIT);
	BarBoxesInit();

	dlogr("Running HeroAppearanceModInit().", DL_INIT);
	HeroAppearanceModInit();

	mapName[64] = 0;
	if (GetConfigString("Misc", "StartingMap", "", mapName, 64)) {
		dlog("Applying starting map patch.", DL_INIT);
		SafeWrite32(0x480AAA, (DWORD)&mapName);
		dlogr(" Done", DL_INIT);
	}

	versionString[64] = 0;
	if (GetConfigString("Misc", "VersionString", "", versionString, 64)) {
		dlog("Applying version string patch.", DL_INIT);
		SafeWrite32(0x4B4588, (DWORD)&versionString);
		dlogr(" Done", DL_INIT);
	}

	configName[64] = 0;
	if (GetConfigString("Misc", "ConfigFile", "", configName, 64)) {
		dlog("Applying config file patch.", DL_INIT);
		SafeWrite32(0x444BA5, (DWORD)&configName);
		SafeWrite32(0x444BCA, (DWORD)&configName);
		dlogr(" Done", DL_INIT);
	}

	patchName[64] = 0;
	if (GetConfigString("Misc", "PatchFile", "", patchName, 64)) {
		dlog("Applying patch file patch.", DL_INIT);
		SafeWrite32(0x444323, (DWORD)&patchName);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "SingleCore", 1)) {
		dlog("Applying single core patch.", DL_INIT);
		HANDLE process = GetCurrentProcess();
		SetProcessAffinityMask(process, 1);
		CloseHandle(process);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "OverrideArtCacheSize", 0)) {
		dlog("Applying override art cache size patch.", DL_INIT);
		SafeWrite32(0x418867, 0x90909090);
		SafeWrite32(0x418872, 256);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "Fallout1Behavior", 0)) {
		dlog("Applying Fallout 1 engine behavior patch.", DL_INIT);
		BlockCall(0x4A4343); // disable playing the final movie/credits after the endgame slideshow
		SafeWrite8(0x477C71, 0xEB); // disable halving the weight for power armor items
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "DialogueFix", 1)) {
		dlog("Applying dialogue patch.", DL_INIT);
		SafeWrite8(0x446848, 0x31);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "AdditionalWeaponAnims", 0)) {
		dlog("Applying additional weapon animations patch.", DL_INIT);
		SafeWrite8(0x419320, 0x12);
		HookCall(0x4194CC, WeaponAnimHook);
		HookCall(0x451648, WeaponAnimHook);
		HookCall(0x451671, WeaponAnimHook);
		dlogr(" Done", DL_INIT);
	}

	//if (GetConfigInt("Misc", "MultiPatches", 0)) {
		dlog("Applying load multiple patches patch.", DL_INIT);
		SafeWrite8(0x444354, 0x90); // Change step from 2 to 1
		SafeWrite8(0x44435C, 0xC4); // Disable check
		dlogr(" Done", DL_INIT);
	//}

	if (GetConfigInt("Misc", "AlwaysReloadMsgs", 0)) {
		dlog("Applying always reload messages patch.", DL_INIT);
		SafeWrite8(0x4A6B8D, 0x0);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "PlayIdleAnimOnReload", 0)) {
		dlog("Applying idle anim on reload patch.", DL_INIT);
		HookCall(0x460B8C, ReloadHook);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "CorpseLineOfFireFix", 1)) {
		dlog("Applying corpse line of fire patch.", DL_INIT);
		MakeJump(0x48B994, CorpseHitFix2);
		MakeJump(0x48BA04, CorpseHitFix2b);
		dlogr(" Done", DL_INIT);
	}

	dlog("Checking for changed skilldex images.", DL_INIT);
	tmp = GetConfigInt("Misc", "Lockpick", 293);
	if (tmp != 293) SafeWrite32(0x518D54, tmp);
	tmp = GetConfigInt("Misc", "Steal", 293);
	if (tmp != 293) SafeWrite32(0x518D58, tmp);
	tmp = GetConfigInt("Misc", "Traps", 293);
	if (tmp != 293) SafeWrite32(0x518D5C, tmp);
	tmp = GetConfigInt("Misc", "FirstAid", 293);
	if (tmp != 293) SafeWrite32(0x518D4C, tmp);
	tmp = GetConfigInt("Misc", "Doctor", 293);
	if (tmp != 293) SafeWrite32(0x518D50, tmp);
	tmp = GetConfigInt("Misc", "Science", 293);
	if (tmp != 293) SafeWrite32(0x518D60, tmp);
	tmp = GetConfigInt("Misc", "Repair", 293);
	if (tmp != 293) SafeWrite32(0x518D64, tmp);
	dlogr(" Done", DL_INIT);

	switch (GetConfigInt("Misc", "SpeedInterfaceCounterAnims", 0)) {
	case 1:
		dlog("Applying SpeedInterfaceCounterAnims patch.", DL_INIT);
		MakeJump(0x460BA1, intface_rotate_numbers_hack);
		dlogr(" Done", DL_INIT);
		break;
	case 2:
		dlog("Applying SpeedInterfaceCounterAnims patch. (Instant)", DL_INIT);
		SafeWrite32(0x460BB6, 0x90DB3190); // xor ebx, ebx
		dlogr(" Done", DL_INIT);
		break;
	}

	switch (GetConfigInt("Misc", "ScienceOnCritters", 0)) {
	case 1:
		HookCall(0x41276E, ScienceCritterCheckHook);
		break;
	case 2:
		SafeWrite8(0x41276A, 0xEB);
		break;
	}

	tmp = GetConfigInt("Misc", "SpeedInventoryPCRotation", 166);
	if (tmp != 166 && tmp <= 1000) {
		dlog("Applying SpeedInventoryPCRotation patch.", DL_INIT);
		SafeWrite32(0x47066B, tmp);
		dlogr(" Done", DL_INIT);
	}

	dlogr("Patching out ereg call.", DL_INIT);
	BlockCall(0x4425E6);

	if (tmp = GetConfigInt("Sound", "OverrideMusicDir", 0)) {
		SafeWrite32(0x4449C2, (DWORD)musicOverridePath);
		SafeWrite32(0x4449DB, (DWORD)musicOverridePath);
		if (tmp == 2) {
			SafeWrite32(0x518E78, (DWORD)musicOverridePath);
			SafeWrite32(0x518E7C, (DWORD)musicOverridePath);
		}
	}

	if (GetConfigInt("Misc", "BoostScriptDialogLimit", 0)) {
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

	if (tmp = GetConfigInt("Misc", "MotionScannerFlags", 1)) {
		dlog("Applying MotionScannerFlags patch.", DL_INIT);
		if (tmp & 1) MakeJump(0x41BBE9, ScannerAutomapHook);
		if (tmp & 2) {
			// automap_
			SafeWrite16(0x41BC24, 0x9090);
			BlockCall(0x41BC3C);
			// item_m_use_charged_item_
			SafeWrite8(0x4794B3, 0x5E); // jbe short 0x479512
		}
		dlogr(" Done", DL_INIT);
	}

	tmp = GetConfigInt("Misc", "EncounterTableSize", 0);
	if (tmp > 40 && tmp <= 127) {
		dlog("Applying EncounterTableSize patch.", DL_INIT);
		SafeWrite8(0x4BDB17, (BYTE)tmp);
		DWORD nsize = (tmp + 1) * 180 + 0x50;
		for (int i = 0; i < sizeof(EncounterTableSize) / 4; i++) {
			SafeWrite32(EncounterTableSize[i], nsize);
		}
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "DisablePipboyAlarm", 0)) {
		dlog("Applying Disable Pip-Boy alarm button patch.", DL_INIT);
		SafeWrite8(0x499518, 0xC3);
		SafeWrite8(0x443601, 0x0);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "ObjCanSeeObj_ShootThru_Fix", 0)) {
		dlog("Applying ObjCanSeeObj ShootThru Fix.", DL_INIT);
		HookCall(0x456BC6, op_obj_can_see_obj_hook);
		dlogr(" Done", DL_INIT);
	}

	// phobos2077:
	DWORD addrs[2] = {0x45F9DE, 0x45FB33};
	SimplePatch<WORD>(addrs, 2, "Misc", "CombatPanelAnimDelay", 1000, 0, 65535);
	addrs[0] = 0x447DF4; addrs[1] = 0x447EB6;
	SimplePatch<BYTE>(addrs, 2, "Misc", "DialogPanelAnimDelay", 33, 0, 255);
	addrs[0] = 0x499B99; addrs[1] = 0x499DA8;
	SimplePatch<BYTE>(addrs, 2, "Misc", "PipboyTimeAnimDelay", 50, 0, 127);

	if (GetConfigInt("Misc", "EnableMusicInDialogue", 0)) {
		dlog("Applying music in dialogue patch.", DL_INIT);
		SafeWrite8(0x44525B, 0x0);
		//BlockCall(0x450627);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "DontTurnOffSneakIfYouRun", 0)) {
		dlog("Applying DontTurnOffSneakIfYouRun patch.", DL_INIT);
		SafeWrite8(0x418135, 0xEB);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "InstantWeaponEquip", 0)) {
		//Skip weapon equip/unequip animations
		dlog("Applying instant weapon equip patch.", DL_INIT);
		for (int i = 0; i < sizeof(PutAwayWeapon) / 4; i++) {
			SafeWrite8(PutAwayWeapon[i], 0xEB);   // jmps
		}
		BlockCall(0x472AD5);                      //
		BlockCall(0x472AE0);                      // invenUnwieldFunc_
		BlockCall(0x472AF0);                      //
		MakeJump(0x415238, register_object_take_out_hack);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "NumbersInDialogue", 0)) {
		dlog("Applying numbers in dialogue patch.", DL_INIT);
		SafeWrite32(0x502C32, 0x2000202E);
		SafeWrite8(0x446F3B, 0x35);
		SafeWrite32(0x5029E2, 0x7325202E);
		SafeWrite32(0x446F03, 0x2424448B);        // mov  eax, [esp+0x24]
		SafeWrite8(0x446F07, 0x50);               // push eax
		SafeWrite32(0x446FE0, 0x2824448B);        // mov  eax, [esp+0x28]
		SafeWrite8(0x446FE4, 0x50);               // push eax
		MakeJump(0x4458F5, gdAddOptionStr_hack);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "DisplaySecondWeaponRange", 1)) {
		dlog("Applying display second weapon range patch.", DL_INIT);
		HookCall(0x472201, display_stats_hook);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigInt("Misc", "InterfaceDontMoveOnTop", 0)) {
		dlog("Applying InterfaceDontMoveOnTop patch.", DL_INIT);
		SafeWrite8(0x46ECE9, 0x10); // set only Exclusive flag for Player Inventory/Loot/UseOn
		SafeWrite8(0x41B966, 0x10); // set only Exclusive flag for Automap
		dlogr(" Done", DL_INIT);
	}

	int distance = GetConfigInt("Misc", "UseWalkDistance", 3) + 2;
	if (distance > 1 && distance < 5) {
		dlog("Applying walk distance for using objects patch.", DL_INIT);
		for (int i = 0; i < sizeof(WalkDistanceAddr) / 4; i++) {
			SafeWrite8(WalkDistanceAddr[i], static_cast<BYTE>(distance)); // default is 5
		}
		dlogr(" Done", DL_INIT);
	}

	int time = GetConfigInt("Misc", "CorpseDeleteTime", 6); // time in days
	if (time != 6) {
		dlog("Applying corpse deletion time patch.", DL_INIT);
		if (time <= 0) {
			time = 12; // hours
		} else if (time > 13) {
			time = 13 * 24;
		} else {
			time *= 24;
		}
		SafeWrite32(0x483348, time);
		dlogr(" Done", DL_INIT);
	}

	SimplePatch<DWORD>(0x440C2A, "Misc", "SpecialDeathGVAR", 491); // GVAR_MODOC_SHITTY_DEATH

	// Remove hardcoding for maps with IDs 19 and 37
	if (GetConfigInt("Misc", "DisableSpecialMapIDs", 0)) {
		dlog("Applying disable special map IDs patch.", DL_INIT);
		SafeWrite8(0x4836D6, 0);
		SafeWrite8(0x4836DB, 0);
		dlogr(" Done", DL_INIT);
	}

	// Increase the max text width of the information card in the character screen
	SafeWrite8(0x43ACD5, 144); // 136
	SafeWrite8(0x43DD37, 144); // 133

	dlogr("Running TalkingHeadsInit().", DL_INIT);
	TalkingHeadsInit();

	// most of modules should be initialized before running the script handlers
	dlogr("Running ScriptExtenderInit().", DL_INIT);
	ScriptExtenderInit();

	dlogr("Running DebugEditorInit().", DL_INIT);
	DebugEditorInit();

	dlogr("Leave DllMain2", DL_MAIN);
}

static void _stdcall OnExit() {
	if (scriptDialog != nullptr) {
		delete[] scriptDialog;
	}
	GraphicsExit();
	MoviesExit();
	SpeedPatchExit();
	SkillsExit();
	ReputationsExit();
	ConsoleExit();
	BooksExit();
	ClearReadExtraGameMsgFiles();
	AnimationsAtOnceExit();
	HeroAppearanceModExit();
	TalkingHeadsExit();
}

static void __declspec(naked) OnExitFunc() {
	__asm {
		pushad;
		call OnExit;
		popad;
		jmp DOSCmdLineDestroy_;
	}
}

static const DWORD loadFunc = 0x4FE1D0;
static void LoadHRPModule() {
	HMODULE dll;
	__asm call loadFunc; // get HRP loading address
	__asm mov  dll, eax;
	if (dll != NULL) hrpDLLBaseAddr = (DWORD)dll;
	dlog_f("Loaded f2_res.dll library at the memory address: 0x%x\n", DL_MAIN, dll);
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

					MessageBoxA(0, "Fallout appears to be running in compatibility mode.\n" //, and sfall was not able to disable it.\n"
								   "Please check the compatibility tab of fallout2.exe, and ensure that the following settings are unchecked:\n"
								   "Run this program in compatibility mode for..., run in 256 colours, and run in 640x480 resolution.\n"
								   "If these options are disabled, click the 'change settings for all users' button and see if that enables them.", "Error", MB_TASKMODAL | MB_ICONERROR);

					ExitProcess(-1);
				}
			}
		}
		RegCloseKey(key);
	}
}

static bool LoadOriginalDll(DWORD dwReason) {
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			char path[MAX_PATH];
			CopyMemory(path + GetSystemDirectoryA(path , MAX_PATH - 10), "\\ddraw.dll", 11); // path to original dll
			ddraw.dll = LoadLibraryA(path);
			if (ddraw.dll) {
				ddraw.AcquireDDThreadLock          = GetProcAddress(ddraw.dll, "AcquireDDThreadLock");
				ddraw.CheckFullscreen              = GetProcAddress(ddraw.dll, "CheckFullscreen");
				ddraw.CompleteCreateSysmemSurface  = GetProcAddress(ddraw.dll, "CompleteCreateSysmemSurface");
				ddraw.D3DParseUnknownCommand       = GetProcAddress(ddraw.dll, "D3DParseUnknownCommand");
				ddraw.DDGetAttachedSurfaceLcl      = GetProcAddress(ddraw.dll, "DDGetAttachedSurfaceLcl");
				ddraw.DDInternalLock               = GetProcAddress(ddraw.dll, "DDInternalLock");
				ddraw.DDInternalUnlock             = GetProcAddress(ddraw.dll, "DDInternalUnlock");
				ddraw.DSoundHelp                   = GetProcAddress(ddraw.dll, "DSoundHelp");
				ddraw.DirectDrawCreateClipper      = GetProcAddress(ddraw.dll, "DirectDrawCreateClipper");
				ddraw.DirectDrawCreate             = GetProcAddress(ddraw.dll, "DirectDrawCreate");
				ddraw.DirectDrawCreateEx           = GetProcAddress(ddraw.dll, "DirectDrawCreateEx");
				ddraw.DirectDrawEnumerateA         = GetProcAddress(ddraw.dll, "DirectDrawEnumerateA");
				ddraw.DirectDrawEnumerateExA       = GetProcAddress(ddraw.dll, "DirectDrawEnumerateExA");
				ddraw.DirectDrawEnumerateExW       = GetProcAddress(ddraw.dll, "DirectDrawEnumerateExW");
				ddraw.DirectDrawEnumerateW         = GetProcAddress(ddraw.dll, "DirectDrawEnumerateW");
				//ddraw.DllCanUnloadNow            = GetProcAddress(ddraw.dll, "DllCanUnloadNow");
				//ddraw.DllGetClassObject          = GetProcAddress(ddraw.dll, "DllGetClassObject");
				ddraw.GetDDSurfaceLocal            = GetProcAddress(ddraw.dll, "GetDDSurfaceLocal");
				ddraw.GetOLEThunkData              = GetProcAddress(ddraw.dll, "GetOLEThunkData");
				ddraw.GetSurfaceFromDC             = GetProcAddress(ddraw.dll, "GetSurfaceFromDC");
				ddraw.RegisterSpecialCase          = GetProcAddress(ddraw.dll, "RegisterSpecialCase");
				ddraw.ReleaseDDThreadLock          = GetProcAddress(ddraw.dll, "ReleaseDDThreadLock");
			}
			return true;
		case DLL_PROCESS_DETACH:
			if (ddraw.dll) FreeLibrary(ddraw.dll);
			break;
	}
	return false;
}

bool _stdcall DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	if (LoadOriginalDll(dwReason)) {
		// enabling debugging features
		isDebug = (iniGetInt("Debugging", "Enable", 0, ddrawIniDef) != 0);
		if (isDebug) {
			LoggingInit();
			if (!ddraw.dll) dlog("Error: Cannot load the original ddraw.dll library.\n", DL_MAIN);
		}

		HookCall(0x4DE7D2, &OnExitFunc);

		char filepath[MAX_PATH];
		GetModuleFileName(0, filepath, MAX_PATH);

		CRC(filepath);

		if (!isDebug || !iniGetInt("Debugging", "SkipCompatModeCheck", 0, ddrawIniDef)) {
			int is64bit;
			typedef int (_stdcall *chk64bitproc)(HANDLE, int*);
			HMODULE h = LoadLibrary("Kernel32.dll");
			chk64bitproc proc = (chk64bitproc)GetProcAddress(h, "IsWow64Process");
			if (proc)
				proc(GetCurrentProcess(), &is64bit);
			else
				is64bit = 0;
			FreeLibrary(h);

			CompatModeCheck(HKEY_CURRENT_USER, filepath, is64bit ? KEY_WOW64_64KEY : 0);
			CompatModeCheck(HKEY_LOCAL_MACHINE, filepath, is64bit ? KEY_WOW64_64KEY : 0);
		}

		// ini file override
		bool cmdlineexists = false;
		char* cmdline = GetCommandLineA();
		if (iniGetInt("Main", "UseCommandLine", 0, ddrawIniDef)) {
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

		if (cmdlineexists && *cmdline != 0) {
			HANDLE h = CreateFileA(cmdline, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (h != INVALID_HANDLE_VALUE) {
				CloseHandle(h);
				strcat_s(ini, cmdline);
			} else {
				MessageBoxA(0, "You gave a command line argument to Fallout, but it couldn't be matched to a file.\n" \
							   "Using default ddraw.ini instead.", "Warning", MB_TASKMODAL | MB_ICONWARNING);
				goto defaultIni;
			}
		} else {
defaultIni:
			strcpy(&ini[2], &ddrawIniDef[2]);
		}

		GetConfigString("Main", "TranslationsINI", ".\\Translations.ini", translationIni, 65);
		modifiedIni = GetConfigInt("Main", "ModifiedIni", 0);

		hrpIsEnabled = (*(DWORD*)0x4E4480 != 0x278805C7); // check if HRP is enabled
		if (hrpIsEnabled) {
			LoadHRPModule();
			if (strncmp((const char*)HRPAddressOffset(0x39940), "4.1.8", 5) == 0) hrpVersionValid = true;
		}

		DllMain2();
	}
	return true;
}
