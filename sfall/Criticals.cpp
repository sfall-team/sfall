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

#include <stdio.h>

#include "main.h"
#include "FalloutEngine.h"
#include "Logging.h"

#include "Criticals.h"

static std::string critTableFile(".\\");

static const DWORD CritTableCount = 2 * 19 + 1;            // Number of species in new critical table
//static const DWORD origCritTableSize = 6 * 9 * 20;         // Number of entries in original table
static const DWORD CritTableSize = 6 * 9 * CritTableCount; // Number of entries in new critical table
static DWORD mode;

static const char* critNames[] = {
	"DamageMultiplier",
	"EffectFlags",
	"StatCheck",
	"StatMod",
	"FailureEffect",
	"Message",
	"FailMessage",
};

static CritInfo baseCritTable[CritTableSize] = {0}; // Base critical table set up via enabling OverrideCriticalTable in ddraw.ini
static CritInfo critTable[CritTableSize];
static CritInfo* playerCrit;

static bool Inited = false;

static const char* errorTable = "\nError: %s - function requires enabling OverrideCriticalTable in ddraw.ini.";

void SetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element, DWORD value) {
	if (!Inited) {
		DebugPrintf(errorTable, "set_critical_table()");
		return;
	}
	critTable[critter * 9 * 6 + bodypart * 6 + slot].values[element] = value;
}

DWORD GetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element) {
	if (!Inited) {
		DebugPrintf(errorTable, "get_critical_table()");
		return 0;
	}
	return critTable[critter * 9 * 6 + bodypart * 6 + slot].values[element];
}

void ResetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element) {
	if (!Inited) {
		DebugPrintf(errorTable, "reset_critical_table()");
		return;
	}
	critTable[critter * 9 * 6 + bodypart * 6 + slot].values[element] = baseCritTable[critter * 9 * 6 + bodypart * 6 + slot].values[element];
}

static int CritTableLoad() {
	if (mode == 1) {
		dlogr("Setting up critical hit table using CriticalOverrides.ini (old fmt)", DL_CRITICALS);
		if (GetFileAttributes(critTableFile.c_str()) == INVALID_FILE_ATTRIBUTES) return 1;
		CritInfo* defaultTable = ptr_crit_succ_eff;
		char section[16];
		for (DWORD critter = 0; critter < 20; critter++) {
			for (DWORD part = 0; part < 9; part++) {
				for (DWORD crit = 0; crit < 6; crit++) {
					sprintf_s(section, "c_%02d_%d_%d", critter, part, crit);
					int slot1 = crit + part * 6 + critter * 9 * 6; // default effect
					int slot2 = crit + part * 6 + ((critter == 19) ? 38 : critter) * 9 * 6; // new effect
					for (int i = 0; i < 7; i++) {
						baseCritTable[slot2].values[i] = iniGetInt(section, critNames[i], defaultTable[slot1].values[i], critTableFile.c_str());
						if (isDebug) {
							char logmsg[256];
							if (baseCritTable[slot2].values[i] != defaultTable[slot1].values[i]) {
								sprintf_s(logmsg, "  Entry %s value %d changed from %d to %d", section, i, defaultTable[slot1].values[i], baseCritTable[slot2].values[i]);
								dlogr(logmsg, DL_CRITICALS);
							}
						}
					}
				}
			}
		}
	} else {
		dlog("Setting up critical hit table using RP fixes", DL_CRITICALS);
		memcpy(baseCritTable, ptr_crit_succ_eff, 19 * 6 * 9 * sizeof(CritInfo));
		//memset(&baseCritTable[6 * 9 * 19], 0, 19 * 6 * 9 * sizeof(CritInfo));
		memcpy(&baseCritTable[6 * 9 * 38], ptr_pc_crit_succ_eff, 6 * 9 * sizeof(CritInfo)); // PC crit table

		if (mode == 3) {
			dlogr(" and CriticalOverrides.ini (new fmt)", DL_CRITICALS);
			if (GetFileAttributes(critTableFile.c_str()) == INVALID_FILE_ATTRIBUTES) return 1;
			char buf[32], buf2[32], buf3[32];
			for (int critter = 0; critter < CritTableCount; critter++) {
				sprintf_s(buf, "c_%02d", critter);
				int all;
				if (!(all = iniGetInt(buf, "Enabled", 0, critTableFile.c_str()))) continue;
				for (int part = 0; part < 9; part++) {
					if (all < 2) {
						sprintf_s(buf2, "Part_%d", part);
						if (!iniGetInt(buf, buf2, 0, critTableFile.c_str())) continue;
					}

					sprintf_s(buf2, "c_%02d_%d", critter, part);
					for (int crit = 0; crit < 6; crit++) {
						int slot = crit + part * 6 + critter * 9 * 6;
						for (int i = 0; i < 7; i++) {
							sprintf_s(buf3, "e%d_%s", crit, critNames[i]);
							baseCritTable[slot].values[i] = iniGetInt(buf2, buf3, baseCritTable[slot].values[i], critTableFile.c_str());
						}
					}
				}
			}
		} else {
			dlog("\n", DL_CRITICALS);
		}
	}
	return 0;
}

#define SetEntry(critter, bodypart, effect, param, value) ptr_crit_succ_eff[critter * 9 * 6 + bodypart * 6 + effect].values[param] = value;

static void CriticalTableOverride() {
	dlogr("Initializing critical table override...", DL_INIT);
	playerCrit = &critTable[6 * 9 * 38];
	SafeWrite32(0x423F96, (DWORD)playerCrit);
	SafeWrite32(0x423FB3, (DWORD)critTable);

	if (mode == 2 || mode == 3) { // bug fixes
		SetEntry(0, 8, 2, 1, 2050);
		SetEntry(0, 8, 2, 5, 5019);

		SetEntry(2, 4, 1, 4, 0);
		SetEntry(2, 4, 1, 5, 5216);
		SetEntry(2, 4, 1, 6, 5000);

		SetEntry(2, 4, 2, 4, 0);
		SetEntry(2, 4, 2, 5, 5216);
		SetEntry(2, 4, 2, 6, 5000);

		SetEntry(2, 5, 1, 4, 0);
		SetEntry(2, 5, 1, 5, 5216);
		SetEntry(2, 5, 1, 6, 5000);

		SetEntry(2, 5, 2, 4, 0);
		SetEntry(2, 5, 2, 5, 5216);
		SetEntry(2, 5, 2, 6, 5000);

		SetEntry(2, 8, 1, 0, 4);
		SetEntry(2, 8, 2, 1, 2050);
		SetEntry(2, 8, 2, 5, 5212);

		SetEntry(3, 5, 1, 6, 5306);

		SetEntry(4, 0, 4, 2, -1);

		SetEntry(5, 0, 4, 2, -1);

		SetEntry(6, 4, 1, 4, 2);

		SetEntry(6, 5, 1, 4, 2);
		SetEntry(6, 5, 2, 6, 5608);

		SetEntry(9, 3, 3, 4, 2);

		SetEntry(9, 8, 3, 4, 2);

		SetEntry(13, 5, 1, 4, 4);
		SetEntry(13, 5, 2, 4, 4);
		SetEntry(13, 5, 3, 4, 4);
		SetEntry(13, 5, 4, 4, 4);
		SetEntry(13, 5, 5, 4, 4);

		SetEntry(15, 8, 0, 5, 6701);
		SetEntry(15, 8, 1, 5, 6701);
		SetEntry(15, 8, 2, 1, 2050);
		SetEntry(15, 8, 2, 5, 6704);
		SetEntry(15, 8, 3, 5, 6704);
		SetEntry(15, 8, 4, 5, 6704);
		SetEntry(15, 8, 5, 5, 6704);

		SetEntry(16, 8, 2, 1, 2050);

		SetEntry(17, 8, 2, 1, 2050);

		SetEntry(18, 0, 0, 5, 5001);
		SetEntry(18, 0, 1, 5, 5001);
		SetEntry(18, 0, 2, 5, 5001);
		SetEntry(18, 0, 3, 5, 7105);
		SetEntry(18, 0, 4, 5, 7101);
		SetEntry(18, 0, 4, 6, 7104);
		SetEntry(18, 0, 5, 5, 7101);

		SetEntry(18, 1, 0, 5, 5008);
		SetEntry(18, 1, 1, 5, 5008);
		SetEntry(18, 1, 2, 5, 5009);
		SetEntry(18, 1, 3, 5, 5009);
		SetEntry(18, 1, 4, 5, 7102);
		SetEntry(18, 1, 5, 5, 7102);

		SetEntry(18, 2, 0, 5, 5008);
		SetEntry(18, 2, 1, 5, 5008);
		SetEntry(18, 2, 2, 5, 5009);
		SetEntry(18, 2, 3, 5, 5009);
		SetEntry(18, 2, 4, 5, 7102);
		SetEntry(18, 2, 5, 5, 7102);

		SetEntry(18, 3, 4, 5, 7101);
		SetEntry(18, 3, 5, 5, 7101);

		SetEntry(18, 4, 0, 5, 5023);
		SetEntry(18, 4, 1, 5, 7101);
		SetEntry(18, 4, 1, 6, 7103);
		SetEntry(18, 4, 2, 5, 7101);
		SetEntry(18, 4, 2, 6, 7103);
		SetEntry(18, 4, 3, 5, 7103);
		SetEntry(18, 4, 4, 5, 7103);
		SetEntry(18, 4, 5, 5, 7103);

		SetEntry(18, 5, 0, 5, 5023);
		SetEntry(18, 5, 1, 5, 7101);
		SetEntry(18, 5, 1, 6, 7103);
		SetEntry(18, 5, 2, 5, 7101);
		SetEntry(18, 5, 2, 6, 7103);
		SetEntry(18, 5, 3, 5, 7103);
		SetEntry(18, 5, 4, 5, 7103);
		SetEntry(18, 5, 5, 5, 7103);

		SetEntry(18, 6, 0, 5, 5027);
		SetEntry(18, 6, 1, 5, 5027);
		SetEntry(18, 6, 2, 5, 5027);
		SetEntry(18, 6, 3, 5, 5027);
		SetEntry(18, 6, 4, 5, 7104);
		SetEntry(18, 6, 5, 5, 7104);

		SetEntry(18, 7, 0, 5, 5033);
		SetEntry(18, 7, 1, 5, 5027);
		SetEntry(18, 7, 1, 6, 7101);
		SetEntry(18, 7, 2, 5, 7101);
		SetEntry(18, 7, 3, 5, 7101);
		SetEntry(18, 7, 4, 5, 7101);
		SetEntry(18, 7, 5, 5, 7101);

		SetEntry(18, 8, 2, 0, 3);
		SetEntry(18, 8, 4, 0, 4);
		SetEntry(18, 8, 4, 5, 7101);
		SetEntry(18, 8, 5, 5, 7101);
	}

	if (CritTableLoad()) {
		dlogr("Failed to initialize critical hit table from file.", DL_INIT);
	} else {
		dlogr("Completed applying critical hit table.", DL_INIT);
	}
	Inited = true;
}
#undef SetEntry

static void RemoveCriticalTimeLimitsPatch() {
	if (GetConfigInt("Misc", "RemoveCriticalTimelimits", 0)) {
		dlog("Removing critical time limits.", DL_INIT);
		SafeWrite8(0x424118, CODETYPE_JumpShort); // jump to 0x424131
		const DWORD rollChkCritAddr[] = {0x4A3052, 0x4A3093};
		SafeWriteBatch<WORD>(0x9090, rollChkCritAddr);
		dlogr(" Done", DL_INIT);
	}
}

void Criticals_Init() {
	mode = GetConfigInt("Misc", "OverrideCriticalTable", 2);
	if (mode < 0 || mode > 3) mode = 0;
	if (mode) {
		critTableFile += GetConfigString("Misc", "OverrideCriticalFile", "CriticalOverrides.ini", MAX_PATH);
		CriticalTableOverride();
	}

	RemoveCriticalTimeLimitsPatch();
}

void CritLoad() {
	if (!Inited) return;
	memcpy(critTable, baseCritTable, sizeof(critTable)); // Apply loaded critical table
}
