/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\LoadGameHook.h"

#include "MiscInterface.h"

namespace HRP
{

namespace sf = sfall;

static long xPosition;
static long yPosition;

static long __fastcall CommonWinAdd(long height, long width, long color, long flags) {
	xPosition = (Setting::ScreenWidth() - width) / 2;
	yPosition = (Setting::ScreenHeight() - height) / 2;

	if (sf::IsGameLoaded()) {
		yPosition -= 50;
		if (yPosition < 0) yPosition = 0;
	}
	return fo::func::win_add(xPosition, yPosition, width, height, color, flags);
}

static __declspec(naked) void CommonWinAddHook() {
	__asm {
		mov  edx, ebx; // width
		jmp  CommonWinAdd;
	}
}

static __declspec(naked) void MouseGetPositionHook() {
	__asm {
		push eax; // outX ref
		push edx; // outY ref
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

void MiscInterface::init() {
	sf::HookCalls(CommonWinAddHook, {
		0x497405, // StartPipboy_
		0x41B979, // automap_
		0x42626B, // get_called_shot_location_
		0x43F560, // elevator_start_
		0x490005, // OptnStart_
		0x490961, // PrefStart_
	});

	sf::HookCalls(MouseGetPositionHook, {
		0x490EAC, 0x491546, // DoThing_
		0x497092, // pipboy_
		0x49A1B6, // ScreenSaver_
	});
	sf::BlockCall(0x49A0FB); // ScreenSaver_

	// PauseWindow_
	sf::SafeWrite32(0x49042A, Setting::ScreenHeight());
	sf::SafeWrite32(0x490436, Setting::ScreenWidth());
	// ShadeScreen_
	sf::SafeWrite32(0x49075F, Setting::ScreenWidth());
}

}
