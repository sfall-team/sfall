/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Utils.h"
#include "..\Modules\Graphics.h"

#include "Image.h"

#include "SplashScreen.h"

namespace HRP
{

namespace sf = sfall;

// 0 - image will display at its original size
// 1 - image will stretch to fit the screen while maintaining its aspect ratio
// 2 - image will stretch to fill the screen
long SplashScreen::SPLASH_SCRN_SIZE;

long SplashScreen::SPLASH_SCRN_TIME;

static WORD rixWidth;
static WORD rixHeight;
static BYTE* rixBuffer;
static DWORD splashStartTime;

static void __cdecl game_splash_screen_hack_scr_blit(BYTE* srcPixels, long srcWidth, long srcHeight, long srcX, long srcY, long width, long height, long x, long y) {
	RECT rect;
	long w = Setting::ScreenWidth();
	long h = Setting::ScreenHeight();

	splashStartTime = GetTickCount();

	// TODO: Load an alternative 32-bit BMP image or DirectX texture
	// stretch texture for DirectX

	if (rixBuffer) {
		srcWidth = rixWidth;
		srcHeight = rixHeight;
		srcPixels = rixBuffer;
	}

	if (SplashScreen::SPLASH_SCRN_SIZE || srcWidth > w || srcHeight > h) {
		if (SplashScreen::SPLASH_SCRN_SIZE == 2) {
			rect.top = 0;
			rect.left = 0;
			rect.right = w - 1;
			rect.bottom = h - 1;
		} else {
			x = 0;
			Image::GetAspectSize(srcWidth, srcHeight, &x, &y, w, h);

			rect.top = y;
			rect.bottom = (y + h) - 1;
			rect.left = x;
			rect.right = (rect.left + w) - 1;
		}
		BYTE* resizeBuff = new BYTE[w * h];
		Image::Scale(srcPixels, srcWidth, srcHeight, resizeBuff, w, h);

		sf::Graphics::UpdateDDSurface(resizeBuff, w, h, w, &rect);

		delete[] resizeBuff;
	} else {
		// original size to center screen

		rect.left = ((Setting::ScreenWidth() - srcWidth) / 2) + x;
		rect.right = (rect.left + srcWidth) - 1;

		rect.top = ((Setting::ScreenHeight() - srcHeight) / 2) + y;
		rect.bottom = (rect.top + srcHeight) - 1;

		sf::Graphics::UpdateDDSurface(srcPixels, srcWidth, srcHeight, srcWidth, &rect);
	}
	if (rixBuffer) {
		delete[] rixBuffer;
		rixBuffer = nullptr;
	}
}

// Fixes colored screen border when the index 0 of the palette contains a color with a non-black (zero) value
static void Clear(fo::PALETTE* palette) {
	long index = Image::GetDarkColor(palette);
	if (index != 0) sf::Graphics::BackgroundClearColor(index);
}

static fo::DbFile* __fastcall ReadRIX(fo::DbFile* file, fo::PALETTE* palette) {
	fo::func::db_fseek(file, 4, SEEK_SET);
	fo::func::db_freadShort(file, &rixWidth);
	fo::func::db_freadShort(file, &rixHeight);

	rixWidth = sf::ByteSwapW(rixWidth);
	rixHeight = sf::ByteSwapW(rixHeight);

	if (rixWidth != 640 || rixHeight != 480) {
		size_t size = rixWidth * rixHeight;
		rixBuffer = new BYTE[size];

		fo::func::db_fseek(file, 2 + 768, SEEK_CUR);
		fo::func::db_fread(rixBuffer, 1, size, file);
	}
	Clear(palette);

	return file;
}

static __declspec(naked) void game_splash_screen_hook() {
	__asm {
		mov  ecx, eax; // file
		mov  edx, ebp; // .rix palette
		call ReadRIX;
		jmp  fo::funcoffs::db_fclose_;
	}
}

static void SplashScreenTime() {
	if (SplashScreen::SPLASH_SCRN_TIME >= 1) {
		DWORD time = 1000 * SplashScreen::SPLASH_SCRN_TIME;
		for (DWORD ticks = GetTickCount() - splashStartTime; ticks < time; ticks = GetTickCount() - splashStartTime) {
			Sleep(500);
		}
	}
}

static __declspec(naked) void game_init_hook_init_options_menu() {
	__asm {
		call SplashScreenTime;
		jmp  fo::funcoffs::init_options_menu_;
	}
}

void SplashScreen::init() {
	sf::HookCall(0x4444FC, game_splash_screen_hook);
	sf::MakeCall(0x44451E, game_splash_screen_hack_scr_blit, 1);
	sf::HookCall(0x442B0B, game_init_hook_init_options_menu);
}

}
