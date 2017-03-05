/*
 *    sfall
 *    Copyright (C) 2008, 2009  The sfall team
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

#include "Module.h"

namespace sfall
{

class FileSystem : public Module {
public:
	const char* name() { return "FileSystem"; }
	void init();

	// save FileSystem data to a save game file
	static void save(HANDLE h);
};

DWORD _stdcall FScreate(const char* path, int size);
DWORD _stdcall FScreateFromData(const char* path, void* data, int size);
DWORD _stdcall FScopy(const char* path, const char* source);
DWORD _stdcall FSfind(const char* path);
void _stdcall FSwrite_byte(DWORD id, int data);
void _stdcall FSwrite_short(DWORD id, int data);
void _stdcall FSwrite_int(DWORD id, int data);
//void _stdcall fs_write_float(DWORD id, int data);
void _stdcall FSwrite_string(DWORD id, const char* data);
void _stdcall FSwrite_bstring(DWORD id, const char* data);
int _stdcall FSread_byte(DWORD id);
int _stdcall FSread_short(DWORD id);
int _stdcall FSread_int(DWORD id);
 void _stdcall FSdelete(DWORD id);
DWORD _stdcall FSsize(DWORD id);
DWORD _stdcall FSpos(DWORD id);
void _stdcall FSseek(DWORD id, DWORD pos);
void _stdcall FSresize(DWORD id, DWORD size);

}
