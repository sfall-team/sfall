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
	long centerX;
	long centerY;
	RECT rect_1;
	RECT rect_2;
	RECT tileRect_3;
	RECT rect_4;
	long field_48;
	Edge* prevEdgeData; // unused? (used in 3.06)
	Edge* nextEdgeData_50;

	void Release() {
		Edge* edge = nextEdgeData_50;
		while (edge) {
			Edge* edgeNext = edge->nextEdgeData_50;
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

long EdgeVersion;
bool isLoadingMapEdge;
bool isDefaultSetEdge;

// Implementation from HRP by Mash
static void CalcEdgeData(Edge* edgeData, long w, long h) {
	long x, y;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.left, x, y);
	edgeData->rect_1.left = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.right, x, y);
	edgeData->rect_1.right = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.top, x, y);
	edgeData->rect_1.top = y;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.bottom, x, y);
	edgeData->rect_1.bottom = y;

	long mapWinW = (fo::var::getInt(FO_VAR_buf_width_2) / 2) - 1;  // 1280/2 -1 = 639 (320)
	long mapWinH = (fo::var::getInt(FO_VAR_buf_length_2) / 2) - 1; // 620/2 -1 = 309 (190)

	edgeData->rect_2.left   = edgeData->rect_1.left   - mapWinW;
	edgeData->rect_2.right  = edgeData->rect_1.right  - mapWinW;
	edgeData->rect_2.top    = edgeData->rect_1.top    + mapWinH;
	edgeData->rect_2.bottom = edgeData->rect_1.bottom + mapWinH;

	long rectW = (edgeData->rect_1.left - edgeData->rect_1.right) / 2;
	long _rectW = rectW;
	if (rectW & 31) {
		rectW &= ~31; // truncate
		_rectW = rectW + 32;
	}
	long left, right;
	if (rectW < w) {
		left = edgeData->rect_1.left - rectW;
		right = _rectW + edgeData->rect_1.right;
	} else {
		left = edgeData->rect_1.left - w;
		right = w + edgeData->rect_1.right;
	}
	edgeData->rect_1.right = right;
	edgeData->rect_1.left = left;

	long rectH = (edgeData->rect_1.bottom - edgeData->rect_1.top) / 2;
	long _rectH = rectH;
	if (rectH % 24) {
		rectH -= rectH % 24; // truncate
		_rectH = rectH + 24;
	}
	long top, bottom;
	if (rectH < h) {
		top = _rectH + edgeData->rect_1.top;
		bottom = edgeData->rect_1.bottom - rectH;
	} else {
		bottom = edgeData->rect_1.bottom - h;
		top = h + edgeData->rect_1.top;
	}
	edgeData->rect_1.top = top;
	edgeData->rect_1.bottom = bottom;

	if (edgeData->rect_1.left   < edgeData->rect_1.right) edgeData->rect_1.left = edgeData->rect_1.right;
	if (edgeData->rect_1.bottom < edgeData->rect_1.top) edgeData->rect_1.bottom = edgeData->rect_1.top;

	if (edgeData->rect_1.left   - edgeData->rect_1.right == 32) edgeData->rect_1.left = edgeData->rect_1.right;
	if (edgeData->rect_1.bottom - edgeData->rect_1.top == 24) edgeData->rect_1.bottom = edgeData->rect_1.top;

	// here right is less than left
	x = edgeData->rect_1.right + ((edgeData->rect_1.left - edgeData->rect_1.right) / 2);
	edgeData->centerX = x & ~31;

	y = edgeData->rect_1.top + ((edgeData->rect_1.bottom - edgeData->rect_1.top) / 2);
	edgeData->centerY = y - (y % 24);
}

static void SetDefaultEdgeData() {
	long w, h;
	ViewMap::GetMapWindowSize(w, h);

	if (MapEdgeData == nullptr) MapEdgeData = new Edge[3];

	for (size_t i = 0; i < 3; i++) {
		Edge* edge = &MapEdgeData[i];

		edge->tileRect_3.left = 199;
		edge->tileRect_3.top = 0;
		edge->tileRect_3.right = 39800;
		edge->tileRect_3.bottom = 39999;
		edge->prevEdgeData = nullptr;

		CalcEdgeData(edge, w, h);

		edge->rect_4.left = 99;
		edge->rect_4.top = 0;
		edge->rect_4.right = 0;
		edge->rect_4.bottom = 99;

		edge->field_48 = 0;
		edge->nextEdgeData_50 = nullptr;
	}
	EdgeVersion = 0;
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
		EdgeVersion = 0;
	} else {
		if (getValue != 2) return file;
		EdgeVersion = 1;
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

		if (EdgeVersion) {
			// load rectangle data (version 1)
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
				long result = fo::func::db_freadIntCount(file, (DWORD*)&edgeData->tileRect_3, 4); // load rectangle data
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
				edge->nextEdgeData_50 = nullptr;
				edge->rect_4 = edgeData->rect_4; // rect copy
				edgeData->nextEdgeData_50 = edge;
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

	// fill with black, why?
	//std::memset((void*)fo::var::getInt(FO_VAR_display_buf), 0, mapWinW * mapWinH); // can use the _buf_size variable instead of multiplication
	//fo::func::win_draw(fo::var::getInt(FO_VAR_display_win));

	if (edgeData->nextEdgeData_50) {
		long width = (mapWinW / 2) - 1;
		long height = (mapWinH / 2) + 1;
		Edge* edge = edgeData;

		while (x >= width + edge->rect_2.left || x <= width + edge->rect_2.right ||
		       y <= edge->rect_2.top - height || y >= edge->rect_2.bottom - height)
		{
			edge = edgeData->nextEdgeData_50;
			if (!edge) break;
			edgeData = edge;
			CurrentMapEdge = edge;
		}
	}

	long left = edgeData->rect_1.left;
	if (x <= left) {
		long right = edgeData->rect_1.right;
		if (x >= right) {
			edgeData->centerX = x;
		} else {
			edgeData->centerX = right;
		}
	} else {
		edgeData->centerX = left;
	}

	long bottom = edgeData->rect_1.bottom;
	if (y <= bottom) {
		long top = edgeData->rect_1.top;
		if (y >= top) {
			edgeData->centerY = y;
		} else {
			edgeData->centerY = top;
		}
	} else {
		edgeData->centerY = bottom;
	}

	ViewMap::mapHalfHeight = 0;
	ViewMap::mapHalfWidth = 0;

	if (edgeData->centerX == edgeData->rect_1.left) {
		ViewMap::mapHalfHeight = -ViewMap::MapDisplayWinHalfWidth;
	} else if (edgeData->centerX == edgeData->rect_1.right) {
		ViewMap::mapHalfHeight = ViewMap::MapDisplayWinHalfWidth;
	}

	if (edgeData->centerY == edgeData->rect_1.top) {
		ViewMap::mapHalfWidth = -ViewMap::MapDisplayWinHalfHeight;
	} else if (edgeData->centerY == edgeData->rect_1.bottom) {
		ViewMap::mapHalfWidth = ViewMap::MapDisplayWinHalfHeight;
	}

	long cX = edgeData->centerX;
	long cY = edgeData->centerY;
	ViewMap::GetCoord(cX, cY);

	return cX + (cY * 200); // tile of center?
}

// Implementation from HRP by Mash
long EdgeBorder::CheckBorder(long tile) {
	long y, x;
	ViewMap::GetTileCoordOffset(tile, x, y);

	if (x > CurrentMapEdge->rect_1.left   || x < CurrentMapEdge->rect_1.right ||
	    y > CurrentMapEdge->rect_1.bottom || y < CurrentMapEdge->rect_1.top)
	{
		return 0;
	}

	long _mapHalfWidth = ViewMap::mapHalfWidth;
	long _mapHalfHeight = ViewMap::mapHalfHeight;
	ViewMap::mapHalfHeight = 0;
	ViewMap::mapHalfWidth = 0;

	long halfWidth = 0;
	long halfHeight = 0;

	if (x == CurrentMapEdge->rect_1.left) {
		halfWidth = -ViewMap::MapDisplayWinHalfWidth;
		ViewMap::mapHalfHeight = halfWidth;
	} else if (x == CurrentMapEdge->rect_1.right) {
		halfWidth = ViewMap::MapDisplayWinHalfWidth;
		ViewMap::mapHalfHeight = halfWidth;
	}

	if (y == CurrentMapEdge->rect_1.top) {
		halfHeight = -ViewMap::MapDisplayWinHalfHeight;
		ViewMap::mapHalfWidth = halfHeight;
	} else if (y == CurrentMapEdge->rect_1.bottom) {
		halfHeight = ViewMap::MapDisplayWinHalfHeight;
		ViewMap::mapHalfWidth = halfHeight;
	}

	return (_mapHalfHeight != halfWidth || _mapHalfWidth != halfHeight) ? 1 : -1;
}

void EdgeBorder::init() {
	LoadGameHook::OnBeforeMapLoad() += LoadMapEdgeFile;
}

}
