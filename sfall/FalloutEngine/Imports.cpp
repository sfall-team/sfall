#include "FalloutEngine.h"
#include "Imports.h"

int db_fclose(sFile* file) {
	int result;
	_asm {
		mov eax, file;
		call FuncOffs::db_fclose_;
		mov result, eax;
	}
	return result;
}
sFile* db_fopen(const char* path, const char* mode) {
	sFile* result;
	_asm {
		mov eax, path;
		mov edx, mode;
		call FuncOffs::db_fopen_;
		mov result, eax;
	}
	return result;
}
int db_fgetc(sFile* file) {
	int result;
	_asm {
		mov eax, file;
		call FuncOffs::db_fgetc_;
		mov result, eax;
	}
	return result;
}
char* db_fgets(char* buf, int max_count, sFile* file) {
	char* result;
	_asm {
		mov eax, buf;
		mov edx, max_count;
		mov ebx, file;
		call FuncOffs::db_fgets_;
		mov result, eax;
	}
	return result;
}
//int db_ungetc(int c, sFile* file);
int db_fread(void* buf, int elsize, int count, sFile* file) {
	int result;
	_asm {
		mov eax, buf;
		mov edx, elsize;
		mov ebx, count;
		mov ecx, file;
		call FuncOffs::db_fread_;
		mov result, eax;
	}
	return result;
}
int db_fseek(sFile* file, long pos, int origin) {
	int result;
	_asm {
		mov eax, file;
		mov edx, pos;
		mov ebx, origin;
		call FuncOffs::db_fseek_;
		mov result, eax;
	}
	return result;
}
//long db_ftell(sFile* file);
//void db_rewind(sFile* file);
//int db_feof(sFile* file);
int db_freadByte(sFile* file, __int8* _out) {
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call FuncOffs::db_freadByte_;
		mov result, eax;
	}
	return result;
}
int db_freadShort(sFile* file, __int16* _out) {
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call FuncOffs::db_freadShort_;
		mov result, eax;
	}
	return result;
}
int db_freadInt(sFile* file, __int32* _out) {
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call FuncOffs::db_freadInt_;
		mov result, eax;
	}
	return result;
}
int db_freadFloat(sFile* file, float* _out) {
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call FuncOffs::db_freadInt_;
		mov result, eax;
	}
	return result;
}
//int db_freadByteCount(sFile* file, BYTE* ptr, int count);
//int db_freadShortCount(sFile* file, WORD* ptr, int count);
//int db_freadIntCount(sFile* file, DWORD* ptr, int count);
//int db_freadFloatCount(sFile* file, float* ptr, int count);

int message_init(sMsgFile* file) {
	int result;
	_asm {
		mov eax, file;
		call FuncOffs::message_init_;
		mov result, eax;
	}
	return result;
}
int message_exit(sMsgFile* file) {
	int result;
	_asm {
		mov eax, file;
		call FuncOffs::message_exit_;
		mov result, eax;
	}
	return result;
}
int message_load(sMsgFile* file, char* path) {
	int result;
	_asm {
		mov eax, file;
		mov edx, path
		call FuncOffs::message_load_;
		mov result, eax;
	}
	return result;
}
int message_search(sMsgFile* file, sMessage* msg) {
	int result;
	_asm {
		mov eax, file;
		mov edx, msg;
		call FuncOffs::message_search_;
		mov result, eax;
	}
	return result;
}
int message_make_path(char* outpath, char* path) {
	int result;
	_asm {
		mov eax, outpath;
		mov edx, path;
		call FuncOffs::message_make_path_;
		mov result, eax;
	}
	return result;
}
int message_find(sMsgFile* file, DWORD number, DWORD* _out) {
	int result;
	_asm {
		mov eax, file;
		mov edx, number;
		mov ebx, _out;
		call FuncOffs::message_find_;
		mov result, eax;
	}
	return result;
}
int message_add(sMsgFile* file, sMessage* msg) {
	int result;
	_asm {
		mov eax, file;
		mov edx, msg;
		call FuncOffs::message_add_;
		mov result, eax;
	}
	return result;
}
char* getmsg(sMsgFile* file, sMessage* msg, DWORD num) {
	char* result;
	_asm {
		mov eax, file;
		mov edx, msg;
		mov ebx, num;
		call FuncOffs::getmsg_;
		mov result, eax;
	}
	return result;
}
int message_filter(sMsgFile* file) {
	int result;
	_asm {
		mov eax, file;
		call FuncOffs::message_filter_;
		mov result, eax;
	}
	return result;
}

void get_input() {
	_asm {
		call FuncOffs::get_input_;
	}
}
