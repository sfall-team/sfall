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

#include "main.h"
#include "FalloutStructs.h"

// TODO: replace with enum class
enum SfallDataType {
	DATATYPE_NONE = 0,
	DATATYPE_INT,
	DATATYPE_FLOAT,
	DATATYPE_STR
};

#define SCRIPT_PROC_MAX (27)
typedef struct {
	DWORD ptr;
	DWORD procLookup[SCRIPT_PROC_MAX + 1];
	char initialized;
} sScriptProgram;

#pragma pack(push, 8)
struct sGlobalVar {
	__int64 id;
	__int32 val;
	__int32 unused;
};
#pragma pack(pop)

void ScriptExtenderInit();
bool _stdcall IsGameScript(const char* filename);
void LoadGlobalScripts();
void ClearGlobalScripts();

void RunGlobalScripts2();
void RunGlobalScripts3();
void _stdcall RunGlobalScriptsAtProc(DWORD procId);
void AfterAttackCleanup();

bool LoadGlobals(HANDLE h);
void SaveGlobals(HANDLE h);
void ClearGlobals();

int GetNumGlobals();
void GetGlobals(sGlobalVar* globals);
void SetGlobals(sGlobalVar* globals);

void SetAppearanceGlobals(int race, int style);
void GetAppearanceGlobals(int *race, int *style);

void _stdcall RegAnimCombatCheck(DWORD newValue);
DWORD _stdcall ForceEncounterRestore();

bool _stdcall ScriptHasLoaded(DWORD script);
// finds procedure ID for given script program pointer and procedure name
DWORD GetScriptProcByName(DWORD scriptPtr, const char* procName);
// loads script from .int file into scripting engine, fill scriptPtr and proc table
void LoadScriptProgram(sScriptProgram &prog, const char* fileName);
// init program after load, needs to be called once
void InitScriptProgram(sScriptProgram &prog);
// execute script proc by internal proc number (from script's proc table, basically a sequential number of a procedure as defined in code, starting from 1)
void RunScriptProcByNum(DWORD sptr, DWORD procNum);
// execute script by specific proc name
void RunScriptProc(sScriptProgram* prog, const char* procName);
// execute script proc by procId from define.h
void RunScriptProc(sScriptProgram* prog, DWORD procId);

int RunScriptStartProc(sScriptProgram* prog);

long __stdcall GetScriptReturnValue();
long __stdcall GetResetScriptReturnValue();

void AddProgramToMap(sScriptProgram &prog);
sScriptProgram* GetGlobalScriptProgram(DWORD scriptPtr);

void _stdcall AddTimerEventScripts(DWORD script, long time, long param);
void _stdcall RemoveTimerEventScripts(DWORD script, long param);
void _stdcall RemoveTimerEventScripts(DWORD script);

// variables
static char reg_anim_combat_check = 1;
extern DWORD isGlobalScriptLoading;
extern DWORD AvailableGlobalScriptTypes;

// Script data types
#define VAR_TYPE_INT    (0xC001)
#define VAR_TYPE_FLOAT  (0xA001)
#define VAR_TYPE_STR    (0x9801)
#define VAR_TYPE_STR2   (0x9001)

// Script procedure types
enum ScriptProc : long
{
	no_p_proc = 0,
	start = 1,
	spatial_p_proc = 2,
	description_p_proc = 3,
	pickup_p_proc = 4,
	drop_p_proc = 5,
	use_p_proc = 6,
	use_obj_on_p_proc = 7,
	use_skill_on_p_proc = 8,
	none_x_bad = 9,
	none_x_bad2 = 10,
	talk_p_proc = 11,
	critter_p_proc = 12,
	combat_p_proc = 13,
	damage_p_proc = 14,
	map_enter_p_proc = 15,
	map_exit_p_proc = 16,
	create_p_proc = 17,
	destroy_p_proc = 18,
	none_x_bad3 = 19,
	none_x_bad4 = 20,
	look_at_p_proc = 21,
	timed_event_p_proc = 22,
	map_update_p_proc = 23,
	push_p_proc = 24,
	is_dropping_p_proc = 25,
	combat_is_starting_p_proc = 26,
	combat_is_over_p_proc = 27
};

enum ScriptTypes : long
{
	SCRIPT_SYSTEM = 0,
	SCRIPT_SPATIAL = 1,
	SCRIPT_TIME = 2,
	SCRIPT_ITEM = 3,
	SCRIPT_CRITTER = 4,
};
