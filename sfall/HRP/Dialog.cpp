/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Modules\ExtraArt.h"
#include "..\Modules\LoadGameHook.h"

#include "Init.h"
#include "InterfaceBar.h"
#include "ViewMap\ViewMap.h"

#include "Dialog.h"

namespace HRP
{

namespace sf = sfall;

static const long width = 640; // art
static long scr_width = 639;

static fo::FrmFile* altDialogArt;

bool Dialog::DIALOG_SCRN_ART_FIX = true;
bool Dialog::DIALOG_SCRN_BACKGROUND = false;

static long xPosition;
static long yPosition;
static long dialogExpandedHeight = 0;

static long __fastcall CreateWinDialog(long height, long yPos, long xPos, long color, long flags) {
	long fitRectW = fo::var::getInt(FO_VAR_buf_width_2);
	long fitRectH = fo::var::getInt(FO_VAR_buf_length_2);

	if (Dialog::DIALOG_SCRN_BACKGROUND) {
		fo::func::win_hide(fo::var::getInt(FO_VAR_display_win));
		IFaceBar::Hide();

		fitRectH += fo::func::GNW_find(fo::var::interfaceWindow)->height;
	}

	long expandedHeight = dialogExpandedHeight > 0 ? dialogExpandedHeight : height;
	xPos += (fitRectW - width) / 2;  // xPos:0
	yPos += (fitRectH - expandedHeight) / 2; // yPos:0 = 480 - art_frame_length
	if (yPos < 0) yPos = 0;

	yPosition = yPos;
	xPosition = xPos;

	// item move window
	fo::var::iscr_data[4].x = xPos + 185;
	fo::var::iscr_data[4].y = yPos + 115;
	// about window
	fo::var::iscr_data[3].x = xPos + 80;
	fo::var::iscr_data[3].y = yPos + 290;

	return fo::func::win_add(xPos, yPos, width, height, color, flags);
}

static __declspec(naked) void gdCreateHeadWindow_hook_win_add() {
	__asm {
		pop	 ebx; // ret addr
		push eax; // xPos
		push ebx;
		jmp  CreateWinDialog;
	}
}

static void ShowMapWindow() {
	fo::func::win_show(fo::var::getInt(FO_VAR_display_win));
	fo::func::win_draw(fo::var::getInt(FO_VAR_display_win));
	IFaceBar::Show();
}

static __declspec(naked) void gdDestroyHeadWindow_hook_win_delete() {
	__asm {
		call fo::funcoffs::win_delete_;
		jmp  ShowMapWindow;
	}
}

static __declspec(naked) void GeneralDialogWinAdd() {
	__asm {
		add  eax, xPosition;
		add  edx, yPosition;
		mov  ebx, width;
		jmp  fo::funcoffs::win_add_;
	}
}

static __declspec(naked) void gdProcess_hook_win_add() {
	__asm {
		add  edx, yPosition;
		add  eax, xPosition;
		jmp  fo::funcoffs::win_add_;
	}
}

static __declspec(naked) void setup_inventory_hook_win_add() {
	__asm {
		add  eax, xPosition;
		add  edx, yPosition;
		jmp  fo::funcoffs::win_add_;
	}
}

static __declspec(naked) void setup_inventory_hack() {
	__asm {
		mov  dword ptr ds:[FO_VAR_i_wid], eax;
		add  ebx, xPosition;
		add  ecx, yPosition;
		retn;
	}
}

static __declspec(naked) void barter_move_hook_mouse_click_in() {
	__asm {
		add  eax, xPosition; // left
		add  ebx, xPosition; // right
		add  edx, yPosition; // top
		add  ecx, yPosition; // bottom
		jmp  fo::funcoffs::mouse_click_in_;
	}
}

static __declspec(naked) void hook_buf_to_buf() {
	__asm {
		imul eax, yPosition, 640;
		add  [esp + 4], eax;
		jmp  fo::funcoffs::buf_to_buf_;
	}
}

// Implementation from HRP by Mash
static void __cdecl gdDisplayFrame_hook_buf_to_buf(BYTE* src, long w, long h, long srcWidth, BYTE* dst, long dstWidth) {
	long cx, cy;
	ViewMap::GetTileCoord(fo::var::getInt(FO_VAR_tile_center_tile), cx, cy);

	fo::GameObject* dialog_target = fo::var::dialog_target;

	long x, y;
	ViewMap::GetTileCoord(dialog_target->tile, x, y);

	long xDist = 16 * (cx - x);
	long yDist = 12 * (y - cy);

	DWORD lockPtr;
	auto frm = fo::func::art_ptr_lock(dialog_target->artFid, &lockPtr);
	long yOffset = fo::func::art_frame_length(frm, dialog_target->frm, dialog_target->rotation) / 2;
	fo::func::art_ptr_unlock(lockPtr);

	long mapWinH = fo::var::getInt(FO_VAR_buf_length_2) - h;
	y = (mapWinH / 2) + (yDist - yOffset);
	if (y < 0) {
		y = 0;
	} else if (y > mapWinH) {
		y = mapWinH;
	}

	long mapWinW = srcWidth - w; // fo::var::getInt(FO_VAR_buf_width_2);
	x = (mapWinW / 2) + xDist;
	if (x < 0) {
		x = 0;
	} else if (x > (mapWinW)) {
		x = mapWinW;
	}
	fo::func::buf_to_buf((BYTE*)fo::var::getInt(FO_VAR_display_buf) + x + (y * srcWidth), w, h, srcWidth, dst, width);
}

static bool loadAltDialogArt = false;

static void __cdecl talk_to_refresh_background_window_hook_buf_to_buf(BYTE* src, long w, long h, long srcWidth, BYTE* dst, long dstWidth) {
	if (!loadAltDialogArt) {
		loadAltDialogArt = true;
		altDialogArt = sf::LoadUnlistedFrmCached("HR_ALLTLK.frm", fo::ArtType::OBJ_TYPE_INTRFACE);
	}
	if (altDialogArt) {
		src = altDialogArt->frameData[0].data;
		srcWidth = altDialogArt->frameData[0].width;
	}
	fo::func::buf_to_buf(src, w, h, srcWidth, dst, dstWidth);
}

static void UnloadDialogArt() {
	loadAltDialogArt = false;
}

void Dialog::init() {
	// replace width size for buffers functons
	sf::SafeWriteBatch(&scr_width, {
		0x447201, 0x447248, //0x44716F,         // gdCreateHeadWindow_
		0x4459F0, 0x445A0C, //0x44597A,         // gdReviewInit_
		0x445DA7, 0x445DD1,                     // gdReviewDisplay_
		0x447E43, 0x447E75, 0x447EE2, 0x447EFE, 0x447DB5, // gdialog_scroll_subwin_
		0x44A705, //0x44A6C3,                   // gdialog_window_create_
		0x44AA28,                               // gdialog_window_destroy_
		0x448354, //0x448312,                   // gdialog_barter_create_win_
		0x4485AC,                               // gdialog_barter_destroy_win_
		0x4487FF, //0x4487BD,                   // gdControlCreateWin_
		0x448CA0,                               // gdControlDestroyWin_
		0x4497A0, //0x44975E,                   // gdCustomCreateWin_
		0x449AA0,                               // gdCustomDestroyWin_
		0x449C2C,                               // gdCustomUpdateInfo_
		0x44AB4C, 0x44AB6A,                     // talk_to_refresh_background_window_
		0x44AC23,                               // talkToRefreshDialogWindowRect_
		0x44AE4D, 0x44AF94, 0x44AFD5, 0x44B015, 0x44AD7B, 0x44AE88, // gdDisplayFrame_
		//0x44AADE,                             // talk_to_create_background_window_ (unused)
		// for barter interface
		0x46EE04,                               // setup_inventory_
		0x4753C1, 0x475400, 0x47559F, 0x4755F5, // display_table_inventories_
		0x47005A, 0x47008F,                     // display_inventory_
		0x470401, 0x47043E,                     // display_target_inventory_
		0x47080E,                               // display_body_

		0x474E35, 0x474E76,                     // barter_move_inventory_
		0x4750FE, 0x475139,                     // barter_move_from_table_inventory_
		0x4733DF, 0x47343A,                     // inven_action_cursor_
	});

	sf::HookCall(0x44718F, gdCreateHeadWindow_hook_win_add);
	sf::HookCalls(GeneralDialogWinAdd, {
		0x445997, // gdReviewInit_
		0x44A6E4, // gdialog_window_create_
		0x448333, // gdialog_barter_create_win_
		0x4487CE, // gdControlCreateWin_
		0x44976F, // gdCustomCreateWin_
	});

	// gdCustomSelect_
	long yoffset = (DIALOG_SCRN_BACKGROUND) ? 100 : 200; // shifted the window up so that the window with the selected options was visible
	sf::SafeWrite32(0x44A03E, Setting::ScreenHeight() - yoffset);
	sf::SafeWrite32(0x44A02A, Setting::ScreenWidth());

	sf::HookCall(0x4462A7, gdProcess_hook_win_add);
	sf::HookCall(0x446387, gdProcess_hook_win_add);

	sf::HookCall(0x447900, hook_buf_to_buf); // demo_copy_options_
	sf::HookCall(0x447A46, hook_buf_to_buf); // gDialogRefreshOptionsRect_

	sf::HookCall(0x44AF39, gdDisplayFrame_hook_buf_to_buf);

	// Barter interface hacks
	sf::HookCall(0x46EDC9, setup_inventory_hook_win_add);
	sf::MakeCall(0x46EDD8, setup_inventory_hack);
	sf::HookCalls(barter_move_hook_mouse_click_in, {
		0x474F76, 0x474FF9, // barter_move_inventory_
		0x475241, 0x4752C2, // barter_move_from_table_inventory_
		0x449661            // gdControl_ (custom disposition button)
	});

	if (DIALOG_SCRN_BACKGROUND) sf::HookCall(0x4472D8, gdDestroyHeadWindow_hook_win_delete);

	if (DIALOG_SCRN_ART_FIX) {
		sf::HookCall(0x44AB79, talk_to_refresh_background_window_hook_buf_to_buf);

		// gdCreateHeadWindow_
		sf::SafeWrite16(0x447255, 0xD269);
		sf::SafeWrite32(0x447257, 19); // y
		sf::SafeWrite8(0x44725B, 0x90);
		sf::SafeWrite16(0x44725C, 0x3EB);
		// gdDisplayFrame_
		sf::SafeWrite8(0x44AF9D, 20);   //  15 to 20
		sf::SafeWrite32(0x44AFEB, 219); // 214 to 219
		sf::SafeWrite32(0x44AF47, 19);  //  14 to 19
		sf::SafeWrite32(0x44AF51, 219); // 214 to 219

		for (size_t i = 0; i < 8; i++) {
			fo::var::backgrndRects[i].y += 5;
			fo::var::backgrndRects[i].offy += 5;
		}

		sf::LoadGameHook::OnGameReset() += UnloadDialogArt;
	}
}

void Dialog::SetDialogExpandedHeight(long height) {
	assert(height >= 0);

	dialogExpandedHeight = height;
}

}
