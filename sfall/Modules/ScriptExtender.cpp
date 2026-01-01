/*
 *    sfall
 *    Copyright (C) 2008-2026  The sfall team
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

#include <unordered_set>
#include <unordered_map>
#include <stack>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\Translate.h"
#include "..\version.h"
#include "HookScripts.h"
#include "LoadGameHook.h"

#include "SubModules\ObjectName.h"

#include "Scripting\Arrays.h"
#include "Scripting\Opcodes.h"
#include "Scripting\OpcodeContext.h"
#include "Scripting\Handlers\Anims.h"

#include "ScriptExtender.h"

namespace sfall
{

using namespace script;

static DWORD __stdcall HandleMapUpdateForScripts(const DWORD procId);

static void ClearEventsOnMapExit();

static DWORD highlightingToggled = 0;
static DWORD toggleHighlightsKey;
static DWORD highlightContainers;
static DWORD highlightCorpses;
static DWORD motionScanner;
static int outlineColor;
static int outlineColorContainers;
static int outlineColorCorpses;
static char highlightFail1[128];
static char highlightFail2[128];

char ScriptExtender::gTextBuffer[5120]; // used as global temp text buffer for script functions

char ScriptExtender::iniConfigFolder[64];

bool ScriptExtender::OnMapLeave;

static std::vector<long> scriptsIndexList;

struct GlobalScript {
	ScriptProgram prog;
	int startProc; // position of the 'start' procedure in the script
	int count;
	int repeat;
	int mode;      // 0 - local map loop, 1 - input loop, 2 - world map loop, 3 - local and world map loops

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

static std::vector<std::string> globalScriptFilesList;

static std::vector<GlobalScript> globalScripts;
static std::unordered_set<fo::Program*> checkedScripts;

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
typedef std::pair<__int64, int> glob_pair;

DWORD availableGlobalScriptTypes = 0;
static DWORD isGlobalScriptLoading = 0;
static bool isGameReset;
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

static __declspec(naked) void scr_find_sid_from_program_hack() {
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

static __declspec(naked) void scr_ptr_hack() {
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

static __declspec(naked) void ExecMapScriptsHack() {
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
	devlog_f("Trying to find exported var %s\n", DL_MAIN, name);
	std::string str(name);
	ExportedVarsMap::iterator it = globalExportedVars.find(str);
	if (it != globalExportedVars.end()) {
		ExportedVar *ptr = &it->second;
		return ptr;
	}
	return nullptr;
}

static void __fastcall CreateGlobalExportedVar(fo::Program* script, const char* name) {
	devlog_f("Trying to export variable %s (%d)\n", DL_MAIN, name, isGlobalScriptLoading);
	std::string str(name);
	globalExportedVars[str] = ExportedVar(); // add new
}

/*
	when fetching/storing into exported variables, first search in our own hash table instead, then (if not found), resume normal search

	reason for this: game frees all scripts and exported variables from memory when you exit map
	so if you create exported variable in sfall global script, it will work until you exit map, after that you will get crashes

	with this you can still use variables exported from global scripts even between map changes (per global scripts logic)
*/
static __declspec(naked) void Export_FetchOrStore_FindVar_Hook() {
	__asm {
		push ecx;
		push edx;
		mov  ecx, edx;  // varName
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

static __declspec(naked) void exportExportVariable_hook() {
	static const DWORD exportExportVariable_BackRet = 0x4414AE;
	__asm {
		cmp  isGlobalScriptLoading, 0;
		jz   proceedNormal;
		mov  ecx, ebp;                     // script ptr
		call CreateGlobalExportedVar;      // edx - var name
		xor  eax, eax;
		add  esp, 4;                       // destroy return
		jmp  exportExportVariable_BackRet; // if sfall exported var, jump to the end of function
proceedNormal:
		jmp  fo::funcoffs::findVar_;       // else - proceed normal
	}
}

// this hook prevents sfall scripts from being removed after switching to another map, since normal script engine re-loads completely
static void __stdcall FreeProgram(fo::Program* progPtr) {
	if (isGameReset || (sfallProgsMap.find(progPtr) == sfallProgsMap.end())) { // only delete non-sfall scripts or when actually loading the game
		__asm {
			mov  eax, progPtr;
			call fo::funcoffs::interpretFreeProgram_;
		}
	}
}

static __declspec(naked) void FreeProgramHook() {
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

static __declspec(naked) void CombatBeginHook() {
	using namespace fo::Scripts;
	__asm {
		push eax;
		call fo::funcoffs::scr_set_ext_param_;
		pop  eax;                                 // pobj.sid
		mov  edx, combat_is_starting_p_proc;
		jmp  fo::funcoffs::exec_script_proc_;
	}
}

static __declspec(naked) void CombatOverHook() {
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
		globalVars.insert(glob_pair(var, val));
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
			selfOverrideMap.insert(std::make_pair(script, obj));
		}
	} else if (isFind) {
		selfOverrideMap.erase(it);
	}
}

////////////////////////// BUILT-IN ITEM HIGHLIGHTING //////////////////////////

static __declspec(naked) void obj_outline_all_items_on() {
	using namespace fo;
	using namespace Fields;
	__asm {
		pushadc;
		mov  eax, ds:[FO_VAR_map_elevation];
		call fo::funcoffs::obj_find_first_at_;
		jmp  checkObject;
nextObject:
		call fo::funcoffs::obj_find_next_at_;
checkObject:
		test eax, eax;
		jz   end;
		cmp  eax, ds:[FO_VAR_outlined_object];
		je   nextObject;
		cmp  dword ptr [eax + owner], 0;          // Owned by someone?
		jne  nextObject;                          // Yes
		cmp  dword ptr [eax + outline], 0;        // Already outlined?
		jne  nextObject;                          // Yes
		mov  ecx, [eax + artFid];
		and  ecx, 0xF000000;
		sar  ecx, 0x18;
		cmp  ecx, OBJ_TYPE_CRITTER;               // Is this a critter?
		mov  ecx, eax;
		je   isCritter;                           // Yes
		ja   nextObject;                          // Neither an item nor a critter
		test byte ptr [eax + flags + 1], 0x10;    // Is NoHighlight_ flag set?
		jz   normalItem;                          // No
		call fo::funcoffs::item_get_type_;
		cmp  eax, item_type_container;            // Is this item a container?
		je   isContainer;                         // Yes
		jmp  nextObject;
isCritter:
		cmp  highlightCorpses, 0;                 // Highlight corpses?
		je   nextObject;                          // No
		test byte ptr [ecx + damageFlags], DAM_DEAD; // source.results & DAM_DEAD?
		jz   nextObject;                          // No
		mov  edx, NoSteal;                        // _Steal flag
		mov  eax, [ecx + protoId];                // eax = source.pid
		call fo::funcoffs::critter_flag_check_;
		test eax, eax;                            // Can't be stolen from?
		jnz  nextObject;                          // Yes
		mov  eax, outlineColorCorpses;            // Set outline color for corpses
		jmp  setOutline;
isContainer:
		cmp  highlightContainers, 0;              // Highlight containers?
		je   nextObject;
		mov  eax, outlineColorContainers;         // Set outline color for containers
		jmp  setOutline;
normalItem:
		mov  eax, outlineColor;                   // Set outline color for items
setOutline:
		mov  [ecx + outline], eax;
		jmp  nextObject;
end:
		popadc;
		jmp  fo::funcoffs::tile_refresh_display_;
	}
}

static __declspec(naked) void obj_outline_all_items_off() {
	using namespace fo;
	using namespace Fields;
	__asm {
		pushadc;
		mov  eax, ds:[FO_VAR_map_elevation];
		call fo::funcoffs::obj_find_first_at_;
		jmp  checkObject;
nextObject:
		call fo::funcoffs::obj_find_next_at_;
checkObject:
		test eax, eax;
		jz   end;
		cmp  eax, ds:[FO_VAR_outlined_object];
		je   nextObject;
		mov  ecx, [eax + artFid];
		and  ecx, 0xF000000;
		sar  ecx, 0x18;
		test ecx, ecx;                            // Is this an item?
		jz   isItem;                              // Yes
		dec  ecx;                                 // Is this a critter?
		jnz  nextObject;                          // No
		test byte ptr [eax + damageFlags], DAM_DEAD; // source.results & DAM_DEAD?
		jz   nextObject;                          // No
isItem:
		cmp  [eax + owner], ecx;                  // Owned by someone? (ecx = 0)
		jne  nextObject;                          // Yes
		mov  [eax + outline], ecx;                // Remove outline
		jmp  nextObject;
end:
		popadc;
		jmp  fo::funcoffs::tile_refresh_display_;
	}
}

static __declspec(naked) void obj_remove_outline_hook() {
	__asm {
		call fo::funcoffs::obj_remove_outline_;
		test eax, eax;
		jnz  end;
		cmp  highlightingToggled, 1;
		jne  end;
		mov  ds:[FO_VAR_outlined_object], eax;
		call obj_outline_all_items_on;
end:
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////

// loads script from .int file into a ScriptProgram struct, filling script pointer and proc lookup table
void InitScriptProgram(ScriptProgram &prog, const char* fileName) {
	prog.initialized = false;
	fo::Program* scriptPtr = fo::func::loadProgram(fileName);

	if (scriptPtr) {
		prog.ptr = scriptPtr;
		// fill lookup table
		const char** procTable = fo::ptr::procTableStrs;
		for (int i = 0; i < fo::Scripts::ScriptProc::count; ++i) {
			prog.procLookup[i] = fo::func::interpretFindProcedure(prog.ptr, procTable[i]);
		}
	} else {
		prog.ptr = nullptr;
	}
}

void RunScriptProgram(ScriptProgram &prog) {
	if (!prog.initialized && prog.ptr) {
		fo::func::runProgram(prog.ptr);
		fo::func::interpret(prog.ptr, -1);
		prog.initialized = true;
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
	long mid, left = 0, right = *fo::ptr::maxScriptNum;
	if (right > 0) {
		do {
			mid = (left + right) / 2;
			int result = std::strcmp(filename, (*fo::ptr::scriptListInfo)[scriptsIndexList[mid]].fileName);
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

// loads and initializes script file (for normal game scripts)
long __fastcall ScriptExtender::InitScript(long sid) {
	fo::ScriptInstance* scriptPtr;
	if (fo::func::scr_ptr(sid, &scriptPtr) == -1) return -1;

	scriptPtr->program = fo::func::loadProgram((*fo::ptr::scriptListInfo)[scriptPtr->scriptIdx & 0xFFFFFF].fileName);
	if (!scriptPtr->program) return -1;
	if (scriptPtr->program->flags & 0x124) return 0;

	// fill lookup table
	fo::func::scr_build_lookup_table(scriptPtr);

	scriptPtr->flags |= 4 | 1; // init | loaded
	scriptPtr->action = fo::Scripts::ScriptProc::no_p_proc;
	scriptPtr->scriptOverrides = 0;

	fo::func::runProgram(scriptPtr->program);
	return 0;
}

static void LoadGlobalScriptsList() {
	dlogr("Running global scripts...", DL_SCRIPT);

	ScriptProgram prog;
	for (std::vector<std::string>::const_iterator it = globalScriptFilesList.begin(); it != globalScriptFilesList.end(); ++it) {
		dlog("> ", DL_SCRIPT);
		dlog(it->c_str(), DL_SCRIPT);
		InitScriptProgram(prog, it->c_str());
		if (prog.ptr) {
			GlobalScript gscript = GlobalScript(prog);
			gscript.startProc = prog.procLookup[fo::Scripts::ScriptProc::start]; // get 'start' procedure position
			globalScripts.push_back(gscript);
			ScriptExtender::AddProgramToMap(prog);
			dlogr(" Done", DL_SCRIPT);
			// initialize script (start proc will be executed for the first time) -- this needs to be after script is added to "globalScripts" array
			RunScriptProgram(prog);
		} else {
			dlogr(" Error!", DL_SCRIPT);
		}
	}
}

// this runs after the game was loaded/started
void InitGlobalScripts() {
	isGameReset = false;
	isGlobalScriptLoading = 1; // this should allow to register global exported variables

	HookScripts::InitHookScripts();
	LoadGlobalScriptsList();
	// Fix map lighting from Night Vision perk when loading a saved game
	fo::func::light_set_ambient(*fo::ptr::ambient_light, 1); // refresh map lighting

	isGlobalScriptLoading = 0;
}

static void PrepareGlobalScriptsList() {
	globalScriptFilesList.clear();

	char** filenames;
	int count = fo::func::db_get_file_list("scripts\\gl*.int", &filenames);

	for (int i = 0; i < count; i++) {
		char* name = _strlwr(filenames[i]); // name of the script in lower case
		if (name[0] != 'g' || name[1] != 'l') continue; // fix bug in db_get_file_list fuction (if the script name begins with a non-Latin character)

		std::string baseName(name);
		int lastDot = baseName.find_last_of('.');
		if ((baseName.length() - lastDot) > 4) continue; // skip files with invalid extension (bug in db_get_file_list fuction)

		baseName = baseName.substr(0, lastDot); // script name without extension
		if (!IsGameScript(baseName.c_str())) {
			dlog_f("Found global script: %s\n", DL_SCRIPT, name);
			globalScriptFilesList.push_back(std::move(baseName));
		}
	}
	fo::func::db_free_file_list(&filenames, 0);
	globalScripts.reserve(globalScriptFilesList.size());
	float bCount = globalScriptFilesList.size() / checkedScripts.max_load_factor();
	checkedScripts.rehash(static_cast<size_t>(bCount) + (bCount > static_cast<size_t>(bCount) ? 1 : 0)); // ceil
}

// this runs before the game was loaded/started
void LoadGlobalScripts() {
	static bool listIsPrepared = false;

	HookScripts::LoadHookScripts();

	dlogr("Loading global scripts:", DL_SCRIPT|DL_INIT);
	if (!listIsPrepared) { // only once
		PrepareGlobalScriptsList();
		listIsPrepared = !alwaysFindScripts;
	}
	dlogr("Finished loading global scripts.", DL_SCRIPT|DL_INIT);
}

static struct {
	fo::Program* script;
	long index;
} lastProgram = {nullptr};

int __stdcall ScriptExtender::ScriptHasLoaded(fo::Program* script) {
	if (checkedScripts.find(script) != checkedScripts.end()) {
		return 0; // has already been called from the script
	}
	checkedScripts.insert(script);
	return 1;
}

// this runs before actually loading/starting the game
static void ClearGlobalScripts() {
	devlog_f("\nReset global scripts.", DL_MAIN);
	isGameReset = true;
	sfallProgsMap.clear();
	globalScripts.clear();
	checkedScripts.clear();
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
	int procPosition = fo::func::interpretFindProcedure(sptr, procName);
	if (procPosition != -1) {
		fo::func::executeProcedure(sptr, procPosition);
	}
}

void RunScriptProc(ScriptProgram* prog, long procId) {
	if (procId > 0 && procId < fo::Scripts::ScriptProc::count) {
		int procPosition = prog->procLookup[procId];
		if (procPosition != -1) {
			fo::func::executeProcedure(prog->ptr, procPosition);
		}
	}
}

int RunScriptStartProc(ScriptProgram* prog) {
	int procPosition = prog->procLookup[fo::Scripts::ScriptProc::start];
	if (procPosition != -1) {
		fo::func::executeProcedure(prog->ptr, procPosition);
	}
	return procPosition;
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
	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].repeat
			&& (globalScripts[i].mode == mode1 || globalScripts[i].mode == mode2)
			&& ++globalScripts[i].count >= globalScripts[i].repeat) {
			RunScript(&globalScripts[i]);
		}
	}
	ResetStateAfterFrame();
}

void RunGlobalScriptsOnMainLoop() {
	if (toggleHighlightsKey) {
		// 0x48C294 to toggle
		if (KeyDown(toggleHighlightsKey)) {
			if (!highlightingToggled) {
				if (motionScanner & 4) {
					fo::GameObject* scanner = fo::func::inven_pid_is_carried_ptr(*fo::ptr::obj_dude, fo::PID_MOTION_SENSOR);
					if (scanner) {
						if (!(motionScanner & 2)) {
							highlightingToggled = fo::func::item_m_dec_charges(scanner) + 1;
							if (!highlightingToggled) fo::func::display_print(highlightFail2);
							else fo::func::intface_redraw();
						} else {
							highlightingToggled = 1;
						}
					} else {
						fo::func::display_print(highlightFail1);
					}
				} else {
					highlightingToggled = 1;
				}
				if (highlightingToggled) obj_outline_all_items_on();
				else highlightingToggled = 2;
			}
		} else if (highlightingToggled) {
			if (highlightingToggled == 1) obj_outline_all_items_off();
			highlightingToggled = 0;
		}
	}
	RunGlobalScripts(0, 3);
}

void RunGlobalScriptsOnInput() {
	if (IsGameLoaded()) {
		RunGlobalScripts(1, 1);
	}
}

void RunGlobalScriptsOnWorldMap() {
	RunGlobalScripts(2, 3);
}

static DWORD __stdcall HandleMapUpdateForScripts(const DWORD procId) {
	if (procId == fo::Scripts::ScriptProc::map_enter_p_proc) {
		// map changed, all game objects were destroyed and scripts detached, need to re-insert global scripts into the game
		for (std::vector<GlobalScript>::const_iterator it = globalScripts.cbegin(); it != globalScripts.cend(); ++it) {
			fo::func::runProgram(it->prog.ptr);
		}
	} else if (procId == fo::Scripts::ScriptProc::map_exit_p_proc) {
		ClearEventsOnMapExit(); // for reordering the execution of functions before exiting the map
	}

	RunGlobalScriptsAtProc(procId);            // gl* scripts of types 0 and 3
	HookScripts::RunHookScriptsAtProc(procId); // all hs_ scripts

	return procId; // restore eax (don't delete)
}

static DWORD HandleTimedEventScripts() {
	DWORD currentTime = *fo::ptr::fallout_game_time;
	if (timerEventScripts.empty()) return currentTime;

	executeTimedEventDepth++;

	fo::func::dev_printf("\n[TimedEventScripts] Time: %d / Depth: %d", currentTime, executeTimedEventDepth);
	for (std::list<TimedEvent>::const_iterator it = timerEventScripts.cbegin(); it != timerEventScripts.cend(); ++it) {
		fo::func::dev_printf("\n[TimedEventScripts] Event: %d", it->time);
	}

	bool eventWasRunning = false;
	for (std::list<TimedEvent>::const_iterator timerIt = timerEventScripts.cbegin(); timerIt != timerEventScripts.cend(); ++timerIt) {
		if (!timerIt->isActive) continue;
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
		for (std::list<TimedEvent>::const_iterator it = timerEventScripts.cbegin(); it != timerEventScripts.cend();) {
			if (!it->isActive) {
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
	return (!*fo::ptr::queue && timerEventScripts.empty());
}

void ScriptExtender::AddTimerEventScripts(fo::Program* script, long time, long param) {
	ScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	TimedEvent timer;
	timer.isActive = true;
	timer.script = scriptProg;
	timer.fixed_param = param;
	timer.time = *fo::ptr::fallout_game_time + time;
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
		globalVars.insert(glob_pair(var.id, var.val));
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

static DWORD pcFlagSneak;

static __declspec(naked) void map_save_in_game_hook() {
	__asm {
		test cl, 1;
		jz   skip;
		mov  ScriptExtender::OnMapLeave, 1;
		call fo::funcoffs::scr_exec_map_exit_scripts_;
		xor  eax, eax;
		call fo::funcoffs::is_pc_flag_;
		mov  pcFlagSneak, eax; // save sneak state
		mov  ScriptExtender::OnMapLeave, 0;
		retn;
skip:
		jmp  fo::funcoffs::partyMemberSaveProtos_;
	}
}

static __declspec(naked) void map_load_file_hook() {
	__asm {
		cmp  pcFlagSneak, 0;
		jz   skip;
		xor  eax, eax;
		mov  pcFlagSneak, eax;
		call fo::funcoffs::pc_flag_on_;
skip:
		jmp  fo::funcoffs::scr_exec_map_enter_scripts_;
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
void BuildSortedIndexList() {
	scriptsIndexList.reserve(*fo::ptr::maxScriptNum);
	scriptsIndexList.push_back(0);

	for (long index = 1; index < *fo::ptr::maxScriptNum; index++) {
		size_t size = scriptsIndexList.size();
		size_t lastPos = size - 1;
		for (size_t posIndex = 0; posIndex < size; posIndex++) {
			if (std::strcmp((*fo::ptr::scriptListInfo)[index].fileName, (*fo::ptr::scriptListInfo)[scriptsIndexList[posIndex]].fileName) > 0) {
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
	//	devlog_f("\nName: %s, i: %d", DL_MAIN, (*fo::ptr::scriptListInfo)[scriptsIndexList[i]].fileName, scriptsIndexList[i]);
	//}
	//devlog_f("\nCount: %d\n", DL_MAIN, scriptsIndexList.size());
}

void ScriptExtender::OnGameReset() {
	ClearGlobalScripts();
	ClearGlobals();
	RegAnimCombatCheck(1);

	Opcodes::OnGameReset();
	ObjectName::OnGameReset();
}

void ScriptExtender::init() {
	toggleHighlightsKey = IniReader::GetConfigInt("Input", "ToggleItemHighlightsKey", 0);
	if (toggleHighlightsKey) {
		highlightContainers = IniReader::GetConfigInt("Input", "HighlightContainers", 0);
		highlightCorpses = IniReader::GetConfigInt("Input", "HighlightCorpses", 0);
		outlineColor = IniReader::GetConfigInt("Input", "OutlineColor", 16);
		if (outlineColor < 1) outlineColor = 64;
		outlineColorContainers = IniReader::GetConfigInt("Input", "OutlineColorContainers", 16);
		if (outlineColorContainers < 1) outlineColorContainers = 64;
		outlineColorCorpses = IniReader::GetConfigInt("Input", "OutlineColorCorpses", 16);
		if (outlineColorCorpses < 1) outlineColorCorpses = 64;
		motionScanner = IniReader::GetConfigInt("Misc", "MotionScannerFlags", 1);
		Translate::Get("Sfall", "HighlightFail1", "You aren't carrying a motion sensor.", highlightFail1);
		Translate::Get("Sfall", "HighlightFail2", "Your motion sensor is out of charge.", highlightFail2);
		const DWORD objRemoveOutlineAddr[] = {
			0x44BD1C, // gmouse_bk_process_
			0x44E559  // gmouse_remove_item_outline_
		};
		HookCalls(obj_remove_outline_hook, objRemoveOutlineAddr);
	}

	size_t len = IniReader::GetConfigString("Scripts", "IniConfigFolder", "", iniConfigFolder, 64);
	if (len) {
		char c = iniConfigFolder[len - 1];
		bool pathSeparator = (c == '\\' || c == '/');
		if (len > 62 || (len == 62 && !pathSeparator)) {
			iniConfigFolder[0] = '\0';
			dlogr("Error: IniConfigFolder path is too long.", DL_MAIN);
		} else if (!pathSeparator) {
			iniConfigFolder[len++] = '\\';
			iniConfigFolder[len] = '\0';
		}
	}

	alwaysFindScripts = isDebug && (IniReader::GetIntDefaultConfig("Debugging", "AlwaysFindScripts", 0) != 0);
	if (alwaysFindScripts) dlogr("Always searching for global/hook scripts behavior enabled.", DL_SCRIPT);

	MakeJump(0x4A390C, scr_find_sid_from_program_hack);
	MakeJump(0x4A5E34, scr_ptr_hack);

	MakeJump(0x4A67F0, ExecMapScriptsHack);

	HookCall(0x4A26D6, HandleTimedEventScripts); // queue_process_
	const DWORD queueNextTimeAddr[] = {
		0x4C1C67, // wmGameTimeIncrement_
		0x4A3E1C, // script_chk_timed_events_
		0x499AFA, 0x499CD7, 0x499E2B // TimedRest_
	};
	HookCalls(TimedEventNextTime, queueNextTimeAddr);
	HookCall(0x4A3E08, script_chk_timed_events_hook);

	// this patch makes it possible to export variables from sfall global scripts
	HookCall(0x4414C8, exportExportVariable_hook);
	const DWORD exportFindVarAddr[] = {
		0x441285, // exportStoreVariable_
		0x4413D9  // exportFetchVariable_
	};
	HookCalls(Export_FetchOrStore_FindVar_Hook, exportFindVarAddr);

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
	// Tweak for restoring the sneak state when switching between maps
	HookCall(0x4830B6, map_load_file_hook);

	// Set the DAM_BACKWASH flag for the attacker before calling compute_damage_
	SafeWrite32(0x423DE7, 0x40164E80); // or [esi+ctd.flags3Source], DAM_BACKWASH_
	long idata = 0x146E09;             // or dword ptr [esi+ctd.flagsSource], ebp
	SafeWriteBytes(0x423DF0, (BYTE*)&idata, 3);
	if (*(BYTE*)0x423DEB != 0xE8) { // not hook call
		MakeCall(0x423DEB, (void*)fo::funcoffs::compute_damage_);
	}

	Opcodes::InitNew();
	ObjectName::init();
}

}
