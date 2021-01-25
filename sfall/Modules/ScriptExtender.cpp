/*
 *    sfall
 *    Copyright (C) 2008-2016  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//#include <unordered_set>
#include <unordered_map>
#include <map>
#include <stack>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\Version.h"
#include "..\Utils.h"
#include "BarBoxes.h"
#include "Console.h"
#include "HookScripts.h"
#include "LoadGameHook.h"
#include "MainLoopHook.h"
#include "Worldmap.h"

#include "Scripting\Arrays.h"
#include "Scripting\Opcodes.h"
#include "Scripting\OpcodeContext.h"
#include "Scripting\Handlers\Anims.h"

#include "ScriptExtender.h"

namespace sfall
{

using namespace script;

static Delegate<> onMapExit;

static DWORD __stdcall HandleMapUpdateForScripts(const DWORD procId);

static int idle;

char ScriptExtender::gTextBuffer[5120]; // used as global temp text buffer for script functions

std::string ScriptExtender::iniConfigFolder;

static std::vector<long> scriptsIndexList;

struct GlobalScript {
	ScriptProgram prog;
	int startProc; // position of the 'start' procedure in the script
	int count;
	int repeat;
	int mode; // 0 - local map loop, 1 - input loop, 2 - world map loop, 3 - local and world map loops

	//GlobalScript() {}
	GlobalScript(ScriptProgram script) : prog(script), startProc(-1), count(0), repeat(0), mode(0) {}
};

struct ExportedVar {
	int type; // in scripting engine terms, eg. VAR_TYPE_*
	int val;
	ExportedVar() : val(0), type(VAR_TYPE_INT) {}
};

struct SelfOverrideObj {
	fo::GameObject* object;
	char counter;

	SelfOverrideObj(fo::GameObject* obj) : object(obj), counter(0) {}

	bool UnSetSelf() {
		if (counter) counter--;
		return counter == 0;
	}
};

// Events in global scripts cannot be saved because the scripts don't have a binding object
struct TimedEvent {
	ScriptProgram* script;
	unsigned long time;
	long fixed_param;
	bool isActive;

	bool operator() (const TimedEvent &a, const TimedEvent &b) {
		return a.time < b.time;
	}
} *timedEvent = nullptr;

static long executeTimedEventDepth = 0;
static std::stack<TimedEvent*> executeTimedEvents;
static std::list<TimedEvent> timerEventScripts;

static std::vector<std::string> globalScriptPathList;
static std::map<std::string, std::string> globalScriptFilesList;

static std::vector<fo::Program*> checkedScripts;
static std::vector<GlobalScript> globalScripts;

// a map of all sfall programs (global and hook scripts) by thier scriptPtr
typedef std::unordered_map<fo::Program*, ScriptProgram> SfallProgsMap;
static SfallProgsMap sfallProgsMap;

// a map scriptPtr => self_obj  to override self_obj for all script types using set_self
std::unordered_map<fo::Program*, SelfOverrideObj> selfOverrideMap;

typedef std::unordered_map<std::string, ExportedVar> ExportedVarsMap;
static ExportedVarsMap globalExportedVars;

std::unordered_map<__int64, int> globalVars;
typedef std::unordered_map<__int64, int>::iterator glob_itr;
typedef std::unordered_map<__int64, int>::const_iterator glob_citr;

DWORD availableGlobalScriptTypes = 0;
DWORD isGlobalScriptLoading = 0;
bool isGameLoading;
bool alwaysFindScripts;

fo::ScriptInstance overrideScript = {0};

long ScriptExtender::GetScriptReturnValue() {
	return overrideScript.returnValue;
}

long ScriptExtender::GetResetScriptReturnValue() {
	long val = GetScriptReturnValue();
	overrideScript.returnValue = 0;
	return val;
}

static __forceinline long FindProgram(fo::Program* program) {
	std::unordered_map<fo::Program*, SelfOverrideObj>::iterator overrideIt = selfOverrideMap.find(program);
	if (overrideIt != selfOverrideMap.end()) {
		DWORD scriptId = overrideIt->second.object->scriptId; // script
		overrideScript.id = scriptId;
		if (scriptId != -1) {
			if (overrideIt->second.UnSetSelf()) selfOverrideMap.erase(overrideIt);
			return scriptId; // returns the real scriptId of object if it is scripted
		}
		overrideScript.selfObject = overrideIt->second.object;
		overrideScript.targetObject = overrideIt->second.object;
		if (overrideIt->second.UnSetSelf()) selfOverrideMap.erase(overrideIt); // this reverts self_obj back to original value for next function calls
		return -2; // override struct
	}
	// this will allow to use functions like roll_vs_skill, etc without calling set_self (they don't really need self object)
	if (sfallProgsMap.find(program) != sfallProgsMap.end()) {
		if (timedEvent && timedEvent->script->ptr == program) {
			overrideScript.fixedParam = timedEvent->fixed_param;
		} else {
			overrideScript.fixedParam = 0;
		}
		overrideScript.targetObject = 0;
		overrideScript.selfObject = 0;
		overrideScript.returnValue = 0;
		return -2; // override struct
	}
	return -1; // change nothing
}

static long __fastcall FindOverride(fo::Program* program, fo::ScriptInstance** script) {
	long result = FindProgram(program);
	if (result == -2) {
		if (script) {
			*script = &overrideScript; // unsafe method! script may contain an incorrect address value in some engine functions
		} else {
			result--; // set -3
		}
	}
	return result;
}

static const DWORD scr_ptr_back = fo::funcoffs::scr_ptr_ + 5;
static const DWORD scr_find_sid_from_program_back = fo::funcoffs::scr_find_sid_from_program_ + 5;

static void __declspec(naked) scr_find_sid_from_program_hack() {
	__asm {
		pushadc;
		mov  ecx, eax;     // program
		call FindOverride; // edx - ref to script
		pop  ecx;
		pop  edx;
		cmp  eax, -1;
		je   normal;
		add  esp, 4; // destroy push eax
		retn; // exit from scr_find_sid_from_program_, eax = scriptId or -2, -3
normal:
		pop  eax; // restore
		push ebx;
		push ecx;
		push edx;
		push esi;
		push ebp;
		jmp  scr_find_sid_from_program_back;
	}
}

static void __declspec(naked) scr_ptr_hack() {
	__asm {
		cmp  eax, -2; // value from FindOverride
		jne  skip;
		xor  eax, eax;
		retn;
skip:
		cmp  eax, -3;
		je   override;
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		jmp  scr_ptr_back;
override: // for scr_find_obj_from_program_
		lea  eax, overrideScript;
		mov  [edx], eax;
		mov  esi, [eax]; // script.id
		xor  eax, eax;
		retn;
	}
}

static void __declspec(naked) ExecMapScriptsHack() {
	static const DWORD ExecMapScriptsRet = 0x4A67F5;
	__asm {
		push edi;
		push ebp;
		sub  esp, 0x20;
		//------
		push eax; // procId
		call HandleMapUpdateForScripts;
		jmp  ExecMapScriptsRet;
	}
}

static ExportedVar* __fastcall GetGlobalExportedVarPtr(const char* name) {
	std::string str(name);
	ExportedVarsMap::iterator it = globalExportedVars.find(str);
	//dlog_f("\n Trying to find exported var %s... ", DL_MAIN, name);
	if (it != globalExportedVars.end()) {
		ExportedVar *ptr = &it->second;
		return ptr;
	}
	return nullptr;
}

static void __stdcall CreateGlobalExportedVar(DWORD scr, const char* name) {
	//dlog_f("\nTrying to export variable %s (%d)\n", DL_MAIN, name, isGlobalScriptLoading);
	std::string str(name);
	globalExportedVars[str] = ExportedVar(); // add new
}

/*
	when fetching/storing into exported variables, first search in our own hash table instead, then (if not found), resume normal search

	reason for this: game frees all scripts and exported variables from memory when you exit map
	so if you create exported variable in sfall global script, it will work until you exit map, after that you will get crashes

	with this you can still use variables exported from global scripts even between map changes (per global scripts logic)
*/
static void __declspec(naked) Export_FetchOrStore_FindVar_Hook() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, edx;                 // varName
		call GetGlobalExportedVarPtr;
		pop  edx;
		pop  ecx;
		test eax, eax
		jz   proceedNormal;
		sub  eax, 0x24; // adjust pointer for the code
		retn;
proceedNormal:
		mov  eax, edx;  // varName
		jmp  fo::funcoffs::findVar_;
	}
}

static void __declspec(naked) Export_Export_FindVar_Hook() {
	static const DWORD Export_Export_FindVar_back = 0x4414AE;
	__asm {
		cmp  isGlobalScriptLoading, 0;
		jz   proceedNormal;
		push edx; // var name
		push ebp; // script ptr
		call CreateGlobalExportedVar;
		xor  eax, eax;
		add  esp, 4;                      // destroy return
		jmp  Export_Export_FindVar_back;  // if sfall exported var, jump to the end of function
proceedNormal:
		jmp  fo::funcoffs::findVar_;      // else - proceed normal
	}
}

// this hook prevents sfall scripts from being removed after switching to another map, since normal script engine re-loads completely
static void __stdcall FreeProgram(fo::Program* progPtr) {
	if (isGameLoading || (sfallProgsMap.find(progPtr) == sfallProgsMap.end())) { // only delete non-sfall scripts or when actually loading the game
		__asm {
			mov  eax, progPtr;
			call fo::funcoffs::interpretFreeProgram_;
		}
	}
}

static void __declspec(naked) FreeProgramHook() {
	__asm {
		push ecx;
		push edx;
		push eax;
		call FreeProgram;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) CombatBeginHook() {
	using namespace fo::Scripts;
	__asm {
		push eax;
		call fo::funcoffs::scr_set_ext_param_;
		pop  eax;                                 // pobj.sid
		mov  edx, combat_is_starting_p_proc;
		jmp  fo::funcoffs::exec_script_proc_;
	}
}

static void __declspec(naked) CombatOverHook() {
	using namespace fo::Scripts;
	__asm {
		push eax;
		call fo::funcoffs::scr_set_ext_param_;
		pop  eax;                                 // pobj.sid
		mov  edx, combat_is_over_p_proc;
		jmp  fo::funcoffs::exec_script_proc_;
	}
}

void __fastcall SetGlobalScriptRepeat(fo::Program* script, int frames) {
	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].prog.ptr == script) {
			if (frames == -1) {
				globalScripts[i].mode = !globalScripts[i].mode;
			} else {
				globalScripts[i].repeat = frames;
			}
			break;
		}
	}
}

void __fastcall SetGlobalScriptType(fo::Program* script, int type) {
	if (type <= 3) {
		for (size_t i = 0; i < globalScripts.size(); i++) {
			if (globalScripts[i].prog.ptr == script) {
				globalScripts[i].mode = type;
				break;
			}
		}
	}
}

static void SetGlobalVarInternal(__int64 var, int val) {
	glob_itr itr = globalVars.find(var);
	if (itr == globalVars.end()) {
		globalVars.emplace(var, val);
	} else {
		if (val == 0) {
			globalVars.erase(itr); // applies for both float 0.0 and integer 0
		} else {
			itr->second = val;
		}
	}
}

void SetGlobalVarInt(DWORD var, int val) {
	SetGlobalVarInternal(static_cast<__int64>(var), val);
}

long SetGlobalVar(const char* var, int val) {
	if (strlen(var) != 8) {
		return -1;
	}
	SetGlobalVarInternal(*(__int64*)var, val);
	return 0;
}

long GetGlobalVarInternal(__int64 val) {
	glob_citr itr = globalVars.find(val);
	return (itr != globalVars.end()) ? itr->second : 0;
}

long GetGlobalVarInt(DWORD var) {
	return GetGlobalVarInternal(static_cast<__int64>(var));
}

long GetGlobalVar(const char* var) {
	return (strlen(var) == 8) ? GetGlobalVarInternal(*(__int64*)var) : 0;
}

void __fastcall SetSelfObject(fo::Program* script, fo::GameObject* obj) {
	std::unordered_map<fo::Program*, SelfOverrideObj>::iterator it = selfOverrideMap.find(script);
	bool isFind = (it != selfOverrideMap.end());
	if (obj) {
		if (isFind) {
			if (it->second.object == obj) {
				it->second.counter = 2;
			} else {
				it->second.object = obj;
				it->second.counter = 0;
			}
		} else {
			selfOverrideMap.emplace(script, obj);
		}
	} else if (isFind) {
		selfOverrideMap.erase(it);
	}
}

// loads script from .int file into a sScriptProgram struct, filling script pointer and proc lookup table
void LoadScriptProgram(ScriptProgram &prog, const char* fileName, bool fullPath) {
	fo::Program* scriptPtr = fullPath
		? fo::func::allocateProgram(fileName)
		: fo::func::loadProgram(fileName);

	if (scriptPtr) {
		const char** procTable = fo::var::procTableStrs;
		prog.ptr = scriptPtr;
		// fill lookup table
		for (int i = 0; i < fo::Scripts::ScriptProc::count; ++i) {
			prog.procLookup[i] = fo::func::interpretFindProcedure(prog.ptr, procTable[i]);
		}
		prog.initialized = 0;
	} else {
		prog.ptr = nullptr;
	}
}

void InitScriptProgram(ScriptProgram &prog) {
	if (prog.initialized == 0) {
		fo::func::runProgram(prog.ptr);
		fo::func::interpret(prog.ptr, -1);
		prog.initialized = 1;
	}
}

void ScriptExtender::AddProgramToMap(ScriptProgram &prog) {
	sfallProgsMap[prog.ptr] = prog;
}

ScriptProgram* ScriptExtender::GetGlobalScriptProgram(fo::Program* scriptPtr) {
	SfallProgsMap::iterator it = sfallProgsMap.find(scriptPtr);
	return (it == sfallProgsMap.end()) ? nullptr : &it->second ; // prog
}

bool IsGameScript(const char* filename) {
	for (int i = 1; filename[i]; ++i) if (i > 7) return false; // script name is more than 8 characters
	// script name is 8 characters, try to find this name in the array of game scripts
	long mid, left = 0, right = fo::var::maxScriptNum;
	if (right > 0) {
		do {
			mid = (left + right) / 2;
			int result = std::strcmp(filename, fo::var::scriptListInfo[scriptsIndexList[mid]].fileName);
			if (result == 0) return true;
			if (result > 0) {
				left = mid + 1;
			} else {
				right = mid - 1;
			}
		} while (left <= right);
	}
	return false; // script name was not found in scripts.lst
}

static void LoadGlobalScriptsList() {
	ScriptProgram prog;
	for (auto& item : globalScriptFilesList) {
		auto &scriptFile = item.second;
		dlog("> " + scriptFile, DL_SCRIPT);
		isGlobalScriptLoading = 1;
		LoadScriptProgram(prog, scriptFile.c_str(), true);
		if (prog.ptr) {
			dlogr(" Done", DL_SCRIPT);
			GlobalScript gscript = GlobalScript(prog);
			gscript.startProc = prog.procLookup[fo::Scripts::ScriptProc::start]; // get 'start' procedure position
			globalScripts.push_back(gscript);
			ScriptExtender::AddProgramToMap(prog);
			// initialize script (start proc will be executed for the first time) -- this needs to be after script is added to "globalScripts" array
			InitScriptProgram(prog);
		} else {
			dlogr(" Error!", DL_SCRIPT);
		}
		isGlobalScriptLoading = 0;
	}
}

static void PrepareGlobalScriptsListByMask() {
	globalScriptFilesList.clear();
	for (auto& fileMask : globalScriptPathList) {
		char** filenames;
		auto basePath = fileMask.substr(0, fileMask.find_last_of("\\/") + 1); // path to scripts without mask
		int count = fo::func::db_get_file_list(fileMask.c_str(), &filenames);

		for (int i = 0; i < count; i++) {
			char* name = _strlwr(filenames[i]); // name of the script in lower case
			if (name[0] != 'g' || name[1] != 'l') continue; // fix bug in db_get_file_list fuction (if the script name begins with a non-Latin character)

			std::string baseName(name);
			int lastDot = baseName.find_last_of('.');
			if ((baseName.length() - lastDot) > 4) continue; // skip files with invalid extension (bug in db_get_file_list fuction)

			baseName = baseName.substr(0, lastDot); // script name without extension
			if (basePath != fo::var::script_path_base || !IsGameScript(baseName.c_str())) {
				std::string fullPath(basePath);
				fullPath += name;
				// prevent loading global scripts with the same name from different directories
				if (globalScriptFilesList.find(baseName) != globalScriptFilesList.end()) {
					fo::func::debug_printf("\n[SFALL] Script: %s will not be executed. A script with the same name already exists in another directory.", fullPath);
					continue;
				}
				globalScriptFilesList.insert(std::make_pair(baseName, fullPath)); // script files should be sorted in alphabetical order
			}
		}
		fo::func::db_free_file_list(&filenames, 0);
	}
}

// this runs after the game was loaded/started
static void LoadGlobalScripts() {
	static bool listIsPrepared = false;
	isGameLoading = false;

	HookScripts::LoadHookScripts();

	dlogr("Loading global scripts:", DL_SCRIPT|DL_INIT);
	if (!listIsPrepared) { // only once
		PrepareGlobalScriptsListByMask();
		listIsPrepared = !alwaysFindScripts;
		if (listIsPrepared) globalScriptPathList.clear(); // clear path list, it is no longer needed
	}
	LoadGlobalScriptsList();
	dlogr("Finished loading global scripts.", DL_SCRIPT|DL_INIT);
}

bool __stdcall ScriptHasLoaded(fo::Program* script) {
	for (size_t i = 0; i < checkedScripts.size(); i++) {
		if (checkedScripts[i] == script) {
			return false;
		}
	}
	checkedScripts.push_back(script);
	return true;
}

// this runs before actually loading/starting the game
static void ClearGlobalScripts() {
	isGameLoading = true;
	checkedScripts.clear();
	sfallProgsMap.clear();
	globalScripts.clear();
	selfOverrideMap.clear();
	globalExportedVars.clear();
	timerEventScripts.clear();
	timedEvent = nullptr;
	executeTimedEventDepth = 0;
	while (!executeTimedEvents.empty()) executeTimedEvents.pop();
	HookScripts::HookScriptClear();
}

void RunScriptProc(ScriptProgram* prog, const char* procName) {
	fo::Program* sptr = prog->ptr;
	int procNum = fo::func::interpretFindProcedure(sptr, procName);
	if (procNum != -1) {
		fo::func::executeProcedure(sptr, procNum);
	}
}

void RunScriptProc(ScriptProgram* prog, long procId) {
	if (procId > 0 && procId < fo::Scripts::ScriptProc::count) {
		int procNum = prog->procLookup[procId];
		if (procNum != -1) {
			fo::func::executeProcedure(prog->ptr, procNum);
		}
	}
}

int RunScriptStartProc(ScriptProgram* prog) {
	int procNum = prog->procLookup[fo::Scripts::ScriptProc::start];
	if (procNum != -1) {
		fo::func::executeProcedure(prog->ptr, procNum);
	}
	return procNum;
}

static void RunScript(GlobalScript* script) {
	script->count = 0;
	if (script->startProc != -1) {
		fo::func::executeProcedure(script->prog.ptr, script->startProc); // run "start"
	}
}

/**
	Do some clearing after each frame:
	- delete all temp arrays
	- reset reg_anim_* combatstate checks
*/
static void ResetStateAfterFrame() {
	DeleteAllTempArrays();
	RegAnimCombatCheck(1);
}

static inline void RunGlobalScripts(int mode1, int mode2) {
	if (idle > -1) Sleep(idle);

	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].repeat
			&& (globalScripts[i].mode == mode1 || globalScripts[i].mode == mode2)
			&& ++globalScripts[i].count >= globalScripts[i].repeat) {
			RunScript(&globalScripts[i]);
		}
	}
	ResetStateAfterFrame();
}

static void RunGlobalScriptsOnMainLoop() {
	RunGlobalScripts(0, 3);
}

static void RunGlobalScriptsOnInput() {
	if (IsGameLoaded()) {
		RunGlobalScripts(1, 1);
	}
}

static void RunGlobalScriptsOnWorldMap() {
	RunGlobalScripts(2, 3);
}

static DWORD __stdcall HandleMapUpdateForScripts(const DWORD procId) {
	if (procId == fo::Scripts::ScriptProc::map_enter_p_proc) {
		// map changed, all game objects were destroyed and scripts detached, need to re-insert global scripts into the game
		for (std::vector<GlobalScript>::const_iterator it = globalScripts.cbegin(); it != globalScripts.cend(); ++it) {
			fo::func::runProgram(it->prog.ptr);
		}
	} else if (procId == fo::Scripts::ScriptProc::map_exit_p_proc) {
		onMapExit.invoke();
	}

	RunGlobalScriptsAtProc(procId); // gl* scripts of types 0 and 3
	HookScripts::RunHookScriptsAtProc(procId);   // all hs_ scripts

	return procId; // restore eax (don't delete)
}

static DWORD HandleTimedEventScripts() {
	DWORD currentTime = fo::var::fallout_game_time;
	if (timerEventScripts.empty()) return currentTime;

	executeTimedEventDepth++;

	fo::func::dev_printf("\n[TimedEventScripts] Time: %d / Depth: %d", currentTime, executeTimedEventDepth);
	for (auto it = timerEventScripts.cbegin(); it != timerEventScripts.cend(); ++it) {
		fo::func::dev_printf("\n[TimedEventScripts] Event: %d", it->time);
	}

	bool eventWasRunning = false;
	for (auto timerIt = timerEventScripts.cbegin(); timerIt != timerEventScripts.cend(); ++timerIt) {
		if (timerIt->isActive == false) continue;
		if (currentTime >= timerIt->time) {
			if (timedEvent) executeTimedEvents.push(timedEvent); // store a pointer to the currently running event

			timedEvent = const_cast<TimedEvent*>(&(*timerIt));
			timedEvent->isActive = false;

			fo::func::dev_printf("\n[TimedEventScripts] Run event: %d", timerIt->time);
			RunScriptProc(timerIt->script, fo::Scripts::ScriptProc::timed_event_p_proc);
			fo::func::dev_printf("\n[TimedEventScripts] Event done: %d", timerIt->time);

			timedEvent = nullptr;
			if (!executeTimedEvents.empty()) {
				timedEvent = executeTimedEvents.top(); // restore a pointer to a previously running event
				executeTimedEvents.pop();
			}
			eventWasRunning = true;
		} else {
			break;
		}
	}
	executeTimedEventDepth--;

	if (eventWasRunning && executeTimedEventDepth == 0) {
		timedEvent = nullptr;
		// delete all previously executed events
		for (auto it = timerEventScripts.cbegin(); it != timerEventScripts.cend();) {
			if (it->isActive == false) {
				fo::func::dev_printf("\n[TimedEventScripts] Remove event: %d", it->time);
				it = timerEventScripts.erase(it);
			} else {
				++it;
			}
		}
	}
	return currentTime;
}

static DWORD TimedEventNextTime() {
	DWORD nextTime;
	__asm {
		push ecx;
		call fo::funcoffs::queue_next_time_;
		mov  nextTime, eax;
		push edx;
	}
	if (!timerEventScripts.empty() && timerEventScripts.front().isActive) {
		DWORD time = timerEventScripts.front().time;
		if (!nextTime || time < nextTime) nextTime = time;
	}
	__asm pop edx;
	__asm pop ecx;
	return nextTime;
}

static DWORD script_chk_timed_events_hook() {
	return (!fo::var::queue && timerEventScripts.empty());
}

void ScriptExtender::AddTimerEventScripts(fo::Program* script, long time, long param) {
	ScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	TimedEvent timer;
	timer.isActive = true;
	timer.script = scriptProg;
	timer.fixed_param = param;
	timer.time = fo::var::fallout_game_time + time;
	timerEventScripts.push_back(std::move(timer));
	timerEventScripts.sort(TimedEvent());
}

void ScriptExtender::RemoveTimerEventScripts(fo::Program* script, long param) {
	ScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	timerEventScripts.remove_if([scriptProg, param] (TimedEvent timer) {
		return timer.script == scriptProg && timer.fixed_param == param;
	});
}

void ScriptExtender::RemoveTimerEventScripts(fo::Program* script) {
	ScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	timerEventScripts.remove_if([scriptProg] (TimedEvent timer) {
		return timer.script == scriptProg;
	});
}

// run all global scripts of types 0 and 3 at specific procedure (if exist)
void RunGlobalScriptsAtProc(DWORD procId) {
	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].mode != 0 && globalScripts[i].mode != 3) continue;
		RunScriptProc(&globalScripts[i].prog, procId);
	}
}

bool LoadGlobals(HANDLE h) {
	DWORD count, unused;
	ReadFile(h, &count, 4, &unused, 0);
	if (unused != 4) return true;
	GlobalVar var;
	for (DWORD i = 0; i < count; i++) {
		ReadFile(h, &var, sizeof(GlobalVar), &unused, 0);
		globalVars.emplace(var.id, var.val);
	}
	return false;
}

void SaveGlobals(HANDLE h) {
	DWORD count, unused;
	count = globalVars.size();
	WriteFile(h, &count, 4, &unused, 0);
	GlobalVar var;
	glob_citr itr = globalVars.begin();
	while (itr != globalVars.end()) {
		var.id = itr->first;
		var.val = itr->second;
		WriteFile(h, &var, sizeof(GlobalVar), &unused, 0);
		++itr;
	}
}

static void ClearGlobals() {
	globalVars.clear();
	for (array_itr it = arrays.begin(); it != arrays.end(); ++it) {
		it->second.clearArrayVar();
	}
	arrays.clear();
	savedArrays.clear();
}

int GetNumGlobals() {
	return globalVars.size();
}

void GetGlobals(GlobalVar* globals) {
	glob_citr itr = globalVars.begin();
	int i = 0;
	while (itr != globalVars.end()) {
		globals[i].id = itr->first;
		globals[i++].val = itr->second;
		++itr;
	}
}

void SetGlobals(GlobalVar* globals) {
	glob_itr itr = globalVars.begin();
	int i = 0;
	while (itr != globalVars.end()) {
		itr->second = globals[i++].val;
		++itr;
	}
}

static void __declspec(naked) map_save_in_game_hook() {
	__asm {
		test cl, 1;
		jz   skip;
		jmp  fo::funcoffs::scr_exec_map_exit_scripts_;
skip:
		jmp  fo::funcoffs::partyMemberSaveProtos_;
	}
}

static void ClearEventsOnMapExit() {
	using namespace fo;
	__asm {
		mov  eax, explode_event; // type
		mov  edx, fo::funcoffs::queue_explode_exit_; // func
		call fo::funcoffs::queue_clear_type_;
		mov  eax, explode_fail_event;
		mov  edx, fo::funcoffs::queue_explode_exit_;
		call fo::funcoffs::queue_clear_type_;
	}
}

// Creates an list of indexes arranged in sorted order relative to the script names
static void BuildSortedIndexList() {
	scriptsIndexList.reserve(fo::var::maxScriptNum);
	scriptsIndexList.push_back(0);

	for (long index = 1; index < fo::var::maxScriptNum; index++) {
		size_t size = scriptsIndexList.size();
		size_t lastPos = size - 1;
		for (size_t posIndex = 0; posIndex < size; posIndex++) {
			if (std::strcmp(fo::var::scriptListInfo[index].fileName, fo::var::scriptListInfo[scriptsIndexList[posIndex]].fileName) > 0) {
				if (posIndex < lastPos) continue;  // if this is not the end of the array, then go to the next name
				scriptsIndexList.push_back(index); // otherwise insert at the end of the array
			} else {
				scriptsIndexList.insert(scriptsIndexList.cbegin() + posIndex, index); // insert before if the comparison result is less than or equal to
			}
			break;
		}
	}
	// preview the sorted list
	//for (size_t i = 0; i < scriptsIndexList.size(); i++) {
	//	devlog_f("\nName: %s, i: %d", DL_MAIN, fo::var::scriptListInfo[scriptsIndexList[i]].fileName, scriptsIndexList[i]);
	//}
	//devlog_f("\nCount: %d\n", DL_MAIN, scriptsIndexList.size());
}

void ScriptExtender::init() {
	LoadGameHook::OnAfterGameInit() += BuildSortedIndexList;
	LoadGameHook::OnAfterGameStarted() += LoadGlobalScripts;
	LoadGameHook::OnGameReset() += [] () {
		ClearGlobalScripts();
		ClearGlobals();
		RegAnimCombatCheck(1);
	};

	MainLoopHook::OnMainLoop() += RunGlobalScriptsOnMainLoop;
	MainLoopHook::OnCombatLoop() += RunGlobalScriptsOnMainLoop;
	OnInputLoop() += RunGlobalScriptsOnInput;
	Worldmap::OnWorldmapLoop() += RunGlobalScriptsOnWorldMap;

	globalScriptPathList = GetConfigList("Scripts", "GlobalScriptPaths", "scripts\\gl*.int", 255);
	for (unsigned int i = 0; i < globalScriptPathList.size(); i++) {
		ToLowerCase(globalScriptPathList[i]);
	}

	idle = GetConfigInt("Misc", "ProcessorIdle", -1);
	if (idle > -1) {
		if (idle > 127) idle = 127;
		fo::var::idle_func = reinterpret_cast<void*>(Sleep);
		SafeWrite8(0x4C9F12, 0x6A); // push idle
		SafeWrite8(0x4C9F13, idle);
	}

	arraysBehavior = GetConfigInt("Misc", "arraysBehavior", 1);
	if (arraysBehavior > 0) {
		arraysBehavior = 1; // only 1 and 0 allowed at this time
		dlogr("New arrays behavior enabled.", DL_SCRIPT);
	} else {
		dlogr("Arrays in backward-compatiblity mode.", DL_SCRIPT);
	}

	iniConfigFolder = GetConfigString("Scripts", "IniConfigFolder", "", 64);
	size_t len = iniConfigFolder.length();
	if (len) {
		char c = iniConfigFolder[len - 1];
		bool pathSeparator = (c == '\\' || c == '/');
		if (len > 62 || (len == 62 && !pathSeparator)) {
			iniConfigFolder.clear();
			dlogr("Error: IniConfigFolder path is too long.", DL_INIT|DL_SCRIPT);
		} else if (!pathSeparator) {
			iniConfigFolder += '\\';
		}
	}

	alwaysFindScripts = isDebug && (iniGetInt("Debugging", "AlwaysFindScripts", 0, ::sfall::ddrawIni) != 0);
	if (alwaysFindScripts) dlogr("Always searching for global/hook scripts behavior enabled.", DL_SCRIPT);

	MakeJump(0x4A390C, scr_find_sid_from_program_hack);
	MakeJump(0x4A5E34, scr_ptr_hack);

	MakeJump(0x4A67F0, ExecMapScriptsHack);

	HookCall(0x4A26D6, HandleTimedEventScripts); // queue_process_
	HookCalls(TimedEventNextTime, {
		0x4C1C67, // wmGameTimeIncrement_
		0x4A3E1C, // script_chk_timed_events_
		0x499AFA, 0x499CD7, 0x499E2B // TimedRest_
	});
	HookCall(0x4A3E08, script_chk_timed_events_hook);

	// this patch makes it possible to export variables from sfall global scripts
	HookCall(0x4414C8, Export_Export_FindVar_Hook);
	HookCalls(Export_FetchOrStore_FindVar_Hook, {
		0x441285, // store
		0x4413D9  // fetch
	});

	HookCall(0x46E141, FreeProgramHook);

	// combat_is_starting_p_proc / combat_is_over_p_proc
	HookCall(0x421B72, CombatBeginHook);
	HookCall(0x421FC1, CombatOverHook);

	// Reorder the execution of functions before exiting the map
	// Call saving party member prototypes and removing the drug effects for NPC after executing map_exit_p_proc procedure
	HookCall(0x483CB4, map_save_in_game_hook);
	HookCall(0x483CC3, (void*)fo::funcoffs::partyMemberSaveProtos_);
	HookCall(0x483CC8, (void*)fo::funcoffs::partyMemberPrepLoad_);
	HookCall(0x483CCD, (void*)fo::funcoffs::partyMemberPrepItemSaveAll_);

	// Set the DAM_BACKWASH flag for the attacker before calling compute_damage_
	SafeWrite32(0x423DE7, 0x40164E80); // or [esi+ctd.flags3Source], DAM_BACKWASH_
	long idata = 0x146E09;             // or dword ptr [esi+ctd.flagsSource], ebp
	SafeWriteBytes(0x423DF0, (BYTE*)&idata, 3);
	if (*(BYTE*)0x423DEB != 0xE8) { // not hook call
		MakeCall(0x423DEB, (void*)fo::funcoffs::compute_damage_);
	}

	InitNewOpcodes();

	ScriptExtender::OnMapExit() += ClearEventsOnMapExit; // for reordering the execution of functions before exiting the map
}

Delegate<>& ScriptExtender::OnMapExit() {
	return onMapExit;
}

}
