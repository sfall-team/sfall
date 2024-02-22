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
#pragma warning(disable: 4996) // function or variable may be unsafe
#pragma warning(disable: 4995) // 'function': name was marked as #pragma deprecated
#ifdef NDEBUG
#pragma warning(disable: 4414) // 'function': short jump to function converted to near
#endif

#pragma intrinsic(memcpy, memset)

#include <cassert>
#include <string>
#include <initializer_list>
#include <list>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <Windows.h>

#include "SafeWrite.h"
#include "Logging.h"
#include "IniReader.h"

#include "HRP\Init.h"

struct ddrawDll {
	HMODULE sfall;
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
	FARPROC DllCanUnloadNow;
	FARPROC DllGetClassObject;
	FARPROC GetDDSurfaceLocal;
	FARPROC GetOLEThunkData;
	FARPROC GetSurfaceFromDC;
	FARPROC RegisterSpecialCase;
	FARPROC SetAppCompatData;
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

extern bool versionCHI;

extern bool extWrapper;

__inline long GetIntHRPValue(DWORD addr) {
	return *reinterpret_cast<DWORD*>(HRP::Setting::GetAddress(addr));
}

__inline BYTE GetByteHRPValue(DWORD addr) {
	return *reinterpret_cast<BYTE*>(HRP::Setting::GetAddress(addr));
}

}
