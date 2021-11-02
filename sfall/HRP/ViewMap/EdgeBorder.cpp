/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\Modules\LoadGameHook.h"

#include "ViewMap.h"

#include "EdgeBorder.h"

namespace sfall
{

static struct Edge {
	POINT center; // x/y center of borderRect
	RECT borderRect;
	RECT rect_2;
	RECT tileRect;
	RECT rect_4;
	long field_48;
	Edge* prevEdgeData; // unused? (used in 3.06)
	Edge* nextEdgeData;

	void Release() {
		Edge* edge = nextEdgeData;
		while (edge) {
			Edge* edgeNext = edge->nextEdgeData;
			delete edge;
			edge = edgeNext;
		};
	}

	~Edge() {
		Release();
	}
} *MapEdgeData;

// reference
Edge* CurrentMapEdge;

static long edgeVersion;
bool isLoadingMapEdge;
bool isDefaultSetEdge;

// Implementation from HRP by Mash
static void CalcEdgeData(Edge* edgeData, long w, long h) {
	long x, y;

	ViewMap::GetTileCoordOffset(edgeData->tileRect.left, x, y);
	edgeData->borderRect.left = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect.right, x, y);
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
		rectH -= rectH % 24; // truncate
		_rectH = rectH + 24;
	}
	if (rectH < h) {
		edgeData->borderRect.top    += _rectH;
		edgeData->borderRect.bottom -= rectH;
	} else {
		edgeData->borderRect.top    += h;
		edgeData->borderRect.bottom -= h;
	}

	// borderRect: right is less than left
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
	ViewMap::GetMapWindowSize(w, h);

	if (MapEdgeData == nullptr) MapEdgeData = new Edge[3];

	for (size_t i = 0; i < 3; i++) {
		Edge* edge = &MapEdgeData[i];

		edge->tileRect.left = 199;
		edge->tileRect.top = 0;
		edge->tileRect.right = 39800;
		edge->tileRect.bottom = 39999;

		CalcEdgeData(edge, w, h);

		edge->rect_4.left = 99;
		edge->rect_4.top = 0;
		edge->rect_4.right = 0;
		edge->rect_4.bottom = 99;

		edge->field_48 = 0;
		edge->prevEdgeData = nullptr;
		edge->nextEdgeData = nullptr;
	}
	edgeVersion = 0;
	isDefaultSetEdge = true;
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
		SetDefaultEdgeData();
		return file;
	}

	DWORD getValue;
	fo::func::db_freadInt(file, &getValue);
	if (getValue != 'EDGE') return file;

	fo::func::db_freadInt(file, &getValue);
	if (getValue == 1) {
		edgeVersion = 0;
	} else {
		if (getValue != 2) return file;
		edgeVersion = 1;
	}

	getValue = 0;
	if (fo::func::db_freadInt(file, &getValue) || getValue) return file; // unknown for now

	long w, h;
	ViewMap::GetMapWindowSize(w, h);

	if (MapEdgeData) {
		MapEdgeData[0].Release();
		MapEdgeData[1].Release();
		MapEdgeData[2].Release();
	} else {
		MapEdgeData = new Edge[3];
	}

	long mapLevel = 0;
	do {
		Edge* edgeData = &MapEdgeData[mapLevel];

		if (edgeVersion) {
			// load rectangle data (version 1) - unused/obsolete?
			if (fo::func::db_freadIntCount(file, (DWORD*)&edgeData->rect_4, 4) || fo::func::db_freadInt(file, (DWORD*)&edgeData->field_48)) {
				return file; // read error
			}
		} else {
			edgeData->rect_4.left = 99;
			edgeData->rect_4.top = 0;
			edgeData->rect_4.right = 0;
			edgeData->rect_4.bottom = 99;
			edgeData->field_48 = 0;
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

				Edge *edge = new Edge;
				edge->nextEdgeData = nullptr;
				edge->rect_4 = edgeData->rect_4; // rect copy
				edgeData->nextEdgeData = edge;
				edgeData = edge;
			}
		}
	} while (++mapLevel < 3);

	fo::func::db_fclose(file);
	return 0;
}

static void __fastcall LoadMapEdgeFile() {
	//isLoadingMapEdge = 0;

	fo::DbFile* file = LoadMapEdgeFileSub(LoadGameHook::mapLoadingName);
	if (file) { // load error
		fo::func::db_fclose(file);
		SetDefaultEdgeData();
	}
}

// Implementation from HRP by Mash
long EdgeBorder::GetCenterTile(long tile, long mapLevel) {
	if (!isDefaultSetEdge) SetDefaultEdgeData(); // needed at game initialization

	long x, y;
	ViewMap::GetTileCoordOffset(tile, x, y);

	Edge* edgeData = &MapEdgeData[mapLevel];
	CurrentMapEdge = edgeData;

	long mapWinW = fo::var::getInt(FO_VAR_buf_width_2);
	long mapWinH = fo::var::getInt(FO_VAR_buf_length_2);

	// fill with black, why? (black flickering appears if this is turned on)
	//std::memset((void*)fo::var::getInt(FO_VAR_display_buf), 0, mapWinW * mapWinH); // can use the _buf_size variable instead of multiplication
	//fo::func::win_draw(fo::var::getInt(FO_VAR_display_win));

	long width = (mapWinW / 2) - 1;
	long height = (mapWinH / 2) + 1;

	if (edgeData->nextEdgeData) {
		Edge* edge = edgeData;

		while (x >= (edge->rect_2.left + width) || x <= (edge->rect_2.right + width) ||
		       y <= (edge->rect_2.top - height) || y >= (edge->rect_2.bottom - height))
		{
			edge = edgeData->nextEdgeData;
			if (!edge) break;

			edgeData = edge;
			CurrentMapEdge = edge;
		}
	}

	long left = edgeData->borderRect.left;
	if (x <= left) {
		long right = edgeData->borderRect.right;
		if (x >= right) {
			edgeData->center.x = x;
		} else {
			edgeData->center.y = right;
		}
	} else {
		edgeData->center.x = left;
	}

	long bottom = edgeData->borderRect.bottom;
	if (y <= bottom) {
		long top = edgeData->borderRect.top;
		if (y >= top) {
			edgeData->center.y = y;
		} else {
			edgeData->center.y = top;
		}
	} else {
		edgeData->center.y = bottom;
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

	if (x > CurrentMapEdge->borderRect.left   || x < CurrentMapEdge->borderRect.right ||
	    y > CurrentMapEdge->borderRect.bottom || y < CurrentMapEdge->borderRect.top)
	{
		return 0; // block
	}

	long _mapModWidth = ViewMap::mapModWidth;
	long _mapModHeight = ViewMap::mapModHeight;
	ViewMap::mapModHeight = 0;
	ViewMap::mapModWidth = 0;

	long modWidth = 0;
	long modHeight = 0;

	if (x == CurrentMapEdge->borderRect.left) {
		modWidth = -ViewMap::mapWidthModSize;
		ViewMap::mapModWidth = modWidth;
	} else if (x == CurrentMapEdge->borderRect.right) {
		modWidth = ViewMap::mapWidthModSize;
		ViewMap::mapModWidth = modWidth;
	}

	if (y == CurrentMapEdge->borderRect.top) {
		modHeight = -ViewMap::mapHeightModSize;
		ViewMap::mapModHeight = modHeight;
	} else if (y == CurrentMapEdge->borderRect.bottom) {
		modHeight = ViewMap::mapHeightModSize;
		ViewMap::mapModHeight = modHeight;
	}

	return (_mapModWidth != modWidth || _mapModHeight != modHeight) ? 1 : -1; // 1 - for redrawing map
}

void EdgeBorder::init() {
	LoadGameHook::OnBeforeMapLoad() += LoadMapEdgeFile;
}

}
