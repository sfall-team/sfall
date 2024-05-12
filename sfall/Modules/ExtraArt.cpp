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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "ExtraArt.h"

namespace sfall
{

typedef std::unordered_map<std::string, fo::FrmFile*> TFrmCache;
typedef std::unordered_map<std::string, PcxFile> TPcxCache;

static TFrmCache frmFileCache;
static TPcxCache pcxFileCache;

static PcxFile LoadPcxFile(const char* file) {
	PcxFile pcx;
	pcx.pixelData = fo::func::loadPCX(file, &pcx.width, &pcx.height, fo::var::pal);
	if (pcx.pixelData == nullptr) return PcxFile();

	fo::func::datafileConvertData(pcx.pixelData, fo::var::pal, pcx.width, pcx.height);
	return pcx;
}

static fo::FrmFile* LoadFrmFile(const char* file) {
	fo::FrmFile* frmPtr = nullptr;
	if (fo::func::load_frame(file, &frmPtr)) {
		frmPtr = nullptr;
	}
	return frmPtr;
}

void UnloadFrmFile(fo::FrmFile* frm) {
	fo::func::mem_free(frm);
}

fo::FrmFile* LoadFrmFileCached(const char* file) {
	fo::FrmFile* frmPtr = nullptr;
	auto cacheHit = frmFileCache.find(file);
	if (cacheHit != frmFileCache.end()) {
		frmPtr = cacheHit->second;
	} else {
		frmPtr = LoadFrmFile(file);
		frmFileCache.emplace(file, frmPtr);
	}
	return frmPtr;
}

PcxFile LoadPcxFileCached(const char* file) {
	auto cacheHit = pcxFileCache.find(file);
	if (cacheHit != pcxFileCache.end()) {
		return cacheHit->second;
	}
	return pcxFileCache.emplace(file, LoadPcxFile(file)).first->second;
}

static void GetUnlistedFrmPath(const char* frmName, unsigned int folderRef, bool useLanguage, char* pathBuf) {
	const char* artfolder = fo::var::art[folderRef].path; // address of art type name
	if (useLanguage) {
		sprintf_s(pathBuf, MAX_PATH, "art\\%s\\%s\\%s", (const char*)fo::var::language, artfolder, frmName);
	} else {
		sprintf_s(pathBuf, MAX_PATH, "art\\%s\\%s", artfolder, frmName);
	}
}

static bool CheckUnlistedFrm(const char* frmName, unsigned int folderRef, char* frmPath) {
	if (folderRef > fo::OBJ_TYPE_SKILLDEX) return nullptr;

	GetUnlistedFrmPath(frmName, folderRef, fo::var::use_language != 0, frmPath);

	bool exists = fo::func::db_access(frmPath);
	if (!exists && fo::var::use_language) {
		GetUnlistedFrmPath(frmName, folderRef, false, frmPath);
		exists = fo::func::db_access(frmPath);
	}
	return exists;
}

bool UnlistedFrmExists(const char* frmName, unsigned int folderRef) {
	char frmPath[MAX_PATH];
	return CheckUnlistedFrm(frmName, folderRef, frmPath);
}

fo::FrmFile* LoadUnlistedFrm(const char* frmName, unsigned int folderRef) {
	char frmPath[MAX_PATH];
	if (!CheckUnlistedFrm(frmName, folderRef, frmPath)) return nullptr;
	return LoadFrmFile(frmPath);
}

fo::FrmFile* LoadUnlistedFrmCached(const char* frmName, unsigned int folderRef) {
	char frmPath[MAX_PATH];
	if (!CheckUnlistedFrm(frmName, folderRef, frmPath)) return nullptr;
	return LoadFrmFileCached(frmPath);
}

static void ClearInterfaceArtCache() {
	for (auto &pair : pcxFileCache) {
		fo::func::freePtr_invoke(pair.second.pixelData);
	}
	pcxFileCache.clear();

	for (auto &pair : frmFileCache) {
		UnloadFrmFile(pair.second);
	}
	frmFileCache.clear();
}

TempFrmHandle::TempFrmHandle(fo::FrmFile* frm) : _frm(frm) {
}

TempFrmHandle::TempFrmHandle(TempFrmHandle&& other) : _frm(other._frm) {
	other._frm = nullptr;
}

TempFrmHandle::~TempFrmHandle() {
	if (_frm == nullptr) return;
	UnloadFrmFile(_frm);
	_frm = nullptr;
}

bool TempFrmHandle::IsValid() {
	return _frm != nullptr;
}

const fo::FrmFile& TempFrmHandle::Frm() const {
	assert(_frm != nullptr);
	return *_frm;
}

void ExtraArt::init() {
	LoadGameHook::OnGameReset() += []() {
		ClearInterfaceArtCache();
	};
}

}