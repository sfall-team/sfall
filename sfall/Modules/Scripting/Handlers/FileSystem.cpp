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

#include "..\..\..\FalloutEngine\Fallout2.h"
#include "..\..\ScriptExtender.h"
#include "..\..\FileSystem.h"
#include "..\OpcodeContext.h"

#include "FileSystem.h"

namespace sfall
{
namespace script
{

void op_fs_create(OpcodeContext& ctx) {
	ctx.setReturn(FScreate(ctx.arg(0).strValue(), ctx.arg(1).rawValue()));
}

void op_fs_copy(OpcodeContext& ctx) {
	ctx.setReturn(FScopy(ctx.arg(0).strValue(), ctx.arg(1).strValue()));
}

void op_fs_find(OpcodeContext& ctx) {
	ctx.setReturn(FSfind(ctx.arg(0).strValue()));
}

void op_fs_write_byte(OpcodeContext& ctx) {
	FSwrite_byte(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void op_fs_write_short(OpcodeContext& ctx) {
	FSwrite_short(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void op_fs_write_int(OpcodeContext& ctx) {
	FSwrite_int(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void op_fs_write_string(OpcodeContext& ctx) {
	FSwrite_string(ctx.arg(0).rawValue(), ctx.arg(1).strValue());
}

void op_fs_write_bstring(OpcodeContext& ctx) {
	FSwrite_bstring(ctx.arg(0).rawValue(), ctx.arg(1).strValue());
}

void op_fs_read_byte(OpcodeContext& ctx) {
	ctx.setReturn(FSread_byte(ctx.arg(0).rawValue()));
}

void op_fs_read_short(OpcodeContext& ctx) {
	ctx.setReturn(FSread_short(ctx.arg(0).rawValue()));
}

void op_fs_read_int(OpcodeContext& ctx) {
	ctx.setReturn(FSread_int(ctx.arg(0).rawValue()));
}

void op_fs_read_float(OpcodeContext& ctx) {
	ctx.setReturn(FSread_int(ctx.arg(0).rawValue()), DataType::FLOAT);
}

void op_fs_delete(OpcodeContext& ctx) {
	FSdelete(ctx.arg(0).rawValue());
}

void op_fs_size(OpcodeContext& ctx) {
	ctx.setReturn(FSsize(ctx.arg(0).rawValue()));
}

void op_fs_pos(OpcodeContext& ctx) {
	ctx.setReturn(FSpos(ctx.arg(0).rawValue()));
}

void op_fs_seek(OpcodeContext& ctx) {
	FSseek(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

void op_fs_resize(OpcodeContext& ctx) {
	FSresize(ctx.arg(0).rawValue(), ctx.arg(1).rawValue());
}

}
}
