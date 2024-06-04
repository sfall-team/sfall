/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
 *
 */

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "CreditsScreen.h"

namespace HRP
{

namespace sf = sfall;

static void __cdecl credits_hook_buf_to_buf_art(BYTE* src, long w, long h, long srcW, BYTE* dst, long dstW) {
	long y = (Setting::ScreenHeight() - h) / 2;
	dst += (y * Setting::ScreenWidth());

	fo::func::buf_to_buf(src, w, h, srcW, dst, Setting::ScreenWidth());
}

static __declspec(naked) void credits_hack() {
	__asm { // edx: H
		call Setting::ScreenWidth;
		imul eax, edx; // ScreenWidth * H
		retn;
	}
}

static void PatchBufToBuf(DWORD w, DWORD h, DWORD srcW, DWORD dstW) {
	sf::SafeWriteBatch<DWORD>(Setting::ScreenWidth(), { w, srcW, dstW });
	sf::SafeWrite32(h, Setting::ScreenHeight());
}

void CreditsScreen::init() {
	// credits_
	// set window size
	sf::SafeWrite32(0x42C945, Setting::ScreenHeight());
	sf::SafeWrite32(0x42C94F, Setting::ScreenWidth());

	sf::HookCall(0x42CA38, credits_hook_buf_to_buf_art);
	sf::SafeWrite16(0x42CA21, 0x9009); // remove 'add edi, eax'

	sf::MakeCall(0x42CAAE, credits_hack);

	sf::SafeWriteBatch<DWORD>(Setting::ScreenWidth(), {
		0x42CA18, // _text_to_buf
		0x42CB1C,
		0x42CB79, // textWidth < ScreenWidth
		0x42CBA8, // _text_to_buf
		0x42CBCB, 0x42CBEC,
		0x42CCD9,
		0x42CD1D, 0x42CD55
	});

	size_t screenBufSize = Setting::ScreenWidth() * Setting::ScreenHeight();
	size_t screenBufSize0 = screenBufSize - Setting::ScreenWidth();

	sf::SafeWriteBatch<DWORD>(screenBufSize, {
		0x42C98B, // allocate backgrounf art buffer
		0x42C9AB, // clear art buffer
		0x42CA4D, // allocate buffer
		0x42CA68  // clear buffer
	});

	sf::SafeWriteBatch<DWORD>(screenBufSize0, {0x42CB22, 0x42CC22, 0x42CD23, 0x42CD3D});

	PatchBufToBuf(0x42CAE6, 0x42CAE1, 0x42CADC, 0x42CACF); // buf_to_buf_
	PatchBufToBuf(0x42CC76, 0x42CC71, 0x42CC6C, 0x42CC5F); // buf_to_buf_
	PatchBufToBuf(0x42CC9B, 0x42CC96, 0x42CC91, 0x42CC8B); // trans_buf_to_buf_
	PatchBufToBuf(0x42CD7A, 0x42CD75, 0x42CD70, 0x42CD63); // buf_to_buf_
	PatchBufToBuf(0x42CDA6, 0x42CDA1, 0x42CD9C, 0x42CD8F); // trans_buf_to_buf_

	sf::SafeWrite32(0x42CDDC, Setting::ScreenHeight());
}

}
