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
#include "Message.h"

#include "Premade.h"

namespace sfall
{

static fo::PremadeChar* premade;

static const char* __fastcall GetLangPremadePath(const char* premadePath) {
	static char premadeLangPath[56]; // premade\<language>\combat.bio
	static bool isDefault = false;
	static long len = 0;

	if (isDefault) return nullptr;
	if (len == 0) {
		len = std::strlen(Message::GameLanguage());
		if (len == 0 || len >= 32) {
			isDefault = true;
			return nullptr;
		}
		isDefault = (_stricmp(Message::GameLanguage(), "english") == 0);
		if (isDefault) return nullptr;

		std::strncpy(premadeLangPath, premadePath, 8);
		std::strcpy(&premadeLangPath[8], Message::GameLanguage());
	}
	std::strcpy(&premadeLangPath[8 + len], &premadePath[7]);

	return premadeLangPath;
}

static const char* __fastcall PremadeGCD(const char* premadePath) {
	const char* path = GetLangPremadePath(premadePath);
	return (path && fo::func::db_access(path)) ? path : premadePath;
}

static fo::DbFile* __fastcall PremadeBIO(const char* premadePath, const char* mode) {
	premadePath = GetLangPremadePath(premadePath);
	return (premadePath) ? fo::func::db_fopen(premadePath, mode) : nullptr;
}

static __declspec(naked) void select_display_bio_hook() {
	__asm {
		push eax;
		push edx;
		mov  ecx, eax; // premade path
		call PremadeBIO;
		test eax, eax;
		jz   default;
		add  esp, 8;
		retn;
default:
		pop  edx;
		pop  eax;
		jmp  fo::funcoffs::db_fopen_;
	}
}

static __declspec(naked) void select_update_display_hook() {
	__asm {
		mov  ecx, eax; // premade path
		call PremadeGCD;
		jmp  fo::funcoffs::proto_dude_init_;
	}
}

static __declspec(naked) void select_display_stats_hook() {
	__asm {
		call fo::funcoffs::trait_name_;
		test eax, eax;
		jz   skip;
		retn;
skip:
		pop  eax;
		add  eax, 94; // offset to next section (0x4A8A60, 0x4A8AC9)
		jmp  eax;
	}
}

void Premade::init() {
	auto premadePaths = IniReader::GetConfigList("misc", "PremadePaths", "");
	auto premadeFids = IniReader::GetConfigList("misc", "PremadeFIDs", "");
	if (!premadePaths.empty() && !premadeFids.empty()) {
		dlogr("Applying premade characters patch.", DL_INIT);
		int count = min(premadePaths.size(), premadeFids.size());
		premade = new fo::PremadeChar[count];
		for (int i = 0; i < count; i++) {
			auto path = "premade\\" + premadePaths[i];
			if (path.size() > 19) {
				dlog_f(" Failed: %s exceeds 11 characters\n", DL_INIT, premadePaths[i].c_str());
				return;
			}
			std::strcpy(premade[i].path, path.c_str());
			premade[i].fid = atoi(premadeFids[i].c_str());
		}

		SafeWrite32(0x51C8D4, count);                  // _premade_total
		SafeWrite32(0x4A7D76, (DWORD)premade);         // select_update_display_
		SafeWrite32(0x4A8B1E, (DWORD)premade);         // select_display_bio_
		SafeWrite32(0x4A7E2C, (DWORD)&premade[0].fid); // select_display_portrait_
		std::strcpy((char*)0x50AF68, premade[0].path); // for selfrun
	}

	// Add language path for premade GCD/BIO files
	HookCall(0x4A8B44, select_display_bio_hook);
	HookCall(0x4A7D91, select_update_display_hook);

	// Allow premade characters to have less than two traits
	HookCalls(select_display_stats_hook, {0x4A89FD, 0x4A8A66});
}

}
