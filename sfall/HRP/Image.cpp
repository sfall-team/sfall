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
	float width = (float)dW;
	float height = (float)dH;
	float aspectD = (float)dW / dH; // 1280/720 = 1.77
	float aspectS = sW / sH;        // 640/480 = 1.33

	if (aspectD < aspectS) { // d960/720 = 1.3333 | s1280/720 = 1.77
		dH = (long)((sH / sW) * dW); // set new height  720 / 1280 = 0,5625 * 960 = 540 (960x540)
		return (long)(((height - dH) / 2) * dW); // shift y-offset center: 720 - 540 / 2 =
	} else if (aspectD > aspectS) {
		dW = (long)((dH / sH) * sW); // set new wight  (720 / 480) * 640 = 960 (960x720)
		return (long)((width - dW) / 2); // shift x-offset center: 1280 - 959 / 2 = 160
	}
	return 0;
}

// Resizes src image to dWidth/dHeight size
void Image::Scale(BYTE* src, long sWidth, long sHeight, BYTE* dst, long dWidth, long dHeight) {
	if (dWidth <= 0 || dHeight <= 0) return;

	float sw = (float)sWidth;
	float sh = (float)sHeight;
	float xStep = sw / dWidth;
	float hStep = sh / dHeight;

	float sy = 0.0f;
	do {
		float sx = 0.0f;
		int w = dWidth;
		do {
			*dst++ = *(src + std::lround(sx)); // copy pixel
			sx += xStep;
			if (sx >= sw) sx = sw - 1.0f;
		} while (--w);

		if (sHeight) {
			sy += hStep;
			float y = std::round(sy);
			if (y >= 1.0f && --sHeight) {
				src += sWidth * (int)y;
				sy -= y;
			}
		}
	}
	while (--dHeight);
}

// Fixme
void Image::TransScale(BYTE* src, long sWidth, long sHeight, BYTE* dst, long dWigth, long dHeight, long dPitch, long tranColorIdx) {
	if (dWigth <= 0 || dHeight <= 0) return;

	float sw = (float)sWidth;
	float sh = (float)sHeight;
	float xStep = sw / dWigth;
	float hStep = sh / dHeight;

	long height = dHeight;
	dPitch -= dWigth;

	float sy = 0.0f;
	do {
		float sx = 0.0f;
		int w = dWigth;
		do {
			BYTE pixel = *(src + std::lround(sx));
			if (pixel != tranColorIdx) *dst = pixel;
			dst++;
			sx += xStep;
			if (sx >= sw) sx = sw - 1.0f;
		} while (--w);

		if (sHeight) {
			sy += hStep;
			float y = std::round(sy);
			if (y >= 1.0f && --sHeight) {
				src += sWidth * (int)y;
				sy -= y;
			}
		}
		dst += dPitch;
	}
	while (--height);
}

// Simplified implementation of FMtext_to_buf_ engine function with text scaling
void Image::ScaleText(BYTE* dstSurf, const char* text, long txtWidth, long dstWidth, long colorFlags, float scaleFactor) {
	if (*text) {
		if (scaleFactor <= 0.0f) scaleFactor = 1.0f;

		txtWidth = std::lround(txtWidth * scaleFactor);
		float step = 1.0f / scaleFactor;

		fo::BlendColorTableData* blendTable = fo::func::getColorBlendTable(colorFlags);
		fo::FontData* font = (fo::FontData*)fo::var::getInt(FO_VAR_gCurrentFont);

		BYTE* surface = dstSurf;
		do {
			unsigned char charText = *text;
			signed int fWidth;
			if (charText == ' ') {
				fWidth = *(DWORD*)&font->field2;
			} else {
				fWidth = *((DWORD*)&font->fieldA + 2 * charText);
			}
			fWidth >>= 16; // div 65536

			int fontWidth = std::lround(fWidth * scaleFactor);

			BYTE* _surface = &surface[fontWidth] + (*(DWORD*)&font->field0 >> 16); // position of the next character
			if ((_surface - dstSurf) > txtWidth) break; // something went wrong (out of size)

			int fontHeight = *((DWORD*)&font->fieldC + 2 * charText) >> 16;

			BYTE* dstPixels = &surface[dstWidth * (font->field0 - fontHeight)]; //
			BYTE* fontPixels = (BYTE*) (*(DWORD*)&font->eUnkArray[8 * charText + 2] + font->field80C);

			fontHeight = std::lround(fontHeight * scaleFactor);
			float fh = 0.0f;

			// copy character pixels
			for (int h = 0; h < fontHeight; h++) {
				float fw = 0.0f;
				for (int w = 0; w < fontWidth; w++) {
					// replace the pixel of the image with the color from the blending table
					*dstPixels++ = blendTable->data[*fontPixels].colors[*dstPixels]; // pixel of the color index

					fw += step;
					if (fw >= 1.0f) {
						fw -= 1.0f;
						fontPixels++;
					}
				}
				fh += step;
				if (fh < 1.0f) {
					fontPixels -= fWidth;
				} else {
					fh -= 1.0f;
				}
				dstPixels += dstWidth - fontWidth;
			}
			surface = _surface;

			++text;
		} while (*text);

		fo::func::freeColorBlendTable(colorFlags);
	}
}

}
