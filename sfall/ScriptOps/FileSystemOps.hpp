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
	opHandler.setReturn(FScreate(opHandler.arg(0).asString(), opHandler.arg(1).asInt()), DATATYPE_INT);
}

static void __declspec(naked) fs_create() {
	_WRAP_OPCODE(fs_create2, 2, 1)
}

static void fs_copy2() {
	opHandler.setReturn(FScopy(opHandler.arg(0).asString(), opHandler.arg(1).asString()), DATATYPE_INT);
}

static void __declspec(naked) fs_copy() {
	_WRAP_OPCODE(fs_copy2, 2, 1)
}

static void fs_find2() {
	opHandler.setReturn(FSfind(opHandler.arg(0).asString()), DATATYPE_INT);
}

static void __declspec(naked) fs_find() {
	_WRAP_OPCODE(fs_find2, 1, 1)
}

static void fs_write_byte2() {
	FSwrite_byte(opHandler.arg(0).asInt(), opHandler.arg(1).asInt());
}

static void __declspec(naked) fs_write_byte() {
	_WRAP_OPCODE(fs_write_byte2, 2, 0)
}

static void fs_write_short2() {
	FSwrite_short(opHandler.arg(0).asInt(), opHandler.arg(1).asInt());
}

static void __declspec(naked) fs_write_short() {
	_WRAP_OPCODE(fs_write_short2, 2, 0)
}

static void fs_write_int2() {
	FSwrite_int(opHandler.arg(0).asInt(), opHandler.arg(1).asInt());
}

static void __declspec(naked) fs_write_int() {
	_WRAP_OPCODE(fs_write_int2, 2, 0)
}

static void fs_write_string2() {
	FSwrite_string(opHandler.arg(0).asInt(), opHandler.arg(1).asString());
}

static void __declspec(naked) fs_write_string() {
	_WRAP_OPCODE(fs_write_string2, 2, 0)
}

static void fs_write_bstring2() {
	FSwrite_bstring(opHandler.arg(0).asInt(), opHandler.arg(1).asString());
}

static void __declspec(naked) fs_write_bstring() {
	_WRAP_OPCODE(fs_write_bstring2, 2, 0)
}

static void fs_read_byte2() {
	opHandler.setReturn(FSread_byte(opHandler.arg(0).asInt()), DATATYPE_INT);
}

static void __declspec(naked) fs_read_byte() {
	_WRAP_OPCODE(fs_read_byte2, 1, 1)
}

static void fs_read_short2() {
	opHandler.setReturn(FSread_short(opHandler.arg(0).asInt()), DATATYPE_INT);
}

static void __declspec(naked) fs_read_short() {
	_WRAP_OPCODE(fs_read_short2, 1, 1)
}

static void fs_read_int2() {
	opHandler.setReturn(FSread_int(opHandler.arg(0).asInt()), DATATYPE_INT);
}

static void __declspec(naked) fs_read_int() {
	_WRAP_OPCODE(fs_read_int2, 1, 1)
}

static void fs_read_float2() {
	opHandler.setReturn(FSread_int(opHandler.arg(0).asInt()), DATATYPE_FLOAT);
}

static void __declspec(naked) fs_read_float() {
	_WRAP_OPCODE(fs_read_float2, 1, 1)
}

static void fs_delete2() {
	FSdelete(opHandler.arg(0).asInt());
}

static void __declspec(naked) fs_delete() {
	_WRAP_OPCODE(fs_delete2, 1, 0)
}

static void fs_size2() {
	opHandler.setReturn(FSsize(opHandler.arg(0).asInt()), DATATYPE_INT);
}

static void __declspec(naked) fs_size() {
	_WRAP_OPCODE(fs_size2, 1, 1)
}

static void fs_pos2() {
	opHandler.setReturn(FSpos(opHandler.arg(0).asInt()), DATATYPE_INT);
}

static void __declspec(naked) fs_pos() {
	_WRAP_OPCODE(fs_pos2, 1, 1)
}

static void fs_seek2() {
	FSseek(opHandler.arg(0).asInt(), opHandler.arg(1).asInt());
}

static void __declspec(naked) fs_seek() {
	_WRAP_OPCODE(fs_seek2, 2, 0)
}

static void fs_resize2() {
	FSresize(opHandler.arg(0).asInt(), opHandler.arg(1).asInt());
}

static void __declspec(naked) fs_resize() {
	_WRAP_OPCODE(fs_resize2, 2, 0)
}
