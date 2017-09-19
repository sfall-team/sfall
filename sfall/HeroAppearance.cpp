/*
 *    sfall
 *    Copyright (C) 2009, 2010  Mash (Matt Wells, mashw at bigpond dot net dot au)
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

#include <stdio.h>

#include "main.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "HeroAppearance.h"
#include "Message.h"
#include "ScriptExtender.h"

bool AppModEnabled = false; //check if Appearance mod enabled for script fuctions

//char scrn surfaces
BYTE *NewButt01Surface = NULL;
BYTE *CharScrnBackSurface = NULL;

//char scrn critter rotation vars
DWORD CharRotTick = 0;
DWORD CharRotOri = 0;

int CurrentRaceVal = 0, CurrentStyleVal = 0; //holds Appearance values to restore after global reset in NewGame2 fuction in LoadGameHooks.cpp
DWORD critterListSize = 0, critterArraySize = 0; //Critter art list size

//fallout2 path node structure
struct sPath {
	char* path;
	void* pDat;
	DWORD isDat;
	sPath* next;
};

sPath **TempPathPtr = (sPath**)_paths;
sPath *HeroPathPtr = NULL;
sPath *RacePathPtr = NULL;

//for word wrapping
typedef struct LINENode {
	DWORD offset;
	LINENode *next;

	LINENode() {
		next = NULL;
		offset = 0;
	}
} LINENode;

//structures for holding frms loaded with fallout2 functions
#pragma pack(2)
typedef class FRMframe {
	public:
	WORD width;
	WORD height;
	DWORD size;
	WORD x;
	WORD y;
} FRMframe;

typedef class FRMhead {
	public:
	DWORD version; //version num
	WORD FPS; //frames per sec
	WORD actionFrame;
	WORD numFrames; //number of frames per direction
	WORD xCentreShift[6]; //offset from frm centre +=right -=left
	WORD yCentreShift[6]; //offset from frm centre +=down -=up
	DWORD oriOffset[6]; //frame area offset for diff orientations
	DWORD frameAreaSize;
} FRMhead;
#pragma pack()

//structures for loading unlisted frms
typedef class UNLSTDframe {
	public:
	WORD width;
	WORD height;
	DWORD size;
	WORD x;
	WORD y;
	BYTE *indexBuff;
	UNLSTDframe() {
		width = 0;
		height = 0;
		size = 0;
		x = 0;
		y = 0;
		indexBuff = NULL;
	}
	~UNLSTDframe() {
		if (indexBuff != NULL)
		delete[] indexBuff;
	}
} UNLSTDframe;

typedef class UNLSTDfrm {
	public:
	DWORD version;
	WORD FPS;
	WORD actionFrame;
	WORD numFrames;
	WORD xCentreShift[6];
	WORD yCentreShift[6];
	DWORD oriOffset[6];
	DWORD frameAreaSize;
	UNLSTDframe *frames;
	UNLSTDfrm() {
		version = 0;
		FPS = 0;
		actionFrame = 0;
		numFrames = 0;
		for (int i = 0; i < 6; i++) {
			xCentreShift[i] = 0;
			yCentreShift[i] = 0;
			oriOffset[i] = 0;
		}
		frameAreaSize = 0;
		frames = NULL;
	}
	~UNLSTDfrm() {
		if (frames != NULL)
		delete[] frames;
	}
} UNLSTDfrm;

/////////////////////////////////////////////////////////////////FILE FUNCTIONS////////////////////////////////////////////////////////////////////////

//--------------------------------------------------
void* FOpenFile(char *FileName, char *flags) {
	void *retVal;
	__asm {
		mov edx, flags
		mov eax, FileName
		call xfopen_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FCloseFile(void *FileStream) {
	int retVal;
	__asm {
		mov eax, FileStream
		call db_fclose_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int Ffseek(void *FileStream, long fOffset, int origin) {
	int retVal;
	__asm {
		mov ebx, origin
		mov edx, fOffset
		mov eax, FileStream
		call xfseek_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FReadByte(void *FileStream, BYTE *toMem) {
	int retVal;
	__asm {
		mov edx, toMem
		mov eax, FileStream
		call db_freadByte_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FReadWord(void *FileStream, WORD *toMem) {
	int retVal;
	__asm {
		mov edx, toMem
		mov eax, FileStream
		call db_freadShort_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FReadDword(void *FileStream, DWORD *toMem) {
	int retVal;
	__asm {
		mov edx, toMem
		mov eax, FileStream
		call db_freadInt_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FReadWordArray(void *FileStream, WORD *toMem, DWORD NumElements) {
	int retVal;
	__asm {
		mov ebx, NumElements
		mov edx, toMem
		mov eax, FileStream
		call db_freadShortCount_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FReadDwordArray(void *FileStream, DWORD *toMem, DWORD NumElements) {
	int retVal;
	__asm {
		mov ebx, NumElements
		mov edx, toMem
		mov eax, FileStream
		call db_freadIntCount_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FReadString(void *FileStream, char *toMem, DWORD charLength, DWORD NumStrings) {
	int retVal;
	__asm {
		mov ecx, FileStream
		mov ebx, NumStrings
		mov edx, charLength
		mov eax, toMem
		call db_fread_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FWriteByte(void *FileStream, BYTE bVal) {
	int retVal;
	__asm {
		xor edx, edx
		mov dl, bVal
		mov eax, FileStream
		call db_fwriteByte_
		mov retVal, eax
	}
	return retVal;
}

//--------------------------------------------------
int FWriteDword(void *FileStream, DWORD bVal) {
	int retVal;
	__asm {
		mov edx, bVal
		mov eax, FileStream
		call db_fwriteInt_
		mov retVal, eax
	}
	return retVal;
}

/////////////////////////////////////////////////////////////////MOUSE FUNCTIONS////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------
//get current mouse pic ref
int GetMousePic() {
	return *(DWORD*)_gmouse_current_cursor;
}

//-----------------------------------------------------
//set mouse pic
int SetMousePic(int picNum) {
	__asm {
		mov eax, picNum
		call gmouse_set_cursor_
		mov picNum, eax
	}
return picNum; //0 = success, -1 = fail
}

//--------------------------------------------------
void GetMousePos(int *x_out, int *y_out) {
	__asm {
		push esi
		mov edx, y_out
		mov eax, x_out
		call mouse_get_position_
		pop esi
	}
}

//-----------------------------------------------------
void ShowMouse() {
	__asm {
		call mouse_show_
	}
}

//-----------------------------------------------------
void HideMouse() {
	__asm {
		call mouse_hide_
	}
}

//-------------------------------------------------------
//returns 0 if mouse is hidden
int IsMouseHidden() {
	return *(DWORD*)_mouse_is_hidden;
}

/////////////////////////////////////////////////////////////////FRM FUNCTIONS////////////////////////////////////////////////////////////////////////

//--------------------------------------------------
DWORD LoadFrm(DWORD LstRef, DWORD LstNum) {
	DWORD FrmID;
	__asm {
		push 0
		xor ecx, ecx
		xor ebx, ebx
		mov edx, LstNum
		mov eax, LstRef
		call art_id_
		mov FrmID, eax
	}
	return FrmID;
}

//---------------------------------------------
void UnloadFrm(DWORD FrmObj) {
	__asm {
		mov eax, FrmObj
		call art_ptr_unlock_
	}
}

//--------------------------------------------------------
BYTE* GetFrmSurface(DWORD FrmID, DWORD FrameNum, DWORD Ori, DWORD *FrmObj_out) {
	BYTE *Surface;
	__asm {
		mov ecx, FrmObj_out //0x518F4C
		mov ebx, Ori
		mov edx, FrameNum
		mov eax, FrmID
		call art_ptr_lock_data_
		mov Surface,eax
	}
	return Surface;
}

//--------------------------------------------------------
BYTE* GetFrmSurface2(DWORD FrmID, DWORD *FrmObj_out, DWORD *frmWidth_out, DWORD *frmHeight_out) {
	BYTE *Surface;
	__asm {
		mov ecx, frmHeight_out
		mov ebx, frmWidth_out
		mov edx, FrmObj_out //0x518F4C
		mov eax, FrmID
		call art_lock_
		mov Surface,eax
	}
	return Surface;
}

//--------------------------------------------------------
FRMhead* GetFrm(DWORD FrmID, DWORD *FrmObj_out) {
	FRMhead* Frm;
	__asm {
		mov edx, FrmObj_out
		mov eax, FrmID
		call art_ptr_lock_
		mov Frm, eax
	}
	return Frm;
}

//--------------------------------------------------------
DWORD GetFrmFrameWidth(FRMhead* Frm, DWORD FrameNum, DWORD Ori) {
	DWORD Width;
	__asm {
		mov ebx, Ori //0-5
		mov edx, FrameNum
		mov eax, Frm
		call art_frame_width_
		mov Width,eax
	}
	return Width;
}

//--------------------------------------------------------
DWORD GetFrmFrameHeight(FRMhead* Frm, DWORD FrameNum, DWORD Ori) {
	DWORD Height;
	__asm {
		mov ebx, Ori //0-5
		mov edx, FrameNum
		mov eax, Frm
		call art_frame_length_
		mov Height,eax
	}
	return Height;
}

//--------------------------------------------------------
BYTE* GetFrmFrameSurface(FRMhead* Frm,  DWORD FrameNum, DWORD Ori) {
	BYTE *Surface;
	__asm {
		mov ebx, Ori //0-5
		mov edx, FrameNum
		mov eax, Frm
		call art_frame_data_
		mov Surface,eax
	}
	return Surface;
}

/////////////////////////////////////////////////////////////////UNLISTED FRM FUNCTIONS////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------
bool LoadFrmHeader(UNLSTDfrm *frmHeader, void*frmStream) {
	if (FReadDword(frmStream, &frmHeader->version) == -1) return 0;
	else if (FReadWord(frmStream, &frmHeader->FPS) == -1) return 0;
	else if (FReadWord(frmStream, &frmHeader->actionFrame) == -1) return 0;
	else if (FReadWord(frmStream, &frmHeader->numFrames) == -1) return 0;

	else if (FReadWordArray(frmStream, frmHeader->xCentreShift, 6) == -1) return 0;
	else if (FReadWordArray(frmStream, frmHeader->yCentreShift, 6) == -1) return 0;
	else if (FReadDwordArray(frmStream, frmHeader->oriOffset, 6) == -1) return 0;
	else if (FReadDword(frmStream, &frmHeader->frameAreaSize) == -1) return 0;

	return 1;
}

//------------------------------------------------------------
bool LoadFrmFrame(UNLSTDframe *frame, void *frmStream) {

	//FRMframe *frameHeader = (FRMframe*)frameMEM;
	//BYTE* frameBuff = frame + sizeof(FRMframe);

	if (FReadWord(frmStream, &frame->width) == -1) return 0;
	else if (FReadWord(frmStream, &frame->height) == -1) return 0;
	else if (FReadDword(frmStream, &frame->size) == -1) return 0;
	else if (FReadWord(frmStream, &frame->x) == -1) return 0;
	else if (FReadWord(frmStream, &frame->y) == -1) return 0;
	frame->indexBuff = new BYTE[frame->size];
	if (FReadString(frmStream, (char*)frame->indexBuff, frame->size, 1) != 1) return 0;

	return 1;
}

//-------------------------------------------------------------------
UNLSTDfrm *LoadUnlistedFrm(char *FrmName, unsigned int folderRef) {

	if (folderRef > 10) return NULL;

	char*artfolder = (char*)(0x51073C + folderRef * 32); //address of art type name
	char FrmPath[MAX_PATH];

	sprintf_s(FrmPath, MAX_PATH, "art\\%s\\%s\0", artfolder, FrmName);

	UNLSTDfrm *frm = new UNLSTDfrm;

	void *frmStream = FOpenFile(FrmPath, "rb");

	if (frmStream) {
		if (!LoadFrmHeader(frm, frmStream)) {
			FCloseFile(frmStream);
			delete frm;
			return NULL;
		}

		DWORD oriOffset_1st = frm->oriOffset[0];
		DWORD oriOffset_new = 0;
		frm->frames = new UNLSTDframe[6*frm->numFrames];
		for (int ori = 0; ori < 6; ori++) {
			if (ori == 0 || frm->oriOffset[ori] != oriOffset_1st) {
				frm->oriOffset[ori] = oriOffset_new;
				for (int fNum = 0; fNum < frm->numFrames; fNum++) {
					if (!LoadFrmFrame(&frm->frames[oriOffset_new + fNum], frmStream)) {
						FCloseFile(frmStream);
						delete frm;
						return NULL;
					}
				}
				oriOffset_new += frm->numFrames;
			} else frm->oriOffset[ori] = 0;
		}

		FCloseFile(frmStream);
	} else {
		delete frm;
		return NULL;
	}
	return frm;
}

/////////////////////////////////////////////////////////////////WINDOW FUNCTIONS////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------------------------
int CreateWin(DWORD x, DWORD y, DWORD width, DWORD height, DWORD BGColourIndex, DWORD flags) {
	int WinRef;
	__asm {
		push flags
		push BGColourIndex
		mov ecx, height
		mov ebx, width
		mov edx, y
		mov eax, x
		call win_add_
		mov WinRef, eax
	}
	return WinRef;
}

//----------------------------------------------------------------------------------
void DestroyWin(int WinRef) {
	__asm {
		mov eax, WinRef
		call win_delete_
	}
}

//--------------------------------------------------
WINinfo *GetWinStruct(int WinRef) {
	WINinfo *winStruct;
	__asm {
		push edx
		mov eax, WinRef
		call GNW_find_
		mov winStruct, eax
		pop edx
	}
	return winStruct;
}

//---------------------------------
BYTE* GetWinSurface(int WinRef) {
	BYTE *surface;
	__asm {
		mov eax, WinRef
		call win_get_buf_
		mov surface, eax
	}
	return surface;
}

//----------------------------------------------------------------------------------
void ShowWin(int WinRef) {
	__asm {
		mov eax, WinRef
		call win_show_
	}
}
/*
//----------------------------------------------------------------------------------
void HideWin(int WinRef) {
	__asm {
		mov eax, WinRef
		call win_hide_
	}
}
*/
//-----------------------------------------------------------------------------------------------------------------------
void RedrawWin(int WinRef) {
	__asm {
		mov eax, WinRef
		call win_draw_
	}
}

/////////////////////////////////////////////////////////////////BUTTON FUNCTIONS////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
int CreateButton(int WinRef, DWORD Xpos, DWORD Ypos, DWORD Width, DWORD Height, DWORD HovOn, DWORD HovOff,
                 DWORD ButtDown, DWORD ButtUp, BYTE *PicUp, BYTE *PicDown, DWORD ButType) {
	int ret_val;
	__asm {
		push ButType //button type: 0x10 = move window pos, 0x20 or 0x0 = regular click, 0x23 = toggle click
		push 0x0 //? always 0
		push PicDown //0//button down pic index
		push PicUp //0//button up pic index
		push ButtUp
		push ButtDown
		push HovOff
		push HovOn
		push Height
		mov ecx, Width
		mov edx, Xpos
		mov ebx, Ypos
		mov eax, WinRef
		call win_register_button_
		mov ret_val, eax
	}
	return ret_val;
}

//-----------------------------
int check_buttons(void) {
	int key_code;
	__asm {
		call get_input_
		mov key_code, eax
	}
	return key_code;
}

/////////////////////////////////////////////////////////////////TEXT FUNCTIONS////////////////////////////////////////////////////////////////////////

//-------------------------
void SetFont(int ref) {
	__asm {
		mov eax, ref
		call text_font_
	}
}

//-----------------------
int GetFont(void) {
	return *(DWORD*)_curr_font_num;
}

//---------------------------------------------------------
//print text to surface
void PrintText(char *DisplayText, BYTE ColourIndex, DWORD Xpos, DWORD Ypos, DWORD TxtWidth, DWORD ToWidth, BYTE *ToSurface) {
	DWORD posOffset = Ypos*ToWidth + Xpos;
	__asm {
		xor eax, eax
		MOV AL, ColourIndex
		push eax
		mov edx, DisplayText
		mov ebx, TxtWidth
		mov ecx, ToWidth
		mov eax, ToSurface
		add eax, posOffset
		call dword ptr ds:[_text_to_buf]
	}
}

//---------------------------------------------------------
//gets the height of the currently selected font
DWORD GetTextHeight() {
	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[_text_height] //get text height
		mov TxtHeight, eax
	}
	return TxtHeight;
}

//---------------------------------------------------------
//gets the length of a string using the currently selected font
DWORD GetTextWidth(char *TextMsg) {
	DWORD TxtWidth;
	__asm {
		mov eax, TextMsg
		call dword ptr ds:[_text_width] //get text width
		mov TxtWidth, eax
	}
	return TxtWidth;
}

//---------------------------------------------------------
//get width of Char for current font
DWORD GetCharWidth(char CharVal) {
	DWORD charWidth;
	__asm {
		mov al, CharVal
		call dword ptr ds:[_text_char_width]
		mov charWidth, eax
	}
	return charWidth;
}

//---------------------------------------------------------
//get maximum string length for current font - if all characters were maximum width
DWORD GetMaxTextWidth(char *TextMsg) {
	DWORD msgWidth;
	__asm {
		mov eax, TextMsg
		call dword ptr ds:[_text_mono_width]
		mov msgWidth, eax
	}
	return msgWidth;
}

//---------------------------------------------------------
//get number of pixels between characters for current font
DWORD GetCharGapWidth() {
	DWORD gapWidth;
	__asm {
		call dword ptr ds:[_text_spacing]
		mov gapWidth, eax
	}
	return gapWidth;
}

//---------------------------------------------------------
//get maximum character width for current font
DWORD GetMaxCharWidth() {
	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[_text_max]
		mov charWidth, eax
	}
	return charWidth;
}

/////////////////////////////////////////////////////////////////DAT FUNCTIONS////////////////////////////////////////////////////////////////////////

//-----------------------------------------
void* LoadDat(char*FileName) {
	void *dat=NULL;
	__asm {
		mov eax, FileName
		call dbase_open_
		mov dat, eax
	}
	return dat;
}

//-----------------------------------------
void UnloadDat(void *dat) {
	__asm {
		mov eax, dat
		call dbase_close_
	}
}

/////////////////////////////////////////////////////////////////OTHER FUNCTIONS////////////////////////////////////////////////////////////////////////

/*
void DrawLineX(int WinRef, DWORD XStartPos, DWORD XEndPos, DWORD Ypos, BYTE ColourIndex) {
	__asm {
		xor eax, eax
		mov al, ColourIndex
		push eax
		mov ebx, Ypos
		push ebx
		mov ecx, XEndPos
		mov edx, XStartPos
		mov eax, WinRef

		call win_line_
	}
}
*/

//---------------------------------------------------------
void PlayAcm(char *AcmName) {
	__asm {
		mov eax, AcmName
		call gsound_play_sfx_file_
	}
}

//------------------------------
void _stdcall RefreshArtCache(void) {
	__asm {
		call art_flush_
	}
}

// Check fallout paths for file----------------
int CheckFile(char*FileName, DWORD *size_out) {
	int retVal = 0;
	__asm {
		mov edx, size_out
		mov eax, FileName
		call db_dir_entry_
		mov retVal, eax
	}
	return retVal;
}

/////////////////////////////////////////////////////////////////APP MOD FUNCTIONS////////////////////////////////////////////////////////////////////////

//-----------------------------------------
char _stdcall GetSex(void) {
	char sex;
	__asm {
		mov edx, STAT_gender //sex stat ref
		mov eax, dword ptr ds:[_obj_dude] //hero state structure
		call stat_level_ //get Player stat val
		test eax, eax //male=0, female=1
		jne Female
		mov sex, 'M'
		jmp EndFunc
Female:
		mov sex, 'F'
EndFunc:
	}
	return sex;
}

//----------------------------------------------------------------
int _stdcall LoadHeroDat(unsigned int Race, unsigned int Style) {

	if (HeroPathPtr->pDat) { //unload previous Dats
		UnloadDat(HeroPathPtr->pDat);
		HeroPathPtr->pDat = NULL;
		HeroPathPtr->isDat = 0;
	}
	if (RacePathPtr->pDat) {
		UnloadDat(RacePathPtr->pDat);
		RacePathPtr->pDat = NULL;
		RacePathPtr->isDat = 0;
	}

	sprintf_s(HeroPathPtr->path, 64, "Appearance\\h%cR%02dS%02d.dat\0", GetSex(), Race, Style);
	if (GetFileAttributes(HeroPathPtr->path) != INVALID_FILE_ATTRIBUTES) { //check if Dat exists for selected appearance
		HeroPathPtr->pDat = LoadDat(HeroPathPtr->path);
		HeroPathPtr->isDat = 1;
	}
	else {
		sprintf_s(HeroPathPtr->path, 64, "Appearance\\h%cR%02dS%02d\0", GetSex(), Race, Style);
		if (GetFileAttributes(HeroPathPtr->path) == INVALID_FILE_ATTRIBUTES) //check if folder exists for selected appearance
			return -1;
	}

	TempPathPtr = &HeroPathPtr; //set path for selected appearance
	HeroPathPtr->next = *(sPath**)_paths;

	if (Style != 0) {
		sprintf_s(RacePathPtr->path, 64, "Appearance\\h%cR%02dS%02d.dat\0", GetSex(), Race, 0);
		if (GetFileAttributes(RacePathPtr->path) != INVALID_FILE_ATTRIBUTES) { //check if Dat exists for selected race base appearance
			RacePathPtr->pDat = LoadDat(RacePathPtr->path);
			RacePathPtr->isDat = 1;
		}
		else
			sprintf_s(RacePathPtr->path, 64, "Appearance\\h%cR%02dS%02d\0", GetSex(), Race, 0);

		if (GetFileAttributes(RacePathPtr->path) != INVALID_FILE_ATTRIBUTES) { //check if folder/Dat exists for selected race base appearance
			HeroPathPtr->next = RacePathPtr; //set path for selected race base appearance
			RacePathPtr->next = *(sPath**)_paths;
		}
	}
	return 0;
}

//---------------------------------------------------------
//insert hero art path in front of main path structure when loading art
static void __declspec(naked) LoadNewHeroArt() {
	__asm {
		cmp byte ptr ds:[esi], 'r'
		je isReading
		mov ecx, _paths
		jmp setPath
isReading:
		mov ecx, TempPathPtr
setPath:
		mov ecx, dword ptr ds:[ecx]
		ret
	}
}

/*
//----------------------------------------------------------------
int _stdcall CheckHeroFile(char* FileName) {

	char TempPath[64];
	//sprintf_s(TempPath, 64, "%s\\%s\0", cPath, FileName);
	sprintf_s(TempPath, 64, "%s\\%s\0", HeroPathPtr->path, FileName);

	if (GetFileAttributes(TempPath) == INVALID_FILE_ATTRIBUTES)
		return -1;


	return 0;
}
*/

//---------------------------------------------------------
static void __declspec(naked) CheckHeroExist() {
	__asm {
		//pushad
		cmp esi, critterArraySize //check if loading hero art
		jle EndFunc

		sub esp, 0x4
		lea ebx, [esp]
		push ebx
		push _art_name //critter art file name address
		//call CheckHeroFile//check if art file exists
		call CheckFile
		add esp, 0xC
		cmp eax, -1
		jne EndFunc

		//pop eax//drop func ret address
		add esp, 0x4
		//if file not found load regular critter art instead
		sub esi, critterArraySize
		//popad
		mov eax, 0x4194E2
		jmp eax
EndFunc:
		//popad
		mov eax, _art_name
		ret
	}
}

//---------------------------------------------------------
//adjust base hero art if num below hero art range
static void __declspec(naked) AdjustHeroBaseArt() {
	__asm {
		//cmp eax, critterListSize
		// jg EndFunc
		add eax, critterListSize
//EndFunc:
		mov dword ptr ds:[_art_vault_guy_num],eax
		ret
	}
}

//---------------------------------------------------------
//adjust armor art if num below hero art range
static void __declspec(naked) AdjustHeroArmorArt() {
	__asm {
		pop ebx //get ret addr
		mov dword ptr ss:[esp], ebx //reinsert ret addr in old (push 0)
		xor ebx, ebx
		push 0
		call art_id_ //call load frm func
		mov dx, ax
		and dh, 0xFF00 //mask out current weapon flag
		cmp edx, critterListSize //check if critter art in PC range
		jg EndFunc
		add eax, critterListSize //shift critter art index up into hero range
EndFunc:
		//mov dword ptr ds:[_i_fid],eax
		ret
	}
}

//-----------------------------------------
void _stdcall SetHeroArt(int NewArtFlag) {
	__asm {
		mov eax, dword ptr ds:[_obj_dude] //hero state struct
		mov eax, dword ptr ds:[eax + 0x20] //get hero FrmID
		xor edx, edx
		mov dx, ax
		and dh, 0xFF00 //mask out current weapon flag
		cmp edx, critterListSize //check if critter LST index is in Hero range
		jg IsHero
		cmp NewArtFlag, 1
		jne EndFunc
		add eax, critterListSize //shift index up into hero range
		jmp SetArt
IsHero:
		cmp NewArtFlag, 0
		jne EndFunc
		sub eax, critterListSize //shift index down into normal critter range
SetArt:
		mov ebx, 0 //SomePtr
		mov edx, eax
		mov eax, dword ptr ds:[_obj_dude] //hero state struct
		mov dword ptr ds:[eax + 0x20],edx //copy new FrmID to hero state struct
		//call obj_change_fid_ // set critter FrmID function
EndFunc:
	}
}

//----------------------------------------------
//return hero art val to normal before saving
static void __declspec(naked) SavCritNumFix() {
	__asm {
		push eax
		push edx
		push 0 //set hero FrmID LST index to normal range before saving
		call SetHeroArt
		pop edx
		pop eax

		push ebx
		call obj_save_dude_ //save current hero state structure fuction
		pop ebx

		push eax
		push edx
		push 1 //return hero FrmID LST index back to hero art range after saving hero state structure
		call SetHeroArt
		pop edx
		pop eax

		ret
	}
}

//---------------------------------------------------------
static void __declspec(naked) DoubleArt() {
	__asm {
		cmp dword ptr ss:[esp+0xCC], 0x510774 //check if loading critter lst. 0x510774= addr of critter list size val
		jne EndFunc
		shl edi, 1 //double critter list size to add room for hero art
EndFunc:
		mov eax, ecx
		xor ebx, ebx
		xor edx, edx
		ret
	}
}

//------------------------------
void FixCritList() {
	//int size = (*(DWORD*)0x510774) / 2; //critter list size after resize by DoubleArt func
	critterListSize = (*(DWORD*)0x510774) / 2;
	critterArraySize = critterListSize * 13;

	char *CritList = (*(char**)0x51076C); //critter list offset
	char *HeroList = CritList + (critterArraySize); //set start of hero critter list after regular critter list

	memset( HeroList, '\0', critterArraySize);

	for (DWORD i = 0; i < critterListSize; i++) { //copy critter name list to hero name list
		*HeroList = '_'; //insert a '_' char at the front of new hero critt names. -fallout wont load the same name twice
		memcpy(HeroList + 1, CritList, 11);
		HeroList += 13;
		CritList += 13;
	}
}

//---------------------------------------------------------
static void __declspec(naked) AddHeroCritNames() {
	__asm {
		call FixCritList //insert names for hero critters
		mov eax, dword ptr ds:[_art + 0x3C]
		ret
	}
}

//-----------------------------------------------------------------------------------------------------------------------
void sub_draw(long subWidth, long subHeight, long fromWidth, long fromHeight, long fromX, long fromY, BYTE *fromBuff,
              long toWidth, long toHeight, long toX, long toY, BYTE *toBuff, int maskRef) {

	fromBuff = fromBuff + fromY*fromWidth + fromX;
	toBuff = toBuff + toY*toWidth + toX;

	for (long h = 0; h < subHeight; h++) {
		for (long w = 0; w < subWidth; w++) {
			if (fromBuff[w] != maskRef)
				toBuff[w] = fromBuff[w];
		}
		fromBuff += fromWidth;
		toBuff += toWidth;
	}
}


//-----------------------------------------------
void DrawPC(void) {
	RECT critRect;
	__asm {
		/*
		lea ebx, //-out- RECT*
		mov eax, dword ptr ds:[_obj_dude] //critter struct
		mov edx, dword ptr ds:[eax + 0x20] // new frmId
		call obj_change_fid_ //set new critt FrmID func
		*/
		lea edx, critRect //out critter RECT*
		mov eax, dword ptr ds:[_obj_dude] //dude critter struct
		call obj_bound_ //get critter rect func

		mov edx, dword ptr ds:[_obj_dude] //dude critter struct
		lea eax, critRect //RECT*
		mov edx, dword ptr ds:[edx + 0x28] //map level the dude is on
		call tile_refresh_rect_ //draw rect area func
	}
}

//----------------------------------------------------------------------
void _stdcall RefreshPCArt() {
//scan inventory items for armor and weapons currently being worn or wielded
//and setup matching FrmID for PC
	__asm {
		call proto_dude_update_gender_ //refresh PC base model art

		mov eax, dword ptr ds:[_obj_dude] //PC state struct
		mov dword ptr ds:[_inven_dude], eax //inventory temp pointer to PC state struct
		mov eax, dword ptr ds:[_inven_dude]
		lea edx, dword ptr ds:[eax + 0x2C]
		mov dword ptr ds:[_pud], edx //PC inventory

		xor eax, eax
		xor edx, edx //itemListOffset
		xor ebx, ebx //itemNum

		mov dword ptr ds:[_i_rhand], eax //item2
		mov dword ptr ds:[_i_worn], eax //armor
		mov dword ptr ds:[_i_lhand], eax //item1
		jmp LoopStart

CheckNextItem:
		mov eax, dword ptr ds:[eax + 0x8] //PC inventory item list
		mov eax, dword ptr ds:[edx + eax] //PC inventory item list + itemListOffset

		test byte ptr ds:[eax + 0x27], 1 //if item in item1 slot
		jnz IsItem1
		test byte ptr ds:[eax + 0x27], 2 //if item in item2 slot
		jnz IsItem2
		test byte ptr ds:[eax + 0x27], 4 //if item in armor slot
		jnz IsArmor
		jmp SetNextItem

IsItem1:
		mov dword ptr ds:[_i_lhand], eax //set item1
		test byte ptr ds:[eax + 0x27], 2 //check if same item type also in item2 slot
		jz SetNextItem

IsItem2:
		mov dword ptr ds:[_i_rhand], eax //set item2
		jmp SetNextItem

IsArmor:
		mov dword ptr ds:[_i_worn], eax //set armor

SetNextItem:
		inc ebx //itemNum++
		add edx, 0x8 //itemListOffset + itemsize
LoopStart:
		mov eax, dword ptr ds:[_pud] //PC inventory
		cmp ebx, dword ptr ds:[eax] //size of item list
		jl CheckNextItem

		//inventory function - setup pc FrmID and store at address _i_fid
		call adjust_fid_

		//copy new FrmID to hero state struct
		mov edx, dword ptr ds:[_i_fid]
		mov eax, dword ptr ds:[_inven_dude]
		mov dword ptr ds:[eax + 0x20], edx
		//call obj_change_fid_

		xor eax,eax
		mov dword ptr ds:[_i_rhand], eax //item2
		mov dword ptr ds:[_i_worn], eax //armor
		mov dword ptr ds:[_i_lhand], eax //item1
	}

	if (!AppModEnabled) return;

	if (LoadHeroDat(CurrentRaceVal, CurrentStyleVal) != 0) { //if load fails
		CurrentStyleVal = 0; //set style to default
		if (LoadHeroDat(CurrentRaceVal, CurrentStyleVal) != 0) { //if race fails with style at default
			CurrentRaceVal = 0; //set race to default
			LoadHeroDat(CurrentRaceVal, CurrentStyleVal);
		}
	}
	RefreshArtCache();
	DrawPC();
}

//----------------------------------------------------------------------
void _stdcall LoadHeroAppearance(void) {
	if (!AppModEnabled) return;

	GetAppearanceGlobals(&CurrentRaceVal, &CurrentStyleVal);
	RefreshArtCache();
	LoadHeroDat(CurrentRaceVal, CurrentStyleVal);
	SetHeroArt(1);
	DrawPC();
}

//---------------------------------------
void _stdcall SetNewCharAppearanceGlobals(void) {
	if (!AppModEnabled) return;

	if (CurrentRaceVal > 0 || CurrentStyleVal > 0)
		SetAppearanceGlobals(CurrentRaceVal, CurrentStyleVal);
}

//----------------------------------------------------------------------
void _stdcall SetHeroStyle(int newStyleVal) {
	if (!AppModEnabled) return;

	if (newStyleVal == CurrentStyleVal) return;

	RefreshArtCache();

	if (LoadHeroDat(CurrentRaceVal, newStyleVal) != 0) { //if new style cannot be set
		if (CurrentRaceVal == 0 && newStyleVal == 0) {
			CurrentStyleVal = 0; //ignore error if appearance = default
		} else {
			LoadHeroDat(CurrentRaceVal, CurrentStyleVal); //reload original style
		}
	} else {
		CurrentStyleVal=newStyleVal;
	}

	SetAppearanceGlobals(CurrentRaceVal, CurrentStyleVal);
	DrawPC();
}

//----------------------------------------------------------------------
void _stdcall SetHeroRace(int newRaceVal) {

	if (!AppModEnabled) return;

	if (newRaceVal == CurrentRaceVal) return;

	RefreshArtCache();

	if (LoadHeroDat(newRaceVal, 0) != 0) {   //if new race fails with style at 0
		if (newRaceVal == 0) CurrentRaceVal = 0, CurrentStyleVal = 0; //ignore if appearance = default
		else LoadHeroDat(CurrentRaceVal, CurrentStyleVal); //reload original race & style
	}
	else
		CurrentRaceVal = newRaceVal, CurrentStyleVal = 0;

	SetAppearanceGlobals(CurrentRaceVal, CurrentStyleVal); //store new globals
	DrawPC();
}

//--------------------------------------------------------------------------------------
bool CreateWordWrapList(char *TextMsg, DWORD WrapWidth, DWORD *lineNum, LINENode *StartLine) {
	*lineNum = 1;

	if (GetMaxCharWidth() >= WrapWidth) return FALSE;

	if (GetTextWidth(TextMsg) < WrapWidth) return TRUE;

	DWORD GapWidth = GetCharGapWidth();

	StartLine->next = new LINENode;
	LINENode *NextLine = StartLine->next;

	DWORD lineWidth = 0, wordWidth = 0;

	char CurrentChar = NULL;
	DWORD i = 0;

	while (TextMsg[i] != NULL) {
		CurrentChar = TextMsg[i];

		lineWidth = lineWidth + GetCharWidth(CurrentChar) + GapWidth;
		wordWidth = wordWidth + GetCharWidth(CurrentChar) + GapWidth;

		if (lineWidth <= WrapWidth) {
			if (isspace(CurrentChar) || CurrentChar == '-')
				NextLine->offset = i + 1, wordWidth = 0;
		}
		else {
			if (isspace(CurrentChar))
				NextLine->offset = i + 1, wordWidth = 0;

			lineWidth = wordWidth;
			wordWidth = 0;
			CurrentChar = NULL;
			*lineNum = *lineNum + 1;
			NextLine->next = new LINENode;
			NextLine = NextLine->next;
		}
		i++;

		if (TextMsg[i] == NULL)
			NextLine->offset = 0;
	}

	return TRUE;
}

/*
int WordWrap(char *TextMsg, DWORD lineLength, WORD *lineNum, WORD *lineOffsets) {
	int retVal;
	__asm {
		mov ebx, lineOffsets
		mov ecx, lineNum
		mov edx, lineLength
		mov eax, TextMsg
		call _word_wrap_
		mov retVal, eax
	}
	return retVal;
}
*/

void DeleteWordWrapList(LINENode *CurrentLine) {
	LINENode *NextLine = NULL;

	while (CurrentLine != NULL) {
		NextLine = CurrentLine->next;
		delete CurrentLine;
		CurrentLine = NextLine;
	}
}

//----------------------------------------------------------------
void DrawPCConsole() {

	DWORD NewTick = *(DWORD*)0x5709C4; //char scrn gettickcount ret
	DWORD RotSpeed = *(DWORD*)0x47066B; //get rotation speed -inventory rotation speed

	if (CharRotTick > NewTick)
		CharRotTick = NewTick;

	if (NewTick - CharRotTick > RotSpeed) {
		CharRotTick = NewTick;
		if (CharRotOri < 5)
			CharRotOri++;
		else CharRotOri = 0;


		int WinRef = *(DWORD*)_edit_win; //char screen window ref
		//BYTE *WinSurface = GetWinSurface(WinRef);
		WINinfo *WinInfo = GetWinStruct(WinRef);

		BYTE *ConSurface = new BYTE [70*102];

		sub_draw(70, 102, 640, 480, 338, 78, CharScrnBackSurface, 70, 102, 0, 0, ConSurface, 0);
		//sub_draw(70, 102, widthBG, heightBG, xPosBG, yPosBG, BGSurface, 70, 102, 0, 0, ConSurface, 0);

		//DWORD CritNum = *(DWORD*)_art_vault_guy_num; //pointer to current base hero critter FrmId
		DWORD CritNum = *(DWORD*)(*(DWORD*)_obj_dude + 0x20); //pointer to current armored hero critter FrmId
		DWORD CritFrmObj;
		FRMhead *CritFrm;
		//DWORD PcCritOri = 0;
		DWORD CritWidth;
		DWORD CritHeight;
		BYTE *CritSurface;

		CritFrm = GetFrm(LoadFrm(1, CritNum), &CritFrmObj);
		CritWidth = GetFrmFrameWidth(CritFrm, 0, CharRotOri);
		CritHeight = GetFrmFrameHeight(CritFrm, 0, CharRotOri);
		CritSurface = GetFrmFrameSurface(CritFrm, 0, CharRotOri);

		sub_draw(CritWidth, CritHeight, CritWidth, CritHeight, 0, 0, CritSurface, 70, 102, 35-CritWidth/2, 51-CritHeight/2, ConSurface, 0);

		BYTE ConsoleGreen = *(BYTE*)_GreenColor; //palette offset stored in mem - text colour
		BYTE ConsoleGold = *(BYTE*)_YellowColor; //palette offset stored in mem - text colour

		BYTE styleColour = ConsoleGreen, raceColour = ConsoleGreen;
		if (*(DWORD*)_info_line == 0x501)
			raceColour = ConsoleGold;
		else if (*(DWORD*)_info_line == 0x502)
			styleColour = ConsoleGold;
/*
		int oldFont = GetFont(); //store current font
		SetFont(0x65); //set font for consol text
		char TextBuf[12];

		sprintf_s(TextBuf, 12, "%2d\0", CurrentRaceVal);
		PrintText(TextBuf, raceColour, 2, 2, 64, 70, ConSurface);

		sprintf_s(TextBuf, 12, "%2d\0", CurrentStyleVal);
		PrintText(TextBuf, styleColour, 5, 88, 64, 70, ConSurface);

		SetFont(oldFont); //restore previous font
*/

		//sub_draw(70, 102, 70, 102, 0, 0, ConSurface, 640, 480, 338, 78, WinSurface, 0);
		sub_draw(70, 102, 70, 102, 0, 0, ConSurface, WinInfo->width, WinInfo->height, 338, 78, WinInfo->surface, 0);

		UnloadFrm(CritFrmObj);
		CritSurface = NULL;
		delete[] ConSurface;
		WinInfo = NULL;
		RedrawWin(WinRef);
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------------
void DrawCharNote(bool Style, int WinRef, DWORD xPosWin, DWORD yPosWin, BYTE *BGSurface, DWORD xPosBG, DWORD yPosBG, DWORD widthBG, DWORD heightBG) {
	MSGList MsgList;
	char *TitleMsg = NULL;
	char *InfoMsg = NULL;

	char *MsgFileName = NULL;

	if (!Style) MsgFileName = "game\\AppRace.msg";
	else MsgFileName = "game\\AppStyle.msg";

	if (LoadMsgList(&MsgList, MsgFileName) == 1) {
		TitleMsg = GetMsg(&MsgList, 100, 2);
		InfoMsg = GetMsg(&MsgList, 101, 2);
	}

	BYTE colour = *(BYTE*)0x6A38D0; //brown

	WINinfo *WinInfo = GetWinStruct(WinRef);

	BYTE *PadSurface;
	PadSurface = new BYTE [280*168];
	sub_draw(280, 168, widthBG, heightBG, xPosBG, yPosBG, BGSurface, 280, 168, 0, 0, PadSurface, 0);

	UNLSTDfrm *frm;
	if (Style) frm = LoadUnlistedFrm("AppStyle.frm", 10);
	else frm = LoadUnlistedFrm("AppRace.frm", 10);

	if (frm) {
		sub_draw(frm->frames[0].width, frm->frames[0].height, frm->frames[0].width, frm->frames[0].height, 0, 0, frm->frames[0].indexBuff, 280, 168, 136, 37, PadSurface, 0); //cover buttons pics bottom
		//sub_draw(frm->width, frm->height, frm->width, frm->height, 0, 0, frm->surface, 280, 168, 136, 37, PadSurface, 0); //cover buttons pics bottom
		//sub_draw(frm->width, frm->height, frm->width, frm->height, 0, 0, frm->surface, 280, 168, 135, 36, PadSurface, 0); //cover buttons pics bottom
		delete frm;
	}

	int oldFont = GetFont(); //store current font
	SetFont(0x66); //set font for title

	DWORD textHeight=GetTextHeight();

	if (TitleMsg != NULL) {
		PrintText(TitleMsg, colour, 0, 0, 265, 280, PadSurface);
		//DrawLineX(WinRef, 348, 613, 272+textHeight, colour);
		//DrawLineX(WinRef, 348, 613, 273+textHeight, colour);
		memset(PadSurface + 280*textHeight, colour, 265);
		memset(PadSurface + 280*(textHeight + 1), colour, 265);
	}

	SetFont(0x65); //set font for info

	textHeight = GetTextHeight();

	DWORD lineNum = 0;

	LINENode *StartLine = new LINENode;
	LINENode *CurrentLine, *NextLine;

	if (InfoMsg != NULL) {
		if (CreateWordWrapList(InfoMsg, 160, &lineNum, StartLine)) {
			int lineHeight = 43;
			char TempChar = 0;

			if (lineNum == 1) PrintText(InfoMsg, colour, 0, lineHeight, 280, 280, PadSurface);
			else{
				if (lineNum > 11) lineNum = 11;
				CurrentLine = StartLine;

				for (DWORD line = 0; line < lineNum; line++) {
					NextLine = CurrentLine->next;
					TempChar = InfoMsg[NextLine->offset]; //[line+1]];
					InfoMsg[NextLine->offset] = '\0';
					PrintText(InfoMsg+CurrentLine->offset, colour, 0, lineHeight, 280, 280, PadSurface);
					InfoMsg[NextLine->offset] = TempChar;
					lineHeight = lineHeight + textHeight + 1;
					CurrentLine = NextLine;
				}
			}
		}
	}

	sub_draw(280, 168, 280, 168, 0, 0, PadSurface, WinInfo->width, WinInfo->height, xPosWin, yPosWin, WinInfo->surface, 0);

	DeleteWordWrapList(StartLine);
	CurrentLine = NULL;
	NextLine = NULL;
	delete[]PadSurface;
	WinInfo = NULL;
	SetFont(oldFont); //restore previous font
	DestroyMsgList(&MsgList);
	//RedrawWin(*(DWORD*)_edit_win);
}

/*
void DrawCharNote(DWORD LstNum, char *TitleTxt, char *AltTitleTxt, char *Message) {
	__asm {
		MOV ECX,Message//100//DWORD PTR DS:[_folder_card_desc]
		MOV EBX,AltTitleTxt//DWORD PTR DS:[_folder_card_title2]
		MOV EDX,TitleTxt//DWORD PTR DS:[_folder_card_title]
		MOV EAX,LstNum//11//LstNum//DWORD PTR DS:[_folder_card_fid]
		CALL DrawCard_
	}
}
*/

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void _stdcall HeroSelectWindow(int RaceStyleFlag) {

	if (!AppModEnabled) return;

	bool isStyle = TRUE;
	if (RaceStyleFlag == 0) isStyle = FALSE;

	DWORD ResWidth = *(DWORD*)0x4CAD6B;
	DWORD ResHeight = *(DWORD*)0x4CAD66;

	int WinRef = CreateWin(ResWidth/2 - 242, (ResHeight - 100)/2 - 65, 484, 230, 100, 0x4);
	if (WinRef == -1) return;

	int mouseWasHidden = IsMouseHidden();
	if (mouseWasHidden) {
	   ShowMouse();
	}

	int oldMouse = GetMousePic();
	SetMousePic(1);

	BYTE *WinSurface = GetWinSurface(WinRef);

	BYTE *mainSurface;
	mainSurface = new BYTE [484*230];

	DWORD tempObj;
	BYTE *tempSurface;
	//perkwin
	tempSurface = GetFrmSurface(LoadFrm(6, 86), 0, 0, &tempObj);
	sub_draw(484, 230, 573, 230, 89, 0, tempSurface, 484, 230, 0, 0, mainSurface, 0);
	sub_draw(13, 230, 573, 230, 0, 0, tempSurface, 484, 230, 0, 0, mainSurface, 0);
	UnloadFrm(tempObj);

	//opbase
	tempSurface = GetFrmSurface(LoadFrm(6, 220), 0, 0, &tempObj);
	sub_draw(164, 217, 164, 217, 0, 0, tempSurface, 484, 230, 12, 4, mainSurface, 0);
	UnloadFrm(tempObj);

	//use
	tempSurface = GetFrmSurface(LoadFrm(6, 113), 0, 0, &tempObj);
	sub_draw(138, 132, 292, 376, 128, 20, tempSurface, 484, 230, 25, 38, mainSurface, 0);
	sub_draw(2, 132, 292, 376, 23, 224, tempSurface, 484, 230, 25, 38, mainSurface, 0);
	sub_draw(12, 4, 292, 376, 135, 148, tempSurface, 484, 230, 25, 166, mainSurface, 0);
	UnloadFrm(tempObj);

	//barter
	tempSurface = GetFrmSurface(LoadFrm(6, 111), 0, 0, &tempObj);
	sub_draw(25, 52, 640, 191, 190, 54, tempSurface, 484, 230, 27, 57, mainSurface, 0); //button background up down
	UnloadFrm(tempObj);

	//loot
	tempSurface = GetFrmSurface(LoadFrm(6, 114), 0, 0, &tempObj);
	sub_draw(116, 27, 537, 376, 392, 325, tempSurface, 484, 230, 36, 180, mainSurface, 0); //button background "done"
	UnloadFrm(tempObj);

	DWORD MenuUObj, MenuDObj;
	BYTE *MenuUSurface = GetFrmSurface(LoadFrm(6, 299), 0, 0, &MenuUObj); //MENUUP Frm
	BYTE *MenuDSurface = GetFrmSurface(LoadFrm(6, 300), 0, 0, &MenuDObj); //MENUDOWN Frm
	CreateButton(WinRef, 116, 181, 26, 26, -1, -1, -1, 0x0D, MenuUSurface, MenuDSurface, 0x20);

	DWORD DidownUObj, DidownDObj;
	BYTE *DidownUSurface = GetFrmSurface(LoadFrm(6, 93), 0, 0, &DidownUObj); //MENUUP Frm
	BYTE *DidownDSurface = GetFrmSurface(LoadFrm(6, 94), 0, 0, &DidownDObj); //MENUDOWN Frm
	CreateButton(WinRef, 28, 84, 24, 25, -1, -1, -1, 0x150, DidownUSurface, DidownDSurface, 0x20);

	DWORD DiupUObj, DiupDObj;
	BYTE *DiupUSurface = GetFrmSurface(LoadFrm(6, 100), 0, 0, &DiupUObj); //MENUUP Frm
	BYTE *DiupDSurface = GetFrmSurface(LoadFrm(6, 101), 0, 0, &DiupDObj); //MENUDOWN Frm
	CreateButton(WinRef, 28, 59, 23, 24, -1, -1, -1, 0x148, DiupUSurface, DiupDSurface, 0x20);

	int oldFont;
	oldFont = GetFont();
	SetFont(0x67);
	BYTE textColour = *(BYTE*)0x6A82F3; //PeanutButter colour -palette offset stored in mem

	char titleText[8];
	DWORD titleTextWidth;
	//Get alternate text from ini if available
	if (isStyle) {
		GetPrivateProfileString("AppearanceMod", "StyleText", "Style", titleText, 8, translationIni);
	} else {
		GetPrivateProfileString("AppearanceMod", "RaceText", "Race", titleText, 8, translationIni);
	}

	titleTextWidth = GetTextWidth(titleText);

	PrintText(titleText, textColour, 94 - titleTextWidth/2, 10, titleTextWidth, 484, mainSurface);

	DWORD titleTextHeight = GetTextHeight();
	//Title underline
	memset(mainSurface + 484*(10 + titleTextHeight) + 94 - titleTextWidth/2, textColour, titleTextWidth );
	memset(mainSurface + 484*(10 + titleTextHeight + 1) + 94 - titleTextWidth/2, textColour, titleTextWidth );

	sub_draw(484, 230, 484, 230, 0, 0, mainSurface, 484, 230, 0, 0, WinSurface, 0);

	ShowWin(WinRef);

	int raceVal = CurrentRaceVal, styleVal = CurrentStyleVal; //show default style when setting race
	if (!isStyle) styleVal = 0;
	LoadHeroDat(raceVal, styleVal);

	BYTE *ConDraw;
	ConDraw = new BYTE [70*102];

	//char TextBuf[12];

	DWORD NewTick = 0, OldTick = 0;

	textColour = *(BYTE*)_GreenColor; //ConsoleGreen colour -palette offset stored in mem
	SetFont(0x65);

	DWORD CritNum = *(DWORD*)_art_vault_guy_num; //pointer to current base hero critter FrmID
	//DWORD CritNum = *(DWORD*)(*(DWORD*)_obj_dude+0x20); //pointer to current armored hero critter FrmID
	FRMhead *CritFrm;
	DWORD CritFrmObj = 0, CritOri = 0, CritWidth = 0, CritHeight = 0;
	BYTE *CritSurface = NULL;

	int button = 0, exitMenu = 0;

	bool drawFlag = TRUE; //redraw flag for char note pad

	DWORD RotSpeed = *(DWORD*)0x47066B; //get rotation speed -inventory rotation speed

	DWORD RedrawTick = 0;

	while (!exitMenu) { //main loop
		NewTick = GetTickCount(); //timer for redraw
		if (OldTick > NewTick)
			OldTick = NewTick;

		if (NewTick - OldTick > RotSpeed) { //time to rotate critter
			OldTick = NewTick;
			if (CritOri < 5)
				CritOri++;
			else CritOri = 0;
		}

		if (RedrawTick > NewTick)
			RedrawTick = NewTick;

		if (NewTick - RedrawTick > 60) { //time to redraw
			RedrawTick = NewTick;
			sub_draw(70, 102, 484, 230, 66, 53, mainSurface, 70, 102, 0, 0, ConDraw, 0);

			CritFrm = GetFrm(LoadFrm(1, CritNum), &CritFrmObj);
			CritWidth = GetFrmFrameWidth(CritFrm, 0, CritOri);
			CritHeight = GetFrmFrameHeight(CritFrm, 0, CritOri);
			CritSurface = GetFrmFrameSurface(CritFrm, 0, CritOri);
			sub_draw(CritWidth, CritHeight, CritWidth, CritHeight, 0, 0, CritSurface, 70, 102, 35 - CritWidth / 2, 51 - CritHeight / 2, ConDraw, 0);
			UnloadFrm(CritFrmObj);
			CritSurface = NULL;
/*
			if (isStyle) sprintf_s(TextBuf, 12, "%2d\0", styleVal);
			else sprintf_s(TextBuf, 12, "%2d\0", raceVal);

			PrintText(TextBuf, textColour, 2, 2, 64, 70, ConDraw);
*/
			sub_draw( 70, 102, 70, 102, 0, 0, ConDraw, 484, 230, 66, 53, WinSurface, 0);

			if (drawFlag == TRUE)
				DrawCharNote(isStyle, WinRef, 190, 29, mainSurface, 190, 29, 484, 230);
			drawFlag = FALSE;

			RedrawWin(WinRef);
		}

		button = check_buttons();
		if (button == 0x148) { //previous style/race -up arrow button pushed
			drawFlag = TRUE;
			PlayAcm("ib1p1xx1");
			RefreshArtCache();

			if (isStyle) {
				if (styleVal > 0) styleVal--;
				if (LoadHeroDat(raceVal, styleVal) != 0) {
					styleVal = 0;
					LoadHeroDat(raceVal, styleVal);
				}
			} else { //Race
				if (raceVal > 0) styleVal = 0, raceVal--;
				if (LoadHeroDat(raceVal, styleVal) != 0) {
					raceVal = 0;
					LoadHeroDat(raceVal, styleVal);
				}

			}
		} else if (button == 0x150) { //Next style/race -down arrow button pushed
			drawFlag = TRUE;
			PlayAcm("ib1p1xx1");
			RefreshArtCache();

			if (isStyle) {
				styleVal++;
				if (LoadHeroDat(raceVal, styleVal) != 0) {
					styleVal--;
					LoadHeroDat(raceVal, styleVal);
				}
			} else { //Race
				styleVal = 0, raceVal++;
				if (LoadHeroDat(raceVal, styleVal) != 0) {
					raceVal--;
					LoadHeroDat(raceVal, styleVal);
				}
			}
		} else if (button == 0x0D) { //save and exit -Enter button pushed
			exitMenu = -1;
			if (!isStyle && CurrentRaceVal == raceVal) {//return style to previous value if no race change
				styleVal = CurrentStyleVal;
			}
			CurrentRaceVal = raceVal;
			CurrentStyleVal = styleVal;
		} else if (button == 0x1B) {//exit -ESC button pushed
			exitMenu = -1;
		}
	}

	RefreshArtCache();
	LoadHeroDat(CurrentRaceVal, CurrentStyleVal);
	SetAppearanceGlobals(CurrentRaceVal, CurrentStyleVal);

	DestroyWin(WinRef);
	delete[]mainSurface;
	delete[]ConDraw;
	UnloadFrm(MenuUObj);
	UnloadFrm(MenuDObj);
	MenuUSurface = NULL;
	MenuDSurface = NULL;

	UnloadFrm(DidownUObj);
	UnloadFrm(DidownDObj);
	DidownUSurface = NULL;
	DidownDSurface = NULL;

	UnloadFrm(DiupUObj);
	UnloadFrm(DiupDObj);
	DiupUSurface = NULL;
	DiupDSurface = NULL;

	SetFont(oldFont);
	SetMousePic(oldMouse);

	if (mouseWasHidden) {
		HideMouse();
	}
}

void FixTextHighLight() {
	__asm {
		//redraw special text
		mov eax, 7
		xor ebx, ebx
		xor edx, edx
		call PrintBasicStat_
		//redraw trait options text
		call ListTraits_
		//redraw skills text
		xor eax, eax
		call ListSkills_
		//redraw level text
		call PrintLevelWin_
		//redraw perks, karma, kill text
		call DrawFolder_
		//redraw hit points to crit chance text
		call ListDrvdStats_
		//redraw note pad area text
		//call DrawInfoWin_
	}
}

//-------------------------------------------
void _stdcall DrawCharNoteNewChar(bool Style) {
	DrawCharNote(Style, *(DWORD*)_edit_win, 348, 272, CharScrnBackSurface, 348, 272, 640, 480);
}

//-------------------------------------------------------------------
int _stdcall CheckCharButtons() {
	int button = check_buttons();

	int raceVal = CurrentRaceVal;
	int styleVal = CurrentStyleVal;

	int drawFlag = -1;

	if (*(DWORD*)_info_line == 0x503) {
		button = 0x501;
	} else if (*(DWORD*)_info_line == 0x504) {
		button = 0x502;
	} else if (*(DWORD*)_info_line == 0x501 || *(DWORD*)_info_line == 0x502) {
		switch (button) {
			case 0x14B: //button =left
			case 0x14D: //button =right
				if (*(DWORD*)_glblmode == 1) { //if in char creation scrn
					if (*(DWORD*)_info_line == 0x501) {
						button = button + 0x3C6;
					} else if (*(DWORD*)_info_line == 0x502) {
						button = button + 0x3C6 + 1;
					}
				}
			break;
			case 0x148: //button =up
			case 0x150: //button =down
				if (*(DWORD*)_info_line == 0x501) {
					button = 0x502;
				} else if (*(DWORD*)_info_line == 0x502) {
					button = 0x501;
				}
			break;
			case 0x0D: //button =return
			case 0x1B: //button =esc
			case 0x1F4: //button =done
			case 'd': //button =done
			case 'D': //button =done
			case 0x1F6: //button =cancel
			case 'c': //button =cancel
			case 'C': //button =cancel
			if (*(DWORD*)_info_line == 0x501) { //for redrawing note when reentering char screen
				*(DWORD*)_info_line = 0x503;
			} else {
				*(DWORD*)_info_line = 0x504;
			}
			break;

			default:
			break;
		}
	}

	switch (button) {
		case 0x9: //tab button pushed
			if (*(DWORD*)_info_line >= 0x3D && *(DWORD*)_info_line < 0x4F) { //if menu ref in last menu go to race
				button = 0x501, drawFlag = 0;
			}
		break;
		case 0x501: //race button pushed
			drawFlag = 0;
		break;
		case 0x502: //style button pushed
			drawFlag = 1;
		break;
		case 0x511: //race left button pushed
			RefreshArtCache();

			if (raceVal > 0) styleVal = 0, raceVal--;

			if (LoadHeroDat(raceVal, styleVal) != 0) {
				raceVal = 0;
				LoadHeroDat(raceVal, styleVal);
			}
			drawFlag = 0;
		break;
		case 0x513: //race right button pushed
			RefreshArtCache();

			styleVal = 0, raceVal++;
			if (LoadHeroDat(raceVal, styleVal) != 0) {
				raceVal--;
				LoadHeroDat(raceVal, styleVal);
			}
			drawFlag = 0;
		break;
		case 0x512: //style left button pushed
			RefreshArtCache();

			if (styleVal > 0) styleVal--;

			if (LoadHeroDat(raceVal, styleVal) != 0) {
				styleVal = 0;
				LoadHeroDat(raceVal, styleVal);
			}
			drawFlag = 1;
		break;
		case 0x514: //style right button pushed
			RefreshArtCache();

			styleVal++;
			if (LoadHeroDat(raceVal, styleVal) != 0) {
				styleVal--;
				LoadHeroDat(raceVal, styleVal);
			}
			drawFlag = 1;
		break;
		default:
		break;
	}
	CurrentRaceVal = raceVal;
	CurrentStyleVal = styleVal;

	if (drawFlag == 1) {
		PlayAcm("ib3p1xx1");
		*(DWORD*)_info_line = 0x502;
		FixTextHighLight();
		DrawCharNoteNewChar(1);
		//DrawCharNote(1, *(DWORD*)_edit_win, 348, 272, CharScrnBackSurface, 348, 272, 640, 480);
	}
	else if (drawFlag == 0) {
		PlayAcm("ib3p1xx1");
		*(DWORD*)_info_line = 0x501;
		FixTextHighLight();
		DrawCharNoteNewChar(0);
		//DrawCharNote(0, *(DWORD*)_edit_win, 348, 272, CharScrnBackSurface, 348, 272, 640, 480);
	}

	DrawPCConsole(); //(*(DWORD*)_edit_win, 338, 78, CharScrnBackSurface, 338, 78, 640, 480);

	return button;
}

//------------------------------------------
static void __declspec(naked) CheckCharScrnButtons(void) {
	__asm {
		call CheckCharButtons
		cmp eax, 0x500
		jl EndFunc
		cmp eax, 0x515
		jg EndFunc
		pop eax //ditch old ret addr
		push 0x431E8A //recheck buttons if app mod button
EndFunc:
		ret
	}
}

//-------------------------------
void DeleteCharSurfaces() {
	delete[]NewButt01Surface;
	NewButt01Surface = NULL;
	delete[]CharScrnBackSurface;
	CharScrnBackSurface = NULL;
}

//------------------------------------------
static void __declspec(naked) CharScrnEnd(void) {
	__asm {
		pushad
		call DeleteCharSurfaces
		popad
		mov ebp, dword ptr ds:[_info_line]
		retn
	}
}

//------------------------------------------
static void __declspec(naked) SexScrnEnd(void) {
	__asm {
		pushad
		mov edx, STAT_gender
		mov eax, dword ptr ds:[_obj_dude]
		call stat_level_ //get PC stat current gender
		mov ecx, eax
		call SexWindow_ //call gender selection window
		mov edx, STAT_gender
		mov eax, dword ptr ds:[_obj_dude]
		call stat_level_ //get PC stat current gender
		cmp ecx, eax //check if gender has been changed
		je EndFunc

		xor ebx, ebx
		//cmp byte ptr ds:[_gmovie_played_list + 0x3],1 //check if wearing vault suit
		//jne NoVaultSuit
		//mov ebx, 0x8
//NoVaultSuit:
		test eax, eax //check if male 0
		jnz IsFemale
		mov eax, dword ptr ds:[ebx + _art_vault_person_nums] //base male model
		jmp ChangeSex
IsFemale:
		mov eax, dword ptr ds:[ebx + 0x5108AC] //base female model
ChangeSex:
		call AdjustHeroBaseArt
		//mov dword ptr ds:[_art_vault_guy_num], eax //current base dude model
		mov eax, dword ptr ds:[_obj_dude] //dude state structure
		call inven_worn_
		mov CurrentRaceVal, 0 //reset race and style to defaults
		mov CurrentStyleVal, 0

		push CurrentStyleVal
		push CurrentRaceVal
		call LoadHeroDat
		call RefreshPCArt
		//Check If Race or Style selected to redraw info note
		cmp dword ptr ds:[_info_line], 0x501
		jne CheckIfStyle
		push 0
		call DrawCharNoteNewChar
CheckIfStyle:
		cmp dword ptr ds:[_info_line], 0x502
		jne EndFunc
		push 1
		call DrawCharNoteNewChar

EndFunc:
		popad
		retn
	}
}

//------------------------------------------
static void __declspec(naked) AddCharScrnButtons(void) {
	__asm {
		push ebp //prolog
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		pushad
	}

	int WinRef;
	WinRef = *(DWORD*)_edit_win; //char screen window ref

	//race and style buttons
	CreateButton(WinRef, 332, 0, 82, 32, -1, -1, 0x501, -1, 0, 0, 0);
	CreateButton(WinRef, 332, 226, 82, 32, -1, -1, 0x502, -1, 0, 0, 0);

	if (*(DWORD*)_glblmode == 1) { //equals 1 if new char screen - equals 0 if ingame char screen
		if (NewButt01Surface == NULL) {
			NewButt01Surface = new BYTE [20*18*4];

			DWORD FrmObj; //frm objects for char screen Appearance button
			BYTE *FrmSurface;

			FrmSurface = GetFrmSurface(LoadFrm(6, 122), 0, 0, &FrmObj); //SLUFrm
			sub_draw(20, 18, 20, 18, 0, 0, FrmSurface, 20, 18*4, 0, 0, NewButt01Surface, 0x0);
			UnloadFrm(FrmObj);
			FrmSurface = GetFrmSurface(LoadFrm(6, 123), 0, 0, &FrmObj); //SLDFrm
			sub_draw(20, 18, 20, 18, 0, 0, FrmSurface, 20, 18*4, 0, 18, NewButt01Surface, 0x0);
			UnloadFrm(FrmObj);
			FrmSurface = GetFrmSurface(LoadFrm(6, 124), 0, 0, &FrmObj); //SRUFrm
			sub_draw(20, 18, 20, 18, 0, 0, FrmSurface, 20, 18*4, 0, 18*2, NewButt01Surface, 0x0);
			UnloadFrm(FrmObj);
			FrmSurface = GetFrmSurface(LoadFrm(6, 125), 0, 0, &FrmObj); //SRDFrm
			sub_draw(20, 18, 20, 18, 0, 0, FrmSurface, 20, 18*4, 0, 18*3, NewButt01Surface, 0x0);
			UnloadFrm(FrmObj);
			FrmSurface = NULL;
		}

		//check if Data exists for other races male or female, and if so enable race selection buttons.
		if (GetFileAttributes("Appearance\\hmR01S00\0") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR01S00\0") != INVALID_FILE_ATTRIBUTES ||
			GetFileAttributes("Appearance\\hmR01S00.dat\0") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR01S00.dat\0") != INVALID_FILE_ATTRIBUTES) {
			//race selection buttons
			CreateButton(WinRef, 348, 37, 20, 18, -1, -1, -1, 0x511, NewButt01Surface, NewButt01Surface + (20*18), 0x20);
			CreateButton(WinRef, 373, 37, 20, 18, -1, -1, -1, 0x513, NewButt01Surface + (20*18*2), NewButt01Surface + (20*18*3), 0x20);
		}
		//style selection buttons
		CreateButton(WinRef, 348, 199, 20, 18, -1, -1, -1, 0x512, NewButt01Surface, NewButt01Surface+(20*18), 0x20);
		CreateButton(WinRef, 373, 199, 20, 18, -1, -1, -1, 0x514, NewButt01Surface + (20*18*2), NewButt01Surface + (20*18*3), 0x20);
	}

	__asm {
		popad
		mov esp, ebp //epilog
		pop ebp
		//move tag skills button to fit Appearance interface
		mov edx, 0x1AA; //0x18C+36 was 0x18C tag/skills button xpos
		retn
	}
}

//------------------------------------------
static void __declspec(naked) FixCharScrnBack(void) {
//00432B92  |. A3 A4075700    MOV DWORD PTR DS:[5707A4],EAX
	__asm {
		mov dword ptr ds:[_bckgnd], eax //surface ptr for char scrn back
		test eax, eax //check if frm loaded ok
		je EndFunc

		push ebp // prolog
		mov ebp, esp
		sub esp, __LOCAL_SIZE
		pushad
	}

	if (CharScrnBackSurface == NULL) {
		CharScrnBackSurface = new BYTE [640*480];

		BYTE *OldCharScrnBackSurface;
		OldCharScrnBackSurface = *(BYTE**)_bckgnd; //char screen background frm surface

		//copy old charscrn surface to new
		sub_draw(640, 480, 640, 480, 0, 0, OldCharScrnBackSurface, 640, 480, 0, 0, CharScrnBackSurface, 0);

		//copy Tag Skill Counter background to the right
		sub_draw(38, 26, 640, 480, 519, 228, OldCharScrnBackSurface, 640, 480, 519+36, 228, CharScrnBackSurface, 0);
		//copy a blank part of the Tag Skill Bar hiding the old counter
		sub_draw(38, 26, 640, 480, 460, 228, OldCharScrnBackSurface, 640, 480, 519, 228, CharScrnBackSurface, 0);

		sub_draw(36, 258, 640, 480, 332, 0, OldCharScrnBackSurface, 640, 480, 408, 0, CharScrnBackSurface, 0); //shift behind button rail
		sub_draw(6, 32, 640, 480, 331, 233, OldCharScrnBackSurface, 640, 480, 330, 6, CharScrnBackSurface, 0); //shadow for style/race button


		DWORD FrmObj, FrmMaskObj; //frm objects for char screen Appearance button
		BYTE *FrmSurface,*FrmMaskSurface;

		FrmSurface = GetFrmSurface(LoadFrm(6, 113), 0, 0, &FrmObj);
		sub_draw(81, 132, 292, 376, 163, 20, FrmSurface, 640, 480, 331, 63, CharScrnBackSurface, 0); //char view win
		sub_draw(79, 31, 292, 376, 154, 228, FrmSurface, 640, 480, 331, 32, CharScrnBackSurface, 0); //upper  char view win
		sub_draw(79, 30, 292, 376, 158, 236, FrmSurface, 640, 480, 331, 195, CharScrnBackSurface, 0); //lower  char view win
		UnloadFrm(FrmObj);

		//Sexoff Frm
		FrmSurface = GetFrmSurface(LoadFrm(6, 188), 0, 0, &FrmObj);
		//Sex button mask frm
		FrmMaskSurface = GetFrmSurface(LoadFrm(6, 187), 0, 0, &FrmMaskObj);

		sub_draw(80, 28, 80, 32, 0, 0, FrmMaskSurface, 80, 32, 0, 0, FrmSurface, 0x39); //mask for style and race buttons
		UnloadFrm(FrmMaskObj);
		FrmMaskSurface = NULL;

		FrmSurface[80*32 - 1] = 0;
		FrmSurface[80*31 - 1] = 0;
		FrmSurface[80*30 - 1] = 0;

		FrmSurface[80*32 - 2] = 0;
		FrmSurface[80*31 - 2] = 0;
		FrmSurface[80*30 - 2] = 0;

		FrmSurface[80*32 - 3] = 0;
		FrmSurface[80*31 - 3] = 0;
		FrmSurface[80*30 - 3] = 0;

		FrmSurface[80*32 - 4] = 0;
		FrmSurface[80*31 - 4] = 0;
		FrmSurface[80*30 - 4] = 0;

		sub_draw(80, 32, 80, 32, 0, 0, FrmSurface, 640, 480, 332, 0, CharScrnBackSurface, 0); //style and race buttons
		sub_draw(80, 32, 80, 32, 0, 0, FrmSurface, 640, 480, 332, 225, CharScrnBackSurface, 0); //style and race buttons
		UnloadFrm(FrmObj);

		//frm background for char screen Appearance button
		FrmSurface = GetFrmSurface(LoadFrm(6, 174), 0, 0, &FrmObj); //Pickchar frm
		sub_draw(69, 20, 640, 480, 282, 320, FrmSurface, 640, 480, 337, 37, CharScrnBackSurface, 0); //button backround top
		sub_draw(69, 20, 640, 480, 282, 320, FrmSurface, 640, 480, 337, 199, CharScrnBackSurface, 0); //button backround bottom
		sub_draw(47, 16, 640, 480, 94, 394, FrmSurface, 640, 480, 347, 39, CharScrnBackSurface, 0); //cover buttons pics top
		sub_draw(47, 16, 640, 480, 94, 394, FrmSurface, 640, 480, 347, 201, CharScrnBackSurface, 0); //cover buttons pics bottom
		UnloadFrm(FrmObj);
		FrmSurface = NULL;

		int oldFont;
		oldFont = GetFont();
		SetFont(0x67);
		BYTE PeanutButter;
		PeanutButter = *(BYTE*)0x6A82F3; //palette offset stored in mem
		char RaceText[8], StyleText[8];
		DWORD raceTextWidth, styleTextWidth;

		//Get alternate text from ini if available
		GetPrivateProfileString("AppearanceMod", "RaceText", "Race", RaceText, 8, translationIni);
		GetPrivateProfileString("AppearanceMod", "StyleText", "Style", StyleText, 8, translationIni);

		raceTextWidth = GetTextWidth(RaceText);
		styleTextWidth = GetTextWidth(StyleText);

		PrintText(RaceText, PeanutButter, 372 - raceTextWidth/2, 6, raceTextWidth, 640, CharScrnBackSurface);
		PrintText(StyleText, PeanutButter, 372 - styleTextWidth/2, 231, styleTextWidth, 640, CharScrnBackSurface);
		SetFont(oldFont);
	}

	__asm {
		popad
		mov esp, ebp //epilog
		pop ebp
		mov eax, CharScrnBackSurface
EndFunc:
		mov dword ptr ds:[_bckgnd], eax //surface ptr for char scrn back
		retn
	}
}


//Adjust PC SFX Name---------------------------------------------
static void __declspec(naked) FixPcSFX() {
//Skip Underscore char at the start of PC App Name
	__asm {
		mov ah, byte ptr ds:[ebx]
		cmp ah, 0x5F //check if Name begins with an '_' character
		jne ExitFunc
		add ebx, 1 //shift address to next char
ExitFunc:
		//restore original code
		mov eax, ebx
		cmp dword ptr ds:[0x518E30], 0
		ret
	}
}

/*
//Set path to normal before printing or saving character details------------
static void __declspec(naked) FixCharScrnSaveNPrint() {
//00432359  |> E8 AA580000    |CALL fallout2.00437C08
	__asm {
		push TempPathPtr //store current path
		mov eax, _paths
		mov TempPathPtr, eax //set path to normal
		push esi
		call OptionWindow_ //call char-scrn menu function
		pop esi
		pop TempPathPtr //restore stored path

		call RefreshPCArt
		ret
	}
}
*/

// Load Appearance data from GCD file------------
void _stdcall LoadGCDAppearance(void *FileStream) {
	CurrentRaceVal = 0;
	CurrentStyleVal = 0;
	DWORD temp;
	if (FReadDword(FileStream, &temp) != -1) {
		CurrentRaceVal = (int)temp;
		if (FReadDword(FileStream, &temp) != -1)
			CurrentStyleVal = (int)temp;
	}

	//reset hero appearance
	RefreshArtCache();
	LoadHeroDat(CurrentRaceVal, CurrentStyleVal);
	RefreshPCArt();

	FCloseFile(FileStream);
}

// Save Appearance data to GCD file--------------
void _stdcall SaveGCDAppearance(void *FileStream) {
	if (FWriteDword(FileStream, (DWORD)CurrentRaceVal) != -1) {
		FWriteDword(FileStream, (DWORD)CurrentStyleVal);
	}

	FCloseFile(FileStream);
}

// Reset Appearance when selecting "Create Character" from the New Char screen------
static void __declspec(naked) CreateCharReset() {
	__asm {
		mov CurrentRaceVal, 0 //reset race and style to defaults
		mov CurrentStyleVal, 0

		push CurrentStyleVal
		push CurrentRaceVal
		call LoadHeroDat
		call RefreshPCArt

		mov eax, 1
		ret
	}
}

//---------------------------------
void HeroAppearanceModExit() {
	if (!AppModEnabled) return;

	if (HeroPathPtr) {
		delete[] HeroPathPtr->path;
		delete HeroPathPtr;
	}
	if (RacePathPtr) {
		delete[] RacePathPtr->path;
		delete RacePathPtr;
	}
}

//-------------------------------------------------
static void __declspec(naked) FixPcCriticalHitMsg() {
	__asm {
		and eax, 0x00000FFF
		cmp eax, critterListSize //check if critter art in PC range
		jle EndFunc
		sub eax, critterListSize //shift critter art index down out of hero range
EndFunc:
		ret
	}
}

//--------------------------------------------------------------------------
void EnableHeroAppearanceMod() {
	AppModEnabled = true;

	//setup paths
	HeroPathPtr = new sPath;
	RacePathPtr = new sPath;
	HeroPathPtr->path = new char[64];
	RacePathPtr->path = new char[64];

	HeroPathPtr->isDat = 0;
	RacePathPtr->isDat = 0;
	HeroPathPtr->pDat = NULL;
	RacePathPtr->pDat = NULL;

	//Check if new Appearance char scrn button pushed
	SafeWrite32(0x431E9E, (DWORD)&CheckCharScrnButtons - 0x431EA2);

	//Destroy new Appearance button mem after use
	SafeWrite16(0x4329D8, 0xE890);
	SafeWrite32(0x4329DA, (DWORD)&CharScrnEnd - 0x4329DE);

	//Check if sex has changed and reset char appearance
	SafeWrite8(0x4322E8, 0xe8);
	SafeWrite32(0x4322E9, (DWORD)&SexScrnEnd - 0x4322ED);

	//Load New Hero Art
	SafeWrite16(0x4DEEE5, 0xE890);
	SafeWrite32(0x4DEEE7, (DWORD)&LoadNewHeroArt - 0x4DEEEB);

	//Divert critter frm file name function exit for file checking
	SafeWrite8(0x419520, 0xEB); //divert func exit
	SafeWrite32(0x419521, 0x9090903E);

	//Check if new hero art exists otherwise use regular art
	SafeWrite8(0x419560, 0xE8);
	SafeWrite32(0x419561, (DWORD)&CheckHeroExist - 0x419565);

	//Double size of critter art index creating a new area for hero art
	SafeWrite16(0x4196AA, 0xE890);
	SafeWrite32(0x4196AC, (DWORD)&DoubleArt - 0x4196B0);

	//Add new hero critter names at end of critter list
	SafeWrite8(0x418B39, 0xE8);
	SafeWrite32(0x418B3A, (DWORD)&AddHeroCritNames - 0x418B3E);

	//Shift base hero critter art offset up into hero section
	SafeWrite8(0x49F9DA, 0xE8);
	SafeWrite32(0x49F9DB, (DWORD)&AdjustHeroBaseArt - 0x49F9DF);

	//Adjust hero art index offset when changing armor
	//SafeWrite8(0x4717D6, 0xE8);
	//SafeWrite32(0x4717D7, (DWORD)&AdjustHeroArmorArt - 0x4717DB);
	SafeWrite32(0x4717D2, (DWORD)&AdjustHeroArmorArt - 0x4717D6);

	//Hijack Save Hero State Structure fuction address 9CD54800
	//Return hero art index offset back to normal before saving
	SafeWrite32(0x519400, (DWORD)&SavCritNumFix);



	//Tag Skills text x pos
	SafeWrite32(0x433372, 0x24826+36); //Tag Skills text x pos1
	SafeWrite32(0x4362BE, 0x24826+36); //Tag Skills text x pos2
	SafeWrite32(0x4362F2, 0x20A+36); //Tag Skills num counter2 x pos1
	SafeWrite32(0x43631E, 0x20A+36); //Tag Skills num counter2 x pos2

	//Add new Appearance mod buttons
	SafeWrite8(0x43A788, 0xe8);
	SafeWrite32(0x43A789, (DWORD)&AddCharScrnButtons - 0x43A78D);

	//Mod char scrn background and add new app mod graphics. also adjust tag/skill button x pos
	SafeWrite8(0x432B92, 0xe8);
	SafeWrite32(0x432B93, (DWORD)&FixCharScrnBack - 0x432B97);

	//skill points
	SafeWrite32(0x436262, 0x24810+36); //Skill Points text x pos
	SafeWrite32(0x43628A, 0x20A+36); //Skill Points num counter x pos1
	SafeWrite32(0x43B5B2, 0x20A+36); //Skill Points num counter x pos2

	//make room for char view window
	SafeWrite32(0x433678, 347+76); //shift skill buttons right 80
	SafeWrite32(0x4363CE, 380+68); //shift skill name text right 80
	SafeWrite32(0x43641C, 573+10); //shift skill % num text right 80
	SafeWrite32(0x43A74C, 223-76+10); // skill list mouse area button width
	SafeWrite32(0x43A75B, 370+76); // skill list mouse area button xpos
	SafeWrite32(0x436220, 0x0DFC+68); //"skill" text xpos
	SafeWrite32(0x43A71E, 0xDF-68); //skill button width
	SafeWrite32(0x43A72A, 0x178+68); //skill button xpos

	//redraw area for skill list
	SafeWrite32(0x4361C4, 370+76); //xpos
	SafeWrite32(0x4361D9, 270-76); //width
	SafeWrite32(0x4361DE, 370+76); //xpos

	//skill slider thingy
	SafeWrite32(0x43647C, 592+3); //xpos
	SafeWrite32(0x4364FA, 614+3); //plus button xpos
	SafeWrite32(0x436567, 614+3); //minus button xpos

	//fix for Char Screen note position was x484 y309 now x383 y308
	//SafeWrite32(0x43AB55, 308*640+483); //minus button xpos

	//Adjust PC SFX Name
	SafeWrite8(0x4510EB, 0xE8);
	SafeWrite32(0x4510EC, (DWORD)&FixPcSFX - 0x4510F0);
	SafeWrite16(0x4510F0, 0x9090);

	//Set path to normal before printing or saving character details
	//SafeWrite32(0x43235A, (DWORD)&FixCharScrnSaveNPrint - 0x43235E);

	//Load Appearance data from GCD file------------
	SafeWrite16(0x42DF5D, 0x9056); //push esi "*FileStream"
	SafeWrite32(0x42DF60, (DWORD)&LoadGCDAppearance - 0x42DF64);

	//Save Appearance data to GCD file------------
	SafeWrite16(0x42E161, 0x9056); //push esi "*FileStream"
	SafeWrite32(0x42E164, (DWORD)&SaveGCDAppearance - 0x42E168);

	//Reset Appearance when selecting "Create Character" from the New Char screen------
	SafeWrite8(0x4A7405, 0xE8);
	SafeWrite32(0x4A7406, (DWORD)&CreateCharReset - 0x4A740A);

	//Fixes missing console critical hit messages when PC is attacked.
	//00426135  |.  25 FF0F0000                      AND EAX,00000FFF
	SafeWrite8(0x426135, 0xE8);
	SafeWrite32(0x426136, (DWORD)&FixPcCriticalHitMsg - 0x42613A);

	//Force Criticals For Testing
	//00423A8F  |. /2E:FF249D 7C374200    JMP DWORD PTR CS:[EBX*4+42377C]
	//SafeWrite32(0x423A8F, 0x90909090);
	//SafeWrite32(0x423A93, 0x90909090);
}
