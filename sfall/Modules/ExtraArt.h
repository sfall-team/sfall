/*
 *    sfall
 *    Copyright (C) 2008-2026  The sfall team
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

#pragma once

#include "Module.h"

namespace sfall
{

struct PcxFile {
	BYTE* pixelData;
	long  width;
	long  height;

	PcxFile() : pixelData(nullptr), width(0), height(0) {}
};

class ExtraArt : public Module {
public:
	const char* name() { return "ExtraArt"; }
	void init();

	static void OnGameReset();
};

// TODO: more robust caching, similar to how art_ptr_lock works.
fo::FrmFile* LoadFrmFileCached(const char* file);
PcxFile LoadPcxFileCached(const char* file);

bool UnlistedFrmExists(const char* frmName, unsigned int folderRef);
fo::FrmFile* LoadUnlistedFrm(const char* frmName, unsigned int folderRef);
fo::FrmFile* LoadUnlistedFrmCached(const char* file, unsigned int folderRef);
void UnloadFrmFile(fo::FrmFile* frm);

}
