/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace game
{

class Render {
public:
	static void init();

	static void __fastcall GNW_win_refresh(fo::Window* win, RECT* updateRect, BYTE* toBuffer);
};

}