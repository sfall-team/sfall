/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\Modules\LoadGameHook.h"

#include "ViewMap.h"

#include "EdgeBorder.h"

namespace HRP
{

namespace sf = sfall;

static EdgeBorder::Edge* MapEdgeData = nullptr;
static EdgeBorder::Edge* currentMapEdge; // reference

static long edgeVersion; // 0 - version 1 (obsolete), 1 - version 2 (current)
static bool isSetDefaultEdge;

long EdgeBorder::EdgeVersion() { return edgeVersion; }

EdgeBorder::Edge* EdgeBorder::CurrentMapEdge() { return currentMapEdge; }

// Implementation from HRP by Mash
static void CalcEdgeData(EdgeBorder::Edge* edgeData, long w, long h) {
	long x, y;

	ViewMap::GetTileCoordOffset(edgeData->tileRect.left, x, y); // upper left corner?
	edgeData->borderRect.left = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect.right, x, y); // upper right corner?
	edgeData->borderRect.right = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect.top, x, y);
	edgeData->borderRect.top = y;

	ViewMap::GetTileCoordOffset(edgeData->tileRect.bottom, x, y);
	edgeData->borderRect.bottom = y;

	long mapWinW = (fo::var::getInt(FO_VAR_buf_width_2) / 2) - 1;  // 1280/2 -1 = 639 (320)
	long mapWinH = (fo::var::getInt(FO_VAR_buf_length_2) / 2) - 1; // 620/2 -1 = 309 (190)

	edgeData->rect_2.left   = edgeData->borderRect.left   - mapWinW;
	edgeData->rect_2.right  = edgeData->borderRect.right  - mapWinW;
	edgeData->rect_2.top    = edgeData->borderRect.top    + mapWinH;
	edgeData->rect_2.bottom = edgeData->borderRect.bottom + mapWinH;

	long rectW = (edgeData->borderRect.left - edgeData->borderRect.right) / 2;
	long _rectW = rectW;
	if (rectW & 31) {
		rectW &= ~31; // truncate
		_rectW = rectW + 32;
	}
	if (rectW < w) {
		edgeData->borderRect.left  -= rectW;
		edgeData->borderRect.right += _rectW;
	} else {
		edgeData->borderRect.left  -= w;
		edgeData->borderRect.right += w;
	}

	long rectH = (edgeData->borderRect.bottom - edgeData->borderRect.top) / 2;
	long _rectH = rectH;
	if (rectH % 24) {
		rectH -= rectH % 24;
		_rectH = rectH + 24;
	}
	if (rectH < h) {
		edgeData->borderRect.top    += _rectH;
		edgeData->borderRect.bottom -= rectH;
	} else {
		edgeData->borderRect.top    += h;
		edgeData->borderRect.bottom -= h;
	}

	if ((edgeData->borderRect.left < edgeData->borderRect.right) || (edgeData->borderRect.left - edgeData->borderRect.right) == 32) {
		edgeData->borderRect.left = edgeData->borderRect.right;
	}
	if ((edgeData->borderRect.bottom < edgeData->borderRect.top) || (edgeData->borderRect.bottom - edgeData->borderRect.top) == 24) {
		edgeData->borderRect.bottom = edgeData->borderRect.top;
	}

	x = edgeData->borderRect.right + ((edgeData->borderRect.left - edgeData->borderRect.right) / 2);
	edgeData->center.x = x & ~31;

	y = edgeData->borderRect.top + ((edgeData->borderRect.bottom - edgeData->borderRect.top) / 2);
	edgeData->center.y = y - (y % 24);
}

static void SetDefaultEdgeData() {
	long w, h;
	ViewMap::GetWinMapHalfSize(w, h);

	if (MapEdgeData == nullptr) MapEdgeData = new EdgeBorder::Edge[3];

	for (size_t i = 0; i < 3; i++) {
		EdgeBorder::Edge* edge = &MapEdgeData[i];

		edge->tileRect.left = 199;
		edge->tileRect.top = 0;
		edge->tileRect.right = 39800;
		edge->tileRect.bottom = 39999;

		CalcEdgeData(edge, w, h);

		edge->squareRect.left = 99;
		edge->squareRect.top = 0;
		edge->squareRect.right = 0;
		edge->squareRect.bottom = 99;

		edge->clipData = 0;
		edge->prevEdgeData = nullptr;
		edge->nextEdgeData = nullptr;
	}
	edgeVersion = 0;
	isSetDefaultEdge = true;
}

// Implementation from HRP by Mash
static fo::DbFile* LoadMapEdgeFileSub(char* mapName) {
	char edgPath[33];

	char* posDot = std::strchr(mapName, '.');
	*posDot = '\0';
	std::sprintf(edgPath, "maps\\%s.edg", mapName);
	*posDot = '.';

	fo::DbFile* file = fo::func::db_fopen(edgPath, "rb");
	if (!file) {
		SetDefaultEdgeData(); // TODO: support for original blockers
		return file;
	}

	DWORD getValue;
	fo::func::db_freadInt(file, &getValue);
	if (getValue != 'EDGE') return file;

	fo::func::db_freadInt(file, &getValue);
	if (getValue == 1) {
		edgeVersion = 0; // old
	} else {
		if (getValue != 2) return file; // incorrect version
		edgeVersion = 1;
	}

	getValue = 0;
	if (fo::func::db_freadInt(file, &getValue) || getValue) return file; // error, incorrect map level

	long w, h;
	ViewMap::GetWinMapHalfSize(w, h);

	if (MapEdgeData) {
		MapEdgeData[0].Release();
		MapEdgeData[1].Release();
		MapEdgeData[2].Release();
	} else {
		MapEdgeData = new EdgeBorder::Edge[3];
	}

	long mapLevel = 0;
	do {
		EdgeBorder::Edge* edgeData = &MapEdgeData[mapLevel];

		if (edgeVersion) {
			// load rectangle data (version 2)
			if (fo::func::db_freadIntCount(file, (DWORD*)&edgeData->squareRect, 4) || fo::func::db_freadInt(file, (DWORD*)&edgeData->clipData)) {
				return file; // read error
			}
		} else {
			edgeData->squareRect.left = 99;
			edgeData->squareRect.top = 0;
			edgeData->squareRect.right = 0;
			edgeData->squareRect.bottom = 99;
			edgeData->clipData = 0;
		}

		if (getValue == mapLevel) {
			while (true) {
				long result = fo::func::db_freadIntCount(file, (DWORD*)&edgeData->tileRect, 4); // load rectangle data
				if (result != 0) return file; // read error

				CalcEdgeData(edgeData, w, h);

				if (fo::func::db_freadInt(file, &getValue)) { // are there more rectangles on the current map level?
					// the end of file is reached (read error)
					if (mapLevel != 2) return file;

					getValue = -1;
					break; // next level
				}
				if (getValue != mapLevel) break; // next level

				EdgeBorder::Edge *edge = new EdgeBorder::Edge;
				edge->prevEdgeData = edgeData;
				edge->nextEdgeData = nullptr;
				edge->clipData = 0;
				edge->squareRect = edgeData->squareRect; // rect copy
				edgeData->nextEdgeData = edge;
				edgeData = edge;
			}
		}
	} while (++mapLevel < 3);

	fo::func::db_fclose(file);
	return 0;
}

static void __fastcall LoadMapEdgeFile() {
	fo::DbFile* file = LoadMapEdgeFileSub(sf::LoadGameHook::mapLoadingName);
	if (file) { // load error
		fo::func::db_fclose(file);
		SetDefaultEdgeData();
	}
}

// Implementation from HRP by Mash
long EdgeBorder::GetCenterTile(long tile, long mapLevel) {
	if (!isSetDefaultEdge) SetDefaultEdgeData(); // needed at game initialization

	long tX, tY;
	ViewMap::GetTileCoordOffset(tile, tX, tY);

	Edge* edgeData = &MapEdgeData[mapLevel];
	currentMapEdge = edgeData;

	// clear unreferenced areas when EDGE_CLIPPING is enabled (maybe there is another way to do this, and not every time)
	std::memset((void*)fo::var::getInt(FO_VAR_display_buf), 0, fo::var::getInt(FO_VAR_buf_size));
	//fo::func::win_draw(fo::var::getInt(FO_VAR_display_win)); // for test

	if (edgeData->nextEdgeData) {
		long mapWinW = fo::var::getInt(FO_VAR_buf_width_2);
		long mapWinH = fo::var::getInt(FO_VAR_buf_length_2);

		long width = (mapWinW / 2) - 1;
		long height = (mapWinH / 2) + 1;
		Edge* edge = edgeData;

		while (tX >= (edge->rect_2.left + width) || tX <= (edge->rect_2.right + width) ||
		       tY <= (edge->rect_2.top - height) || tY >= (edge->rect_2.bottom - height))
		{
			edge = edgeData->nextEdgeData;
			if (!edge) break;

			edgeData = edge;
			currentMapEdge = edge;
		}
	}

	long leftX = edgeData->borderRect.left;
	if (tX <= leftX) {
		long rightX = edgeData->borderRect.right;
		if (tX >= rightX) {
			edgeData->center.x = tX;
		} else {
			edgeData->center.x = rightX;
		}
	} else {
		edgeData->center.x = leftX;
	}

	long bottomY = edgeData->borderRect.bottom;
	if (tY <= bottomY) {
		long topY = edgeData->borderRect.top;
		if (tY >= topY) {
			edgeData->center.y = tY;
		} else {
			edgeData->center.y = topY;
		}
	} else {
		edgeData->center.y = bottomY;
	}

	ViewMap::mapModHeight = 0;
	ViewMap::mapModWidth = 0;

	if (edgeData->center.x == edgeData->borderRect.left) {
		ViewMap::mapModWidth = -ViewMap::mapWidthModSize;
	} else if (edgeData->center.x == edgeData->borderRect.right) {
		ViewMap::mapModWidth = ViewMap::mapWidthModSize;
	}

	if (edgeData->center.y == edgeData->borderRect.top) {
		ViewMap::mapModHeight = -ViewMap::mapHeightModSize;
	} else if (edgeData->center.y == edgeData->borderRect.bottom) {
		ViewMap::mapModHeight = ViewMap::mapHeightModSize;
	}

	long cX = edgeData->center.x;
	long cY = edgeData->center.y;
	ViewMap::GetCoordFromOffset(cX, cY);

	return cX + (cY * 200); // tile of center
}

// Implementation from HRP by Mash
long EdgeBorder::CheckBorder(long tile) {
	long x, y;
	ViewMap::GetTileCoordOffset(tile, x, y);

	if (x > currentMapEdge->borderRect.left || x < currentMapEdge->borderRect.right ||
	    y < currentMapEdge->borderRect.top  || y > currentMapEdge->borderRect.bottom)
	{
		return 0; // block
	}

	long mapModWidth = ViewMap::mapModWidth;
	long mapModHeight = ViewMap::mapModHeight;
	ViewMap::mapModHeight = 0;
	ViewMap::mapModWidth = 0;

	if (x == currentMapEdge->borderRect.left) {
		ViewMap::mapModWidth = -ViewMap::mapWidthModSize;
	} else if (x == currentMapEdge->borderRect.right) {
		ViewMap::mapModWidth = ViewMap::mapWidthModSize;
	}

	if (y == currentMapEdge->borderRect.top) {
		ViewMap::mapModHeight = -ViewMap::mapHeightModSize;
	} else if (y == currentMapEdge->borderRect.bottom) {
		ViewMap::mapModHeight = ViewMap::mapHeightModSize;
	}

	return (mapModWidth != ViewMap::mapModWidth || mapModHeight != ViewMap::mapModHeight) ? 1 : -1; // 1 - for redrawing map
}

void EdgeBorder::init() {
	sf::LoadGameHook::OnBeforeMapLoad() += LoadMapEdgeFile;

	sf::LoadGameHook::OnBeforeGameClose() += []() { delete[] MapEdgeData; };
}

}
