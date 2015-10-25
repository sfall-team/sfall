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

#define HOOK_TOHIT           (0)
#define HOOK_AFTERHITROLL    (1)
#define HOOK_CALCAPCOST      (2)
#define HOOK_DEATHANIM1      (3)
#define HOOK_DEATHANIM2      (4)
#define HOOK_COMBATDAMAGE    (5)
#define HOOK_ONDEATH         (6)
#define HOOK_FINDTARGET      (7)
#define HOOK_USEOBJON        (8)
#define HOOK_REMOVEINVENOBJ  (9)
#define HOOK_BARTERPRICE     (10)
#define HOOK_MOVECOST        (11)
#define HOOK_HEXMOVEBLOCKING (12)
#define HOOK_HEXAIBLOCKING   (13)
#define HOOK_HEXSHOOTBLOCKING (14)
#define HOOK_HEXSIGHTBLOCKING (15)
#define HOOK_ITEMDAMAGE      (16)
#define HOOK_AMMOCOST        (17)
#define HOOK_USEOBJ          (18)
#define HOOK_KEYPRESS        (19)
#define HOOK_MOUSECLICK      (20)
#define HOOK_USESKILL        (21)
#define HOOK_STEAL           (22)
#define HOOK_WITHINPERCEPTION (23)
#define HOOK_INVENTORYMOVE    (24)

DWORD _stdcall GetHSArgCount();
DWORD _stdcall GetHSArg();
DWORD* _stdcall GetHSArgs();
void _stdcall SetHSArg(DWORD id, DWORD value);
void _stdcall SetHSReturn(DWORD d);
// register hook by proc num (special values: -1 - use default (start) procedure, 0 - unregister)
void _stdcall RegisterHook(DWORD script, DWORD id, DWORD procNum);

void HookScriptInit();
void HookScriptClear();

extern DWORD InitingHookScripts;
extern void __declspec() AmmoCostHookWrapper();
void _stdcall MouseClickHook(DWORD button, bool pressed);
void _stdcall KeyPressHook(DWORD dxKey, bool pressed, DWORD vKey);
void _stdcall RunHookScriptsAtProc(DWORD procId);