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

class Translate {
public:
	static void init(const char* config);

	// Translates given string using sfall translation INI file and puts the result into given buffer
	static size_t Get(const char* section, const char* setting, const char* defaultValue, char* buffer, size_t bufSize = 128);

	// Translates given string using sfall translation INI file
	static std::string Get(const char* section, const char* setting, const char* defaultValue, size_t bufSize = 128);

	// Translates given list of strings using sfall translation INI file
	static std::vector<std::string> GetList(const char* section, const char* setting, const char* defaultValue, char delimiter, size_t bufSize = 256);

	/* Messages */

	// SaveSfallDataFail: "ERROR saving extended savegame information!"
	static const char* SfallSaveDataFailure();

	// SaveInCombat: "Cannot save at this time."
	static const char* CombatSaveBlockMessage();

	// BlockedCombat: "You cannot enter combat at this time."
	static const char* CombatBlockMessage();
};

}
