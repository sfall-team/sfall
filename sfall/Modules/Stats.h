/*
 *    sfall
 *    Copyright (C) 2008, 2009  The sfall team
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

namespace sfall
{

class Stats {
public:
	static const char* name() { return "Stats"; }
	static void init();

	static void OnGameLoad();

	static float experienceMod;
	static DWORD standardApAcBonus;
	static DWORD extraApAcBonus;

	static long __stdcall GetStatMax(int stat, int isNPC);
	static long __stdcall GetStatMin(int stat, int isNPC);

	static void UpdateHPStat(fo::GameObject* critter);
};

void __stdcall SetPCStatMax(int stat, int value);
void __stdcall SetPCStatMin(int stat, int value);
void __stdcall SetNPCStatMax(int stat, int value);
void __stdcall SetNPCStatMin(int stat, int value);

}
