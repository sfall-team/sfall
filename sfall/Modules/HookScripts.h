/*
 *    sfall
 *    Copyright (C) 2008, 2009  The sfall team
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

#pragma once

#include "..\FalloutEngine\Structs.h"
#include "Module.h"

namespace sfall
{

enum HookType
{
	HOOK_TOHIT            = 0,
	HOOK_AFTERHITROLL     = 1,
	HOOK_CALCAPCOST       = 2,
	HOOK_DEATHANIM1       = 3,
	HOOK_DEATHANIM2       = 4,
	HOOK_COMBATDAMAGE     = 5,
	HOOK_ONDEATH          = 6,
	HOOK_FINDTARGET       = 7,
	HOOK_USEOBJON         = 8,
	HOOK_REMOVEINVENOBJ   = 9,
	HOOK_BARTERPRICE      = 10,
	HOOK_MOVECOST         = 11,
	HOOK_HEXMOVEBLOCKING  = 12,
	HOOK_HEXAIBLOCKING    = 13,
	HOOK_HEXSHOOTBLOCKING = 14,
	HOOK_HEXSIGHTBLOCKING = 15,
	HOOK_ITEMDAMAGE       = 16,
	HOOK_AMMOCOST         = 17,
	HOOK_USEOBJ           = 18,
	HOOK_KEYPRESS         = 19,
	HOOK_MOUSECLICK       = 20,
	HOOK_USESKILL         = 21,
	HOOK_STEAL            = 22,
	HOOK_WITHINPERCEPTION = 23,
	HOOK_INVENTORYMOVE    = 24,
	HOOK_INVENWIELD       = 25,
	HOOK_ADJUSTFID        = 26,
	HOOK_COMBATTURN       = 27,
	HOOK_CARTRAVEL        = 28,
	HOOK_SETGLOBALVAR     = 29,
	HOOK_RESTTIMER        = 30,
	HOOK_GAMEMODECHANGE   = 31,
	HOOK_USEANIMOBJ       = 32,
	HOOK_EXPLOSIVETIMER   = 33,
	HOOK_DESCRIPTIONOBJ   = 34,
	HOOK_USESKILLON       = 35,
	HOOK_ONEXPLOSION      = 36,
	HOOK_SUBCOMBATDAMAGE  = 37,
	HOOK_SETLIGHTING      = 38,
	HOOK_SNEAK            = 39,
	HOOK_STDPROCEDURE     = 40,
	HOOK_STDPROCEDURE_END = 41,
	HOOK_TARGETOBJECT     = 42,
	HOOK_ENCOUNTER        = 43,
	HOOK_COUNT
};

class HookScripts : public Module {

public:
	const char* name() { return "HookScripts"; }
	void init();

	static bool HookHasScript(int hookId);

	static bool injectAllHooks;
	static void InjectingHook(int hookId);
	static bool IsInjectHook(int hookId);

	static void GameModeChangeHook(DWORD exit);
	static void KeyPressHook(DWORD* dxKey, bool pressed, DWORD vKey);

	static DWORD GetHSArgCount();
	static DWORD GetHSArg();
	static DWORD GetHSArgAt(DWORD id);
	static DWORD* GetHSArgs();
	static void SetHSArg(DWORD id, DWORD value);
	static void __stdcall SetHSReturn(DWORD d);
};

// register hook by proc num (special values: -1 - use default (start) procedure, 0 - unregister)
void RegisterHook(fo::Program* script, int id, int procNum, bool specReg);

// TODO: move
void HookScriptClear();
void LoadHookScripts();

extern DWORD initingHookScripts;
extern int __fastcall AmmoCostHook_Script(DWORD hookType, fo::GameObject* weapon, DWORD &rounds);
void RunHookScriptsAtProc(DWORD procId);

}
