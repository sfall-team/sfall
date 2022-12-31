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

#include "Utils.h"

#include "IniReader.h"

namespace sfall
{

DWORD IniReader::modifiedIni;

static const char* ddrawIni = ".\\ddraw.ini";
static char ini[65] = ".\\";

static int getInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	return GetPrivateProfileIntA(section, setting, defaultValue, iniFile);
}

static size_t getString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	return GetPrivateProfileStringA(section, setting, defaultValue, buf, bufSize, iniFile);
}

static std::string getString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile) {
	char* buf = new char[bufSize];
	getString(section, setting, defaultValue, buf, bufSize, iniFile);
	std::string str(buf);
	delete[] buf;
	return str;
}

static std::vector<std::string> getList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile) {
	std::vector<std::string> list = split(getString(section, setting, defaultValue, bufSize, iniFile), delimiter);
	std::transform(list.cbegin(), list.cend(), list.begin(), (std::string (*)(const std::string&))trim);
	return list;
}

static int setInt(const char* section, const char* setting, int value, const char* iniFile) {
	char buf[33];
	_itoa_s(value, buf, 33, 10);
	return WritePrivateProfileStringA(section, setting, buf, iniFile);
}

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
	return getInt(section, setting, defaultValue, ddrawIni);
}

std::string IniReader::GetStringDefaultConfig(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return getString(section, setting, defaultValue, bufSize, ddrawIni);
}

std::vector<std::string> IniReader::GetListDefaultConfig(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter) {
	return getList(section, setting, defaultValue, bufSize, delimiter, ddrawIni);
}

int IniReader::GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return getInt(section, setting, defaultValue, ini);
}

std::string IniReader::GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return trim(getString(section, setting, defaultValue, bufSize, ini));
}

size_t IniReader::GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize) {
	return getString(section, setting, defaultValue, buf, bufSize, ini);
}

std::vector<std::string> IniReader::GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize) {
	return getList(section, setting, defaultValue, bufSize, ',', ini);
}

int IniReader::GetInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	return getInt(section, setting, defaultValue, iniFile);
}

size_t IniReader::GetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	return getString(section, setting, defaultValue, buf, bufSize, iniFile);
}

std::string IniReader::GetString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile) {
	return getString(section, setting, defaultValue, bufSize, iniFile);
}

std::vector<std::string> IniReader::GetList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile) {
	return getList(section, setting, defaultValue, bufSize, delimiter, iniFile);
}

int IniReader::SetConfigInt(const char* section, const char* setting, int value) {
	return setInt(section, setting, value, ini);
}

int IniReader::SetDefaultConfigInt(const char* section, const char* setting, int value) {
	return setInt(section, setting, value, ddrawIni);
}

int IniReader::SetDefaultConfigString(const char* section, const char* setting, const char* value) {
	return WritePrivateProfileStringA(section, setting, value, ddrawIni);
}

void IniReader::init() {
	modifiedIni = IniReader::GetConfigInt("Main", "ModifiedIni", 0);
}

}
