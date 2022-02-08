#pragma once

#include "..\HookScripts.h"
#include "..\ScriptExtender.h"

// Common variables and functions for hook script implementations

namespace sfall
{

class HookCommon {
public:
	static DWORD GetHSArgCount();
	static DWORD GetHSArg();
	static DWORD GetHSArgAt(DWORD id);
	static DWORD* GetHSArgs();
	static void SetHSArg(DWORD id, DWORD value);
	static void __stdcall SetHSReturn(DWORD d);

	static void GameModeChangeHook(DWORD exit);
	static void __stdcall KeyPressHook(DWORD* dxKey, bool pressed, DWORD vKey);
	static void __stdcall MouseClickHook(DWORD button, bool pressed);

	static void Reset();
};

// Struct for registered hook script
struct HookScript {
	ScriptProgram prog;
	int callback;        // procedure position in script's proc table
	bool isGlobalScript; // false for hs_* scripts, true for gl* scripts
};

// All currently registered hook scripts
extern std::vector<HookScript> hooks[];

extern DWORD args[];  // current hook arguments
extern DWORD rets[];  // current hook return values

extern DWORD argCount;
extern DWORD cArg;    // how many arguments were taken by current hook script
extern DWORD cRet;    // how many return values were set by current hook script
extern DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

void __stdcall BeginHook();
void __stdcall RunHookScript(DWORD hook);
void __stdcall EndHook();

#define HookBegin pushadc __asm call BeginHook popadc
#define HookEnd pushadc __asm call EndHook popadc

}
