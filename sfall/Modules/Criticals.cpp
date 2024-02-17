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

#include <stdio.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Logging.h"
#include "LoadGameHook.h"

#include "Criticals.h"

namespace sfall
{

static std::string critTableFile(".\\");

const DWORD Criticals::critTableCount = 2 * 19 + 1; // Number of species in new critical table

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

static fo::CritInfo baseCritTable[Criticals::critTableCount][9][6] = {0}; // Base critical table set up via enabling OverrideCriticalTable in ddraw.ini
static fo::CritInfo critTable[Criticals::critTableCount][9][6];
static fo::CritInfo (*playerCrit)[9][6];

static bool Inited = false;

static const char* errorTable = "\nError: %s - function requires enabling OverrideCriticalTable in ddraw.ini.";

void Criticals::SetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element, DWORD value) {
	if (!Inited) {
		fo::func::debug_printf(errorTable, "set_critical_table()");
		return;
	}
	critTable[critter][bodypart][slot].values[element] = value;
}

DWORD Criticals::GetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element) {
	if (!Inited) {
		fo::func::debug_printf(errorTable, "get_critical_table()");
		return 0;
	}
	return critTable[critter][bodypart][slot].values[element];
}

void Criticals::ResetCriticalTable(DWORD critter, DWORD bodypart, DWORD slot, DWORD element) {
	if (!Inited) {
		fo::func::debug_printf(errorTable, "reset_critical_table()");
		return;
	}
	critTable[critter][bodypart][slot].values[element] = baseCritTable[critter][bodypart][slot].values[element];
}

static int CritTableLoad() {
	if (mode == 1) {
		dlogr("Setting up critical hit table using CriticalOverrides.ini (old fmt).", DL_CRITICALS);
		if (GetFileAttributesA(critTableFile.c_str()) == INVALID_FILE_ATTRIBUTES) return 1;
		char section[16];
		for (DWORD critter = 0; critter < 20; critter++) {
			for (DWORD part = 0; part < 9; part++) {
				for (DWORD crit = 0; crit < 6; crit++) {
					sprintf_s(section, "c_%02d_%d_%d", critter, part, crit);
					DWORD newCritter = (critter == 19) ? 38 : critter;
					fo::CritInfo& newEffect = baseCritTable[newCritter][part][crit];
					fo::CritInfo& defaultEffect = fo::var::crit_succ_eff[critter][part][crit];
					for (int i = 0; i < 7; i++) {
						newEffect.values[i] = IniReader::GetInt(section, critNames[i], defaultEffect.values[i], critTableFile.c_str());
						if (isDebug) {
							char logmsg[256];
							if (newEffect.values[i] != defaultEffect.values[i]) {
								sprintf_s(logmsg, "  Entry %s value %d changed from %d to %d", section, i, defaultEffect.values[i], newEffect.values[i]);
								dlogr(logmsg, DL_CRITICALS);
							}
						}
					}
				}
			}
		}
	} else {
		dlog("Setting up critical hit table using RP fixes", DL_CRITICALS);
		memcpy(baseCritTable, fo::var::crit_succ_eff, 19 * 6 * 9 * sizeof(fo::CritInfo));
		//memset(&baseCritTable[19], 0, 19 * 6 * 9 * sizeof(fo::CritInfo));
		memcpy(&baseCritTable[38], fo::var::pc_crit_succ_eff, 6 * 9 * sizeof(fo::CritInfo)); // PC crit table

		if (mode == 3) {
			dlogr(" and CriticalOverrides.ini (new fmt).", DL_CRITICALS);
			if (GetFileAttributesA(critTableFile.c_str()) == INVALID_FILE_ATTRIBUTES) return 1;
			char buf[32], buf2[32], buf3[32];
			for (int critter = 0; critter < Criticals::critTableCount; critter++) {
				sprintf_s(buf, "c_%02d", critter);
				int all;
				if (!(all = IniReader::GetInt(buf, "Enabled", 0, critTableFile.c_str()))) continue;
				for (int part = 0; part < 9; part++) {
					if (all < 2) {
						sprintf_s(buf2, "Part_%d", part);
						if (!IniReader::GetInt(buf, buf2, 0, critTableFile.c_str())) continue;
					}

					sprintf_s(buf2, "c_%02d_%d", critter, part);
					for (int crit = 0; crit < 6; crit++) {
						fo::CritInfo& effect = baseCritTable[critter][part][crit];
						for (int i = 0; i < 7; i++) {
							sprintf_s(buf3, "e%d_%s", crit, critNames[i]);
							effect.values[i] = IniReader::GetInt(buf2, buf3, effect.values[i], critTableFile.c_str());
						}
					}
				}
			}
		} else {
			dlogr(".", DL_CRITICALS);
		}
	}
	return 0;
}

enum CritParam {
	DmgMult,
	Flags,
	StatCheck,
	StatMod,
	FlagsFail,
	Message,
	MsgFail
};

#define SetEntry(critter, bodypart, effect, param, value) fo::var::crit_succ_eff[critter][bodypart][effect].values[param] = value

static void CriticalTableOverride() {
	dlogr("Initializing critical table override...", DL_INIT);
	playerCrit = &critTable[38];
	SafeWrite32(0x423F96, (DWORD)playerCrit);
	SafeWrite32(0x423FB3, (DWORD)critTable);

	if (mode == 2 || mode == 3) { // bug fixes
		using namespace fo;
		// Men
		SetEntry(0, ArmRight, 2, Message,   5010);
		SetEntry(0, ArmRight, 2, MsgFail,   5014);

		// Women
		SetEntry(1, ArmRight, 2, Message,   5110);
		SetEntry(1, ArmRight, 2, MsgFail,   5114);

		SetEntry(1, ArmRight, 3, Message,   5110);
		SetEntry(1, ArmRight, 3, MsgFail,   5114);

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
		SetEntry(3, ArmRight, 1, MsgFail,   5306);

		// Ghouls
		SetEntry(4, Head,     4, StatCheck, -1);

		// Brahmin
		SetEntry(5, Head,     4, StatCheck, -1);

		// Radscorpions
		SetEntry(6, LegRight, 1, FlagsFail, fo::DAM_KNOCKED_DOWN);

		SetEntry(6, LegLeft,  1, FlagsFail, fo::DAM_KNOCKED_DOWN);
		SetEntry(6, LegLeft,  2, MsgFail,   5608);

		// Centaurs
		SetEntry(9, Torso,    3, FlagsFail, fo::DAM_KNOCKED_DOWN);

		// Deathclaws
		SetEntry(13, LegLeft, 1, FlagsFail, fo::DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 2, FlagsFail, fo::DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 3, FlagsFail, fo::DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 4, FlagsFail, fo::DAM_CRIP_LEG_LEFT);
		SetEntry(13, LegLeft, 5, FlagsFail, fo::DAM_CRIP_LEG_LEFT);

		// Geckos
		SetEntry(15, ArmRight, 2, MsgFail,  5014);

		// Aliens
		SetEntry(16, ArmRight, 2, MsgFail,  5014);

		// Giant Ants
		SetEntry(17, ArmRight, 2, MsgFail,  5014);

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
		SetEntry(0, Uncalled, 2, Flags,     fo::DAM_KNOCKED_DOWN | fo::DAM_BYPASS); // 0
		SetEntry(0, Uncalled, 2, Message,   5019); // 5018

		// Children
		SetEntry(2, Uncalled, 1, DmgMult,   4);    // 3
		SetEntry(2, Uncalled, 2, Flags,     fo::DAM_KNOCKED_DOWN | fo::DAM_BYPASS); // fo::DAM_BYPASS
		SetEntry(2, Uncalled, 2, Message,   5212); // 5211

		// Centaurs
		SetEntry(9, Uncalled, 3, FlagsFail, fo::DAM_KNOCKED_DOWN); // 0

		// Geckos
		SetEntry(15, Uncalled, 0, Message,  6701); // 6700
		SetEntry(15, Uncalled, 1, Message,  6701); // 6700
		SetEntry(15, Uncalled, 2, Flags,    fo::DAM_KNOCKED_DOWN | fo::DAM_BYPASS); // 0
		SetEntry(15, Uncalled, 2, Message,  6704); // 6700
		SetEntry(15, Uncalled, 3, Message,  6704); // 6700
		SetEntry(15, Uncalled, 4, Message,  6704); // 6700
		SetEntry(15, Uncalled, 5, Message,  6704); // 6700

		// Aliens
		SetEntry(16, Uncalled, 2, Flags,    fo::DAM_KNOCKED_DOWN | fo::DAM_BYPASS); // 0

		// Giant Ants
		SetEntry(17, Uncalled, 2, Flags,    fo::DAM_KNOCKED_DOWN | fo::DAM_BYPASS); // 0

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
	if (IniReader::GetConfigInt("Misc", "RemoveCriticalTimelimits", 0)) {
		dlogr("Removing critical time limits.", DL_INIT);
		SafeWrite8(0x424118, CodeType::JumpShort); // jump to 0x424131
		SafeWriteBatch<WORD>(0x9090, {0x4A3052, 0x4A3093});
	}
}

void Criticals::init() {
	mode = IniReader::GetConfigInt("Misc", "OverrideCriticalTable", 2);
	if (mode < 0 || mode > 3) mode = 0;
	if (mode) {
		critTableFile += IniReader::GetConfigString("Misc", "OverrideCriticalFile", "CriticalOverrides.ini");
		CriticalTableOverride();
		LoadGameHook::OnBeforeGameStart() += []() {
			memcpy(critTable, baseCritTable, sizeof(critTable)); // Apply loaded critical table
		};
	}

	RemoveCriticalTimeLimitsPatch();
}

}
