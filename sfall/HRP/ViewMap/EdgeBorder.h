/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace sfall
{

class EdgeBorder {
public:
	static void init();

	static long GetCenterTile(long tile, long mapLevel);
	static long CheckBorder(long tile);
};

}
