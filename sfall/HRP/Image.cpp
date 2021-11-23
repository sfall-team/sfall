/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\FalloutEngine\Fallout2.h"

#include "Image.h"

namespace HRP
{

long Image::GetDarkColor(fo::PALETTE* palette) {
	long minValue = (palette->B + palette->G + palette->R);
	if (minValue == 0) return 0;

	long index = 0;

	// search index of the darkest color in the palette
	for (size_t i = 1; i < 256; i++) {
		long rgbVal = palette[i].B + palette[i].G + palette[i].R;
		if (rgbVal < minValue) {
			minValue = rgbVal;
			index = i;
		}
	}
	return index;
}

long Image::GetAspectSize(long &dW, long &dH, float sW, float sH) {
	float width = (float)dW;
	float height = (float)dH;
	float aspectD = (float)dW / dH;
	float aspectS = sW / sH;

	if (aspectD < aspectS) {
		dH = (long)((sH / sW) * dW);             // set new height
		return (long)(((height - dH) / 2) * dW); // shift y-offset center
	} else if (aspectD > aspectS) {
		dW = (long)((dH / sH) * sW);     // set new width
		return (long)((width - dW) / 2); // shift x-offset center
	}
	return 0;
}

// Resizes src image to dWidth/dHeight size
void Image::Scale(BYTE* src, long sWidth, long sHeight, BYTE* dst, long dWidth, long dHeight, long dPitch, long sPitch) {
	if (dWidth <= 0 || dHeight <= 0) return;

	float sw = (float)sWidth;
	float sh = (float)sHeight;
	float xStep = sw / dWidth;
	float hStep = sh / dHeight;

	if (sPitch > 0) sWidth = sPitch;
	if (dPitch > 0) dPitch -= dWidth;

	float sy = 0.0f;
	do {
		float sx = 0.0f;
		int x = 0, w = dWidth;
		do {
			*dst++ = *(src + x); // copy pixel
			if (x < sw) {
				sx += xStep;
				if (sx >= 1.0f) {
					x += (int)sx;
					sx -= (int)sx;
				}
			}
		} while (--w);
		dst += dPitch;

		if (sHeight) {
			sy += hStep;
			if (sy >= 1.0f && --sHeight) {
				src += (sWidth * (int)sy);
				sy -= (int)sy;
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
		if (scaleFactor < 0.5f) scaleFactor = 0.5f;

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

			BYTE* _surface = &surface[fontWidth] + (*((DWORD*)&font->field0) >> 16); // position of the next character
			if ((_surface - dstSurf) > txtWidth) break; // something went wrong (out of size)

			int fontHeight = *((DWORD*)&font->fieldC + 2 * charText) >> 16;

			BYTE* dstPixels = &surface[dstWidth * (font->field0 - fontHeight)]; //
			BYTE* fontPixels = (BYTE*) (*(DWORD*)&font->eUnkArray[8 * charText + 2] + font->field80C);

			fontHeight = std::lround(fontHeight * scaleFactor);
			float fh = 0.0f;

			// copy character pixels
			for (int h = 0; h < fontHeight; h++) {
				float fw = 0.0f;
				int count = 0;
				for (int w = 0; w < fontWidth; w++) {
					// replace the pixel of the image with the color from the blending table
					*dstPixels++ = blendTable->data[*fontPixels].colors[*dstPixels]; // pixel of the color index

					fw += step;
					if (fw >= 1.0f) {
						fw -= 1.0f;
						fontPixels++;
						count++;
					}
				}
				fh += step;
				if (fh < 1.0f) {
					fontPixels -= count;
				} else {
					fh -= 1.0f;
					fontPixels += (fWidth - count);
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