/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\version.h"

#include "..\HRP\Init.h"

#include "MainMenu.h"

namespace sfall
{

#ifdef NDEBUG
static const char* VerString1 = "SFALL " VERSION_STRING;
#else
static const char* VerString1 = "SFALL " VERSION_STRING " Debug Build";
#endif

long MainMenu::mXOffset;
long MainMenu::mYOffset;
long MainMenu::mTextOffset; // sum: x + (y * w)

static long OverrideColour, OverrideColour2;

static __declspec(naked) void MainMenuHookButtonYOffset() {
	static const DWORD MainMenuButtonYHookRet = 0x48184A;
	__asm {
		xor edi, edi;
		xor esi, esi;
		mov ebp, MainMenu::mYOffset;
		jmp MainMenuButtonYHookRet;
	}
}

static __declspec(naked) void MainMenuHookTextYOffset() {
	__asm {
		add eax, MainMenu::mTextOffset;
		jmp dword ptr ds:[FO_VAR_text_to_buf];
	}
}

static void __fastcall main_menu_create_hook_print_text(long xPos, const char* text, long yPos, long color) {
	long winId = fo::var::main_window;
	if (!HRP::Setting::ExternalEnabled() && HRP::Setting::IsEnabled()) {
		fo::Window* win = fo::util::GetWindow(winId);
		yPos = ((yPos - 460) - 20) + win->height;
		xPos = ((xPos - 615) - 25) + win->width;
	}
	if (OverrideColour) color = OverrideColour;

	long fWidth = fo::util::GetTextWidth(text);
	fo::func::win_print(winId, text, fWidth, xPos, yPos - 12, color); // fallout print

	long sWidth = fo::util::GetTextWidth(VerString1);
	fo::func::win_print(winId, VerString1, sWidth, xPos + fWidth - sWidth, yPos, color); // sfall print
}

void MainMenu::init() {
	int offset;
	if (offset = IniReader::GetConfigInt("Misc", "MainMenuCreditsOffsetX", 0)) {
		SafeWrite32(0x481753, 15 + offset);
	}
	if (offset = IniReader::GetConfigInt("Misc", "MainMenuCreditsOffsetY", 0)) {
		SafeWrite32(0x48175C, 460 + offset);
	}
	if (offset = IniReader::GetConfigInt("Misc", "MainMenuOffsetX", 0)) {
		mXOffset = offset;
	}
	if (offset = IniReader::GetConfigInt("Misc", "MainMenuOffsetY", 0)) {
		mYOffset = offset;
	}
	if (!HRP::Setting::IsEnabled()) {
		if (mXOffset) SafeWrite32(0x48187C, 30 + mXOffset); // button
		if (mYOffset) MakeJump(0x481844, MainMenuHookButtonYOffset);

		mTextOffset = mXOffset + (mYOffset * 640);
		if (mTextOffset) MakeCall(0x481933, MainMenuHookTextYOffset, 1);
	}

	HookCall(0x4817AB, main_menu_create_hook_print_text);

	OverrideColour = IniReader::GetConfigInt("Misc", "MainMenuFontColour", 0);
	if (OverrideColour & 0xFF) {
		OverrideColour &= 0x00FF00FF;
		OverrideColour |= 0x06000000;
		unsigned char flags = static_cast<unsigned char>((OverrideColour & 0xFF0000) >> 16);
		if (!(flags & 1)) SafeWrite32(0x481748, (DWORD)&OverrideColour);
	}
	OverrideColour2 = IniReader::GetConfigInt("Misc", "MainMenuBigFontColour", 0) & 0xFF;
	if (OverrideColour2) SafeWrite32(0x481906, (DWORD)&OverrideColour2);
}

}
