#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct sFile {
	DWORD fileType;
	void* handle;
};
struct sMessage {
	int number;
	DWORD flags;
	char* audio;
	char* message;
};
struct sMsgFile {
	DWORD count;
	sMessage* messages;
};

int db_fclose(sFile* file);
sFile* db_fopen(const char* path, const char* mode);
int db_fgetc(sFile* file);
char* db_fgets(char* buf, int max_count, sFile* file);
int db_ungetc(int c, sFile* file);
int db_fread(void* buf, int elsize, int count, sFile* file);
int db_fseek(sFile* file, long pos, int origin);
long db_ftell(sFile* file);
void db_rewind(sFile* file);
int db_feof(sFile* file);
int db_freadByte(sFile* file, __int8* out);
int db_freadShort(sFile* file, __int16* out);
int db_freadInt(sFile* file, __int32* out);
int db_freadFloat(sFile* file, float* out);
int db_freadByteCount(sFile* file, BYTE* ptr, int count);
int db_freadShortCount(sFile* file, WORD* ptr, int count);
int db_freadIntCount(sFile* file, DWORD* ptr, int count);
int db_freadFloatCount(sFile* file, float* ptr, int count);

int message_init(sMsgFile* file);
int message_exit(sMsgFile* file);
int message_load(sMsgFile* file, char* path);
int message_search(sMsgFile* file, sMessage* msg);
int message_make_path(char* outpath, char* path);
int message_find(sMsgFile* file, DWORD number, DWORD* out);
int message_add(sMsgFile* file, sMessage* msg);
char* getmsg(sMsgFile* file, sMessage* msg, DWORD num);
int message_filter(sMsgFile* file);

void get_input();