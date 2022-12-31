/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

namespace sfall
{

typedef struct {
	fo::Program* ptr;
	int procLookup[fo::Scripts::ScriptProc::count];
	bool initialized;
} ScriptProgram;

class ScriptExtender{
public:
	static const char* name() { return "ScriptExtender"; }
	static void init();

	static void OnGameLoad();

	static bool OnMapLeave;

	static char gTextBuffer[5120];

	// returns the size of the global text buffer
	inline static const long TextBufferSize() { return sizeof(gTextBuffer); }

	static long GetScriptReturnValue();
	static long GetResetScriptReturnValue();

	static void AddProgramToMap(ScriptProgram &prog);
	static ScriptProgram* GetGlobalScriptProgram(fo::Program* scriptPtr);

	static void AddTimerEventScripts(fo::Program* script, long time, long param);
	static void RemoveTimerEventScripts(fo::Program* script, long param);
	static void RemoveTimerEventScripts(fo::Program* script);

	static int __stdcall ScriptHasLoaded(fo::Program* script);

	// loads and initializes script file (for normal game scripts)
	static long __fastcall InitScript(long sid);
};

class ObjectName {
public:
	static const char* __stdcall GetName(fo::GameObject* object);
	static void SetName(long sid, const char* name);

	static void Reset();
};

#pragma pack(push, 8)
struct GlobalVar {
	__int64 id;
	__int32 val;
	__int32 unused;
};
#pragma pack(pop)

void __fastcall SetGlobalScriptRepeat(fo::Program* script, int frames);
void __fastcall SetGlobalScriptType(fo::Program* script, int type);

void BuildSortedIndexList();
void LoadGlobalScripts();
void InitGlobalScripts();
bool IsGameScript(const char* filename);

void RunGlobalScriptsOnMainLoop();
void RunGlobalScriptsOnInput();
void RunGlobalScriptsOnWorldMap();
void RunGlobalScriptsAtProc(DWORD procId);

bool LoadGlobals(HANDLE h);
void SaveGlobals(HANDLE h);

int GetNumGlobals();
void GetGlobals(GlobalVar* globals);
void SetGlobals(GlobalVar* globals);

long SetGlobalVar(const char* var, int val);
void SetGlobalVarInt(DWORD var, int val);

long GetGlobalVar(const char* var);
long GetGlobalVarInt(DWORD var);
long GetGlobalVarInternal(__int64 val);

void __fastcall SetSelfObject(fo::Program* script, fo::GameObject* obj);

// loads script from .int file into a ScriptProgram struct, filling script pointer and proc lookup table
// prog - reference to program structure
// fileName - the script file name without extension
void InitScriptProgram(ScriptProgram &prog, const char* fileName);

// init program after load, needs to be called once
void RunScriptProgram(ScriptProgram &prog);

// execute script by specific proc name
void RunScriptProc(ScriptProgram* prog, const char* procName);

// execute script proc by procId from define.h
void RunScriptProc(ScriptProgram* prog, long procId);

int RunScriptStartProc(ScriptProgram* prog);

// variables
extern DWORD availableGlobalScriptTypes;
extern bool alwaysFindScripts;

}
