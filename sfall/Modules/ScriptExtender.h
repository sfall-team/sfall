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

#pragma once

#include "..\main.h"
#include "..\FalloutEngine\Structs.h"

struct sGlobalVar {
	__int64 id;
	int val;
};

#define SCRIPT_PROC_MAX (27)
typedef struct {
	TProgram* ptr;
	int procLookup[SCRIPT_PROC_MAX+1];
	char initialized;
} sScriptProgram;

void ScriptExtenderSetup();
bool _stdcall IsGameScript(const char* filename);
void LoadGlobalScripts();
void ClearGlobalScripts();

void RunGlobalScripts1();
void RunGlobalScripts2();
void RunGlobalScripts3();
void _stdcall RunGlobalScriptsAtProc(DWORD procId);
void AfterAttackCleanup();

void LoadGlobals(HANDLE h);
void SaveGlobals(HANDLE h);
void ClearGlobals();

int GetNumGlobals();
void GetGlobals(sGlobalVar* globals);
void SetGlobals(sGlobalVar* globals);

extern DWORD AddUnarmedStatToGetYear;
extern DWORD AvailableGlobalScriptTypes;

void SetAppearanceGlobals(int race, int style);
void GetAppearanceGlobals(int *race, int *style);

void _stdcall RegAnimCombatCheck(DWORD newValue);

bool _stdcall ScriptHasLoaded(TProgram* script);
// loads script from .int file into scripting engine, fill scriptPtr and proc table
void LoadScriptProgram(sScriptProgram &prog, const char* fileName);
// init program after load, needs to be called once
void InitScriptProgram(sScriptProgram &prog);
// execute script by specific proc name
void RunScriptProc(sScriptProgram* prog, const char* procName);
// execute script proc by procId from define.h
void RunScriptProc(sScriptProgram* prog, int procId);

void AddProgramToMap(sScriptProgram &prog);
sScriptProgram* GetGlobalScriptProgram(TProgram* scriptPtr);

char* _stdcall mysubstr(char* str, int pos, int length);
// variables
static char reg_anim_combat_check = 1;
extern DWORD isGlobalScriptLoading;

// types for script variables
#define VAR_TYPE_INT    (0xC001)
#define VAR_TYPE_FLOAT  (0xA001)
#define VAR_TYPE_STR    (0x9801)
#define VAR_TYPE_STR2   (0x9001)

// script procs
#define start               (1)
#define map_enter_p_proc    (15)
#define destroy_p_proc      (18)
#define map_update_p_proc   (23)
