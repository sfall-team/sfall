/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\Graphics.h"

#include "Image.h"

#include "SplashScreen.h"

namespace sfall
{

// 0 - image will display at its original size
// 1 - image will stretch to fit the screen while maintaining its aspect ratio
// 2 - image will stretch to fill the screen
long SplashScreen::SPLASH_SCRN_SIZE;

static void __cdecl SplashBlit(BYTE* srcPixels, long srcWidth, long srcHeight, long srcX, long srcY, long width, long height, long x, long y) {
	RECT rect;
	long w = Graphics::GetGameWidthRes();
	long h = Graphics::GetGameHeightRes();

	// TODO: Load an alternative 32-bit BMP image or DirectX texture

	// stretch texture for DirectX
	if (SplashScreen::SPLASH_SCRN_SIZE == 1) {
		//float aspect = (float)srcHeight / Graphics::GetGameHeightRes(); // 480/720 = 0.6666 | 768/720 = 1.0666
		//long  aspectWidth = (long)(srcWidth / aspect); // 960

		long offset = Image::GetAspectSize(w, h, (float)srcWidth, (float)srcHeight);

		BYTE* resizeBuff = new BYTE[w * h];
		Image::Scale(srcPixels, srcWidth, srcHeight, resizeBuff, w, h);

		rect.top = 0;
		rect.bottom = h - 1;
		rect.left = offset;
		rect.right = (rect.left + w) - 1;
		Graphics::UpdateDDSurface(resizeBuff, w, h, w, &rect);

		delete[] resizeBuff;
		return;
	} else if (SplashScreen::SPLASH_SCRN_SIZE == 2) {
		BYTE* resizeBuff = new BYTE[Graphics::GetGameWidthRes() * Graphics::GetGameHeightRes()];
		Image::Scale(srcPixels, srcWidth, srcHeight, resizeBuff, Graphics::GetGameWidthRes(), Graphics::GetGameHeightRes());

		rect.top = 0;
		rect.left = 0;
		rect.right = Graphics::GetGameWidthRes() - 1;
		rect.bottom = Graphics::GetGameHeightRes() - 1;
		Graphics::UpdateDDSurface(resizeBuff, Graphics::GetGameWidthRes(), Graphics::GetGameHeightRes(), Graphics::GetGameWidthRes(), &rect);

		delete[] resizeBuff;
		return;
	}

	// original size to center screen

	rect.left = (Graphics::GetGameWidthRes() / 2) - (srcWidth / 2) + x;
	rect.right = rect.left + srcWidth; // -1

	rect.top = (Graphics::GetGameHeightRes() / 2) - (srcHeight / 2) + y;
	rect.bottom = rect.top + srcHeight; // -1

	Graphics::UpdateDDSurface(srcPixels, srcWidth, srcHeight, srcWidth, &rect);
}

void SplashScreen::init() {
	MakeCall(0x44451E, SplashBlit, 1); // game_splash_screen_ hack at scr_blit()
}

}
