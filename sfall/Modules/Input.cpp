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

//#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\SafeWrite.h"
//#include "LoadGameHook.h"
#include "ScriptExtender.h"

#include "Input.h"

namespace sfall
{

void Input::init() {
	//if (IniReader::GetConfigInt("Input", "Enable", 0)) {
		dlogr("Applying input patch.", DL_INIT);
		SafeWrite32(0x4DE902, 0x50FB50); // "DDRAW.DLL"
		::sfall::availableGlobalScriptTypes |= 1;
	//}
}

}
