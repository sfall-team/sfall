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

#include "Module.h"

namespace sfall
{

class ScriptExtender : public Module {
public:
	const char* name() { return "ScriptExtender"; }
	void init();
};

#pragma pack(8)
struct GlobalVar {
	__int64 id;
	__int32 val;
	__int32 unused;
};

typedef struct {
	fo::Program* ptr = nullptr;
	int procLookup[fo::ScriptProc::count];
	char initialized;
} ScriptProgram;

void _stdcall SetGlobalScriptRepeat(fo::Program* script, int frames);
void _stdcall SetGlobalScriptType(fo::Program* script, int type);
bool _stdcall IsGameScript(const char* filename);

void _stdcall RunGlobalScriptsAtProc(DWORD procId);

void LoadGlobals(HANDLE h);
void SaveGlobals(HANDLE h);

int GetNumGlobals();
void GetGlobals(GlobalVar* globals);
void SetGlobals(GlobalVar* globals);

void _stdcall SetGlobalVar(const char* var, int val);
void _stdcall SetGlobalVarInt(DWORD var, int val);
DWORD _stdcall GetGlobalVar(const char* var);
DWORD _stdcall GetGlobalVarInt(DWORD var);

void _stdcall SetSelfObject(fo::Program* script, fo::GameObject* obj);

void SetAppearanceGlobals(int race, int style);
void GetAppearanceGlobals(int *race, int *style);

void _stdcall RegAnimCombatCheck(DWORD newValue);

bool _stdcall ScriptHasLoaded(fo::Program* script);
// loads script from .int file into a sScriptProgram struct, filling script pointer and proc lookup table
// prog - reference to program structure
// fileName - the script file name without extension (if fullPath is false) or a full file path (if fullPath is true)
// fullPath - controls how fileName is used (see above)
void LoadScriptProgram(ScriptProgram &prog, const char* fileName, bool fullPath = false);
// init program after load, needs to be called once
void InitScriptProgram(ScriptProgram &prog);
// execute script by specific proc name
void RunScriptProc(ScriptProgram* prog, const char* procName);
// execute script proc by procId from define.h
void RunScriptProc(ScriptProgram* prog, long procId);

void AddProgramToMap(ScriptProgram &prog);
ScriptProgram* GetGlobalScriptProgram(fo::Program* scriptPtr);

// variables
static char regAnimCombatCheck = 1;
extern DWORD isGlobalScriptLoading;
extern DWORD availableGlobalScriptTypes;

}
