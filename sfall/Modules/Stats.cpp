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

#include <math.h>
#include <stdio.h>
#include "..\FalloutEngine\Fallout2.h"
#include "Stats.h"

static DWORD StatMaximumsPC[STAT_max_stat];
static DWORD StatMinimumsPC[STAT_max_stat];
static DWORD StatMaximumsNPC[STAT_max_stat];
static DWORD StatMinimumsNPC[STAT_max_stat];

static DWORD cCritter;

static DWORD xpTable[99];

static void __declspec(naked) GetCurrentStatHook1() {
	__asm {
		mov cCritter, eax;
		push ebx;
		push ecx;
		push esi;
		push edi;
		push ebp;
		push 0x4AEF4D;
		retn;
	}
}
static void __declspec(naked) GetCurrentStatHook2() {
	__asm {
		shl esi, 2;
		mov eax, cCritter;
		cmp eax, dword ptr ds:[VARPTR_obj_dude];
		je pc;
		cmp ecx, StatMinimumsNPC[esi];
		jg npc1;
		mov eax, StatMinimumsNPC[esi];
		jmp end;
npc1:
		cmp ecx, StatMaximumsNPC[esi];
		jl npc2;
		mov eax, StatMaximumsNPC[esi];
		jmp end;
npc2:
		mov eax, ecx;
		jmp end;
pc:
		cmp ecx, StatMinimumsPC[esi];
		jge pc1;
		mov eax, StatMinimumsPC[esi];
		jmp end;
pc1:
		cmp ecx, StatMaximumsPC[esi];
		jle pc2;
		mov eax, StatMaximumsPC[esi];
		jmp end;
pc2:
		mov eax, ecx;
end:
		push 0x4AF3D7;
		retn;
	}
}

static void __declspec(naked) SetCurrentStatHook() {
	__asm {
		cmp esi, dword ptr ds:[VARPTR_obj_dude];
		je pc;
		cmp ebx, StatMinimumsNPC[ecx*4];
		jl fail;
		cmp ebx, StatMaximumsNPC[ecx*4];
		jg fail;
		jmp end;
pc:
		cmp ebx, StatMinimumsPC[ecx*4];
		jl fail;
		cmp ebx, StatMaximumsPC[ecx*4];
		jg fail;
		jmp end;
fail:
		push 0x4AF57E;
		retn;
end:
		push 0x4AF59C;
		retn;
	}
}

static void __declspec(naked) GetLevelXPHook() {
	__asm {
		dec eax;
		mov eax, [xpTable+eax*4];
		ret;
	}
}
static void __declspec(naked) GetNextLevelXPHook() {
	__asm {
		mov eax, ds:[VARPTR_Level_];
		jmp GetLevelXPHook;
	}
}

unsigned short StandardApAcBonus=4;
unsigned short ExtraApAcBonus=4;
static const DWORD ApAcRetAddr=0x4AF0A4;
static void __declspec(naked) ApplyApAcBonus() {
	__asm {
		push edi;
		push edx;
		cmp [esp+12], 2;
		jge h2hEvade;
		xor edi, edi;
		jmp standard;
h2hEvade:
		mov edx, PERK_hth_evade_perk;
		mov eax, dword ptr ds:[VARPTR_obj_dude];
		call FuncOffs::perk_level_;
		imul ax, ExtraApAcBonus;
		imul ax, [ebx+0x40];
		mov edi, eax;
standard:
		mov eax, [ebx+0x40];
		imul ax, StandardApAcBonus;
		add eax, edi;
		shr eax, 2;
		pop edx;
		pop edi;
		jmp ApAcRetAddr;
	}
}

static int StatFormulas[33*2];
static int StatShifts[33*7];
static double StatMulti[33*7];
static int __declspec(naked) _stdcall StatLevel(void* critter, int id) {
	__asm {
		mov eax, [esp+4];
		mov edx, [esp+8];
		call FuncOffs::stat_level_;
		retn 8;
	}
}
static void __declspec(naked) _stdcall ProtoPtr(DWORD pid, int** proto) {
	__asm {
		mov eax, [esp+4];
		mov edx, [esp+8];
		call FuncOffs::proto_ptr_;
		retn 8;
	}
}
static void _stdcall StatRecalcDerived(DWORD* critter) {
	int basestats[7];
	for(int i=0;i<7;i++) basestats[i]=StatLevel(critter, i);
	int* proto;
	ProtoPtr(critter[25], &proto);

	for(int i=7;i<=32;i++) {
		if(i>=17&&i<=30) continue;
		
		double sum=0;
		for(int j=0;j<7;j++) {
			sum+=(basestats[j]+StatShifts[i*7+j])*StatMulti[i*7+j];
		}
		proto[i+9]=StatFormulas[i*2] + (int)floor(sum);
		if(proto[i+9]<StatFormulas[i*2+1]) proto[i+9]=StatFormulas[i*2+1];
	}
}

static void __declspec(naked) stat_recalc_derived() {
	__asm {
		push edx;
		push ecx;
		push eax;
		call StatRecalcDerived;
		pop ecx;
		pop edx;
		retn;
	}
}

void StatsReset() {
	for(int i = 0; i < STAT_max_stat; i++) {
		StatMaximumsPC[i] = StatMaximumsNPC[i] = *(VarPtr::stat_data + 4 + i*6);
		StatMinimumsPC[i] = StatMinimumsNPC[i] = *(VarPtr::stat_data + 3 + i*6);
	}
	StandardApAcBonus = 4;
	ExtraApAcBonus = 4;
}
void StatsInit() {
	StatsReset();
	SafeWrite8(0x004AEF48, 0xe9);
	HookCall(0x004AEF48, GetCurrentStatHook1);
	SafeWrite8(0x004AF3AF, 0xe9);
	HookCall(0x004AF3AF, GetCurrentStatHook2);
	SafeWrite8(0x004AF56A, 0xe9);
	HookCall(0x004AF56A, SetCurrentStatHook);
	SafeWrite8(0x4AF09C, 0xe9);
	HookCall(0x4AF09C, ApplyApAcBonus);

	char table[2048];
	GetPrivateProfileString("Misc", "XPTable", "", table, 2048, ini);
	if(strlen(table)>0) {
		char *ptr=table, *ptr2;
		DWORD level=0;
		
		HookCall(0x434AA7, GetNextLevelXPHook);
		HookCall(0x439642, GetNextLevelXPHook);
		HookCall(0x4AFB22, GetNextLevelXPHook);
		HookCall(0x496C8D, GetLevelXPHook);
		HookCall(0x4AFC53, GetLevelXPHook);

		while((ptr2=strstr(ptr, ","))&&level<99) {
			ptr2[0]='\0';
			xpTable[level++]=atoi(ptr);
			ptr=ptr2+1;
		}
		if(level<99&&ptr[0]!='\0') {
			xpTable[level++]=atoi(ptr);
		}
		for(int i=level;i<99;i++) xpTable[i]=-1;
		SafeWrite8(0x4AFB1B, (BYTE)(level+1));
	}

	GetPrivateProfileStringA("Misc", "DerivedStats", "", table, 2048, ini);
	if(strlen(table)) {
		MakeCall(0x4AF6FC, &stat_recalc_derived, true);
		memset(StatFormulas, 0, sizeof(StatFormulas));
		memset(StatShifts, 0, sizeof(StatShifts));
		memset(StatMulti, 0, sizeof(StatMulti));

		StatFormulas[7*2]=15; //max hp
		StatMulti[7*7+0]=1;
		StatMulti[7*7+2]=2;
		StatFormulas[8*2]=5; //max ap
		StatMulti[8*7+5]=0.5;
		StatMulti[9*7+5]=1; //ac
		StatFormulas[11*2+1]=1; //melee damage
		StatShifts[11*7+0]=-5;
		StatMulti[11*7+0]=1;
		StatFormulas[12*2]=25; //carry weight
		StatMulti[12*7+0]=25;
		StatMulti[13*7+1]=2; //sequence
		StatFormulas[14*2+1]=1; //heal rate
		StatMulti[14*7+2]=1.0/3.0;
		StatMulti[15*7+6]=1; //critical chance
		StatMulti[31*7+2]=2; //rad resist
		StatMulti[32*7+2]=5; //poison resist

		char key[6], buf2[256], buf3[256];
		strcpy(buf3, table);
		sprintf(table, ".\\%s", buf3);
		for(int i=7;i<=32;i++) {
			if(i>=17&&i<=30) continue;

			_itoa(i, key, 10);
			StatFormulas[i*2]=GetPrivateProfileInt(key, "base", StatFormulas[i*2], table);
			StatFormulas[i*2+1]=GetPrivateProfileInt(key, "min", StatFormulas[i*2+1], table);
			for(int j=0;j<7;j++) {
				sprintf(buf2, "shift%d", j);
				StatShifts[i*7+j]=GetPrivateProfileInt(key, buf2, StatShifts[i*7+0], table);
				sprintf(buf2, "multi%d", j);
				_gcvt(StatMulti[i*7+j], 16, buf3);
				GetPrivateProfileStringA(key, buf2, buf3, buf2, 256, table);
				StatMulti[i*7+j]=atof(buf2);
			}
		}
	}
}

void _stdcall SetPCStatMax(int stat, int i) {
	if(stat<0||stat>=STAT_max_stat) return;
	StatMaximumsPC[stat]=i;
}
void _stdcall SetPCStatMin(int stat, int i) {
	if(stat<0||stat>=STAT_max_stat) return;
	StatMinimumsPC[stat]=i;
}
void _stdcall SetNPCStatMax(int stat, int i) {
	if(stat<0||stat>=STAT_max_stat) return;
	StatMaximumsNPC[stat]=i;
}
void _stdcall SetNPCStatMin(int stat, int i) {
	if(stat<0||stat>=STAT_max_stat) return;
	StatMinimumsNPC[stat]=i;
}