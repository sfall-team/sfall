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

#include "Utils.h"

#include "IniReader.h"

namespace sfall
{

DWORD IniReader::modifiedIni;

static const char* ddrawIni = ".\\ddraw.ini";
static char ini[65] = ".\\";
static char translationIni[65];

const char* IniReader::GetConfigFile() {
	return ini;
}

void IniReader::SetDefaultConfigFile() {
	std::strcpy(&ini[2], &ddrawIni[2]);
}

void IniReader::SetConfigFile(const char* iniFile) {
	strcat_s(ini, iniFile);
}

int IniReader::GetIntDefaultConfig(const char* section, const char* setting, int defaultValue) {
	return iniGetInt(section, setting, defaultValue, ddrawIni);
}

std::vector<std::string> IniReader::GetListDefaultConfig(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter) {
	return GetIniList(section, setting, defaultValue, bufSize, delimiter, ddrawIni);
}

int IniReader::iniGetInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	return GetPrivateProfileIntA(section, setting, defaultValue, iniFile);
}

size_t IniReader::iniGetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	return GetPrivateProfileStringA(section, setting, defaultValue, buf, bufSize, iniFile);
}

std::string IniReader::GetIniString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile) {
	char* buf = new char[bufSize];
	iniGetString(section, setting, defaultValue, buf, bufSize, iniFile);
	std::string str(buf);
	delete[] buf;
	return str;
}

std::vector<std::string> IniReader::GetIniList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile) {
	auto list = split(GetIniString(section, setting, defaultValue, bufSize, iniFile), delimiter);
	std::transform(list.cbegin(), list.cend(), list.begin(), trim);
	return list;
}

/*
	For ddraw.ini config
*/
int IniReader::GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return iniGetInt(section, setting, defaultValue, ini);
}

std::string IniReader::GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return trim(GetIniString(section, setting, defaultValue, bufSize, ini));
}

size_t IniReader::GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize) {
	return iniGetString(section, setting, defaultValue, buf, bufSize, ini);
}

std::vector<std::string> IniReader::GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniList(section, setting, defaultValue, bufSize, ',', ini);
}

size_t IniReader::Translate(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize) {
	return iniGetString(section, setting, defaultValue, buffer, bufSize, translationIni);
}

std::string IniReader::Translate(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return GetIniString(section, setting, defaultValue, bufSize, translationIni);
}

std::vector<std::string> IniReader::TranslateList(const char* section, const char* setting, const char* defaultValue, char delimiter, size_t bufSize) {
	return GetIniList(section, setting, defaultValue, bufSize, delimiter, translationIni);
}

int IniReader::SetConfigInt(const char* section, const char* setting, int value) {
	char buf[33];
	_itoa_s(value, buf, 33, 10);
	int result = WritePrivateProfileStringA(section, setting, buf, ini);
	return result;
}

void IniReader::init() {
	modifiedIni = IniReader::GetConfigInt("Main", "ModifiedIni", 0);
	IniReader::GetConfigString("Main", "TranslationsINI", ".\\Translations.ini", translationIni, 65);
}

}
