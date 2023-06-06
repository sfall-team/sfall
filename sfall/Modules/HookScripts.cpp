/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

#include <string>
#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "HookScripts\Common.h"
#include "HookScripts\CombatHs.h"
#include "HookScripts\DeathHs.h"
#include "HookScripts\HexBlockingHs.h"
#include "HookScripts\InventoryHs.h"
#include "HookScripts\ObjectHs.h"
#include "HookScripts\MiscHs.h"

#include "HookScripts.h"

namespace sfall
{

// Number of types of hooks
static const int numHooks = HOOK_COUNT;

static bool injectAllHooks;

DWORD HookScripts::initingHookScripts;

static std::vector<HookFile> hookScriptFilesList;

typedef void(*HookInjectFunc)();

struct HooksInjectInfo {
	int id;
	HookInjectFunc inject;
	bool injectState;
};

static struct HooksPositionInfo {
	long hsPosition;    // index of the hs_* script, or the beginning of the position for registering scripts using register_hook
//	long positionShift; // offset to the last script registered by register_hook
	bool hasHsScript;

	HooksPositionInfo() : hsPosition(0), /*positionShift(0),*/ hasHsScript(false) {}
} hooksInfo[numHooks];

static HooksInjectInfo injectHooks[] = {
	{HOOK_TOHIT,            Inject_ToHitHook,            false},
	{HOOK_AFTERHITROLL,     Inject_AfterHitRollHook,     false},
	{HOOK_CALCAPCOST,       Inject_CalcApCostHook,       false},
	{HOOK_DEATHANIM1,       Inject_DeathAnim1Hook,       false},
	{HOOK_DEATHANIM2,       Inject_DeathAnim2Hook,       false},
	{HOOK_COMBATDAMAGE,     Inject_CombatDamageHook,     false},
	{HOOK_ONDEATH,          Inject_OnDeathHook,          false},
	{HOOK_FINDTARGET,       Inject_FindTargetHook,       false},
	{HOOK_USEOBJON,         Inject_UseObjOnHook,         false},
	{HOOK_REMOVEINVENOBJ,   Inject_RemoveInvenObjHook,   false},
	{HOOK_BARTERPRICE,      Inject_BarterPriceHook,      false},
	{HOOK_MOVECOST,         Inject_MoveCostHook,         false},
	{HOOK_HEXMOVEBLOCKING,  Inject_HexMoveBlockHook,     false},
	{HOOK_HEXAIBLOCKING,    Inject_HexIABlockHook,       false},
	{HOOK_HEXSHOOTBLOCKING, Inject_HexShootBlockHook,    false},
	{HOOK_HEXSIGHTBLOCKING, Inject_HexSightBlockHook,    false},
	{HOOK_ITEMDAMAGE,       Inject_ItemDamageHook,       false},
	{HOOK_AMMOCOST,         Inject_AmmoCostHook,         false},
	{HOOK_USEOBJ,           Inject_UseObjHook,           false},
	{HOOK_KEYPRESS,         nullptr,                      true}, // no embed code to the engine
	{HOOK_MOUSECLICK,       nullptr,                      true}, // no embed code to the engine
	{HOOK_USESKILL,         Inject_UseSkillHook,         false},
	{HOOK_STEAL,            Inject_StealCheckHook,       false},
	{HOOK_WITHINPERCEPTION, Inject_WithinPerceptionHook, false},
	{HOOK_INVENTORYMOVE,    Inject_InventoryMoveHook,    false},
	{HOOK_INVENWIELD,       Inject_InvenWieldHook,       false},
	{HOOK_ADJUSTFID,        nullptr,                      true}, // always embedded to the engine
	{-1,                    nullptr,                     false}, // dummy
	{HOOK_CARTRAVEL,        Inject_CarTravelHook,        false},
	{HOOK_SETGLOBALVAR,     Inject_SetGlobalVarHook,     false},
	{HOOK_RESTTIMER,        Inject_RestTimerHook,        false},
	{HOOK_GAMEMODECHANGE,   nullptr,                      true}, // always embedded to the engine
	{HOOK_USEANIMOBJ,       Inject_UseAnimateObjHook,    false},
	{HOOK_EXPLOSIVETIMER,   Inject_ExplosiveTimerHook,   false},
	{HOOK_DESCRIPTIONOBJ,   Inject_DescriptionObjHook,   false},
	{HOOK_USESKILLON,       Inject_UseSkillOnHook,       false},
	{HOOK_ONEXPLOSION,      Inject_OnExplosionHook,      false},
	{-1,                    nullptr,                     false}, // dummy
	{HOOK_SETLIGHTING,      Inject_SetLightingHook,      false},
	{HOOK_SNEAK,            Inject_SneakCheckHook,       false},
	{HOOK_STDPROCEDURE,     Inject_ScriptProcedureHook,  false},
	{HOOK_STDPROCEDURE_END, Inject_ScriptProcedureHook2, false},
	{HOOK_TARGETOBJECT,     Inject_TargetObjectHook,     false},
	{HOOK_ENCOUNTER,        Inject_EncounterHook,        false},
	{-1,                    nullptr,                     false}, // dummy
	{-1,                    nullptr,                     false}, // dummy
	{HOOK_ROLLCHECK,        Inject_RollCheckHook,        false},
	{HOOK_BESTWEAPON,       Inject_BestWeaponHook,       false},
	{HOOK_CANUSEWEAPON,     Inject_CanUseWeaponHook,     false},
};

void HookScripts::InjectingHook(int hookId) {
	if (!IsInjectHook(hookId) && injectHooks[hookId].id == hookId) {
		injectHooks[hookId].injectState = true;
		injectHooks[hookId].inject();
		devlog_f("Inject hook ID: %d\n", DL_INIT, hookId);
	}
}

bool HookScripts::IsInjectHook(int hookId) {
	return injectHooks[hookId].injectState;
}

bool HookScripts::HookHasScript(int hookId) {
	return (!hooks[hookId].empty());
}

void HookScripts::RegisterHook(fo::Program* script, int id, int procNum, bool specReg) {
	if (id >= numHooks || injectHooks[id].id < 0) return;
	for (std::vector<HookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister
			return;
		}
	}
	if (procNum == 0) return; // prevent registration to first location in procedure when reusing "unregister" method

	ScriptProgram *prog = ScriptExtender::GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Script: %s registered as hook ID %d\n", DL_HOOK, script->fileName, id);
		HookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;

		std::vector<HookScript>::const_iterator c_it = hooks[id].cend();
		if (specReg) {
			c_it = hooks[id].cbegin();
			hooksInfo[id].hsPosition++;
		}
		hooks[id].insert(c_it, hook);

		HookScripts::InjectingHook(id); // inject hook to engine code
	}
}

// run specific event procedure for all hook scripts
void HookScripts::RunHookScriptsAtProc(DWORD procId) {
	for (int i = 0; i < numHooks; i++) {
		if (hooksInfo[i].hasHsScript /*&& !hooks[i][hooksInfo[i].hsPosition].isGlobalScript*/) {
			RunScriptProc(&hooks[i][hooksInfo[i].hsPosition].prog, procId); // run hs_*.int
		}
	}
}

void HookScripts::LoadHookScript(const char* name, int id) {
	char filePath[MAX_PATH];

	sprintf(filePath, "scripts\\%s.int", name);
	bool hookHasScript = fo::func::db_access(filePath);

	if (hookHasScript || injectAllHooks) {
		HookScripts::InjectingHook(id); // inject hook to engine code

		if (!hookHasScript) return; // only inject

		HookFile hookFile = {id, name};
		hookScriptFilesList.push_back(hookFile);

		dlog_f("Found hook script: %s\n", DL_HOOK, name);
	}
}

static void InitHookScriptFile(const char* name, int id) {
	ScriptProgram prog;
	dlog("> ", DL_HOOK);
	dlog(name, DL_HOOK);
	InitScriptProgram(prog, name);
	if (prog.ptr) {
		HookScript hook;
		hook.prog = prog;
		hook.callback = -1;
		hook.isGlobalScript = false;
		hooks[id].push_back(hook);
		ScriptExtender::AddProgramToMap(prog);
		dlogr(" Done", DL_HOOK);
	} else {
		dlogr(" Error!", DL_HOOK);
	}
	//return (prog.ptr != nullptr);
}

void HookScripts::LoadHookScripts() {
	dlogr("Loading hook scripts:", DL_HOOK|DL_INIT);

	static bool hooksFilesLoaded = false;
	if (!hooksFilesLoaded) { // hook files are already put in the list
		hookScriptFilesList.clear();

		InitCombatHookScripts();
		InitDeathHookScripts();
		InitHexBlockingHookScripts();
		InitInventoryHookScripts();
		InitObjectHookScripts();
		InitMiscHookScripts();

		HookScripts::LoadHookScript("hs_keypress", HOOK_KEYPRESS);
		HookScripts::LoadHookScript("hs_mouseclick", HOOK_MOUSECLICK);
		HookScripts::LoadHookScript("hs_gamemodechange", HOOK_GAMEMODECHANGE);

		hooksFilesLoaded = !alwaysFindScripts;
	}
	dlogr("Finished loading hook scripts.", DL_HOOK|DL_INIT);
}

void HookScripts::InitHookScripts() {
	// Note: here isGlobalScriptLoading must be already set, this should allow to register global exported variables
	dlogr("Running hook scripts...", DL_HOOK);

	for (std::vector<HookFile>::const_iterator it = hookScriptFilesList.begin(); it != hookScriptFilesList.end(); ++it) {
		InitHookScriptFile(it->name.c_str(), it->id);
	}

	initingHookScripts = 1;
	for (int i = 0; i < numHooks; i++) {
		if (!hooks[i].empty()) {
			hooksInfo[i].hasHsScript = true;
			RunScriptProgram(hooks[i][0].prog); // zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	initingHookScripts = 0;
}

void HookScripts::HookScriptClear() {
	for (int i = 0; i < numHooks; i++) {
		hooks[i].clear();
	}
	std::memset(hooksInfo, 0, numHooks * sizeof(HooksPositionInfo));
	HookCommon::Reset();
}

void HookScripts::init() {
	injectAllHooks = isDebug && (IniReader::GetIntDefaultConfig("Debugging", "InjectAllGameHooks", 0) != 0);
	if (injectAllHooks) dlogr("Injecting all game hooks.", DL_HOOK|DL_INIT);
}

}
