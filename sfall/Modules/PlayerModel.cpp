/*
*    sfall
*    Copyright (C) 2008-2017  The sfall team
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

#include "..\Logging.h"
#include "..\SafeWrite.h"

#include "PlayerModel.h"

static char StartMaleModelName[65];
char DefaultMaleModelName[65];
static char StartFemaleModelName[65];
char DefaultFemaleModelName[65];

void PlayerModel::init() {
	StartMaleModelName[64] = 0;
	if (GetPrivateProfileString("Misc", "MaleStartModel", "", StartMaleModelName, 64, ini)) {
		dlog("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x00418B88, (DWORD)&StartMaleModelName);
		dlogr(" Done", DL_INIT);
	}

	StartFemaleModelName[64] = 0;
	if (GetPrivateProfileString("Misc", "FemaleStartModel", "", StartFemaleModelName, 64, ini)) {
		dlog("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x00418BAB, (DWORD)&StartFemaleModelName);
		dlogr(" Done", DL_INIT);
	}

	DefaultMaleModelName[64] = 0;
	GetPrivateProfileString("Misc", "MaleDefaultModel", "hmjmps", DefaultMaleModelName, 64, ini);
	dlog("Applying male model patch.", DL_INIT);
	SafeWrite32(0x00418B50, (DWORD)&DefaultMaleModelName);
	dlogr(" Done", DL_INIT);

	DefaultFemaleModelName[64] = 0;
	GetPrivateProfileString("Misc", "FemaleDefaultModel", "hfjmps", DefaultFemaleModelName, 64, ini);
	dlog("Applying female model patch.", DL_INIT);
	SafeWrite32(0x00418B6D, (DWORD)&DefaultFemaleModelName);
	dlogr(" Done", DL_INIT);
}
