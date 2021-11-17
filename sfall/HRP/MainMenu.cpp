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

namespace HRP
{

namespace sf = sfall;

// 0 - main-menu image will display at its original size
// 1 - main-menu image will stretch to fit the screen while maintaining its aspect ratio
// 2 - main-menu image will stretch to fill the screen
long MainMenuScreen::MAIN_MENU_SIZE;

bool MainMenuScreen::USE_HIRES_IMAGES;
bool MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU; // if the value is false and USE_HIRES_IMAGES is false, btnBackgroundFrm and buttons with text labels are not scaled
long MainMenuScreen::MENU_BG_OFFSET_X = 29;
long MainMenuScreen::MENU_BG_OFFSET_Y = 19;

static fo::UnlistedFrm* mainBackgroundFrm;
static fo::UnlistedFrm* btnBackgroundFrm;

static long mainmenuWidth = 640;
static float scaleFactor = 1.0f; // for buttons and text
static float scaleXFactor = 1.0f;

static long GetXOffset() {
	return (scaleXFactor > 1.0f) ? std::lround(15 * scaleXFactor) : 0;
}

static long GetYOffset() {
	return (scaleXFactor > 1.0f) ? std::lround(9 * scaleXFactor) : 0;
}

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
	} else {
		fo::func::buf_to_buf(src, sw, sh, sw, dst, dstW); // direct copy
	}

	// background image for buttons
	if (btnBackgroundFrm) {
		sh = btnBackgroundFrm->frames->height;
		sw = btnBackgroundFrm->frames->width;

		if (MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU) {
			scaleFactor *= 1.6f; // additional scaling
			dst += (MainMenuScreen::MENU_BG_OFFSET_Y * dstW) + MainMenuScreen::MENU_BG_OFFSET_X;
			fo::func::trans_cscale(btnBackgroundFrm->frames->indexBuff, sw, sh, sw, dst, (long)(sw * scaleFactor), (long)(sh * scaleFactor), dstW);
		} else {
			dst += ((MainMenuScreen::MENU_BG_OFFSET_Y + GetYOffset()) * dstW) + MainMenuScreen::MENU_BG_OFFSET_X + GetXOffset();
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

	sf::Graphics::BackgroundClearColor(0);

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
			w = Setting::ScreenWidth();
			h = Setting::ScreenHeight();

			offset = Image::GetAspectSize(w, h, (float)sw, (float)sh);

			if (w > Setting::ScreenWidth()) w = Setting::ScreenWidth();
			if (h > Setting::ScreenHeight()) h = Setting::ScreenHeight();

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
			// nothing???
		}
	} else if (MainMenuScreen::MAIN_MENU_SIZE == 2) {
		w = Setting::ScreenWidth();
		h = Setting::ScreenHeight();
		mainmenuWidth = w;
	} else {
		// centering
		x += (Setting::ScreenWidth() / 2) - (w / 2);
		y += (Setting::ScreenHeight() / 2) - (h / 2);
	}

	// set scaling factor
	// is not scaled if USE_HIRES_IMAGES is used and the SCALE_BUTTONS_AND_TEXT_MENU option is disabled
	if (MainMenuScreen::USE_HIRES_IMAGES == false || (MainMenuScreen::USE_HIRES_IMAGES && MainMenuScreen::SCALE_BUTTONS_AND_TEXT_MENU)) {
		scaleFactor = h / (float)sh;
		//scaleFactor = std::round(scaleFactor * 100.0f) / 100.0f;
	}
	scaleXFactor = Setting::ScreenWidth() / (float)640; // TODO: find out how Mash's HRP gets the coefficient for button offsets for different game resolutions

	sf::MainMenu::mTextOffset = sf::MainMenu::mXOffset;
	if (sf::MainMenu::mYOffset) sf::MainMenu::mTextOffset += (sf::MainMenu::mYOffset * w);

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
	if (MainMenuScreen::USE_HIRES_IMAGES) {
		xOffset += GetXOffset();
		yPos += GetYOffset();
	}

	xOffset = (long)(xOffset * scaleFactor);
	yPos = (long)(yPos * scaleFactor);
	yPos *= mainmenuWidth;
	yPos += sf::MainMenu::mTextOffset; // TODO: check

	Image::ScaleText(
		(BYTE*)(fo::var::getInt(FO_VAR_main_window_buf) + yPos + xOffset),
		text, fo::util::GetTextWidth(text), mainmenuWidth, color, scaleFactor
	);
}

// buttons text print
static void __declspec(naked) main_menu_create_hook_text_to_buf() {
	__asm { // eax:xOffset, ebp:yPos, edx:text, ebx:txtWidth (640-txtWidth)-1
		cmp  scaleFactor, 1.0f;
		jne  scale;
		mov  ecx, mainmenuWidth;
		imul ebp, ecx; // yPos * width
		add  eax, ds:[FO_VAR_main_window_buf];
		add  eax, sf::MainMenu::mTextOffset;
		add  eax, ebp;
		jmp  dword ptr ds:[FO_VAR_text_to_buf];
scale:
		push [esp + 4]; // color
		push ebp;
		mov  ecx, eax;
		call TextScale;
		retn 4;
	}
}

static BYTE* buttonImageData;
static BYTE* downButtonImageData; // reference

static long __fastcall ButtonScale(long &width, long xPos, BYTE* &upImageData, BYTE* &downImageData, long &yPos) {
	if (MainMenuScreen::USE_HIRES_IMAGES) {
		xPos += GetXOffset();
		yPos += GetYOffset();
	}
	int sWidth = (int)(width * scaleFactor);

	if (!buttonImageData) {
		int size = sWidth * sWidth;
		buttonImageData = new BYTE[size * 2]; // two images

		// up
		Image::Scale(*(BYTE**)FO_VAR_button_up_data, width, width, buttonImageData, sWidth, sWidth);
		// down
		downButtonImageData = &buttonImageData[size];
		Image::Scale(*(BYTE**)FO_VAR_button_down_data, width, width, downButtonImageData, sWidth, sWidth);
	}
	upImageData = buttonImageData;
	downImageData = downButtonImageData;

	width = sWidth;
	yPos = (long)(yPos * scaleFactor);
	return (long)(xPos * scaleFactor);
}

static void __declspec(naked) main_menu_create_hook_register_button() {
	__asm { // eax:_main_window, edx:Xpos, ebx:Ypos, ecx:Width
		cmp  scaleFactor, 1.0f;
		jne  scale;
		jmp  fo::funcoffs::win_register_button_;
scale:
		push ecx;                  // width
		mov  ecx, esp;             // width ref
		push ebx;                  // Ypos
		push esp;                  // Ypos ref
		lea  ebx, [esp + 28 + 12]; // button_down_data ref
		push ebx;
		lea  ebx, [esp + 24 + 16]; // button_up_data ref
		push ebx;
		call ButtonScale;
		// set scale size
		mov  edx, eax;             // out Xpos
		pop  ebx;                  // out Ypos
		pop  ecx;                  // out width
		mov  [esp + 4], ecx;       // height
		mov  eax, ds:[FO_VAR_main_window];
		jmp  fo::funcoffs::win_register_button_;
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

	// Mainmenu window size
	//sf::SafeWrite32(0x481674, HRP::ScreenHeight());
	//sf::SafeWrite32(0x48167A, HRP::ScreenWidth());

	sf::HookCall(0x481680, main_menu_create_hook_add_win);
	sf::HookCall(0x481704, main_menu_create_hook_buf_to_buf);
	sf::HookCall(0x481767, main_menu_create_hook_win_print);
	sf::HookCall(0x481883, main_menu_create_hook_register_button);
	sf::MakeCall(0x481933, main_menu_create_hook_text_to_buf, 1);

	// imul ebp, edx, 640 -> mov ebp, edx
	sf::SafeWrite16(0x481912, 0xD589);
	sf::SafeWrite32(0x481914, 0x90909090);

	sf::SafeWrite16(0x48192D, 0x9090);

	sf::LoadGameHook::OnBeforeGameStart() += FreeMainMenuImages;
	sf::LoadGameHook::OnBeforeGameClose() += FreeMainMenuImages;
}

}
