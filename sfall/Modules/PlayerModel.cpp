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

namespace sfall
{

static char startMaleModelName[65]   = {};
char defaultMaleModelName[65]        = {};
static char startFemaleModelName[65] = {};
char defaultFemaleModelName[65]      = {};

void PlayerModel::init() {
	if (GetConfigString("Misc", "MaleStartModel", "", startMaleModelName, 64)) {
		dlog("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x418B88, (DWORD)&startMaleModelName);
		dlogr(" Done", DL_INIT);
	}

	if (GetConfigString("Misc", "FemaleStartModel", "", startFemaleModelName, 64)) {
		dlog("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x418BAB, (DWORD)&startFemaleModelName);
		dlogr(" Done", DL_INIT);
	}

	GetConfigString("Misc", "MaleDefaultModel", "hmjmps", defaultMaleModelName, 64);
	dlog("Applying male model patch.", DL_INIT);
	SafeWrite32(0x418B50, (DWORD)&defaultMaleModelName);
	dlogr(" Done", DL_INIT);

	GetConfigString("Misc", "FemaleDefaultModel", "hfjmps", defaultFemaleModelName, 64);
	dlog("Applying female model patch.", DL_INIT);
	SafeWrite32(0x418B6D, (DWORD)&defaultFemaleModelName);
	dlogr(" Done", DL_INIT);
}

}
