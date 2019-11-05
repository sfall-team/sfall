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

static DWORD elevatorType[elevatorCount] = {0};
static fo::ElevatorExit elevatorExits[elevatorCount][exitsPerElevator] = {0}; // _retvals
static fo::ElevatorFrms elevatorsFrms[elevatorCount] = {0};                   // _intotal
static DWORD elevatorsBtnCount[elevatorCount] = {0};                          // _btncount

static void __declspec(naked) GetMenuHook() {
	__asm {
		lea  edx, elevatorType;
		shl  eax, 2;
		mov  eax, [edx + eax];
		jmp  fo::funcoffs::elevator_start_;
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
		call fo::funcoffs::Check4Keys_;
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
		jmp  fo::funcoffs::elevator_end_;
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
	//memset(&elevatorExits[vanillaElevatorCount], 0, sizeof(fo::ElevatorExit) * (elevatorCount - vanillaElevatorCount) * exitsPerElevator);
	//memset(&elevatorsFrms[vanillaElevatorCount], 0, sizeof(fo::ElevatorFrms) * (elevatorCount - vanillaElevatorCount));
	//for (int i = vanillaElevatorCount; i < elevatorCount; i++) elevatorType[i] = 0;
}

static void LoadElevators(const char* elevFile) {
	//ResetElevators();

	memcpy(elevatorExits, fo::var::retvals, sizeof(fo::ElevatorExit) * vanillaElevatorCount * exitsPerElevator);
	memcpy(elevatorsFrms, (void*)FO_VAR_intotal, sizeof(fo::ElevatorFrms) * vanillaElevatorCount);
	memcpy(elevatorsBtnCount, (void*)FO_VAR_btncnt, sizeof(DWORD) * vanillaElevatorCount);

	for (int i = 0; i < vanillaElevatorCount; i++) elevatorType[i] = i;

	char section[4];
	if (elevFile && GetFileAttributes(elevFile) != INVALID_FILE_ATTRIBUTES) {
		for (int i = 0; i < elevatorCount; i++) {
			_itoa_s(i, section, 10);
			int type = iniGetInt(section, "Image", elevatorType[i], elevFile);
			elevatorType[i] = min(type, elevatorCount - 1);
			if (i >= vanillaElevatorCount) {
				int cBtn = iniGetInt(section, "ButtonCount", 2, elevFile);
				if (cBtn > exitsPerElevator) cBtn = exitsPerElevator;
				elevatorsBtnCount[i] = max(2, cBtn);
			}
			elevatorsFrms[i].main = iniGetInt(section, "MainFrm", elevatorsFrms[i].main, elevFile);
			elevatorsFrms[i].buttons = iniGetInt(section, "ButtonsFrm", elevatorsFrms[i].buttons, elevFile);
			char setting[32];
			for (int j = 0; j < exitsPerElevator; j++) {
				sprintf_s(setting, "ID%d", j + 1);
				elevatorExits[i][j].id = iniGetInt(section, setting, elevatorExits[i][j].id, elevFile);
				sprintf_s(setting, "Elevation%d", j + 1);
				elevatorExits[i][j].elevation = iniGetInt(section, setting, elevatorExits[i][j].elevation, elevFile);
				sprintf_s(setting, "Tile%d", j + 1);
				elevatorExits[i][j].tile = iniGetInt(section, setting, elevatorExits[i][j].tile, elevFile);
			}
		}
	}
}

void ElevatorsInit() {
	HookCall(0x43EF83, GetMenuHook);
	HookCall(0x43F141, CheckHotKeysHook);
	//HookCall(0x43F2D2, UnknownHook2); // unused

	SafeWrite8(0x43EF76, (BYTE)elevatorCount);
	SafeWrite32(0x43EFA4, (DWORD)elevatorExits);
	SafeWrite32(0x43EFB9, (DWORD)elevatorExits);
	SafeWrite32(0x43F2FC, (DWORD)elevatorExits);
	SafeWrite32(0x43EFEA, (DWORD)&elevatorExits[0][0].tile);
	SafeWrite32(0x43F315, (DWORD)&elevatorExits[0][0].tile);
	SafeWrite32(0x43F309, (DWORD)&elevatorExits[0][0].elevation);

	SafeWrite32(0x43F438, (DWORD)&elevatorsFrms[0].main);
	SafeWrite32(0x43F475, (DWORD)&elevatorsFrms[0].buttons);

	// _btncnt
	SafeWrite32(0x43F65E, (DWORD)elevatorsBtnCount);
	SafeWrite32(0x43F6BB, (DWORD)elevatorsBtnCount);
	MakeCall(0x43F05D, GetNumButtonsHook1, 2);
	MakeCall(0x43F184, GetNumButtonsHook2, 2);
	MakeCall(0x43F1E4, GetNumButtonsHook3, 2);
}

void Elevators::init() {
	auto elevPath = GetConfigString("Misc", "ElevatorsFile", "", MAX_PATH);
	if (!elevPath.empty()) {
		dlog("Applying elevator patch.", DL_INIT);
		ElevatorsInit();
		LoadElevators(elevPath.insert(0, ".\\").c_str());
		dlogr(" Done", DL_INIT);
	}
}

}
