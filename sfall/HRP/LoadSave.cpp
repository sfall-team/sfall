/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\LoadGameHook.h"

#include "LoadSave.h"

namespace HRP
{

namespace sf = sfall;

static long xPosition;
static long yPosition;

static long __fastcall LoadSaveWinAdd(long height, long yPos, long xPos, long width, long color, long flags) {
	xPos += (Setting::ScreenWidth() - width) / 2;
	yPos += (Setting::ScreenHeight() - height) / 2;

	if (sfall::IsGameLoaded()) {
		yPos -= 50;
		if (yPos < 0) yPos = 0;
	}
	xPosition = xPos;
	yPosition = yPos;

	return fo::func::win_add(xPos, yPos, width, height, color, flags);
}

static __declspec(naked) void LSGameStart_hook_win_add() {
	__asm {
		xchg ebx, [esp]; // width
		push eax;        // xPos
		push ebx;        // ret addr
		jmp  LoadSaveWinAdd;
	}
}

static __declspec(naked) void GetComment_hook_win_add() {
	__asm {
		add  edx, yPosition;
		add  eax, xPosition;
		jmp  fo::funcoffs::win_add_;
	}
}

static __declspec(naked) void LoadSaveMouseGetPositionHook() {
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

static __declspec(naked) void QuickSnapShotHook() {
	__asm {
		mov  ebx, ds:[FO_VAR_buf_length_2];
		mov  ecx, ds:[FO_VAR_buf_width_2];
		mov  edx, ecx;
		jmp  fo::funcoffs::cscale_;
	}
}

void LoadSave::init() {
	sf::HookCall(0x47D529, LSGameStart_hook_win_add);
	sf::HookCall(0x47ED8C, GetComment_hook_win_add);

	sf::HookCalls(LoadSaveMouseGetPositionHook, {
		0x47CC0F, // LoadGame_
		0x47BE3D  // SaveGame_
	});

	sf::HookCalls(QuickSnapShotHook, {
		0x47C627, // QuickSnapShot_
		0x47D42F  // LSGameStart_
	});
}

}
