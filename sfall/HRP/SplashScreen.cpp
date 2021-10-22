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

static void __cdecl game_splash_screen_hook_scr_blit(BYTE* srcPixels, long srcWidth, long srcHeight, long srcX, long srcY, long width, long height, long x, long y) {
	RECT rect;
	long w = Graphics::GetGameWidthRes();
	long h = Graphics::GetGameHeightRes();

	// TODO: Load an alternative 32-bit BMP image or DirectX texture
	// stretch texture for DirectX
	if (SplashScreen::SPLASH_SCRN_SIZE == 1) {
		x = Image::GetAspectSize(w, h, (float)srcWidth, (float)srcHeight);

		BYTE* resizeBuff = new BYTE[w * h];
		Image::Scale(srcPixels, srcWidth, srcHeight, resizeBuff, w, h);

		// extract x/y image position
		if (x >= w) {
			y = x / w;
			x -= y * w;
		}

		rect.top = y;
		rect.bottom = (y + h) - 1;
		rect.left = x;
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
	rect.right = (rect.left + srcWidth) - 1 ;

	rect.top = (Graphics::GetGameHeightRes() / 2) - (srcHeight / 2) + y;
	rect.bottom = (rect.top + srcHeight) - 1;

	Graphics::UpdateDDSurface(srcPixels, srcWidth, srcHeight, srcWidth, &rect);
}

void SplashScreen::init() {

	MakeCall(0x44451E, game_splash_screen_hook_scr_blit, 1);
}

}
