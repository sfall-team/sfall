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
#include "..\FalloutEngine\Fallout2.h"

#include "..\Game\items.h"

#include "EngineTweaks.h"

namespace sfall
{

void EngineTweaks::init() {
	auto tweaksFile = IniReader::GetConfigString("Misc", "TweaksFile", "");
	if (!tweaksFile.empty()) {
		const char* cTweaksFile = tweaksFile.insert(0, ".\\").c_str();
		if (GetFileAttributesA(cTweaksFile) == INVALID_FILE_ATTRIBUTES) return;

		game::Items::SetHealingPID(0, IniReader::GetInt("Items", "STIMPAK", fo::PID_STIMPAK, cTweaksFile));
		game::Items::SetHealingPID(1, IniReader::GetInt("Items", "SUPER_STIMPAK", fo::PID_SUPER_STIMPAK, cTweaksFile));
		game::Items::SetHealingPID(2, IniReader::GetInt("Items", "HEALING_POWDER", fo::PID_HEALING_POWDER, cTweaksFile));
	}
}

//void EngineTweaks::exit() {
//}

}
