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
	struct Edge {
		POINT center;    // x/y center of current map screen?
		RECT borderRect; // right is less than left
		RECT rect_2;
		RECT tileRect;
		RECT squareRect;
		long field_48;      // unknown
		Edge* prevEdgeData; // unused (used in 3.06)
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
	};

	static void init();

	static Edge* CurrentMapEdge();
	static long EdgeVersion();

	static long GetCenterTile(long tile, long mapLevel);
	static long CheckBorder(long tile);
};

}
