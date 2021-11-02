/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Init.h"

#include "InterfaceBar.h"

namespace sfall
{

long xPosition;

static long __fastcall intface_init_win_add(long height, long yPos, long xPos, long width, long color, long flags) {
	yPos += HRP::ScreenHeight() - 480; // yPos:379 = 479-100
	xPos += (HRP::ScreenWidth() - width) / 2;
	xPosition = xPos;

	return fo::func::win_add(xPos, yPos, width, height, color, flags);
}

static void __declspec(naked) intface_init_hook_win_add() {
	__asm {
		pop  ebp; // ret addr
		push ebx; // width
		push eax; // xPos
		push ebp;
		jmp  intface_init_win_add;
	}
}

void IFaceBar::init() {
	HookCall(0x45D8BC, intface_init_hook_win_add);
}

}
