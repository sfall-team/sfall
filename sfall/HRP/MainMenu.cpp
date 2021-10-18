/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\Graphics.h"
#include "..\Modules\MainMenu.h"

#include "Init.h"
#include "Image.h"

#include "MainMenu.h"

namespace sfall
{

// 0 - main-menu image will display at its original size
// 1 - main-menu image will stretch to fit the screen while maintaining its aspect ratio
// 2 - main-menu image will stretch to fill the screen
long MainMenuScreen::MAIN_MENU_SIZE;

bool MainMenuScreen::USE_HIRES_IMAGES;
bool MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU;
long MainMenuScreen::MENU_BG_OFFSET_X = 30;
long MainMenuScreen::MENU_BG_OFFSET_Y = 20;

static fo::UnlistedFrm* mainBackgroundFrm;
static fo::UnlistedFrm* btnBackgroundFrm;

static long mainmenuWidth = 640;

static void __cdecl main_menu_create_hook_buf_to_buf(BYTE* src, long w, long h, long srcW, BYTE* dst, long dstW) {
	if (mainBackgroundFrm) {
		src = mainBackgroundFrm->frames->indexBuff;

		fo::Window* win = fo::var::window[fo::var::main_window];
		h = win->height;
		w = win->width;
		dstW = w;
	}

	if (MainMenuScreen::MAIN_MENU_SIZE == 1) {
		//
	} else if (MainMenuScreen::MAIN_MENU_SIZE == 2) {
		Image::Scale(src, w, h, dst, HRP::ScreenWidth(), HRP::ScreenHeight());
	} else {
		fo::func::buf_to_buf(src, w, h, w, dst, dstW); // direct copy
	}

	if (btnBackgroundFrm) {
		h = btnBackgroundFrm->frames->height;
		w = btnBackgroundFrm->frames->width;
		long offset = (MainMenuScreen::MENU_BG_OFFSET_Y * dstW) + MainMenuScreen::MENU_BG_OFFSET_X;
		fo::func::trans_buf_to_buf(btnBackgroundFrm->frames->indexBuff, w, h, w, dst + offset, dstW); // direct copy
	}
}

static long __fastcall main_menu_create_hook_add_win(long h, long y, long color, long flags) {
	long x = 0;
	long w = 640;

	if (MainMenuScreen::USE_HIRES_IMAGES) {
		if (!mainBackgroundFrm) {
			mainBackgroundFrm = fo::util::LoadUnlistedFrm("HR_MAINMENU.frm", fo::ArtType::OBJ_TYPE_INTRFACE);
			btnBackgroundFrm = fo::util::LoadUnlistedFrm("HR_MENU_BG.frm", fo::ArtType::OBJ_TYPE_INTRFACE);
		}
		h = mainBackgroundFrm->frames->height;
		w = mainBackgroundFrm->frames->width;

		if (w > HRP::ScreenWidth()) w = HRP::ScreenWidth();
		if (h > HRP::ScreenHeight()) h = HRP::ScreenHeight();

		mainmenuWidth = w;

		MainMenu::mTextOffset = MainMenu::mXOffset;
		if (MainMenu::mYOffset) MainMenu::mTextOffset += (MainMenu::mYOffset * w);
	}

	if (MainMenuScreen::MAIN_MENU_SIZE == 1) {


	} else if (MainMenuScreen::MAIN_MENU_SIZE == 2) {
		w = HRP::ScreenWidth();
		h = HRP::ScreenHeight();
	} else {
		x += (HRP::ScreenWidth() / 2) - (w / 2);
		y += (HRP::ScreenHeight() / 2) - (h / 2);
	}
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	return fo::func::win_add(x, y, w, h, color, flags | fo::WinFlags::DontMoveTop);
}

static long __fastcall GetHeightOffset(long &y) {
	long h = fo::var::window[fo::var::main_window]->height;
	y = ((y - 460) - 20) + h;
	if (y > h) {
		y = h - 10;
	} else if (y < 0) {
		y = 0;
	}
	return fo::var::main_window;
}

// credit text print
static void __declspec(naked) main_menu_create_hook_win_print() {
	__asm {
		mov  ebx, ecx;       // x offset
		lea  ecx, [esp + 4]; // y offset
		push edx;
		call GetHeightOffset;
		pop  edx;
		mov  ecx, ebx;
		xor  ebx, ebx;
		jmp  fo::funcoffs::win_print_;
	}
}

// buttons text print
static void __declspec(naked) main_menu_create_hook_text_to_buf() {
	__asm { // eax:xOffset, ebp:yPos, edx:text, ebx:(640-xOffset)-1
		add  ebx, ecx; // +640
		mov  ecx, mainmenuWidth;
		imul ebp, ecx; // yPos * width
		add  eax, ds:[FO_VAR_main_window_buf];
		sub  ebx, ecx; // -width
		add  eax, MainMenu::mTextOffset;
		add  eax, ebp;
		jmp  dword ptr ds:[FO_VAR_text_to_buf];
	}
}

void MainMenuScreen::init() {
	if (MENU_BG_OFFSET_X < 0) MENU_BG_OFFSET_X = 0;
	if (MENU_BG_OFFSET_Y < 0) MENU_BG_OFFSET_Y = 0;

	// Main menu window size
	//SafeWrite32(0x481674, HRP::ScreenHeight());
	//SafeWrite32(0x48167A, HRP::ScreenWidth());

	HookCall(0x481680, main_menu_create_hook_add_win);
	HookCall(0x481704, main_menu_create_hook_buf_to_buf);
	HookCall(0x481767, main_menu_create_hook_win_print);
	MakeCall(0x481933, main_menu_create_hook_text_to_buf, 1);

	// imul ebp, edx, 640 -> mov ebp, edx
	SafeWrite16(0x481912, 0xD589);
	SafeWrite32(0x481914, 0x90909090);

	SafeWrite16(0x48192D, 0x9090);
}

}
