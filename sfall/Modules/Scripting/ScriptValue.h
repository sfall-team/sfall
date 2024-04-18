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

#include "..\..\FalloutEngine\Structs.h"

namespace sfall
{
namespace script
{

enum class DataType : unsigned short {
	NONE  = 0,
	INT   = 1,
	FLOAT = 2,
	STR   = 3,
};

/*
	Represents value passed to/from scripting engine.
*/
class ScriptValue {
public:
	ScriptValue(DataType type, unsigned long value);

	ScriptValue();
	ScriptValue(const char* strval);
	ScriptValue(int val);
	ScriptValue(long val);
	ScriptValue(unsigned long val);
	ScriptValue(float val);
	ScriptValue(bool val);

	ScriptValue(fo::GameObject* obj);

	bool isInt() const;

	bool isFloat() const;

	bool isString() const;

	unsigned long rawValue() const;

	long intValue() const;

	float floatValue() const;

	const char* strValue() const;

	fo::GameObject* object() const;

	// returns value as integer, converting if needed
	int asInt() const;

	// returns value converted as boolean (there is no real boolean type in SSL, 1 or 0 are used instead)
	bool asBool() const;

	// returns value as float, converting if needed
	float asFloat() const;

	// returns string value or empty string if value is not string type
	const char* asString() const;

	// returns value as object pointer or nullptr if value is not integer
	fo::GameObject* asObject() const;

	DataType type() const;

private:
	union Value {
		unsigned long dw;
		long i;
		float f;
		const char* str;
		fo::GameObject* gObj;
	} _val;

	DataType _type;
};

}
}
