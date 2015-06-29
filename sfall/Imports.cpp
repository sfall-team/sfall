#include "Imports.h"

int db_fclose(sFile* file) {
	DWORD addr=0x4C5EB4;
	int result;
	_asm {
		mov eax, file;
		call addr;
		mov result, eax;
	}
	return result;
}
sFile* db_fopen(const char* path, const char* mode) {
	DWORD addr=0x4C5EC8;
	sFile* result;
	_asm {
		mov eax, path;
		mov edx, mode;
		call addr;
		mov result, eax;
	}
	return result;
}
int db_fgetc(sFile* file) {
	DWORD addr=0x4C5F24;
	int result;
	_asm {
		mov eax, file;
		call addr;
		mov result, eax;
	}
	return result;
}
char* db_fgets(char* buf, int max_count, sFile* file) {
	DWORD addr=0x4C5F70;
	char* result;
	_asm {
		mov eax, buf;
		mov edx, max_count;
		mov ebx, file;
		call addr;
		mov result, eax;
	}
	return result;
}
//int db_ungetc(int c, sFile* file);
int db_fread(void* buf, int elsize, int count, sFile* file) {
	DWORD addr=0x4C5FFC;
	int result;
	_asm {
		mov eax, buf;
		mov edx, elsize;
		mov ebx, count;
		mov ecx, file;
		call addr;
		mov result, eax;
	}
	return result;
}
int db_fseek(sFile* file, long pos, int origin) {
	DWORD addr=0x4C60C0;
	int result;
	_asm {
		mov eax, file;
		mov edx, pos;
		mov ebx, origin;
		call addr;
		mov result, eax;
	}
	return result;
}
//long db_ftell(sFile* file);
//void db_rewind(sFile* file);
//int db_feof(sFile* file);
int db_freadByte(sFile* file, __int8* _out) {
	DWORD addr=0x4C60E0;
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call addr;
		mov result, eax;
	}
	return result;
}
int db_freadShort(sFile* file, __int16* _out) {
	DWORD addr=0x4C60F4;
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call addr;
		mov result, eax;
	}
	return result;
}
int db_freadInt(sFile* file, __int32* _out) {
	DWORD addr=0x4C614C;
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call addr;
		mov result, eax;
	}
	return result;
}
int db_freadFloat(sFile* file, float* _out) {
	DWORD addr=0x4C614C;
	int result;
	_asm {
		mov eax, file;
		mov edx, _out;
		call addr;
		mov result, eax;
	}
	return result;
}
//int db_freadByteCount(sFile* file, BYTE* ptr, int count);
//int db_freadShortCount(sFile* file, WORD* ptr, int count);
//int db_freadIntCount(sFile* file, DWORD* ptr, int count);
//int db_freadFloatCount(sFile* file, float* ptr, int count);

int message_init(sMsgFile* file) {
	DWORD addr=0x48494C;
	int result;
	_asm {
		mov eax, file;
		call addr;
		mov result, eax;
	}
	return result;
}
int message_exit(sMsgFile* file) {
	DWORD addr=0x484964;
	int result;
	_asm {
		mov eax, file;
		call addr;
		mov result, eax;
	}
	return result;
}
int message_load(sMsgFile* file, char* path) {
	DWORD addr=0x484AA4;
	int result;
	_asm {
		mov eax, file;
		mov edx, path
		call addr;
		mov result, eax;
	}
	return result;
}
int message_search(sMsgFile* file, sMessage* msg) {
	DWORD addr=0x484C30;
	int result;
	_asm {
		mov eax, file;
		mov edx, msg;
		call addr;
		mov result, eax;
	}
	return result;
}
int message_make_path(char* outpath, char* path) {
	DWORD addr=0x484CB8;
	int result;
	_asm {
		mov eax, outpath;
		mov edx, path;
		call addr;
		mov result, eax;
	}
	return result;
}
int message_find(sMsgFile* file, DWORD number, DWORD* _out) {
	DWORD addr=0x484D10;
	int result;
	_asm {
		mov eax, file;
		mov edx, number;
		mov ebx, _out;
		call addr;
		mov result, eax;
	}
	return result;
}
int message_add(sMsgFile* file, sMessage* msg) {
	DWORD addr=0x484D68;
	int result;
	_asm {
		mov eax, file;
		mov edx, msg;
		call addr;
		mov result, eax;
	}
	return result;
}
char* getmsg(sMsgFile* file, sMessage* msg, DWORD num) {
	DWORD addr=0x48504C;
	char* result;
	_asm {
		mov eax, file;
		mov edx, msg;
		mov ebx, num;
		call addr;
		mov result, eax;
	}
	return result;
}
int message_filter(sMsgFile* file) {
	DWORD addr=0x485078;
	int result;
	_asm {
		mov eax, file;
		call addr;
		mov result, eax;
	}
	return result;
}

void get_input() {
	DWORD addr=0x4C8B78;
	_asm {
		call addr;
	}
}