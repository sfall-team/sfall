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
	HOOK_COUNT
};

class HookScripts : public Module {
	const char* name() { return "HookScripts"; }
	void init();
};

DWORD _stdcall GetHSArgCount();
DWORD _stdcall GetHSArg();
DWORD* _stdcall GetHSArgs();
void _stdcall SetHSArg(DWORD id, DWORD value);
void _stdcall SetHSReturn(DWORD d);
// register hook by proc num (special values: -1 - use default (start) procedure, 0 - unregister)
void _stdcall RegisterHook(TProgram* script, int id, int procNum);

// TODO: move
void HookScriptClear();
void LoadHookScripts();

extern DWORD InitingHookScripts;
extern void __declspec() AmmoCostHookWrapper();
void _stdcall MouseClickHook(DWORD button, bool pressed);
void _stdcall KeyPressHook(DWORD dxKey, bool pressed, DWORD vKey);
void _stdcall RunHookScriptsAtProc(DWORD procId);

}
