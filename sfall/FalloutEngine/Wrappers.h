/*
* sfall
* Copyright (C) 2008-2016 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "..\CommonTypes.h"
#include "Structs.h"

// 
// WRAPPERS for FO engine functions. 
// Use those as you would if there were source code for the engine...
//
namespace Wrapper
{

bool __stdcall art_exists(int artFid);

// Returns the name of the critter
const char* __stdcall critter_name(TGameObj* critter);

// Change the name of playable character
void critter_pc_set_name(const char* newName);

// Checks if given file exist in DB
bool __stdcall db_access(const char* fileName);

int __stdcall db_fclose(DBFile* file);

DBFile* __stdcall db_fopen(const char* path, const char* mode);

int __stdcall db_fgetc(DBFile* file);

char* __stdcall db_fgets(char* buf, int max_count, DBFile* file);

//int db_ungetc(int c, DBFile* file);
int __stdcall db_fread(void* buf, int elsize, int count, DBFile* file);

int __stdcall db_fseek(DBFile* file, long pos, int origin);

int __stdcall db_freadByte(DBFile* file, __int8* _out);

int __stdcall db_freadByteCount(DBFile* file, void* cptr, int count);

int __stdcall db_freadShort(DBFile* file, __int16* _out);

int __stdcall db_freadInt(DBFile* file, __int32* _out);

// destroys filelist array created by db_get_file_list
void __stdcall db_free_file_list(char* * *fileList, DWORD arg2);

int __stdcall db_fwriteByteCount(DBFile* file, void* cptr, int count);

// searches files in DB by given path/filename mask and stores result in fileList
// fileList is a pointer to a variable, that will be assigned with an address of an array of char* strings
// returns number of elements in *fileList
int __stdcall db_get_file_list(const char* searchMask, char* * *fileList, DWORD arg3, DWORD arg4);

// prints message to debug.log file
void __declspec() debug_printf(const char* fmt, ...);

// Displays message in main UI console window
void display_print(const char* msg);

// execute script proc by internal proc number (from script's proc table, basically a sequential number of a procedure as defined in code, starting from 1)
void executeProcedure(TProgram* sptr, int procNum);

int __stdcall get_input();

int _stdcall isPartyMember(TGameObj* obj);

int __stdcall item_get_type(TGameObj* item);

// searches for message ID in given message file and places result in @result
const char* _stdcall getmsg(const MessageList* fileAddr, MessageNode* result, int messageId);

// plays SFX sound with given name 
void __stdcall gsound_play_sfx_file(const char* name);

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void intface_redraw();

int __stdcall interpret(TProgram* program, int arg2);

// finds procedure ID for given script program pointer and procedure name
int __stdcall interpretFindProcedure(TProgram* scriptPtr, const char* procName);

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall interpretPopShort(TProgram* scriptPtr);

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall interpretPopLong(TProgram* scriptPtr);

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall interpretPushLong(TProgram* scriptPtr, DWORD val);

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall interpretPushShort(TProgram* scriptPtr, DWORD valType);

const char* __stdcall interpretGetString(TProgram* scriptPtr, DWORD dataType, DWORD strId);

DWORD __stdcall interpretAddString(TProgram* scriptPtr, const char* str);

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec() interpretError(const char* fmt, ...);

// Returns 0 on success, -1 if the item has no charges
int __stdcall item_m_dec_charges(TGameObj* item);

TGameObj* __stdcall inven_pid_is_carried_ptr(TGameObj* invenObj, int pid);

// critter worn item (armor)
TGameObj* __stdcall inven_worn(TGameObj* critter);

// item in critter's left hand slot
TGameObj* __stdcall inven_left_hand(TGameObj* critter);

// item in critter's right hand slot
TGameObj* __stdcall inven_right_hand(TGameObj* critter);

// returns the name of current procedure by program pointer
const char* __stdcall findCurrentProc(TProgram* program);

TProgram* __stdcall loadProgram(const char* fileName);

void* __stdcall mem_realloc(void* lpmem, DWORD msize);

int __stdcall message_add(MessageList* file, MessageNode* msg);

int __stdcall message_filter(MessageList* file);

int __stdcall message_make_path(char* outpath, char* path);

int __stdcall message_search(MessageList* file, MessageNode* msg);

// loads MSG file into given MessageList
int __stdcall message_load(MessageList *msgList, const char *msgFilePath);

// destroys message list
int __stdcall message_exit(MessageList *msgList);

TGameObj* __stdcall obj_find_first_at_tile(int elevation, int tileNum);

TGameObj* __stdcall obj_find_next_at_tile();

int _stdcall partyMemberGetCurLevel(TGameObj* obj);

int _stdcall perk_level(TGameObj* critter, int perkId);

// places pointer to a prototype structure into ptrPtr and returns 0 on success or -1 on failure
int proto_ptr(int pid, sProtoBase* *ptrPtr);

DWORD* __stdcall runProgram(TProgram* progPtr);

TScript* __stdcall scr_find_first_at(int elevation);

TScript* __stdcall scr_find_next_at();

TGameObj* __stdcall scr_find_obj_from_program(TProgram* program);

// Saves pointer to script object into scriptPtr using scriptID. 
// Returns 0 on success, -1 on failure.
int __stdcall scr_ptr(int scriptId, TScript** scriptPtr);

void skill_get_tags(int* result, int num);
void skill_set_tags(int* tags, int num);

int __stdcall stat_level(TGameObj* critter, int statId);

int __stdcall win_register_button(DWORD winRef, int xPos, int yPos, int width, int height, int hoverOn, int hoverOff, int buttonDown, int buttonUp, int pictureUp, int pictureDown, int arg12, int buttonType);



#define WRAP_WATCOM_FUNC0(retType, name) \
	retType __stdcall name();

#define WRAP_WATCOM_FUNC1(retType, name, arg1t, arg1) \
	retType __stdcall name(arg1t arg1);

#define WRAP_WATCOM_FUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __stdcall name(arg1t arg1, arg2t arg2);

#define WRAP_WATCOM_FUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3);

#define WRAP_WATCOM_FUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t, arg4);

#define WRAP_WATCOM_FUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t, arg4, arg5t, arg5);

#include "Wrappers_def.h"

#undef WRAP_WATCOM_FUNC0
#undef WRAP_WATCOM_FUNC1
#undef WRAP_WATCOM_FUNC2
#undef WRAP_WATCOM_FUNC3
#undef WRAP_WATCOM_FUNC4
#undef WRAP_WATCOM_FUNC5
//#undef WRAP_WATCOM_FUNC6
//#undef WRAP_WATCOM_FUNC7

}
