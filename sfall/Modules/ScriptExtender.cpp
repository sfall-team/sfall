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

#include <cassert>
#include <set>
#include <string>
#include <unordered_map>

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
#include "Scripting\Handlers\Utils.h"

#include "ScriptExtender.h"

namespace sfall
{

using namespace script;

void _stdcall HandleMapUpdateForScripts(DWORD procId);

// TODO: move to a better place
static char idle;

struct GlobalScript {
	ScriptProgram prog;
	int count;
	int repeat;
	int mode; //0 - local map loop, 1 - input loop, 2 - world map loop, 3 - local and world map loops

	GlobalScript() {}
	GlobalScript(ScriptProgram script) {
		prog = script;
		count = 0;
		repeat = 0;
		mode = 0;
	}
};

struct ExportedVar {
	int type; // in scripting engine terms, eg. VAR_TYPE_*
	int val;
	ExportedVar() : val(0), type(VAR_TYPE_INT) {}
};

static std::vector<fo::Program*> checkedScripts;
static std::vector<GlobalScript> globalScripts;
// a map of all sfall programs (global and hook scripts) by thier scriptPtr
typedef std::unordered_map<fo::Program*, ScriptProgram> SfallProgsMap;
static SfallProgsMap sfallProgsMap;
// a map scriptPtr => self_obj  to override self_obj for all script types using set_self
std::unordered_map<fo::Program*, fo::GameObject*> selfOverrideMap;

typedef std::tr1::unordered_map<std::string, ExportedVar> ExportedVarsMap;
static ExportedVarsMap globalExportedVars;
DWORD isGlobalScriptLoading = 0;
DWORD modifiedIni;

std::unordered_map<__int64, int> globalVars;
typedef std::unordered_map<__int64, int> :: iterator glob_itr;
typedef std::unordered_map<__int64, int> :: const_iterator glob_citr;
typedef std::pair<__int64, int> glob_pair;

DWORD addUnarmedStatToGetYear = 0;
DWORD availableGlobalScriptTypes = 0;
bool isGameLoading;

fo::ScriptInstance overrideScriptStruct;

static const DWORD scr_ptr_back = fo::funcoffs::scr_ptr_ + 5;
static const DWORD scr_find_sid_from_program = fo::funcoffs::scr_find_sid_from_program_ + 5;
static const DWORD scr_find_obj_from_program = fo::funcoffs::scr_find_obj_from_program_ + 7;

DWORD _stdcall FindSidHook2(fo::Program* script) {
	std::unordered_map<fo::Program*, fo::GameObject*>::iterator overrideIt = selfOverrideMap.find(script);
	if (overrideIt != selfOverrideMap.end()) {
		DWORD scriptId = overrideIt->second->scriptId;
		if (scriptId != -1) {
			selfOverrideMap.erase(overrideIt);
			return scriptId; // returns the real scriptId of object if it is scripted
		}
		overrideScriptStruct.selfObject = overrideIt->second;
		overrideScriptStruct.targetObject = overrideIt->second;
		selfOverrideMap.erase(overrideIt); // this reverts self_obj back to original value for next function calls
		return -2; // override struct
	}
	// this will allow to use functions like roll_vs_skill, etc without calling set_self (they don't really need self object)
	if (sfallProgsMap.find(script) != sfallProgsMap.end()) {
		overrideScriptStruct.targetObject = overrideScriptStruct.selfObject = 0;
		return -2; // override struct
	}
	return -1; // change nothing
}

static void __declspec(naked) FindSidHook() {
	__asm {
		sub esp, 4;
		pushad;
		push eax;
		call FindSidHook2;
		mov [esp+32], eax; // to save value after popad
		popad;
		add esp, 4;
		cmp [esp-4], -1;
		jz end;
		cmp [esp-4], -2;
		jz override_script;
		mov eax, [esp-4];
		retn;
override_script:
		test edx, edx;
		jz end;
		lea eax, overrideScriptStruct;
		mov [edx], eax;
		mov eax, -2;
		retn;
end:
		push ebx;
		push ecx;
		push edx;
		push esi;
		push ebp;
		jmp scr_find_sid_from_program;
	}
}

static void __declspec(naked) ScrPtrHook() {
	__asm {
		cmp eax, -2;
		jnz end;
		xor eax, eax;
		retn;
end:
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		jmp scr_ptr_back;
	}
}

static void __declspec(naked) ExecMapScriptsHook() {
	__asm {
		sub esp, 32;
		mov [esp+12], eax;
		pushad;
		push eax; // int procId
		call HandleMapUpdateForScripts;
		popad;
		push 0x4A67F9; // jump back
		retn;
	}
}

static DWORD __stdcall GetGlobalExportedVarPtr(const char* name) {
	std::string str(name);
	ExportedVarsMap::iterator it = globalExportedVars.find(str);
	//dlog_f("\n Trying to find exported var %s... ", DL_MAIN, name);
	if (it != globalExportedVars.end()) {
		ExportedVar *ptr = &it->second;
		return (DWORD)ptr;
	}
	return 0;
}

static DWORD __stdcall CreateGlobalExportedVar(DWORD scr, const char* name) {
	//dlog_f("\nTrying to export variable %s (%d)\n", DL_MAIN, name, isGlobalScriptLoading);
	std::string str(name);
	globalExportedVars[str] = ExportedVar(); // add new
	return 1;
}

/*
	when fetching/storing into exported variables, first search in our own hash table instead, then (if not found), resume normal search

	reason for this: game frees all scripts and exported variables from memory when you exit map
	so if you create exported variable in sfall global script, it will work until you exit map, after that you will get crashes

	with this you can still use variables exported from global scripts even between map changes (per global scripts logic)
*/
static void __declspec(naked) Export_FetchOrStore_FindVar_Hook() {
	__asm {
		push eax;
		push edx;
		push ecx;
		push ebx;
		push edx // char* varName
		call GetGlobalExportedVarPtr;
		test eax, eax
		jz proceedNormal;
		pop ebx;
		pop ecx;
		pop edx;
		pop edx;
		sub eax, 0x24; // adjust pointer for the code
		retn;

	proceedNormal:
		pop ebx;
		pop ecx;
		pop edx;
		pop eax;
		call fo::funcoffs::findVar_
		retn
	}
}

static const DWORD Export_Export_FindVar_back1 = 0x4414CD;
static const DWORD Export_Export_FindVar_back2 = 0x4414AC;
static void __declspec(naked) Export_Export_FindVar_Hook() {
	__asm {
		pushad;
		mov eax, isGlobalScriptLoading;
		test eax, eax;
		jz proceedNormal;
		push edx; // char* - var name
		push ebp; // script ptr
		call CreateGlobalExportedVar;
		popad;
		jmp Export_Export_FindVar_back2; // if sfall exported var, jump to the end of function

proceedNormal:
		popad;
		call fo::funcoffs::findVar_;  // else - proceed normal
		jmp Export_Export_FindVar_back1;
	}
}

// this hook prevents sfall scripts from being removed after switching to another map, since normal script engine re-loads completely
static void _stdcall FreeProgramHook2(fo::Program* progPtr) {
	if (isGameLoading || (sfallProgsMap.find(progPtr) == sfallProgsMap.end())) { // only delete non-sfall scripts or when actually loading the game
		__asm {
			mov eax, progPtr;
			call fo::funcoffs::interpretFreeProgram_;
		}
	}
}

static void __declspec(naked) FreeProgramHook() {
	__asm {
		pushad;
		push eax;
		call FreeProgramHook2;
		popad;
		retn;
	}
}

void _stdcall SetGlobalScriptRepeat(fo::Program* script, int frames) {
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (globalScripts[d].prog.ptr == script) {
			if (frames == -1) {
				globalScripts[d].mode = !globalScripts[d].mode;
			} else {
				globalScripts[d].repeat = frames;
			}
			break;
		}
	}
}

void _stdcall SetGlobalScriptType(fo::Program* script, int type) {
	if (type <= 3) {
		for (size_t d = 0; d < globalScripts.size(); d++) {
			if (globalScripts[d].prog.ptr == script) {
				globalScripts[d].mode = type;
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
			globalVars.erase(itr);    // applies for both float 0.0 and integer 0
		} else {
			itr->second = val;
		}
	}
}

void _stdcall SetGlobalVarInt(DWORD var, int val) {
	SetGlobalVarInternal(var, val);
}

void _stdcall SetGlobalVar(const char* var, int val) {
	if (strlen(var) != 8) {
		return;
	}
	SetGlobalVarInternal(*(__int64*)var, val);
}

static DWORD GetGlobalVarInternal(__int64 val) {
	glob_citr itr = globalVars.find(val);
	if (itr == globalVars.end()) {
		return 0;
	} else {
		return itr->second;
	}
}

DWORD _stdcall GetGlobalVar(const char* var) {
	if (strlen(var) != 8) {
		return 0;
	}
	return GetGlobalVarInternal(*(__int64*)var);
}

DWORD _stdcall GetGlobalVarInt(DWORD var) {
	return GetGlobalVarInternal(var);
}

void _stdcall SetSelfObject(fo::Program* script, fo::GameObject* obj) {
	if (obj) {
		selfOverrideMap[script] = obj;
	} else {
		std::unordered_map<fo::Program*, fo::GameObject*>::iterator it = selfOverrideMap.find(script);
		if (it != selfOverrideMap.end()) {
			selfOverrideMap.erase(it);
		}
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
		for (int i = 0; i < fo::ScriptProc::count; ++i) {
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

void AddProgramToMap(ScriptProgram &prog) {
	sfallProgsMap[prog.ptr] = prog;
}

ScriptProgram* GetGlobalScriptProgram(fo::Program* scriptPtr) {
	for (std::vector<GlobalScript>::iterator it = globalScripts.begin(); it != globalScripts.end(); it++) {
		if (it->prog.ptr == scriptPtr) return &it->prog;
	}
	return nullptr;
}

bool _stdcall IsGameScript(const char* filename) {
	// TODO: write better solution
	for (int i = 0; i < fo::var::maxScriptNum; i++) {
		if (strcmp(filename, fo::var::scriptListInfo[i].fileName) == 0) return true;
	}
	return false;
}

void LoadGLobalScriptsByMask(const std::string& fileMask) {
	char* *filenames;
	auto basePath = fileMask.substr(0, fileMask.find_last_of("\\/") + 1);
	ToLowerCase(basePath);
	int count = fo::func::db_get_file_list(fileMask.c_str(), &filenames);

	// TODO: refactor script programs
	ScriptProgram prog;
	for (int i = 0; i < count; i++) {
		char* name = _strlwr(filenames[i]);
		std::string baseName(name);
		baseName = baseName.substr(0, baseName.find_last_of('.'));
		if (basePath != fo::var::script_path_base || !IsGameScript(baseName.c_str())) {
			dlog(">", DL_SCRIPT);
			std::string fullPath(basePath);
			fullPath += name;
			dlog(fullPath, DL_SCRIPT);
			isGlobalScriptLoading = 1;
			LoadScriptProgram(prog, fullPath.c_str(), true);
			if (prog.ptr) {
				dlogr(" Done", DL_SCRIPT);
				DWORD idx;
				GlobalScript gscript = GlobalScript(prog);
				idx = globalScripts.size();
				globalScripts.push_back(gscript);
				AddProgramToMap(prog);
				// initialize script (start proc will be executed for the first time) -- this needs to be after script is added to "globalScripts" array
				InitScriptProgram(prog);
			} else {
				dlogr(" Error!", DL_SCRIPT);
			}
			isGlobalScriptLoading = 0;
		}
	}
	fo::func::db_free_file_list(&filenames, 0);
}

// this runs after the game was loaded/started
void LoadGlobalScripts() {
	isGameLoading = false;
	LoadHookScripts();
	dlogr("Loading global scripts", DL_SCRIPT|DL_INIT);
	auto maskList = GetConfigList("Scripts", "GlobalScriptPaths", "scripts\\gl*.int", 255);
	for (auto& mask : maskList) {
		LoadGLobalScriptsByMask(mask);
	}
	dlogr("Finished loading global scripts", DL_SCRIPT|DL_INIT);
	//ButtonsReload();
}

bool _stdcall ScriptHasLoaded(fo::Program* script) {
	for (size_t d = 0; d < checkedScripts.size(); d++) {
		if (checkedScripts[d] == script) {
			return false;
		}
	}
	checkedScripts.push_back(script);
	return true;
}

void _stdcall RegAnimCombatCheck(DWORD newValue) {
	char oldValue = regAnimCombatCheck;
	regAnimCombatCheck = (newValue > 0);
	if (oldValue != regAnimCombatCheck) {
		SafeWrite8(0x459C97, regAnimCombatCheck); // reg_anim_func
		SafeWrite8(0x459D4B, regAnimCombatCheck); // reg_anim_animate
		SafeWrite8(0x459E3B, regAnimCombatCheck); // reg_anim_animate_reverse
		SafeWrite8(0x459EEB, regAnimCombatCheck); // reg_anim_obj_move_to_obj
		SafeWrite8(0x459F9F, regAnimCombatCheck); // reg_anim_obj_run_to_obj
		SafeWrite8(0x45A053, regAnimCombatCheck); // reg_anim_obj_move_to_tile
		SafeWrite8(0x45A10B, regAnimCombatCheck); // reg_anim_obj_run_to_tile
		SafeWrite8(0x45AE53, regAnimCombatCheck); // reg_anim_animate_forever
	}
}

// this runs before actually loading/starting the game
void ClearGlobalScripts() {
	isGameLoading = true;
	checkedScripts.clear();
	sfallProgsMap.clear();
	globalScripts.clear();
	selfOverrideMap.clear();
	globalExportedVars.clear();
	HookScriptClear();

	//Reset some settable game values back to the defaults
	//Pyromaniac bonus
	SafeWrite8(0x424AB6, 5);
	//xp mod
	SafeWrite8(0x004AFAB8, 0x53);
	SafeWrite32(0x004AFAB9, 0x55575651);
	//Perk level mod
	SafeWrite32(0x00496880, 0x00019078);
	//HP bonus
	SafeWrite8(0x4AFBC1, 2);
	// TODO: move this elsewhere
	//Bodypart hit chances
	using fo::var::hit_location_penalty;
	hit_location_penalty[0] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Head", -40));
	hit_location_penalty[1] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Left_Arm", -30));
	hit_location_penalty[2] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Right_Arm", -30));
	hit_location_penalty[3] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Torso_Uncalled", 0));
	hit_location_penalty[4] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Right_Leg", -20));
	hit_location_penalty[5] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Left_Leg", -20));
	hit_location_penalty[6] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Eyes", -60));
	hit_location_penalty[7] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Groin", -30));
	hit_location_penalty[8] = static_cast<long>(GetConfigInt("Misc", "BodyHit_Torso_Uncalled", 0));
	//skillpoints per level mod
	SafeWrite8(0x43C27a, 5);
}

void RunScriptProc(ScriptProgram* prog, const char* procName) {
	fo::Program* sptr = prog->ptr;
	int procNum = fo::func::interpretFindProcedure(sptr, procName);
	if (procNum != -1) {
		fo::func::executeProcedure(sptr, procNum);
	}
}

void RunScriptProc(ScriptProgram* prog, long procId) {
	if (procId > 0 && procId < fo::ScriptProc::count) {
		fo::Program* sptr = prog->ptr;
		int procNum = prog->procLookup[procId];
		if (procNum != -1) {
			fo::func::executeProcedure(sptr, procNum);
		}
	}
}

static void RunScript(GlobalScript* script) {
	script->count = 0;
	RunScriptProc(&script->prog, fo::ScriptProc::start); // run "start"
}

/**
	Do some clearing after each frame:
	- delete all temp arrays
	- reset reg_anim_* combatstate checks
*/
static void ResetStateAfterFrame() {
	if (tempArrays.size()) {
 		for (std::set<DWORD>::iterator it = tempArrays.begin(); it != tempArrays.end(); ++it)
			FreeArray(*it);
		tempArrays.clear();
	}
	RegAnimCombatCheck(1);
}

static inline void RunGlobalScripts(int mode1, int mode2) {
	// TODO: move processor idle out?
	if (idle > -1) {
		Sleep(idle);
	}
	for (DWORD d=0; d<globalScripts.size(); d++) {
		if (globalScripts[d].repeat
			&& (globalScripts[d].mode == mode1 || globalScripts[d].mode == mode2)
			&& ++globalScripts[d].count >= globalScripts[d].repeat) {
			RunScript(&globalScripts[d]);
		}
	}
	ResetStateAfterFrame();
}

static void RunGlobalScriptsOnMainLoop() {
	RunGlobalScripts(0, 3);
}

void RunGlobalScriptsOnInput() {
	if (IsMapLoaded()) {
		RunGlobalScripts(1, 1);
	}
}

void RunGlobalScriptsOnWorldMap() {
	RunGlobalScripts(2, 3);
}

void _stdcall HandleMapUpdateForScripts(DWORD procId) {
	if (procId == fo::ScriptProc::map_enter_p_proc) {
		// map changed, all game objects were destroyed and scripts detached, need to re-insert global scripts into the game
		for (SfallProgsMap::iterator it = sfallProgsMap.begin(); it != sfallProgsMap.end(); it++) {
			fo::func::runProgram(it->second.ptr);
		}
	}
	RunGlobalScriptsAtProc(procId); // gl* scripts of types 0 and 3
	RunHookScriptsAtProc(procId); // all hs_ scripts
}

// run all global scripts of types 0 and 3 at specific procedure (if exist)
void _stdcall RunGlobalScriptsAtProc(DWORD procId) {
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (globalScripts[d].mode != 0 && globalScripts[d].mode != 3) continue;
		RunScriptProc(&globalScripts[d].prog, procId);
	}
}

void LoadGlobals(HANDLE h) {
	DWORD count, unused;
	ReadFile(h, &count, 4, &unused, 0);
	if (unused!=4) return;
	GlobalVar var;
	for (DWORD i = 0; i<count; i++) {
		ReadFile(h, &var, sizeof(GlobalVar), &unused, 0);
		globalVars.insert(glob_pair(var.id, var.val));
	}
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
		itr++;
	}
}

void ClearGlobals() {
	globalVars.clear();
	for (array_itr it = arrays.begin(); it != arrays.end(); ++it) {
		it->second.clear();
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
		itr++;
	}
}

void SetGlobals(GlobalVar* globals) {
	glob_itr itr = globalVars.begin();
	int i = 0;
	while(itr != globalVars.end()) {
		itr->second = globals[i++].val;
		itr++;
	}
}

void ScriptExtender::init() {
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

	// TODO: move out?
	idle = GetConfigInt("Misc", "ProcessorIdle", -1);
	if (idle > -1) {
		fo::var::idle_func = reinterpret_cast<DWORD>(Sleep);
		SafeWrite8(0x4C9F12, 0x6A); // push
		SafeWrite8(0x4C9F13, idle);
	}
	modifiedIni = GetConfigInt("Main", "ModifiedIni", 0);
	
	arraysBehavior = GetConfigInt("Misc", "arraysBehavior", 1);
	if (arraysBehavior > 0) {
		arraysBehavior = 1; // only 1 and 0 allowed at this time
		dlogr("New arrays behavior enabled.", DL_SCRIPT);
	} else {
		dlogr("Arrays in backward-compatiblity mode.", DL_SCRIPT);
	}

	MakeJump(0x4A390C, FindSidHook);
	MakeJump(0x4A5E34, ScrPtrHook);
	memset(&overrideScriptStruct, 0, sizeof(fo::ScriptInstance));

	MakeJump(0x4A67F2, ExecMapScriptsHook);

	// this patch makes it possible to export variables from sfall global scripts
	MakeJump(0x4414C8, Export_Export_FindVar_Hook);
	HookCall(0x441285, &Export_FetchOrStore_FindVar_Hook); // store
	HookCall(0x4413D9, &Export_FetchOrStore_FindVar_Hook); // fetch

	// fix vanilla negate operator on float values
	MakeJump(0x46AB63, NegateFixHook);
	// fix incorrect int-to-float conversion
	// op_mult:
	SafeWrite16(0x46A3F4, 0x04DB); // replace operator to "fild 32bit"
	SafeWrite16(0x46A3A8, 0x04DB);
	// op_div:
	SafeWrite16(0x46A566, 0x04DB);
	SafeWrite16(0x46A4E7, 0x04DB);

	HookCall(0x46E141, FreeProgramHook);
	
	InitNewOpcodes();
}

}
