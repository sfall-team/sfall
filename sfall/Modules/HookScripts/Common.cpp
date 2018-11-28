#include "..\..\FalloutEngine\Fallout2.h"

#include "Common.h"

namespace sfall
{

constexpr int maxArgs  = 16;
constexpr int maxDepth = 8;

struct {
	DWORD argCount;
	DWORD cArg;
	DWORD cRet;
	DWORD oldArgs[maxArgs];
	DWORD oldRets[maxRets];
} savedArgs[maxDepth];

static DWORD callDepth;

DWORD args[maxArgs]; // current hook arguments
DWORD rets[maxRets]; // current hook return values

DWORD argCount;
DWORD cArg;          // how many arguments were taken by current hook script
DWORD cRet;          // how many return values were set by current hook script
DWORD cRetTmp;       // how many return values were set by specific hook script (when using register_hook)

std::vector<HookScript> hooks[numHooks];

bool LoadHookScript(const char* name, int id) {
	if (id >= numHooks || IsGameScript(name)) return false;

	bool usePath = false;
	char filename[MAX_PATH];
	if (HookScripts::hookScriptPathFmt.empty()) {
		sprintf(filename, "scripts\\%s.int", name);
	} else {
		sprintf_s(filename, HookScripts::hookScriptPathFmt.c_str(), name);
		name = filename;
		usePath = true;
	}

	ScriptProgram prog;
	if (fo::func::db_access(filename)) {
		dlog(">", DL_HOOK);
		dlog(filename, DL_HOOK);
		LoadScriptProgram(prog, name, usePath);
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
	if (callDepth <= maxDepth) {
		// save all values of the current hook, if during the execution of the current hook, another hook was called
		if (callDepth) {
			int cDepth = callDepth - 1;
			savedArgs[cDepth].argCount = argCount;                                     // number of arguments of the current hook
			savedArgs[cDepth].cArg = cArg;                                             // current count of taken arguments
			savedArgs[cDepth].cRet = cRet;                                             // number of returned arguments for the current hook
			memcpy(&savedArgs[cDepth].oldArgs, args, argCount * sizeof(DWORD));        // values of the arguments
			if (cRet) memcpy(&savedArgs[cDepth].oldRets, rets, cRet * sizeof(DWORD));  // returned values

			// for debuging
			/*dlog_f("Saved cArgs/cRet: %d / %d(%d)\n", DL_HOOK, savedArgs[cDepth].argCount, savedArgs[cDepth].cRet, cRetTmp);
			for (unsigned int i = 0; i < maxArgs; i++) {
				dlogh("Saved Args/Rets: %d / %d\n", savedArgs[cDepth].oldArgs[i], ((i < maxRets) ? savedArgs[cDepth].oldRets[i] : -1));
			}*/
		}
	} else {
		MessageBoxA(0, "Exceeded depth limit for hook execution!", "sfall: Hooks depth", MB_TASKMODAL);
	}
	callDepth++;
	#ifndef NDEBUG
		dlogh("Begin running hook, current depth: %d\n", callDepth, 0);
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
		if (isDebug) dlogh("Running hook %d, which has %0d entries attached\n", hook, hooks[hook].size());
		for (int i = hooks[hook].size() - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);
		}
	} else {
		cArg = 0; // for what purpose is it here?
	}
}

void _stdcall EndHook() {
	#ifndef NDEBUG
		dlogh("Ending running hook, current depth: %d\n", callDepth, 0);
	#endif
	callDepth--;
	if (callDepth && callDepth <= maxDepth) {
		// restored all saved values of the previous hook
		int cDepth = callDepth - 1;
		argCount = savedArgs[cDepth].argCount;
		cArg = savedArgs[cDepth].cArg;
		cRet = cRetTmp = savedArgs[cDepth].cRet;  // also restore current count of the number of returned arguments
		memcpy(args, &savedArgs[cDepth].oldArgs, argCount * sizeof(DWORD));
		if (cRet) memcpy(rets, &savedArgs[cDepth].oldRets, cRet * sizeof(DWORD));

		// for debuging
		/*dlog_f("Restored cArgs/cRet: %d / %d(%d)\n", DL_HOOK, argCount, cRet, cRetTmp);
		for (unsigned int i = 0; i < maxArgs; i++) {
			dlogh("Restored Args/Rets: %d / %d\n", args[i], ((i < maxRets) ? rets[i] : -1));
		}*/
	}
}

}
