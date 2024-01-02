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

#include "main.h"

extern ddrawDll ddraw;

//typedef HRESULT (__stdcall *DDrawCreateProc)(DWORD a, IDirectDraw* b, DWORD c);

//HRESULT __stdcall FakeDirectDrawCreate(DWORD a, IDirectDraw* b, DWORD c) {
//	DDrawCreateProc proc = (DDrawCreateProc)ddraw.DirectDrawCreate;
//	if (!proc) return -1;
//	return proc(a, b, c);
//}

__declspec(naked) void FakeDirectDrawCreate() {
	__asm jmp [ddraw.DirectDrawCreate];
}

__declspec(naked) void FakeAcquireDDThreadLock() {
	__asm jmp [ddraw.AcquireDDThreadLock];
}

__declspec(naked) void FakeCheckFullscreen() {
	__asm jmp [ddraw.CheckFullscreen];
}

__declspec(naked) void FakeCompleteCreateSysmemSurface() {
	__asm jmp [ddraw.CompleteCreateSysmemSurface];
}

__declspec(naked) void FakeD3DParseUnknownCommand() {
	__asm jmp [ddraw.D3DParseUnknownCommand];
}

__declspec(naked) void FakeDDGetAttachedSurfaceLcl() {
	__asm jmp [ddraw.DDGetAttachedSurfaceLcl];
}

__declspec(naked) void FakeDDInternalLock() {
	__asm jmp [ddraw.DDInternalLock];
}

__declspec(naked) void FakeDDInternalUnlock() {
	__asm jmp [ddraw.DDInternalUnlock];
}

__declspec(naked) void FakeDSoundHelp() {
	__asm jmp [ddraw.DSoundHelp];
}

__declspec(naked) void FakeDirectDrawCreateClipper() {
	__asm jmp [ddraw.DirectDrawCreateClipper];
}

__declspec(naked) void FakeDirectDrawCreateEx() {
	__asm jmp [ddraw.DirectDrawCreateEx];
}

__declspec(naked) void FakeDirectDrawEnumerateA() {
	__asm jmp [ddraw.DirectDrawEnumerateA];
}

__declspec(naked) void FakeDirectDrawEnumerateExA() {
	__asm jmp [ddraw.DirectDrawEnumerateExA];
}

__declspec(naked) void FakeDirectDrawEnumerateExW() {
	__asm jmp [ddraw.DirectDrawEnumerateExW];
}

__declspec(naked) void FakeDirectDrawEnumerateW() {
	__asm jmp [ddraw.DirectDrawEnumerateW];
}

__declspec(naked) void FakeDllCanUnloadNow() {
	__asm jmp [ddraw.DllCanUnloadNow];
}

__declspec(naked) void FakeDllGetClassObject() {
	__asm jmp [ddraw.DllGetClassObject];
}

__declspec(naked) void FakeGetDDSurfaceLocal() {
	__asm jmp [ddraw.GetDDSurfaceLocal];
}

__declspec(naked) void FakeGetOLEThunkData() {
	__asm jmp [ddraw.GetOLEThunkData];
}

__declspec(naked) void FakeGetSurfaceFromDC() {
	__asm jmp [ddraw.GetSurfaceFromDC];
}

__declspec(naked) void FakeRegisterSpecialCase() {
	__asm jmp [ddraw.RegisterSpecialCase];
}

__declspec(naked) void FakeReleaseDDThreadLock() {
	__asm jmp [ddraw.ReleaseDDThreadLock];
}

__declspec(naked) void FakeSetAppCompatData() {
	__asm jmp [ddraw.SetAppCompatData];
}
