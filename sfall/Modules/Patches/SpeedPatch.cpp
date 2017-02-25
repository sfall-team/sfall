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

#include "..\..\main.h"
#include "..\..\FalloutEngine\Fallout2.h"
#include "..\Timer.h"

#include "SpeedPatch.h"

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

void ApplySpeedPatch() {
	if (GetPrivateProfileIntA("Speed", "Enable", 0, ini)) {
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
