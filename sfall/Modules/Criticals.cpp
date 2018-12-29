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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Logging.h"
#include "LoadGameHook.h"

#include "Criticals.h"

namespace sfall
{

static const char* critTableFile = ".\\CriticalOverrides.ini";
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

static fo::CritInfo loadCritTable[Criticals::critTableCount][9][6]; // Loaded table from CriticalOverrides.ini (with bug fixes and default engine values)

static fo::CritInfo critTable[Criticals::critTableCount][9][6];
static fo::CritInfo (*playerCrit)[9][6];

static bool Inited = false;

static const char* errorTable = "\nError: %s - to use this function, need to set the OverrideCriticalTable option in the ddraw.ini file.";

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
	critTable[critter][bodypart][slot].values[element] = loadCritTable[critter][bodypart][slot].values[element];
}

static int CritTableLoad() {
	if (mode == 1) {
		dlog("\n  Setting up critical hit table using CriticalOverrides.ini file", DL_CRITICALS);
		char section[16];
		memset(loadCritTable, 0, sizeof(critTable));
		for (DWORD critter = 0; critter < 20; critter++) {
			for (DWORD part = 0; part < 9; part++) {
				for (DWORD crit = 0; crit < 6; crit++) {
					sprintf_s(section, "c_%02d_%d_%d", critter, part, crit);
					DWORD newCritter = (critter == 19) ? 38 : critter;
					fo::CritInfo& newEffect = loadCritTable[newCritter][part][crit];
					fo::CritInfo& defaultEffect = fo::var::crit_succ_eff[critter][part][crit];
					for (int i = 0; i < 7; i++) {
						newEffect.values[i] = GetPrivateProfileIntA(section, critNames[i], defaultEffect.values[i], critTableFile);
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
		if (mode != 4) dlog("\n  Setting up critical hit table using RP fixes", DL_CRITICALS);
		constexpr int size = 6 * 9 * sizeof(fo::CritInfo);
		constexpr int sizeF = 19 * size;
		memcpy(loadCritTable, fo::var::crit_succ_eff, sizeF);
		memset(&loadCritTable[19], 0, sizeF);
		memcpy(&loadCritTable[38], fo::var::pc_crit_succ_eff, size); // PC crit table

		if (mode == 3) {
			dlog(" and CriticalOverrides.ini file", DL_CRITICALS);
			char buf[32], buf2[32], buf3[32];
			for (int critter = 0; critter < Criticals::critTableCount; critter++) {
				sprintf_s(buf, "c_%02d", critter);
				int all;
				if (!(all = GetPrivateProfileIntA(buf, "Enabled", 0, critTableFile))) continue;
				for (int part = 0; part < 9; part++) {
					if (all < 2) {
						sprintf_s(buf2, "Part_%d", part);
						if (!GetPrivateProfileIntA(buf, buf2, 0, critTableFile)) continue;
					}

					sprintf_s(buf2, "c_%02d_%d", critter, part);
					for (int crit = 0; crit < 6; crit++) {
						fo::CritInfo& effect = loadCritTable[critter][part][crit];
						for (int i = 0; i < 7; i++) {
							sprintf_s(buf3, "e%d_%s", crit, critNames[i]);
							if (i == 1) { // EffectFlags
								auto valStr = GetIniList(buf2, buf3, "", 128, ',', critTableFile);
								int size = valStr.size();
								if (size == 0) continue;
								int value = 0;
								for (int j = 0; j < size; j++) {
									try {
										value |= std::stoi(valStr[j], nullptr, 0);
									} catch (...) {
										char msgbuff[128];
										sprintf_s(msgbuff, "Incorrect value of the EffectFlags parameter in the section [%s] of the table.", buf2);
										MessageBoxA(0, msgbuff, "Critical table file", MB_TASKMODAL);
										return -1;
									}
								}
								effect.values[i] = value;
							} else {
								effect.values[i] = GetPrivateProfileIntA(buf2, buf3, effect.values[i], critTableFile);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

#define SetEntry(critter, bodypart, effect, param, value) fo::var::crit_succ_eff[critter][bodypart][effect].values[param] = value;

static void CriticalTableOverride() {
	dlog("Initializing critical table override...", DL_INIT);
	playerCrit = &critTable[38];
	SafeWrite32(0x423F96, (DWORD)playerCrit);
	SafeWrite32(0x423FB3, (DWORD)critTable);

	if (mode == 2 || mode == 3) { // bug fixes
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

		SetEntry(3, 5, 1, 6, 5306);

		SetEntry(4, 0, 4, 2, -1);

		SetEntry(5, 0, 4, 2, -1);

		SetEntry(6, 4, 1, 4, 2);

		SetEntry(6, 5, 1, 4, 2);

		SetEntry(6, 5, 2, 6, 5608);

		SetEntry(9, 3, 3, 4, 2);

		SetEntry(13, 5, 1, 4, 4);
		SetEntry(13, 5, 2, 4, 4);
		SetEntry(13, 5, 3, 4, 4);
		SetEntry(13, 5, 4, 4, 4);
		SetEntry(13, 5, 5, 4, 4);

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
		//SetEntry(18,6,2,6,0);
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
	}

	if (CritTableLoad()) {
		dlogr(". Failure initializing critical hit table from file.", DL_INIT);
	} else {
		dlogr(". Completed applying critical hit table.", DL_INIT);
	}
	Inited = true;
}
#undef SetEntry

static void RemoveCriticalTimeLimitsPatch() {
	if (GetConfigInt("Misc", "RemoveCriticalTimelimits", 0)) {
		dlog("Removing critical time limits.", DL_INIT);
		SafeWrite8(0x424118, 0xEB);  // jump to 0x424131
		SafeWrite16(0x4A3052, 0x9090);
		SafeWrite16(0x4A3093, 0x9090);
		dlogr(" Done", DL_INIT);
	}
}

void Criticals::init() {

	mode = GetConfigInt("Misc", "OverrideCriticalTable", 2);
	if (mode < 0 || mode > 4) mode = 0;
	if (mode) {
		CriticalTableOverride();
		LoadGameHook::OnBeforeGameStart() += []() {
			memcpy(critTable, loadCritTable, sizeof(critTable)); // Apply loaded critical table
		};
	}

	RemoveCriticalTimeLimitsPatch();
}

}
