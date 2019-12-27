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

#pragma warning(disable:4996)

#ifdef NDEBUG
#pragma warning(disable:4414)
#endif

#include <assert.h>
//#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

#include "SafeWrite.h"
#include "Logging.h"

struct ddrawDll {
	HMODULE dll;
	FARPROC DirectDrawEnumerateExA;
	FARPROC DirectDrawCreate;
	FARPROC DirectDrawCreateEx;
	FARPROC AcquireDDThreadLock;
	FARPROC ReleaseDDThreadLock;
	FARPROC CheckFullscreen;
	FARPROC CompleteCreateSysmemSurface;
	FARPROC D3DParseUnknownCommand;
	FARPROC DDGetAttachedSurfaceLcl;
	FARPROC DDInternalLock;
	FARPROC DDInternalUnlock;
	FARPROC DSoundHelp;
	FARPROC DirectDrawCreateClipper;
	FARPROC DirectDrawEnumerateA;
	FARPROC DirectDrawEnumerateExW;
	FARPROC DirectDrawEnumerateW;
	//FARPROC DllCanUnloadNow;
	//FARPROC DllGetClassObject;
	FARPROC GetDDSurfaceLocal;
	FARPROC GetOLEThunkData;
	FARPROC GetSurfaceFromDC;
	FARPROC RegisterSpecialCase;
};

namespace sfall
{

// global flag, indicating that debugging features of Sfall are enabled
#ifndef NO_SFALL_DEBUG
	extern bool isDebug;
#else
	#define isDebug false
#endif

// Trap for Debugger
#define BREAKPOINT __asm int 3

// Macros for quick replacement of assembler opcodes pushad/popad
#define pushadc __asm push eax __asm push edx __asm push ecx
#define popadc __asm pop ecx __asm pop edx __asm pop eax

// Gets the integer value from given INI file.
int iniGetInt(const char* section, const char* setting, int defaultValue, const char* iniFile);

// Gets the string value from given INI file.
size_t iniGetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile);

// Gets the string value from given INI file.
std::string GetIniString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile);

// Parses the comma-separated list setting from given INI file.
std::vector<std::string> GetIniList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile);

// Gets the integer value from Sfall configuration INI file.
unsigned int GetConfigInt(const char* section, const char* setting, int defaultValue);

// Gets the string value from Sfall configuration INI file with trim function.
std::string GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

// Loads the string value from Sfall configuration INI file into the provided buffer.
size_t GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize = 128);

// Parses the comma-separated list from the settings from Sfall configuration INI file.
std::vector<std::string> GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

// Translates given string using Sfall translation INI file.
std::string Translate(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

// Translates given string using Sfall translation INI file and puts the result into given buffer.
size_t Translate(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize = 128);

DWORD HRPAddress(DWORD addr);

extern const char ddrawIni[];
extern DWORD modifiedIni;

extern bool hrpIsEnabled;
extern bool hrpVersionValid;

}
