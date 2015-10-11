/*
 *    sfall
 *    Copyright (C) 2009, 2010  The sfall team
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

#include "main.h"

#include <stdio.h>
#include "version.h"
#include "Logging.h"

static const DWORD ExpectedSize=0x00122800;
static const DWORD ExpectedCRC[]= {0xe1680293, 0xef34f989};

static void inline Fail(const char* a) { MessageBoxA(0, a, "Error", MB_TASKMODAL); ExitProcess(1); }

static DWORD crcInternal(BYTE* data, DWORD size) {
	DWORD table[256];
	const DWORD kPoly = 0x1EDC6F41;
	for (DWORD i = 0;i < 256;i++)	{
		DWORD r = i;
		for (int j = 0;j < 8;j++) {
			if ((r & 1) != 0) r = (r >> 1) ^ kPoly;
			else r >>= 1;
		}
		table[i] = r;
	}

	DWORD crc=0xffffffff;
	for (DWORD i = 0;i < size;i++) {
		crc = table[(((BYTE)(crc)) ^ data[i])] ^ (crc >> 8);
	}
	return crc^0xffffffff;
}

void CRC(const char* filepath) {
	char buf[512];
	HANDLE h = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (h == INVALID_HANDLE_VALUE) Fail("Cannot open fallout2.exe for CRC check");
	DWORD size = GetFileSize(h, 0), crc;
	bool sizeMatch = (size == ExpectedSize);

	if (IsDebug && !sizeMatch && GetPrivateProfileIntA("Debugging", "SkipSizeCheck", 0, ".\\ddraw.ini")) {
		sizeMatch = true;
	}

	if (!sizeMatch) {
		sprintf_s(buf, "You're trying to use sfall with an incompatible version of fallout\nWas expecting '" TARGETVERSION "'\n\nfallout2.exe was an unexpected size. Expected 0x%x but got 0x%x", ExpectedSize, size);
		Fail(buf);
	}
	BYTE* bytes = new BYTE[size];
	ReadFile(h, bytes, size, &crc, 0);
	crc = crcInternal(bytes, size);

	bool matchedCRC = false;

	if (IsDebug && GetPrivateProfileStringA("Debugging", "ExtraCRC", "", buf, 512, ".\\ddraw.ini") > 0) {
		char *TestCRC;
		TestCRC = strtok(buf, ",");
		while (TestCRC) {
			DWORD extraCRC = strtoul(TestCRC, 0, 16);
			if (crc == extraCRC) matchedCRC = true;
			TestCRC = strtok(0, ",");
		}
	}

	for (int i=0; i < sizeof(ExpectedCRC)/4; i++) {
		if (crc == ExpectedCRC[i]) matchedCRC = true;
	}
	if (!matchedCRC) {
		sprintf_s(buf, "You're trying to use sfall with an incompatible version of fallout\nWas expecting '" TARGETVERSION "'\n\nfallout2.exe had an unexpected crc. Expected 0x%x but got 0x%x", ExpectedCRC[0], crc);
		Fail(buf);
	}

	CloseHandle(h);
	delete[] bytes;
}
