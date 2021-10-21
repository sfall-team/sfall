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
	static long GetAspectSize(long &dW, long &dH, float sW, float sH);

	static void Scale(BYTE* src, long sWight, long sHeight, BYTE* dst, long dWight, long dHeight);
	static void TransScale(BYTE* src, long sWight, long sHeight, BYTE* dst, long dWight, long dHeight, long dPitch, long tranColorIdx);
	static void ScaleText(BYTE* dst, const char* text, long txtWidth, long dstWidth, long colorFlags, float scaleFactor);
};

}
