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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Elevators.h"

namespace sfall
{

static const int exitsPerElevator = 4;
static const int vanillaElevatorCount = 24;
static const int elevatorCount = 50; // The maximum allowed for Elevator stub in the BIS mapper

static DWORD elevatorType[elevatorCount] = {0};
static fo::ElevatorExit elevatorExits[elevatorCount][exitsPerElevator] = {0}; // _retvals
static fo::ElevatorFrms elevatorsFrms[elevatorCount] = {0};                   // _intotal
static DWORD elevatorsBtnCount[elevatorCount] = {0};                          // _btncount

static __declspec(naked) void GetMenuHook() {
	__asm {
		lea  edx, elevatorType;
		shl  eax, 2;
		mov  eax, [edx + eax];
		jmp  fo::funcoffs::elevator_start_;
	}
}

static __declspec(naked) void CheckHotKeysHook() {
	__asm {
		lea  ebx, elevatorType;
		shl  eax, 2;
		mov  eax, [ebx + eax];
		cmp  eax, vanillaElevatorCount;
		jge  skip; // skip hotkeys data (prevent array out of bounds error)
		call fo::funcoffs::Check4Keys_;
		retn;
skip:
		xor  eax, eax;
		retn;
	}
}

/*
static __declspec(naked) void UnknownHook2() {
	__asm {
		lea  edx, elevatorType;
		shl  eax, 2;
		mov  eax, [edx + eax];
		jmp  fo::funcoffs::elevator_end_;
	}
}
*/

static __declspec(naked) void GetNumButtonsHook1() {
	__asm {
		lea  esi, elevatorType;
		mov  eax, [esi + edi * 4];
		mov  eax, [elevatorsBtnCount + eax * 4];
		retn;
	}
}

static __declspec(naked) void GetNumButtonsHook2() {
	__asm {
		lea  edx, elevatorType;
		mov  eax, [edx + edi * 4];
		mov  eax, [elevatorsBtnCount + eax * 4];
		retn;
	}
}

static __declspec(naked) void GetNumButtonsHook3() {
	__asm {
		lea  eax, elevatorType;
		mov  eax, [eax + edi * 4];
		mov  eax, [elevatorsBtnCount + eax * 4];
		retn;
	}
}

static void ResetElevators() {
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
	for (int i = 0; i < elevatorCount; i++) {
		_itoa_s(i, section, 10);
		int type = IniReader::GetInt(section, "Image", elevatorType[i], elevFile);
		elevatorType[i] = min(type, elevatorCount - 1);
		if (i >= vanillaElevatorCount) {
			int cBtn = IniReader::GetInt(section, "ButtonCount", 2, elevFile);
			if (cBtn < 2) cBtn = 2;
			elevatorsBtnCount[i] = min(cBtn, exitsPerElevator);
		}
		elevatorsFrms[i].main = IniReader::GetInt(section, "MainFrm", elevatorsFrms[i].main, elevFile);
		elevatorsFrms[i].buttons = IniReader::GetInt(section, "ButtonsFrm", elevatorsFrms[i].buttons, elevFile);
		char setting[32];
		for (int j = 0; j < exitsPerElevator; j++) {
			sprintf(setting, "ID%d", j + 1);
			elevatorExits[i][j].id = IniReader::GetInt(section, setting, elevatorExits[i][j].id, elevFile);
			sprintf(setting, "Elevation%d", j + 1);
			elevatorExits[i][j].elevation = IniReader::GetInt(section, setting, elevatorExits[i][j].elevation, elevFile);
			sprintf(setting, "Tile%d", j + 1);
			elevatorExits[i][j].tile = IniReader::GetInt(section, setting, elevatorExits[i][j].tile, elevFile);
		}
	}
}

static void ElevatorsInit() {
	HookCall(0x43EF83, GetMenuHook);
	HookCall(0x43F141, CheckHotKeysHook);
	//HookCall(0x43F2D2, UnknownHook2); // unused

	SafeWrite8(0x43EF76, (BYTE)elevatorCount);
	SafeWriteBatch<DWORD>((DWORD)elevatorExits, {0x43EFA4, 0x43EFB9, 0x43F2FC});
	SafeWriteBatch<DWORD>((DWORD)&elevatorExits[0][0].tile, {0x43EFEA, 0x43F315});
	SafeWrite32(0x43F309, (DWORD)&elevatorExits[0][0].elevation);

	SafeWrite32(0x43F438, (DWORD)&elevatorsFrms[0].main);
	SafeWrite32(0x43F475, (DWORD)&elevatorsFrms[0].buttons);

	// _btncnt
	SafeWriteBatch<DWORD>((DWORD)elevatorsBtnCount, {0x43F65E, 0x43F6BB});
	MakeCall(0x43F05D, GetNumButtonsHook1, 2);
	MakeCall(0x43F184, GetNumButtonsHook2, 2);
	MakeCall(0x43F1E4, GetNumButtonsHook3, 2);
}

void Elevators::init() {
	auto elevPath = IniReader::GetConfigString("Misc", "ElevatorsFile", "");
	if (!elevPath.empty()) {
		const char* elevFile = elevPath.insert(0, ".\\").c_str();
		if (GetFileAttributesA(elevFile) == INVALID_FILE_ATTRIBUTES) return;

		dlogr("Applying elevator patch.", DL_INIT);
		ElevatorsInit();
		LoadElevators(elevFile);
	}
}

}
