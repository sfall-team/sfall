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

#include "IniReader.h"

#include "Translate.h"

namespace sfall
{

static struct Translation {
	char def[65];
	char lang[170];
	bool state = false;

	const char* File() {
		return (state) ? lang : def;
	}
} translationIni;

size_t Translate::Get(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize) {
	return IniReader::GetString(section, setting, defaultValue, buffer, bufSize, translationIni.File());
}

std::string Translate::Get(const char* section, const char* setting, const char* defaultValue) {
	return std::move(IniReader::GetString(section, setting, defaultValue, translationIni.File()));
}

std::vector<std::string> Translate::GetList(const char* section, const char* setting, const char* defaultValue, char delimiter) {
	return std::move(IniReader::GetList(section, setting, defaultValue, delimiter, translationIni.File()));
}

////////////////////////////////////////////////////////////////////////////////

static void MakeLangTranslationPath(const char* config) {
	char patches[65], language[32];
	char fileConfig[67] = ".\\";
	std::strcpy(&fileConfig[2], config);

	IniReader::GetString("system", "language", "english", language, 32, fileConfig);
	IniReader::GetString("system", "master_patches", "data", patches, 65, fileConfig);

	const char* iniDef = translationIni.def;
	while (*iniDef == '\\' || *iniDef == '/' || *iniDef == '.') iniDef++; // skip first characters
	sprintf(translationIni.lang, "%s\\text\\%s\\%s", patches, language, iniDef);

	translationIni.state = (GetFileAttributesA(translationIni.lang) != INVALID_FILE_ATTRIBUTES);
}

static std::string saveSfallDataFailMsg;
static std::string combatSaveFailureMsg;
static std::string combatBlockedMessage;

std::string& Translate::SfallSaveDataFailure()   { return saveSfallDataFailMsg; }
std::string& Translate::CombatSaveBlockMessage() { return combatSaveFailureMsg; }
std::string& Translate::CombatBlockMessage()     { return combatBlockedMessage; }

static void InitMessagesTranslate() {
	combatBlockedMessage = Translate::Get("sfall", "BlockedCombat", "You cannot enter combat at this time.");
	combatSaveFailureMsg = Translate::Get("sfall", "SaveInCombat", "Cannot save at this time.");
	saveSfallDataFailMsg = Translate::Get("sfall", "SaveSfallDataFail", "ERROR saving extended savegame information! "
	                                      "Check if other programs interfere with savegame files/folders and try again.");
}

void Translate::init(const char* config) {
	IniReader::GetConfigString("Main", "TranslationsINI", ".\\Translations.ini", translationIni.def, 65);

	MakeLangTranslationPath(config);
	InitMessagesTranslate();
}

}
