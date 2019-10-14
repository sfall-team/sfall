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

#include "main.h"

#include "FalloutEngine.h"
#include "input.h"
#include "LoadGameHook.h"

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

DWORD sf_GetTickCount = (DWORD)&GetTickCount;
DWORD sf_GetLocalTime;

static bool enabled = true;
static bool toggled = false;
static bool slideShow = false;

static double multi;
static DWORD storedTickCount = 0;
static DWORD lastTickCount;
static double tickCountFraction = 0.0;

static __int64 startTime;

static struct SpeedCfg {
	int key;
	double multiplier;
} *speed = nullptr;

static int modKey;
static int toggleKey;

static DWORD _stdcall FakeGetTickCount() {
	// Keyboard control
	if (modKey && ((modKey > 0 && KeyDown(modKey))
		|| (modKey == -1 && (KeyDown(DIK_LCONTROL) || KeyDown(DIK_RCONTROL)))
		|| (modKey == -2 && (KeyDown(DIK_LMENU)    || KeyDown(DIK_RMENU)))
		|| (modKey == -3 && (KeyDown(DIK_LSHIFT)   || KeyDown(DIK_RSHIFT)))))
	{
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
	// Just in case someone's been running their computer for 49 days straight
	if (newTickCount < lastTickCount) {
		newTickCount = lastTickCount;
		return storedTickCount;
	}

	double elapsed = (double)(newTickCount - lastTickCount);
	lastTickCount = newTickCount;

	// Multiply the tick count difference by the multiplier
	if (enabled && !slideShow
		&& !(GetLoopFlags() & (INVENTORY | INTFACEUSE | INTFACELOOT | DIALOG)))
	{
		elapsed *= multi;
		tickCountFraction += modf(elapsed, &elapsed);
	}
	storedTickCount += (DWORD)elapsed;

	if (tickCountFraction > 1.0) {
		tickCountFraction -= 1.0;
		storedTickCount++;
	}
	return storedTickCount;
}

void _stdcall FakeGetLocalTime(LPSYSTEMTIME time) {
	__int64 currentTime = startTime + storedTickCount * 10000;
	FileTimeToSystemTime((FILETIME*)&currentTime, time);
}

//Could divide 'uDelay' by 'Multi', but doesn't seem to have any effect.
/*MMRESULT _stdcall fTimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent ) {
	return timeSetEvent(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
}

MMRESULT _stdcall fTimeKillEvent(UINT id) {
	return timeKillEvent(id);
}*/

static void __declspec(naked) scripts_check_state_hook() {
	__asm {
		inc  slideShow;
		call endgame_slideshow_;
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
		_itoa_s(i, buf, 10);
		spKey[8] = spMulti[10] = buf[0];
		speed[i].key = GetConfigInt("Input", spKey, 0);
		speed[i].multiplier = GetConfigInt("Speed", spMulti, 0) / 100.0;
	}
}

void SpeedPatchInit() {
	if (GetConfigInt("Speed", "Enable", 0)) {
		modKey = GetConfigInt("Input", "SpeedModKey", 0);
		int init = GetConfigInt("Speed", "SpeedMultiInitial", 100);
		if (init == 100 && !modKey) return;

		dlog("Applying speed patch.", DL_INIT);

		multi = (double)init / 100.0;
		toggleKey = GetConfigInt("Input", "SpeedToggleKey", 0);

		sf_GetTickCount = (DWORD)&FakeGetTickCount;
		sf_GetLocalTime = (DWORD)&FakeGetLocalTime;

		int size = sizeof(offsets) / 4;
		if (GetConfigInt("Speed", "AffectPlayback", 0) == 0) size -= 4;

		for (int i = 0; i < size; i++) {
			SafeWrite32(offsets[i], (DWORD)&sf_GetTickCount);
		}
		SafeWrite32(0x4FDF58, (DWORD)&sf_GetLocalTime);
		HookCall(0x4A433E, scripts_check_state_hook);

		TimerInit();
		dlogr(" Done", DL_INIT);
	}
}

void SpeedPatchExit() {
	if (speed) delete[] speed;
}
