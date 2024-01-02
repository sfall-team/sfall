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

enum OffsetStat : long {
	base  = 9, // offset from base_stat_srength
	bonus = 44 // offset from bonus_stat_srength
};

class CritterStats : public Module {
public:
	const char* name() { return "CritterStats"; }
	void init();

	static long* __fastcall GetProto(fo::GameObject* critter);
	static void RecalcDerivedHook();

	static long GetStat(fo::GameObject* critter, long stat, long offset);
	static void SetStat(fo::GameObject* critter, long stat, long amount, long offset);
	static long SetProtoData(long pid, long offset, long amount);

	static void SaveStatData(HANDLE file);
	static bool LoadStatData(HANDLE file);
};

}
