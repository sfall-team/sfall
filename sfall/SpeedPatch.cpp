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
#include "InputFuncs.h"
#include "LoadGameHook.h"

#include "SpeedPatch.h"

static const DWORD offsets[] = {
	// GetTickCount calls
	0x4C9375, // get_time_
	0x4C93E8, // elapsed_time_
	0x4C93C0, // block_for_tocks_
	0x4C9384, // pause_for_tocks_
	0x4FE01E, // unused???
	0x4C9D2E, // GNW95_process_message_
	0x4C8D34, // GNW_do_bk_process_

	// Delayed GetTickCount calls
	0x4FDF64,

	// timeGetTime calls
//	0x4A3179, // init_random_ (unused)
//	0x4A325D, // timer_read_
//	0x4F482B, // unused
	0x4FE036, // unused???

	// Affect the playback speed of MVE video files without an audio track
//	0x4F4E53, // syncWait_
//	0x4F5542, // syncInit_
//	0x4F56CC, 0x4F59C6, // MVE_syncSync_
};

static DWORD getLocalTimeOffs;

DWORD getTickCountOffs = (DWORD)&GetTickCount;

DWORD SpeedPatch_getTickCount() {
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

static bool defaultDelay = true;

static void SetKeyboardDefaultDelay() {
	if (defaultDelay) return;
	defaultDelay = true;
	var_setInt(FO_VAR_GNW95_repeat_rate) = 80;
	var_setInt(FO_VAR_GNW95_repeat_delay) = 500;
}

static void SetKeyboardDelay() {
	var_setInt(FO_VAR_GNW95_repeat_rate) = static_cast<long>(80 * multi);
	var_setInt(FO_VAR_GNW95_repeat_delay) = static_cast<long>(500 * multi);
	defaultDelay = false;
}

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
					SetKeyboardDelay();
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
	long mode;
	if (IsGameLoaded() && enabled &&
	    (!(mode = GetLoopFlags()) || mode == COMBAT || mode == (COMBAT | PCOMBAT) || (mode & WORLDMAP)) && !slideShow)
	{
		elapsed *= multi;
		elapsed += tickCountFraction;
		tickCountFraction = modf(elapsed, &elapsed);
		if (defaultDelay) SetKeyboardDelay();
	} else {
		SetKeyboardDefaultDelay();
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
		_itoa(i, buf, 10);
		spKey[8] = spMulti[10] = buf[0];
		speed[i].key = GetConfigInt("Input", spKey, 0);
		speed[i].multiplier = GetConfigInt("Speed", spMulti, 0) / 100.0;
	}
}

void SpeedPatch_Init() {
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

		for (int i = 0; i < sizeof(offsets) / 4; i++) {
			SafeWrite32(offsets[i], (DWORD)&getTickCountOffs);
		}
		SafeWrite32(0x4FDF58, (DWORD)&getLocalTimeOffs);
		HookCall(0x4A433E, scripts_check_state_hook);

		TimerInit();
		dlogr(" Done", DL_INIT);
	}
}

void SpeedPatch_Exit() {
	if (speed) delete[] speed;
}
