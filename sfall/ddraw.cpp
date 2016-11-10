/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include "main.h"

typedef HRESULT (_stdcall *DDrawCreateProc)(void* a, void* b, void* c);

HRESULT _stdcall FakeDirectDrawCreate(void* a, void* b, void* c) {
	char path[MAX_PATH];
	GetSystemDirectoryA(path, MAX_PATH);
	strcat_s(path, "\\ddraw.dll");
	HMODULE ddraw = LoadLibraryA(path);
	if (!ddraw || ddraw == INVALID_HANDLE_VALUE) return -1;
	DDrawCreateProc proc = (DDrawCreateProc)GetProcAddress(ddraw, "DirectDrawCreate");
	if (!proc) return -1;
	return proc(a, b, c);
}
