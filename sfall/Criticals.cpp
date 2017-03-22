/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include <stdio.h>
#include "Criticals.h"
#include "FalloutEngine.h"
#include "Logging.h"

static const DWORD CritTableCount=2*19+1;              //Number of species in new critical table
//static const DWORD origCritTableSize = 6*9*20;         //Number of entries in original table
static const DWORD CritTableSize = 6*9*CritTableCount; //Number of entries in new critical table
static DWORD mode;

static const char* CritNames[] = {
	"DamageMultiplier",
	"EffectFlags",
	"StatCheck",
	"StatMod",
	"FailureEffect",
	"Message",
	"FailMessage",
};

struct CritStruct {
	union {
		struct {
			DWORD DamageMultiplier;
			DWORD EffectFlags;
			DWORD StatCheck;
			DWORD StatMod;
			DWORD FailureEffect;
			DWORD Message;
			DWORD FailMessage;
		};
		DWORD values[7];
	};
};

static CritStruct* critTable;
static CritStruct* playerCrit;
static bool Inited=false;

void _stdcall SetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element, DWORD value) {
	if(!Inited) return;
	if(critter>=CritTableCount||bodypart>=9||slot>=6||element>=7) return;
	critTable[critter*9*6+bodypart*6+slot].values[element]=value;
}
DWORD _stdcall GetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element) {
	if(!Inited) return 0;
	if(critter>=CritTableCount||bodypart>=9||slot>=6||element>=7) return 0;
	return critTable[critter*9*6+bodypart*6+slot].values[element];
}
void _stdcall ResetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element) {
	if(!Inited) return;
	if(critter>=CritTableCount||bodypart>=9||slot>=6||element>=7) return;
	//It's been a long time since we worried about win9x compatibility, so just sprintf it for goodness sake...
	char section[16];
	sprintf_s(section, "c_%02d_%d_%d", critter, bodypart, slot);
	CritStruct* defaultTable=(CritStruct*)_crit_succ_eff;
	critTable[slot].values[element]=critTable[slot].DamageMultiplier=GetPrivateProfileIntA(section, CritNames[element], defaultTable[slot].values[element], ".\\CriticalOverrides.ini");
}

void CritLoad() {
	if (!Inited) return;
	CritStruct* defaultTable=(CritStruct*)_crit_succ_eff;
	if (mode==1) {
		char section[16];
		dlogr("Setting up critical hit table using CriticalOverrides.ini", DL_CRITICALS);
		memset(critTable, 0, CritTableSize*sizeof(CritStruct));
		for (DWORD critter=0;critter<20;critter++) {
			for (DWORD part=0;part<9;part++) {
				for (DWORD crit=0;crit<6;crit++) {
					sprintf_s(section, "c_%02d_%d_%d", critter, part, crit);
					int slot1=crit+part*6+critter*9*6;
					int slot2=crit+part*6+((critter==19)?38:critter)*9*6;
					for (int i=0;i<7;i++) {
						critTable[slot2].values[i]=GetPrivateProfileIntA(section, CritNames[i], defaultTable[slot1].values[i], ".\\CriticalOverrides.ini");
						if (IsDebug) {
							char logmsg[256];
							if(critTable[slot2].values[i]!=defaultTable[slot1].values[i]) {
								sprintf_s(logmsg, "Entry %s value %d changed from %d to %d", section, i, defaultTable[slot1].values[i], critTable[slot2].values[i]);
								dlogr(logmsg, DL_CRITICALS);
							}
						}
					}
				}
			}
		}
	} else {
		dlogr("Setting up critical hit table using RP fixes", DL_CRITICALS);
		memcpy(critTable, defaultTable, 6*9*19*sizeof(CritStruct));
		memset(&critTable[6*9*19], 0, 6*9*19*sizeof(CritStruct));
		memcpy(playerCrit, (void*)_pc_crit_succ_eff, 6*9*sizeof(CritStruct));

		if (mode==3) {
			char buf[32], buf2[32], buf3[32];
			for (int critter=0;critter<CritTableCount;critter++) {
				sprintf_s(buf, "c_%02d", critter);
				int all;
				if(!(all=GetPrivateProfileIntA(buf, "Enabled", 0, ".\\CriticalOverrides.ini"))) continue;
				for (int part=0;part<9;part++) {
					if (all<2) {
						sprintf_s(buf2, "Part_%d", part);
						if (!GetPrivateProfileIntA(buf, buf2, 0, ".\\CriticalOverrides.ini")) continue;
					}

					sprintf_s(buf2, "c_%02d_%d", critter, part);
					for (int crit=0;crit<6;crit++) {
						int slot=crit+part*6+critter*9*6;
						for (int i=0;i<7;i++) {
							sprintf_s(buf3, "e%d_%s", crit, CritNames[i]);
							critTable[slot].values[i]=GetPrivateProfileIntA(buf2, buf3, critTable[slot].values[i], ".\\CriticalOverrides.ini");
						}
					}
				}
			}
		}
	}
	dlogr("Completed critical hit table.", DL_CRITICALS);
}

#define SetEntry(a,b,c,d,e) defaultTable[a*9*6 + b*6 + c].values[d]=e;
void CritInit() {
	mode=GetPrivateProfileIntA("Misc", "OverrideCriticalTable", 2, ini);
	if(mode<0||mode>3) mode=0;

	if(!mode) return;

	dlog("Initializing critical table override.", DL_INIT);
	critTable=new CritStruct[CritTableSize];
	playerCrit=&critTable[6*9*38];
	SafeWrite32(0x423F96, (DWORD)playerCrit);
	SafeWrite32(0x423FB3, (DWORD)critTable);

	if(mode==2 || mode==3) {
		CritStruct* defaultTable=(CritStruct*)_crit_succ_eff;

		SetEntry(2,4,1,4,0);
		SetEntry(2,4,1,5,5216);
		SetEntry(2,4,1,6,5000);

		SetEntry(2,4,2,4,0);
		SetEntry(2,4,2,5,5216);
		SetEntry(2,4,2,6,5000);
	
		SetEntry(2,5,1,4,0);
		SetEntry(2,5,1,5,5216);
		SetEntry(2,5,1,6,5000);

		SetEntry(2,5,2,4,0);
		SetEntry(2,5,2,5,5216);
		SetEntry(2,5,2,6,5000);

		SetEntry(3,5,1,6,5306);

		SetEntry(4,0,4,2,-1);

		SetEntry(5,0,4,2,-1);

		SetEntry(6,4,1,4,2);

		SetEntry(6,5,1,4,2);

		SetEntry(6,5,2,6,5608);

		SetEntry(9,3,3,4,2);

		SetEntry(13,5,1,4,4);
		SetEntry(13,5,2,4,4);
		SetEntry(13,5,3,4,4);
		SetEntry(13,5,4,4,4);
		SetEntry(13,5,5,4,4);

		SetEntry(18,0,0,5,5001);
		SetEntry(18,0,1,5,5001);
		SetEntry(18,0,2,5,5001);
		SetEntry(18,0,3,5,7105);
		SetEntry(18,0,4,5,7101);
		SetEntry(18,0,4,6,7104);
		SetEntry(18,0,5,5,7101);

		SetEntry(18,1,0,5,5008);
		SetEntry(18,1,1,5,5008);
		SetEntry(18,1,2,5,5009);
		SetEntry(18,1,3,5,5009);
		SetEntry(18,1,4,5,7102);
		SetEntry(18,1,5,5,7102);
		
		SetEntry(18,2,0,5,5008);
		SetEntry(18,2,1,5,5008);
		SetEntry(18,2,2,5,5009);
		SetEntry(18,2,3,5,5009);
		SetEntry(18,2,4,5,7102);
		SetEntry(18,2,5,5,7102);

		SetEntry(18,3,4,5,7101);
		SetEntry(18,3,5,5,7101);

		SetEntry(18,4,0,5,5023);
		SetEntry(18,4,1,5,7101);
		SetEntry(18,4,1,6,7103);
		SetEntry(18,4,2,5,7101);
		SetEntry(18,4,2,6,7103);
		SetEntry(18,4,3,5,7103);
		SetEntry(18,4,4,5,7103);
		SetEntry(18,4,5,5,7103);

		SetEntry(18,5,0,5,5023);
		SetEntry(18,5,1,5,7101);
		SetEntry(18,5,1,6,7103);
		SetEntry(18,5,2,5,7101);
		SetEntry(18,5,2,6,7103);
		SetEntry(18,5,3,5,7103);
		SetEntry(18,5,4,5,7103);
		SetEntry(18,5,5,5,7103);

		SetEntry(18,6,0,5,5027);
		SetEntry(18,6,1,5,5027);
		SetEntry(18,6,2,5,5027);
		//SetEntry(18,6,2,6,0);
		SetEntry(18,6,3,5,5027);
		SetEntry(18,6,4,5,7104);
		SetEntry(18,6,5,5,7104);

		SetEntry(18,7,0,5,5033);
		SetEntry(18,7,1,5,5027);
		SetEntry(18,7,1,6,7101);
		SetEntry(18,7,2,5,7101);
		SetEntry(18,7,3,5,7101);
		SetEntry(18,7,4,5,7101);
		SetEntry(18,7,5,5,7101);
	}

	Inited=true;
	dlogr(" Done", DL_INIT);
}
