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

void LoadHookScript(const char* name, int id) {
	if (id >= numHooks) return;

	char filename[MAX_PATH];
	sprintf(filename, "scripts\\%s.int", name);
	if (fo::func::db_access(filename) && !IsGameScript(name)) {
		ScriptProgram prog;
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
			if (!HookScripts::injectAllHooks) HookScripts::InjectingHook(id); // inject hook to engine code
		} else {
			dlogr(" Error!", DL_HOOK);
		}
	}
	if (HookScripts::injectAllHooks) HookScripts::InjectingHook(id);
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
	callDepth--;
	if (callDepth && callDepth <= maxDepth) {
		argCount = lastCount[callDepth - 1];
		memcpy(args, &oldargs[maxArgs * (callDepth - 1)], maxArgs * sizeof(DWORD));
	}
}

}
