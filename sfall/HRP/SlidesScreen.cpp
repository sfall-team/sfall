/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Image.h"

#include "SlidesScreen.h"

namespace HRP
{

namespace sf = sfall;

// 0 - background image will display at its original size
// 1 - background image will stretch to fit the screen while maintaining its aspect ratio
// 2 - background image will stretch to fill the screen
long SlidesScreen::END_SLIDE_SIZE;

static fo::FrmData* frmArt;
static long artID;
static long color = -1;
static long textLine;
static long bottomPos;
static long panDesert;

static __declspec(naked) void endgame_display_image_hook_art_frame_data() {
	__asm {
		mov  panDesert, 0;
		mov  color, -1;
		mov  artID, esi;
		mov  frmArt, eax;
		jmp  fo::funcoffs::art_frame_data_;
	}
}

static void __cdecl endgame_display_image_hook_buf_to_buf(BYTE* src, long w, long h, long srcW, BYTE* dst, long dstW) {
	textLine = 0;
	long width = frmArt->frame.width;
	long height = frmArt->frame.height;

	w = Setting::ScreenWidth();
	h = Setting::ScreenHeight();

	if (color == -1) {
		fo::func::endgame_load_palette(fo::ArtType::OBJ_TYPE_INTRFACE, artID & 0xFFF);
		color = Image::GetDarkColor((fo::PALETTE*)FO_VAR_cmap);
	}
	if (SlidesScreen::END_SLIDE_SIZE != 2) std::memset(dst, color, w * h);

	if (SlidesScreen::END_SLIDE_SIZE || width > w || height > h) {
		if (SlidesScreen::END_SLIDE_SIZE <= 1) {
			long x = 0;
			long y = 0;
			Image::GetAspectSize(width, height, &x, &y, w, h);
			if (x || y) dst += x + (y * Setting::ScreenWidth());
		}
		bottomPos = h;

		Image::Scale(src, width, height, dst, w, h, Setting::ScreenWidth());
	} else {
		bottomPos = height;
		long y = (h - height) / 2;
		long x = (w - width) / 2;
		if (x || y) {
			bottomPos += y;
			dst += x + (y * Setting::ScreenWidth());
		}
		fo::func::buf_to_buf(src, width, height, width, dst, Setting::ScreenWidth());
	}
}

static __declspec(naked) void endgame_display_image_hook_buf_to_buf_loop() {
	static long tmp;
	__asm {
		pop  ebp; // ret addr
		mov  tmp, edx;
		call endgame_display_image_hook_buf_to_buf;
		mov  edx, tmp;
		jmp  ebp;
	}
}

// rectangle area for printing text
static sf::Rectangle darkRect;

static long __fastcall DarkRectangle(long tH, long tW, short totalLines) {
	totalLines--; // count text lines
	long y = Setting::ScreenHeight() - (totalLines * tH) - 20;
	long x = (Setting::ScreenWidth() - tW) / 2; // tW - max 540

	if (textLine == 0) {
		if (panDesert <= 1) {
			darkRect.x = x - 10;
			darkRect.y = y - 5;
			darkRect.width = tW + 20;
			darkRect.height = (totalLines * (tH + 2)) + 6;
		}
		if (y <= bottomPos) {
			fo::util::TranslucentDarkFill((BYTE*)fo::var::getInt(FO_VAR_endgame_window_buffer), darkRect.x, darkRect.y, darkRect.width, darkRect.height, Setting::ScreenWidth());
		} else if (panDesert) { // correct the overlapping of text when the text is located below the image
			panDesert = 2;
			fo::util::FillRect((BYTE*)fo::var::getInt(FO_VAR_endgame_window_buffer), darkRect.x, darkRect.y, darkRect.width, darkRect.height, Setting::ScreenWidth(), (BYTE)color);
		}
	}
	return x + ((y + (tH * textLine++)) * Setting::ScreenWidth()); // text print offset
}

static __declspec(naked) void endgame_show_subtitles_hook_buf_fill() {
	__asm {
		push [esp + 0xBC - 0x20 + 8]; // count lines
		mov  ecx, ebx;
		call DarkRectangle;
		mov  [esp + 0xBC - 0x34 + 8], eax; // replace x_offset
		mov  dword ptr [esp + 0xBC - 0x38 + 8], 0; // y_offset
		retn 4;
	}
}

static void __fastcall endgame_pan_desert_hook_buf_fill(long, long, fo::FrmData* frm) {
	frmArt = frm;
	fo::func::endgame_load_palette(fo::ArtType::OBJ_TYPE_INTRFACE, 327); // panning desert image (DP.FRM)
	if (SlidesScreen::END_SLIDE_SIZE != 2) {
		color = Image::GetDarkColor((fo::PALETTE*)FO_VAR_cmap);
		std::memset((void*)fo::var::getInt(FO_VAR_endgame_window_buffer), color, Setting::ScreenWidth() * Setting::ScreenHeight());
	}
	panDesert = 1;
}

static void __cdecl endgame_pan_desert_hook_buf_to_buf(BYTE* src, long w, long h, long srcW, BYTE* dst, long dstW) {
	textLine = 0;
	h = frmArt->frame.height; // h: default 480

	if (SlidesScreen::END_SLIDE_SIZE == 1) {
		long width = Setting::ScreenWidth();
		long height = Setting::ScreenHeight();
		long x = 0;
		long y = 0;
		Image::GetAspectSize(w, h, &x, &y, width, height);
		if (x || y) dst += x + (y * Setting::ScreenWidth());
		bottomPos = height;

		Image::Scale(src, w, h, dst, width, height, Setting::ScreenWidth(), srcW);

	} else if (SlidesScreen::END_SLIDE_SIZE == 2) {
		bottomPos = Setting::ScreenHeight();
		Image::Scale(src, w, h, dst, Setting::ScreenWidth(), Setting::ScreenHeight(), 0, srcW);
	} else {
		bottomPos = h;
		long y = (Setting::ScreenHeight() - h) / 2;
		long x = (Setting::ScreenWidth() - w) / 2;
		if (x || y) {
			bottomPos += y;
			dst += x + (y * Setting::ScreenWidth());
		}
		fo::func::buf_to_buf(src, w, h, srcW, dst, Setting::ScreenWidth());
	}
}

void SlidesScreen::init() {
	// endgame_init_
	sf::SafeWrite32(0x43FA01, Setting::ScreenHeight());
	sf::SafeWrite32(0x43FA14, Setting::ScreenWidth());
	// endgame_show_subtitles_
	sf::SafeWrite32(0x44065A, Setting::ScreenWidth());

	sf::HookCall(0x440023, endgame_display_image_hook_art_frame_data);
	sf::HookCall(0x44004E, endgame_display_image_hook_buf_to_buf);
	sf::HookCall(0x440129, endgame_display_image_hook_buf_to_buf_loop);
	sf::HookCall(0x440639, endgame_show_subtitles_hook_buf_fill);

	sf::HookCall(0x43FDE3, endgame_pan_desert_hook_buf_to_buf);
	sf::HookCall(0x43FC80, endgame_pan_desert_hook_buf_fill);
	sf::SafeWrite8(0x43FC75, 0x56); //push eax > push esi

	// block endgame_load_palette_
	sf::BlockCall(0x440072); // endgame_display_image_
	sf::BlockCall(0x43FC94); // endgame_pan_desert_
}

}
