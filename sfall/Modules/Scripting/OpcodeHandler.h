/*
* sfall
* Copyright (C) 2008-2016 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <unordered_map>
#include <vector>

#include "..\..\FalloutEngine\Structs.h"
#include "ScriptValue.h"

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

class OpcodeHandler {
public:
	OpcodeHandler();

	void addOpcodeMetaData(const SfallOpcodeMetadata* data);

	// number of arguments, possibly reduced by argShift
	int numArgs() const;

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
	TProgram* program() const;
	
	// set return value for current opcode
	void setReturn(unsigned long value, SfallDataType type);
	
	// set return value for current opcode
	void setReturn(const ScriptValue& val);

	// resets the state of handler for new opcode invocation
	void resetState(TProgram* program, int argNum);
	
	// writes error message to debug.log along with the name of script & procedure
	void printOpcodeError(const char* fmt, ...) const;

	// Validate opcode arguments against type masks
	// if validation pass, returns true, otherwise writes error to debug.log and returns false
	bool validateArguments(const int argTypeMasks[], int argCount, const char* opcodeName) const;

	// validate opcode arguments given opcode handler function and actual number of arguments
	bool validateArguments(void (*func)(), int argNum) const;

	// Handle opcodes
	// scriptPtr - pointer to script program (from the engine)
	// func - opcode handler
	// hasReturn - true if opcode has return value (is expression)
	void __thiscall handleOpcode(TProgram* program, void(*func)(), int argNum, bool hasReturn);

	static const char* getSfallTypeName(DWORD dataType);
	static DWORD getSfallTypeByScriptType(DWORD varType);
	static DWORD getScriptTypeBySfallType(DWORD dataType);

private:
	TProgram* _program;

	int _argShift;
	std::vector<ScriptValue> _args;
	ScriptValue _ret;
	OpcodeMetaTableType _opcodeMetaTable;
};