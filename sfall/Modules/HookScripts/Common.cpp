#include "..\..\FalloutEngine\Fallout2.h"

#include "Common.h"

namespace sfall
{

constexpr int maxArgs = 16;
constexpr int maxDepth = 8;

DWORD args[maxArgs]; // current hook arguments
DWORD oldargs[maxArgs * maxDepth];
DWORD* argPtr;
DWORD rets[16]; // current hook return values

DWORD firstArg = 0;
DWORD callDepth;
DWORD lastCount[maxDepth];

DWORD argCount;
DWORD cArg; // how many arguments were taken by current hook script
DWORD cRet; // how many return values were set by current hook script
DWORD cRetTmp; // how many return values were set by specific hook script (when using register_hook)

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
		if (callDepth) {
			lastCount[callDepth - 1] = argCount;
			memcpy(&oldargs[maxArgs * (callDepth - 1)], args, maxArgs * sizeof(DWORD));
		}
		argPtr = args;
		for (DWORD i = 0; i < callDepth; i++) {
			argPtr += lastCount[i];
		}
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
	if (hooks[hook].size()) {
		if (isDebug) dlogh("Running hook %d, which has %0d entries attached\n", hook, hooks[hook].size());
		cRet = 0;
		for (int i = hooks[hook].size() - 1; i >= 0; i--) {
			RunSpecificHookScript(&hooks[hook][i]);
		}
	} else {
		cArg = 0;
		cRet = 0;
	}
}

void _stdcall EndHook() {
	#ifndef NDEBUG
		dlogh("Ending running hook, current depth: %d\n", callDepth, 0);
	#endif
	callDepth--;
	if (callDepth && callDepth <= maxDepth) {
		argCount = lastCount[callDepth - 1];
		memcpy(args, &oldargs[maxArgs * (callDepth - 1)], maxArgs * sizeof(DWORD));
	}
}

}
