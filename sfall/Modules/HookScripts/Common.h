#pragma once

#include "..\HookScripts.h"
#include "..\ScriptExtender.h"
#include "..\Scripting\ScriptValue.h"

// Common variables and functions for hook script implementations

namespace sfall
{

class HookCommon {
public:
	static DWORD GetHSArgCount();
	static script::ScriptValue GetHSArg();
	static script::ScriptValue GetHSArgAt(DWORD id);
	static void SetHSArg(DWORD id, const script::ScriptValue& value);
	static void __stdcall SetHSReturn(const script::ScriptValue& value);

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

extern bool allowNonIntReturn; // allow set_sfall_return with non-int values (validate value in the hook code)
extern script::DataType argTypes[]; // current hook arguments types
extern script::DataType retTypes[]; // current hook return value types
extern DWORD args[];  // current hook arguments
extern DWORD rets[];  // current hook return values

extern DWORD argCount;
extern DWORD cRet;    // how many return values were set by current hook script
extern DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

void __stdcall BeginHook();
void __stdcall RunHookScript(DWORD hook);
void __stdcall EndHook();

#define HookBegin pushadc __asm call BeginHook popadc
#define HookEnd pushadc __asm call EndHook popadc

}
