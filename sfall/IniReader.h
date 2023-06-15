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

#pragma once

namespace sfall
{
class Config;

class IniReader {
public:
	static void init();

	static DWORD modifiedIni;

	static const char* GetConfigFile();
	static void SetDefaultConfigFile();
	static void SetConfigFile(const char* iniFile);

	// Gets a Config from an INI file at given path, relative to game root folder.
	// Config is loaded once per given path when requested and only unloaded on game reset (returning to main menu).
	static Config* GetIniConfig(const char* iniFile);

	// Gets the integer value from the default config (i.e. ddraw.ini)
	static int GetIntDefaultConfig(const char* section, const char* setting, int defaultValue);

	static std::string GetStringDefaultConfig(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

	// Gets a list of values separated by the delimiter from the default config (i.e. ddraw.ini)
	static std::vector<std::string> GetListDefaultConfig(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter);

	// Gets the integer value from sfall configuration INI file
	static int GetConfigInt(const char* section, const char* setting, int defaultValue);

	// Gets the string value from sfall configuration INI file with trim function
	static std::string GetConfigString(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

	// Loads the string value from sfall configuration INI file into the provided buffer
	static size_t GetConfigString(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize = 128);

	// Parses the comma-separated list from the settings from sfall configuration INI file
	static std::vector<std::string> GetConfigList(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

	// Gets the integer value from given INI file
	static int GetInt(const char* section, const char* setting, int defaultValue, const char* iniFile);

	// Gets the string value from given INI file
	static size_t GetString(const char* section, const char* setting, const char* defaultValue, char* buf, size_t bufSize, const char* iniFile);

	// Gets the string value from given INI file
	static std::string GetString(const char* section, const char* setting, const char* defaultValue, size_t bufSize, const char* iniFile);

	// Parses the comma-separated list setting from given INI file
	static std::vector<std::string> GetList(const char* section, const char* setting, const char* defaultValue, size_t bufSize, char delimiter, const char* iniFile);

	static int SetConfigInt(const char* section, const char* setting, int value);

	static int SetConfigString(const char* section, const char* setting, const char* value);

	static int SetDefaultConfigInt(const char* section, const char* setting, int value);

	static int SetDefaultConfigString(const char* section, const char* setting, const char* value);
};

}
