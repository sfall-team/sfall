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

static const int exitsPerElevator = 4;
static const int vanillaElevatorCount = 24;
static const int elevatorCount = 50;
static char elevFile[MAX_PATH];

static fo::ElevatorExit elevatorExits[elevatorCount][exitsPerElevator];
static DWORD menus[elevatorCount];
static fo::ElevatorFrms elevatorsFrms[elevatorCount];

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
		retn;
	}
}

static void __declspec(naked) GetNumButtonsHook2() {
	__asm {
		lea  edx, menus;
		mov  eax, [edx+edi*4];
		mov  eax, [FO_VAR_btncnt + eax*4];
		retn;
	}
}

static void __declspec(naked) GetNumButtonsHook3() {
	__asm {
		lea  eax, menus;
		mov  eax, [eax+edi*4];
		mov  eax, [FO_VAR_btncnt+eax*4];
		retn;
	}
}

void ResetElevators() {
	memcpy(elevatorExits, fo::var::retvals, sizeof(fo::ElevatorExit) * vanillaElevatorCount * exitsPerElevator);
	memset(&elevatorExits[vanillaElevatorCount], 0, sizeof(fo::ElevatorExit) * (elevatorCount - vanillaElevatorCount) * exitsPerElevator);

	memcpy(elevatorsFrms, (void*)FO_VAR_intotal, sizeof(fo::ElevatorFrms) * vanillaElevatorCount);
	memset(&elevatorsFrms[vanillaElevatorCount], 0, sizeof(fo::ElevatorFrms) * (elevatorCount - vanillaElevatorCount));

	for (int i = 0; i < vanillaElevatorCount; i++) menus[i] = i;
	for (int i = vanillaElevatorCount; i < elevatorCount; i++) menus[i] = 0;

	char section[4];
	if (elevFile) {
		for (int i = 0; i < elevatorCount; i++) {
			_itoa_s(i, section, 10);
			menus[i] = GetPrivateProfileIntA(section, "Image", menus[i], elevFile);
			elevatorsFrms[i].main = GetPrivateProfileIntA(section, "MainFrm", elevatorsFrms[i].main, elevFile);
			elevatorsFrms[i].buttons = GetPrivateProfileIntA(section, "ButtonsFrm", elevatorsFrms[i].buttons, elevFile);
			char setting[32];
			for (int j = 0; j < exitsPerElevator; j++) {
				sprintf_s(setting, "ID%d", j + 1);
				elevatorExits[i][j].id = GetPrivateProfileIntA(section, setting, elevatorExits[i][j].id, elevFile);
				sprintf_s(setting, "Elevation%d", j + 1);
				elevatorExits[i][j].elevation = GetPrivateProfileIntA(section, setting, elevatorExits[i][j].elevation, elevFile);
				sprintf_s(setting, "Tile%d", j + 1);
				elevatorExits[i][j].tile = GetPrivateProfileIntA(section, setting, elevatorExits[i][j].tile, elevFile);
			}
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
	SafeWrite32(0x43EFA4, (DWORD)elevatorExits);
	SafeWrite32(0x43EFB9, (DWORD)elevatorExits);
	SafeWrite32(0x43EFEA, (DWORD)&elevatorExits[0][0].tile);
	SafeWrite32(0x43F2FC, (DWORD)elevatorExits);
	SafeWrite32(0x43F309, (DWORD)&elevatorExits[0][0].elevation);
	SafeWrite32(0x43F315, (DWORD)&elevatorExits[0][0].tile);

	SafeWrite32(0x43F438, (DWORD)&elevatorsFrms[0].main);
	SafeWrite32(0x43F475, (DWORD)&elevatorsFrms[0].buttons);

	MakeCall(0x43F05D, GetNumButtonsHook1, 2);
	MakeCall(0x43F184, GetNumButtonsHook2, 2);
	MakeCall(0x43F1E4, GetNumButtonsHook3, 2);

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
