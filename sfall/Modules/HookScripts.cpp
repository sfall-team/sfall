/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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
#include "..\InputFuncs.h"
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
static constexpr int numHooks = HOOK_COUNT;

bool injectAllHooks;

DWORD HookScripts::initingHookScripts;

std::vector<HookFile> HookScripts::hookScriptFilesList;

typedef void(*HookInjectFunc)();

struct HooksInjectInfo {
	int id;
	HookInjectFunc inject;
	char injectState;
};

static struct HooksPositionInfo {
	long hsPosition    = 0; // index of the hs_* script, or the beginning of the position for registering scripts using register_hook
//	long positionShift = 0; // offset to the last script registered by register_hook
	bool hasHsScript   = false;
} hooksInfo[numHooks];

static HooksInjectInfo injectHooks[] = {
	{HOOK_TOHIT,            Inject_ToHitHook,            0},
	{HOOK_AFTERHITROLL,     Inject_AfterHitRollHook,     0},
	{HOOK_CALCAPCOST,       Inject_CalcApCostHook,       0},
	{HOOK_DEATHANIM1,       Inject_DeathAnim1Hook,       0},
	{HOOK_DEATHANIM2,       Inject_DeathAnim2Hook,       0},
	{HOOK_COMBATDAMAGE,     Inject_CombatDamageHook,     0},
	{HOOK_ONDEATH,          Inject_OnDeathHook,          0},
	{HOOK_FINDTARGET,       Inject_FindTargetHook,       0},
	{HOOK_USEOBJON,         Inject_UseObjOnHook,         0},
	{HOOK_REMOVEINVENOBJ,   Inject_RemoveInvenObjHook,   0},
	{HOOK_BARTERPRICE,      Inject_BarterPriceHook,      0},
	{HOOK_MOVECOST,         Inject_MoveCostHook,         0},
	{HOOK_HEXMOVEBLOCKING,  Inject_HexMoveBlockHook,     0},
	{HOOK_HEXAIBLOCKING,    Inject_HexIABlockHook,       0},
	{HOOK_HEXSHOOTBLOCKING, Inject_HexShootBlockHook,    0},
	{HOOK_HEXSIGHTBLOCKING, Inject_HexSightBlockHook,    0},
	{HOOK_ITEMDAMAGE,       Inject_ItemDamageHook,       0},
	{HOOK_AMMOCOST,         Inject_AmmoCostHook,         0},
	{HOOK_USEOBJ,           Inject_UseObjHook,           0},
	{HOOK_KEYPRESS,         nullptr,                     2}, // no embed code to the engine
	{HOOK_MOUSECLICK,       nullptr,                     2}, // no embed code to the engine
	{HOOK_USESKILL,         Inject_UseSkillHook,         0},
	{HOOK_STEAL,            Inject_StealCheckHook,       0},
	{HOOK_WITHINPERCEPTION, Inject_WithinPerceptionHook, 0},
	{HOOK_INVENTORYMOVE,    Inject_InventoryMoveHook,    0},
	{HOOK_INVENWIELD,       Inject_InvenWieldHook,       0},
	{HOOK_ADJUSTFID,        nullptr,                     2}, // always embedded to the engine
	{HOOK_COMBATTURN,       Inject_CombatTurnHook,       0},
	{HOOK_CARTRAVEL,        Inject_CarTravelHook,        0},
	{HOOK_SETGLOBALVAR,     Inject_SetGlobalVarHook,     0},
	{HOOK_RESTTIMER,        Inject_RestTimerHook,        0},
	{HOOK_GAMEMODECHANGE,   nullptr,                     2}, // always embedded to the engine
	{HOOK_USEANIMOBJ,       Inject_UseAnimateObjHook,    0},
	{HOOK_EXPLOSIVETIMER,   Inject_ExplosiveTimerHook,   0},
	{HOOK_DESCRIPTIONOBJ,   Inject_DescriptionObjHook,   0},
	{HOOK_USESKILLON,       Inject_UseSkillOnHook,       0},
	{HOOK_ONEXPLOSION,      Inject_OnExplosionHook,      0},
	{HOOK_SUBCOMBATDAMAGE,  Inject_SubCombatDamageHook,  0}, // replace the code logic
	{HOOK_SETLIGHTING,      Inject_SetLightingHook,      0},
	{HOOK_SNEAK,            Inject_SneakCheckHook,       0},
	{HOOK_STDPROCEDURE,     Inject_ScriptProcedureHook,  0},
	{HOOK_STDPROCEDURE_END, Inject_ScriptProcedureHook2, 0},
	{HOOK_TARGETOBJECT,     Inject_TargetObjectHook,     0},
	{HOOK_ENCOUNTER,        Inject_EncounterHook,        0},
	{HOOK_ADJUSTPOISON,     Inject_AdjustPoisonHook,     0},
	{HOOK_ADJUSTRADS,       Inject_AdjustRadsHook,       1},  // always embedded for party control fix
	{HOOK_ROLLCHECK,        Inject_RollCheckHook,        0},
};

void HookScripts::InjectingHook(int hookId) {
	if (!IsInjectHook(hookId) && injectHooks[hookId].id == hookId) {
		injectHooks[hookId].injectState = 2;
		injectHooks[hookId].inject();
		devlog_f("Inject hook ID: %d\n", DL_INIT, hookId);
	}
}

bool HookScripts::IsInjectHook(int hookId) {
	return injectHooks[hookId].injectState == 2;
}

bool HookScripts::HookHasScript(int hookId) {
	return (!hooks[hookId].empty());
}

void HookScripts::RegisterHook(fo::Program* script, int id, int procNum, bool specReg) {
	if (id >= numHooks) return;
	for (std::vector<HookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister
			return;
		}
	}
	if (procNum == 0) return; // prevent registration to first location in procedure when reusing "unregister" method

	ScriptProgram *prog = ScriptExtender::GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Global script: %08x registered as hook ID %d\n", DL_HOOK, script, id);
		HookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;

		auto c_it = hooks[id].cend();
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
	//if (id >= numHooks || IsGameScript(name)) return;

	bool hookIsLoaded = HookScripts::LoadHookScriptFile(name, id);
	if (hookIsLoaded || injectHooks[id].injectState == 1 || (injectAllHooks && id != HOOK_SUBCOMBATDAMAGE)) {
		HookScripts::InjectingHook(id); // inject hook to engine code

		if (!hookIsLoaded) return; // only inject

		HookFile hookFile = { id, name };
		HookScripts::hookScriptFilesList.push_back(hookFile);
	}
}

bool HookScripts::LoadHookScriptFile(const char* name, int id) {
	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);

	ScriptProgram prog;
	if (fo::func::db_access(filename)) {
		dlog("> ", DL_HOOK);
		dlog(name, DL_HOOK);
		LoadScriptProgram(prog, name);
		if (prog.ptr) {
			dlogr(" Done", DL_HOOK);
			HookScript hook;
			hook.prog = prog;
			hook.callback = -1;
			hook.isGlobalScript = false;
			hooks[id].push_back(hook);
			ScriptExtender::AddProgramToMap(prog);
		} else {
			dlogr(" Error!", DL_HOOK);
		}
	}
	return (prog.ptr != nullptr);
}

static void HookScriptInit() {
	dlogr("Loading hook scripts:", DL_HOOK|DL_INIT);

	static bool hooksFilesLoaded = false;
	if (!hooksFilesLoaded) { // hook files are already put in the list
		HookScripts::hookScriptFilesList.clear();

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
	} else {
		for (auto& hook : HookScripts::hookScriptFilesList) {
			HookScripts::LoadHookScriptFile(hook.name.c_str(), hook.id);
		}
	}
	dlogr("Finished loading hook scripts.", DL_HOOK|DL_INIT);
}

void HookScripts::LoadHookScripts() {
	isGlobalScriptLoading = 1; // this should allow to register global exported variables
	HookScriptInit();
	initingHookScripts = 1;
	for (int i = 0; i < numHooks; i++) {
		if (!hooks[i].empty()) {
			hooksInfo[i].hasHsScript = true;
			InitScriptProgram(hooks[i][0].prog); // zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	isGlobalScriptLoading = 0;
	initingHookScripts = 0;
}

void HookScripts::HookScriptClear() {
	for(int i = 0; i < numHooks; i++) {
		hooks[i].clear();
	}
	std::memset(hooksInfo, 0, numHooks * sizeof(HooksPositionInfo));
	HookCommon::Reset();
}

void HookScripts::init() {
	OnMouseClick() += HookCommon::MouseClickHook;
	LoadGameHook::OnGameModeChange() += HookCommon::GameModeChangeHook;
	LoadGameHook::OnAfterGameStarted() += SourceUseSkillOnInit;

	injectAllHooks = isDebug && (iniGetInt("Debugging", "InjectAllGameHooks", 0, ::sfall::ddrawIni) != 0);
	if (injectAllHooks) dlogr("Injecting all game hooks", DL_HOOK|DL_INIT);
}

}
