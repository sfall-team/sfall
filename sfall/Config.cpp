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

#include "Config.h"

#include "main.h"
#include "FalloutEngine\Fallout2.h"
#include "Utils.h"

using namespace fo;

namespace sfall
{

const Config::Data& Config::data()
{
	return _data;
}

bool Config::read(const char* filePath, bool isDb)
{
	if (filePath == nullptr) {
		return false;
	}

	char string[2048];

	if (isDb) {
		DbFile* stream = func::db_fopen(filePath, "rb");

		if (stream == nullptr) return false;

		while (func::db_fgets(string, sizeof(string), stream) != nullptr) {
			parseLine(string);
		}
		func::db_fclose(stream);
	} else {
		FILE* stream = fopen(filePath, "rt");

		// CE: Return false if file does not exist on the file system.
		if (stream == nullptr) {
			return false;
		}

		while (fgets(string, sizeof(string), stream) != nullptr) {
			parseLine(string);
		}
		fclose(stream);
	}
	return true;
}

// Splits "key=value" pair from [string] and copy appropriate parts into [key]
// and [value] respectively.
//
// Both key and value are trimmed.
bool Config::parseKeyValue(char* string, std::string& key, std::string& value)
{
	if (string == nullptr) {
		return false;
	}

	// Find equals character.
	char* pch = strchr(string, '=');
	if (pch == nullptr) {
		return false;
	}

	*pch = '\0';
	trim(string);
	key = string;
	trim(pch + 1);
	value = pch + 1;
	return true;
}

// TODO:
// bool Config::write(const char* filePath, bool isDb) {}

// Based on original code with tweaks from Fallout CE.
bool Config::parseLine(char* string)
{
	char* pch;

	// Find comment marker and truncate the string.
	pch = strchr(string, ';');
	if (pch != nullptr) {
		*pch = '\0';
	}

	// Skip leading whitespace.
	while (isspace(static_cast<unsigned char>(*string))) {
		string++;
	}

	// Check if it's a section key.
	if (*string == '[') {
		char* sectionKey = string + 1;

		// Find closing bracket.
		pch = strchr(sectionKey, ']');
		if (pch != nullptr) {
			*pch = '\0';
			trim(sectionKey);
			_lastSection = sectionKey;
			return true;
		}
	}

	std::string key, value;
	if (!parseKeyValue(string, key, value)) {
		return false;
	}

	ensureSection(_lastSection.c_str())->second.emplace(std::move(key), std::move(value));
	return true;
}

Config::Data::iterator Config::ensureSection(const char* sectionKey)
{
	auto sectionIt = _data.find(sectionKey);
	if (sectionIt == _data.end()) {
		return _data.emplace(sectionKey, Section()).first;
	}
	return sectionIt;
}

bool Config::getString(const char* sectionKey, const char* key, const std::string*& outValue)
{
	auto sectionIt = _data.find(sectionKey);
	if (sectionIt == _data.end()) return false;

	const auto& section = sectionIt->second;
	auto valueIt = section.find(key);
	if (valueIt == section.end()) return false;

	outValue = &valueIt->second;
	return true;
}

bool Config::getInt(const char* sectionKey, const char* key, int& outValue, unsigned char base /* = 0 */)
{
	const std::string* value;
	if (!getString(sectionKey, key, value)) return false;

	outValue = StrToLong(value->c_str(), base);
	return true;
}

bool Config::getDouble(const char* sectionKey, const char* key, double& outValue)
{
	const std::string* value;
	if (!getString(sectionKey, key, value)) return false;

	outValue = strtod(value->c_str(), nullptr);
	return true;
}

}
