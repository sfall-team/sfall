#include "..\..\FalloutEngine\Fallout2.h"

#include "Common.h"

namespace sfall
{

constexpr int maxArgs  = 16;
constexpr int maxDepth = 8;

struct {
	DWORD hookID;
	DWORD argCount;
	DWORD cArg;
	DWORD cRet;
	DWORD cRetTmp;
	DWORD oldArgs[maxArgs];
	DWORD oldRets[maxRets];
} savedArgs[maxDepth];

static DWORD callDepth;
static DWORD currentRunHook = -1;

DWORD args[maxArgs]; // current hook arguments
DWORD rets[maxRets]; // current hook return values

DWORD argCount;
DWORD cArg;          // how many arguments were taken by current hook script
DWORD cRet;          // how many return values were set by current hook script
DWORD cRetTmp;       // how many return values were set by specific hook script (when using register_hook)

std::vector<HookScript> hooks[numHooks];

void LoadHookScript(const char* name, int id) {
	//if (id >= numHooks || IsGameScript(name)) return;
	char filePath[MAX_PATH];

	bool customPath = !HookScripts::hookScriptPathFmt.empty();
	if (!customPath) {
		sprintf(filePath, "scripts\\%s.int", name);
	} else {
		sprintf_s(filePath, HookScripts::hookScriptPathFmt.c_str(), name);
		name = filePath;
	}

	bool hookIsLoaded = LoadHookScriptFile(filePath, name, id, customPath);
	if (hookIsLoaded || (HookScripts::injectAllHooks && id != HOOK_SUBCOMBATDAMAGE)) {
		HookScripts::InjectingHook(id); // inject hook to engine code

		if (!hookIsLoaded) return;
		HookFile hookFile = { id, filePath, name };
		HookScripts::hookScriptFilesList.push_back(hookFile);
	}
}

bool LoadHookScriptFile(std::string filePath, const char* name, int id, bool fullPath) {
	ScriptProgram prog;
	if (fo::func::db_access(filePath.c_str())) {
		dlog(">" + filePath, DL_HOOK);
		LoadScriptProgram(prog, name, fullPath);
		if (prog.ptr) {
			dlogr(" Done", DL_HOOK);
			HookScript hook;
			hook.prog = prog;
			hook.callback = -1;
			hook.isGlobalScript = false;
			hooks[id].push_back(hook);
			AddProgramToMap(prog);
		} else {
			dlogr(" Error!", DL_HOOK);
		}
	}
	return (prog.ptr != nullptr);
}

// List of hooks that are denied recursive calls
static bool CheckRecursiveHooks(DWORD hook) {
	if (hook == currentRunHook) {
		switch (hook) {
		case HOOK_SETGLOBALVAR:
		case HOOK_SETLIGHTING:
			return true;
		}
	}
	return false;
}

void _stdcall BeginHook() {
	if (callDepth && callDepth <= maxDepth) {
		// save all values of the current hook, if during the execution of the current hook, another hook was called
		int cDepth = callDepth - 1;
		savedArgs[cDepth].hookID = currentRunHook;
		savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
		savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
		savedArgs[cDepth].cRet = cRet;                                             // number of returned arguments for the current hook
		savedArgs[cDepth].cRetTmp = cRetTmp;
		memcpy(&savedArgs[cDepth].oldArgs, args, argCount * sizeof(DWORD));        // values of the arguments
		if (cRet) memcpy(&savedArgs[cDepth].oldRets, rets, cRet * sizeof(DWORD));  // returned values

		// for debugging
		/*dlogh("\nSaved cArgs/cRet: %d / %d(%d)\n", savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
		for (unsigned int i = 0; i < maxArgs; i++) {
			dlogh("Saved Args/Rets: %d / %d\n", savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1), 0);
		}*/
	}
	callDepth++;
	#ifndef NDEBUG
		dlogh("Begin running hook, current depth: %d, current executable hook: %d\n", callDepth, currentRunHook, 0);
	#endif
}

static void _stdcall RunSpecificHookScript(HookScript *hook) {
	cArg = 0;
	cRetTmp = 0;
	if (hook->callback != -1) {
		fo::func::executeProcedure(hook->prog.ptr, hook->callback);
	} else {
		RunScriptProc(&hook->prog, fo::ScriptProc::start);
	}
}

void _stdcall RunHookScript(DWORD hook) {
	cRet = 0;
	if (hooks[hook].size()) {
		if (callDepth > 1) {
			if (CheckRecursiveHooks(hook) || callDepth > 8) {
				fo::func::debug_printf("\n[SFALL] The hook ID: %d cannot be executed.", hook);
				dlog_f("The hook %d cannot be executed due to exceeded depth limit or recursive calls\n", DL_MAIN, hook);
				return;
			}
		}
		currentRunHook = hook;
		if (isDebug) dlogh("Running hook %d, which has %0d entries attached, depth: %d\n", hook, hooks[hook].size(), callDepth);
		for (int i = hooks[hook].size() - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);

			// for debugging
			/*dlogh("> Hook: %d, script entry: %d done\n", hook, i, 0);
			dlogh("> Check cArg/cRet: %d / %d(%d)\n", cArg, cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlogh("> Check Args/Rets: %d / %d\n", args[i], ((i < maxRets) ? rets[i] : -1), 0);
			}*/
		}
	} else {
		cArg = 0; // for what purpose is it here?
		#ifndef NDEBUG
			dlogh(">>> Try running hook ID: %d\n", hook, 0, 0);
		#endif
	}
}

void _stdcall EndHook() {
	#ifndef NDEBUG
		dlogh("Ending running hook %d, current depth: %d\n", currentRunHook, callDepth, 0);
	#endif
	callDepth--;
	if (callDepth) {
		if (callDepth <= maxDepth) {
			// restored all saved values of the previous hook
			int cDepth = callDepth - 1;
			currentRunHook = savedArgs[cDepth].hookID;
			argCount = savedArgs[cDepth].argCount;
			cArg = savedArgs[cDepth].cArg;
			cRet = savedArgs[cDepth].cRet;
			cRetTmp = savedArgs[cDepth].cRetTmp;  // also restore current count of the number of returned arguments
			memcpy(args, &savedArgs[cDepth].oldArgs, argCount * sizeof(DWORD));
			if (cRet) memcpy(rets, &savedArgs[cDepth].oldRets, cRet * sizeof(DWORD));

			// for debugging
			/*dlogh("Restored cArgs/cRet: %d / %d(%d)\n", argCount, cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlogh("Restored Args/Rets: %d / %d\n", args[i], ((i < maxRets) ? rets[i] : -1), 0);
			}*/
		}
	} else {
		currentRunHook = -1;
	}
}

}
