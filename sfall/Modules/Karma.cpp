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

#include <math.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Karma.h"

namespace sfall
{

struct KarmaFrmSetting {
	DWORD frm;
	int points;
};

static std::vector<KarmaFrmSetting> karmaFrms;

static std::string karmaGainMsg;
static std::string karmaLossMsg;
bool displayKarmaChanges;

static DWORD _stdcall DrawCardHook2() {
	int reputation = fo::var::game_global_vars[fo::GVAR_PLAYER_REPUTATION];
	for (auto& info : karmaFrms) {
		if (reputation < info.points) {
			return info.frm;
		}
	}
	return karmaFrms.end()->frm;
}

static void __declspec(naked) DrawCardHook() {
	__asm {
		cmp ds : [FO_VAR_info_line], 10;
		jne skip;
		cmp eax, 0x30;
		jne skip;
		push ecx;
		push edx;
		call DrawCardHook2;
		pop edx;
		pop ecx;
skip:
		jmp fo::funcoffs::DrawCard_;
	}
}

void Karma::DisplayKarma(int value) {
	char buf[128];
	if (value > 0) {
		sprintf_s(buf, karmaGainMsg.c_str(), value);
	} else {
		sprintf_s(buf, karmaLossMsg.c_str(), -value);
	}
	fo::func::display_print(buf);
}

void ApplyDisplayKarmaChangesPatch() {
	displayKarmaChanges = GetConfigInt("Misc", "DisplayKarmaChanges", 0) != 0;
	if (displayKarmaChanges) {
		dlog("Applying display karma changes patch.", DL_INIT);
		karmaGainMsg = Translate("sfall", "KarmaGain", "You gained %d karma.");
		karmaLossMsg = Translate("sfall", "KarmaLoss", "You lost %d karma.");
		dlogr(" Done", DL_INIT);
	}
}

void ApplyKarmaFRMsPatch() {
	auto karmaFrmList = GetConfigList("Misc", "KarmaFRMs", "", 512);
	if (karmaFrmList.size() > 0) {
		dlog("Applying karma frm patch.", DL_INIT);

		auto karmaPointsList = GetConfigList("Misc", "KarmaPoints", "", 512);
		karmaFrms.resize(karmaFrmList.size());
		for (size_t i = 0; i < karmaFrmList.size(); i++) {
			karmaFrms[i].frm = atoi(karmaFrmList[i].c_str());
			karmaFrms[i].points = (karmaPointsList.size() > i)
				? atoi(karmaPointsList[i].c_str())
				: INT_MAX;
		}
		HookCall(0x4367A9, DrawCardHook);

		dlogr(" Done", DL_INIT);
	}
}

void Karma::init() {
	ApplyDisplayKarmaChangesPatch();
	ApplyKarmaFRMsPatch();
}

}
