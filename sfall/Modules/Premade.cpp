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

#include "..\main.h"

struct PremadeChar {
	char path[20];
	DWORD fid;
	char unknown[20];
};

PremadeChar* premade;

void PremadeInit() {
	char buf[512];
	GetPrivateProfileString("misc", "PremadePaths", "", buf, 512, ini);
	if(buf[0]) {
		char buf2[512];
		GetPrivateProfileString("misc", "PremadeFIDs", "", buf2, 512, ini);

		int count=1;
		char* tmp=buf;
		while(tmp=strchr(tmp, ',')) { tmp++; count++; }
		premade=new PremadeChar[count];

		tmp=buf;
		char* tmp2=buf2;
		for(int i=0;i<count;i++) {
			char* tmp3=strchr(tmp, ',');
			if(tmp3) *tmp3=0;
			strcpy_s(premade[i].path, 20, "premade\\");
			strcat_s(premade[i].path, 20, tmp);
			tmp=tmp3 + 1;
			
			tmp3=strchr(tmp2, ',');
			if(tmp3) *tmp3=0;
			premade[i].fid=atoi(tmp2);
			tmp2=tmp3 + 1;
		}

		SafeWrite32(0x51C8D4, count);
		SafeWrite32(0x4A7D76, (DWORD)premade);
		SafeWrite32(0x4A8B1E, (DWORD)premade);
		SafeWrite32(0x4A7E2C, (DWORD)premade + 20);
		strcpy_s((char*)0x50AF68, 20, premade[0].path);
	}
}