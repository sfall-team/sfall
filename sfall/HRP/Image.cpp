/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

long Image::GetAspectSize(long sW, long sH, long* x, long* y, long &dW, long &dH) {
	float sWf = (float)sW;
	float sHf = (float)sH;

	long  width = dW;
	long  height = dH;
	float aspectD = (float)dW / dH;
	float aspectS = sWf / sHf;

	if (aspectD < aspectS) {
		dH = (long)((sHf / sWf) * dW); // set new height
		long cy = (height - dH) / 2;   // shift y-offset center
		if (y) *y = cy;
		return cy * width;
	} else if (aspectD > aspectS) {
		dW = (long)((dH / sHf) * sWf); // set new width
		long cx = (width - dW) / 2;    // shift x-offset center
		if (x) *x = cx;
		return cx;
	}
	return 0;
}

// Resizes src image to dWidth/dHeight size
void Image::Scale(BYTE* src, long sWidth, long sHeight, BYTE* dst, long dWidth, long dHeight, long dPitch, long sPitch) {
	if (dWidth <= 0 || dHeight <= 0) return;

	float xStep = (float)sWidth / dWidth;
	float hStep = (float)sHeight / dHeight;

	if (sPitch <= 0) sPitch = sWidth;
	if (dPitch > 0) dPitch -= dWidth;

	float sy = 0.0f;
	do {
		float sx = 0.0f;
		int x = 0, w = dWidth;
		do {
			*dst++ = *(src + x); // copy pixel
			if (x < sWidth) {
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
				src += (sPitch * (int)sy);
				sy -= (int)sy;
			}
		}
	}
	while (--dHeight);
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
			BYTE* fontPixels = (BYTE*)(*(DWORD*)&font->eUnkArray[8 * charText + 2] + font->field80C);

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

#pragma pack(push, 1)
struct BMPHEADER {
	BITMAPFILEHEADER bFile;
	BITMAPINFOHEADER bInfo;
};
#pragma pack(pop)

// BMP 24-bit
bool Image::MakeBMP(const char* file, BYTE* dataRGB32, long width, long height, long pitch) {
	long bmpExtraSize = width * 3 % 4;
	if (bmpExtraSize != 0) bmpExtraSize = 4 - bmpExtraSize;

	DWORD sizeImage = width * height * 3;
	sizeImage += height * bmpExtraSize;

	BMPHEADER bmpHeader;
	std::memset(&bmpHeader, 0, sizeof(BMPHEADER));

	bmpHeader.bFile.bfType = 'MB';
	bmpHeader.bFile.bfSize = sizeImage + sizeof(BMPHEADER);
	bmpHeader.bFile.bfOffBits = sizeof(BMPHEADER);
	bmpHeader.bInfo.biSize = sizeof(BITMAPINFOHEADER);
	bmpHeader.bInfo.biWidth = width;
	bmpHeader.bInfo.biHeight = 0 - height;
	bmpHeader.bInfo.biPlanes = 1;
	bmpHeader.bInfo.biBitCount = 24;
	bmpHeader.bInfo.biCompression = BI_RGB;
	bmpHeader.bInfo.biSizeImage = sizeImage;

	BYTE* bmpImageData = new BYTE[sizeImage];
	BYTE* dData = bmpImageData;

	// 32-bit to 24-bit
	for (long h = 0; h < height; h++) {
		BYTE* sData = dataRGB32;
		for (long w = 0; w < width; w++) {
			*dData++ = *sData++;
			*dData++ = *sData++;
			*dData++ = *sData++;
			sData++;
		}
		dataRGB32 += pitch;
		dData += bmpExtraSize;
	}

	HANDLE hFile = CreateFileA(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	bool result = (hFile != INVALID_HANDLE_VALUE);

	if (result) {
		DWORD dwWritten;
		WriteFile(hFile, &bmpHeader, sizeof(BMPHEADER), &dwWritten, 0);
		WriteFile(hFile, bmpImageData, sizeImage, &dwWritten, 0);
		CloseHandle(hFile);
	}
	delete[] bmpImageData;

	return result;
}

}
