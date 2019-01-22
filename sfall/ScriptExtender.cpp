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

#include "main.h"

#include <cassert>
#include <hash_map>
#include <set>
#include <string>

#include "Arrays.h"
#include "BarBoxes.h"
#include "Console.h"
#include "Define.h"
#include "Explosions.h"
#include "FalloutEngine.h"
#include "HookScripts.h"
#include "input.h"
#include "LoadGameHook.h"
#include "Logging.h"
#include "ScriptExtender.h"
#include "version.h"
#if (_MSC_VER < 1600)
#include "Cpp11_emu.h"
#endif

static DWORD _stdcall HandleMapUpdateForScripts(const DWORD procId);

// variables for new opcodes
#define OP_MAX_ARGUMENTS	(10)

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

	ScriptValue(int val) {
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

	DWORD rawValue() const {
		return _val.dw;
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

	const char* String() const {
		return _val.str;
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
} ;

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
		return _args.at(index + _argShift);
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
					i,
					GetSfallTypeName(argI.type()));

				return false;
			} else if ((typeMask & DATATYPE_MASK_NOT_NULL) && argI.rawValue() == 0) {
				printOpcodeError(
					"%s() - argument #%d is null.", 
					opcodeName, 
					i);

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
			DWORD rawValueType = InterpretPopShort(program);
			DWORD rawValue = InterpretPopLong(program);
			SfallDataType type = static_cast<SfallDataType>(getSfallTypeByScriptType(rawValueType));

			// retrieve string argument
			if (type == DATATYPE_STR) {
				_args.at(i) = InterpretGetString(program, rawValue, rawValueType);
			} else {
				_args.at(i) = ScriptValue(type, rawValue);
			}
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
				// if no value was set in handler, force return 0 to avoid stack error
				_ret = ScriptValue(0);
			}
			DWORD rawResult = _ret.rawValue();
			if (_ret.type() == DATATYPE_STR) {
				rawResult = InterpretAddString(program, _ret.asString());
			}
			InterpretPushLong(program, rawResult);
			InterpretPushShort(program, getScriptTypeBySfallType(_ret.type()));
		}
	}

private:
	TProgram* _program;

	int _argShift;
	std::vector<ScriptValue> _args;
	ScriptValue _ret;
};

static OpcodeHandler opHandler;



#include "ScriptOps\AsmMacros.h"
#include "ScriptOps\ScriptArrays.hpp"
#include "ScriptOps\ScriptUtils.hpp"

#include "ScriptOps\WorldmapOps.hpp"
#include "ScriptOps\PerksOp.hpp"
#include "ScriptOps\MemoryOp.hpp"
#include "ScriptOps\StatsOp.hpp"
#include "ScriptOps\InterfaceOp.hpp"
#include "ScriptOps\GraphicsOp.hpp"
#include "ScriptOps\FileSystemOps.hpp"
#include "ScriptOps\ObjectsOps.hpp"
#include "ScriptOps\AnimOps.hpp"
#include "ScriptOps\MiscOps.hpp"
#include "ScriptOps\MetaruleOp.hpp"

/*
	Array for opcodes metadata.

	This is completely optional, added for convenience only.

	By adding opcode to this array, Sfall will automatically validate it's arguments using provided info.
	On fail, errors will be printed to debug.log and opcode will not be executed.
	If you don't include opcode in this array, you should take care of all argument validation inside handler itself.
*/
static const SfallOpcodeMetadata opcodeMetaArray[] = {
	{sf_create_win, "create_win", {DATATYPE_MASK_STR, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{sf_critter_inven_obj2, "critter_inven_obj2", {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{sf_get_current_inven_size, "get_current_inven_size", {DATATYPE_MASK_VALID_OBJ}},
	{sf_get_flags, "get_flags", {DATATYPE_MASK_VALID_OBJ}},
	{sf_get_object_data, "get_object_data", {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{sf_get_outline, "get_outline", {DATATYPE_MASK_VALID_OBJ}},
	{sf_inventory_redraw, "inventory_redraw", {DATATYPE_MASK_INT}},
	{sf_item_weight, "item_weight", {DATATYPE_MASK_VALID_OBJ}},
	{sf_lock_is_jammed, "lock_is_jammed", {DATATYPE_MASK_VALID_OBJ}},
	{sf_get_obj_under_cursor, "obj_under_cursor", {DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{sf_set_cursor_mode, "set_cursor_mode", {DATATYPE_MASK_INT}},
	{sf_set_flags, "set_flags", {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{sf_set_ini_setting, "set_ini_setting", {DATATYPE_MASK_STR, DATATYPE_MASK_INT | DATATYPE_MASK_STR}},
	{sf_set_map_enter_position, "set_map_enter_position", {DATATYPE_MASK_INT, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{sf_set_object_data, "set_object_data", {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT, DATATYPE_MASK_INT}},
	{sf_set_outline, "set_outline", {DATATYPE_MASK_VALID_OBJ, DATATYPE_MASK_INT}},
	{sf_spatial_radius, "spatial_radius", {DATATYPE_MASK_VALID_OBJ}},
	{sf_unjam_lock, "unjam_lock", {DATATYPE_MASK_VALID_OBJ}},
	{sf_test, "validate_test", {DATATYPE_MASK_INT, DATATYPE_MASK_INT | DATATYPE_MASK_FLOAT, DATATYPE_MASK_STR, DATATYPE_NONE}},
	//{op_message_str_game, {}}
};

static void InitOpcodeMetaTable() {
	int length = sizeof(opcodeMetaArray) / sizeof(SfallOpcodeMetadata);
	for (int i = 0; i < length; ++i) {
		opcodeMetaTable[opcodeMetaArray[i].handler] = &opcodeMetaArray[i];
	}
}

typedef void (_stdcall *regOpcodeProc)(WORD opcode,void* ptr);

static DWORD highlightingToggled = 0;
static DWORD MotionSensorMode;
static BYTE toggleHighlightsKey;
static DWORD highlightContainers = 0;
static int outlineColor = 0x10;
static int idle;
static char HighlightFail1[128];
static char HighlightFail2[128];

struct sGlobalScript {
	sScriptProgram prog;
	DWORD count;
	DWORD repeat;
	DWORD mode; //0 - local map loop, 1 - input loop, 2 - world map loop, 3 - local and world map loops

	sGlobalScript() {}
	sGlobalScript(sScriptProgram script) {
		prog = script;
		count = 0;
		repeat = 0;
		mode = 0;
	}
};

struct sExportedVar {
	DWORD type; // in scripting engine terms, eg. VAR_TYPE_*
	DWORD val;
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

static std::vector<DWORD> checkedScripts;
static std::vector<sGlobalScript> globalScripts;

// a map of all sfall programs (global and hook scripts) by thier scriptPtr
typedef stdext::hash_map<DWORD, sScriptProgram> SfallProgsMap;
static SfallProgsMap sfallProgsMap;

// a map scriptPtr => self_obj  to override self_obj for all script types using set_self
stdext::hash_map<DWORD, SelfOverrideObj> selfOverrideMap;

typedef std::tr1::unordered_map<std::string, sExportedVar> ExportedVarsMap;
static ExportedVarsMap globalExportedVars;
DWORD isGlobalScriptLoading = 0;

stdext::hash_map<__int64, int> globalVars;
typedef stdext::hash_map<__int64, int> :: iterator glob_itr;
typedef stdext::hash_map<__int64, int> :: const_iterator glob_citr;
typedef std::pair<__int64, int> glob_pair;

static void* opcodes[0x300];
DWORD AvailableGlobalScriptTypes = 0;
bool isGameLoading;

TScript OverrideScriptStruct = {0};

//eax contains the script pointer, edx contains the opcode*4

//To read a value, mov the script pointer to eax, call interpretPopShort_, eax now contains the value type
//mov the script pointer to eax, call interpretPopLong_, eax now contains the value

//To return a value, move it to edx, mov the script pointer to eax, call interpretPushLong_
//mov the value type to edx, mov the script pointer to eax, call interpretPushShort_



static void _stdcall SetGlobalScriptRepeat2(DWORD script, DWORD frames) {
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (globalScripts[d].prog.ptr == script) {
			if (frames == 0xFFFFFFFF) globalScripts[d].mode = !globalScripts[d].mode;
			else globalScripts[d].repeat = frames;
			break;
		}
	}
}

static void __declspec(naked) SetGlobalScriptRepeat() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		push ecx;
		call SetGlobalScriptRepeat2;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void _stdcall SetGlobalScriptType2(DWORD script, DWORD type) {
	if (type > 3) return;
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (globalScripts[d].prog.ptr == script) {
			globalScripts[d].mode = type;
			break;
		}
	}
}

static void __declspec(naked) SetGlobalScriptType() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		push ecx;
		call SetGlobalScriptType2;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) GetGlobalScriptTypes() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov edx, AvailableGlobalScriptTypes;
		mov ecx, eax;
		call interpretPushLong_;
		mov edx, VAR_TYPE_INT;
		mov eax, ecx;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		pop ebx;
		retn;
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

static long SetGlobalVar2(const char* var, int val) {
	if (strlen(var) != 8) {
		return -1;
	}
	SetGlobalVarInternal(*(__int64*)var, val);
	return 0;
}

static void SetGlobalVar2Int(DWORD var, int val) {
	SetGlobalVarInternal(var, val);
}

static void SetGlobalVarFunc() {
	const ScriptValue &varArg = opHandler.arg(0),
					  &valArg = opHandler.arg(1);

	if ((varArg.isInt() || varArg.isString()) && (valArg.isInt() || valArg.isFloat())) {
		if (varArg.isString()) {
			if (SetGlobalVar2(varArg.String(), valArg.rawValue())) {
				opHandler.printOpcodeError("set_sfall_global() - The name of the global variable must consist of 8 characters.");
			}
		} else {
			SetGlobalVar2Int(varArg.rawValue(), valArg.rawValue());
		}
	} else {
		opHandler.printOpcodeError("set_sfall_global() - invalid arguments.");
	}
}

static void __declspec(naked) SetGlobalVar() {
	_WRAP_OPCODE(SetGlobalVarFunc, 2, 0)
}

static long GetGlobalVarInternal(__int64 val) {
	glob_citr itr = globalVars.find(val);
	if (itr == globalVars.end()) {
		return 0;
	} else {
		return itr->second;
	}
}

static long GetGlobalVar2(const char* var) {
	if (strlen(var) != 8) {
		return 0;
	}
	return GetGlobalVarInternal(*(__int64*)var);
}

static long GetGlobalVar2Int(DWORD var) {
	return GetGlobalVarInternal(var);
}

static void GetGlobalVarIntFunc() {
	const ScriptValue &varArg = opHandler.arg(0);
	long result = 0;

	if (varArg.isInt() || varArg.isString()) {
		if (varArg.isString()) {
			const char* var = varArg.String();
			if (strlen(var) != 8) {
				opHandler.printOpcodeError("get_sfall_global_int() - The name of the global variable must consist of 8 characters.");
			} else {
				result = GetGlobalVarInternal(*(__int64*)var);
			}
		} else {
			result = GetGlobalVar2Int(varArg.rawValue());
		}
		opHandler.setReturn(result, DATATYPE_INT);
	} else {
		opHandler.printOpcodeError("get_sfall_global_int() - argument is not an integer or a string.");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) GetGlobalVarInt() {
	_WRAP_OPCODE(GetGlobalVarIntFunc, 1, 1)
}

static void GetGlobalVarFloatFunc() {
	const ScriptValue &varArg = opHandler.arg(0);
	long result = 0;

	if (varArg.isInt() || varArg.isString()) {
		if (varArg.isString()) {
			const char* var = varArg.String();
			if (strlen(var) != 8) {
				opHandler.printOpcodeError("get_sfall_global_float() - The name of the global variable must consist of 8 characters.");
			} else {
				result = GetGlobalVarInternal(*(__int64*)var);
			}
		} else {
			result = GetGlobalVar2Int(varArg.rawValue());
		}
		opHandler.setReturn(result, DATATYPE_FLOAT);
	} else {
		opHandler.printOpcodeError("get_sfall_global_float() - argument is not an integer or a string.");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) GetGlobalVarFloat() {
	_WRAP_OPCODE(GetGlobalVarFloatFunc, 1, 1)
}

static void __declspec(naked) GetSfallArg() {
	__asm {
		pushad;
		push eax;
		call GetHSArg;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}

static DWORD _stdcall GetSfallArgs2() {
	DWORD argCount = GetHSArgCount();
	DWORD id = TempArray(argCount, 4);
	DWORD* args = GetHSArgs();
	for (DWORD i = 0; i < argCount; i++) {
		arrays[id].val[i].set(*(long*)&args[i]);
	}
	return id;
}

static void __declspec(naked) GetSfallArgs() {
	__asm {
		pushad;
		push eax;
		call GetSfallArgs2;
		pop ecx;
		mov edx, eax;
		mov eax, ecx;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		popad;
		retn;
	}
}

static void __declspec(naked) SetSfallArg() {
	__asm {
		pushad;
		mov ecx, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopShort_;
		mov esi, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		cmp si, VAR_TYPE_INT;
		jnz end;
		push edx;
		push eax;
		call SetHSArg;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) SetSfallReturn() {
	__asm {
		push ebx;
		push ecx;
		push edx;
		mov ecx, eax;
		call interpretPopShort_;
		mov edx, eax;
		mov eax, ecx;
		call interpretPopLong_;
		cmp dx, VAR_TYPE_INT;
		jnz end;
		push eax;
		call SetHSReturn;
end:
		pop edx;
		pop ecx;
		pop ebx;
		retn;
	}
}

static void __declspec(naked) InitHook() {
	__asm {
		push ecx;
		push edx;
		mov ecx, eax;
		mov edx, InitingHookScripts;
		call interpretPushLong_;
		mov eax, ecx;
		mov edx, VAR_TYPE_INT;
		call interpretPushShort_;
		pop edx;
		pop ecx;
		retn;
	}
}

static void _stdcall set_self2(DWORD script, TGameObj* obj) {
	stdext::hash_map<DWORD, SelfOverrideObj>::iterator it = selfOverrideMap.find(script);
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
	} else {
		if (isFind) selfOverrideMap.erase(it);
	}
}

static void __declspec(naked) set_self() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		push eax;
		push ebp;
		call set_self2;
end:
		popad;
		retn;
	}
}
static void __declspec(naked) register_hook() {
	__asm {
		pushad;
		mov ebp, eax;
		call interpretPopShort_;
		mov edi, eax;
		mov eax, ebp;
		call interpretPopLong_;
		cmp di, VAR_TYPE_INT;
		jnz end;
		push -1;
		push eax;
		push ebp;
		call RegisterHook;
end:
		popad;
		retn;
	}
}

static void __declspec(naked) register_hook_proc() {
	_OP_BEGIN(ebp)
	_GET_ARG_R32(ebp, ecx, esi)
	_GET_ARG_R32(ebp, ebx, edi)
	_CHECK_ARG_INT(cx, fail)
	_CHECK_ARG_INT(bx, fail)
	__asm {
		push esi;
		push edi;
		push ebp;
		call RegisterHook;
	fail:
	}
	_OP_END
}

static void __declspec(naked) sfall_ver_major() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_MAJOR;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) sfall_ver_minor() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_MINOR;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}

static void __declspec(naked) sfall_ver_build() {
	_OP_BEGIN(ebp)
	__asm {
		mov eax, VERSION_BUILD;
	}
	_RET_VAL_INT(ebp)
	_OP_END
}


//END OF SCRIPT FUNCTIONS
static const DWORD scr_ptr_back = scr_ptr_ + 5;
static const DWORD scr_find_sid_from_program = scr_find_sid_from_program_ + 5;
static const DWORD scr_find_obj_from_program = scr_find_obj_from_program_ + 7;

static DWORD _stdcall FindSid(DWORD script) {
	stdext::hash_map<DWORD, SelfOverrideObj>::iterator overrideIt = selfOverrideMap.find(script);
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
		OverrideScriptStruct.target_obj = OverrideScriptStruct.self_obj = 0;
		return -2; // override struct
	}
	return -1; // change nothing
}

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
		push eax;
		call RunGlobalScripts1;
		pop  eax;
		pop  edx;
		pop  ecx;
		jmp  get_input_;
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

static const DWORD ExecMapScriptsRet = 0x4A67F5;
static void __declspec(naked) ExecMapScriptsHack() {
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

static DWORD __fastcall GetGlobalExportedVarPtr(const char* name) {
	std::string str(name);
	ExportedVarsMap::iterator it = globalExportedVars.find(str);
	//dlog_f("\n Trying to find exported var %s... ", DL_MAIN, name);
	if (it != globalExportedVars.end()) {
		sExportedVar *ptr = &it->second;
		return (DWORD)ptr;
	}
	return 0;
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

static const DWORD Export_Export_FindVar_back = 0x4414AE;
static void __declspec(naked) Export_Export_FindVar_Hook() {
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
static void _stdcall FreeProgram(DWORD progPtr) {
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

static void __declspec(naked) obj_outline_all_items_on() {
	__asm {
		pushad
		mov  eax, dword ptr ds:[_map_elevation]
		call obj_find_first_at_
loopObject:
		test eax, eax
		jz   end
		cmp  eax, ds:[_outlined_object]
		je   nextObject
		xchg ecx, eax
		mov  eax, [ecx+0x20]
		and  eax, 0xF000000
		sar  eax, 0x18
		test eax, eax                             // Is this an item?
		jnz  nextObject                           // No
		cmp  dword ptr [ecx+0x7C], eax            // Owned by someone?
		jnz  nextObject                           // Yes
		test dword ptr [ecx+0x74], eax            // Already outlined?
		jnz  nextObject                           // Yes
		test byte ptr [ecx+0x25], 0x10            // Is NoHighlight_ flag set (is this a container)?
		jz   NoHighlight                          // No
		cmp  highlightContainers, eax             // Highlight containers?
		je   nextObject                           // No
NoHighlight:
		mov  edx, outlineColor
		mov  [ecx+0x74], edx
nextObject:
		call obj_find_next_at_
		jmp  loopObject
end:
		call tile_refresh_display_
		popad
		retn
	}
}

static void __declspec(naked) obj_outline_all_items_off() {
	__asm {
		pushad
		mov  eax, dword ptr ds:[_map_elevation]
		call obj_find_first_at_
loopObject:
		test eax, eax
		jz   end
		cmp  eax, ds:[_outlined_object]
		je   nextObject
		xchg ecx, eax
		mov  eax, [ecx+0x20]
		and  eax, 0xF000000
		sar  eax, 0x18
		test eax, eax                             // Is this an item?
		jnz  nextObject                           // No
		cmp  dword ptr [ecx+0x7C], eax            // Owned by someone?
		jnz  nextObject                           // Yes
		mov  dword ptr [ecx+0x74], eax
nextObject:
		call obj_find_next_at_
		jmp  loopObject
end:
		call tile_refresh_display_
		popad
		retn
	}
}

static void __declspec(naked) obj_remove_outline_hook() {
	__asm {
		call obj_remove_outline_
		test eax, eax
		jnz  end
		cmp  highlightingToggled, 1
		jne  end
		mov  ds:[_outlined_object], eax
		call obj_outline_all_items_on
end:
		retn
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

static int maxCountProto = 512;
static void __declspec(naked) proto_ptr_hack() {
	__asm {
		mov  ecx, maxCountProto;
		cmp  ecx, 4096;
		jae  skip;
		cmp  eax, ecx;
		jb   end;
		add  ecx, 256;
		mov  maxCountProto, ecx;
skip:
		cmp  eax, ecx;
end:
		retn;
	}
}

void LoadProtoAutoMaxLimit() {
	if (maxCountProto != -1) {
		MakeCall(0x4A21B2, proto_ptr_hack);
	}
}

long objUniqueID = UID_START; // saving to sfallgv.sav

// Assigns a new unique identifier to an object if it has not been previously assigned
// the identifier is saved with the object in the saved game and this can used in various script
long SetObjectUniqueID(TGameObj* obj) {
	if (obj->ID > UID_START || obj == *ptr_obj_dude) return obj->ID; // dude id = 1800. TODO: perhaps his id needs to be changed to 0x10000000

	if ((DWORD)objUniqueID >= UID_END) objUniqueID = UID_START;
	obj->ID = ++objUniqueID;
	return objUniqueID;
}

void ScriptExtenderSetup() {
	bool AllowUnsafeScripting = IsDebug
		&& GetPrivateProfileIntA("Debugging", "AllowUnsafeScripting", 0, ".\\ddraw.ini") != 0;

	toggleHighlightsKey = GetPrivateProfileIntA("Input", "ToggleItemHighlightsKey", 0, ini);
	if (toggleHighlightsKey) {
		MotionSensorMode = GetPrivateProfileIntA("Misc", "MotionScannerFlags", 1, ini);
		highlightContainers = GetPrivateProfileIntA("Input", "HighlightContainers", 0, ini);
		outlineColor = GetPrivateProfileIntA("Input", "OutlineColor", 0x10, ini);
		if (outlineColor < 1) outlineColor = 0x40;
		HookCall(0x44BD1C, &obj_remove_outline_hook);
		HookCall(0x44E559, &obj_remove_outline_hook);
	}
	GetPrivateProfileStringA("Sfall", "HighlightFail1", "You aren't carrying a motion sensor.", HighlightFail1, 128, translationIni);
	GetPrivateProfileStringA("Sfall", "HighlightFail2", "Your motion sensor is out of charge.", HighlightFail2, 128, translationIni);

	idle = GetPrivateProfileIntA("Misc", "ProcessorIdle", -1, ini);
	if (idle > -1 && idle <= 127) {
		SafeWrite32(_idle_func, (DWORD)Sleep);
		SafeWrite8(0x4C9F12, 0x6A); // push
		SafeWrite8(0x4C9F13, idle);
	}

	int maxlimit = GetPrivateProfileIntA("Misc", "LoadProtoMaxLimit", -1, ini);
	if (maxlimit != -1) {
		maxCountProto = -1;
		if (maxlimit > 512) {
			if (maxlimit > 4096) maxlimit = 4096;
			SafeWrite32(0x4A21B3, maxlimit);
		}
	}

	arraysBehavior = GetPrivateProfileIntA("Misc", "arraysBehavior", 1, ini);
	if (arraysBehavior > 0) {
		arraysBehavior = 1; // only 1 and 0 allowed at this time
		dlogr("New arrays behavior enabled.", DL_SCRIPT);
	} else dlogr("Arrays in backward-compatiblity mode.", DL_SCRIPT);

	HookCall(0x480E7B, MainGameLoopHook); //hook the main game loop
	HookCall(0x422845, CombatLoopHook); //hook the combat loop

	MakeJump(0x4A390C, FindSidHack);
	MakeJump(0x4A5E34, ScrPtrHack);

	MakeCall(0x4230D5, AfterCombatAttackHook);
	MakeJump(0x4A67F0, ExecMapScriptsHack);

	// this patch makes it possible to export variables from sfall global scripts
	HookCall(0x4414C8, Export_Export_FindVar_Hook);
	HookCall(0x441285, &Export_FetchOrStore_FindVar_Hook); // store
	HookCall(0x4413D9, &Export_FetchOrStore_FindVar_Hook); // fetch

	HookCall(0x46E141, FreeProgramHook);

	// combat_is_starting_p_proc / combat_is_over_p_proc
	HookCall(0x421B72, CombatBeginHook);
	HookCall(0x421FC1, CombatOverHook);

	dlogr("Adding additional opcodes", DL_SCRIPT);
	if (AllowUnsafeScripting) {
		dlogr("  Unsafe opcodes enabled.", DL_SCRIPT);
	} else {
		dlogr("  Unsafe opcodes disabled.", DL_SCRIPT);
	}

	SafeWrite32(0x46E370, 0x300);	//Maximum number of allowed opcodes
	SafeWrite32(0x46ce34, (DWORD)opcodes);	//cmp check to make sure opcode exists
	SafeWrite32(0x46ce6c, (DWORD)opcodes);	//call that actually jumps to the opcode
	SafeWrite32(0x46e390, (DWORD)opcodes);	//mov that writes to the opcode

	opcodes[0x156] = ReadByte;
	opcodes[0x157] = ReadShort;
	opcodes[0x158] = ReadInt;
	opcodes[0x159] = ReadString;
	opcodes[0x15a] = SetPCBaseStat;
	opcodes[0x15b] = SetPCExtraStat;
	opcodes[0x15c] = GetPCBaseStat;
	opcodes[0x15d] = GetPCExtraStat;
	opcodes[0x15e] = SetCritterBaseStat;
	opcodes[0x15f] = SetCritterExtraStat;
	opcodes[0x160] = GetCritterBaseStat;
	opcodes[0x161] = GetCritterExtraStat;
	opcodes[0x162] = funcTapKey;
	opcodes[0x163] = GetYear;
	opcodes[0x164] = GameLoaded;
	opcodes[0x165] = GraphicsFuncsAvailable;
	opcodes[0x166] = funcLoadShader;
	opcodes[0x167] = funcFreeShader;
	opcodes[0x168] = funcActivateShader;
	opcodes[0x169] = funcDeactivateShader;
	opcodes[0x16a] = SetGlobalScriptRepeat;
	opcodes[0x16b] = InputFuncsAvailable;
	opcodes[0x16c] = KeyPressed;
	opcodes[0x16d] = funcSetShaderInt;
	opcodes[0x16e] = funcSetShaderFloat;
	opcodes[0x16f] = funcSetShaderVector;
	opcodes[0x170] = funcInWorldMap;
	opcodes[0x171] = ForceEncounter;
	opcodes[0x172] = SetWorldMapPos;
	opcodes[0x173] = GetWorldMapXPos;
	opcodes[0x174] = GetWorldMapYPos;
	opcodes[0x175] = SetDMModel;
	opcodes[0x176] = SetDFModel;
	opcodes[0x177] = SetMoviePath;
	for (int i = 0x178; i < 0x189; i++) {
		opcodes[i] = funcSetPerkValue;
	}
	opcodes[0x189] = funcSetPerkName;
	opcodes[0x18a] = funcSetPerkDesc;
	opcodes[0x18b] = SetPipBoyAvailable;
	if (UsingExtraKillTypes()) {
		opcodes[0x18c] = GetKillCounter2;
		opcodes[0x18d] = ModKillCounter2;
	} else {
		opcodes[0x18c] = GetKillCounter;
		opcodes[0x18d] = ModKillCounter;
	}
	opcodes[0x18e] = GetPerkOwed;
	opcodes[0x18f] = SetPerkOwed;
	opcodes[0x190] = GetPerkAvailable;
	opcodes[0x191] = GetCritterAP;
	opcodes[0x192] = SetCritterAP;
	opcodes[0x193] = GetActiveHand;
	opcodes[0x194] = ToggleActiveHand;
	opcodes[0x195] = SetWeaponKnockback;
	opcodes[0x196] = SetTargetKnockback;
	opcodes[0x197] = SetAttackerKnockback;
	opcodes[0x198] = RemoveWeaponKnockback;
	opcodes[0x199] = RemoveTargetKnockback;
	opcodes[0x19a] = RemoveAttackerKnockback;
	opcodes[0x19b] = SetGlobalScriptType;
	opcodes[0x19c] = GetGlobalScriptTypes;
	opcodes[0x19d] = SetGlobalVar;
	opcodes[0x19e] = GetGlobalVarInt;
	opcodes[0x19f] = GetGlobalVarFloat;
	opcodes[0x1a0] = fSetPickpocketMax;
	opcodes[0x1a1] = fSetHitChanceMax;
	opcodes[0x1a2] = fSetSkillMax;
	opcodes[0x1a3] = EaxAvailable;
	//opcodes[0x1a4] = SetEaxEnvironmentFunc;
	opcodes[0x1a5] = IncNPCLevel;
	opcodes[0x1a6] = GetViewportX;
	opcodes[0x1a7] = GetViewportY;
	opcodes[0x1a8] = SetViewportX;
	opcodes[0x1a9] = SetViewportY;
	opcodes[0x1aa] = SetXpMod;
	opcodes[0x1ab] = SetPerkLevelMod;
	opcodes[0x1ac] = GetIniSetting;
	opcodes[0x1ad] = funcGetShaderVersion;
	opcodes[0x1ae] = funcSetShaderMode;
	opcodes[0x1af] = GetGameMode;
	opcodes[0x1b0] = funcForceGraphicsRefresh;
	opcodes[0x1b1] = funcGetShaderTexture;
	opcodes[0x1b2] = funcSetShaderTexture;
	opcodes[0x1b3] = funcGetTickCount;
	opcodes[0x1b4] = SetStatMax;
	opcodes[0x1b5] = SetStatMin;
	opcodes[0x1b6] = SetCarTown;
	opcodes[0x1b7] = fSetPCStatMax;
	opcodes[0x1b8] = fSetPCStatMin;
	opcodes[0x1b9] = fSetNPCStatMax;
	opcodes[0x1ba] = fSetNPCStatMin;
	opcodes[0x1bb] = fSetFakePerk;
	opcodes[0x1bc] = fSetFakeTrait;
	opcodes[0x1bd] = fSetSelectablePerk;
	opcodes[0x1be] = fSetPerkboxTitle;
	opcodes[0x1bf] = fIgnoreDefaultPerks;
	opcodes[0x1c0] = fRestoreDefaultPerks;
	opcodes[0x1c1] = fHasFakePerk;
	opcodes[0x1c2] = fHasFakeTrait;
	opcodes[0x1c3] = fAddPerkMode;
	opcodes[0x1c4] = fClearSelectablePerks;
	opcodes[0x1c5] = SetCritterHitChance;
	opcodes[0x1c6] = SetBaseHitChance;
	opcodes[0x1c7] = SetCritterSkillMod;
	opcodes[0x1c8] = SetBaseSkillMod;
	opcodes[0x1c9] = SetCritterPickpocket;
	opcodes[0x1ca] = SetBasePickpocket;
	opcodes[0x1cb] = SetPyromaniacMod;
	opcodes[0x1cc] = fApplyHeaveHoFix;
	opcodes[0x1cd] = SetSwiftLearnerMod;
	opcodes[0x1ce] = SetLevelHPMod;
	if (AllowUnsafeScripting) {
		opcodes[0x1cf] = WriteByte;
		opcodes[0x1d0] = WriteShort;
		opcodes[0x1d1] = WriteInt;
		for (int i = 0x1d2; i < 0x1dc; i++) {
			opcodes[i] = CallOffset;
		}
	}
	opcodes[0x1dc] = ShowIfaceTag;
	opcodes[0x1dd] = HideIfaceTag;
	opcodes[0x1de] = IsIfaceTagActive;
	opcodes[0x1df] = GetBodypartHitModifier;
	opcodes[0x1e0] = SetBodypartHitModifier;
	opcodes[0x1e1] = funcSetCriticalTable;
	opcodes[0x1e2] = funcGetCriticalTable;
	opcodes[0x1e3] = funcResetCriticalTable;
	opcodes[0x1e4] = GetSfallArg;
	opcodes[0x1e5] = SetSfallReturn;
	opcodes[0x1e6] = SetApAcBonus;
	opcodes[0x1e7] = GetApAcBonus;
	opcodes[0x1e8] = SetApAcEBonus;
	opcodes[0x1e9] = GetApAcEBonus;
	opcodes[0x1ea] = InitHook;
	opcodes[0x1eb] = GetIniString;
	opcodes[0x1ec] = funcSqrt;
	opcodes[0x1ed] = funcAbs;
	opcodes[0x1ee] = funcSin;
	opcodes[0x1ef] = funcCos;
	opcodes[0x1f0] = funcTan;
	opcodes[0x1f1] = funcATan;
	opcodes[0x1f2] = SetPalette;
	opcodes[0x1f3] = RemoveScript;
	opcodes[0x1f4] = SetScript;
	opcodes[0x1f5] = GetScript;
	opcodes[0x1f6] = NBCreateChar;
	opcodes[0x1f7] = fs_create;
	opcodes[0x1f8] = fs_copy;
	opcodes[0x1f9] = fs_find;
	opcodes[0x1fa] = fs_write_byte;
	opcodes[0x1fb] = fs_write_short;
	opcodes[0x1fc] = fs_write_int;
	opcodes[0x1fd] = fs_write_int;
	opcodes[0x1fe] = fs_write_string;
	opcodes[0x1ff] = fs_delete;
	opcodes[0x200] = fs_size;
	opcodes[0x201] = fs_pos;
	opcodes[0x202] = fs_seek;
	opcodes[0x203] = fs_resize;
	opcodes[0x204] = get_proto_data;
	opcodes[0x205] = set_proto_data;
	opcodes[0x206] = set_self;
	opcodes[0x207] = register_hook;
	opcodes[0x208] = fs_write_bstring;
	opcodes[0x209] = fs_read_byte;
	opcodes[0x20a] = fs_read_short;
	opcodes[0x20b] = fs_read_int;
	opcodes[0x20c] = fs_read_float;
	opcodes[0x20d] = list_begin;
	opcodes[0x20e] = list_next;
	opcodes[0x20f] = list_end;
	opcodes[0x210] = sfall_ver_major;
	opcodes[0x211] = sfall_ver_minor;
	opcodes[0x212] = sfall_ver_build;
	opcodes[0x213] = funcHeroSelectWin;
	opcodes[0x214] = funcSetHeroRace;
	opcodes[0x215] = funcSetHeroStyle;
	opcodes[0x216] = set_critter_burst_disable;
	opcodes[0x217] = get_weapon_ammo_pid;
	opcodes[0x218] = set_weapon_ammo_pid;
	opcodes[0x219] = get_weapon_ammo_count;
	opcodes[0x21a] = set_weapon_ammo_count;
	if (AllowUnsafeScripting) {
		opcodes[0x21b] = WriteString;
	}
	opcodes[0x21c] = get_mouse_x;
	opcodes[0x21d] = get_mouse_y;
	opcodes[0x21e] = get_mouse_buttons;
	opcodes[0x21f] = get_window_under_mouse;
	opcodes[0x220] = get_screen_width;
	opcodes[0x221] = get_screen_height;
	opcodes[0x222] = stop_game;
	opcodes[0x223] = resume_game;
	opcodes[0x224] = create_message_window;
	opcodes[0x225] = remove_trait;
	opcodes[0x226] = get_light_level;
	opcodes[0x227] = refresh_pc_art;
	opcodes[0x228] = get_attack_type;
	opcodes[0x229] = ForceEncounterWithFlags;
	opcodes[0x22a] = set_map_time_multi;
	opcodes[0x22b] = play_sfall_sound;
	opcodes[0x22c] = stop_sfall_sound;
	opcodes[0x22d] = create_array;
	opcodes[0x22e] = set_array;
	opcodes[0x22f] = get_array;
	opcodes[0x230] = free_array;
	opcodes[0x231] = len_array;
	opcodes[0x232] = resize_array;
	opcodes[0x233] = temp_array;
	opcodes[0x234] = fix_array;
	opcodes[0x235] = string_split;
	opcodes[0x236] = list_as_array;
	opcodes[0x237] = str_to_int;
	opcodes[0x238] = str_to_flt;
	opcodes[0x239] = scan_array;
	opcodes[0x23a] = get_tile_pid;
	opcodes[0x23b] = modified_ini;
	opcodes[0x23c] = GetSfallArgs;
	opcodes[0x23d] = SetSfallArg;
	opcodes[0x23e] = force_aimed_shots;
	opcodes[0x23f] = disable_aimed_shots;
	opcodes[0x240] = mark_movie_played;
	opcodes[0x241] = get_npc_level;
	opcodes[0x242] = set_critter_skill_points;
	opcodes[0x243] = get_critter_skill_points;
	opcodes[0x244] = set_available_skill_points;
	opcodes[0x245] = get_available_skill_points;
	opcodes[0x246] = mod_skill_points_per_level;
	opcodes[0x247] = set_perk_freq;
	opcodes[0x248] = get_last_attacker;
	opcodes[0x249] = get_last_target;
	opcodes[0x24a] = block_combat;
	opcodes[0x24b] = tile_under_cursor;
	opcodes[0x24c] = gdialog_get_barter_mod;
	opcodes[0x24d] = set_inven_ap_cost;
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
	opcodes[0x262] = register_hook_proc;
	opcodes[0x263] = funcPow;
	opcodes[0x264] = funcLog;
	opcodes[0x265] = funcExp;
	opcodes[0x266] = funcCeil;
	opcodes[0x267] = funcRound;
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

	InitOpcodeMetaTable();
	InitMetaruleTable();
}


DWORD GetScriptProcByName(DWORD scriptPtr, const char* procName) {
	DWORD result;
	__asm {
		mov edx, procName;
		mov eax, scriptPtr;
		call interpretFindProcedure_;
		mov result, eax;
	}
	return result;
}

// loads script from .int file into a sScriptProgram struct, filling script pointer and proc lookup table
void LoadScriptProgram(sScriptProgram &prog, const char* fileName) {
	DWORD scriptPtr;
	__asm {
		mov eax, fileName;
		call loadProgram_;
		mov scriptPtr, eax;
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
			mov eax, progPtr;
			call runProgram_;
			mov edx, -1;
			mov eax, progPtr;
			call interpret_;
		}
		prog.initialized = 1;
	}
}

void AddProgramToMap(sScriptProgram &prog) {
	sfallProgsMap[prog.ptr] = prog;
}

sScriptProgram* GetGlobalScriptProgram(DWORD scriptPtr) {
	for (std::vector<sGlobalScript>::iterator it = globalScripts.begin(); it != globalScripts.end(); it++) {
		if (it->prog.ptr == scriptPtr) return &it->prog;
	}
	return NULL;
}

bool _stdcall isGameScript(const char* filename) {
	if (strlen(filename) > 8) return false;
	for (DWORD i = 0; i < *(DWORD*)_maxScriptNum; i++) {
		if (strcmp(filename, (char*)(*(DWORD*)_scriptListInfo + i * 20)) == 0) return true;
	}
	return false;
}

// this runs after the game was loaded/started
void LoadGlobalScripts() {
	isGameLoading = false;
	HookScriptInit();
	dlogr("Loading global scripts", DL_SCRIPT|DL_INIT);

	char* name = "scripts\\gl*.int";
	DWORD count, *filenames;
	__asm {
		xor  ecx, ecx
		xor  ebx, ebx
		lea  edx, filenames
		mov  eax, name
		call db_get_file_list_
		mov  count, eax
	}

	// TODO: refactor script programs
	sScriptProgram prog;
	for (DWORD i = 0; i < count; i++) {
		name = _strlwr((char*)filenames[i]);

		std::string baseName(name);
		int lastDot = baseName.find_last_of('.');
		if ((baseName.length() - lastDot) > 4) continue; // skip files with invalid extension (bug in db_get_file_list fuction)

		baseName = baseName.substr(0, lastDot); // script name without extension
		if (!isGameScript(baseName.c_str())) {
			dlog(">", DL_SCRIPT);
			dlog(name, DL_SCRIPT);
			isGlobalScriptLoading = 1;
			LoadScriptProgram(prog, baseName.c_str());
			if (prog.ptr) {
				dlogr(" Done", DL_SCRIPT);
				DWORD idx;
				sGlobalScript gscript = sGlobalScript(prog);
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

	__asm {
		xor  edx, edx
		lea  eax, filenames
		call db_free_file_list_
	}

	dlogr("Finished loading global scripts.", DL_SCRIPT|DL_INIT);
}

static DWORD _stdcall ScriptHasLoaded(DWORD script) {
	for (DWORD d = 0; d < checkedScripts.size(); d++) {
		if (checkedScripts[d] == script) return 0;
	}
	checkedScripts.push_back(script);
	return 1;
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
	//xp mod
	SafeWrite8(0x4AFAB8, 0x53);
	SafeWrite32(0x4AFAB9, 0x55575651);
	//HP bonus
	SafeWrite8(0x4AFBC1, 2);
	//Stat ranges
	StatsReset();
	//Bodypart hit chances
	*((DWORD*)0x510954) = GetPrivateProfileIntA("Misc", "BodyHit_Head",           0xFFFFFFD8, ini);
	*((DWORD*)0x510958) = GetPrivateProfileIntA("Misc", "BodyHit_Left_Arm",       0xFFFFFFE2, ini);
	*((DWORD*)0x51095C) = GetPrivateProfileIntA("Misc", "BodyHit_Right_Arm",      0xFFFFFFE2, ini);
	*((DWORD*)0x510960) = GetPrivateProfileIntA("Misc", "BodyHit_Torso_Uncalled", 0x00000000, ini);
	*((DWORD*)0x510964) = GetPrivateProfileIntA("Misc", "BodyHit_Right_Leg",      0xFFFFFFEC, ini);
	*((DWORD*)0x510968) = GetPrivateProfileIntA("Misc", "BodyHit_Left_Leg",       0xFFFFFFEC, ini);
	*((DWORD*)0x51096C) = GetPrivateProfileIntA("Misc", "BodyHit_Eyes",           0xFFFFFFC4, ini);
	*((DWORD*)0x510970) = GetPrivateProfileIntA("Misc", "BodyHit_Groin",          0xFFFFFFE2, ini);
	*((DWORD*)0x510974) = GetPrivateProfileIntA("Misc", "BodyHit_Torso_Uncalled", 0x00000000, ini);
	//skill points per level mod
	SafeWrite8(0x43C27a, 5);
}

void RunScriptProcByNum(DWORD sptr, DWORD procNum) {
	__asm {
		mov edx, procNum;
		mov eax, sptr;
		call executeProcedure_
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
		DWORD sptr = prog->ptr;
		DWORD procNum = prog->procLookup[procId];
		if (procNum != -1) {
			RunScriptProcByNum(sptr, procNum);
		}
	}
}

static void RunScript(sGlobalScript* script) {
	script->count = 0;
	RunScriptProc(&script->prog, start); // run "start"
}

/**
	Do some clearing after each frame:
	- delete all temp arrays
	- reset reg_anim_* combatstate checks
*/
static void ResetStateAfterFrame() {
	if (!tempArrays.empty()) {
		for (std::set<DWORD>::iterator it = tempArrays.begin(); it != tempArrays.end(); ++it)
			FreeArray(*it);
		tempArrays.clear();
	}
	RegAnimCombatCheck(1);
}

/**
	Do some cleaning after each combat attack action
*/
void AfterAttackCleanup() {
	ResetExplosionSettings();
}

static void RunGlobalScripts1() {
	if (idle > -1 && idle <= 127) Sleep(idle);
	if (toggleHighlightsKey) {
		//0x48C294 to toggle
		if (KeyDown(toggleHighlightsKey)) {
			if (!highlightingToggled) {
				if (MotionSensorMode & 4) {
					DWORD scanner;
					__asm {
						mov eax, ds:[_obj_dude];
						mov edx, PID_MOTION_SENSOR;
						call inven_pid_is_carried_ptr_;
						mov scanner, eax;
					}
					if (scanner) {
						if (!(MotionSensorMode & 2)) {
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
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (!globalScripts[d].repeat || (globalScripts[d].mode != 0 && globalScripts[d].mode != 3)) continue;
		if (++globalScripts[d].count >= globalScripts[d].repeat) {
			RunScript(&globalScripts[d]);
		}
	}
	ResetStateAfterFrame();
}

void RunGlobalScripts2() {
	if (idle > -1 && idle <= 127) Sleep(idle);
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (!globalScripts[d].repeat || globalScripts[d].mode != 1) continue;
		if (++globalScripts[d].count >= globalScripts[d].repeat) {
			RunScript(&globalScripts[d]);
		}
	}
	ResetStateAfterFrame();
}

void RunGlobalScripts3() {
	if (idle > -1 && idle <= 127) Sleep(idle);
	for (DWORD d = 0; d < globalScripts.size(); d++) {
		if (!globalScripts[d].repeat || (globalScripts[d].mode != 2 && globalScripts[d].mode != 3)) continue;
		if (++globalScripts[d].count >= globalScripts[d].repeat) {
			RunScript(&globalScripts[d]);
		}
	}
	ResetStateAfterFrame();
}

static DWORD _stdcall HandleMapUpdateForScripts(const DWORD procId) {
	if (procId == map_enter_p_proc) {
		// map changed, all game objects were destroyed and scripts detached, need to re-insert global scripts into the game
		for (SfallProgsMap::iterator it = sfallProgsMap.begin(); it != sfallProgsMap.end(); it++) {
			DWORD progPtr = it->second.ptr;
			__asm {
				mov eax, progPtr;
				call runProgram_;
			}
		}
	}
	RunGlobalScriptsAtProc(procId); // gl* scripts of types 0 and 3
	RunHookScriptsAtProc(procId); // all hs_ scripts

	return procId; // restore eax (don't delete)
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
	if (unused != 4) return;
	sGlobalVar var;
	for (DWORD i = 0; i < count; i++) {
		ReadFile(h, &var, sizeof(sGlobalVar), &unused, 0);
		globalVars.insert(glob_pair(var.id, var.val));
	}
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

void GetGlobals(sGlobalVar* globals) {
	glob_citr itr = globalVars.begin();
	int i = 0;
	while (itr != globalVars.end()) {
		globals[i].id = itr->first;
		globals[i++].val = itr->second;
		itr++;
	}
}

void SetGlobals(sGlobalVar* globals) {
	glob_itr itr = globalVars.begin();
	int i = 0;
	while (itr != globalVars.end()) {
		itr->second = globals[i++].val;
		itr++;
	}
}

//fuctions to load and save appearance globals
void SetAppearanceGlobals(int race, int style) {
	SetGlobalVar2("HAp_Race", race);
	SetGlobalVar2("HApStyle", style);
}

void GetAppearanceGlobals(int *race, int *style) {
	*race = GetGlobalVar2("HAp_Race");
	*style = GetGlobalVar2("HApStyle");
}
