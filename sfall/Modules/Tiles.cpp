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

#include <math.h>
#include <stdio.h>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Utils.h"
//#include "FileSystem.h"

#include "Tiles.h"

namespace sfall
{

typedef int (__stdcall *functype)();

static const DWORD Tiles_0E[] = {
	0x484255, 0x48429D, // square_reset_
	0x484377, 0x484385, // square_load_
	0x48A897, 0x48A89A, // obj_move_to_tile_
	0x4B2231,           // square_render_roof_
	0x4B2374, 0x4B2381, // roof_fill_on_
	0x4B2480, 0x4B248D, // tile_fill_roof_
	0x4B2A7C,           // square_render_floor_
	0x4B2BDA,           // square_roof_intersect_
};

static const DWORD Tiles_3F[] = {
	0x41875D, 0x4839E6, 0x483A2F, 0x484380, 0x48A803, 0x48A9F2, 0x48C96C,
	0x48C99F, 0x48C9D2, 0x4B2247, 0x4B2334, 0x4B2440, 0x4B2AB9, 0x4B2B8F,
	0x4B2BF4,
};

static const DWORD Tiles_40[] = {
	0x48C941, 0x48C955, 0x48CA5F, 0x48CA71, 0x48CA9B, 0x48CAAD, 0x48CADB,
	0x48CAEB, 0x48CAF7, 0x48CB20, 0x48CB32, 0x48CB61,
};

static const DWORD Tiles_C0[] = {
	0x48424E, 0x484296, 0x484372, 0x48A88D, 0x48A892, 0x4B222C, 0x4B236F,
	0x4B247B, 0x4B2A77, 0x4B2BD5,
};

#pragma pack(push, 1)
struct TilesData {
	short tile[2];
};
#pragma pack(pop)

struct OverrideEntry {
	DWORD xTiles;
	DWORD yTiles;
	DWORD replacementID;

	OverrideEntry(DWORD xtiles, DWORD ytiles, DWORD repid)
		: xTiles(xtiles), yTiles(ytiles), replacementID(repid) {
	}
};

static OverrideEntry** overrides = nullptr;
static DWORD origTileCount = 0;
static DWORD tileMode;
static BYTE* mask;

static bool LoadMask() {
	fo::DbFile* file = fo::func::db_fopen("art\\tiles\\gridmask.frm", "rb"); // same as grid000.frm from HRP
	if (!file) {
		dlogr("AllowLargeTiles: Unable to open art\\tiles\\gridmask.frm file.", DL_INIT);
		return false;
	}
	mask = new BYTE[80 * 36];

	fo::func::db_fseek(file, 0x4A, SEEK_SET);
	fo::func::db_freadByteCount(file, mask, 80 * 36);
	fo::func::db_fclose(file);
	return true;
}

static int ProcessTile(fo::Art* tiles, int tile, int listPos) {
	char buf[32] = "art\\tiles\\";
	const char* name = &tiles->names[13 * tile];
	for (size_t i = 10; ; i++) {
		if (i == 32) return 0; // name too long
		char c = *name++;
		buf[i] = c;
		if (c == '\0') break;
	}

	fo::DbFile* artFile = fo::func::db_fopen(buf, "rb");
	if (!artFile) return 0;

	fo::func::db_fseek(artFile, 0x3E, SEEK_SET); // frameData

	WORD width;
	fo::func::db_freadShort(artFile, &width);
	if (width <= 80) goto exit;

	WORD height;
	fo::func::db_freadShort(artFile, &height);
	if (height < 36) goto exit;

	float newWidth = (float)(width - (width % 8));
	float newHeight = (float)(height - (height % 12));

	// Number of tiles horizontally and vertically
	// the calculation is unstable if the large tile is not proportional to the size of 80x36 (aspect ratio)
	int xSize = std::lroundf(((newWidth / 32.0f) - (newHeight / 24.0f)) - 0.01f);
	int ySize = std::lroundf(((newHeight / 16.0f) - (newWidth / 64.0f)) - 0.01f);
	if (xSize <= 0 || ySize <= 0) goto exit;

	// Check if total dimension of split tiles exceeds the original
	if (xSize > 1 && (xSize * 80) > width) xSize -= 1;
	if (ySize > 1 && (ySize * 36) > height) ySize -= 1;

	long bytes = width * height;
	BYTE* pixelData = new BYTE[bytes];

	// Read pixels data
	if (fo::func::db_fseek(artFile, 0x4A, SEEK_SET) ||
	    fo::func::db_fread(pixelData, 1, bytes, artFile) != bytes)
	{
		delete[] pixelData;
exit:
		fo::func::db_fclose(artFile);
		return 0;
	}
	long listID = listPos - tiles->total;

	fo::TileFrmFile frame;
	fo::func::db_fseek(artFile, 0, SEEK_SET);
	fo::func::db_freadByteCount(artFile, (BYTE*)&frame, 74);

	frame.frameHeader.height = ByteSwapW(36);
	frame.frameHeader.width = ByteSwapW(80);
	frame.frameHeader.size = ByteSwapD(80 * 36);
	frame.frameAreaSize = ByteSwapD(80 * 36 + 12);

	for (int y = 0; y < ySize; y++) {
		for (int x = 0; x < xSize; x++) {
			// offset relative to the top-left corner
			int xOffset = x * 48 + (ySize - (y + 1)) * 32;
			int yOffset = height - (36 + x * 12 + y * 24);

			for (int pY = 0; pY < 36; pY++) {
				for (int pX = 0; pX < 80; pX++) {
					int point = (80 * pY) + pX;
					if (mask[point]) {
						frame.pixels[point] = pixelData[(width * (yOffset + pY)) + xOffset + pX];
					} else {
						frame.pixels[point] = 0;
					}
				}
			}
			sprintf(&buf[10], "zzz%04d.frm", listID++);
			//FScreateFromData(buf, &frame, sizeof(frame));
			fo::DbFile* file = fo::func::db_fopen(buf, "wb");
			fo::func::db_fwriteByteCount(file, (BYTE*)&frame, sizeof(frame));
			fo::func::db_fclose(file);
		}
	}
	overrides[tile] = new OverrideEntry(xSize, ySize, listPos);

	fo::func::db_fclose(artFile);
	delete[] pixelData;

	return xSize * ySize; // number of tiles added
}

static const functype art_init = (functype)fo::funcoffs::art_init_;

static int __stdcall ArtInitHook() {
	if (art_init()) return -1;
	if (!LoadMask()) return 0;

	fo::Art* tiles = &fo::var::art[4];

	long listpos = origTileCount = tiles->total;
	overrides = new OverrideEntry*[listpos]();

	if (tileMode == 2) {
		fo::DbFile* file = fo::func::db_fopen("art\\tiles\\xltiles.lst", "rt");
		if (!file) return 0;

		char buf[32];
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
		for (long i = tiles->total; i < listpos; i++) {
			sprintf(&tiles->names[i * 13], "zzz%04d.frm", i - tiles->total);
		}
		tiles->total = listpos;
	}

	delete[] mask;
	return 0;
}

static __declspec(naked) void iso_init_hook() {
	__asm {
		mov  ebx, dword ptr ds:[FO_VAR_read_callback];
		xor  eax, eax;
		mov  dword ptr ds:[FO_VAR_read_callback], eax;
		call ArtInitHook;
		mov  dword ptr ds:[FO_VAR_read_callback], ebx;
		retn;
	}
}

static long __fastcall SquareLoadCheck(TilesData* data) {
	if (!overrides) return 0;
	for (DWORD y = 0; y < 100; y++) {
		for (DWORD x = 0; x < 100; x++) {
			for (DWORD z = 0; z < 2; z++) {
				DWORD tile = data[y * 100 + x].tile[z];
				if (tile > 1 && tile < origTileCount && overrides[tile]) {
					DWORD newtile = overrides[tile]->replacementID - 1;
					for (DWORD y2 = 0; y2 < overrides[tile]->yTiles; y2++) {
						for (DWORD x2 = 0; x2 < overrides[tile]->xTiles; x2++) {
							newtile++;
							if (x - x2 < 0 || y - y2 < 0) continue;
							data[(y - y2) * 100 + x - x2].tile[z] = (short)newtile;
						}
					}
				}
			}
		}
	}
	return 0; // don't delete
}

static __declspec(naked) void square_load_hook() {
	__asm {
		mov  ecx, edx;
		call fo::funcoffs::db_freadIntCount_;
		test eax, eax;
		jnz  end;
		jmp  SquareLoadCheck; // ecx - data
end:
		retn;
	}
}

static __declspec(naked) void art_id_hack() {
	using namespace fo;
	__asm {
		cmp  esi, (OBJ_TYPE_TILE << 24); // 0x4000000
		jne  end;
		and  eax, 0x3FFF;
		retn;
end:
		and  eax, 0x0FFF;
		retn;
	}
}

static __declspec(naked) void art_get_name_hack() {
	using namespace fo;
	__asm {
		sar  eax, 24;
		cmp  eax, OBJ_TYPE_TILE;
		jne  end;
		mov  ebp, edx;
		mov  esi, edx;
		and  ebp, 0xC000;
		and  esi, 0x3FFF;
		sar  ebp, 14;
end:
		test esi, esi;
		retn;
	}
}

void Tiles::init() {
	if (tileMode = IniReader::GetConfigInt("Misc", "AllowLargeTiles", 0)) {
		dlogr("Applying allow large tiles patch.", DL_INIT);
		HookCall(0x481D72, iso_init_hook);
		HookCall(0x48434C, square_load_hook);
	}

	//if (IniReader::GetConfigInt("Misc", "MoreTiles", 1)) {
		dlogr("Applying tile FRM limit patch.", DL_INIT);
		MakeCall(0x419D46, art_id_hack);
		MakeCall(0x419479, art_get_name_hack);
		SafeWriteBatch<BYTE>(0x0E, Tiles_0E);
		SafeWriteBatch<BYTE>(0x3F, Tiles_3F);
		SafeWriteBatch<BYTE>(0x40, Tiles_40);
		SafeWriteBatch<BYTE>(0xC0, Tiles_C0);
		if (HRP::Setting::VersionIsValid) { // Check HRP 4.1.8
			SafeWrite8(HRP::Setting::GetAddress(0x1000E1C0), 0x40); // 4000 > 16384
			SafeWrite8(HRP::Setting::GetAddress(0x1000E1DA), 0x3F); // and esi, 0x3FFF
		}
	//}
}

void Tiles::exit() {
	if (overrides) {
		for (size_t i = 0; i < origTileCount; i++) delete overrides[i]; // free OverrideEntry
		delete[] overrides;
	}
}

}
