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

#include "..\..\main.h"

#include "..\..\FalloutEngine\Fallout2.h"
#include "..\ScriptExtender.h"

#include "OpcodeContext.h"

OpcodeContext::OpcodeContext(TProgram* program, DWORD opcode, int argNum, bool hasReturn) {
	assert(argNum < OP_MAX_ARGUMENTS);

	_program = program;
	_opcode = opcode;

	_numArgs = argNum;
	_hasReturn = hasReturn;
	_argShift = 0;
}

int OpcodeContext::numArgs() const {
	return _numArgs - _argShift;
}

bool OpcodeContext::hasReturn() const {
	return _hasReturn;
}

int OpcodeContext::argShift() const {
	return _argShift;
}

void OpcodeContext::setArgShift (int shift) {
	assert(shift >= 0);

	_argShift = shift;
}

const ScriptValue& OpcodeContext::arg(int index) const {
	return _args.at(index + _argShift);
}

const ScriptValue& OpcodeContext::returnValue() const {
	return _ret;
}

TProgram* OpcodeContext::program() const {
	return _program;
}

DWORD OpcodeContext::opcode() const {
	return _opcode;
}

void OpcodeContext::setReturn(unsigned long value, SfallDataType type) {
	_ret = ScriptValue(type, value);
}

void OpcodeContext::setReturn(const ScriptValue& val) {
	_ret = val;
}

void OpcodeContext::printOpcodeError(const char* fmt, ...) const {
	assert(_program != nullptr);

	va_list args;
	va_start(args, fmt);
	char msg[1024];
	vsnprintf_s(msg, sizeof msg, _TRUNCATE, fmt, args);
	va_end(args);

	const char* procName = Wrapper::findCurrentProc(_program);
	Wrapper::debug_printf("\nOPCODE ERROR: %s\n   Current script: %s, procedure %s.", msg, _program->fileName, procName);
}

bool OpcodeContext::validateArguments(const OpcodeArgumentType argTypes[], const char* opcodeName) const {
	for (int i = 0; i < _numArgs; i++) {
		auto argType = argTypes[i];
		auto actualType = arg(i).type();
		// display invalid type error if type is set and differs from actual type
		// exception is when type set to 
		if ((argType == ARG_INT || argType == ARG_OBJECT) && !(actualType == DATATYPE_INT)) {
			printOpcodeError("%s() - argument #%d is not an integer.", opcodeName, i);
			return false;
		} else if (argType == ARG_NUMBER && !(actualType == DATATYPE_INT || actualType == DATATYPE_FLOAT)) {
			printOpcodeError("%s() - argument #%d is not a number.", opcodeName, i);
			return false;
		} else if (argType == ARG_STRING && !(actualType == DATATYPE_STR)) {
			printOpcodeError("%s() - argument #%d is not a string.", opcodeName, i);
			return false;
		} else if (argType == ARG_OBJECT && arg(i).rawValue() == 0) {
			printOpcodeError("%s() - argument #%d is null.", opcodeName, i);
			return false;
		}
	}
	return true;
}

void OpcodeContext::handleOpcode(ScriptingFunctionHandler func) {
	_popArguments();

	func(*this);

	_pushReturnValue();
}

void OpcodeContext::handleOpcode(ScriptingFunctionHandler func, const OpcodeArgumentType argTypes[], const char* opcodeName) {
	_popArguments();

	if (validateArguments(argTypes, opcodeName)) {
		func(*this);
	} else if (_hasReturn) {
		// is a common practice to return -1 in case of errors in fallout engine
		setReturn(-1);
	}

	_pushReturnValue();
}

void __stdcall OpcodeContext::handleOpcodeStatic(TProgram* program, DWORD opcodeOffset, ScriptingFunctionHandler func, int argNum, bool hasReturn) {
	// for each opcode create new context on stack (no allocations at this point)
	OpcodeContext currentContext(program, opcodeOffset / 4, argNum, hasReturn);
	// handle the opcode using provided handler
	currentContext.handleOpcode(func);
}

const char* OpcodeContext::getSfallTypeName(DWORD dataType) {
	switch (dataType) {
		case DATATYPE_NONE:
			return "(none)";
		case DATATYPE_STR:
			return "string";
		case DATATYPE_FLOAT:
			return "float";
		case DATATYPE_INT:
			return "integer";
		default:
			return "(unknown)";
	}
}

DWORD OpcodeContext::getSfallTypeByScriptType(DWORD varType) {
	varType &= 0xffff;
	switch (varType) {
		case VAR_TYPE_STR:
		case VAR_TYPE_STR2:
			return DATATYPE_STR;
		case VAR_TYPE_FLOAT:
			return DATATYPE_FLOAT;
		case VAR_TYPE_INT:
		default:
			return DATATYPE_INT;
	}
}

DWORD OpcodeContext::getScriptTypeBySfallType(DWORD dataType) {
	switch (dataType) {
		case DATATYPE_STR:
			return VAR_TYPE_STR;
		case DATATYPE_FLOAT:
			return VAR_TYPE_FLOAT;
		case DATATYPE_INT:
		default:
			return VAR_TYPE_INT;
	}
}

void OpcodeContext::_popArguments() {	
	// process arguments on stack (reverse order)
	for (int i = _numArgs - 1; i >= 0; i--) {
		// get argument from stack
		DWORD rawValueType = Wrapper::interpretPopShort(_program);
		DWORD rawValue = Wrapper::interpretPopLong(_program);
		SfallDataType type = static_cast<SfallDataType>(getSfallTypeByScriptType(rawValueType));

		// retrieve string argument
		if (type == DATATYPE_STR) {
			_args.at(i) = Wrapper::interpretGetString(_program, rawValueType, rawValue);
		} else {
			_args.at(i) = ScriptValue(type, rawValue);
		}
	}
}

void OpcodeContext::_pushReturnValue() {
	if (_hasReturn) {
		if (_ret.type() == DATATYPE_NONE) {
			// if no value was set in handler, force return 0 to avoid stack error
			_ret = ScriptValue(0);
		}
		DWORD rawResult = _ret.rawValue();
		if (_ret.type() == DATATYPE_STR) {
			rawResult = Wrapper::interpretAddString(_program, _ret.asString());
		}
		Wrapper::interpretPushLong(_program, rawResult);
		Wrapper::interpretPushShort(_program, getScriptTypeBySfallType(_ret.type()));
	}
}
