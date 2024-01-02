/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

	static long __fastcall make_path_func(fo::GameObject* srcObject, long sourceTile, long targetTile, long maxNodes, void* arrayRef, long checkTargetTile, void* blockFunc);

	static void SetPathMaxNodes(long maxNodes);
};

}
