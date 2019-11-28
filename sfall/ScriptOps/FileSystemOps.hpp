/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include "main.h"
#include "FileSystem.h"
#include "ScriptExtender.h"

//file system functions
static void fs_create2() {
	const ScriptValue &pathArg = opHandler.arg(0),
					  &sizeArg = opHandler.arg(1);

	if (pathArg.isString() && sizeArg.isInt()) {
		const char* path = pathArg.asString();
		int size = sizeArg.asInt();
		opHandler.setReturn(FScreate(path, size), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_create");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) fs_create() {
	_WRAP_OPCODE(fs_create2, 2, 1)
}

static void fs_copy2() {
	const ScriptValue &pathArg = opHandler.arg(0),
					  &srcArg = opHandler.arg(1);

	if (pathArg.isString() && srcArg.isString()) {
		const char* path = pathArg.asString();
		const char* src = srcArg.asString();
		opHandler.setReturn(FScopy(path, src), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_copy");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) fs_copy() {
	_WRAP_OPCODE(fs_copy2, 2, 1)
}

static void fs_find2() {
	const ScriptValue &pathArg = opHandler.arg(0);

	if (pathArg.isString()) {
		const char* path = pathArg.asString();
		opHandler.setReturn(FSfind(path), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_find");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) fs_find() {
	_WRAP_OPCODE(fs_find2, 1, 1)
}

static void fs_write_byte2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &dataArg = opHandler.arg(1);

	if (idArg.isInt() && dataArg.isInt()) {
		int id = idArg.asInt(), data = dataArg.asInt();
		FSwrite_byte(id, data);
	} else {
		OpcodeInvalidArgs("fs_write_byte");
	}
}

static void __declspec(naked) fs_write_byte() {
	_WRAP_OPCODE(fs_write_byte2, 2, 0)
}

static void fs_write_short2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &dataArg = opHandler.arg(1);

	if (idArg.isInt() && dataArg.isInt()) {
		int id = idArg.asInt(), data = dataArg.asInt();
		FSwrite_short(id, data);
	} else {
		OpcodeInvalidArgs("fs_write_short");
	}
}

static void __declspec(naked) fs_write_short() {
	_WRAP_OPCODE(fs_write_short2, 2, 0)
}

static void fs_write_int2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &dataArg = opHandler.arg(1);

	if (idArg.isInt() && (dataArg.isInt() || dataArg.isFloat())) {
		int id = idArg.asInt(), data = dataArg.asInt();
		FSwrite_int(id, data);
	} else {
		OpcodeInvalidArgs("fs_write_int");
	}
}

static void __declspec(naked) fs_write_int() {
	_WRAP_OPCODE(fs_write_int2, 2, 0)
}

static void fs_write_string2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &dataArg = opHandler.arg(1);

	if (idArg.isInt() && dataArg.isString()) {
		int id = idArg.asInt();
		const char* data = dataArg.asString();
		FSwrite_string(id, data);
	} else {
		OpcodeInvalidArgs("fs_write_string");
	}
}

static void __declspec(naked) fs_write_string() {
	_WRAP_OPCODE(fs_write_string2, 2, 0)
}

static void fs_write_bstring2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &dataArg = opHandler.arg(1);

	if (idArg.isInt() && dataArg.isString()) {
		int id = idArg.asInt();
		const char* data = dataArg.asString();
		FSwrite_bstring(id, data);
	} else {
		OpcodeInvalidArgs("fs_write_bstring");
	}
}

static void __declspec(naked) fs_write_bstring() {
	_WRAP_OPCODE(fs_write_bstring2, 2, 0)
}

static void fs_read_byte2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		opHandler.setReturn(FSread_byte(id), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_read_byte");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) fs_read_byte() {
	_WRAP_OPCODE(fs_read_byte2, 1, 1)
}

static void fs_read_short2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		opHandler.setReturn(FSread_short(id), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_read_short");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) fs_read_short() {
	_WRAP_OPCODE(fs_read_short2, 1, 1)
}

static void fs_read_int2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		opHandler.setReturn(FSread_int(id), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_read_int");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) fs_read_int() {
	_WRAP_OPCODE(fs_read_int2, 1, 1)
}

static void fs_read_float2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		opHandler.setReturn(FSread_int(id), DATATYPE_FLOAT);
	} else {
		OpcodeInvalidArgs("fs_read_float");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) fs_read_float() {
	_WRAP_OPCODE(fs_read_float2, 1, 1)
}

static void fs_delete2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		FSdelete(id);
	} else {
		OpcodeInvalidArgs("fs_delete");
	}
}

static void __declspec(naked) fs_delete() {
	_WRAP_OPCODE(fs_delete2, 1, 0)
}

static void fs_size2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		opHandler.setReturn(FSsize(id), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_size");
		opHandler.setReturn(0);
	}
}

static void __declspec(naked) fs_size() {
	_WRAP_OPCODE(fs_size2, 1, 1)
}

static void fs_pos2() {
	const ScriptValue &idArg = opHandler.arg(0);

	if (idArg.isInt()) {
		int id = idArg.asInt();
		opHandler.setReturn(FSpos(id), DATATYPE_INT);
	} else {
		OpcodeInvalidArgs("fs_pos");
		opHandler.setReturn(-1);
	}
}

static void __declspec(naked) fs_pos() {
	_WRAP_OPCODE(fs_pos2, 1, 1)
}

static void fs_seek2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &posArg = opHandler.arg(1);

	if (idArg.isInt() && posArg.isInt()) {
		int id = idArg.asInt(), pos = posArg.asInt();
		FSseek(id, pos);
	} else {
		OpcodeInvalidArgs("fs_seek");
	}
}

static void __declspec(naked) fs_seek() {
	_WRAP_OPCODE(fs_seek2, 2, 0)
}

static void fs_resize2() {
	const ScriptValue &idArg = opHandler.arg(0),
					  &sizeArg = opHandler.arg(1);

	if (idArg.isInt() && sizeArg.isInt()) {
		int id = idArg.asInt(), size = sizeArg.asInt();
		FSresize(id, size);
	} else {
		OpcodeInvalidArgs("fs_resize");
	}
}

static void __declspec(naked) fs_resize() {
	_WRAP_OPCODE(fs_resize2, 2, 0)
}
