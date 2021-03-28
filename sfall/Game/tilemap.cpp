/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "tilemap.h"

namespace game
{

namespace sf = sfall;

static std::vector<int> buildLineTiles;

static bool TileExists(long tile) {
	return (std::find(buildLineTiles.cbegin(), buildLineTiles.cend(), tile) != buildLineTiles.cend());
}

// Fixed and improved implementation of tile_num_beyond_ engine function
// compared to the original implementation, this function gets the hex (tile) from the constructed line more correctly
long __fastcall Tilemap::tile_num_beyond(long sourceTile, long targetTile, long maxRange) {
	if (maxRange <= 0 || sourceTile == targetTile) return sourceTile;

	if (buildLineTiles.empty()) {
		buildLineTiles.reserve(100);
	} else {
		buildLineTiles.clear();
	}

	long currentRange = fo::func::tile_dist(sourceTile, targetTile);
	//fo::func::debug_printf("\ntile_dist: %d", currentRange);
	if (currentRange == maxRange) maxRange++; // increase the range if the target is located at a distance equal to maxRange (fix range)

	long lastTile = targetTile;
	long source_X, source_Y, target_X, target_Y;

	fo::func::tile_coord(sourceTile, &source_X, &source_Y);
	fo::func::tile_coord(targetTile, &target_X, &target_Y);

	// set the point to the center of the hexagon
	source_X += 16;
	source_Y += 8;
	target_X += 16;
	target_Y += 8;

	long diffX = target_X - source_X;
	long addToX = -1;
	if (diffX >= 0) addToX = (diffX > 0);

	long diffY = target_Y - source_Y;
	long addToY = -1;
	if (diffY >= 0) addToY = (diffY > 0);

	long diffX_x2 = 2 * std::abs(diffX);
	long diffY_x2 = 2 * std::abs(diffY);

	// Shift the starting point depending on the direction of the line building
	// to reduce the inaccuracy when getting the tile from the x/y coordinates
	// TODO: find a better way without having to shift the point
	long direction = (source_X != target_X) ? fo::func::tile_dir(sourceTile, targetTile) : -1;
	//fo::func::debug_printf("\ntile_dir: %d", direction);
	switch (direction) {
		case 0:
			target_X += 8;
			target_Y -= 4;
			break;
		case 1:
			target_X += 15;
			break;
		case 2:
			target_X += 8;
			target_Y += 4;
			break;
		case 3:
			target_X -= 8;
			target_Y += 4;
			break;
		case 4:
			target_X -= 15;
			break;
		case 5:
			target_X -= 8;
			target_Y -= 4;
			break;
	}
	const int step = 4;
	long stepCounter = step - 1;

	if (diffX_x2 > diffY_x2) {
		long stepY = diffY_x2 - (diffX_x2 >> 1);
		while (true) {
			if (++stepCounter == step) {
				long tile = fo::func::tile_num(target_X, target_Y);
				//fo::func::debug_printf("\ntile_num: %d [x:%d y:%d]", tile, target_X, target_Y);
				if (tile != lastTile) {
					if (!TileExists(tile)) {
						if (++currentRange >= maxRange || fo::func::tile_on_edge(tile)) return tile;
						buildLineTiles.push_back(tile);
					}
					lastTile = tile;
				}
				stepCounter = 0;
			}
			if (stepY >= 0) {
				stepY -= diffX_x2;
				target_Y += addToY;
			}
			stepY += diffY_x2;
			target_X += addToX;

			// Example of an algorithm constructing a straight line from point A to B
			// source: x = 784(800), y = 278(286)
			// target: x = 640(656), y = 314(322) - the target is located to the left and below
			// 0. x = 800, y = 286, stepY = -72
			// 1. x = 799, y = 286, stepY = -72+72=0
			// 2. x = 798, y = 287, stepY = 0-288+72=-216
			// 3. x = 797, y = 287, stepY = -216+72=-144
			// 4. x = 796, y = 287, stepY = -144+72=-72
			// 5. x = 795, y = 287, stepY = -72+72=0
			// 6. x = 794, y = 288, stepY = 0-288+72=-216
		}
	} else {
		long stepX = diffX_x2 - (diffY_x2 >> 1);
		while (true) {
			if (++stepCounter == step) {
				long tile = fo::func::tile_num(target_X, target_Y);
				//fo::func::debug_printf("\ntile_num: %d [x:%d y:%d]", tile, target_X, target_Y);
				if (tile != lastTile) {
					if (!TileExists(tile)) {
						if (++currentRange >= maxRange || fo::func::tile_on_edge(tile)) return tile;
						buildLineTiles.push_back(tile);
					}
					lastTile = tile;
				}
				stepCounter = 0;
			}
			if (stepX >= 0) {
				stepX -= diffY_x2;
				target_X += addToX;
			}
			stepX += diffX_x2;
			target_Y += addToY;
		}
	}
}

static void __declspec(naked) tile_num_beyond_hack() {
	__asm {
		//push ecx;
		push ebx;
		mov  ecx, eax;
		call Tilemap::tile_num_beyond;
		pop  ecx;
		retn;
	}
}

void Tilemap::init() {
	// Replace tile_num_beyond_ function
	sf::MakeJump(fo::funcoffs::tile_num_beyond_ + 1, tile_num_beyond_hack); // 0x4B1B84
}

}
