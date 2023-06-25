/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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

#include <string>
#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Translate.h"

#include "HookScripts.h"

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

static DWORD __stdcall DrawCard() {
	int reputation = fo::var::game_global_vars[fo::GVAR_PLAYER_REPUTATION];
	for (const auto& info : karmaFrms) {
		if (reputation < info.points) {
			return info.frm;
		}
	}
	return karmaFrms.end()->frm;
}

static void __declspec(naked) DrawInfoWin_hook() {
	__asm {
		cmp  ds:[FO_VAR_info_line], 10;
		jne  skip;
		cmp  eax, 0x30;
		jne  skip;
		push ecx;
		push edx;
		call DrawCard;
		pop  edx;
		pop  ecx;
skip:
		jmp  fo::funcoffs::DrawCard_;
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

static void ApplyDisplayKarmaChangesPatch() {
	displayKarmaChanges = IniReader::GetConfigInt("Misc", "DisplayKarmaChanges", 0) != 0;
	if (displayKarmaChanges) {
		dlogr("Applying display karma changes patch.", DL_INIT);
		karmaGainMsg = Translate::Get("sfall", "KarmaGain", "You gained %d karma.");
		karmaLossMsg = Translate::Get("sfall", "KarmaLoss", "You lost %d karma.");
		HookScripts::InjectingHook(HOOK_SETGLOBALVAR);
	}
}

static void ApplyKarmaFRMsPatch() {
	auto karmaFrmList = IniReader::GetConfigList("Misc", "KarmaFRMs", "", 512);
	size_t countFrm = karmaFrmList.size();
	if (countFrm) {
		dlogr("Applying karma FRM patch.", DL_INIT);
		auto karmaPointsList = IniReader::GetConfigList("Misc", "KarmaPoints", "", 512);

		karmaFrms.resize(countFrm);
		size_t countPoints = karmaPointsList.size();
		for (size_t i = 0; i < countFrm; i++) {
			karmaFrms[i].frm = atoi(karmaFrmList[i].c_str());
			karmaFrms[i].points = (countPoints > i)
			                    ? atoi(karmaPointsList[i].c_str())
			                    : INT_MAX;
		}
		HookCall(0x4367A9, DrawInfoWin_hook);
	}
}

void Karma::init() {
	ApplyDisplayKarmaChangesPatch();
	ApplyKarmaFRMsPatch();
}

}
