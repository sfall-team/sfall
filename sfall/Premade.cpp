/*
 *    sfall
 *    Copyright (C) 2009, 2010  The sfall team
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
#include "FalloutEngine.h"

PremadeChar* premade;

void Premade_Init() {
	std::vector<std::string> premadePaths = GetConfigList("misc", "PremadePaths", "", 512);
	std::vector<std::string> premadeFids = GetConfigList("misc", "PremadeFIDs", "", 512);
	if (!premadePaths.empty() && !premadeFids.empty()) {
		dlog("Applying premade characters patch.", DL_INIT);
		int count = min(premadePaths.size(), premadeFids.size());
		premade = new PremadeChar[count];
		for (int i = 0; i < count; i++) {
			std::string path = "premade\\" + premadePaths[i];
			if (path.size() > 19) {
				dlog_f(" Failed: %s exceeds 11 characters\n", DL_INIT, premadePaths[i].c_str());
				return;
			}
			strcpy(premade[i].path, path.c_str());
			premade[i].fid = atoi(premadeFids[i].c_str());
		}

		SafeWrite32(0x51C8D4, count);
		SafeWrite32(0x4A7D76, (DWORD)premade);
		SafeWrite32(0x4A8B1E, (DWORD)premade);
		SafeWrite32(0x4A7E2C, (DWORD)&premade[0].fid);
		strcpy((char*)0x50AF68, premade[0].path);
		dlogr(" Done", DL_INIT);
	}
}
