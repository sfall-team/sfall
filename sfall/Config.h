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

#pragma once

#include "Utils.h"

#include <map>
#include <string>

namespace sfall
{

/*
	Implements reading from INI-like text config files in normal and DAT-filesystem.
	Supposed to be 100% compatible with Fallout TXT config formats (such as worldmap.txt).
*/
class Config {
public:
	typedef std::map<std::string, std::string, ci_less> Section;
	typedef std::map<std::string, Section, ci_less> Data;

	bool read(const char* filePath, bool isDb);

	bool getString(const char* sectionKey, const char* key, const std::string*& outValue);
	bool getInt(const char* sectionKey, const char* key, int& outValue, unsigned char base = 0);
	bool getDouble(const char* sectionKey, const char* key, double& outValue);

	const Data& data();

	// TODO:
	// bool write(const char* filePath, bool isDb);
	// bool setString(const char* sectionKey, const char* key, const char* value);
	// bool setInt(const char* sectionKey, const char* key, int value);
	// bool setDouble(const char* sectionKey, const char* key, double value);
private:
	std::string _lastSection;
	Data _data;

	static bool parseKeyValue(char* string, std::string& key, std::string& value);

	Data::iterator ensureSection(const char* sectionKey);
	bool parseLine(char* string);
};

}
