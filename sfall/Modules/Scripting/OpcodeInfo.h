#pragma once

#include "ScriptValue.h"

typedef struct OpcodeArgumentInfo {
	// the type of argument, NONE means any type
	SfallDataType type;
	// set true to check for null (0) value - useful for objects, arrays, etc.
	bool notNull;
} OpcodeArgumentInfo;

typedef struct SfallOpcodeInfo {
	// opcode number
	int opcode;

	// opcode name
	const char name[32];

	// opcode handler
	ScriptingFunctionHandler handler;

	// number of arguments
	int argNum;

	// has return value or not
	bool hasReturn;

	// argument validation settings
	OpcodeArgumentInfo argValidation[OP_MAX_ARGUMENTS];
} SfallOpcodeInfo;

#define ARG_ANY		{DATATYPE_NONE}
#define ARG_INT		{DATATYPE_INT}
#define ARG_FLOAT	{DATATYPE_FLOAT}
#define ARG_STRING	{DATATYPE_STR}
#define ARG_OBJECT	{DATATYPE_INT, true}
