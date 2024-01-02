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

#include "..\main.h"

#include "..\Logging.h"
#include "..\SafeWrite.h"

#include "PlayerModel.h"

namespace sfall
{

static char startMaleModelName[33]   = {};
char defaultMaleModelName[65]        = {};
static char startFemaleModelName[33] = {};
char defaultFemaleModelName[65]      = {};

void PlayerModel::init() {
	if (IniReader::GetConfigString("Misc", "MaleStartModel", "", startMaleModelName, 33)) {
		dlogr("Applying male start model patch.", DL_INIT);
		SafeWrite32(0x418B88, (DWORD)&startMaleModelName);
	}

	if (IniReader::GetConfigString("Misc", "FemaleStartModel", "", startFemaleModelName, 33)) {
		dlogr("Applying female start model patch.", DL_INIT);
		SafeWrite32(0x418BAB, (DWORD)&startFemaleModelName);
	}

	IniReader::GetConfigString("Misc", "MaleDefaultModel", "hmjmps", defaultMaleModelName, 65);
	dlogr("Applying male model patch.", DL_INIT);
	SafeWrite32(0x418B50, (DWORD)&defaultMaleModelName);

	IniReader::GetConfigString("Misc", "FemaleDefaultModel", "hfjmps", defaultFemaleModelName, 65);
	dlogr("Applying female model patch.", DL_INIT);
	SafeWrite32(0x418B6D, (DWORD)&defaultFemaleModelName);
}

}
