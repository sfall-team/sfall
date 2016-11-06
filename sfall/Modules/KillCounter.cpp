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
#include "..\Version.h"

static int usingExtraKillTypes;
bool UsingExtraKillTypes() { return usingExtraKillTypes!=0; }

//Fallout's idea of _fastcall seems to be different to VS2005's.
//Might as well do this in asm, or the custom prolog code would end up being longer than the function
static DWORD __declspec(naked) ReadKillCounter(DWORD killtype) {
	//if(killtype>38) return 0;
	//return ((WORD*)_pc_kill_counts)[killtype];
	__asm {
		cmp eax, 38;
		jle func;
		xor eax, eax;
		ret;
func:
		push ebx;
		lea ebx, ds:[VarPtr::pc_kill_counts+eax*2];
		xor eax,eax;
		mov ax, word ptr [ebx]
		pop ebx;
		ret;
	}
}

static void __declspec(naked) IncKillCounter(DWORD killtype) {
	//if(killtype>38) return;
	//((WORD*)_pc_kill_counts)[killtype]++;
	__asm {
		cmp eax, 38;
		jle func;
		ret;
func:
		push ebx;
		lea ebx, ds:[VarPtr::pc_kill_counts+eax*2];
		xor eax, eax;
		mov ax, word ptr [ebx];
		inc ax;
		mov word ptr [ebx], ax;
		pop ebx;
		ret;
   }
}

void KillCounterInit(bool use) {
	if(!use) {
		usingExtraKillTypes=0;
		return;
	}
	usingExtraKillTypes=1;

	//Overwrite the function that reads the kill counter with my own
	SafeWrite32(0x004344C0, ((DWORD)&ReadKillCounter) - 0x004344C4);
	SafeWrite32(0x0043A163, ((DWORD)&ReadKillCounter) - 0x0043A167);
	SafeWrite32(0x004571D9, ((DWORD)&ReadKillCounter) - 0x004571DD);

	//Overwrite the function that increments the kill counter with my own
	SafeWrite32(0x00425145, ((DWORD)&IncKillCounter) - 0x00425149);

	//Edit the GetKillTypeName function to accept kill types over 0x13
	SafeWrite8(0x0042D980, 38);
	SafeWrite8(0x0042D990, 38);

	//And the same for GetKillTypeDesc
	SafeWrite8(0x0042D9C0, 38);
	SafeWrite8(0x0042D9D0, 38);
	SafeWrite32(0x0042D9DD, 1488);

	//Change char sheet to loop through the extra kill types
	SafeWrite8(0x004344E4, 38);

	//Where fallout clears the counters
	/*SafeWrite32(0x0042CF5E, sizeof(KillCounters));
	SafeWrite32(0x0042CFEC, sizeof(KillCounters));
	SafeWrite32(0x0042D863, sizeof(KillCounters));
	SafeWrite32(0x0042CF63, (DWORD)KillCounters);
	SafeWrite32(0x0042CFF1, (DWORD)KillCounters);
	SafeWrite32(0x0042D868, (DWORD)KillCounters);

	//Where fallout increments the kill counter
	SafeWrite8(0x0042D881, COUNTERS);
	SafeWrite32(0x0042D895, (DWORD)KillCounters);
	SafeWrite32(0x0042D89E, (DWORD)KillCounters);

	//A function that reads the kill counter
	SafeWrite8(0x0042D8AF, COUNTERS);
	SafeWrite32(0x0042D8B8, (DWORD)KillCounters);

	//Not sure what these two do. Possibly related to loading the names/descriptions?
	SafeWrite32(0x0042D8C6, COUNTERS); //This one causes a crash on load?
	SafeWrite32(0x0042D8CB, (DWORD)KillCounters);

	SafeWrite32(0x0042D8F6, COUNTERS);
	SafeWrite32(0x0042D8FB, (DWORD)KillCounters);*/
}
