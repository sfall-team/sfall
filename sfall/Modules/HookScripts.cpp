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
#include "HookScripts\Common.h"
#include "HookScripts\CombatHs.h"
#include "HookScripts\DeathHs.h"
#include "HookScripts\HexBlockingHs.h"
#include "HookScripts\InventoryHs.h"
#include "HookScripts\MiscHs.h"
#include "LoadGameHook.h"

#include "HookScripts.h"

namespace sfall
{

typedef void(*HookInjectFunc)();
struct HooksInjectInfo {
	int id;
	HookInjectFunc inject;
	bool isInject;
};

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
	{HOOK_COMBATTURN,       Inject_CombatTurnHook,       false},
	{HOOK_CARTRAVEL,        Inject_CarTravelHook,        false},
	{HOOK_SETGLOBALVAR,     Inject_SetGlobalVarHook,     false},
	{HOOK_RESTTIMER,        Inject_RestTimerHook,        false},
	{HOOK_GAMEMODECHANGE,   nullptr,                      true}, // always embedded to the engine
};

bool injectAllHooks;
DWORD initingHookScripts;

// BEGIN HOOKS

void _stdcall KeyPressHook(DWORD dxKey, bool pressed, DWORD vKey) {
	if (!IsMapLoaded()) {
		return;
	}
	BeginHook();
	argCount = 3;
	args[0] = (DWORD)pressed;
	args[1] = dxKey;
	args[2] = vKey;
	RunHookScript(HOOK_KEYPRESS);
	EndHook();
}

void _stdcall MouseClickHook(DWORD button, bool pressed) {
	if (!IsMapLoaded()) {
		return;
	}
	BeginHook();
	argCount = 2;
	args[0] = (DWORD)pressed;
	args[1] = button;
	RunHookScript(HOOK_MOUSECLICK);
	EndHook();
}

void _stdcall GameModeChangeHook(DWORD exit) {
	BeginHook();
	argCount = 1;
	args[0] = exit;
	RunHookScript(HOOK_GAMEMODECHANGE);
	EndHook();
}

// END HOOKS

DWORD _stdcall GetHSArgCount() {
	return argCount;
}

DWORD _stdcall GetHSArg() {
	if (cArg == argCount) return 0;
	else return args[cArg++];
}

void _stdcall SetHSArg(DWORD id, DWORD value) {
	if(id<argCount) args[id]=value;
}

DWORD* _stdcall GetHSArgs() {
	return args;
}

void _stdcall SetHSReturn(DWORD d) {
	if (cRetTmp < 8) {
		rets[cRetTmp++] = d;
	}
	if (cRetTmp > cRet) {
		cRet = cRetTmp;
	}
}

void HookScripts::InjectingHook(int hookId) {
	if (!injectHooks[hookId].isInject && injectHooks[hookId].id == hookId) {
		injectHooks[hookId].isInject = true;
		injectHooks[hookId].inject();
	}
}

void _stdcall RegisterHook(fo::Program* script, int id, int procNum) {
	if (id >= numHooks) return;
	for (std::vector<HookScript>::iterator it = hooks[id].begin(); it != hooks[id].end(); ++it) {
		if (it->prog.ptr == script) {
			if (procNum == 0) hooks[id].erase(it); // unregister
			return;
		}
	}
	ScriptProgram *prog = GetGlobalScriptProgram(script);
	if (prog) {
		dlog_f("Global script %08x registered as hook id %d\n", DL_HOOK, script, id);
		HookScript hook;
		hook.prog = *prog;
		hook.callback = procNum;
		hook.isGlobalScript = true;
		hooks[id].push_back(hook);
		HookScripts::InjectingHook(id); // inject hook to engine code
	}
}

static void HookScriptInit() {
	dlogr("Loading hook scripts", DL_HOOK|DL_INIT);

	InitCombatHookScripts();
	InitDeathHookScripts();
	InitHexBlockingHookScripts();
	InitInventoryHookScripts();
	InitMiscHookScripts();

	LoadHookScript("hs_keypress", HOOK_KEYPRESS);
	LoadHookScript("hs_mouseclick", HOOK_MOUSECLICK);
	LoadHookScript("hs_gamemodechange", HOOK_GAMEMODECHANGE);

	dlogr("Finished loading hook scripts", DL_HOOK|DL_INIT);
}

void HookScriptClear() {
	for(int i = 0; i < numHooks; i++) {
		hooks[i].clear();
	}
}

void LoadHookScripts() {
	isGlobalScriptLoading = 1; // this should allow to register global exported variables
	HookScriptInit();
	initingHookScripts = 1;
	for (int i = 0; i < numHooks; i++) {
		if (hooks[i].size()) {
			InitScriptProgram(hooks[i][0].prog);// zero hook is always hs_*.int script because Hook scripts are loaded BEFORE global scripts
		}
	}
	isGlobalScriptLoading = 0;
	initingHookScripts = 0;
}

// run specific event procedure for all hook scripts
void _stdcall RunHookScriptsAtProc(DWORD procId) {
	for (int i = 0; i < numHooks; i++) {
		if (hooks[i].size() > 0 && !hooks[i][0].isGlobalScript) {
			RunScriptProc(&hooks[i][0].prog, procId);
		}
	}
}

void HookScripts::init() {
	OnKeyPressed() += KeyPressHook;
	OnMouseClick() += MouseClickHook;
	LoadGameHook::OnGameModeChange() += GameModeChangeHook;

	injectAllHooks = (isDebug && (GetConfigInt("Debugging", "InjectingAllGameHooks", 0) != 0));
}

}
