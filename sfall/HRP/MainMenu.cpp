/*
 *    sfall
 *    Copyright (C) 2008-2021  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "..\Modules\LoadGameHook.h"
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
long MainMenuScreen::MENU_BG_OFFSET_X = 29;
long MainMenuScreen::MENU_BG_OFFSET_Y = 19;

static fo::UnlistedFrm* mainBackgroundFrm;
static fo::UnlistedFrm* btnBackgroundFrm;

static long mainmenuWidth = 640;

// draw image to main menu window
static void __cdecl main_menu_create_hook_buf_to_buf(BYTE* src, long w, long h, long srcW, BYTE* dst, long dstW) {
	long sw = w;
	long sh = h;

	fo::Window* win = fo::var::window[fo::var::main_window]; // the window size is always equal to the scaled image
	h = win->height;
	w = win->width;
	dstW = w;

	if (mainBackgroundFrm) {
		src = mainBackgroundFrm->frames->indexBuff;
		sh = mainBackgroundFrm->frames->height;
		sw = mainBackgroundFrm->frames->width;
	}

	bool stretch = (MainMenuScreen::MAIN_MENU_SIZE == 1 || MainMenuScreen::MAIN_MENU_SIZE == 2);
	if (!stretch) stretch = (sw != w) || (sh != h); // for MAIN_MENU_SIZE = 0

	if (stretch) {
		Image::Scale(src, sw, sh, dst, w, h);
	/*} else if (MainMenuScreen::MAIN_MENU_SIZE == 2) {
		Image::Scale(src, sw, sh, dst, HRP::ScreenWidth(), HRP::ScreenHeight());*/ // scale to the size of the set game resolution
	} else {
		fo::func::buf_to_buf(src, sw, sh, sw, dst, dstW); // direct copy
	}

	// background image for buttons
	if (btnBackgroundFrm) {
		sh = btnBackgroundFrm->frames->height;
		sw = btnBackgroundFrm->frames->width;
		dst += (MainMenuScreen::MENU_BG_OFFSET_Y * dstW) + MainMenuScreen::MENU_BG_OFFSET_X;

		if (MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU) {
			fo::func::trans_cscale(btnBackgroundFrm->frames->indexBuff, sw, sh, sw, dst, (long)(sw * 1.5f), (long)(sh * 1.5f), dstW);
		} else {
			fo::func::trans_buf_to_buf(btnBackgroundFrm->frames->indexBuff, sw, sh, sw, dst, dstW); // direct copy
		}
	}
}

// create main menu window
// the window is created according to the size of the image, if the image exceeds the set game resolution, the window size is scaled
static long __fastcall main_menu_create_hook_add_win(long h, long y, long color, long flags) {
	long x = 0;
	long w = 640;
	long offset = 0;
	long sw = w, sh = h;

	if (MainMenuScreen::USE_HIRES_IMAGES) {
		if (!mainBackgroundFrm) {
			mainBackgroundFrm = fo::util::LoadUnlistedFrm("HR_MAINMENU.frm", fo::ArtType::OBJ_TYPE_INTRFACE);
			btnBackgroundFrm = fo::util::LoadUnlistedFrm("HR_MENU_BG.frm", fo::ArtType::OBJ_TYPE_INTRFACE);
		}
		if (mainBackgroundFrm) {
			sw = mainBackgroundFrm->frames->width;
			sh = mainBackgroundFrm->frames->height;
		}
	}

	if (MainMenuScreen::MAIN_MENU_SIZE != 2) {
		if (mainBackgroundFrm || MainMenuScreen::MAIN_MENU_SIZE == 1) {
			w = HRP::ScreenWidth();
			h = HRP::ScreenHeight();

			offset = Image::GetAspectSize(w, h, (float)sw, (float)sh);

			if (w > HRP::ScreenWidth()) w = HRP::ScreenWidth();
			if (h > HRP::ScreenHeight()) h = HRP::ScreenHeight();

			mainmenuWidth = w;
		}
	}

	if (MainMenuScreen::MAIN_MENU_SIZE == 1) {
		if (offset) {
			x = offset;
			// extract x/y window position
			if (x >= mainmenuWidth) {
				y = x / mainmenuWidth;
				x -= y * mainmenuWidth;
			}
		} else {
			// ???
		}
	} else if (MainMenuScreen::MAIN_MENU_SIZE == 2) {
		w = HRP::ScreenWidth();
		h = HRP::ScreenHeight();
		mainmenuWidth = w;
	} else {
		// centering
		x += (HRP::ScreenWidth() / 2) - (w / 2);
		y += (HRP::ScreenHeight() / 2) - (h / 2);
	}

	MainMenu::mTextOffset = MainMenu::mXOffset;
	if (MainMenu::mYOffset) MainMenu::mTextOffset += (MainMenu::mYOffset * w);

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

static void __fastcall TextScale(long xOffset, const char* text, long yPos, long color) {
	xOffset *= 1.5f;
	yPos *= 1.5f;
	yPos *= mainmenuWidth;
	yPos += MainMenu::mTextOffset; // (TODO: check)

	Image::ScaleText(
		(BYTE*)(fo::var::getInt(FO_VAR_main_window_buf) + yPos + xOffset),
		text, fo::util::GetTextWidth(text), mainmenuWidth, color, 1.5f
	);
}

// buttons text print
static void __declspec(naked) main_menu_create_hook_text_to_buf() {
	__asm { // eax:xOffset, ebp:yPos, edx:text, ebx:txtWidth
		cmp  MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU, 1;
		jne  noScale;
		push [esp + 4]; // color
		push ebp;
		mov  ecx, eax;
		call TextScale;
		retn 4;
noScale:
		mov  ecx, mainmenuWidth;
		imul ebp, ecx; // yPos * width
		add  eax, ds:[FO_VAR_main_window_buf];
		add  eax, MainMenu::mTextOffset;
		add  eax, ebp;
		jmp  dword ptr ds:[FO_VAR_text_to_buf];
	}
}

BYTE* buttonImageData;

static long __fastcall ButtonScale(long &xPos, long yPos, BYTE* &upImageData, BYTE* &downImageData) {
	if (!buttonImageData) {
		buttonImageData = new BYTE[(39 * 39) * 2]();

		BYTE* buttonUpData = *(BYTE**)FO_VAR_button_up_data;
		BYTE* buttonDownData = *(BYTE**)FO_VAR_button_down_data;

		// up
		fo::func::trans_cscale(buttonUpData, 26, 26, 26, buttonImageData, 39, 39, 39);

		// down
		BYTE* down = &buttonImageData[39 * 39];
		fo::func::trans_cscale(buttonDownData, 26, 26, 26, down, 39, 39, 39);

		upImageData = buttonImageData;
		downImageData = down;

		fo::var::setInt(FO_VAR_button_up_data) = (long)buttonImageData;
		fo::var::setInt(FO_VAR_button_down_data) = (long)down;
	}
	xPos *= 1.5f;
	return yPos * 1.5f;
}

static void __declspec(naked) main_menu_create_hook_register_button() {
	__asm { // eax:_main_window, edx:Xpos, ebx:Ypos, ecx:Width
		cmp  MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU, 1;
		jne  noScale;
		push ebx;
		mov  ecx, esp;            // xPos
		lea  ebx, [esp + 24 + 4]; // button_up_data
		push ebx;
		lea  ebx, [esp + 28 + 8]; // button_down_data
		push ebx;
		call ButtonScale;
		mov  edx, eax;
		pop  ebx;
		// set scale size
		mov  ecx, 39;        // width
		mov  [esp + 4], ecx; // height
		mov  eax, ds:[FO_VAR_main_window];
noScale:
		jmp fo::funcoffs::win_register_button_;
	}
}

static void FreeMainMenuImages() {
	if (mainBackgroundFrm) {
		delete mainBackgroundFrm;
		mainBackgroundFrm = nullptr;
	}
	if (btnBackgroundFrm) {
		delete btnBackgroundFrm;
		btnBackgroundFrm = nullptr;
	}
	if (buttonImageData) {
		delete[] buttonImageData;
		buttonImageData = nullptr;
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

	HookCall(0x481883, main_menu_create_hook_register_button);

	LoadGameHook::OnBeforeGameStart() += FreeMainMenuImages;
	LoadGameHook::OnBeforeGameClose() += FreeMainMenuImages;
}

}
