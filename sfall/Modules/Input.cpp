
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "..\Logging.h"
#include "..\SafeWrite.h"
#include "LoadGameHook.h"
#include "ScriptExtender.h"

#include "Input.h"

namespace sfall
{

static char* xltTable = nullptr;
static char xltKey; // 4 = Scroll Lock, 2 = Caps Lock, 1 = Num Lock

static void __declspec(naked) get_input_str_hack() {
	__asm {
		push ecx;
		mov  cl, xltKey;
		test byte ptr ds:[FO_VAR_kb_lock_flags], cl;
		jz   end;
		and  eax, 0xFF;
		mov  ecx, [xltTable];
		mov  al, [ecx][eax - 0x20];
end:
		mov  [esp + esi + 4], al;
		mov  ecx, 0x433F43;
		jmp  ecx;
	}
}

static void __declspec(naked) get_input_str2_hack() {
	__asm {
		push edx;
		mov  dl, xltKey;
		test byte ptr ds:[FO_VAR_kb_lock_flags], dl;
		jz   end;
		and  eax, 0xFF;
		mov  edx, [xltTable];
		mov  al, [edx][eax - 0x20];
end:
		mov  [esp + edi + 4], al;
		mov  edx, 0x47F369;
		jmp  edx;
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
	//if(GetConfigInt("Input", "Enable", 0)) {
		dlog("Applying input patch.", DL_INIT);
		SafeWriteStr(0x50FB70, "ddraw.dll");
		::sfall::availableGlobalScriptTypes |= 1;
		dlogr(" Done", DL_INIT);
	//}

	xltKey = GetConfigInt("Input", "XltKey", 0);
	if (xltKey > 0) {
		auto table = GetConfigString("Input", "XltTable", "", 512);
		if (!table.empty()) {
			dlog("Applying alternate keyboard character codes patch.", DL_INIT);
			int count = 0;
			xltTable = new char[94];
			std::string::size_type pos = 0;
			while (pos != std::string::npos && count < 94) {
				if (pos) pos++;
				int code = atoi(table.c_str() + pos);
				if (code > 31 && code < 256) xltTable[count++] = static_cast<BYTE>(code);
				pos = table.find(',', ++pos);
			}
			if (count == 94) {
				if (xltKey > 2) xltKey = 4;
				MakeJump(0x433F3E, get_input_str_hack);
				MakeJump(0x47F364, get_input_str2_hack);
				MakeCall(0x4CC358, kb_next_ascii_English_US_hack);
				SafeWriteBatch<BYTE>(0x7D, {0x433ED6, 0x47F2F7}); // number of additional keys allowed to input 125, instead of 122
				dlogr(" Done", DL_INIT);
			} else {
				delete[] xltTable;
				xltTable = nullptr;
				dlogr(" Failed", DL_INIT);
			}
		}
	}

	LoadGameHook::OnGameReset() += []() {
		ForceGraphicsRefresh(0);
	};
}

void Input::exit() {
	if (xltTable) delete[] xltTable;
}

}
