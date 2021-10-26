/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"

#include "ViewMap.h"

#include "EdgeBorder.h"

namespace sfall
{

static struct Edge { // sizeof = 0x54
	long centerX;
	long centerY;
	RECT rect_1;
	RECT rect_2;
	RECT tileRect_3;
	RECT rect_4;
	long field_48;
	Edge* nextEdgeData_4C; // unused? (used in 3.06)
	Edge* nextEdgeData_50;

	// make a destructor

} *MapEdgeData;

long EdgeVersion;
long isLoadingMapEdge;

// Implementation from HRP by Mash
static void CalcEdgeData(Edge* edgeData, long w, long h) {
	long x = 0, y = 0;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.left, x, y);
	edgeData->rect_1.left = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.right, x, y);
	edgeData->rect_1.right = x;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.top, x, y);
	edgeData->rect_1.top = y;

	ViewMap::GetTileCoordOffset(edgeData->tileRect_3.bottom, x, y);
	edgeData->rect_1.bottom = y;

	long mapWinW = (fo::var::getInt(FO_VAR_buf_width_2) / 2) - 1;  // 1280/2 -1 = 639 (320)
	long mapWinH = (fo::var::getInt(FO_VAR_buf_length_2) / 2) - 1; // 720/2 -1 = 359 (240)

	edgeData->rect_2.left   = edgeData->rect_1.left   - mapWinW;
	edgeData->rect_2.right  = edgeData->rect_1.right  - mapWinW;
	edgeData->rect_2.top    = edgeData->rect_1.top    + mapWinH;
	edgeData->rect_2.bottom = edgeData->rect_1.bottom + mapWinH;

	long v13 = (edgeData->rect_1.left - edgeData->rect_1.right) / 2;
	long v14 = v13;
	if (v13 & 31) {
		v13 &= ~31; // truncate
		v14 = v13 + 32;
	}
	long left, right;
	if (v13 < w) {
		left = edgeData->rect_1.left - v13;
		right = v14 + edgeData->rect_1.right;
	} else {
		left = edgeData->rect_1.left - w;
		right = w + edgeData->rect_1.right;
	}
	edgeData->rect_1.right = right;
	edgeData->rect_1.left = left;

	long v19 = (edgeData->rect_1.bottom - edgeData->rect_1.top) / 2;
	long v20 = v19;
	if (v19 % 24) {
		v19 -= v19 % 24; // truncate
		v20 = v19 + 24;
	}
	long top, bottom;
	if (v19 < h) {
		top = v20 + edgeData->rect_1.top;
		bottom = edgeData->rect_1.bottom - v19;
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

	// R2000, L1000 : 1000-2000 = -1000/2 = -500+2000 = 1500 not an error here?
	x = edgeData->rect_1.right + ((edgeData->rect_1.left - edgeData->rect_1.right) / 2);
	edgeData->centerX = x & ~31;

	y = edgeData->rect_1.top + ((edgeData->rect_1.bottom - edgeData->rect_1.top) / 2);
	edgeData->centerY = y - (y % 24);
}

static void SetDefaultEdgeData() {
	long w = 0, h = 0;
	ViewMap::GetMapWindowSize(w, h);

	if (MapEdgeData == nullptr) MapEdgeData = new Edge[3];

	for (size_t i = 0; i < 3; i++) {
		Edge* edge = &MapEdgeData[i];

		edge->tileRect_3.left = 199;
		edge->tileRect_3.top = 0;
		edge->tileRect_3.right = 39800;
		edge->tileRect_3.bottom = 39999;
		//edge->nextEdgeData_50 = nullptr;

		CalcEdgeData(edge, w, h);

		edge->rect_4.left = 99;
		edge->rect_4.top = 0;
		edge->rect_4.right = 0;
		edge->rect_4.bottom = 99;

		edge->field_48 = 0;
		edge->nextEdgeData_50 = nullptr;
	}
	EdgeVersion = 0;
	//isDefaultSetEdge = 1;
}

// Implementation from HRP by Mash
static fo::DbFile* LoadMapEdgeFileSub(char* mapName) {
	char mapPath[32];

	char* posDot = std::strchr(mapName, '.');
	*posDot = '\0';
	std::sprintf(mapPath, "maps\\%s.edg", mapName);
	*posDot = '.';

	fo::DbFile* file = fo::func::db_fopen(mapPath, "rb");
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

	// get count?
	getValue = 0;
	if (fo::func::db_freadInt(file, &getValue) || getValue) return file;

	long w = 0, h = 0;
	ViewMap::GetMapWindowSize(w, h);

	if (MapEdgeData) {
		//delete[] MapEdgeData;
		// release nested data
	} else {
		MapEdgeData = new Edge[3];
	}

	long mapLevel = 0;
	do {
		Edge* edgeData = &MapEdgeData[mapLevel];

		if (EdgeVersion) {
			// loading a rectangle?
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
				// loading a rectangle?
				long result = fo::func::db_freadIntCount(file, (DWORD*)&edgeData->tileRect_3, 4);
				if (result != 0) return file; // read error

				CalcEdgeData(edgeData, w, h);

				if (fo::func::db_freadInt(file, &getValue)) {
					// read error
					if (mapLevel != 2) return file;

					getValue = -1;
					break; // next level
				}

				if (getValue == mapLevel) {
					Edge *edge = new Edge;
					edge->nextEdgeData_50 = nullptr;
					edge->rect_4 = edgeData->rect_4; // copy
					edgeData->nextEdgeData_50 = edge;
					edgeData = edge;
					continue; // next read
				}
				break; // next level
			}
		}
	} while (++mapLevel < 3);

	fo::func::db_fclose(file);
	return 0;
}

static void __fastcall LoadMapEdgeFile(char* mapName) {
	isLoadingMapEdge = 0;

	fo::DbFile* file = LoadMapEdgeFileSub(mapName);
	if (file) { // load error
		fo::func::db_fclose(file);
		SetDefaultEdgeData();
	}
}

// open map file
static void __declspec(naked) map_load_hook_db_fopen() {
	__asm {
		push ecx;
		mov  ecx, eax; // mapName
		call fo::funcoffs::db_fopen_;
		test eax, eax;
		jz   fail;
		push eax;
		call LoadMapEdgeFile;
		pop  eax;
fail:
		pop  ecx;
		retn;
	}
}

void EdgeBorder::init() {
	//HookCall(0x482AE1, map_load_hook_db_fopen);
}

}
