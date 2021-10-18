/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\FalloutEngine\Fallout2.h"

#include "Image.h"

namespace sfall
{

long Image::GetAspectSize(long &dW, long &dH, float sW, float sH) {
	float wight = (float)dW;
	float height = (float)dH;
	float aspectD = (float)dW / dH; // 1280/720 = 1.77
	float aspectS = sW / sH;        // 640/480 = 1.33

	if (aspectD < aspectS) { // d960/720 = 1.3333 | s1280/720 = 1.77
		dH = (long)((sH / sW) * dW); // set new height  720 / 1280 = 0,5625 * 960 = 540 (960x540)
		return (long)(((height - dH) / 2) * dW); // shift y-offset center: 720 - 540 / 2 =
	}
	else if (aspectD > aspectS) {
		dW = (long)((dH / sH) * sW); // set new wight  (720 / 480) * 640 = 960 (960x720)
		return (long)((wight - dW) / 2); // shift x-offset center: 1280 - 959 / 2 = 160
	}
	return 0;
}

// Resizes the source image to dWight/dHeight
void Image::Scale(BYTE* src, long sWight, long sHeight, BYTE* dst, long dWight, long dHeight) {
	if (dWight <= 0 || dHeight <= 0) return;

	float sw = (float)sWight;
	float sh = (float)sHeight;
	float xStep = sw / dWight;
	float hStep = sh / dHeight;

	float sy = 0.0f;
	do {
		float sx = 0.0f;
		int w = dWight;
		do {
			*dst++ = *(src + std::lround(sx)); // copy pixel
			sx += xStep;
			if (sx >= sw) sx = sw - 1.0f;
		} while (--w);

		if (sHeight) {
			sy += hStep;
			float y = std::round(sy);
			if (y >= 1.0f && --sHeight) {
				src += sWight * (int)y;
				sy -= y;
			}
		}
	}
	while (--dHeight);
}

}
