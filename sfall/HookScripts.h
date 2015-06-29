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