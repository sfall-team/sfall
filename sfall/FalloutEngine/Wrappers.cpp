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

#include "Functions.h"
#include "Wrappers.h"

namespace Wrapper
{

// Fallout2.exe was compiled using WATCOM compiler, which uses Watcom register calling convention.
// In this convention, up to 4 arguments are passed via registers in this order: EAX, EDX, EBX, ECX.

#define WRAP_WATCOM_CALL0(offs) \
	__asm call FuncOffs::offs

#define WRAP_WATCOM_CALL1(offs, arg1) \
	__asm mov eax, arg1				\
	WRAP_WATCOM_CALL0(offs)

#define WRAP_WATCOM_CALL2(offs, arg1, arg2) \
	__asm mov edx, arg2				\
	WRAP_WATCOM_CALL1(offs, arg1)

#define WRAP_WATCOM_CALL3(offs, arg1, arg2, arg3) \
	__asm mov ebx, arg3				\
	WRAP_WATCOM_CALL2(offs, arg1, arg2)

#define WRAP_WATCOM_CALL4(offs, arg1, arg2, arg3, arg4) \
	__asm mov ecx, arg4						\
	WRAP_WATCOM_CALL3(offs, arg1, arg2, arg3)

#define WRAP_WATCOM_CALL5(offs, arg1, arg2, arg3, arg4, arg5) \
	push arg5				\
	WRAP_WATCOM_CALL4(offs, arg1, arg2, arg3, arg4)

#define WRAP_WATCOM_CALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6) \
	push arg6				\
	WRAP_WATCOM_CALL5(offs, arg1, arg2, arg3, arg4, arg5)

#define WRAP_WATCOM_CALL7(offs, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
	push arg7				\
	WRAP_WATCOM_CALL6(offs, arg1, arg2, arg3, arg4, arg5, arg6)

bool __stdcall art_exists(int artFid) {
	WRAP_WATCOM_CALL1(art_exists_, artFid)
}

// Returns the name of the critter
const char* __stdcall critter_name(TGameObj* critter) {
	WRAP_WATCOM_CALL1(critter_name_, critter)
}

// Change the name of playable character
void critter_pc_set_name(const char* newName) {
	WRAP_WATCOM_CALL1(critter_pc_set_name_, newName)
}

bool __stdcall db_access(const char* fileName) {
	WRAP_WATCOM_CALL1(db_access_, fileName)
}

int __stdcall db_fclose(DBFile* file) {
	WRAP_WATCOM_CALL1(db_fclose_, file)
}

DBFile* __stdcall db_fopen(const char* path, const char* mode) {
	WRAP_WATCOM_CALL2(db_fopen_, path, mode)
}

int __stdcall db_fgetc(DBFile* file) {
	WRAP_WATCOM_CALL1(db_fgetc_, file)
}

char* __stdcall db_fgets(char* buf, int max_count, DBFile* file) {
	WRAP_WATCOM_CALL3(db_fgets_, buf, max_count, file)
}

int __stdcall db_fread(void* buf, int elsize, int count, DBFile* file) {
	WRAP_WATCOM_CALL4(db_fread_, buf, elsize, count, file)
}

int __stdcall db_fseek(DBFile* file, long pos, int origin) {
	WRAP_WATCOM_CALL3(db_fseek_, file, pos, origin)
}

int __stdcall db_freadByte(DBFile* file, __int8* _out) {
	WRAP_WATCOM_CALL2(db_freadByte_, file, _out)
}

int __stdcall db_freadByteCount(DBFile* file, void* cptr, int count) {
	WRAP_WATCOM_CALL3(db_freadByteCount_, file, cptr, count)
}

int __stdcall db_freadShort(DBFile* file, __int16* _out) {
	WRAP_WATCOM_CALL2(db_freadShort_, file, _out)
}

int __stdcall db_freadInt(DBFile* file, __int32* _out) {
	WRAP_WATCOM_CALL2(db_freadInt_, file, _out)
}

void __stdcall db_free_file_list(char* * *fileList, DWORD arg2) {
	WRAP_WATCOM_CALL2(db_free_file_list_, fileList, arg2)
}

int __stdcall db_fwriteByteCount(DBFile* file, void* cptr, int count) {
	WRAP_WATCOM_CALL3(db_fwriteByteCount_, file, cptr, count)
}

int __stdcall db_get_file_list(const char* searchMask, char* * *fileList, DWORD arg3, DWORD arg4) {
	WRAP_WATCOM_CALL4(db_get_file_list_, searchMask, fileList, arg3, arg4)
}

// prints message to debug.log file
void __declspec(naked) debug_printf(const char* fmt, ...) {
	__asm jmp FuncOffs::debug_printf_
}

// Displays message in main UI console window
void display_print(const char* msg) {
	WRAP_WATCOM_CALL1(display_print_, msg)
}

void executeProcedure(TProgram* sptr, int procNum) {
	WRAP_WATCOM_CALL2(executeProcedure_, sptr, procNum)
}

int __stdcall get_input() {
	WRAP_WATCOM_CALL0(get_input_)
}

// returns the name of current procedure by program pointer
const char* __stdcall findCurrentProc(TProgram* program) {
	WRAP_WATCOM_CALL1(findCurrentProc_, program)
}

const char* _stdcall getmsg(const MessageList* fileAddr, MessageNode* result, int messageId) {
	WRAP_WATCOM_CALL3(getmsg_, fileAddr, result, messageId)
}

void __stdcall gsound_play_sfx_file(const char* name) {
	WRAP_WATCOM_CALL1(gsound_play_sfx_file_, name)
}

// redraws the main game interface windows (useful after changing some data like active hand, etc.)
void intface_redraw() {
	WRAP_WATCOM_CALL0(intface_redraw_)
}

int __stdcall interpret(TProgram* program, int arg2) {
	WRAP_WATCOM_CALL2(interpret_, program, arg2)
}

int __stdcall interpretFindProcedure(TProgram* scriptPtr, const char* procName) {
	WRAP_WATCOM_CALL2(interpretFindProcedure_, scriptPtr, procName)
}

// pops value type from Data stack (must be followed by InterpretPopLong)
DWORD __stdcall interpretPopShort(TProgram* scriptPtr) {
	WRAP_WATCOM_CALL1(interpretPopShort_, scriptPtr)
}

// pops value from Data stack (must be preceded by InterpretPopShort)
DWORD __stdcall interpretPopLong(TProgram* scriptPtr) {
	WRAP_WATCOM_CALL1(interpretPopLong_, scriptPtr)
}

// pushes value to Data stack (must be followed by InterpretPushShort)
void __stdcall interpretPushLong(TProgram* scriptPtr, DWORD val) {
	WRAP_WATCOM_CALL2(interpretPushLong_, scriptPtr, val)
}

// pushes value type to Data stack (must be preceded by InterpretPushLong)
void __stdcall interpretPushShort(TProgram* scriptPtr, DWORD valType) {
	WRAP_WATCOM_CALL2(interpretPushShort_, scriptPtr, valType)
}

DWORD __stdcall interpretAddString(TProgram* scriptPtr, const char* strval) {
	WRAP_WATCOM_CALL2(interpretAddString_, scriptPtr, strval)
}

const char* __stdcall interpretGetString(TProgram* scriptPtr, DWORD dataType, DWORD strId) {
	WRAP_WATCOM_CALL3(interpretGetString_, scriptPtr, dataType, strId)
}

// prints scripting error in debug.log and stops current script execution by performing longjmp
// USE WITH CAUTION
void __declspec(naked) interpretError(const char* fmt, ...) {
	__asm jmp FuncOffs::interpretError_
}

int _stdcall isPartyMember(TGameObj* obj) {
	WRAP_WATCOM_CALL1(isPartyMember_, obj)
}

int __stdcall item_get_type(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_get_type_, item)
}

int __stdcall item_m_dec_charges(TGameObj* item) {
	WRAP_WATCOM_CALL1(item_m_dec_charges_, item) //Returns -1 if the item has no charges
}

TGameObj* __stdcall inven_pid_is_carried_ptr(TGameObj* invenObj, int pid) {
	WRAP_WATCOM_CALL2(inven_pid_is_carried_ptr_, invenObj, pid)
}

// critter worn item (armor)
TGameObj* __stdcall inven_worn(TGameObj* critter) {
	WRAP_WATCOM_CALL1(inven_worn_, critter)
}

// item in critter's left hand slot
TGameObj* __stdcall inven_left_hand(TGameObj* critter) {
	WRAP_WATCOM_CALL1(inven_left_hand_, critter)
}

// item in critter's right hand slot
TGameObj* __stdcall inven_right_hand(TGameObj* critter) {
	WRAP_WATCOM_CALL1(inven_right_hand_, critter)
}

TProgram* __stdcall loadProgram(const char* fileName) {
	WRAP_WATCOM_CALL1(loadProgram_, fileName)
}

void* __stdcall mem_realloc(void* lpmem, DWORD msize) {
	WRAP_WATCOM_CALL2(mem_realloc_, lpmem, msize)
}

int __stdcall message_add(MessageList* file, MessageNode* msg) {
	WRAP_WATCOM_CALL2(message_add_, file, msg)
}

int __stdcall message_filter(MessageList* file) {
	WRAP_WATCOM_CALL1(message_filter_, file)
}

int __stdcall message_make_path(char* outpath, char* path) {
	WRAP_WATCOM_CALL2(message_make_path_, outpath, path)
}

int __stdcall message_search(MessageList* file, MessageNode* msg) {
	WRAP_WATCOM_CALL2(message_search_, file, msg)
}

int __stdcall message_load(MessageList *msgList, const char *msgFilePath) {
	WRAP_WATCOM_CALL2(message_load_, msgList, msgFilePath)
}

int __stdcall message_exit(MessageList *msgList) {
	WRAP_WATCOM_CALL1(message_exit_, msgList)
}

TGameObj* __stdcall obj_find_first_at_tile(int elevation, int tileNum) {
	WRAP_WATCOM_CALL2(obj_find_first_at_tile_, elevation, tileNum)
}

TGameObj* __stdcall obj_find_next_at_tile() {
	WRAP_WATCOM_CALL0(obj_find_next_at_tile_)
}

int _stdcall partyMemberGetCurLevel(TGameObj* obj) {
	WRAP_WATCOM_CALL1(partyMemberGetCurLevel_, obj)
}

int _stdcall perk_level(TGameObj* critter, int perkId) {
	WRAP_WATCOM_CALL2(perk_level_, critter, perkId)
}

int proto_ptr(int pid, sProtoBase* *ptrPtr) {
	WRAP_WATCOM_CALL2(proto_ptr_, pid, ptrPtr)
}

DWORD* __stdcall runProgram(TProgram* progPtr) {
	WRAP_WATCOM_CALL1(runProgram_, progPtr)
}

TScript* __stdcall scr_find_first_at(int elevation) {
	WRAP_WATCOM_CALL1(scr_find_first_at_, elevation)
}

TScript* __stdcall scr_find_next_at() {
	WRAP_WATCOM_CALL0(scr_find_next_at_)
}

TGameObj* __stdcall scr_find_obj_from_program(TProgram* program) {
	WRAP_WATCOM_CALL1(scr_find_obj_from_program_, program)
}

// Saves pointer to script object into scriptPtr using scriptID. 
// Returns 0 on success, -1 on failure.
int __stdcall scr_ptr(int scriptId, TScript** scriptPtr) {
	WRAP_WATCOM_CALL2(scr_ptr_, scriptId, scriptPtr)
}

void skill_get_tags(int* result, int num) {
	WRAP_WATCOM_CALL2(skill_get_tags_, result, num)
}

void skill_set_tags(int* tags, int num) {
	WRAP_WATCOM_CALL2(skill_set_tags_, tags, num)
}

int __stdcall stat_level(TGameObj* critter, int statId) {
	WRAP_WATCOM_CALL2(stat_level_, critter, statId)
}

int __stdcall win_register_button(DWORD winRef, int xPos, int yPos, int width, int height, int hoverOn, int hoverOff, int buttonDown, int buttonUp, int pictureUp, int pictureDown, int arg12, int buttonType) {
	__asm {
		push buttonType
		push arg12
		push pictureDown
		push pictureUp
		push buttonUp
		push buttonDown
		push hoverOff
		push hoverOn
		push height
		mov ecx, width
		mov ebx, yPos
		mov edx, xPos
		mov eax, winRef
		call FuncOffs::win_register_button_
	}
}


#define WRAP_WATCOM_FUNC0(retType, name) \
	retType __stdcall name() { \
		WRAP_WATCOM_CALL0(name##_) \
	}

#define WRAP_WATCOM_FUNC1(retType, name, arg1t, arg1) \
	retType __stdcall name(arg1t arg1) { \
		WRAP_WATCOM_CALL1(name##_, arg1) \
	}

#define WRAP_WATCOM_FUNC2(retType, name, arg1t, arg1, arg2t, arg2) \
	retType __stdcall name(arg1t arg1, arg2t arg2) { \
		WRAP_WATCOM_CALL2(name##_, arg1, arg2) \
	}

#define WRAP_WATCOM_FUNC3(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3) { \
		WRAP_WATCOM_CALL3(name##_, arg1, arg2, arg3) \
	}

#define WRAP_WATCOM_FUNC4(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t, arg4) { \
		WRAP_WATCOM_CALL4(name##_, arg1, arg2, arg3, arg4) \
	}

#define WRAP_WATCOM_FUNC5(retType, name, arg1t, arg1, arg2t, arg2, arg3t, arg3, arg4t, arg4, arg5t, arg5) \
	retType __stdcall name(arg1t arg1, arg2t arg2, arg3t arg3, arg4t, arg4, arg5t, arg5) { \
		WRAP_WATCOM_CALL5(name##_, arg1, arg2, arg3, arg4, arg5) \
	}

#include "Wrappers_def.h"

}
