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

#include "main.h"
#include "FalloutEngine.h"
#include "Arrays.h"
#include "BarBoxes.h"
#include "Console.h"
#include "Explosions.h"
#include "HookScripts.h"
#include "InputFuncs.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "ScriptExtender.h"
#include "version.h"

#include "ScriptOps\AsmMacros.h"

static DWORD __stdcall HandleMapUpdateForScripts(const DWORD procId);

static void RunGlobalScripts1();
static void ClearEventsOnMapExit();

static char gTextBuffer[5120]; // used as global temp text buffer for script functions

// returns the size of the global text buffer
inline static const long GlblTextBufferSize() { return sizeof(gTextBuffer); }

// variables for new opcodes
#define OP_MAX_ARGUMENTS	(8)

// masks for argument validation
#define DATATYPE_MASK_INT		(1 << DATATYPE_INT)
#define DATATYPE_MASK_FLOAT		(1 << DATATYPE_FLOAT)
#define DATATYPE_MASK_STR		(1 << DATATYPE_STR)
#define DATATYPE_MASK_NOT_NULL	(0x00010000)
#define DATATYPE_MASK_VALID_OBJ	(DATATYPE_MASK_INT | DATATYPE_MASK_NOT_NULL)

struct SfallOpcodeMetadata {
	// opcode handler, will be used as key
	void (*handler)();

	// opcode name, only used for logging
	const char* name;

	// argument validation masks
	int argTypeMasks[OP_MAX_ARGUMENTS];
};

typedef std::tr1::unordered_map<void(*)(), const SfallOpcodeMetadata*> OpcodeMetaTableType;

static OpcodeMetaTableType opcodeMetaTable;

class ScriptValue {
public:
	ScriptValue(SfallDataType type, DWORD value) {
		_val.dw = value;
		_type = type;
	}

	ScriptValue() {
		_val.dw = 0;
		_type = DATATYPE_NONE;
	}

	ScriptValue(const char* strval) {
		_val.str = strval;
		_type = DATATYPE_STR;
	}

	// int, long and unsigned long behave identically
	ScriptValue(int val) {
		_val.i = val;
		_type = DATATYPE_INT;
	}

	ScriptValue(long val) {
		_val.i = val;
		_type = DATATYPE_INT;
	}

	ScriptValue(unsigned long val) {
		_val.i = val;
		_type = DATATYPE_INT;
	}

	ScriptValue(float strval) {
		_val.f = strval;
		_type = DATATYPE_FLOAT;
	}

	ScriptValue(TGameObj* obj) {
		_val.gObj = obj;
		_type = DATATYPE_INT;
	}

	bool isInt() const {
		return _type == DATATYPE_INT;
	}

	bool isFloat() const {
		return _type == DATATYPE_FLOAT;
	}

	bool isString() const {
		return _type == DATATYPE_STR;
	}

	int asInt() const {
		switch (_type) {
		case DATATYPE_INT:
			return _val.i;
		case DATATYPE_FLOAT:
			return static_cast<int>(_val.f);
		default:
			return 0;
		}
	}

	bool asBool() const {
		switch (_type) {
		case DATATYPE_INT:
			return _val.i != 0;
		case DATATYPE_FLOAT:
			return static_cast<int>(_val.f) != 0;
		default:
			return true;
		}
	}

	float asFloat() const {
		switch (_type) {
		case DATATYPE_FLOAT:
			return _val.f;
		case DATATYPE_INT:
			return static_cast<float>(_val.i);
		default:
			return 0.0;
		}
	}

	const char* asString() const {
		return (_type == DATATYPE_STR)
			? _val.str
			: "";
	}

	TGameObj* asObject() const {
		return (_type == DATATYPE_INT)
			? _val.gObj
			: nullptr;
	}

	TGameObj* object() const {
		return _val.gObj;
	}

	unsigned long rawValue() const {
		return _val.dw;
	}

	const char* strValue() const {
		return _val.str;
	}

	float floatValue() const {
		return _val.f;
	}

	SfallDataType type() const {
		return _type;
	}

private:
	union Value {
		DWORD dw;
		int i;
		float f;
		const char* str;
		TGameObj* gObj;
	} _val;

	SfallDataType _type; // TODO: replace with enum class
};

typedef struct {
	union Value {
		DWORD dw;
		int i;
		float f;
		const char* str;
		TGameObj* gObj;
	} val;
	DWORD type; // TODO: replace with enum class
} ScriptValue1;

class OpcodeHandler {
public:
	OpcodeHandler() {
		_argShift = 0;
		_args.reserve(OP_MAX_ARGUMENTS);
	}

	// number of arguments, possibly reduced by argShift
	int numArgs() const {
		return _args.size() - _argShift;
	}

	// returns current argument shift, default is 0
	int argShift() const {
		return _argShift;
	}

	// sets shift value for arguments
	// for example if shift value is 2, then subsequent calls to arg(i) will return arg(i+2) instead, etc.
	void setArgShift(int shift) {
		assert(shift >= 0);

		_argShift = shift;
	}

	// returns argument with given index, possible shifted by argShift
	const ScriptValue& arg(int index) const {
		assert((index + _argShift) < OP_MAX_ARGUMENTS);
		return _args[index + _argShift];
	}

	// current return value
	const ScriptValue& returnValue() const {
		return _ret;
	}

	// current script program
	TProgram* program() const {
		return _program;
	}

	// set return value for current opcode
	void setReturn(DWORD value, SfallDataType type) {
		_ret = ScriptValue(type, value);
	}

	// set return value for current opcode
	void setReturn(const ScriptValue& val) {
		_ret = val;
	}

	// resets the state of handler for new opcode invocation
	void resetState(TProgram* program, int argNum) {
		_program = program;

		// reset return value
		_ret = ScriptValue();
		// reset argument list
		_args.resize(argNum);
		// reset arg shift
		_argShift = 0;
	}

	// writes error message to debug.log along with the name of script & procedure
	void printOpcodeError(const char* fmt, ...) const {
		assert(_program != nullptr);

		va_list args;
		va_start(args, fmt);
		char msg[1024];
		vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, args);
		va_end(args);

		const char* procName = FindCurrentProc(_program);
		DebugPrintf("\nOPCODE ERROR: %s\n > Script: %s, procedure %s.", msg, _program->fileName, procName);
	}

	// Validate opcode arguments against type masks
	// if validation pass, returns true, otherwise writes error to debug.log and returns false
	bool validateArguments(const int argTypeMasks[], int argCount, const char* opcodeName) const {
		for (int i = 0; i < argCount; i++) {
			int typeMask = argTypeMasks[i];
			const ScriptValue &argI = arg(i);
			if (typeMask != 0 && ((1 << argI.type()) & typeMask) == 0) {
				printOpcodeError(
					"%s() - argument #%d has invalid type: %s.",
					opcodeName,
					++i,
					GetSfallTypeName(argI.type()));

				return false;
			} else if ((typeMask & DATATYPE_MASK_NOT_NULL) && argI.rawValue() == 0) {
				printOpcodeError(
					"%s() - argument #%d is null.",
					opcodeName,
					++i);

				return false;
			}
		}
		return true;
	}

	// Handle opcodes
	// scriptPtr - pointer to script program (from the engine)
	// func - opcode handler
	// hasReturn - true if opcode has return value (is expression)
	void __thiscall handleOpcode(TProgram* program, void(*func)(), int argNum, bool hasReturn) {
		assert(argNum < OP_MAX_ARGUMENTS);

		// reset state after previous
		resetState(program, argNum);

		// process arguments on stack (reverse order)
		for (int i = argNum - 1; i >= 0; i--) {
			// get argument from stack
			DWORD rawValueType;
			DWORD rawValue = InterpretGetValue(program, rawValueType);
			_args[i] = ScriptValue(static_cast<SfallDataType>(getSfallTypeByScriptType(rawValueType)), rawValue);
		}
		// flag that arguments passed are valid
		bool argumentsValid = true;

		// check if metadata is available
		OpcodeMetaTableType::iterator it = opcodeMetaTable.find(func);
		if (it != opcodeMetaTable.end()) {
			const SfallOpcodeMetadata* meta = it->second;

			// automatically validate argument types
			argumentsValid = validateArguments(meta->argTypeMasks, argNum, meta->name);
		}

		// call opcode handler if arguments are valid (or no automatic validation was done)
		if (argumentsValid) {
			func();
		}

		// process return value
		if (hasReturn) {
			if (_ret.type() == DATATYPE_NONE) {
				_ret = ScriptValue(0); // if no value was set in handler, force return 0 to avoid stack error
			}
			InterpretReturnValue(program, _ret.rawValue(), getScriptTypeBySfallType(_ret.type()));
		}
	}

private:
	TProgram* _program;

	int _argShift;
	std::vector<ScriptValue> _args;
	ScriptValue _ret;
};

static OpcodeHandler opHandler;

static void OpcodeInvalidArgs(const char* opcodeName) {
	opHandler.printOpcodeError("%s() - invalid arguments.", opcodeName);
}

static DWORD highlightingToggled = 0;
static DWORD motionScanner;
static BYTE toggleHighlightsKey;
static DWORD highlightContainers = 0;
static DWORD highlightCorpses = 0;
static int outlineColor = 0x10;
static int idle;
static char HighlightFail1[128];
static char HighlightFail2[128];

struct sGlobalScript {
	sScriptProgram prog;
	int startProc; // position of the 'start' procedure in the script
	int count;
	int repeat;
	int mode; // 0 - local map loop, 1 - input loop, 2 - world map loop, 3 - local and world map loops

	//sGlobalScript() {}
	sGlobalScript(sScriptProgram script) : prog(script), startProc(-1), count(0), repeat(0), mode(0) {}
};

struct sExportedVar {
	int type; // in scripting engine terms, eg. VAR_TYPE_*
	int val;
	sExportedVar() : val(0), type(VAR_TYPE_INT) {}
};

struct SelfOverrideObj {
	TGameObj* object;
	char counter;

	bool UnSetSelf() {
		if (counter) counter--;
		return counter == 0;
	}
};

struct TimedEvent {
	sScriptProgram* script;
	unsigned long time;
	long fixed_param;

	bool operator() (const TimedEvent &a, const TimedEvent &b) {
		return a.time < b.time;
	}
} *timedEvent = nullptr;

static std::list<TimedEvent> timerEventScripts;

static std::vector<DWORD> checkedScripts;
static std::vector<sGlobalScript> globalScripts;

// a map of all sfall programs (global and hook scripts) by thier scriptPtr
typedef std::tr1::unordered_map<DWORD, sScriptProgram> SfallProgsMap;
static SfallProgsMap sfallProgsMap;

// a map scriptPtr => self_obj  to override self_obj for all script types using set_self
std::tr1::unordered_map<DWORD, SelfOverrideObj> selfOverrideMap;

typedef std::tr1::unordered_map<std::string, sExportedVar> ExportedVarsMap;
static ExportedVarsMap globalExportedVars;
DWORD isGlobalScriptLoading = 0;

std::tr1::unordered_map<__int64, int> globalVars;
typedef std::tr1::unordered_map<__int64, int>::iterator glob_itr;
typedef std::tr1::unordered_map<__int64, int>::const_iterator glob_citr;
typedef std::pair<__int64, int> glob_pair;

static void* opcodes[0x300];
DWORD availableGlobalScriptTypes = 0;
bool isGameLoading;

TScript OverrideScriptStruct = {0};

//eax contains the script pointer, edx contains the opcode*4

//To read a value, mov the script pointer to eax, call interpretPopShort_, eax now contains the value type
//mov the script pointer to eax, call interpretPopLong_, eax now contains the value

//To return a value, move it to edx, mov the script pointer to eax, call interpretPushLong_
//mov the value type to edx, mov the script pointer to eax, call interpretPushShort_


static void __fastcall SetGlobalScriptRepeat(DWORD script, int frames) {
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

static void __declspec(naked) op_set_global_script_repeat() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		_GET_ARG_INT(end);
		mov  edx, eax;              // frames
		call SetGlobalScriptRepeat; // ecx - script
end:
		mov  ecx, esi;
		retn;
	}
}

static void __fastcall SetGlobalScriptType(DWORD script, int type) {
	if (type <= 3) {
		for (size_t i = 0; i < globalScripts.size(); i++) {
			if (globalScripts[i].prog.ptr == script) {
				globalScripts[i].mode = type;
				break;
			}
		}
	}
}

static void __declspec(naked) op_set_global_script_type() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		_GET_ARG_INT(end);
		mov  edx, eax;            // type
		call SetGlobalScriptType; // ecx - script
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_available_global_script_types() {
	__asm {
		mov  edx, availableGlobalScriptTypes;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
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

static void __stdcall op_set_sfall_global2() {
	const ScriptValue &varArg = opHandler.arg(0),
					  &valArg = opHandler.arg(1);

	if (!varArg.isFloat() && !valArg.isString()) {
		if (varArg.isString()) {
			if (SetGlobalVar(varArg.strValue(), valArg.rawValue())) {
				opHandler.printOpcodeError("set_sfall_global() - the name of the global variable must consist of 8 characters.");
			}
		} else {
			SetGlobalVarInt(varArg.rawValue(), valArg.rawValue());
		}
	} else {
		OpcodeInvalidArgs("set_sfall_global");
	}
}

static void __declspec(naked) op_set_sfall_global() {
	_WRAP_OPCODE(op_set_sfall_global2, 2, 0)
}

static long GetGlobalVarInternal(__int64 val) {
	glob_citr itr = globalVars.find(val);
	return (itr != globalVars.end()) ? itr->second : 0;
}

long GetGlobalVar(const char* var) {
	return (strlen(var) == 8) ? GetGlobalVarInternal(*(__int64*)var) : 0;
}

long GetGlobalVarInt(DWORD var) {
	return GetGlobalVarInternal(static_cast<__int64>(var));
}

static void __stdcall op_get_sfall_global_int2() {
	const ScriptValue &varArg = opHandler.arg(0);
	long result = 0;

	if (!varArg.isFloat()) {
		if (varArg.isString()) {
			const char* var = varArg.strValue();
			if (strlen(var) != 8) {
				opHandler.printOpcodeError("get_sfall_global_int() - the name of the global variable must consist of 8 characters.");
			} else {
				result = GetGlobalVarInternal(*(__int64*)var);
			}
		} else {
			result = GetGlobalVarInt(varArg.rawValue());
		}
		opHandler.setReturn(result, DATATYPE_INT);
	} else {
		opHandler.printOpcodeError("get_sfall_global_int() - argument is not an integer or a string.");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_sfall_global_int() {
	_WRAP_OPCODE(op_get_sfall_global_int2, 1, 1)
}

static void __stdcall op_get_sfall_global_float2() {
	const ScriptValue &varArg = opHandler.arg(0);
	long result = 0;

	if (!varArg.isFloat()) {
		if (varArg.isString()) {
			const char* var = varArg.strValue();
			if (strlen(var) != 8) {
				opHandler.printOpcodeError("get_sfall_global_float() - The name of the global variable must consist of 8 characters.");
			} else {
				result = GetGlobalVarInternal(*(__int64*)var);
			}
		} else {
			result = GetGlobalVarInt(varArg.rawValue());
		}
		opHandler.setReturn(result, DATATYPE_FLOAT);
	} else {
		opHandler.printOpcodeError("get_sfall_global_float() - argument is not an integer or a string.");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) op_get_sfall_global_float() {
	_WRAP_OPCODE(op_get_sfall_global_float2, 1, 1)
}

static void __declspec(naked) op_get_sfall_arg() {
	__asm {
		mov  esi, ecx;
		call GetHSArg;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void mf_get_sfall_arg_at() {
	long argVal = 0;
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		long id = opHandler.arg(0).rawValue();
		if (id >= static_cast<long>(GetHSArgCount()) || id < 0) {
			opHandler.printOpcodeError("get_sfall_arg_at() - invalid value for argument.");
		} else {
			argVal = GetHSArgAt(id);
		}
	} else {
		OpcodeInvalidArgs("get_sfall_arg_at");
	}
	opHandler.setReturn(argVal);
}

static DWORD __stdcall GetSfallArgs() {
	DWORD argCount = GetHSArgCount();
	DWORD id = TempArray(argCount, 0);
	DWORD* args = GetHSArgs();
	for (DWORD i = 0; i < argCount; i++) {
		arrays[id].val[i].set(*(long*)&args[i]);
	}
	return id;
}

static void __declspec(naked) op_get_sfall_args() {
	__asm {
		mov  esi, ecx;
		call GetSfallArgs;
		mov  edx, eax;
		mov  eax, ebx;
		_RET_VAL_INT;
		mov  ecx, esi;
		retn;
	}
}

static void __stdcall op_set_sfall_arg2() {
	const ScriptValue &argNumArg = opHandler.arg(0),
					  &valArg = opHandler.arg(1);

	if (argNumArg.isInt() && valArg.isInt()) {
		SetHSArg(argNumArg.rawValue(), valArg.rawValue());
	} else {
		OpcodeInvalidArgs("set_sfall_arg");
	}
}

static void __declspec(naked) op_set_sfall_arg() {
	_WRAP_OPCODE(op_set_sfall_arg2, 2, 0)
}

static void __declspec(naked) op_set_sfall_return() {
	__asm {
		mov  esi, ecx;
		_GET_ARG_INT(end);
		push eax;
		call SetHSReturn;
end:
		mov  ecx, esi;
		retn;
	}
}

static void __declspec(naked) op_init_hook() {
	__asm {
		mov  edx, initingHookScripts;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
//		retn;
	}
}

static void __fastcall SetSelfObject(DWORD script, TGameObj* obj) {
	std::tr1::unordered_map<DWORD, SelfOverrideObj>::iterator it = selfOverrideMap.find(script);
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
			SelfOverrideObj self;
			self.object = obj;
			self.counter = 0;
			selfOverrideMap[script] = self;
		}
	} else if (isFind) {
		selfOverrideMap.erase(it);
	}
}

static void __declspec(naked) op_set_self() {
	__asm {
		mov  esi, ecx;
		mov  ecx, eax;
		_GET_ARG_INT(end);
		mov  edx, eax;      // object
		call SetSelfObject; // ecx - script
end:
		mov  ecx, esi;
		retn;
	}
}

static void __stdcall op_register_hook2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		RegisterHook((DWORD)opHandler.program(), idArg.rawValue(), -1, false);
	} else {
		OpcodeInvalidArgs("register_hook");
	}
}

static void __declspec(naked) op_register_hook() {
	_WRAP_OPCODE(op_register_hook2, 1, 0)
}

static void __stdcall op_register_hook_proc2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &procArg = opHandler.arg(1);

	if (idArg.isInt() && procArg.isInt()) {
		RegisterHook((DWORD)opHandler.program(), idArg.rawValue(), procArg.rawValue(), false);
	} else {
		OpcodeInvalidArgs("register_hook_proc");
	}
}

static void __declspec(naked) op_register_hook_proc() {
	_WRAP_OPCODE(op_register_hook_proc2, 2, 0)
}

static void __stdcall op_register_hook_proc_spec2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &procArg = opHandler.arg(1);

	if (idArg.isInt() && procArg.isInt()) {
		RegisterHook((DWORD)opHandler.program(), idArg.rawValue(), procArg.rawValue(), true);
	} else {
		OpcodeInvalidArgs("register_hook_proc_spec");
	}
}

static void __declspec(naked) op_register_hook_proc_spec() {
	_WRAP_OPCODE(op_register_hook_proc_spec2, 2, 0)
}

static void mf_add_g_timer_event() {
	AddTimerEventScripts((DWORD)opHandler.program(), opHandler.arg(0).rawValue(), opHandler.arg(1).rawValue());
}

static void mf_remove_timer_event() {
	if (opHandler.numArgs() > 0) {
		RemoveTimerEventScripts((DWORD)opHandler.program(), opHandler.arg(0).rawValue());
	} else {
		RemoveTimerEventScripts((DWORD)opHandler.program()); // remove all
	}
}

static void __declspec(naked) op_sfall_ver_major() {
	__asm {
		mov  edx, VERSION_MAJOR;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_sfall_ver_minor() {
	__asm {
		mov  edx, VERSION_MINOR;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

static void __declspec(naked) op_sfall_ver_build() {
	__asm {
		mov  edx, VERSION_BUILD;
		_J_RET_VAL_TYPE(VAR_TYPE_INT);
	}
}

#include "ScriptOps\ScriptArrays.hpp"
#include "ScriptOps\ScriptUtils.hpp"

#include "ScriptOps\AnimOps.hpp"
#include "ScriptOps\FileSystemOps.hpp"
#include "ScriptOps\GraphicsOps.hpp"
#include "ScriptOps\InterfaceOps.hpp"
#include "ScriptOps\MathOps.hpp"
#include "ScriptOps\MemoryOps.hpp"
#include "ScriptOps\MiscOps.hpp"
#include "ScriptOps\ObjectsOps.hpp"
#include "ScriptOps\PerksOps.hpp"
#include "ScriptOps\StatsOps.hpp"
#include "ScriptOps\WorldmapOps.hpp"
#include "ScriptOps\MetaruleOps.hpp"

/*
	Array for opcodes metadata.

	This is completely optional, added for convenience only.

	By adding opcode to this array, Sfall will automatically validate it's arguments using provided info.
	On fail, errors will be printed to debug.log and opcode will not be executed.
	If you don't include opcode in this array, you should take care of all argument validation inside handler itself.
*/
static const SfallOpcodeMetadata opcodeMetaArray[] = {
	{mf_add_g_timer_event,      "add_g_timer_event",      {DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_add_trait,              "add_trait",              {DATATYPE_MASK_INT}},
	{mf_create_win,             "create_win",             {DATATYPE_MASK_STR, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_draw_image,             "draw_image",             {DATATYPE_MASK_INT | DATATYPE_MASK_STR, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_draw_image_scaled,      "draw_image_scaled",      {DATATYPE_MASK_INT | DATATYPE_MASK_STR, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_get_window_attribute,   "get_window_attribute",   {DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_hide_window,            "hide_window",            {DATATYPE_MASK_STR}},
	{mf_inventory_redraw,       "inventory_redraw",       {DATATYPE_MASK_INT}},
	{mf_message_box,            "message_box",            {DATATYPE_MASK_STR, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_remove_timer_event,     "remove_timer_event",     {DATATYPE_MASK_INT}},
	{mf_set_cursor_mode,        "set_cursor_mode",        {DATATYPE_MASK_INT}},
	{mf_set_flags,              "set_flags",              {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{mf_set_iface_tag_text,     "set_iface_tag_text",     {DATATYPE_MASK_INT, DATATYPE_MASK_STR, DATATYPE_MASK_INT}},
	{mf_set_ini_setting,        "set_ini_setting",        {DATATYPE_MASK_STR, DATATYPE_MASK_INT | DATATYPE_MASK_STR}},
	{mf_set_map_enter_position, "set_map_enter_position", {DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_set_object_data,        "set_object_data",        {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_set_outline,            "set_outline",            {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{mf_set_terrain_name,       "set_terrain_name",       {DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_STR}},
	{mf_set_town_title,         "set_town_title",         {DATATYPE_MASK_INT, DATATYPE_MASK_STR}},
	{mf_set_unique_id,          "set_unique_id",          {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{mf_set_unjam_locks_time,   "set_unjam_locks_time",   {DATATYPE_MASK_INT}},
	{mf_set_window_flag,        "set_window_flag",        {DATATYPE_MASK_INT | DATATYPE_MASK_STR, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_show_window,            "show_window",            {DATATYPE_MASK_STR}},
	{mf_string_to_case,         "string_to_case",         {DATATYPE_MASK_STR, DATATYPE_MASK_INT}},
	{mf_tile_by_position,       "tile_by_position",       {DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{mf_unjam_lock,             "unjam_lock",             {DATATYPE_MASK_VALID_OBJ}},
	{mf_unwield_slot,           "unwield_slot",           {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	#ifndef NDEBUG
	{mf_test,                   "validate_test",          {DATATYPE_MASK_INT, DATATYPE_MASK_INT | DATATYPE_MASK_FLOAT, DATATYPE_MASK_STR, DATATYPE_NONE}},
	#endif
	//{op_message_str_game, {}}
};

static void InitOpcodeMetaTable() {
	int length = sizeof(opcodeMetaArray) / sizeof(SfallOpcodeMetadata);
	for (int i = 0; i < length; ++i) {
		opcodeMetaTable[opcodeMetaArray[i].handler] = &opcodeMetaArray[i];
	}
}

//END OF SCRIPT FUNCTIONS

long GetScriptReturnValue() {
	return OverrideScriptStruct.return_value;
}

long GetResetScriptReturnValue() {
	long val = GetScriptReturnValue();
	OverrideScriptStruct.return_value = 0;
	return val;
}

static DWORD __stdcall FindSid(DWORD script) {
	std::tr1::unordered_map<DWORD, SelfOverrideObj>::iterator overrideIt = selfOverrideMap.find(script);
	if (overrideIt != selfOverrideMap.end()) {
		DWORD scriptId = overrideIt->second.object->scriptID; // script
		OverrideScriptStruct.script_id = scriptId;
		if (scriptId != -1) {
			if (overrideIt->second.UnSetSelf()) selfOverrideMap.erase(overrideIt);
			return scriptId; // returns the real scriptId of object if it is scripted
		}
		OverrideScriptStruct.self_obj = overrideIt->second.object;
		OverrideScriptStruct.target_obj = overrideIt->second.object;
		if (overrideIt->second.UnSetSelf()) selfOverrideMap.erase(overrideIt); // this reverts self_obj back to original value for next function calls
		return -2; // override struct
	}
	// this will allow to use functions like roll_vs_skill, etc without calling set_self (they don't really need self object)
	if (sfallProgsMap.find(script) != sfallProgsMap.end()) {
		if (timedEvent && timedEvent->script->ptr == script) {
			OverrideScriptStruct.fixed_param = timedEvent->fixed_param;
		} else {
			OverrideScriptStruct.fixed_param = 0;
		}
		OverrideScriptStruct.target_obj = 0;
		OverrideScriptStruct.self_obj = 0;
		OverrideScriptStruct.return_value = 0;
		return -2; // override struct
	}
	return -1; // change nothing
}

static const DWORD scr_ptr_back = scr_ptr_ + 5;
static const DWORD scr_find_sid_from_program = scr_find_sid_from_program_ + 5;
//static const DWORD scr_find_obj_from_program = scr_find_obj_from_program_ + 7;

static void __declspec(naked) FindSidHack() {
	__asm {
		push eax;
		push edx;
		push ecx;
		push eax;
		call FindSid;
		pop  ecx;
		pop  edx;
		cmp  eax, -1;  // eax = scriptId
		jz   end;
		cmp  eax, -2;
		jz   override_script;
		add  esp, 4;
		retn;
override_script:
		test edx, edx;
		jz   skip;
		add  esp, 4;
		lea  eax, OverrideScriptStruct;
		mov  [edx], eax;
		mov  eax, -2;
		retn;
skip:
		add  esp, 4;
		dec  eax; // set -3;
		retn;
end:
		pop  eax;
		push ebx;
		push ecx;
		push edx;
		push esi;
		push ebp;
		jmp  scr_find_sid_from_program;
	}
}

static void __declspec(naked) ScrPtrHack() {
	__asm {
		cmp  eax, -2;
		jnz  skip;
		xor  eax, eax;
		retn;
skip:
		cmp  eax, -3;
		jne  end;
		lea  eax, OverrideScriptStruct;
		mov  [edx], eax;
		mov  esi, [eax]; // script.id
		xor  eax, eax;
		retn;
end:
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		jmp  scr_ptr_back;
	}
}

static void __declspec(naked) MainGameLoopHook() {
	__asm {
		call get_input_;
		push ecx;
		push edx;
		push eax;
		call RunGlobalScripts1;
		pop  eax;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) CombatLoopHook() {
	__asm {
		push ecx;
		push edx;
		//push eax;
		call RunGlobalScripts1;
		//pop  eax;
		pop  edx;
		call get_input_;
		pop  ecx; // fix to prevent the combat turn from being skipped after using Alt+Tab
		retn;
	}
}

static void __declspec(naked) AfterCombatAttackHook() {
	__asm {
		push ecx;
		push edx;
		call AfterAttackCleanup;
		pop  edx;
		pop  ecx;
		mov  eax, 1;
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

static sExportedVar* __fastcall GetGlobalExportedVarPtr(const char* name) {
	std::string str(name);
	ExportedVarsMap::iterator it = globalExportedVars.find(str);
	//dlog_f("\n Trying to find exported var %s... ", DL_MAIN, name);
	if (it != globalExportedVars.end()) {
		sExportedVar *ptr = &it->second;
		return ptr;
	}
	return nullptr;
}

static void __stdcall CreateGlobalExportedVar(DWORD scr, const char* name) {
	//dlog_f("\nTrying to export variable %s (%d)\n", DL_MAIN, name, isGlobalScriptLoading);
	std::string str(name);
	globalExportedVars[str] = sExportedVar(); // add new
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
		jmp  findVar_
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
		jmp  findVar_;                    // else - proceed normal
	}
}

// this hook prevents sfall scripts from being removed after switching to another map, since normal script engine re-loads completely
static void __stdcall FreeProgram(DWORD progPtr) {
	if (isGameLoading || (sfallProgsMap.find(progPtr) == sfallProgsMap.end())) { // only delete non-sfall scripts or when actually loading the game
		__asm {
			mov  eax, progPtr;
			call interpretFreeProgram_;
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
	__asm {
		push eax;
		call scr_set_ext_param_;
		pop  eax;                                 // pobj.sid
		mov  edx, combat_is_starting_p_proc;
		jmp  exec_script_proc_;
	}
}

static void __declspec(naked) CombatOverHook() {
	__asm {
		push eax;
		call scr_set_ext_param_;
		pop  eax;                                 // pobj.sid
		mov  edx, combat_is_over_p_proc;
		jmp  exec_script_proc_;
	}
}

static void __declspec(naked) obj_outline_all_items_on() {
	__asm {
		pushadc;
		mov  eax, ds:[_map_elevation];
		call obj_find_first_at_;
loopObject:
		test eax, eax;
		jz   end;
		cmp  eax, ds:[_outlined_object];
		je   nextObject;
		xchg ecx, eax;
		mov  eax, [ecx + 0x20];
		and  eax, 0xF000000;
		sar  eax, 0x18;
		test eax, eax;                            // Is this an item?
		jz   skip;                                // Yes
		dec  eax;                                 // Is this a critter?
		jnz  nextObject;                          // No
		cmp  highlightCorpses, eax;               // Highlight corpses?
		je   nextObject;                          // No
		test byte ptr [ecx + 0x44], DAM_DEAD;     // source.results & DAM_DEAD?
		jz   nextObject;                          // No
		mov  edx, 0x20;                           // _Steal flag
		mov  eax, [ecx + 0x64];                   // eax = source.pid
		call critter_flag_check_;
		test eax, eax;                            // Can't be stolen from?
		jnz  nextObject;                          // Yes
skip:
		cmp  [ecx + 0x7C], eax;                   // Owned by someone?
		jnz  nextObject;                          // Yes
		test [ecx + 0x74], eax;                   // Already outlined?
		jnz  nextObject;                          // Yes
		test byte ptr [ecx + 0x25], 0x10;         // Is NoHighlight_ flag set (is this a container)?
		jz   NoHighlight;                         // No
		cmp  highlightContainers, eax;            // Highlight containers?
		je   nextObject;                          // No
NoHighlight:
		mov  edx, outlineColor;
		mov  [ecx + 0x74], edx;
nextObject:
		call obj_find_next_at_;
		jmp  loopObject;
end:
		call tile_refresh_display_;
		popadc;
		retn;
	}
}

static void __declspec(naked) obj_outline_all_items_off() {
	__asm {
		pushadc;
		mov  eax, ds:[_map_elevation];
		call obj_find_first_at_;
loopObject:
		test eax, eax;
		jz   end;
		cmp  eax, ds:[_outlined_object];
		je   nextObject;
		xchg ecx, eax;
		mov  eax, [ecx + 0x20];
		and  eax, 0xF000000;
		sar  eax, 0x18;
		test eax, eax;                            // Is this an item?
		jz   skip;                                // Yes
		dec  eax;                                 // Is this a critter?
		jnz  nextObject;                          // No
		test byte ptr [ecx + 0x44], DAM_DEAD;     // source.results & DAM_DEAD?
		jz   nextObject;                          // No
skip:
		cmp  [ecx + 0x7C], eax;                   // Owned by someone?
		jnz  nextObject;                          // Yes
		mov  [ecx + 0x74], eax;
nextObject:
		call obj_find_next_at_;
		jmp  loopObject;
end:
		call tile_refresh_display_;
		popadc;
		retn;
	}
}

static void __declspec(naked) obj_remove_outline_hook() {
	__asm {
		call obj_remove_outline_;
		test eax, eax;
		jnz  end;
		cmp  highlightingToggled, 1;
		jne  end;
		mov  ds:[_outlined_object], eax;
		call obj_outline_all_items_on;
end:
		retn;
	}
}

DWORD __stdcall GetScriptProcByName(DWORD scriptPtr, const char* procName) {
	__asm {
		mov  edx, procName;
		mov  eax, scriptPtr;
		call interpretFindProcedure_;
	}
}

// loads script from .int file into a sScriptProgram struct, filling script pointer and proc lookup table
void LoadScriptProgram(sScriptProgram &prog, const char* fileName) {
	DWORD scriptPtr;
	__asm {
		mov  eax, fileName;
		call loadProgram_;
		mov  scriptPtr, eax;
	}
	if (scriptPtr) {
		const char** procTable = (const char **)_procTableStrs;
		prog.ptr = scriptPtr;
		// fill lookup table
		for (int i = 0; i <= SCRIPT_PROC_MAX; i++) {
			prog.procLookup[i] = GetScriptProcByName(prog.ptr, procTable[i]);
		}
		prog.initialized = 0;
	} else {
		prog.ptr = NULL;
	}
}

void InitScriptProgram(sScriptProgram &prog) {
	DWORD progPtr = prog.ptr;
	if (prog.initialized == 0) {
		__asm {
			mov  eax, progPtr;
			call runProgram_;
			mov  edx, -1;
			mov  eax, progPtr;
			call interpret_;
		}
		prog.initialized = 1;
	}
}

void AddProgramToMap(sScriptProgram &prog) {
	sfallProgsMap[prog.ptr] = prog;
}

sScriptProgram* GetGlobalScriptProgram(DWORD scriptPtr) {
	SfallProgsMap::iterator it = sfallProgsMap.find(scriptPtr);
	return (it == sfallProgsMap.end()) ? nullptr : &it->second ; // prog
}

bool __stdcall IsGameScript(const char* filename) {
	for (int i = 0; filename[i]; ++i) if (i > 8) return false;
	for (int i = 0; i < *ptr_maxScriptNum; i++) {
		if (strcmp(filename, (char*)(*ptr_scriptListInfo + i * 20)) == 0) return true;
	}
	return false;
}

// this runs after the game was loaded/started
void LoadGlobalScripts() {
	isGameLoading = false;
	HookScriptInit();
	dlogr("Loading global scripts:", DL_SCRIPT|DL_INIT);

	char* name = "scripts\\gl*.int";
	char** filenames;
	int count = DbGetFileList(name, &filenames);

	// TODO: refactor script programs
	sScriptProgram prog;
	for (int i = 0; i < count; i++) {
		name = _strlwr(filenames[i]); // name of the script in lower case
		if (name[0] != 'g' || name[1] != 'l') continue; // fix bug in db_get_file_list fuction (if the script name begins with a non-Latin character)

		std::string baseName(name);
		int lastDot = baseName.find_last_of('.');
		if ((baseName.length() - lastDot) > 4) continue; // skip files with invalid extension (bug in db_get_file_list fuction)

		baseName = baseName.substr(0, lastDot); // script name without extension
		if (!IsGameScript(baseName.c_str())) {
			dlog("> ", DL_SCRIPT);
			dlog(name, DL_SCRIPT);
			isGlobalScriptLoading = 1;
			LoadScriptProgram(prog, baseName.c_str());
			if (prog.ptr) {
				dlogr(" Done", DL_SCRIPT);
				sGlobalScript gscript = sGlobalScript(prog);
				gscript.startProc = prog.procLookup[start]; // get 'start' procedure position
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
	DbFreeFileList(&filenames, 0);

	dlogr("Finished loading global scripts.", DL_SCRIPT|DL_INIT);
}

static bool __stdcall ScriptHasLoaded(DWORD script) {
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
	HookScriptClear();
}

void __stdcall RunScriptProcByNum(DWORD sptr, DWORD procNum) {
	__asm {
		mov  edx, procNum;
		mov  eax, sptr;
		call executeProcedure_;
	}
}

void RunScriptProc(sScriptProgram* prog, const char* procName) {
	DWORD sptr = prog->ptr;
	DWORD procNum = GetScriptProcByName(sptr, procName);
	if (procNum != -1) {
		RunScriptProcByNum(sptr, procNum);
	}
}

void RunScriptProc(sScriptProgram* prog, DWORD procId) {
	if (procId > 0 && procId <= SCRIPT_PROC_MAX) {
		DWORD procNum = prog->procLookup[procId];
		if (procNum != -1) {
			RunScriptProcByNum(prog->ptr, procNum);
		}
	}
}

int RunScriptStartProc(sScriptProgram* prog) {
	DWORD procNum = prog->procLookup[start];
	if (procNum != -1) {
		RunScriptProcByNum(prog->ptr, procNum);
	}
	return procNum;
}

static void RunScript(sGlobalScript* script) {
	script->count = 0;
	if (script->startProc != -1) {
		RunScriptProcByNum(script->prog.ptr, script->startProc); // run "start"
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

/**
	Do some cleaning after each combat attack action
*/
void AfterAttackCleanup() {
	ResetExplosionSettings();
}

static void RunGlobalScripts1() {
	if (idle > -1) Sleep(idle);

	if (toggleHighlightsKey) {
		//0x48C294 to toggle
		if (KeyDown(toggleHighlightsKey)) {
			if (!highlightingToggled) {
				if (motionScanner & 4) {
					DWORD scanner;
					__asm {
						mov eax, ds:[_obj_dude];
						mov edx, PID_MOTION_SENSOR;
						call inven_pid_is_carried_ptr_;
						mov scanner, eax;
					}
					if (scanner) {
						if (!(motionScanner & 2)) {
							__asm {
								mov eax, scanner;
								call item_m_dec_charges_; //Returns -1 if the item has no charges
								call intface_redraw_;
								inc eax;
								mov highlightingToggled, eax;
							}
							if (!highlightingToggled) DisplayConsoleMessage(HighlightFail2);
						} else highlightingToggled = 1;
					} else {
						DisplayConsoleMessage(HighlightFail1);
					}
				} else highlightingToggled = 1;
				if (highlightingToggled) obj_outline_all_items_on();
				else highlightingToggled = 2;
			}
		} else if (highlightingToggled) {
			if (highlightingToggled == 1) obj_outline_all_items_off();
			highlightingToggled = 0;
		}
	}
	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].repeat
			&& (globalScripts[i].mode == 0 || globalScripts[i].mode == 3)
			&& ++globalScripts[i].count >= globalScripts[i].repeat) {
			RunScript(&globalScripts[i]);
		}
	}
	ResetStateAfterFrame();
}

void RunGlobalScripts2() {
	if (IsGameLoaded()) {
		if (idle > -1) Sleep(idle);

		for (size_t i = 0; i < globalScripts.size(); i++) {
			if (globalScripts[i].repeat
				&& globalScripts[i].mode == 1
				&& ++globalScripts[i].count >= globalScripts[i].repeat) {
				RunScript(&globalScripts[i]);
			}
		}
		ResetStateAfterFrame();
	}
}

void RunGlobalScripts3() {
	if (idle > -1) Sleep(idle);

	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].repeat
			&& (globalScripts[i].mode == 2 || globalScripts[i].mode == 3)
			&& ++globalScripts[i].count >= globalScripts[i].repeat) {
			RunScript(&globalScripts[i]);
		}
	}
	ResetStateAfterFrame();
}

static DWORD __stdcall HandleMapUpdateForScripts(const DWORD procId) {
	if (procId == map_enter_p_proc) {
		// map changed, all game objects were destroyed and scripts detached, need to re-insert global scripts into the game
		for (std::vector<sGlobalScript>::const_iterator it = globalScripts.cbegin(); it != globalScripts.cend(); ++it) {
			DWORD progPtr = it->prog.ptr;
			__asm {
				mov  eax, progPtr;
				call runProgram_;
			}
		}
	} else if (procId == map_exit_p_proc) {
		ClearEventsOnMapExit(); // for reordering the execution of functions before exiting the map
	}

	RunGlobalScriptsAtProc(procId); // gl* scripts of types 0 and 3
	RunHookScriptsAtProc(procId);   // all hs_ scripts

	return procId; // restore eax (don't delete)
}

static DWORD HandleTimedEventScripts() {
	DWORD currentTime = *ptr_fallout_game_time;
	bool wasRunning = false;
	std::list<TimedEvent>::const_iterator timerIt = timerEventScripts.cbegin();
	for (; timerIt != timerEventScripts.cend(); ++timerIt) {
		if (currentTime >= timerIt->time) {
			timedEvent = const_cast<TimedEvent*>(&(*timerIt));
			DevPrintf("\n[TimedEventScripts] run event: %d", timerIt->time);
			RunScriptProc(timerIt->script, timed_event_p_proc);
			wasRunning = true;
		} else {
			break;
		}
	}
	if (wasRunning) {
		for (std::list<TimedEvent>::const_iterator _it = timerEventScripts.cbegin(); _it != timerIt; ++_it) {
			DevPrintf("\n[TimedEventScripts] delete event: %d", _it->time);
		}
		timerEventScripts.erase(timerEventScripts.cbegin(), timerIt);
	}
	timedEvent = nullptr;
	return currentTime;
}

static DWORD TimedEventNextTime() {
	DWORD nextTime;
	__asm {
		push ecx;
		call queue_next_time_;
		mov  nextTime, eax;
		push edx;
	}
	if (!timerEventScripts.empty()) {
		DWORD time = timerEventScripts.front().time;
		if (!nextTime || time < nextTime) nextTime = time;
	}
	__asm pop edx;
	__asm pop ecx;
	return nextTime;
}

static DWORD script_chk_timed_events_hook() {
	return (!*ptr_queue && timerEventScripts.empty());
}

void __stdcall AddTimerEventScripts(DWORD script, long time, long param) {
	sScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	TimedEvent timer;
	timer.script = scriptProg;
	timer.fixed_param = param;
	timer.time = *ptr_fallout_game_time + time;
	timerEventScripts.push_back(std::move(timer));
	timerEventScripts.sort(TimedEvent());
}

void __stdcall RemoveTimerEventScripts(DWORD script, long param) {
	sScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	timerEventScripts.remove_if([scriptProg, param] (TimedEvent timer) {
		return timer.script == scriptProg && timer.fixed_param == param;
	});
}

void __stdcall RemoveTimerEventScripts(DWORD script) {
	sScriptProgram* scriptProg = &(sfallProgsMap.find(script)->second);
	timerEventScripts.remove_if([scriptProg] (TimedEvent timer) {
		return timer.script == scriptProg;
	});
}

// run all global scripts of types 0 and 3 at specific procedure (if exist)
void __stdcall RunGlobalScriptsAtProc(DWORD procId) {
	for (size_t i = 0; i < globalScripts.size(); i++) {
		if (globalScripts[i].mode != 0 && globalScripts[i].mode != 3) continue;
		RunScriptProc(&globalScripts[i].prog, procId);
	}
}

bool LoadGlobals(HANDLE h) {
	DWORD count, unused;
	ReadFile(h, &count, 4, &unused, 0);
	if (unused != 4) return true;
	sGlobalVar var;
	for (DWORD i = 0; i < count; i++) {
		ReadFile(h, &var, sizeof(sGlobalVar), &unused, 0);
		globalVars.insert(glob_pair(var.id, var.val));
	}
	return false;
}

void SaveGlobals(HANDLE h) {
	DWORD count, unused;
	count = globalVars.size();
	WriteFile(h, &count, 4, &unused, 0);
	sGlobalVar var;
	glob_citr itr = globalVars.begin();
	while (itr != globalVars.end()) {
		var.id = itr->first;
		var.val = itr->second;
		WriteFile(h, &var, sizeof(sGlobalVar), &unused, 0);
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

void GetGlobals(sGlobalVar* globals) {
	glob_citr itr = globalVars.begin();
	int i = 0;
	while (itr != globalVars.end()) {
		globals[i].id = itr->first;
		globals[i++].val = itr->second;
		++itr;
	}
}

void SetGlobals(sGlobalVar* globals) {
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
		jmp  scr_exec_map_exit_scripts_;
skip:
		jmp  partyMemberSaveProtos_;
	}
}

static void ClearEventsOnMapExit() {
	__asm {
		mov  eax, explode_event; // type
		mov  edx, queue_explode_exit_; // func
		call queue_clear_type_;
		mov  eax, explode_fail_event;
		mov  edx, queue_explode_exit_;
		call queue_clear_type_;
	}
}

void ScriptExtender_OnGameLoad() {
	ClearGlobalScripts();
	ClearGlobals();
	RegAnimCombatCheck(1);
	ForceEncounterRestore(); // restore if the encounter did not happen
}

void ScriptExtenderInit() {
	toggleHighlightsKey = GetConfigInt("Input", "ToggleItemHighlightsKey", 0);
	if (toggleHighlightsKey) {
		highlightContainers = GetConfigInt("Input", "HighlightContainers", 0);
		highlightCorpses = GetConfigInt("Input", "HighlightCorpses", 0);
		outlineColor = GetConfigInt("Input", "OutlineColor", 0x10);
		if (outlineColor < 1) outlineColor = 0x40;
		motionScanner = GetConfigInt("Misc", "MotionScannerFlags", 1);
		Translate("Sfall", "HighlightFail1", "You aren't carrying a motion sensor.", HighlightFail1);
		Translate("Sfall", "HighlightFail2", "Your motion sensor is out of charge.", HighlightFail2);
		HookCall(0x44BD1C, obj_remove_outline_hook); // gmouse_bk_process_
		HookCall(0x44E559, obj_remove_outline_hook); // gmouse_remove_item_outline_
	}

	idle = GetConfigInt("Misc", "ProcessorIdle", -1);
	if (idle > -1) {
		if (idle > 127) idle = 127;
		*ptr_idle_func = reinterpret_cast<void*>(Sleep);
		SafeWrite8(0x4C9F12, 0x6A); // push idle
		SafeWrite8(0x4C9F13, idle);
	}

	arraysBehavior = GetConfigInt("Misc", "arraysBehavior", 1);
	if (arraysBehavior > 0) {
		arraysBehavior = 1; // only 1 and 0 allowed at this time
		dlogr("New arrays behavior enabled.", DL_SCRIPT);
	} else dlogr("Arrays in backward-compatiblity mode.", DL_SCRIPT);

	HookCall(0x480E7B, MainGameLoopHook); // hook the main game loop
	HookCall(0x422845, CombatLoopHook);   // hook the combat loop
	MakeCall(0x4230D5, AfterCombatAttackHook);

	MakeJump(0x4A390C, FindSidHack); // scr_find_sid_from_program_
	MakeJump(0x4A5E34, ScrPtrHack);

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
	HookCall(0x4414C8, Export_Export_FindVar_Hook);
	const DWORD exportFindVarAddr[] = {
		0x441285, // store
		0x4413D9  // fetch
	};
	HookCalls(Export_FetchOrStore_FindVar_Hook, exportFindVarAddr);

	HookCall(0x46E141, FreeProgramHook);

	// combat_is_starting_p_proc / combat_is_over_p_proc
	HookCall(0x421B72, CombatBeginHook);
	HookCall(0x421FC1, CombatOverHook);

	// Reorder the execution of functions before exiting the map
	// Call saving party member prototypes and removing the drug effects for NPC after executing map_exit_p_proc procedure
	HookCall(0x483CB4, map_save_in_game_hook);
	HookCall(0x483CC3, (void*)partyMemberSaveProtos_);
	HookCall(0x483CC8, (void*)partyMemberPrepLoad_);
	HookCall(0x483CCD, (void*)partyMemberPrepItemSaveAll_);

	// Set the DAM_BACKWASH flag for the attacker before calling compute_damage_
	SafeWrite32(0x423DE7, 0x40164E80); // or [esi+ctd.flags3Source], DAM_BACKWASH_
	long idata = 0x146E09;             // or dword ptr [esi+ctd.flagsSource], ebp
	SafeWriteBytes(0x423DF0, (BYTE*)&idata, 3);
	// 0x423DEB - call ComputeDamageHook (in HookScripts.cpp)

	dlogr("Adding additional opcodes", DL_SCRIPT);

	SafeWrite32(0x46E370, 0x300);          // Maximum number of allowed opcodes
	SafeWrite32(0x46CE34, (DWORD)opcodes); // cmp check to make sure opcode exists
	SafeWrite32(0x46CE6C, (DWORD)opcodes); // call that actually jumps to the opcode
	SafeWrite32(0x46E390, (DWORD)opcodes); // mov that writes to the opcode

	SetExtraKillCounter(UsingExtraKillTypes());

	if (int unsafe = iniGetInt("Debugging", "AllowUnsafeScripting", 0, ddrawIniDef)) {
		if (unsafe == 2) checkValidMemAddr = false;
		dlogr("  Unsafe opcodes enabled.", DL_SCRIPT);
		opcodes[0x1cf] = op_write_byte;
		opcodes[0x1d0] = op_write_short;
		opcodes[0x1d1] = op_write_int;
		opcodes[0x21b] = op_write_string;
		for (int i = 0x1d2; i < 0x1dc; i++) {
			opcodes[i] = op_call_offset;
		}
	} else {
		dlogr("  Unsafe opcodes disabled.", DL_SCRIPT);
	}
	opcodes[0x156] = op_read_byte;
	opcodes[0x157] = op_read_short;
	opcodes[0x158] = op_read_int;
	opcodes[0x159] = op_read_string;
	opcodes[0x15a] = op_set_pc_base_stat;
	opcodes[0x15b] = op_set_pc_extra_stat;
	opcodes[0x15c] = op_get_pc_base_stat;
	opcodes[0x15d] = op_get_pc_extra_stat;
	opcodes[0x15e] = op_set_critter_base_stat;
	opcodes[0x15f] = op_set_critter_extra_stat;
	opcodes[0x160] = op_get_critter_base_stat;
	opcodes[0x161] = op_get_critter_extra_stat;
	opcodes[0x162] = op_tap_key;
	opcodes[0x163] = op_get_year;
	opcodes[0x164] = op_game_loaded;
	opcodes[0x165] = op_graphics_funcs_available;
	opcodes[0x166] = op_load_shader;
	opcodes[0x167] = op_free_shader;
	opcodes[0x168] = op_activate_shader;
	opcodes[0x169] = op_deactivate_shader;
	opcodes[0x16a] = op_set_global_script_repeat;
	opcodes[0x16b] = op_input_funcs_available;
	opcodes[0x16c] = op_key_pressed;
	opcodes[0x16d] = op_set_shader_int;
	opcodes[0x16e] = op_set_shader_float;
	opcodes[0x16f] = op_set_shader_vector;
	opcodes[0x170] = op_in_world_map;
	opcodes[0x171] = op_force_encounter;
	opcodes[0x172] = op_set_world_map_pos;
	opcodes[0x173] = op_get_world_map_x_pos;
	opcodes[0x174] = op_get_world_map_y_pos;
	opcodes[0x175] = op_set_dm_model;
	opcodes[0x176] = op_set_df_model;
	opcodes[0x177] = op_set_movie_path;
	for (int i = 0x178; i < 0x189; i++) {
		opcodes[i] = op_set_perk_value;
	}
	opcodes[0x189] = op_set_perk_name;
	opcodes[0x18a] = op_set_perk_desc;
	opcodes[0x18b] = op_set_pipboy_available;
	opcodes[0x18c] = op_get_kill_counter;
	opcodes[0x18d] = op_mod_kill_counter;
	opcodes[0x18e] = op_get_perk_owed;
	opcodes[0x18f] = op_set_perk_owed;
	opcodes[0x190] = op_get_perk_available;
	opcodes[0x191] = op_get_critter_current_ap;
	opcodes[0x192] = op_set_critter_current_ap;
	opcodes[0x193] = op_active_hand;
	opcodes[0x194] = op_toggle_active_hand;
	opcodes[0x195] = op_set_weapon_knockback;
	opcodes[0x196] = op_set_target_knockback;
	opcodes[0x197] = op_set_attacker_knockback;
	opcodes[0x198] = op_remove_weapon_knockback;
	opcodes[0x199] = op_remove_target_knockback;
	opcodes[0x19a] = op_remove_attacker_knockback;
	opcodes[0x19b] = op_set_global_script_type;
	opcodes[0x19c] = op_available_global_script_types;
	opcodes[0x19d] = op_set_sfall_global;
	opcodes[0x19e] = op_get_sfall_global_int;
	opcodes[0x19f] = op_get_sfall_global_float;
	opcodes[0x1a0] = op_set_pickpocket_max;
	opcodes[0x1a1] = op_set_hit_chance_max;
	opcodes[0x1a2] = op_set_skill_max;
	opcodes[0x1a3] = op_eax_available;
	//opcodes[0x1a4] = op_set_eax_environment;
	opcodes[0x1a5] = op_inc_npc_level;
	opcodes[0x1a6] = op_get_viewport_x;
	opcodes[0x1a7] = op_get_viewport_y;
	opcodes[0x1a8] = op_set_viewport_x;
	opcodes[0x1a9] = op_set_viewport_y;
	opcodes[0x1aa] = op_set_xp_mod;
	opcodes[0x1ab] = op_set_perk_level_mod;
	opcodes[0x1ac] = op_get_ini_setting;
	opcodes[0x1ad] = op_get_shader_version;
	opcodes[0x1ae] = op_set_shader_mode;
	opcodes[0x1af] = op_get_game_mode;
	opcodes[0x1b0] = op_force_graphics_refresh;
	opcodes[0x1b1] = op_get_shader_texture;
	opcodes[0x1b2] = op_set_shader_texture;
	opcodes[0x1b3] = op_get_uptime;
	opcodes[0x1b4] = op_set_stat_max;
	opcodes[0x1b5] = op_set_stat_min;
	opcodes[0x1b6] = op_set_car_current_town;
	opcodes[0x1b7] = op_set_pc_stat_max;
	opcodes[0x1b8] = op_set_pc_stat_min;
	opcodes[0x1b9] = op_set_npc_stat_max;
	opcodes[0x1ba] = op_set_npc_stat_min;
	opcodes[0x1bb] = op_set_fake_perk;
	opcodes[0x1bc] = op_set_fake_trait;
	opcodes[0x1bd] = op_set_selectable_perk;
	opcodes[0x1be] = op_set_perkbox_title;
	opcodes[0x1bf] = op_hide_real_perks;
	opcodes[0x1c0] = op_show_real_perks;
	opcodes[0x1c1] = op_has_fake_perk;
	opcodes[0x1c2] = op_has_fake_trait;
	opcodes[0x1c3] = op_perk_add_mode;
	opcodes[0x1c4] = op_clear_selectable_perks;
	opcodes[0x1c5] = op_set_critter_hit_chance_mod;
	opcodes[0x1c6] = op_set_base_hit_chance_mod;
	opcodes[0x1c7] = op_set_critter_skill_mod;
	opcodes[0x1c8] = op_set_base_skill_mod;
	opcodes[0x1c9] = op_set_critter_pickpocket_mod;
	opcodes[0x1ca] = op_set_base_pickpocket_mod;
	opcodes[0x1cb] = op_set_pyromaniac_mod;
	opcodes[0x1cc] = op_apply_heaveho_fix;
	opcodes[0x1cd] = op_set_swiftlearner_mod;
	opcodes[0x1ce] = op_set_hp_per_level_mod;

	opcodes[0x1dc] = op_show_iface_tag;
	opcodes[0x1dd] = op_hide_iface_tag;
	opcodes[0x1de] = op_is_iface_tag_active;
	opcodes[0x1df] = op_get_bodypart_hit_modifier;
	opcodes[0x1e0] = op_set_bodypart_hit_modifier;
	opcodes[0x1e1] = op_set_critical_table;
	opcodes[0x1e2] = op_get_critical_table;
	opcodes[0x1e3] = op_reset_critical_table;
	opcodes[0x1e4] = op_get_sfall_arg;
	opcodes[0x1e5] = op_set_sfall_return;
	opcodes[0x1e6] = op_set_unspent_ap_bonus;
	opcodes[0x1e7] = op_get_unspent_ap_bonus;
	opcodes[0x1e8] = op_set_unspent_ap_perk_bonus;
	opcodes[0x1e9] = op_get_unspent_ap_perk_bonus;
	opcodes[0x1ea] = op_init_hook;
	opcodes[0x1eb] = op_get_ini_string;
	opcodes[0x1ec] = op_sqrt;
	opcodes[0x1ed] = op_abs;
	opcodes[0x1ee] = op_sin;
	opcodes[0x1ef] = op_cos;
	opcodes[0x1f0] = op_tan;
	opcodes[0x1f1] = op_arctan;
	opcodes[0x1f2] = op_set_palette;
	opcodes[0x1f3] = op_remove_script;
	opcodes[0x1f4] = op_set_script;
	opcodes[0x1f5] = op_get_script;
	opcodes[0x1f6] = op_nb_create_char;
	opcodes[0x1f7] = op_fs_create;
	opcodes[0x1f8] = op_fs_copy;
	opcodes[0x1f9] = op_fs_find;
	opcodes[0x1fa] = op_fs_write_byte;
	opcodes[0x1fb] = op_fs_write_short;
	opcodes[0x1fc] = op_fs_write_int;
	opcodes[0x1fd] = op_fs_write_int;
	opcodes[0x1fe] = op_fs_write_string;
	opcodes[0x1ff] = op_fs_delete;
	opcodes[0x200] = op_fs_size;
	opcodes[0x201] = op_fs_pos;
	opcodes[0x202] = op_fs_seek;
	opcodes[0x203] = op_fs_resize;
	opcodes[0x204] = op_get_proto_data;
	opcodes[0x205] = op_set_proto_data;
	opcodes[0x206] = op_set_self;
	opcodes[0x207] = op_register_hook;
	opcodes[0x208] = op_fs_write_bstring;
	opcodes[0x209] = op_fs_read_byte;
	opcodes[0x20a] = op_fs_read_short;
	opcodes[0x20b] = op_fs_read_int;
	opcodes[0x20c] = op_fs_read_float;
	opcodes[0x20d] = op_list_begin;
	opcodes[0x20e] = op_list_next;
	opcodes[0x20f] = op_list_end;
	opcodes[0x210] = op_sfall_ver_major;
	opcodes[0x211] = op_sfall_ver_minor;
	opcodes[0x212] = op_sfall_ver_build;
	opcodes[0x213] = op_hero_select_win;
	opcodes[0x214] = op_set_hero_race;
	opcodes[0x215] = op_set_hero_style;
	opcodes[0x216] = op_set_critter_burst_disable;
	opcodes[0x217] = op_get_weapon_ammo_pid;
	opcodes[0x218] = op_set_weapon_ammo_pid;
	opcodes[0x219] = op_get_weapon_ammo_count;
	opcodes[0x21a] = op_set_weapon_ammo_count;

	opcodes[0x21c] = op_get_mouse_x;
	opcodes[0x21d] = op_get_mouse_y;
	opcodes[0x21e] = op_get_mouse_buttons;
	opcodes[0x21f] = op_get_window_under_mouse;
	opcodes[0x220] = op_get_screen_width;
	opcodes[0x221] = op_get_screen_height;
	opcodes[0x222] = op_stop_game;
	opcodes[0x223] = op_resume_game;
	opcodes[0x224] = op_create_message_window;
	opcodes[0x225] = op_remove_trait;
	opcodes[0x226] = op_get_light_level;
	opcodes[0x227] = op_refresh_pc_art;
	opcodes[0x228] = op_get_attack_type;
	opcodes[0x229] = op_force_encounter_with_flags;
	opcodes[0x22a] = op_set_map_time_multi;
	opcodes[0x22b] = op_play_sfall_sound;
	opcodes[0x22c] = op_stop_sfall_sound;
	opcodes[0x22d] = op_create_array;
	opcodes[0x22e] = op_set_array;
	opcodes[0x22f] = op_get_array;
	opcodes[0x230] = op_free_array;
	opcodes[0x231] = op_len_array;
	opcodes[0x232] = op_resize_array;
	opcodes[0x233] = op_temp_array;
	opcodes[0x234] = op_fix_array;
	opcodes[0x235] = op_string_split;
	opcodes[0x236] = op_list_as_array;
	opcodes[0x237] = op_atoi;
	opcodes[0x238] = op_atof;
	opcodes[0x239] = op_scan_array;
	opcodes[0x23a] = op_get_tile_fid;
	opcodes[0x23b] = op_modified_ini;
	opcodes[0x23c] = op_get_sfall_args;
	opcodes[0x23d] = op_set_sfall_arg;
	opcodes[0x23e] = op_force_aimed_shots;
	opcodes[0x23f] = op_disable_aimed_shots;
	opcodes[0x240] = op_mark_movie_played;
	opcodes[0x241] = op_get_npc_level;
	opcodes[0x242] = op_set_critter_skill_points;
	opcodes[0x243] = op_get_critter_skill_points;
	opcodes[0x244] = op_set_available_skill_points;
	opcodes[0x245] = op_get_available_skill_points;
	opcodes[0x246] = op_mod_skill_points_per_level;
	opcodes[0x247] = op_set_perk_freq;
	opcodes[0x248] = op_get_last_attacker;
	opcodes[0x249] = op_get_last_target;
	opcodes[0x24a] = op_block_combat;
	opcodes[0x24b] = op_tile_under_cursor;
	opcodes[0x24c] = op_gdialog_get_barter_mod;
	opcodes[0x24d] = op_set_inven_ap_cost;
	opcodes[0x24e] = op_substr;
	opcodes[0x24f] = op_strlen;
	opcodes[0x250] = op_sprintf;
	opcodes[0x251] = op_ord;
	// opcodes[0x252] = RESERVED
	opcodes[0x253] = op_typeof;
	opcodes[0x254] = op_save_array;
	opcodes[0x255] = op_load_array;
	opcodes[0x256] = op_get_array_key;
	opcodes[0x257] = op_stack_array;
	// opcodes[0x258] = RESERVED for arrays
	// opcodes[0x259] = RESERVED for arrays
	opcodes[0x25a] = op_reg_anim_destroy;
	opcodes[0x25b] = op_reg_anim_animate_and_hide;
	opcodes[0x25c] = op_reg_anim_combat_check;
	opcodes[0x25d] = op_reg_anim_light;
	opcodes[0x25e] = op_reg_anim_change_fid;
	opcodes[0x25f] = op_reg_anim_take_out;
	opcodes[0x260] = op_reg_anim_turn_towards;
	opcodes[0x261] = op_explosions_metarule;
	opcodes[0x262] = op_register_hook_proc;
	opcodes[0x263] = op_power; // '^' operator
	opcodes[0x264] = op_log;
	opcodes[0x265] = op_exponent;
	opcodes[0x266] = op_ceil;
	opcodes[0x267] = op_round;
	// opcodes[0x268] = RESERVED
	// opcodes[0x269] = RESERVED
	// opcodes[0x26a] = op_game_ui_redraw;
	opcodes[0x26b] = op_message_str_game;
	opcodes[0x26c] = op_sneak_success;
	opcodes[0x26d] = op_tile_light;
	opcodes[0x26e] = op_make_straight_path;
	opcodes[0x26f] = op_obj_blocking_at;
	opcodes[0x270] = op_tile_get_objects;
	opcodes[0x271] = op_get_party_members;
	opcodes[0x272] = op_make_path;
	opcodes[0x273] = op_create_spatial;
	opcodes[0x274] = op_art_exists;
	opcodes[0x275] = op_obj_is_carrying_obj;
	// universal opcodes
	opcodes[0x276] = op_sfall_metarule0;
	opcodes[0x277] = op_sfall_metarule1;
	opcodes[0x278] = op_sfall_metarule2;
	opcodes[0x279] = op_sfall_metarule3;
	opcodes[0x27a] = op_sfall_metarule4;
	opcodes[0x27b] = op_sfall_metarule5;
	opcodes[0x27c] = op_sfall_metarule6; // if you need more arguments - use arrays

	opcodes[0x27d] = op_register_hook_proc_spec;
	opcodes[0x27e] = op_reg_anim_callback;
	opcodes[0x27f] = op_div; // div operator

	InitOpcodeMetaTable();
	InitMetaruleTable();
}
