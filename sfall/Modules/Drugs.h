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

#pragma once

#include "Module.h"

namespace sfall
{

class Drugs : public Module {
public:
	const char* name() { return "Drugs"; }
	void init();
	void exit() override;

	static long addictionGvarCount;
	static bool JetWithdrawal;

	static long GetDrugCount();
	static long GetDrugPid(long n);
	static long GetDrugGvar(long n);

	static long SetDrugNumEffect(long pid, long effect);
	static long SetDrugAddictTimeOff(long pid, long time);
};

#define SIZE_S_DRUGS    (32 + 1)

#pragma pack(push, 1)
struct sDrugs {
	DWORD drugPid;      // don't move
	long gvarID;        // don't move
	long numEffects;    // don't move
	long addictTimeOff;
	long msgID;
	long frmID;
	long iniNumEffects;
	long iniAddictTimeOff;

	char skip;
};
#pragma pack(pop)

extern sDrugs *drugs;

}
