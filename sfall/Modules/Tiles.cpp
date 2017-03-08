/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010, 2012  The sfall team
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

#include <math.h>
#include <stdio.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "FileSystem.h"

#include "Tiles.h"

namespace sfall
{

struct OverrideEntry {
	//DWORD id;
	DWORD xtiles;
	DWORD ytiles;
	DWORD replacementid;

	OverrideEntry(DWORD _xtiles, DWORD _ytiles, DWORD _repid) {
		xtiles = _xtiles;
		ytiles = _ytiles;
		replacementid = _repid;
	}
};

static OverrideEntry** overrides;
static DWORD origTileCount = 0;

typedef int (_stdcall *functype)();
static const functype _art_init = (functype)FuncOffs::art_init_;
static BYTE* mask;

static void CreateMask() {
	mask = new BYTE[80*36];
	fo::DBFile* file = fo::func::db_fopen("art\\tiles\\grid000.frm", "r");
	fo::func::db_fseek(file, 0x4a, 0);
	fo::func::db_freadByteCount(file, mask, 80*36);
	fo::func::db_fclose(file);
}

static WORD ByteSwapW(WORD w) {
	return ((w & 0xff) << 8) | ((w & 0xff00) >> 8);
}

static DWORD ByteSwapD(DWORD w) {
	return ((w & 0xff) << 24) | ((w & 0xff00) << 8) | ((w & 0xff0000) >> 8) | ((w & 0xff000000) >> 24);
}

static int ProcessTile(fo::sArt* tiles, int tile, int listpos) {
	char buf[32];
	//sprintf_s(buf, "art\\tiles\\%s", &tiles->names[13*tile]);
	strcpy_s(buf, "art\\tiles\\");
	strcat_s(buf, &tiles->names[13 * tile]);

	fo::DBFile* art = fo::func::db_fopen(buf, "r");
	if (!art) return 0;
	fo::func::db_fseek(art, 0x3e, 0);
	short width;
	fo::func::db_freadShort(art, &width);  //80;
	if (width == 80) {
		fo::func::db_fclose(art);
		return 0;
	}
	short height;
	fo::func::db_freadShort(art, &height); //36
	fo::func::db_fseek(art, 0x4A, 0);
	BYTE* pixeldata = new BYTE[width * height];
	fo::func::db_freadByteCount(art, pixeldata, width * height);
	DWORD listid = listpos - tiles->total;
	float newwidth = (float)(width - width % 8);
	float newheight = (float)(height - height % 12);
	int xsize = (int)floor(newwidth / 32.0f - newheight / 24.0f);
	int ysize = (int)floor(newheight / 16.0f - newwidth / 64.0f);
	for (int y = 0; y < ysize; y++) {
		for (int x = 0; x < xsize; x++) {
			fo::FRM frame;
			fo::func::db_fseek(art, 0, 0);
			fo::func::db_freadByteCount(art, &frame, 0x4a);
			frame.height = ByteSwapW(36);
			frame.width = ByteSwapW(80);
			frame.frmsize = ByteSwapD(80 * 36);
			frame.size = ByteSwapD(80 * 36 + 12);
			int xoffset = x * 48 + (ysize - (y + 1)) * 32;
			int yoffset = height - (36 + x * 12 + y * 24);
			for (int y2 = 0; y2 < 36; y2++) {
				for (int x2 = 0; x2 < 80; x2++) {
					if (mask[y2 * 80 + x2]) {
						frame.pixels[y2 * 80 + x2] = pixeldata[(yoffset + y2)*width + xoffset + x2];
					} else {
						frame.pixels[y2 * 80 + x2] = 0;
					}
				}
			}

			sprintf_s(buf, 32, "art\\tiles\\zzz%04d.frm", listid++);
			//FScreateFromData(buf, &frame, sizeof(frame));
			fo::DBFile* file = fo::func::db_fopen(buf, "w");
			fo::func::db_fwriteByteCount(file, &frame, sizeof(frame));
			fo::func::db_fclose(file);
		}
	}
	overrides[tile] = new OverrideEntry(xsize, ysize, listpos);
	fo::func::db_fclose(art);
	delete[] pixeldata;
	return xsize * ysize;
}

static DWORD tileMode;
static int _stdcall ArtInitHook2() {
	if (_art_init()) {
		return 1;
	}

	CreateMask();

	fo::sArt* tiles = &fo::var::art[4];
	char buf[32];
	DWORD listpos = tiles->total;
	origTileCount = listpos;
	overrides = new OverrideEntry*[listpos];
	ZeroMemory(overrides, 4 * (listpos - 1));

	if (tileMode == 2) {
		fo::DBFile* file = fo::func::db_fopen("art\\tiles\\xltiles.lst", "rt");
		if (!file) return 0;
		DWORD id;
		char* comment;
		while (fo::func::db_fgets(buf, 31, file) > 0) {
			if (comment = strchr(buf, ';')) *comment = 0;
			id = atoi(buf);
			if (id > 1) listpos += ProcessTile(tiles, id, listpos);
		}
		fo::func::db_fclose(file);
	} else {
		for (int i = 2; i < tiles->total; i++) listpos += ProcessTile(tiles, i, listpos);
	}
	if (listpos != tiles->total) {
		tiles->names = (char*)fo::func::mem_realloc(tiles->names, listpos * 13);
		for (DWORD i = tiles->total; i < listpos; i++) {
			sprintf_s(&tiles->names[i * 13], 12, "zzz%04d.frm", i - tiles->total);
		}
		tiles->total = listpos;
	}

	delete[] mask;
	return 0;
}

static void __declspec(naked) ArtInitHook() {
	__asm {
		pushad;
		mov eax, dword ptr ds:[VARPTR_read_callback];
		push eax;
		xor eax, eax;
		mov dword ptr ds:[VARPTR_read_callback], eax;
		call ArtInitHook2;
		pop eax;
		mov dword ptr ds:[VARPTR_read_callback], eax;
		popad;
		xor eax, eax;
		retn;
	}
}

struct tilestruct {
	short tile[2];
};

static void _stdcall SquareLoadCheck(tilestruct* data) {
	for (DWORD y = 0; y < 100; y++) {
		for (DWORD x = 0; x < 100; x++) {
			for (DWORD z = 0; z < 2; z++) {
				DWORD tile = data[y * 100 + x].tile[z];
				if (tile > 1 && tile < origTileCount&&overrides[tile]) {
					DWORD newtile = overrides[tile]->replacementid - 1;
					for (DWORD y2 = 0; y2 < overrides[tile]->ytiles; y2++) {
						for (DWORD x2 = 0; x2 < overrides[tile]->xtiles; x2++) {
							newtile++;
							if (x - x2 < 0 || y - y2 < 0) continue;
							data[(y - y2) * 100 + x - x2].tile[z] = (short)newtile;
						}
					}
				}
			}
		}
	}
}

static void __declspec(naked) SquareLoadHook() {
	__asm {
		mov edi, edx;
		call FuncOffs::db_freadIntCount_;
		test eax, eax;
		jnz end;
		pushad;
		push edi;
		call SquareLoadCheck;
		popad;
end:
		retn;
	}
}

void Tiles::init() {
	tileMode = GetConfigInt("Misc", "AllowLargeTiles", 0);
	if (tileMode) {
		HookCall(0x481D72, &ArtInitHook);
		HookCall(0x48434C, SquareLoadHook);
	}
}

}
