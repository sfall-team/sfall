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
	}
}

static void HookScriptInit2() {
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
	HookScriptInit2();
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
}

}
