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

#include "IniReader.h"

#include "Translate.h"

namespace sfall
{

static struct Translation {
	char def[65];
	char lang[170];
	bool state;

	Translation() : state(false) {}

	const char* File() {
		return (state) ? lang : def;
	}
} translationIni;

size_t __stdcall Translate_Get(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize) {
	return IniGetString(section, setting, defaultValue, buffer, bufSize, translationIni.File());
}

std::string __stdcall Translate_Get(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return std::move(IniGetString(section, setting, defaultValue, bufSize, translationIni.File()));
}

std::vector<std::string> __stdcall Translate_GetList(const char* section, const char* setting, const char* defaultValue, char delimiter, size_t bufSize) {
	return std::move(IniGetList(section, setting, defaultValue, bufSize, delimiter, translationIni.File()));
}

////////////////////////////////////////////////////////////////////////////////

static void MakeLangTranslationPath(const char* config) {
	char patches[65], language[32];
	char fileConfig[65] = ".\\";
	std::strcpy(&fileConfig[2], config);

	IniGetString("system", "language", "english", language, 32, fileConfig);
	IniGetString("system", "master_patches", "data", patches, 65, fileConfig);

	const char* iniDef = translationIni.def;
	while (*iniDef == '\\' || *iniDef == '/' || *iniDef == '.') iniDef++; // skip first characters
	sprintf(translationIni.lang, "%s\\text\\%s\\%s", patches, language, iniDef);

	translationIni.state = (GetFileAttributes(translationIni.lang) != INVALID_FILE_ATTRIBUTES);
}

static char saveSfallDataFailMsg[128];
static char combatSaveFailureMsg[128];
static char combatBlockedMessage[128];

const char* Translate_SfallSaveDataFailure()   { return &saveSfallDataFailMsg[0]; }
const char* Translate_CombatSaveBlockMessage() { return &combatSaveFailureMsg[0]; }
const char* Translate_CombatBlockMessage()     { return &combatBlockedMessage[0]; }

static void InitMessagesTranslate() {
	Translate_Get("sfall", "BlockedCombat", "You cannot enter combat at this time.", combatBlockedMessage);
	Translate_Get("sfall", "SaveInCombat", "Cannot save at this time.", combatSaveFailureMsg);
	Translate_Get("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! "
	              "Check if other programs interfere with savegame files/folders and try again!", saveSfallDataFailMsg);
}

void Translate_Init(const char* config) {
	GetConfigString("Main", "TranslationsINI", ".\\Translations.ini", translationIni.def, 65);

	MakeLangTranslationPath(config);
	InitMessagesTranslate();
}

}
