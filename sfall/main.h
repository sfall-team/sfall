/*
 *    sfall
 *    Copyright (C) 2010  The sfall team
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

// global flag, indicating that debugging features of Sfall are enabled
#ifndef NO_SFALL_DEBUG
	extern bool isDebug;
#else
	#define isDebug false
#endif

// Macros for quick replacement of pushad/popad assembler opcodes
#define pushadc __asm push eax __asm push edx __asm push ecx
#define popadc __asm pop ecx __asm pop edx __asm pop eax

extern char ini[65];
extern char translationIni[65];
extern DWORD modifiedIni;

extern char dmModelName[65];
extern char dfModelName[65];

extern bool hrpIsEnabled;
extern bool hrpVersionValid;

DWORD HRPAddressOffset(DWORD offset);

void RemoveSavFiles();
void ClearSavPrototypes();

template<typename T> 
T SimplePatch(DWORD addr, const char* iniSection, const char* iniKey, T defaultValue, T minValue = 0, T maxValue = INT_MAX)
{
	return SimplePatch<T>(&addr, 1, iniSection, iniKey, defaultValue, minValue, maxValue);
}

template<typename T> 
T SimplePatch(DWORD *addrs, int numAddrs, const char* iniSection, const char* iniKey, T defaultValue, T minValue = 0, T maxValue = INT_MAX)
{
	T value;
	char msg[255];
	value = (T)GetPrivateProfileIntA(iniSection, iniKey, defaultValue, ini);
	if (value != defaultValue) {
		if (value < minValue) {
			value = minValue;
		} else if (value > maxValue) {
			value = maxValue;
		}
		_snprintf_s(msg, sizeof(msg), _TRUNCATE, "Applying patch: %s = %d.", iniKey, value);
		dlog((const char*)msg, DL_INIT);
		for (int i = 0; i < numAddrs; i++)
			SafeWrite<T>(addrs[i], (T)value);
		dlogr(" Done", DL_INIT);
	}
	return value;
}
