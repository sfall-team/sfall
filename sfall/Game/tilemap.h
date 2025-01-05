/*
 *    sfall
 *    Copyright (C) 2008-2025  The sfall team
 *
 */

#pragma once

namespace game
{

class Tilemap {
public:
	static void init();

	static void obj_path_blocking_at_();

	static long __fastcall tile_num_beyond(long sourceTile, long targetTile, long maxRange);
};

}
