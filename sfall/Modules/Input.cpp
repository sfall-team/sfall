/*
 *    sfall
 *    Copyright (C) 2008-2026  The sfall team
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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Translate.h"
#include "ScriptExtender.h"

#include "Input.h"

namespace sfall
{

#define TABLE_SIZE    (94) // for ASCII code 33-126

static BYTE xltTable[TABLE_SIZE];
static signed char xltKey; // 4 = Scroll Lock, 2 = Caps Lock, 1 = Num Lock

static DWORD getInputStrRet;

static void __declspec(naked) get_input_str_hack() {
	__asm { // al - key ASCII code, in range 32-126
		pop  getInputStrRet;
		push ecx;
		mov  cl, xltKey;
		test ds:[FO_VAR_kb_lock_flags], cl;
		jz   end;
		and  eax, 0xFF;
		cmp  eax, 32; // skip the space char
		je   end;
		mov  al, [xltTable + eax - 33];
end:
		mov  [esp + esi + 4], al;
		jmp  getInputStrRet;
	}
}

static void __declspec(naked) get_input_str2_hack() {
	__asm { // al - key ASCII code, in range 32-126
		pop  getInputStrRet;
		push edx;
		mov  dl, xltKey;
		test byte ptr ds:[FO_VAR_kb_lock_flags], dl;
		jz   end;
		and  eax, 0xFF;
		cmp  eax, 32; // skip the space char
		je   end;
		mov  al, [xltTable + eax - 33];
end:
		mov  [esp + edi + 4], al;
		jmp  getInputStrRet;
	}
}

static void __declspec(naked) kb_next_ascii_English_US_hack() {
	__asm {
		mov  dh, [eax];
		cmp  dh, DIK_LBRACKET;
		je   end;
		cmp  dh, DIK_RBRACKET;
		je   end;
		cmp  dh, DIK_SEMICOLON;
		je   end;
		cmp  dh, DIK_APOSTROPHE;
		je   end;
		cmp  dh, DIK_COMMA;
		je   end;
		cmp  dh, DIK_PERIOD;
		je   end;
		cmp  dh, DIK_B;
end:
		retn;
	}
}

void Input::init() {
	//if (IniReader::GetConfigInt("Input", "Enable", 0)) {
		dlogr("Applying input patch.", DL_INIT);
		SafeWrite32(0x4DE902, 0x50FB50); // "DDRAW.DLL"
		::sfall::availableGlobalScriptTypes |= 1;
	//}

	xltKey = IniReader::GetConfigInt("Input", "XltKey", 0);
	if (xltKey > 0) {
		auto codeList = Translate::GetList("sfall", "XltTable", "", ',');
		size_t numCodes = codeList.size();
		if (numCodes > 0) {
			dlogr("Applying alternate keyboard character codes patch.", DL_INIT);
			for (size_t i = 0; i < TABLE_SIZE; i++) {
				int code = (i < numCodes) ? atoi(codeList[i].c_str()) : 0;
				xltTable[i] = (code > 32 && code < 256)
				            ? static_cast<BYTE>(code)
				            : static_cast<BYTE>(i + 33); // invalid char code, use default
			}
			if (xltKey > 2) xltKey = 4;
			MakeCall(0x433F3E, get_input_str_hack);
			MakeCall(0x47F364, get_input_str2_hack);
			MakeCall(0x4CC358, kb_next_ascii_English_US_hack);
			SafeWriteBatch<BYTE>(0x7E, {0x433ED6, 0x47F2F7}); // max ASCII code value allowed to input (was 122)
		}
	}
}

}
