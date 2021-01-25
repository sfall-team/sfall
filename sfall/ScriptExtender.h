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
enum SfallDataType : unsigned long {
	DATATYPE_NONE  = 0,
	DATATYPE_INT   = 1,
	DATATYPE_FLOAT = 2,
	DATATYPE_STR   = 3,
};

typedef struct {
	TProgram* ptr;
	int procLookup[Scripts::count];
	char initialized;
} sScriptProgram;

#pragma pack(push, 8)
struct sGlobalVar {
	__int64 id;
	__int32 val;
	__int32 unused;
};
#pragma pack(pop)

void ScriptExtender_Init();
void ScriptExtender_OnGameLoad();
void BuildSortedIndexList();
void LoadGlobalScripts();
bool __stdcall IsGameScript(const char* filename);

void RunGlobalScripts2();
void RunGlobalScripts3();
void __stdcall RunGlobalScriptsAtProc(DWORD procId);

bool LoadGlobals(HANDLE h);
void SaveGlobals(HANDLE h);

int GetNumGlobals();
void GetGlobals(sGlobalVar* globals);
void SetGlobals(sGlobalVar* globals);

long __stdcall SetGlobalVar(const char* var, int val);

long __stdcall GetGlobalVar(const char* var);

bool __stdcall ScriptHasLoaded(TProgram* script);

// loads script from .int file into a sScriptProgram struct, filling script pointer and proc lookup table
// prog - reference to program structure
// fileName - the script file name without extension
void LoadScriptProgram(sScriptProgram &prog, const char* fileName);

// init program after load, needs to be called once
void InitScriptProgram(sScriptProgram &prog);

// execute script by specific proc name
void RunScriptProc(sScriptProgram* prog, const char* procName);

// execute script proc by procId from define.h
void RunScriptProc(sScriptProgram* prog, long procId);

int RunScriptStartProc(sScriptProgram* prog);

long GetScriptReturnValue();
long GetResetScriptReturnValue();

void AddProgramToMap(sScriptProgram &prog);
sScriptProgram* GetGlobalScriptProgram(TProgram* scriptPtr);

void __stdcall AddTimerEventScripts(TProgram* script, long time, long param);
void __stdcall RemoveTimerEventScripts(TProgram* script, long param);
void __stdcall RemoveTimerEventScripts(TProgram* script);

// variables
extern DWORD isGlobalScriptLoading;
extern DWORD availableGlobalScriptTypes;
extern bool displayWinUpdateState;
