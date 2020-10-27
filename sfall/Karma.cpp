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

#include "main.h"
#include "FalloutEngine.h"

struct KarmaFrmSetting {
	DWORD frm;
	int points;
};

static std::vector<KarmaFrmSetting> karmaFrms;

static char karmaGainMsg[128];
static char karmaLossMsg[128];

static DWORD __stdcall DrawCard() {
	int reputation = *ptr_game_global_vars[GVAR_PLAYER_REPUTATION];
	for (std::vector<KarmaFrmSetting>::const_iterator it = karmaFrms.begin(); it != karmaFrms.end(); ++it) {
		if (reputation < it->points) {
			return it->frm;
		}
	}
	return karmaFrms.end()->frm;
}

static void __declspec(naked) DrawInfoWin_hook() {
	__asm {
		cmp  ds:[_info_line], 10;
		jne  skip;
		cmp  eax, 0x30;
		jne  skip;
		push ecx;
		push edx;
		call DrawCard;
		pop  edx;
		pop  ecx;
skip:
		jmp  DrawCard_;
	}
}

static void __stdcall SetKarma(int value) {
	char buf[128];
	if (value > 0) {
		sprintf_s(buf, karmaGainMsg, value);
	} else {
		sprintf_s(buf, karmaLossMsg, -value);
	}
	DisplayPrint(buf);
}

static void __declspec(naked) SetGlobalVarWrapper() {
	__asm {
		test eax, eax; // Gvar number
		jnz  end;
		pushadc;
		call game_get_global_var_;
		sub  edx, eax; // value -= old
		jz   skip;
		push edx;
		call SetKarma;
skip:
		popadc;
end:
		jmp  game_set_global_var_;
	}
}

static void ApplyDisplayKarmaChangesPatch() {
	if (GetConfigInt("Misc", "DisplayKarmaChanges", 0)) {
		dlog("Applying display karma changes patch.", DL_INIT);
		Translate("sfall", "KarmaGain", "You gained %d karma.", karmaGainMsg);
		Translate("sfall", "KarmaLoss", "You lost %d karma.", karmaLossMsg);
		HookCall(0x455A6D, SetGlobalVarWrapper);
		dlogr(" Done", DL_INIT);
	}
}

static void ApplyKarmaFRMsPatch() {
	std::vector<std::string> karmaFrmList = GetConfigList("Misc", "KarmaFRMs", "", 512);
	size_t countFrm = karmaFrmList.size();
	if (countFrm) {
		dlog("Applying karma FRM patch.", DL_INIT);
		std::vector<std::string> karmaPointsList = GetConfigList("Misc", "KarmaPoints", "", 512);

		karmaFrms.resize(countFrm);
		size_t countPoints = karmaPointsList.size();
		for (size_t i = 0; i < countFrm; i++) {
			karmaFrms[i].frm = atoi(karmaFrmList[i].c_str());
			karmaFrms[i].points = (countPoints > i)
			                    ? atoi(karmaPointsList[i].c_str())
			                    : INT_MAX;
		}
		HookCall(0x4367A9, DrawInfoWin_hook);

		dlogr(" Done", DL_INIT);
	}
}

void Karma_Init() {
	ApplyDisplayKarmaChangesPatch();
	ApplyKarmaFRMsPatch();
}
