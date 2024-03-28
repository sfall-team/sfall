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

#include "IniFiles.h"

#include "..\..\..\Config.h"
#include "..\..\..\Utils.h"
#include "..\..\ScriptExtender.h"
#include "..\Arrays.h"

#include <memory>
#include <unordered_map>

namespace sfall
{
namespace script
{

static std::unordered_map<std::string, DWORD> ConfigArrayCache;
static std::unordered_map<std::string, DWORD> ConfigArrayCacheDat;

void ResetIniCache() {
	ConfigArrayCache.clear();
	ConfigArrayCacheDat.clear();
}

static bool IsSpecialIni(const char* str, const char* end) {
	const char* pos = strfind(str, &IniReader::instance().getConfigFile()[2]); // TODO test
	if (pos && pos < end) return true;
	pos = strfind(str, "f2_res.ini");
	if (pos && pos < end) return true;
	return false;
}

static int ParseIniSetting(const char* iniString, const char* &key, char section[], char file[]) {
	key = strstr(iniString, "|");
	if (!key) return -1;

	DWORD filelen = (DWORD)key - (DWORD)iniString;
	if (ScriptExtender::iniConfigFolder.empty() && filelen >= 64) return -1;
	const char* fileEnd = key;

	key = strstr(key + 1, "|");
	if (!key) return -1;

	DWORD seclen = (DWORD)key - ((DWORD)iniString + filelen + 1);
	if (seclen > 32) return -1;

	file[0] = '.';
	file[1] = '\\';

	if (!ScriptExtender::iniConfigFolder.empty() && !IsSpecialIni(iniString, fileEnd)) {
		size_t len = ScriptExtender::iniConfigFolder.length(); // limit up to 62 characters
		memcpy(&file[2], ScriptExtender::iniConfigFolder.c_str(), len);
		memcpy(&file[2 + len], iniString, filelen); // copy path and file
		file[2 + len + filelen] = 0;
		if (GetFileAttributesA(file) & FILE_ATTRIBUTE_DIRECTORY) goto defRoot; // also file not found
	} else {
defRoot:
		memcpy(&file[2], iniString, filelen);
		file[2 + filelen] = 0;
	}
	memcpy(section, &iniString[filelen + 1], seclen);
	section[seclen] = 0;

	key++;
	return 1;
}

static DWORD GetIniSetting(const char* str, bool isString) {
	const char* key;
	char section[33], file[128];

	if (ParseIniSetting(str, key, section, file) < 0) {
		return -1;
	}
	if (isString) {
		ScriptExtender::gTextBuffer[0] = '\0';
		IniReader::GetString(section, key, "", ScriptExtender::gTextBuffer, 1024, file);
		return (DWORD)&ScriptExtender::gTextBuffer[0];
	} else {
		return IniReader::GetInt(section, key, -1, file);
	}
}

void op_get_ini_setting(OpcodeContext& ctx) {
	ctx.setReturn(GetIniSetting(ctx.arg(0).strValue(), false));
}

void op_get_ini_string(OpcodeContext& ctx) {
	DWORD result = GetIniSetting(ctx.arg(0).strValue(), true);
	ctx.setReturn(result, (result != -1) ? DataType::STR : DataType::INT);
}

void op_modified_ini(OpcodeContext& ctx) {
	ctx.setReturn(IniReader::instance().modifiedIni());
}

void mf_set_ini_setting(OpcodeContext& ctx) {
	const ScriptValue &argVal = ctx.arg(1);

	const char* saveValue;
	if (argVal.isInt()) {
		_itoa_s(argVal.rawValue(), ScriptExtender::gTextBuffer, 10);
		saveValue = ScriptExtender::gTextBuffer;
	} else {
		saveValue = argVal.strValue();
	}
	const char* key;
	char section[33], file[128];
	int result = ParseIniSetting(ctx.arg(0).strValue(), key, section, file);
	if (result > 0) {
		result = IniReader::instance().setString(section, key, saveValue, file);
	}

	switch (result) {
	case 0:
		ctx.printOpcodeError("%s() - value save error.", ctx.getMetaruleName());
		break;
	case -1:
		ctx.printOpcodeError("%s() - invalid setting argument.", ctx.getMetaruleName());
		break;
	default:
		return;
	}
	ctx.setReturn(-1);
}

// Sanitizes path for db_fopen:
// - Disallows going outside of game folder.
// - Normalizes directory separators.
// - Trims whitespaces.
static std::string GetSanitizedDBPath(const char* pathArg) {
	const char* whiteSpaces = " \t\r\n";
	std::string path(pathArg);
	std::replace(path.begin(), path.end(), '/', '\\'); // Normalize directory separators.
	path.erase(0, path.find_first_not_of(whiteSpaces)); // trim left
	if (path[0] == '\\' ||
	    path.find(':') != std::string::npos ||
	    path.find("..") != std::string::npos) return ""; // don't allow absolute paths or going outside of root

	if (path.find(".\\") == 0) path.erase(0, 2); // remove leading ".\"
	path.erase(path.find_last_not_of(whiteSpaces) + 1); // trim right
	return std::move(path);
}

static std::string GetIniFilePathFromArg(const ScriptValue& arg) {
	const char* pathArg = arg.strValue();
	std::string fileName(".\\");
	if (ScriptExtender::iniConfigFolder.empty()) {
		fileName += pathArg;
	} else {
		fileName += ScriptExtender::iniConfigFolder;
		fileName += pathArg;
		if (GetFileAttributesA(fileName.c_str()) & FILE_ATTRIBUTE_DIRECTORY) {
			auto str = pathArg;
			for (size_t i = 2; ; i++, str++) {
				//if (*str == '.') str += (str[1] == '.') ? 3 : 2; // skip '.\' or '..\'
				fileName[i] = *str;
				if (!*str) break;
			}
		}
	}
	return std::move(fileName);
}

void mf_get_ini_sections(OpcodeContext& ctx) {
	Config* config = IniReader::instance().getIniConfig(GetIniFilePathFromArg(ctx.arg(0)).c_str());
	if (config == nullptr) {
		ctx.setReturn(CreateTempArray(0, 0));
		return;
	}
	const auto& data = config->data();
	size_t numSections = config->data().size();
	DWORD arrayId = CreateTempArray(numSections, 0);
	size_t i = 0;
	for (auto sectIt = data.cbegin(); sectIt != data.cend(); ++sectIt) {
		arrays[arrayId].val[i].set(sectIt->first.c_str(), sectIt->first.size());
		++i;
	}
	ctx.setReturn(arrayId);
}

static void CopyConfigSectionToArray(DWORD arrayId, const Config::Section& section) {
	for (auto valueIt = section.cbegin(); valueIt != section.cend(); ++valueIt) {
		SetArray(arrayId, valueIt->first.c_str(), valueIt->second.c_str(), false);
	}
}

void mf_get_ini_section(OpcodeContext& ctx) {
	auto sectionName = ctx.arg(1).strValue();
	DWORD arrayId = CreateTempArray(-1, 0); // associative
	ctx.setReturn(arrayId);

	Config* config = IniReader::instance().getIniConfig(GetIniFilePathFromArg(ctx.arg(0)).c_str());
	if (config == nullptr) return; // ini file not found

	const auto& data = config->data();
	auto sectIt = data.find(sectionName);
	if (sectIt == data.end()) return; // ini section not found

	CopyConfigSectionToArray(arrayId, sectIt->second);
}

void mf_get_ini_config(OpcodeContext& ctx) {
	bool isDb = ctx.arg(1).asBool();
	std::string filePath(isDb
		? GetSanitizedDBPath(ctx.arg(0).strValue())
		: GetIniFilePathFromArg(ctx.arg(0)));

	if (filePath.size() == 0) {
		ctx.printOpcodeError("%s() - invalid config file path: %s", ctx.getMetaruleName(), ctx.arg(0).strValue());
		ctx.setReturn(0);
		return;
	}

	// Check if array exists in either cache.
	auto& cache = isDb ? ConfigArrayCacheDat : ConfigArrayCache;
	auto cacheHit = cache.find(filePath);
	if (cacheHit != cache.end()) {
		if (ArrayExists(cacheHit->second)) {
			// Previously loaded array still exists, so return it.
			ctx.setReturn(cacheHit->second);
			return;
		}
		// Array was deleted. Remove it from cache and proceed with loading.
		cache.erase(cacheHit);
	}

	// Try to read INI config from DAT database.
	Config* config;
	std::unique_ptr<Config> configUniq;
	if (isDb) {
		configUniq = std::make_unique<Config>();
		if (!configUniq->read(filePath.c_str(), isDb)) {
			ctx.printOpcodeError("%s() - cannot read config file from DAT: %s", ctx.getMetaruleName(), filePath.c_str());
			ctx.setReturn(0);
			return;
		}
		config = configUniq.get();
	} else {
		// Request config from IniReader (to take advantage of it's cache).
		config = IniReader::instance().getIniConfig(filePath.c_str());
		if (config == nullptr) {
			ctx.printOpcodeError("%s() - cannot read config file: %s", ctx.getMetaruleName(), filePath.c_str());
			ctx.setReturn(0);
			return;
		}
	}
	// Copy data to new Sfall Array.
	DWORD arrayId = CreateArray(-1, 0);
	const auto& data = config->data();
	for (auto sectIt = data.cbegin(); sectIt != data.cend(); ++sectIt) {
		DWORD subArrayId = CreateArray(-1, 0);
		CopyConfigSectionToArray(subArrayId, sectIt->second);
		SetArray(arrayId, sectIt->first.c_str(), subArrayId, false);
	}
	// Save new array ID to cache and return it.
	cache.emplace(std::move(filePath), arrayId);
	ctx.setReturn(arrayId);
}

}
}
