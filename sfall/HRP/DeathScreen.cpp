/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Image.h"

#include "DeathScreen.h"

namespace HRP
{

namespace sf = sfall;

// 0 - background image will display at its original size
// 1 - background image will stretch to fit the screen while maintaining its aspect ratio
// 2 - background image will stretch to fill the screen
long DeathScreen::DEATH_SCRN_SIZE;

static long __fastcall main_death_scene_hook_win_add(long height, long y, long color, long flags) {
	__asm mov  eax, 0x50AF9C; // art\intrface\death.pal
	__asm call fo::funcoffs::loadColorTable_;

	color = Image::GetDarkColor((fo::PALETTE*)FO_VAR_cmap);
	return fo::func::win_add(0, 0, Setting::ScreenWidth(), Setting::ScreenHeight(), color, flags);
}

static BYTE* winSurface;
static long yPosition; // bottom position

static void __cdecl main_death_scene_hook_buf_to_buf(fo::FrmData* frm, long w, long h, long srcW, BYTE* dst, long dstW) {
	winSurface = dst;

	long width = frm->frame.width;
	long height = frm->frame.height;

	w = Setting::ScreenWidth();
	h = Setting::ScreenHeight();

	if (DeathScreen::DEATH_SCRN_SIZE || width > w || height > h) {
		if (DeathScreen::DEATH_SCRN_SIZE <= 1) {
			long x = 0;
			long y = 0;
			Image::GetAspectSize(width, height, &x, &y, w, h);
			if (x || y) {
				dst += x + (y * Setting::ScreenWidth());
			}
		}
		yPosition = h;

		Image::Scale(frm->frame.data, width, height, dst, w, h, Setting::ScreenWidth());
	} else {
		yPosition = height;

		long y = (h - height) / 2;
		long x = (w - width) / 2;
		if (x || y) {
			yPosition += y;
			dst += x + (y * Setting::ScreenWidth());
		}
		fo::func::buf_to_buf(frm->frame.data, width, height, width, dst, Setting::ScreenWidth());
	}
}

// Darkens a rectangle area for printing text
static BYTE* __fastcall DarkRectangle(long totalLines, long w) { // w:564
	long tHeight = fo::util::GetTextHeight();
	long y = (Setting::ScreenHeight() - (totalLines * tHeight) - 20);
	long x = (Setting::ScreenWidth() / 2) - (560 / 2);

	if (y <= yPosition) {
		fo::util::TranslucentDarkFill(winSurface, x - 4, y - 5, w, (totalLines * (tHeight + 2)) + 6, Setting::ScreenWidth());
	}
	long yOffset = Setting::ScreenWidth() * y;
	return winSurface + x + yOffset; // text print offset
}

static __declspec(naked) void main_death_scene_hook_buf_fill() {
	__asm {
		mov  ecx, [esp + 0x2B4 - 0x1C + 4]; // totalLines
		call DarkRectangle;
		mov  esi, eax;
		retn 4;
	}
}

void DeathScreen::init() {
	sf::HookCall(0x4811E1, main_death_scene_hook_win_add);
	sf::HookCall(0x481269, main_death_scene_hook_buf_to_buf);
	sf::HookCall(0x481353, main_death_scene_hook_buf_fill);

	// main_death_scene_
	sf::SafeWrite32(0x48135D, Setting::ScreenWidth()); // ToWidth
	sf::SafeWrite32(0x48138E, Setting::ScreenWidth()); // imul

	// main_death_scene_
	sf::HookCall(0x481225, (void*)fo::funcoffs::art_ptr_lock_); // replace art_ptr_lock_data_ with art_ptr_lock_
	sf::SafeWrite16(0x481223, 0xCA89); // mov edx, ecx

	// block loadColorTable_
	sf::BlockCall(0x4813C1); // main_death_scene_
}

}
