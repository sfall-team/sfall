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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\ScriptExtender.h"

#include "OpcodeContext.h"

namespace sfall
{
namespace script
{

OpcodeContext::OpcodeContext(fo::Program* program, DWORD opcode, int argNum, bool hasReturn)
		: _program(program), _opcode(opcode), _numArgs(argNum), _hasReturn(hasReturn), _errorVal(-1), _argShift(0)
{
	assert(argNum < OP_MAX_ARGUMENTS);
}

OpcodeContext::OpcodeContext(fo::Program* program, const SfallOpcodeInfo* info)
		: _program(program), _opcode(info->opcode), _numArgs(info->argNum), _hasReturn(info->hasReturn),
		  _opcodeName(info->name), _errorVal(info->errValue), _argShift(0)
{
	assert(_numArgs < OP_MAX_ARGUMENTS);
}

const char* OpcodeContext::getOpcodeName() const {
	return _opcodeName;
}

const char* OpcodeContext::getMetaruleName() const {
	return _metarule->name;
}

const SfallMetarule* OpcodeContext::getMetarule() const {
	return _metarule;
}

void OpcodeContext::setMetarule(const SfallMetarule* metarule) {
	_metarule = metarule;
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
	assert((index + _argShift) < OP_MAX_ARGUMENTS);
	return _args[index + _argShift];
}

const ScriptValue& OpcodeContext::returnValue() const {
	return _ret;
}

fo::Program* OpcodeContext::program() const {
	return _program;
}

DWORD OpcodeContext::opcode() const {
	return _opcode;
}

void OpcodeContext::setReturn(unsigned long value, DataType type) {
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
	vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, args);
	va_end(args);

	const char* procName = fo::func::findCurrentProc(_program);
	fo::func::debug_printf("\nOPCODE ERROR: %s\n > Script: %s, procedure %s.", msg, _program->fileName, procName);
}

bool OpcodeContext::validateArguments(const OpcodeArgumentType argTypes[], const char* opcodeName) const {
	for (int i = 0; i < _numArgs; i++) {
		auto actualType = arg(i).type();
		// display invalid type error if type is set and differs from actual type
		// exception is when type set to
		if (actualType == DataType::NONE) break;
		auto argType = argTypes[i];
		switch (argType) {
		case ARG_ANY:
			continue;
		case ARG_INT:
		case ARG_OBJECT:
			if (actualType != DataType::INT) {
				printOpcodeError("%s() - argument #%d is not an integer.", opcodeName, ++i);
				return false;
			}
			if (argType == ARG_OBJECT && arg(i).rawValue() == 0) {
				printOpcodeError("%s() - argument #%d is null.", opcodeName, ++i);
				return false;
			}
			break;
		case ARG_STRING:
			if (actualType != DataType::STR) {
				printOpcodeError("%s() - argument #%d is not a string.", opcodeName, ++i);
				return false;
			}
			break;
		case ARG_INTSTR:
			if (actualType != DataType::STR && actualType != DataType::INT) {
				printOpcodeError("%s() - argument #%d is not an integer or a string.", opcodeName, ++i);
				return false;
			}
			break;
		case ARG_NUMBER:
			if (actualType != DataType::FLOAT && actualType != DataType::INT) {
				printOpcodeError("%s() - argument #%d is not a number.", opcodeName, ++i);
				return false;
			}
		}
	}
	return true;
}

void OpcodeContext::handleOpcode(ScriptingFunctionHandler func) {
	if (_numArgs) _popArguments();

	func(*this);

	if (_hasReturn) _pushReturnValue();
}

void OpcodeContext::handleOpcode(ScriptingFunctionHandler func, const OpcodeArgumentType argTypes[]) {
	if (_numArgs) _popArguments();

	if (!_numArgs || validateArguments(argTypes, _opcodeName)) {
		func(*this);
	} else if (_hasReturn) {
		setReturn(_errorVal); // is a common practice to return -1 in case of errors in fallout engine
	}

	if (_hasReturn) _pushReturnValue();
}

// unused method
void __stdcall OpcodeContext::handleOpcodeStatic(fo::Program* program, DWORD opcodeOffset, ScriptingFunctionHandler func, char argNum, bool hasReturn) {
	// for each opcode create new context on stack (no allocations at this point)
	OpcodeContext currentContext(program, opcodeOffset / 4, argNum, hasReturn);
	// handle the opcode using provided handler
	currentContext.handleOpcode(func);
}

/*
const char* OpcodeContext::getSfallTypeName(DWORD dataType) {
	switch (dataType) {
	case DataType::NONE:
		return "(none)";
	case DataType::STR:
		return "string";
	case DataType::FLOAT:
		return "float";
	case DataType::INT:
		return "integer";
	default:
		return "(unknown)";
	}
}
*/

DataType OpcodeContext::getSfallTypeByScriptType(DWORD varType) {
	switch (varType & 0xFFFF) {
	case VAR_TYPE_STR2:
	case VAR_TYPE_STR:
		return DataType::STR;
	case VAR_TYPE_FLOAT:
		return DataType::FLOAT;
	case VAR_TYPE_INT:
	default:
		return DataType::INT;
	}
}

DWORD OpcodeContext::getScriptTypeBySfallType(DataType dataType) {
	switch (dataType) {
		case DataType::STR:
			return VAR_TYPE_STR;
		case DataType::FLOAT:
			return VAR_TYPE_FLOAT;
		case DataType::INT:
		default:
			return VAR_TYPE_INT;
	}
}

void OpcodeContext::_popArguments() {
	// process arguments on stack (reverse order)
	for (int i = _numArgs - 1; i >= 0; i--) {
		// get argument from stack
		DWORD rawValueType;
		DWORD rawValue = fo::func::interpretGetValue(_program, rawValueType);
		_args[i] = ScriptValue(getSfallTypeByScriptType(rawValueType), rawValue);
	}
}

void OpcodeContext::_pushReturnValue() {
	if (_ret.type() == DataType::NONE) {
		_ret = ScriptValue(0); // if no value was set in handler, force return 0 to avoid stack error
	}
	fo::func::interpretReturnValue(_program, _ret.rawValue(), getScriptTypeBySfallType(_ret.type()));
}

}
}
