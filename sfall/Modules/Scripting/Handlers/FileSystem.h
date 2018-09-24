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

#pragma once

//file system functions

namespace sfall
{
namespace script
{

class OpcodeContext;

void sf_fs_create(OpcodeContext&);

void sf_fs_copy(OpcodeContext&);

void sf_fs_find(OpcodeContext&);

void sf_fs_write_byte(OpcodeContext&);

void sf_fs_write_short(OpcodeContext&);

void sf_fs_write_int(OpcodeContext&);

void sf_fs_write_string(OpcodeContext&);

void sf_fs_write_bstring(OpcodeContext&);

void sf_fs_read_byte(OpcodeContext&);

void sf_fs_read_short(OpcodeContext&);

void sf_fs_read_int(OpcodeContext&);

void sf_fs_read_float(OpcodeContext&);

void sf_fs_delete(OpcodeContext&);

void sf_fs_size(OpcodeContext&);

void sf_fs_pos(OpcodeContext&);

void sf_fs_seek(OpcodeContext&);

void sf_fs_resize(OpcodeContext&);

}
}
