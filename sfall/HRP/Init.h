/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace sfall
{

class HRP {
public:
	static void init();

	// Built-in high resolution patch
	static bool Enabled;

	static long ScreenWidth();
	static long ScreenHeight();
};

}
