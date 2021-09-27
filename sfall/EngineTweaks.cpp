/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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

#include "ReplacementFuncs.h"

void EngineTweaks_Init() {
	std::string tweaksFile = GetConfigString("Misc", "TweaksFile", "", MAX_PATH);
	if (!tweaksFile.empty()) {
		const char* cTweaksFile = tweaksFile.insert(0, ".\\").c_str();

		sfgame_SetHealingPID(0, IniGetInt("Items", "STIMPAK", PID_STIMPAK, cTweaksFile));
		sfgame_SetHealingPID(1, IniGetInt("Items", "SUPER_STIMPAK", PID_SUPER_STIMPAK, cTweaksFile));
		sfgame_SetHealingPID(2, IniGetInt("Items", "HEALING_POWDER", PID_HEALING_POWDER, cTweaksFile));
	}
}

//void EngineTweaks_Exit() {
//}
