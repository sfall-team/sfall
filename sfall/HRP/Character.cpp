/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
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

static void __declspec(naked) CharacterWinAddHook() {
	__asm {
		xchg ebx, [esp]; // width
		push eax;        // xPos
		push ebx;        // ret addr
		jmp  CharacterWinAdd;
	}
}

static void __declspec(naked) CharacterInputWinAddHook() {
	__asm {
		add  edx, yPosition;
		add  eax, xPosition;
		jmp  fo::funcoffs::win_add_;
	}
}

static long __fastcall DialogBoxWinAdd(long height, long yPos, long xPos, long width, long color, long flags) {
	yPos = (Setting::ScreenHeight() - height) / 2;
	xPos = (Setting::ScreenWidth() - width) / 2;

	if (sf::IsGameLoaded()) yPos -= 50;

	return fo::func::win_add(xPos, yPos, width, height, color, flags);
}

static void __declspec(naked) dialog_out_hook_win_add() {
	__asm {
		xchg ebx, [esp]; // width
		push eax;        // xPos
		push ebx;        // ret addr
		jmp  DialogBoxWinAdd;
	}
}

static void __declspec(naked) CharacterMouseGetPositionHook() {
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
		0x43C580, // perks_dialog_
		0x41E027, // file_dialog_
		0x41EC22  // save_file_dialog_
	});

	sf::HookCall(0x41D104, dialog_out_hook_win_add);

	sf::HookCalls(CharacterMouseGetPositionHook, {
		0x43AE93, // FldrButton_
		0x43AF59, // InfoButton_
		0x43CB5C  // InputPDLoop_
	});
}

}
