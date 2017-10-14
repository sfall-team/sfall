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

#include "SpeedPatch.h"

namespace sfall
{

//GetTickCount calls
static const DWORD offsetsA[] = {
	0x4C8D34, 0x4C9375, 0x4C9384, 0x4C93C0, 0x4C93E8, 0x4C9D2E, 0x4FE01E,
};

//Delayed GetTickCount calls
static const DWORD offsetsB[] = {
	0x4FDF64,
};

//timeGetTime calls
static const DWORD offsetsC[] = {
	0x4A3179, 0x4A325D, 0x4F482B, 0x4F4E53, 0x4F5542, 0x4F56CC, 0x4F59C6,
	0x4FE036,
};

static const DWORD getLocalTimePos = 0x4FDF58;
static DWORD addrGetTickCount;
static DWORD addrGetLocalTime;

static bool Enabled;
static bool Toggled;

static double Multi;
static DWORD StoredTickCount;
static DWORD LastTickCount;
static double TickCountFraction;

static __int64 StartTime;

static double Multipliers[10];
static int Keys[10];
static int ModKey;
static int ToggleKey;

DWORD _stdcall FakeGetTickCount() {
	//Keyboard control
	if (!ModKey || (ModKey > 0 && KeyDown(ModKey))
		|| (ModKey == -1 && (KeyDown(DIK_LCONTROL) || KeyDown(DIK_RCONTROL)))
		|| (ModKey == -2 && (KeyDown(DIK_LMENU) || KeyDown(DIK_RMENU)))
		|| (ModKey == -3 && (KeyDown(DIK_LSHIFT) || KeyDown(DIK_RSHIFT)))
		) {

		for (int i = 0; i < 10; i++) if (Keys[i] && KeyDown(Keys[i])) {
			Multi = Multipliers[i];
		}

		if (ToggleKey&&KeyDown(ToggleKey)) {
			if (!Toggled) {
				Toggled = true;
				Enabled = !Enabled;
			}
		} else {
			Toggled = false;
		}
	}

	DWORD NewTickCount = GetTickCount();
	//Just in case someone's been running their computer for 49 days straight
	if (NewTickCount < LastTickCount) {
		NewTickCount = LastTickCount;
		return StoredTickCount;
	}

	//Multiply the tick count difference by the multiplier
	double add = (double)(NewTickCount - LastTickCount)*(Enabled ? Multi : 1.0);
	LastTickCount = NewTickCount;
	TickCountFraction += modf(add, &add);
	StoredTickCount += (DWORD)add;
	if (TickCountFraction > 1) {
		TickCountFraction -= 1;
		StoredTickCount++;
	}
	return StoredTickCount;
}

void _stdcall FakeGetLocalTime(LPSYSTEMTIME time) {
	__int64 CurrentTime = StartTime + StoredTickCount * 10000;
	FileTimeToSystemTime((FILETIME*)&CurrentTime, time);
}

//Could divide 'uDelay' by 'Multi', but doesn't seem to have any effect.
/*MMRESULT _stdcall fTimeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent ) {
	return timeSetEvent(uDelay, uResolution, lpTimeProc, dwUser, fuEvent);
}

MMRESULT _stdcall fTimeKillEvent(UINT id) {
	return timeKillEvent(id);
}*/

void TimerInit() {
	StoredTickCount = 0;
	LastTickCount = GetTickCount();
	TickCountFraction = 0;
	Multi = (double)GetConfigInt("Speed", "SpeedMultiInitial", 100) / 100.0;
	Enabled = true;
	Toggled = false;

	SYSTEMTIME time;
	GetLocalTime(&time);
	SystemTimeToFileTime(&time, (FILETIME*)&StartTime);

	ModKey = GetConfigInt("Input", "SpeedModKey", -1);
	ToggleKey = GetConfigInt("Input", "SpeedToggleKey", 0);
	char c[2];
	char key[12];
	for (int i = 0; i < 10; i++) {
		_itoa_s(i, c, 10);
		strcpy_s(key, "SpeedKey");
		strcat_s(key, c);
		Keys[i] = GetConfigInt("Input", key, 0);
		strcpy_s(key, "SpeedMulti");
		strcat_s(key, c);
		Multipliers[i] = GetConfigInt("Speed", key, 0x00) / 100.0;
	}
}

void SpeedPatch::init() {
	if (GetConfigInt("Speed", "Enable", 0)) {
		dlog("Applying speed patch.", DL_INIT);
		addrGetTickCount = (DWORD)&FakeGetTickCount;
		addrGetLocalTime = (DWORD)&FakeGetLocalTime;

		for (int i = 0; i < sizeof(offsetsA) / 4; i++) {
			SafeWrite32(offsetsA[i], (DWORD)&addrGetTickCount);
		}
		dlog(".", DL_INIT);
		for (int i = 0; i < sizeof(offsetsB) / 4; i++) {
			SafeWrite32(offsetsB[i], (DWORD)&addrGetTickCount);
		}
		dlog(".", DL_INIT);
		for (int i = 0; i < sizeof(offsetsC) / 4; i++) {
			SafeWrite32(offsetsC[i], (DWORD)&addrGetTickCount);
		}
		dlog(".", DL_INIT);

		SafeWrite32(getLocalTimePos, (DWORD)&addrGetLocalTime);
		TimerInit();
		dlogr(" Done", DL_INIT);
	}
}

}
