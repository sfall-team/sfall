/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#pragma once

namespace HRP
{

class Image {
public:
	static long GetDarkColor(fo::PALETTE* palette);

	static long GetAspectSize(long sW, long sH, long* x, long* y, long &dW, long &dH);

	static void Scale(BYTE* src, long sWight, long sHeight, BYTE* dst, long dWight, long dHeight, long dPitch = 0, long sPitch = 0);
	static void ScaleText(BYTE* dst, const char* text, long txtWidth, long dstWidth, long colorFlags, float scaleFactor);

	static bool MakeBMP(const char* file, BYTE* dataRGB32, long width, long height, long pitch);
};

}
