#pragma once

#include <Windows.h>

#include "..\HookScripts.h"
#include "..\ScriptExtender.h"

// Common variables and functions for hook script implementations

namespace sfall
{

// Number of types of hooks
constexpr int numHooks = HOOK_COUNT;

// Struct for registered hook script
struct HookScript {
	ScriptProgram prog;
	int callback; // proc number in script's proc table
	bool isGlobalScript; // false for hs_* scripts, true for gl* scripts
};

// All currently registered hook scripts
extern std::vector<HookScript> hooks[];

extern DWORD args[]; // current hook arguments
extern DWORD oldargs[];
extern DWORD* argPtr;
extern DWORD rets[]; // current hook return values

extern DWORD firstArg;
extern DWORD callDepth;
extern DWORD lastCount[];

extern DWORD argCount;
extern DWORD cArg; // how many arguments were taken by current hook script
extern DWORD cRet; // how many return values were set by current hook script
extern DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

void LoadHookScript(const char* name, int id);
void _stdcall BeginHook();
void _stdcall RunHookScript(DWORD hook);
void _stdcall EndHook();

#define HookBegin __asm pushad __asm call BeginHook __asm popad

#define hookbegin(a) __asm pushad __asm call BeginHook __asm popad __asm mov argCount, a
#define hookend __asm pushad __asm call EndHook __asm popad

}
