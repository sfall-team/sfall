/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#pragma once

namespace sfall
{

class Image {
public:
	//static void init();

	static long GetAspectSize(long &dW, long &dH, float sW, float sH);

	static void Scale(BYTE* src, long sWight, long sHeight, BYTE* dst, long dWight, long dHeight);
};

}
