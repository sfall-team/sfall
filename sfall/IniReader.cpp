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

#include "main.h"
#include "FalloutEngine\Fallout2.h"
#include "Modules\LoadGameHook.h"
#include "Config.h"
#include "Utils.h"

namespace sfall
{

static const char* ddrawIni = ".\\ddraw.ini";

IniReader& IniReader::instance() {
	static IniReader instance;
	return instance;
}

IniReader::IniReader() {
}

Config* IniReader::getIniConfig(const char* iniFile) {
	std::string pathStr(iniFile);
	auto cacheHit = _iniCache.find(pathStr);
	if (cacheHit != _iniCache.end()) {
		return cacheHit->second.get();
	}
	auto config = std::make_unique<Config>();
	if (!config->read(iniFile, false)) {
		_iniCache.emplace(std::move(pathStr), nullptr);
		return nullptr;
	}
	return _iniCache.emplace(std::move(pathStr), std::move(config)).first->second.get();
}

const char* IniReader::getConfigFile() {
	return _ini;
}

void IniReader::setDefaultConfigFile() {
	std::strcpy(&_ini[2], &ddrawIni[2]);
}

void IniReader::setConfigFile(const char* iniFile) {
	strcat_s(_ini, iniFile);
}

static int getInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	auto config = IniReader::instance().getIniConfig(iniFile);
	int value;
	if (config == nullptr || !config->getInt(section, setting, value)) {
		value = defaultValue;
	}
	return value;
}

static size_t getString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	auto config = IniReader::instance().getIniConfig(iniFile);
	const std::string* value;
	const char* result = config != nullptr && config->getString(section, setting, value)
	                   ? value->c_str()
	                   : defaultValue;

	strncpy_s(buf, bufSize, result, bufSize - 1);
	return strlen(buf);
}

static std::string getString(const char* section, const char* setting, const char* defaultValue, const char* iniFile) {
	auto config = IniReader::instance().getIniConfig(iniFile);
	const std::string* value;
	if (config == nullptr || !config->getString(section, setting, value)) {
		return std::string(defaultValue);
	}
	return *value;
}

int IniReader::setString(const char* section, const char* setting, const char* value, const char* iniFile) {
	_iniCache.erase(iniFile); // remove file from cache so it returns updated value on the next read
	return WritePrivateProfileStringA(section, setting, value, iniFile);
}

void IniReader::clearCache() {
	_iniCache.clear();
}

void IniReader::init() {
	_modifiedIni = IniReader::GetConfigInt("Main", "ModifiedIni", 0);

	LoadGameHook::OnGameReset() += [this] { clearCache(); };
}


static int setInt(const char* section, const char* setting, int value, const char* iniFile) {
	char buf[33];
	_itoa_s(value, buf, 33, 10);
	return IniReader::instance().setString(section, setting, buf, iniFile);
}

static std::vector<std::string> getList(const char* section, const char* setting, const char* defaultValue, char delimiter, const char* iniFile) {
	auto list = split(getString(section, setting, defaultValue, iniFile), delimiter);
	std::transform(list.cbegin(), list.cend(), list.begin(), (std::string (*)(const std::string&))trim);
	return list;
}

int IniReader::GetIntDefaultConfig(const char* section, const char* setting, int defaultValue) {
	return getInt(section, setting, defaultValue, ddrawIni);
}

std::string IniReader::GetStringDefaultConfig(const char* section, const char* setting, const char* defaultValue) {
	return getString(section, setting, defaultValue, ddrawIni);
}

std::vector<std::string> IniReader::GetListDefaultConfig(const char* section, const char* setting, const char* defaultValue, char delimiter) {
	return getList(section, setting, defaultValue, delimiter, ddrawIni);
}

int IniReader::GetConfigInt(const char* section, const char* setting, int defaultValue) {
	return getInt(section, setting, defaultValue, instance()._ini);
}

std::string IniReader::GetConfigString(const char* section, const char* setting, const char* defaultValue) {
	return trim(getString(section, setting, defaultValue, instance()._ini));
}

size_t IniReader::GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize) {
	return getString(section, setting, defaultValue, buf, bufSize, instance()._ini);
}

std::vector<std::string> IniReader::GetConfigList(const char* section, const char* setting, const char* defaultValue) {
	return getList(section, setting, defaultValue, ',', instance()._ini);
}

int IniReader::GetInt(const char* section, const char* setting, int defaultValue, const char* iniFile) {
	return getInt(section, setting, defaultValue, iniFile);
}

size_t IniReader::GetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile) {
	return getString(section, setting, defaultValue, buf, bufSize, iniFile);
}

std::string IniReader::GetString(const char* section, const char* setting, const char* defaultValue, const char* iniFile) {
	return getString(section, setting, defaultValue, iniFile);
}

std::vector<std::string> IniReader::GetList(const char* section, const char* setting, const char* defaultValue, char delimiter, const char* iniFile) {
	return getList(section, setting, defaultValue, delimiter, iniFile);
}

int IniReader::SetConfigInt(const char* section, const char* setting, int value) {
	return setInt(section, setting, value, instance()._ini);
}

int IniReader::SetConfigString(const char* section, const char* setting, const char* value) {
	return instance().setString(section, setting, value, instance()._ini);
}

int IniReader::SetDefaultConfigInt(const char* section, const char* setting, int value) {
	return setInt(section, setting, value, ddrawIni);
}

int IniReader::SetDefaultConfigString(const char* section, const char* setting, const char* value) {
	return instance().setString(section, setting, value, ddrawIni);
}

}
