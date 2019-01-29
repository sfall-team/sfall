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

#include "Elevators.h"
#include "FalloutEngine.h"

static const int exitsPerElevator = 4;
static const int vanillaElevatorCount = 24;
static const int elevatorCount = 50;

static char elevFile[MAX_PATH] = ".\\";

struct sElevator {
	DWORD ID1;
	DWORD Elevation1;
	DWORD Tile1;
	DWORD ID2;
	DWORD Elevation2;
	DWORD Tile2;
	DWORD ID3;
	DWORD Elevation3;
	DWORD Tile3;
	DWORD ID4;
	DWORD Elevation4;
	DWORD Tile4;
};

struct sElevatorFrms {
	DWORD main;
	DWORD buttons;
};

static DWORD elevatorType[elevatorCount] = {0};
static sElevator elevators[elevatorCount] = {0};         // _retvals
static sElevatorFrms elevatorsFrms[elevatorCount] = {0}; // _intotal
static DWORD elevatorsBtnCount[elevatorCount] = {0};     // _btncount

static void __declspec(naked) GetMenuHook() {
	__asm {
		lea  edx, elevatorType;
		shl  eax, 2;
		mov  eax, [edx + eax];
		jmp  elevator_start_;
	}
}

static void __declspec(naked) CheckHotKeysHook() {
	__asm {
		cmp  eax, vanillaElevatorCount;
		jge  skip; // skip hotkeys data
		push ebx;
		lea  ebx, elevatorType;
		shl  eax, 2;
		mov  eax, [ebx + eax];
		call Check4Keys_;
		pop  ebx;
		retn;
skip:
		xor  eax, eax;
		retn;
	}
}

/*
static void __declspec(naked) UnknownHook2() {
	__asm {
		lea  edx, elevatorType;
		shl  eax, 2;
		mov  eax, [edx + eax];
		jmp  elevator_end_;
	}
}
*/

static void __declspec(naked) GetNumButtonsHook1() {
	__asm {
		lea  esi, elevatorType;
		mov  eax, [esi + edi * 4];
		mov  eax, [elevatorsBtnCount + eax * 4];
		retn;
	}
}

static void __declspec(naked) GetNumButtonsHook2() {
	__asm {
		lea  edx, elevatorType;
		mov  eax, [edx + edi * 4];
		mov  eax, [elevatorsBtnCount + eax * 4];
		retn;
	}
}

static void __declspec(naked) GetNumButtonsHook3() {
	__asm {
		lea  eax, elevatorType;
		mov  eax, [eax + edi * 4];
		mov  eax, [elevatorsBtnCount + eax * 4];
		retn;
	}
}

void ResetElevators() {
	//memset(&elevators[vanillaElevatorCount], 0, sizeof(sElevator) * (elevatorCount - vanillaElevatorCount));
	//memset(&elevatorsFrms[vanillaElevatorCount], 0, sizeof(sElevatorFrms) * (elevatorCount - vanillaElevatorCount));
	//for (int i = vanillaElevatorCount; i < elevatorCount; i++) elevatorType[i] = 0;
}

static void LoadElevators() {
	//ResetElevators();

	memcpy(elevators, (void*)_retvals, sizeof(sElevator) * vanillaElevatorCount);
	memcpy(elevatorsFrms, (void*)_intotal, sizeof(sElevatorFrms) * vanillaElevatorCount);
	memcpy(elevatorsBtnCount, (void*)_btncnt, sizeof(DWORD) * vanillaElevatorCount);

	for (int i = 0; i < vanillaElevatorCount; i++) elevatorType[i] = i;

	char section[4];
	if (elevFile) {
		for (int i = 0; i < elevatorCount; i++) {
			_itoa_s(i, section, 10);
			int type = GetPrivateProfileIntA(section, "Image", elevatorType[i], elevFile);
			elevatorType[i] = min(type, elevatorCount - 1);
			if (i >= vanillaElevatorCount) {
				int cBtn = GetPrivateProfileIntA(section, "ButtonCount", 2, elevFile);
				if (cBtn > exitsPerElevator) cBtn = exitsPerElevator;
				elevatorsBtnCount[i] = max(2, cBtn);
			}
			elevatorsFrms[i].main = GetPrivateProfileIntA(section, "MainFrm", elevatorsFrms[i].main, elevFile);
			elevatorsFrms[i].buttons = GetPrivateProfileIntA(section, "ButtonsFrm", elevatorsFrms[i].buttons, elevFile);
			elevatorType[i] = GetPrivateProfileIntA(section, "Image", elevatorType[i], elevFile);
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

void ElevatorsInit(char* file) {
	strcat_s(elevFile, file);

	HookCall(0x43EF83, GetMenuHook);
	HookCall(0x43F141, CheckHotKeysHook);
	//HookCall(0x43F2D2, UnknownHook2); // unused

	SafeWrite8(0x43EF76, (BYTE)elevatorCount);
	SafeWrite32(0x43EFA4, (DWORD)elevators);
	SafeWrite32(0x43EFB9, (DWORD)elevators);
	SafeWrite32(0x43F2FC, (DWORD)elevators);
	SafeWrite32(0x43EFEA, (DWORD)&elevators[0].Tile1);
	SafeWrite32(0x43F315, (DWORD)&elevators[0].Tile1);
	SafeWrite32(0x43F309, (DWORD)&elevators[0].Elevation1);

	SafeWrite32(0x43F438, (DWORD)&elevatorsFrms[0].main);
	SafeWrite32(0x43F475, (DWORD)&elevatorsFrms[0].buttons);

	// _btncnt
	SafeWrite32(0x43F65E, (DWORD)elevatorsBtnCount);
	SafeWrite32(0x43F6BB, (DWORD)elevatorsBtnCount);
	MakeCall(0x43F05D, GetNumButtonsHook1, 2);
	MakeCall(0x43F184, GetNumButtonsHook2, 2);
	MakeCall(0x43F1E4, GetNumButtonsHook3, 2);

	LoadElevators();
}
