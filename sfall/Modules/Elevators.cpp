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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Elevators.h"

namespace sfall
{

static const int elevatorCount = 50;
static char elevFile[MAX_PATH];

static fo::sElevator elevators[elevatorCount];
static DWORD menus[elevatorCount];

void SetElevator(DWORD id, DWORD index, DWORD value) {
	if (id >= elevatorCount || index >= 12) return;
	*(DWORD*)(((DWORD)&elevators[id]) + index * 4) = value;
}

static void __declspec(naked) GetMenuHook() {
	__asm {
		push ebx;
		lea ebx, menus;
		shl eax, 2;
		mov eax, [ebx+eax];
		call fo::funcoffs::elevator_start_;
		pop ebx;
		ret;
	}
}

static void __declspec(naked) UnknownHook() {
	__asm {
		push ebx;
		lea ebx, menus;
		shl eax, 2;
		mov eax, [ebx+eax];
		call fo::funcoffs::Check4Keys_;
		pop ebx;
		ret;
	}
}

static void __declspec(naked) UnknownHook2() {
	__asm {
		push ebx;
		lea ebx, menus;
		shl eax, 2;
		mov eax, [ebx+eax];
		call fo::funcoffs::elevator_end_;
		pop ebx;
		ret;
	}
}

static void __declspec(naked) GetNumButtonsHook1() {
	__asm {
		lea  esi, menus;
		mov  eax, [esi+edi*4];
		mov  eax, [FO_VAR_btncnt + eax*4];
		push 0x43F064;
		retn;
	}
}

static void __declspec(naked) GetNumButtonsHook2() {
	__asm {
		lea  edx, menus;
		mov  eax, [edx+edi*4];
		mov  eax, [FO_VAR_btncnt + eax*4];
		push 0x43F18B;
		retn;
	}
}

static void __declspec(naked) GetNumButtonsHook3() {
	__asm {
		lea  eax, menus;
		mov  eax, [eax+edi*4];
		mov  eax, [FO_VAR_btncnt+eax*4];
		push 0x43F1EB;
		retn;
	}
}

void ResetElevators() {
	memcpy(elevators, fo::var::retvals, sizeof(fo::sElevator) * 24);
	memset(&elevators[24], 0, sizeof(fo::sElevator)*(elevatorCount - 24));
	for (int i = 0; i < 24; i++) menus[i] = i;
	for (int i = 24; i < elevatorCount; i++) menus[i] = 0;
	char section[4];
	if (elevFile) {
		for (int i = 0; i < elevatorCount; i++) {
			_itoa_s(i, section, 10);
			menus[i] = GetPrivateProfileIntA(section, "Image", menus[i], elevFile);
			elevators[i].ID1 = GetPrivateProfileIntA(section, "ID1", elevators[i].ID1, elevFile);
			elevators[i].ID2 = GetPrivateProfileIntA(section, "ID2", elevators[i].ID2, elevFile);
			elevators[i].ID3 = GetPrivateProfileIntA(section, "ID3", elevators[i].ID3, elevFile);
			elevators[i].ID4 = GetPrivateProfileIntA(section, "ID4", elevators[i].ID4, elevFile);
			elevators[i].Elevation1 = GetPrivateProfileIntA(section, "Elevation1", elevators[i].Elevation1, elevFile);
			elevators[i].Elevation2 = GetPrivateProfileIntA(section, "Elevation2", elevators[i].Elevation2, elevFile);
			elevators[i].Elevation3 = GetPrivateProfileIntA(section, "Elevation3", elevators[i].Elevation3, elevFile);
			elevators[i].Elevation4 = GetPrivateProfileIntA(section, "Elevation4", elevators[i].Elevation4, elevFile);
			elevators[i].Tile1 = GetPrivateProfileIntA(section, "Tile1", elevators[i].Tile1, elevFile);
			elevators[i].Tile2 = GetPrivateProfileIntA(section, "Tile2", elevators[i].Tile2, elevFile);
			elevators[i].Tile3 = GetPrivateProfileIntA(section, "Tile3", elevators[i].Tile3, elevFile);
			elevators[i].Tile4 = GetPrivateProfileIntA(section, "Tile4", elevators[i].Tile4, elevFile);
		}
	}
}

void ElevatorsInit(const char* file) {
	strcpy_s(elevFile, ".\\");
	strcat_s(elevFile, file);
	HookCall(0x43EF83, GetMenuHook);
	HookCall(0x43F141, UnknownHook);
	HookCall(0x43F2D2, UnknownHook2);
	SafeWrite8(0x43EF76, (BYTE)elevatorCount);
	SafeWrite32(0x43EFA4, (DWORD)elevators);
	SafeWrite32(0x43EFB9, (DWORD)elevators);
	SafeWrite32(0x43EFEA, (DWORD)&elevators[0].Tile1);
	SafeWrite32(0x43F2FC, (DWORD)elevators);
	SafeWrite32(0x43F309, (DWORD)&elevators[0].Elevation1);
	SafeWrite32(0x43F315, (DWORD)&elevators[0].Tile1);

	SafeWrite8(0x43F05D, 0xe9);
	HookCall(0x43F05D, GetNumButtonsHook1);
	SafeWrite8(0x43F184, 0xe9);
	HookCall(0x43F184, GetNumButtonsHook2);
	SafeWrite8(0x43F1E4, 0xe9);
	HookCall(0x43F1E4, GetNumButtonsHook3);
	ResetElevators();
}

void Elevators::init() {
	auto elevPath = GetConfigString("Misc", "ElevatorsFile", "", MAX_PATH);
	if (elevPath.size() > 0) {
		dlogr("Applying elevator patch.", DL_INIT);
		ElevatorsInit(elevPath.c_str());
	}
}

}
