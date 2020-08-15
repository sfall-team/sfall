#include "..\..\FalloutEngine\Fallout2.h"

#include "Common.h"

namespace sfall
{

constexpr int maxArgs = 16;
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
DWORD cArg;    // how many arguments were taken by current hook script
DWORD cRet;    // how many return values were set by current hook script
DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

std::vector<HookScript> hooks[numHooks];

void LoadHookScript(const char* name, int id) {
	//if (id >= numHooks || IsGameScript(name)) return;

	bool hookIsLoaded = LoadHookScriptFile(name, id);
	if (hookIsLoaded || (HookScripts::injectAllHooks && id != HOOK_SUBCOMBATDAMAGE)) {
		HookScripts::InjectingHook(id); // inject hook to engine code

		if (!hookIsLoaded) return;
		HookFile hookFile = { id, name };
		HookScripts::hookScriptFilesList.push_back(hookFile);
	}
}

bool LoadHookScriptFile(const char* name, int id) {
	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);

	ScriptProgram prog;
	if (fo::func::db_access(filename)) {
		dlog("> ", DL_HOOK);
		dlog(name, DL_HOOK);
		LoadScriptProgram(prog, name);
		if (prog.ptr) {
			dlogr(" Done", DL_HOOK);
			HookScript hook;
			hook.prog = prog;
			hook.callback = -1;
			hook.isGlobalScript = false;
			hooks[id].push_back(hook);
			ScriptExtender::AddProgramToMap(prog);
		} else {
			dlogr(" Error!", DL_HOOK);
		}
	}
	return (prog.ptr != nullptr);
}

// List of hooks that are not allowed to be called recursively
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

void __stdcall BeginHook() {
	if (callDepth && callDepth <= maxDepth) {
		// save all values of the current hook if another hook was called during the execution of the current hook
		int cDepth = callDepth - 1;
		savedArgs[cDepth].hookID = currentRunHook;
		savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
		savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
		savedArgs[cDepth].cRet = cRet;                                             // number of return values for the current hook
		savedArgs[cDepth].cRetTmp = cRetTmp;
		memcpy(&savedArgs[cDepth].oldArgs, args, argCount * sizeof(DWORD));        // values of the arguments
		if (cRet) memcpy(&savedArgs[cDepth].oldRets, rets, cRet * sizeof(DWORD));  // return values

		// for debugging
		/*dlog_f("\nSaved cArgs/cRets: %d / %d(%d)\n", DL_HOOK, savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
		for (unsigned int i = 0; i < maxArgs; i++) {
			dlog_f("Saved Args/Rets: %d / %d\n", DL_HOOK, savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1));
		}*/
	}
	callDepth++;
	#ifndef NDEBUG
		dlog_f("Begin running hook, current depth: %d, current executable hook: %d\n", DL_HOOK, callDepth, currentRunHook);
	#endif
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
				fo::func::debug_printf("\n[SFALL] The hook ID: %d cannot be executed.", hook);
				dlog_f("The hook %d cannot be executed due to exceeded depth limit or recursive calls\n", DL_MAIN, hook);
				return;
			}
		}
		currentRunHook = hook;
		size_t hooksCount = hooks[hook].size();
		dlog_f("Running hook %d, which has %0d entries attached, depth: %d\n", DL_HOOK, hook, hooksCount, callDepth);
		for (int i = hooksCount - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);

			// for debugging
			/*dlog_f("> Hook: %d, script entry: %d done\n", DL_HOOK, hook, i);
			dlog_f("> Check cArg/cRet: %d / %d(%d)\n", DL_HOOK, cArg, cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlog_f("> Check Args/Rets: %d / %d\n", DL_HOOK, args[i], ((i < maxRets) ? rets[i] : -1));
			}*/
		}
	} else {
		cArg = 0;
		#ifndef NDEBUG
			dlog_f(">>> Try running hook ID: %d\n", DL_HOOK, hook);
		#endif
	}
}

void __stdcall EndHook() {
	#ifndef NDEBUG
		dlog_f("End running hook %d, current depth: %d\n", DL_HOOK, currentRunHook, callDepth);
	#endif
	callDepth--;
	if (callDepth) {
		if (callDepth <= maxDepth) {
			// restore all saved values of the previous hook
			int cDepth = callDepth - 1;
			currentRunHook = savedArgs[cDepth].hookID;
			argCount = savedArgs[cDepth].argCount;
			cArg = savedArgs[cDepth].cArg;
			cRet = savedArgs[cDepth].cRet;
			cRetTmp = savedArgs[cDepth].cRetTmp;  // also restore current count of the number of return values
			memcpy(args, &savedArgs[cDepth].oldArgs, argCount * sizeof(DWORD));
			if (cRet) memcpy(rets, &savedArgs[cDepth].oldRets, cRet * sizeof(DWORD));

			// for debugging
			/*dlog_f("Restored cArgs/cRets: %d / %d(%d)\n", DL_HOOK, argCount, cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlog_f("Restored Args/Rets: %d / %d\n", args[i], ((i < maxRets) ? rets[i] : -1));
			}*/
		}
	} else {
		currentRunHook = -1;
	}
}

}
