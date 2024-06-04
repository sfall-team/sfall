/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\LoadGameHook.h"

#include "Character.h"

namespace HRP
{

namespace sf = sfall;

static long xPosition;
static long yPosition;

static long __fastcall CharacterWinAdd(long height, long yPos, long xPos, long width, long color, long flags) {
	if (sf::IsGameLoaded()) yPos -= 50;

	yPos += (Setting::ScreenHeight() - height) / 2;
	xPos += (Setting::ScreenWidth() - width) / 2;

	if (yPos < 0) yPos = 0;

	xPosition = xPos;
	yPosition = yPos;

	return fo::func::win_add(xPos, yPos, width, height, color, flags);
}

static __declspec(naked) void CharacterWinAddHook() {
	__asm {
		xchg ebx, [esp]; // width
		push eax;        // xPos
		push ebx;        // ret addr
		jmp  CharacterWinAdd;
	}
}

static __declspec(naked) void CharacterInputWinAddHook() {
	__asm {
		add  edx, yPosition;
		add  eax, xPosition;
		jmp  fo::funcoffs::win_add_;
	}
}

// Implementation from HRP by Mash
static __declspec(naked) void CharacterSubmenuHook() {
	__asm {
		mov  eax, ds:[FO_VAR_edit_win];
		call fo::funcoffs::GNW_find_;
		test eax, eax;
		jz   noCharWin;
		mov  ecx, [eax + 0x8];               // charWin->rect.left
		add  dword ptr ss:[ebp + 0x10], ecx; // subWin xPos
		mov  ecx, [eax + 0xC];               // charWin->rect.top
		add  dword ptr ss:[ebp + 0x14], ecx; // subWin yPos
noCharWin:
		jmp  fo::funcoffs::text_curr_;
	}
}

static long __fastcall DialogBoxWinAdd(long height, long yPos, long xPos, long width, long color, long flags) {
	yPos = (Setting::ScreenHeight() - height) / 2;
	xPos = (Setting::ScreenWidth() - width) / 2;

	if (sf::IsGameLoaded()) yPos -= 50;

	return fo::func::win_add(xPos, yPos, width, height, color, flags);
}

static __declspec(naked) void dialog_out_hook_win_add() {
	__asm {
		xchg ebx, [esp]; // width
		push eax;        // xPos
		push ebx;        // ret addr
		jmp  DialogBoxWinAdd;
	}
}

static __declspec(naked) void CharacterMouseGetPositionHook() {
	__asm {
		push eax;
		push edx;
		call fo::funcoffs::mouse_get_position_;
		pop  edx;
		mov  eax, yPosition;
		sub  [edx], eax;
		pop  edx;
		mov  eax, xPosition;
		sub  [edx], eax;
		retn;
	}
}

void Character::init() {
	sf::HookCalls(CharacterWinAddHook, {
		0x4A7496, // select_init_
		0x432DE8  // CharEditStart_
	});

	sf::HookCalls(CharacterInputWinAddHook, {
		0x436C8E, // NameWindow_
		0x437045, // AgeWindow_
		0x43769B, // SexWindow_
		0x43800A, // OptionWindow_
		0x43C580  // perks_dialog_
	});

	sf::HookCalls(CharacterSubmenuHook, {
		0x41DEBA, // file_dialog_
		0x41EAA2  // save_file_dialog_
	});

	sf::HookCall(0x41D104, dialog_out_hook_win_add);

	sf::HookCalls(CharacterMouseGetPositionHook, {
		0x43AE93, // FldrButton_
		0x43AF59, // InfoButton_
		0x43CB5C  // InputPDLoop_
	});
}

}
