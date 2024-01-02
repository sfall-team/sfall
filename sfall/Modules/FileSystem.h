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

#include "Module.h"

namespace sfall
{

class FileSystem : public Module {
public:
	const char* name() { return "FileSystem"; }
	void init();

	static bool UsingFileSystem;
	// save FileSystem data to a save game file
	static void Save(HANDLE h);
	static bool IsEmpty();
};

DWORD __stdcall FScreate(const char* path, int size);
DWORD __stdcall FScreateFromData(const char* path, void* data, int size);
DWORD __stdcall FScopy(const char* path, const char* source);
DWORD __stdcall FSfind(const char* path);
void __stdcall FSwrite_byte(DWORD id, int data);
void __stdcall FSwrite_short(DWORD id, int data);
void __stdcall FSwrite_int(DWORD id, int data);
void __stdcall FSwrite_string(DWORD id, const char* data);
void __stdcall FSwrite_bstring(DWORD id, const char* data);
int __stdcall FSread_byte(DWORD id);
int __stdcall FSread_short(DWORD id);
int __stdcall FSread_int(DWORD id);
void __stdcall FSdelete(DWORD id);
DWORD __stdcall FSsize(DWORD id);
DWORD __stdcall FSpos(DWORD id);
void __stdcall FSseek(DWORD id, DWORD pos);
void __stdcall FSresize(DWORD id, DWORD size);

}
