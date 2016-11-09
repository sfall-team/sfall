/*
 *    sfall
 *    Copyright (C) 2010  The sfall team
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

struct CityRep {
	DWORD globalID;
	DWORD cityID;
};

static CityRep* repList;

void ReputationsInit() {
	int count;
	if (count = GetPrivateProfileIntA("Misc", "CityRepsCount", 0, ini)) {
		repList = new CityRep[count];
		char buf[512];
		GetPrivateProfileStringA("Misc", "CityRepsList", "", buf, 512, ini);
		char* end;
		char* start = buf;
		for (int i = 0; i < count; i++) {
			end = strchr(start, ':');
			*end = '\0';
			repList[i].cityID = atoi(start);
			start = end + 1;
			if (i == count - 1) {
				repList[i].globalID = atoi(start);
			} else {
				end = strchr(start, ',');
				*end = '\0';
				repList[i].globalID = atoi(start);
				start = end + 1;
			}
		}

		SafeWrite32(0x43BEA5, (DWORD)&repList[0].cityID);
		SafeWrite32(0x43BF3C, (DWORD)&repList[0].cityID);
		SafeWrite32(0x43BF4C, (DWORD)&repList[0].globalID);
		SafeWrite32(0x43C03E, count * 8);
	}
}