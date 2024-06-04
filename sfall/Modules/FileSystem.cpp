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

#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "FileSystem.h"

namespace sfall
{

#define MAX_FILE_SIZE    0xA00000

#pragma pack(push, 1)

struct fsFile {
	BYTE* data;
	DWORD length;
	char name[128];
	DWORD wpos; // current fs read/write position
	BYTE isSave;
};

struct OpenFile {
	DWORD pos;  // current xread/xwrite position
	fsFile* file;

	OpenFile(fsFile* pFile) {
		pos = 0;
		file = pFile;
	}
};

struct sFile {
	DWORD type;
	OpenFile* openFile;
};

#pragma pack(pop)

std::vector<fsFile> files;

static DWORD loadedFiles = 0; // used for internal sfall data
bool FileSystem::UsingFileSystem = false;

static long __stdcall xfclose(sFile* file) {
	delete file->openFile;
	delete file;
	return 0;
}

static __declspec(naked) void asm_xfclose(sFile* file) {
	__asm {
		cmp  byte ptr [eax], 3; // byte
		jnz  end;
		push edx;
		push ecx;
		push eax;
		call xfclose;
		pop  ecx;
		pop  edx;
		retn;
end:
		jmp  fo::funcoffs::xfclose_;
	}
}

static sFile* __stdcall xfopen(const char* path, const char* mode) {
	for (DWORD i = 0; i < files.size(); i++) {
		if (!_stricmp(path, files[i].name)) {
			sFile* file = new sFile();
			file->type = 3;
			file->openFile = new OpenFile(&files[i]);
			return file;
		}
	}
	return 0;
}

static __declspec(naked) sFile* asm_xfopen(const char* path, const char* mode) {
	__asm {
		pushadc;
		push edx;
		push eax;
		call xfopen;
		pop  ecx;
		pop  edx;
		test eax, eax;
		jz   end;
		add  esp, 4;
		retn;
end:
		pop  eax;
		jmp  fo::funcoffs::xfopen_;
	}
}

//db_fprintf calls xvfprintf, not xfprintf
static int __stdcall xvfprintf() {
	return -1;
}

static __declspec(naked) int asm_xvfprintf(sFile* file, const char* format, void* vaargs) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		push ecx;
		call xvfprintf;
		pop  ecx;
		retn;
end:
		jmp  fo::funcoffs::xvfprintf_;
	}
}

static DWORD __stdcall xfgetc(sFile* file) {
	if (file->openFile->pos >= file->openFile->file->length) return -1;
	return static_cast<DWORD>(file->openFile->file->data[file->openFile->pos++]);
}

static __declspec(naked) int asm_xfgetc(sFile* file) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		push edx;
		push ecx;
		push eax;
		call xfgetc;
		pop  ecx;
		pop  edx;
		retn;
end:
		jmp  fo::funcoffs::xfgetc_;
	}
}

static char* __stdcall xfgets(char* buf, int max_count, sFile* file) {
	if (file->openFile->pos >= file->openFile->file->length) return 0;
	for (int i = 0; i < max_count; i++) {
		int c = xfgetc(file);
		if (c == -1) {
			buf[i] = 0;
			break;
		} else {
			buf[i] = (char)c;
			if (!c) break;
		}
	}
	return buf;
}

static __declspec(naked) char* asm_xfgets(char* buf, int max_count, sFile* file) {
	__asm {
		cmp  byte ptr [ebx], 3;
		jnz  end;
		push ecx;
		push ebx;
		push edx;
		push eax;
		call xfgets;
		pop  ecx;
		retn;
end:
		jmp  fo::funcoffs::xfgets_;
	}
}

static int __stdcall xfputc(int c, sFile* file) {
	return -1;
}

static __declspec(naked) int asm_xfputc(int c, sFile* file) {
	__asm {
		cmp  byte ptr [edx], 3;
		jnz  end;
		push ecx;
		push edx;
		push eax;
		call xfputc;
		pop  ecx;
		retn;
end:
		jmp  fo::funcoffs::xfputc_;
	}
}

static int __stdcall xfputs(const char* str, sFile* file) {
	return -1;
}

static __declspec(naked) int asm_xfputs(const char* str, sFile* file) {
	__asm {
		cmp  byte ptr [edx], 3;
		jnz  end;
		push ecx;
		push edx;
		push eax;
		call xfputs;
		pop  ecx;
		retn;
end:
		jmp  fo::funcoffs::xfputs_;
	}
}

static int __stdcall xfungetc(int c, sFile* file) {
	if (file->openFile->pos == 0) return -1;
	if (file->openFile->file->data[file->openFile->pos - 1] != static_cast<BYTE>(c)) return -1;
	file->openFile->pos--;
	return c;
}

static __declspec(naked) int asm_xfungetc(int c, sFile* file) {
	__asm {
		cmp  byte ptr [edx], 3;
		jnz  end;
		push ecx;
		push edx;
		push eax;
		call xfungetc;
		pop  ecx;
		retn;
end:
		jmp  fo::funcoffs::xungetc_;
	}
}

static int __fastcall xfread(sFile* file, int elsize, void* buf, int count) {
	for (int i = 0; i < count; i++) {
		if (file->openFile->pos + elsize > file->openFile->file->length) return i;

		memcpy(buf, &file->openFile->file->data[file->openFile->pos], elsize);
		file->openFile->pos += elsize;
	}
	return count;
}

static __declspec(naked) int asm_xfread(void* buf, int elsize, int count, sFile* file) {
	__asm {
		cmp  byte ptr [ecx], 3;
		jnz  end;
		push ebx;    // count
		push eax;    // buf
		call xfread; // ecx - file, edx - elsize
		retn;
end:
		jmp  fo::funcoffs::xfread_;
	}
}

static __declspec(naked) int asm_xfread_fix(void* buf, int elsize, int count, sFile* file) {
	__asm {
		cmp  byte ptr [ecx], 3;
		jnz  end;
		push ebx;    // count
		push eax;    // buf
		call xfread; // ecx - file, edx - elsize
		retn;
end:
		call fo::funcoffs::xfread_;
		// fix
		test eax, eax;
		jnz  skip;
		dec  eax;
skip:
		retn;
	}
}

static int __fastcall xfwrite(sFile* file, int elsize, const void* buf, int count) {
	return 0;
}

static __declspec(naked) int asm_xfwrite(const void* buf, int elsize, int count, sFile* file) {
	__asm {
		cmp  byte ptr [ecx], 3;
		jnz  end;
		push ebx;       // count
		push eax;       // buf
		call xfwrite;   // ecx - file, edx - elsize
		retn;
end:
		jmp  fo::funcoffs::xfwrite_;
	}
}

static int __stdcall xfseek(sFile* file, long pos, int origin) {
	switch(origin) {
		case 0:
			file->openFile->pos = pos;
			break;
		case 1:
			file->openFile->pos += pos;
			break;
		case 2:
			file->openFile->pos = file->openFile->file->length + pos;
			break;
	}
	return 0;
}

static __declspec(naked) int asm_xfseek(sFile* file, long pos, int origin) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		push ecx;
		push ebx;
		push edx;
		push eax;
		call xfseek;
		pop  ecx;
		retn;
end:
		jmp  fo::funcoffs::xfseek_;
	}
}

static long __stdcall xftell(sFile* file) {
	return file->openFile->pos;
}

static __declspec(naked) long asm_xftell(sFile* file) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		push edx;
		push ecx;
		push eax;
		call xftell;
		pop  ecx;
		pop  edx;
		retn;
end:
		jmp fo::funcoffs::xftell_;
	}
}

static void __stdcall xfrewind(sFile* file) {
	file->openFile->pos = 0;
}

static __declspec(naked) void asm_xfrewind(sFile* file) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		pushadc;
		push eax;
		call xfrewind;
		popadc;
		retn;
end:
		jmp  fo::funcoffs::xrewind_;
	}
}

static int __stdcall xfeof(sFile* file) {
	if (file->openFile->pos >= file->openFile->file->length) {
		return 1;
	}
	return 0;
}

static __declspec(naked) int asm_xfeof(sFile* file) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		push edx;
		push ecx;
		push eax;
		call xfeof;
		pop  ecx;
		pop  edx;
		retn;
end:
		jmp  fo::funcoffs::xfeof_;
	}
}

static int __stdcall xfilelength(sFile* file) {
	return file->openFile->file->length;
}

static __declspec(naked) int asm_xfilelength(sFile* file) {
	__asm {
		cmp  byte ptr [eax], 3;
		jnz  end;
		push edx;
		push ecx;
		push eax;
		call xfilelength;
		pop  ecx;
		pop  edx;
		retn;
end:
		jmp  fo::funcoffs::xfilelength_;
	}
}

static void FileSystemReset() {
	if (files.empty()) return;
	for (DWORD i = loadedFiles; i < files.size(); i++) {
		if (files[i].data) delete[] files[i].data;
	}
	if (!loadedFiles) {
		files.clear();
	} else {
		files.erase(files.begin() + loadedFiles, files.end() - 1);
	}
}

void FileSystem::Save(HANDLE h) {
	DWORD count = 0, unused;
	for (DWORD i = loadedFiles; i < files.size(); i++) {
		if (files[i].isSave && files[i].data) count++;
	}
	WriteFile(h, &count, 4, &unused, 0);
	for (DWORD i = loadedFiles; i < files.size(); i++) {
		if (files[i].isSave && files[i].data) {
			WriteFile(h, &files[i].length, 128 + 8, &unused, 0);
			WriteFile(h, files[i].data, files[i].length, &unused, 0);
		}
	}
}

static void FileSystemLoad() {
	char buf[MAX_PATH];
	GetSavePath(buf, "fs");

	HANDLE h = CreateFileA(buf, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (h != INVALID_HANDLE_VALUE) {
		DWORD count, read;
		ReadFile(h, &count, 4, &read, 0);
		if (read == 4) {
			for (DWORD i = 0; i < count; i++) {
				fsFile file;
				ReadFile(h, &file.length, 128 + 8, &read, 0);
				file.data = new BYTE[file.length];
				ReadFile(h, file.data, file.length, &read, 0);
				file.isSave = 1;
				files.push_back(file);
			}
		}
		CloseHandle(h);
	}
}

static __declspec(naked) void FSLoadHook() {
	static const DWORD LoadHookRetAddr = 0x47CCEE;
	__asm {
		pushadc;
		call FileSystemLoad;
		popadc;
		mov  esi, 1;
		jmp  LoadHookRetAddr;
	}
}

static void FileSystemOverride() {
	HookCalls(asm_xfclose, {0x4C5DBD, 0x4C5EA5, 0x4C5EB4});
	HookCalls(asm_xfopen, {0x4C5DA9, 0x4C5E16, 0x4C5EC8});

	HookCall(0x4C5F04, asm_xvfprintf);

	HookCalls(asm_xfgetc, {0x4C5F31, 0x4C5F64});
	HookCalls(asm_xfgets, {0x4C5F85, 0x4C5FD3});
	HookCalls(asm_xfputc, {0x4C5FE4, 0x4C61B5, 0x4C61E2, 0x4C61FE, 0x4C6479, 0x4C64B3, 0x4C64D2});

	HookCall(0x4C5FEC, asm_xfputs);
	HookCall(0x4C5FF4, asm_xfungetc);

	HookCalls(asm_xfread, {0x4C5E5C, 0x4C5E8A, 0x4C5E9E, 0x4C603D, 0x4C6076, 0x4C60AA});
	HookCall(0x4C6162, asm_xfread_fix); // with bug fix from BugFixes.cpp

	HookCall(0x4C60B8, asm_xfwrite);
	HookCall(0x4C60C0, asm_xfseek);
	HookCall(0x4C60C8, asm_xftell);
	HookCall(0x4C60D0, asm_xfrewind);
	HookCall(0x4C60D8, asm_xfeof);

	HookCalls(asm_xfilelength, {0x4C5DB4, 0x4C5E2D, 0x4C68BC});
}

static DWORD NewFile(fsFile &file, const char* path, int size) {
	strcpy_s(file.name, path);
	file.length = size;
	file.wpos = 0;
	file.isSave = 0;
	files.push_back(file);
	return files.size() - 1;
}

DWORD __stdcall FScreateFromData(const char* path, void* data, int size) {
	loadedFiles++;
	fsFile file;
	file.data = new BYTE[size];
	memcpy(file.data, data, size);

	return NewFile(file, path, size); // return file index in vector
}

DWORD __stdcall FScreate(const char* path, int size) {
	if (size > MAX_FILE_SIZE) return -1;  // size limit 10Mb
	for (DWORD i = 0; i < files.size(); i++) {
		if (!files[i].data) {
			files[i].data = new BYTE[size];
			strcpy_s(files[i].name, path);
			files[i].length = size;
			files[i].wpos = 0;
			return i;
		}
	}
	fsFile file;
	file.data = new BYTE[size];

	return NewFile(file, path, size); // return file index in vector
}

DWORD __stdcall FScopy(const char* path, const char* source) {
	int result = FSfind(path);
	if (result != -1) return result;

	DWORD fsize;
	sFile* file;

	const char* mode = "rb"; // was 'r'
	__asm {
		mov  eax, source;
		mov  edx, mode;
		call fo::funcoffs::xfopen_;
		mov  file, eax;
	}
	if (!file) return -1;

	__asm {
		mov  eax, file;
		call fo::funcoffs::xfilelength_;
		mov  fsize, eax;
	}

	BYTE* fdata = new BYTE[fsize];
	__asm {
		mov  eax, file;
		call fo::funcoffs::xfclose_;
		mov  eax, source;
		mov  edx, fdata;
		call fo::funcoffs::db_read_to_buf_;
	}

	fsFile* fsfile = nullptr;
	for (DWORD i = 0; i < files.size(); i++) {
		if (!files[i].data) {
			result = i;
			fsfile = &files[i];
			break;
		}
	}
	if (!fsfile) {
		files.push_back(fsFile());
		result = files.size() - 1;
		fsfile = &files.back();
	}
	fsfile->data = fdata;
	strcpy_s(fsfile->name, path);
	fsfile->length = fsize;
	fsfile->wpos = 0;
	fsfile->isSave = 0;

	return result; // return file index in vector
}

DWORD __stdcall FSfind(const char* path) {
	if (!*path) return -1;
	for (DWORD i = 0; i < files.size(); i++) {
		if (!_stricmp(files[i].name, path)) return i;
	}
	return -1;
}

void __stdcall FSwrite_byte(DWORD id, int data) {
	if (id >= files.size() || !files[id].data) return;
	if (files[id].wpos + 1 > files[id].length) return;
	files[id].data[files[id].wpos++] = (char)data;
}

void __stdcall FSwrite_short(DWORD id, int data) {
	if (id >= files.size() || !files[id].data) return;
	if (files[id].wpos + 2 > files[id].length) return;

	char* data2 = (char*)&data;
	data2++;
	// write & reverse bytes
	for (int i = 0; i < 2; i++) files[id].data[files[id].wpos++] = *(data2 - i);
}

void __stdcall FSwrite_int(DWORD id, int data) {
	if (id >= files.size() || !files[id].data) return;
	if (files[id].wpos + 4 > files[id].length) return;

	char* data2 = (char*)&data;
	data2 += 3;
	// write & reverse bytes
	for (int i = 0; i < 4; i++) files[id].data[files[id].wpos++] = *(data2 - i);
}

void __stdcall FSwrite_string(DWORD id, const char* data) {
	if (id >= files.size() || !files[id].data) return;
	if (files[id].wpos + strlen(data) + 1 > files[id].length) return;
	memcpy(&files[id].data[files[id].wpos], data, strlen(data) + 1);
	files[id].wpos += strlen(data) + 1;
}

void __stdcall FSwrite_bstring(DWORD id, const char* data) {
	if (id >= files.size() || !files[id].data) return;
	if (files[id].wpos + strlen(data) > files[id].length) return;
	memcpy(&files[id].data[files[id].wpos], data, strlen(data));
	files[id].wpos += strlen(data);
}

int __stdcall FSread_byte(DWORD id) {
	if (id >= files.size() || !files[id].data) return 0;
	if (files[id].wpos + 1 > files[id].length) return 0;
	return files[id].data[files[id].wpos++];
}

int __stdcall FSread_short(DWORD id) {
	if (id >= files.size() || !files[id].data) return 0;
	if (files[id].wpos + 2 > files[id].length) return 0;

	char data[2];
	data[1] = files[id].data[files[id].wpos++];
	data[0] = files[id].data[files[id].wpos++];

	return static_cast<int>(*(short*)data);
}

int __stdcall FSread_int(DWORD id) {
	if (id >= files.size() || !files[id].data) return 0;
	if (files[id].wpos + 4 > files[id].length) return 0;

	char data[4];
	for (int i = 3; i >= 0; i--) data[i] = files[id].data[files[id].wpos++];

	return *(int*)data;
}

void __stdcall FSdelete(DWORD id) {
	if (id >= files.size() || !files[id].data) return;
	files[id].length = 0;
	files[id].wpos = 0;
	files[id].name[0] = 0;
	delete[] files[id].data;
	files[id].data = 0;
}

DWORD __stdcall FSsize(DWORD id) {
	if (id >= files.size() || !files[id].data) return 0;
	return files[id].length;
}

DWORD __stdcall FSpos(DWORD id) {
	if (id >= files.size() || !files[id].data) return 0;
	return files[id].wpos;
}

void __stdcall FSseek(DWORD id, DWORD pos) {
	if (id >= files.size() || !files[id].data) return;
	if (pos > files[id].length) return;
	files[id].wpos = pos;
}

void __stdcall FSresize(DWORD id, DWORD size) {
	if (id >= files.size() || !files[id].data || (size != -1 && size > MAX_FILE_SIZE)) return; // size limit 10Mb
	if (size == -1) {
		files[id].isSave = 1;
		return;
	}
	BYTE* buf = files[id].data;
	files[id].data = new BYTE[size];
	CopyMemory(files[id].data, buf, min(files[id].length, size));
	files[id].length = size;
	files[id].wpos = 0;
	delete[] buf;
}

bool FileSystem::IsEmpty() {
	return (int)(files.size() - loadedFiles) <= 0;
}

////////////////////////////////////////////////////////////////////////////////

static void __stdcall OpenWarning() {
	MessageBoxA(0, "The open file limit is about to be reached.\n"
	               "It is recommended to restart the game to avoid critical errors.", "Warning", MB_TASKMODAL | MB_ICONWARNING);
}

static __declspec(naked) void sopen_hook_warning() {
	static const DWORD __NTAtMaxFiles_ = 0x4EB190;
	__asm {
		cmp  dword ptr ds:[FO_VAR_topFileID], 15;
		jg   warning;
		jmp  __NTAtMaxFiles_;
warning:
		call OpenWarning;
		jmp  __NTAtMaxFiles_;
	}
}

static void __stdcall OpenFail() {
	MessageBoxA(0, "Failed to open file.\nToo many open files.", 0, MB_TASKMODAL | MB_ICONERROR);
}

static __declspec(naked) void sopen_hook_error() {
	static const DWORD __set_errno_ = 0x4E11F5;
	__asm {
		call __set_errno_;
		jmp  OpenFail;
	}
}

#ifndef NDEBUG
static long openfiles = 0; // current number of open files

static void __stdcall OpenLog(const char* file, long id) {
	dlog_f(">> Open %s(%d) [%d]\n", 0, file, id, ++openfiles);
}

static void __stdcall CloseLog(long id) {
	dlog_f("<< Close id:%d [%d]\n", 0, id, openfiles--);
}

static __declspec(naked) void OpenFileLog() {
	__asm {
		pushadc;
		push eax;
		push ebp;
		call OpenLog;
		popadc;
		call dword ptr ds:[0x51E758]; // __InitAccessH
		retn;
	}
}

static __declspec(naked) void CloseFileLog() {
	__asm {
		pushadc;
		push eax;
		call CloseLog;
		popadc;
		retn;
	}
}
#endif

void FileSystem::init() {
	if (IniReader::GetConfigInt("Misc", "UseFileSystemOverride", 0)) {
		FileSystemOverride();
		UsingFileSystem = true;
	}
	MakeJump(0x47CCE2, FSLoadHook); // LoadGame_

	LoadGameHook::OnGameReset() += FileSystemReset;

	// Fix crash if the filename contains a '%' character (relevant to SFX files)
	// xfopen_ - remove sprintf_ function calls that do nothing (probably just checking the filename for the '%' formatting char?)
	BlockCall(0x4DEF12);
	BlockCall(0x4DEF84);

	// Warning message when there are more than 16 open files (max 20)
	HookCall(0x4EE0DE, sopen_hook_warning);
	// Error message "Failed to open file"
	HookCall(0x4EE0EC, sopen_hook_error);

	// DEV Testing: Check open/close file descriptors
	//MakeCall(0x4EE216, OpenFileLog, 1);
	//MakeCall(0x4EB814, CloseFileLog, 1);
}

}
