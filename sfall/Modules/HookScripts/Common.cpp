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

bool LoadHookScript(const char* name, int id) {
	if (id >= numHooks || IsGameScript(name)) return false;

	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);
	ScriptProgram prog;
	if (fo::func::db_access(filename)) {
		dlog(">", DL_HOOK);
		dlog(name, DL_HOOK);
		LoadScriptProgram(prog, name);
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
	bool hookIsLoaded = (prog.ptr != nullptr);
	if (hookIsLoaded || HookScripts::injectAllHooks) HookScripts::InjectingHook(id); // inject hook to engine code
	return hookIsLoaded;
}

void _stdcall BeginHook() {
	if (callDepth && callDepth <= maxDepth) {
		// save all values of the current hook if another hook was called during the execution of the current hook
		int cDepth = callDepth - 1;
		savedArgs[cDepth].hookID = currentRunHook;
		savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
		savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
		savedArgs[cDepth].cRet = cRet;                                             // number of return values for the current hook
		memcpy(&savedArgs[cDepth].oldArgs, args, argCount * sizeof(DWORD));        // values of the arguments
		if (cRet) memcpy(&savedArgs[cDepth].oldRets, rets, cRet * sizeof(DWORD));  // return values

		// for debugging
		/*dlog_f("Saved cArgs/cRet: %d / %d(%d)\n", DL_HOOK, savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
		for (unsigned int i = 0; i < maxArgs; i++) {
			dlog_f("Saved Args/Rets: %d / %d\n", DL_HOOK, savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1));
		}*/
	}
	callDepth++;
	#ifndef NDEBUG
		dlog_f("Begin running hook, current depth: %d, current executable hook: %d\n", DL_HOOK, callDepth, currentRunHook);
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
			if (hook == currentRunHook || callDepth > 8) {
				fo::func::debug_printf("\n[SFALL] The hook ID: %d cannot be executed.", hook);
				dlog_f("The hook %d cannot be executed due to exceeded depth limit or recursive calls\n", DL_MAIN, hook);
				return;
			}
		}
		currentRunHook = hook;
		dlog_f("Running hook %d, which has %0d entries attached, depth: %d\n", DL_HOOK, hook, hooks[hook].size(), callDepth);
		for (int i = hooks[hook].size() - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);
		}
	} else {
		cArg = 0;
	}
}

void _stdcall EndHook() {
	#ifndef NDEBUG
		dlog_f("End running hook %d, current depth: %d\n", DL_HOOK, currentRunHook, callDepth);
	#endif
	callDepth--;
	if (callDepth && callDepth <= maxDepth) {
		// restore all saved values of the previous hook
		int cDepth = callDepth - 1;
		currentRunHook = savedArgs[cDepth].hookID;
		argCount = savedArgs[cDepth].argCount;
		cArg = savedArgs[cDepth].cArg;
		cRet = cRetTmp = savedArgs[cDepth].cRet;  // also restore current count of the number of return values
		memcpy(args, &savedArgs[cDepth].oldArgs, argCount * sizeof(DWORD));
		if (cRet) memcpy(rets, &savedArgs[cDepth].oldRets, cRet * sizeof(DWORD));

		// for debugging
		/*dlog_f("Restored cArgs/cRets: %d / %d(%d)\n", DL_HOOK, argCount, cRet, cRetTmp);
		for (unsigned int i = 0; i < maxArgs; i++) {
			dlog_f("Restored Args/Rets: %d / %d\n", args[i], ((i < maxRets) ? rets[i] : -1));
		}*/
	}
}

}
