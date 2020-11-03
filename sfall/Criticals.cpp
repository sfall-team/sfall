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

enum BodyPart {
	Head,
	ArmLeft,
	ArmRight,
	Torso,
	LegRight,
	LegLeft,
	Eyes,
	Groin,
	Uncalled
};

enum CritParam {
	DmgMult,
	Flags,
	StatCheck,
	StatMod,
	FlagsFail,
	Message,
	MsgFail
};

#define SetEntry(critter, bodypart, effect, param, value) ptr_crit_succ_eff[critter * 9 * 6 + bodypart * 6 + effect].values[param] = value

static void CriticalTableOverride() {
	dlogr("Initializing critical table override...", DL_INIT);
	playerCrit = &critTable[6 * 9 * 38];
	SafeWrite32(0x423F96, (DWORD)playerCrit);
	SafeWrite32(0x423FB3, (DWORD)critTable);

	if (mode == 2 || mode == 3) { // bug fixes
		// Children
		SetEntry(2, LegRight, 1, FlagsFail, 0);
		SetEntry(2, LegRight, 1, Message,   5216);
		SetEntry(2, LegRight, 1, MsgFail,   5000);

		SetEntry(2, LegRight, 2, FlagsFail, 0);
		SetEntry(2, LegRight, 2, Message,   5216);
		SetEntry(2, LegRight, 2, MsgFail,   5000);

		SetEntry(2, LegLeft,  1, FlagsFail, 0);
		SetEntry(2, LegLeft,  1, Message,   5216);
		SetEntry(2, LegLeft,  1, MsgFail,   5000);

		SetEntry(2, LegLeft,  2, FlagsFail, 0);
		SetEntry(2, LegLeft,  2, Message,   5216);
		SetEntry(2, LegLeft,  2, MsgFail,   5000);

		// Super Mutants
		SetEntry(3, LegLeft,  1, MsgFail,   5306);

		// Ghouls
		SetEntry(4, Head,     4, StatCheck, -1);

		// Brahmin
		SetEntry(5, Head,     4, StatCheck, -1);

		// Radscorpions
		SetEntry(6, LegRight, 1, FlagsFail, DAM_KNOCKED_DOWN);

		SetEntry(6, LegLeft,  1, FlagsFail, DAM_KNOCKED_DOWN);
		SetEntry(6, LegLeft,  2, MsgFail,   5608);

		// Centaurs
		SetEntry(9, Torso,    3, FlagsFail, DAM_KNOCKED_DOWN);

		// Deathclaws
		SetEntry(13, LegLeft, 1, FlagsFail, DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 2, FlagsFail, DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 3, FlagsFail, DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 4, FlagsFail, DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 5, FlagsFail, DAM_CRIP_LEG_LEFT);

		// Big Bad Boss
		SetEntry(18, Head,     0, Message,  5001);
		SetEntry(18, Head,     1, Message,  5001);
		SetEntry(18, Head,     2, Message,  5001);
		SetEntry(18, Head,     3, Message,  7105);
		SetEntry(18, Head,     4, Message,  7101);
		SetEntry(18, Head,     4, MsgFail,  7104);
		SetEntry(18, Head,     5, Message,  7101);

		SetEntry(18, ArmLeft,  0, Message,  5008);
		SetEntry(18, ArmLeft,  1, Message,  5008);
		SetEntry(18, ArmLeft,  2, Message,  5009);
		SetEntry(18, ArmLeft,  3, Message,  5009);
		SetEntry(18, ArmLeft,  4, Message,  7102);
		SetEntry(18, ArmLeft,  5, Message,  7102);

		SetEntry(18, ArmRight, 0, Message,  5008);
		SetEntry(18, ArmRight, 1, Message,  5008);
		SetEntry(18, ArmRight, 2, Message,  5009);
		SetEntry(18, ArmRight, 3, Message,  5009);
		SetEntry(18, ArmRight, 4, Message,  7102);
		SetEntry(18, ArmRight, 5, Message,  7102);

		SetEntry(18, Torso,    4, Message,  7101);
		SetEntry(18, Torso,    5, Message,  7101);

		SetEntry(18, LegRight, 0, Message,  5023);
		SetEntry(18, LegRight, 1, Message,  7101);
		SetEntry(18, LegRight, 1, MsgFail,  7103);
		SetEntry(18, LegRight, 2, Message,  7101);
		SetEntry(18, LegRight, 2, MsgFail,  7103);
		SetEntry(18, LegRight, 3, Message,  7103);
		SetEntry(18, LegRight, 4, Message,  7103);
		SetEntry(18, LegRight, 5, Message,  7103);

		SetEntry(18, LegLeft,  0, Message,  5023);
		SetEntry(18, LegLeft,  1, Message,  7101);
		SetEntry(18, LegLeft,  1, MsgFail,  7103);
		SetEntry(18, LegLeft,  2, Message,  7101);
		SetEntry(18, LegLeft,  2, MsgFail,  7103);
		SetEntry(18, LegLeft,  3, Message,  7103);
		SetEntry(18, LegLeft,  4, Message,  7103);
		SetEntry(18, LegLeft,  5, Message,  7103);

		SetEntry(18, Eyes,     0, Message,  5027);
		SetEntry(18, Eyes,     1, Message,  5027);
		SetEntry(18, Eyes,     2, Message,  5027);
		SetEntry(18, Eyes,     3, Message,  5027);
		SetEntry(18, Eyes,     4, Message,  7104);
		SetEntry(18, Eyes,     5, Message,  7104);

		SetEntry(18, Groin,    0, Message,  5033);
		SetEntry(18, Groin,    1, Message,  5027);
		SetEntry(18, Groin,    1, MsgFail,  7101);
		SetEntry(18, Groin,    2, Message,  7101);
		SetEntry(18, Groin,    3, Message,  7101);
		SetEntry(18, Groin,    4, Message,  7101);
		SetEntry(18, Groin,    5, Message,  7101);

		// Fixes for uncalled tables
		// Men
		SetEntry(0, Uncalled, 2, Flags,     DAM_KNOCKED_DOWN | DAM_BYPASS); // 0
		SetEntry(0, Uncalled, 2, Message,   5019); // 5018

		// Children
		SetEntry(2, Uncalled, 1, DmgMult,   4);    // 3
		SetEntry(2, Uncalled, 2, Flags,     DAM_KNOCKED_DOWN | DAM_BYPASS); // DAM_BYPASS
		SetEntry(2, Uncalled, 2, Message,   5212); // 5211

		// Centaurs
		SetEntry(9, Uncalled, 3, FlagsFail, DAM_KNOCKED_DOWN); // 0

		// Geckos
		SetEntry(15, Uncalled, 0, Message,  6701); // 6700
		SetEntry(15, Uncalled, 1, Message,  6701); // 6700
		SetEntry(15, Uncalled, 2, Flags,    DAM_KNOCKED_DOWN | DAM_BYPASS); // 0
		SetEntry(15, Uncalled, 2, Message,  6704); // 6700
		SetEntry(15, Uncalled, 3, Message,  6704); // 6700
		SetEntry(15, Uncalled, 4, Message,  6704); // 6700
		SetEntry(15, Uncalled, 5, Message,  6704); // 6700

		// Aliens
		SetEntry(16, Uncalled, 2, Flags,    DAM_KNOCKED_DOWN | DAM_BYPASS); // 0

		// Giant Ants
		SetEntry(17, Uncalled, 2, Flags,    DAM_KNOCKED_DOWN | DAM_BYPASS); // 0

		// Big Bad Boss
		SetEntry(18, Uncalled, 2, DmgMult,  3);    // 4
		SetEntry(18, Uncalled, 4, DmgMult,  4);    // 5
		SetEntry(18, Uncalled, 4, Message,  7101); // 7106
		SetEntry(18, Uncalled, 5, Message,  7101); // 7106
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
