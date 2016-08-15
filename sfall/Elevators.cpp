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

static const int ElevatorCount = 50;
static char File[MAX_PATH];

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

static sElevator Elevators[ElevatorCount];
static DWORD Menus[ElevatorCount];

void SetElevator(DWORD id, DWORD index, DWORD value) {
	if(id>=ElevatorCount||index>=12) return;
	*(DWORD*)(((DWORD)&Elevators[id])+index*4)=value;
}

static void __declspec(naked) GetMenuHook() {
	__asm {
		push ebx;
		lea ebx, Menus;
		shl eax, 2;
		mov eax, [ebx+eax];
		call elevator_start_;
		pop ebx;
		ret;
	}
}

static void __declspec(naked) UnknownHook() {
	__asm {
		push ebx;
		lea ebx, Menus;
		shl eax, 2;
		mov eax, [ebx+eax];
		call Check4Keys_;
		pop ebx;
		ret;
	}
}
static void __declspec(naked) UnknownHook2() {
	__asm {
		push ebx;
		lea ebx, Menus;
		shl eax, 2;
		mov eax, [ebx+eax];
		call elevator_end_;
		pop ebx;
		ret;
	}
}

static void __declspec(naked) GetNumButtonsHook1() {
	__asm {
		lea  esi, Menus;
		mov  eax, [esi+edi*4];
		mov  eax, [_btncnt+eax*4];
		push 0x43F064;
		retn;
	}
}
static void __declspec(naked) GetNumButtonsHook2() {
	__asm {
		lea  edx, Menus;
		mov  eax, [edx+edi*4];
		mov  eax, [_btncnt+eax*4];
		push 0x43F18B;
		retn;
	}
}
static void __declspec(naked) GetNumButtonsHook3() {
	__asm {
		lea  eax, Menus;
		mov  eax, [eax+edi*4];
		mov  eax, [_btncnt+eax*4];
		push 0x43F1EB;
		retn;
	}
}

void ResetElevators() {
	memcpy(Elevators, (void*)_retvals, sizeof(sElevator)*24);
	memset(&Elevators[24], 0, sizeof(sElevator)*(ElevatorCount-24));
	for(int i=0;i<24;i++) Menus[i]=i;
	for(int i=24;i<ElevatorCount;i++) Menus[i]=0;
	char section[4];
	if(File) {
		for(int i=0;i<ElevatorCount;i++) {
			_itoa_s(i, section, 10);
			Menus[i]=GetPrivateProfileIntA(section, "Image", Menus[i], File);
			Elevators[i].ID1=GetPrivateProfileIntA(section, "ID1", Elevators[i].ID1, File);
			Elevators[i].ID2=GetPrivateProfileIntA(section, "ID2", Elevators[i].ID2, File);
			Elevators[i].ID3=GetPrivateProfileIntA(section, "ID3", Elevators[i].ID3, File);
			Elevators[i].ID4=GetPrivateProfileIntA(section, "ID4", Elevators[i].ID4, File);
			Elevators[i].Elevation1=GetPrivateProfileIntA(section, "Elevation1", Elevators[i].Elevation1, File);
			Elevators[i].Elevation2=GetPrivateProfileIntA(section, "Elevation2", Elevators[i].Elevation2, File);
			Elevators[i].Elevation3=GetPrivateProfileIntA(section, "Elevation3", Elevators[i].Elevation3, File);
			Elevators[i].Elevation4=GetPrivateProfileIntA(section, "Elevation4", Elevators[i].Elevation4, File);
			Elevators[i].Tile1=GetPrivateProfileIntA(section, "Tile1", Elevators[i].Tile1, File);
			Elevators[i].Tile2=GetPrivateProfileIntA(section, "Tile2", Elevators[i].Tile2, File);
			Elevators[i].Tile3=GetPrivateProfileIntA(section, "Tile3", Elevators[i].Tile3, File);
			Elevators[i].Tile4=GetPrivateProfileIntA(section, "Tile4", Elevators[i].Tile4, File);
		}
	}
}

void ElevatorsInit(char* file) {
	strcpy_s(File, ".\\");
	strcat_s(File, file);
	HookCall(0x43EF83, GetMenuHook);
	HookCall(0x43F141, UnknownHook);
	HookCall(0x43F2D2, UnknownHook2);
	SafeWrite8(0x43EF76, (BYTE)ElevatorCount);
	SafeWrite32(0x43EFA4, (DWORD)Elevators);
	SafeWrite32(0x43EFB9, (DWORD)Elevators);
	SafeWrite32(0x43EFEA, (DWORD)&Elevators[0].Tile1);
	SafeWrite32(0x43F2FC, (DWORD)Elevators);
	SafeWrite32(0x43F309, (DWORD)&Elevators[0].Elevation1);
	SafeWrite32(0x43F315, (DWORD)&Elevators[0].Tile1);

	SafeWrite8(0x43F05D, 0xe9);
	HookCall(0x43F05D, GetNumButtonsHook1);
	SafeWrite8(0x43F184, 0xe9);
	HookCall(0x43F184, GetNumButtonsHook2);
	SafeWrite8(0x43F1E4, 0xe9);
	HookCall(0x43F1E4, GetNumButtonsHook3);
	ResetElevators();
}
