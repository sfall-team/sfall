/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
 *
 */

#pragma once

namespace game
{

class Tilemap {
public:
	static void init();

	static long __fastcall tile_num_beyond(long sourceTile, long targetTile, long maxRange);
};

}
