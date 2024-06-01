/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#pragma once

#include <array>
#include <unordered_map>

#include "..\..\FalloutEngine\Structs.h"
#include "ScriptValue.h"

namespace sfall
{
namespace script
{

#define OP_MAX_ARGUMENTS	(10)

class OpcodeContext;

typedef void(*ScriptingFunctionHandler)(OpcodeContext&);

// The type of argument, not the same as actual data type. Useful for validation.
enum OpcodeArgumentType {
	ARG_ANY = 0, // no validation (default)
	ARG_INT,     // integer only
	ARG_OBJECT,  // integer that is not 0
	ARG_STRING,  // string only
	ARG_INTSTR,  // integer OR string
	ARG_NUMBER,  // float OR integer
};

typedef struct SfallOpcodeInfo {
	// opcode number
	short opcode;

	// opcode name
	const char name[32];

	// opcode handler
	ScriptingFunctionHandler handler;

	// number of arguments
	int argNum;

	// has return value or not
	bool hasReturn;

	// default return value when an error occurs during validation of function arguments
	long errValue;

	// argument validation settings
	OpcodeArgumentType argValidation[OP_MAX_ARGUMENTS];
} SfallOpcodeInfo;


typedef struct SfallMetarule {
	// function name
	const char* name;

	// pointer to handler function
	ScriptingFunctionHandler func;

	// minimum number of arguments
	short minArgs;

	// maximum number of arguments
	short maxArgs;

	// default return value when an error occurs during validation of function arguments
	long errValue;

	// argument validation settings
	OpcodeArgumentType argValidation[OP_MAX_ARGUMENTS];
} SfallMetarule;

// A context for handling opcodes. Opcode handlers can retrieve arguments and set opcode return value via context.
class OpcodeContext {
public:
	// program - pointer to script program (from the engine)
	// opcode - opcode number
	// argNum - number of arguments for this opcode
	// hasReturn - true if opcode has return value (is expression)
	OpcodeContext(fo::Program* program, DWORD opcode, int argNum, bool hasReturn);
	OpcodeContext(fo::Program* program, const SfallOpcodeInfo* info);

	const char* getOpcodeName() const;
	const char* getMetaruleName() const;

	// currently executed metarule function
	void setMetarule(const SfallMetarule* metarule);
	const SfallMetarule* getMetarule() const;

	// number of arguments, possibly reduced by argShift
	int numArgs() const;

	// true if the current opcode is supposed to return some value
	bool hasReturn() const;

	// returns current argument shift, default is 0
	int argShift() const;

	// sets shift value for arguments
	// for example if shift value is 2, then subsequent calls to arg(i) will return arg(i+2) instead, etc.
	void setArgShift(int shift);

	// returns argument with given index, possible shifted by argShift
	const ScriptValue& arg(int index) const;

	// current return value
	const ScriptValue& returnValue() const;

	// current script program
	fo::Program* program() const;

	// current opcode number
	DWORD opcode() const;

	// set return value for current opcode
	void setReturn(unsigned long value, DataType type);

	// set return value for current opcode
	void setReturn(const ScriptValue& val);

	// writes error message to debug.log along with the name of script & procedure
	void printOpcodeError(const char* fmt, ...) const;

	// Validate opcode arguments against type specification
	// if validation pass, returns true, otherwise writes error to debug.log and returns false
	bool validateArguments(const OpcodeArgumentType argInfo[], const char* opcodeName) const;

	// Handle opcodes
	// func - opcode handler
	void handleOpcode(ScriptingFunctionHandler func);

	// Handle opcodes with argument validation
	// func - opcode handler
	// argTypes - argument types for validation
	void handleOpcode(ScriptingFunctionHandler func, const OpcodeArgumentType argTypes[]);

	// handles opcode using default instance
	static void __stdcall handleOpcodeStatic(fo::Program* program, DWORD opcodeOffset, ScriptingFunctionHandler func, char argNum, bool hasReturn);

	//static const char* getSfallTypeName(DWORD dataType);

	static DataType getSfallTypeByScriptType(DWORD varType);

	static DWORD getScriptTypeBySfallType(DataType dataType);

private:
	std::array<ScriptValue, OP_MAX_ARGUMENTS> _args;
	ScriptValue _ret;

	fo::Program* _program;
	DWORD _opcode;
	const char* _opcodeName; // name of opcode function (for error logging)

	const SfallMetarule* _metarule;

	int  _numArgs;
	bool _hasReturn;
	int  _argShift;
	long _errorVal; // error value for incorrect arguments

	// pops arguments from data stack
	void _popArguments();
	// pushes return value to data stack
	void _pushReturnValue();
};

}
}
