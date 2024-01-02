/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

#include "..\..\FalloutEngine\Fallout2.h"

namespace game
{
namespace gui
{

class Render {
public:
	static void init();

	static void __fastcall GNW_win_refresh(fo::Window* win, RECT* updateRect, BYTE* toBuffer);
};

}
}
