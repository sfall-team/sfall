/*
 *    sfall
 *    Copyright (C) 2008-2026  The sfall team
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
#include "..\Utils.h"

#include "..\Game\items.h"

#include "EngineTweaks.h"

namespace sfall
{

static long mirrorShadesPid = fo::PID_MIRROR_SHADES;
static long mirrorShadesStat = fo::STAT_ch;

static long __fastcall MirrorShadesOverride(fo::GameObject* critter, long stat) {
	if (stat == mirrorShadesStat) {
		fo::GameObject* item = fo::func::inven_right_hand(critter);
		if (item && item->protoId == mirrorShadesPid) return 1;
		item = fo::func::inven_left_hand(critter);
		if (item && item->protoId == mirrorShadesPid) return 1;
	}
	return 0;
}

static __declspec(naked) void stat_level_hack_shades() {
	__asm {
		push ecx;
		mov  ecx, ebx; // critter (always obj_dude)
		mov  edx, esi; // stat
		call MirrorShadesOverride;
		pop  ecx;
		test eax, eax;
		jz   end;
		inc  ecx; // stat value +1
end: // overwritten engine code
		lea  edi, [ecx + 1];
		test esi, esi;
		retn;
	}
}

void EngineTweaks::init() {
	auto tweaksFile = IniReader::GetConfigString("Misc", "TweaksFile", "");
	if (!tweaksFile.empty()) {
		const char* cTweaksFile = tweaksFile.insert(0, ".\\").c_str();
		if (GetFileAttributesA(cTweaksFile) == INVALID_FILE_ATTRIBUTES) return;

		game::Items::SetHealingPID(0, IniReader::GetInt("Items", "STIMPAK", fo::PID_STIMPAK, cTweaksFile));
		game::Items::SetHealingPID(1, IniReader::GetInt("Items", "SUPER_STIMPAK", fo::PID_SUPER_STIMPAK, cTweaksFile));
		game::Items::SetHealingPID(2, IniReader::GetInt("Items", "HEALING_POWDER", fo::PID_HEALING_POWDER, cTweaksFile));

		mirrorShadesPid = IniReader::GetInt("Items", "MIRROR_SHADES_PID", fo::PID_MIRROR_SHADES, cTweaksFile);
		mirrorShadesStat = clamp<long>(IniReader::GetInt("Items", "MIRROR_SHADES_STAT", fo::STAT_ch, cTweaksFile), -1, fo::STAT_lu);
		if (mirrorShadesPid != fo::PID_MIRROR_SHADES || mirrorShadesStat != fo::STAT_ch) {
			MakeCall(0x4AF10A, stat_level_hack_shades);
			// Disable original handling
			SafeWrite8(0x4AF1C3, CodeType::Jump);
			SafeWrite32(0x4AF1C4, 487); // jmp 0x4AF3AF
		}
	}
}

//void EngineTweaks::exit() {
//}

}
