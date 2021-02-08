/*
 *    sfall
 *    Copyright (C) 2008-2017  The sfall team
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

#include <cmath>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\InputFuncs.h"
#include "LoadGameHook.h"

#include "SpeedPatch.h"

namespace sfall
{

static const DWORD offsets[] = {
	// GetTickCount calls
	0x4C9375, 0x4C93E8, 0x4C93C0, 0x4FE01E,
	0x4C9384, 0x4C9D2E, 0x4C8D34,
	// Delayed GetTickCount calls
	0x4FDF64,
	// timeGetTime calls
	0x4A3179, 0x4A325D, 0x4F482B, 0x4FE036,
	0x4F4E53, 0x4F5542, 0x4F56CC, 0x4F59C6, // for mve
};

static DWORD getLocalTimeOffs;
DWORD SpeedPatch::getTickCountOffs = (DWORD)&GetTickCount;

DWORD SpeedPatch::getTickCount() {
	return ((DWORD (__stdcall*)())getTickCountOffs)();
}

static bool enabled = true;
static bool toggled = false;
static bool slideShow = false;

static double multi;
static DWORD sfallTickCount = 0;
static DWORD lastTickCount;
static double tickCountFraction = 0.0;

static __int64 startTime;

static struct SpeedCfg {
	int key;
	double multiplier;
} *speed = nullptr;

static int modKey[2];
static int toggleKey;

static DWORD __stdcall FakeGetTickCount() {
	// Keyboard control
	if (modKey[0] && (KeyDown(modKey[0]) || (modKey[1] && KeyDown(modKey[1])))) {
		if (toggleKey && KeyDown(toggleKey)) {
			if (!toggled) {
				toggled = true;
				enabled = !enabled;
			}
		} else {
			toggled = false;
			for (int i = 0; i < 10; i++) {
				int key = speed[i].key;
				if (key && KeyDown(key)) {
					multi = speed[i].multiplier;
					break;
				}
			}
		}
	}

	DWORD newTickCount = GetTickCount();
	if (newTickCount == lastTickCount) return sfallTickCount;

	// Just in case someone's been running their computer for 49 days straight
	if (lastTickCount > newTickCount) {
		lastTickCount = newTickCount;
		return sfallTickCount;
	}

	double elapsed = (double)(newTickCount - lastTickCount);
	lastTickCount = newTickCount;

	// Multiply the tick count difference by the multiplier
	if (enabled && !slideShow && !(GetLoopFlags() & (LoopFlag::INVENTORY | LoopFlag::INTFACEUSE | LoopFlag::INTFACELOOT | LoopFlag::DIALOG))) {
		elapsed *= multi;
		elapsed += tickCountFraction;
		tickCountFraction = modf(elapsed, &elapsed);
	}
	sfallTickCount += (DWORD)elapsed;

	return sfallTickCount;
}

void __stdcall FakeGetLocalTime(LPSYSTEMTIME time) {
	__int64 currentTime = startTime + sfallTickCount * 10000;
	FileTimeToSystemTime((FILETIME*)&currentTime, time);
}

//Could divide 'uDelay' by 'Multi', but doesn't seem to have any effect.
/*MMRESULT __stdcall fTimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent ) {
	return timeSetEvent(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
}

MMRESULT __stdcall fTimeKillEvent(UINT id) {
	return timeKillEvent(id);
}*/

static void __declspec(naked) scripts_check_state_hook() {
	__asm {
		inc  slideShow;
		call fo::funcoffs::endgame_slideshow_;
		dec  slideShow;
		retn;
	}
}

void TimerInit() {
	lastTickCount = GetTickCount();

	SYSTEMTIME time;
	GetLocalTime(&time);
	SystemTimeToFileTime(&time, (FILETIME*)&startTime);

	speed = new SpeedCfg[10];

	char buf[2], spKey[10] = "SpeedKey#";
	char spMulti[12] = "SpeedMulti#";
	for (int i = 0; i < 10; i++) {
		_itoa(i, buf, 10);
		spKey[8] = spMulti[10] = buf[0];
		speed[i].key = GetConfigInt("Input", spKey, 0);
		speed[i].multiplier = GetConfigInt("Speed", spMulti, 0) / 100.0;
	}
}

void SpeedPatch::init() {
	if (GetConfigInt("Speed", "Enable", 0)) {
		modKey[0] = GetConfigInt("Input", "SpeedModKey", 0);
		int init = GetConfigInt("Speed", "SpeedMultiInitial", 100);
		if (init == 100 && !modKey[0]) return;

		dlog("Applying speed patch.", DL_INIT);

		switch (modKey[0]) {
		case -1:
			modKey[0] = DIK_LCONTROL;
			modKey[1] = DIK_RCONTROL;
			break;
		case -2:
			modKey[0] = DIK_LMENU;
			modKey[1] = DIK_RMENU;
			break;
		case -3:
			modKey[0] = DIK_LSHIFT;
			modKey[1] = DIK_RSHIFT;
			break;
		default:
			modKey[1] = 0;
		}

		multi = (double)init / 100.0;
		toggleKey = GetConfigInt("Input", "SpeedToggleKey", 0);

		getTickCountOffs = (DWORD)&FakeGetTickCount;
		getLocalTimeOffs = (DWORD)&FakeGetLocalTime;

		int size = sizeof(offsets) / 4;
		if (GetConfigInt("Speed", "AffectPlayback", 0) == 0) size -= 4;

		for (int i = 0; i < size; i++) {
			SafeWrite32(offsets[i], (DWORD)&getTickCountOffs);
		}
		SafeWrite32(0x4FDF58, (DWORD)&getLocalTimeOffs);
		HookCall(0x4A433E, scripts_check_state_hook);

		TimerInit();
		dlogr(" Done", DL_INIT);
	}
}

void SpeedPatch::exit() {
	if (speed) delete[] speed;
}

}
