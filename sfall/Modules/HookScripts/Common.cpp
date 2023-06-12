#include "..\..\FalloutEngine\Fallout2.h"
#include "..\LoadGameHook.h"

#include "Common.h"

using namespace sfall::script;

namespace sfall
{

constexpr int maxArgs = 16; // Maximum number of hook arguments
constexpr int maxRets = 8;  // Maximum number of return values
constexpr int maxDepth = 8; // Maximum recursion depth for hook calls

struct {
	DWORD hookID;
	bool allowNonIntReturn;
	DWORD argCount;
	DWORD cArg;
	DWORD cRet;
	DWORD cRetTmp;
	DWORD args[maxArgs];
	DWORD rets[maxRets];
	DataType argTypes[maxArgs];
	DataType retTypes[maxRets];
} savedArgs[maxDepth];

static DWORD callDepth;
static DWORD currentRunHook = -1;

bool allowNonIntReturn;
DataType argTypes[maxArgs]; // current hook arguments types
DataType retTypes[maxRets]; // current hook return value types
DWORD args[maxArgs]; // current hook arguments
DWORD rets[maxRets]; // current hook return values

DWORD argCount;
DWORD cArg;    // how many arguments were taken by current hook script
DWORD cRet;    // how many return values were set by current hook script
DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

std::vector<HookScript> hooks[HOOK_COUNT];

DWORD HookCommon::GetHSArgCount() {
	return argCount;
}

ScriptValue HookCommon::GetHSArg() {
	return (cArg == argCount) ? 0 : GetHSArgAt(cArg++);
}

void HookCommon::SetHSArg(DWORD id, const ScriptValue& value, bool allowTypeChange) {
	if (id < argCount && (allowTypeChange || value.type() == argTypes[id])) {
		args[id] = value.rawValue();
		argTypes[id] = value.type();
	}
}

ScriptValue HookCommon::GetHSArgAt(DWORD id) {
	return ScriptValue(argTypes[id], args[id]);
}

void __stdcall HookCommon::SetHSReturn(const ScriptValue& value) {
	// For backward compatibility - ignore non-int return values
	if (!allowNonIntReturn && !value.isInt()) return;

	if (cRetTmp < maxRets) {
		retTypes[cRetTmp] = value.type();
		rets[cRetTmp++] = value.rawValue();
	}
	if (cRetTmp > cRet) {
		cRet = cRetTmp;
	}
}

// List of hooks that are not allowed to be called recursively
static bool CheckRecursiveHooks(DWORD hook) {
	if (hook == currentRunHook) {
		switch (hook) {
		case HOOK_SETGLOBALVAR:
		case HOOK_SETLIGHTING:
			return true;
		default:
			if (isDebug) fo::func::debug_printf("\nWARNING: A recursive hook with ID %d was running.", hook);
		}
	}
	return false;
}

void __stdcall BeginHook() {
	if (callDepth && callDepth <= maxDepth) {
		// save all values of the current hook if another hook was called during the execution of the current hook
		int cDepth = callDepth - 1;
		savedArgs[cDepth].hookID = currentRunHook;
		savedArgs[cDepth].allowNonIntReturn = allowNonIntReturn;
		savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
		savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
		savedArgs[cDepth].cRet = cRet;                                             // number of return values for the current hook
		savedArgs[cDepth].cRetTmp = cRetTmp;
		std::memcpy(&savedArgs[cDepth].args, args, maxArgs * sizeof(DWORD));            // values of the arguments
		std::memcpy(&savedArgs[cDepth].argTypes, argTypes, maxArgs * sizeof(DataType)); // types of the arguments
		if (cRet) {
			std::memcpy(&savedArgs[cDepth].rets, rets, maxRets * sizeof(DWORD));            // return values
			std::memcpy(&savedArgs[cDepth].retTypes, retTypes, maxRets * sizeof(DataType)); // return value types
		}

		//devlog_f("\nSaved cArgs/cRet: %d / %d(%d)\n", DL_HOOK, savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
		//for (unsigned int i = 0; i < maxArgs; i++) {
		//	devlog_f("Saved Args/Rets: %d / %d\n", DL_HOOK, savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1));
		//}
	}
	callDepth++;
	allowNonIntReturn = false;
	// Init all arg types as ints by default
	for (size_t i = 0; i < maxArgs; i++) {
		argTypes[i] = DataType::INT;
	}

	devlog_f("Begin running hook, current depth: %d, current executable hook: %d\n", DL_HOOK, callDepth, currentRunHook);
}

static void __stdcall RunSpecificHookScript(HookScript *hook) {
	cArg = 0;
	cRetTmp = 0;
	if (hook->callback != -1) {
		fo::func::executeProcedure(hook->prog.ptr, hook->callback);
	} else {
		hook->callback = RunScriptStartProc(&hook->prog); // run start
	}
}

void __stdcall RunHookScript(DWORD hook) {
	cRet = 0;
	if (!hooks[hook].empty()) {
		if (callDepth > 1) {
			if (CheckRecursiveHooks(hook) || callDepth > 8) {
				fo::func::debug_printf("\n[SFALL] The hook ID %d cannot be executed.", hook);
				dlog_f("The hook ID %d cannot be executed due to exceeding depth limit or disallowed recursive calls\n", DL_MAIN, hook);
				return;
			}
		}
		currentRunHook = hook;
		size_t hooksCount = hooks[hook].size();
		dlog_f("Running hook ID %d, which has %0d entries attached, depth: %d\n", DL_HOOK, hook, hooksCount, callDepth);
		for (int i = hooksCount - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);

			//devlog_f("> Hook: %d, script entry: %d done\n", DL_HOOK, hook, i);
			//devlog_f("> Check cArg/cRet: %d / %d(%d)\n", DL_HOOK, cArg, cRet, cRetTmp);
			//for (unsigned int i = 0; i < maxArgs; i++) {
			//	devlog_f("> Check Args/Rets: %d / %d\n", DL_HOOK, args[i], ((i < maxRets) ? rets[i] : -1));
			//}
		}
	} else {
		cArg = 0;

		devlog_f(">>> Try running hook ID: %d\n", DL_HOOK, hook);
	}
}

void __stdcall EndHook() {
	devlog_f("End running hook ID %d, current depth: %d\n", DL_HOOK, currentRunHook, callDepth);

	callDepth--;
	if (callDepth) {
		if (callDepth <= maxDepth) {
			// restore all saved values of the previous hook
			int cDepth = callDepth - 1;
			currentRunHook = savedArgs[cDepth].hookID;
			allowNonIntReturn = savedArgs[cDepth].allowNonIntReturn;
			argCount = savedArgs[cDepth].argCount;
			cArg = savedArgs[cDepth].cArg;
			cRet = savedArgs[cDepth].cRet;
			cRetTmp = savedArgs[cDepth].cRetTmp;  // also restore current count of the number of return values
			std::memcpy(args, &savedArgs[cDepth].args, maxArgs * sizeof(DWORD));
			std::memcpy(argTypes, &savedArgs[cDepth].argTypes, maxArgs * sizeof(DataType));
			if (cRet > 0) {
				std::memcpy(rets, &savedArgs[cDepth].rets, maxRets * sizeof(DWORD));
				std::memcpy(retTypes, &savedArgs[cDepth].retTypes, maxRets * sizeof(DataType));
			}

			//devlog_f("Restored cArgs/cRet: %d / %d(%d)\n", DL_HOOK, argCount, cRet, cRetTmp);
			//for (unsigned int i = 0; i < maxArgs; i++) {
			//	devlog_f("Restored Args/Rets: %d / %d\n", DL_HOOK, args[i], ((i < maxRets) ? rets[i] : -1));
			//}
		}
	} else {
		currentRunHook = -1;
	}
}

// BEGIN HOOKS
void __stdcall HookCommon::KeyPressHook(DWORD* dxKey, bool pressed, DWORD vKey) {
	if (!IsGameLoaded() || !HookScripts::HookHasScript(HOOK_KEYPRESS)) {
		return;
	}
	BeginHook();
	argCount = 3;
	args[0] = (DWORD)pressed;
	args[1] = *dxKey;
	args[2] = vKey;
	RunHookScript(HOOK_KEYPRESS);
	if (cRet != 0) {
		long retKey = rets[0];
		if (retKey > 0 && retKey < 264) *dxKey = retKey;
	}
	EndHook();
}

void __stdcall HookCommon::MouseClickHook(DWORD button, bool pressed) {
	if (!IsGameLoaded() || !HookScripts::HookHasScript(HOOK_MOUSECLICK)) {
		return;
	}
	BeginHook();
	argCount = 2;
	args[0] = (DWORD)pressed;
	args[1] = button;
	RunHookScript(HOOK_MOUSECLICK);
	EndHook();
}

static unsigned long previousGameMode = 0;

void HookCommon::GameModeChangeHook(DWORD exit) {
	if (HookScripts::HookHasScript(HOOK_GAMEMODECHANGE)) {
		BeginHook();
		argCount = 2;
		args[0] = exit;
		args[1] = previousGameMode;
		RunHookScript(HOOK_GAMEMODECHANGE);
		EndHook();
	}
	previousGameMode = GetLoopFlags();
}
// END HOOKS

void HookCommon::Reset() {
	previousGameMode = 0;
}

}
