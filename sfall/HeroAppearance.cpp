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
#include "PartyControl.h"
#include "ScriptExtender.h"

bool appModEnabled = false; // check if Appearance mod enabled for script fuctions

const char* appearancePathFmt = "Appearance\\h%cR%02dS%02d%s";

// char scrn surfaces
BYTE *newButtonSurface = nullptr;
BYTE *charScrnBackSurface = nullptr;

// char scrn critter rotation vars
DWORD charRotTick = 0;
DWORD charRotOri = 0;

int currentRaceVal = 0, currentStyleVal = 0;     // holds Appearance values to restore after global reset in NewGame2 function in LoadGameHooks.cpp
DWORD critterListSize = 0, critterArraySize = 0; // Critter art list size

// fallout2 path node structure
struct sPath {
	char* path;
	void* pDat;
	DWORD isDat;
	sPath* next;
};

sPath **tempPathPtr = (sPath**)_paths;
sPath *heroPathPtr = nullptr;
sPath *racePathPtr = nullptr;

// for word wrapping
typedef struct LineNode {
	DWORD offset;
	LineNode *next;

	LineNode() {
		next = nullptr;
		offset = 0;
	}
} LineNode;

// structures for loading unlisted frms
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
		indexBuff = nullptr;
	}
	~UNLSTDframe() {
		if (indexBuff != nullptr)
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
		frames = nullptr;
	}
	~UNLSTDfrm() {
		if (frames != nullptr)
			delete[] frames;
	}
} UNLSTDfrm;

/////////////////////////////////////////////////////////////////FILE FUNCTIONS////////////////////////////////////////////////////////////////////////

void* FOpenFile(char *FileName, char *flags) {
	void *retVal;
	__asm {
		mov  edx, flags;
		mov  eax, FileName;
		call xfopen_;
		mov retVal, eax;
	}
	return retVal;
}

int FCloseFile(void *FileStream) {
	int retVal;
	__asm {
		mov  eax, FileStream;
		call db_fclose_;
		mov  retVal, eax;
	}
	return retVal;
}

int Ffseek(void *FileStream, long fOffset, int origin) {
	int retVal;
	__asm {
		mov  ebx, origin;
		mov  edx, fOffset;
		mov  eax, FileStream;
		call xfseek_;
		mov  retVal, eax;
	}
	return retVal;
}

int FReadByte(void *FileStream, BYTE *toMem) {
	int retVal;
	__asm {
		mov  edx, toMem;
		mov  eax, FileStream;
		call db_freadByte_;
		mov  retVal, eax;
	}
	return retVal;
}

int FReadWord(void *FileStream, WORD *toMem) {
	int retVal;
	__asm {
		mov  edx, toMem;
		mov  eax, FileStream;
		call db_freadShort_;
		mov  retVal, eax;
	}
	return retVal;
}

int FReadDword(void *FileStream, DWORD *toMem) {
	int retVal;
	__asm {
		mov  edx, toMem;
		mov  eax, FileStream;
		call db_freadInt_;
		mov  retVal, eax;
	}
	return retVal;
}

int FReadWordArray(void *FileStream, WORD *toMem, DWORD NumElements) {
	int retVal;
	__asm {
		mov  ebx, NumElements;
		mov  edx, toMem;
		mov  eax, FileStream;
		call db_freadShortCount_;
		mov  retVal, eax;
	}
	return retVal;
}

int FReadDwordArray(void *FileStream, DWORD *toMem, DWORD NumElements) {
	int retVal;
	__asm {
		mov  ebx, NumElements;
		mov  edx, toMem;
		mov  eax, FileStream;
		call db_freadIntCount_;
		mov  retVal, eax;
	}
	return retVal;
}

int FReadString(void *FileStream, char *toMem, DWORD charLength, DWORD NumStrings) {
	int retVal;
	__asm {
		mov  ecx, FileStream;
		mov  ebx, NumStrings;
		mov  edx, charLength;
		mov  eax, toMem;
		call db_fread_;
		mov  retVal, eax;
	}
	return retVal;
}

int FWriteByte(void *FileStream, BYTE bVal) {
	int retVal;
	__asm {
		xor  edx, edx;
		mov  dl, bVal;
		mov  eax, FileStream;
		call db_fwriteByte_;
		mov  retVal, eax;
	}
	return retVal;
}

int FWriteDword(void *FileStream, DWORD bVal) {
	int retVal;
	__asm {
		mov  edx, bVal;
		mov  eax, FileStream;
		call db_fwriteInt_;
		mov  retVal, eax;
	}
	return retVal;
}

/////////////////////////////////////////////////////////////////MOUSE FUNCTIONS////////////////////////////////////////////////////////////////////////

// get current mouse pic ref
int GetMousePic() {
	return *(DWORD*)_gmouse_current_cursor;
}

// set mouse pic
int SetMousePic(int picNum) {
	__asm {
		mov  eax, picNum;
		call gmouse_set_cursor_;
		mov  picNum, eax;
	}
	return picNum; // 0 = success, -1 = fail
}

void GetMousePos(int *x_out, int *y_out) {
	__asm {
		push esi;
		mov  edx, y_out;
		mov  eax, x_out;
		call mouse_get_position_;
		pop  esi;
	}
}

void ShowMouse() {
	__asm {
		call mouse_show_;
	}
}

void HideMouse() {
	__asm {
		call mouse_hide_;
	}
}

// returns 0 if mouse is hidden
int IsMouseHidden() {
	return *(DWORD*)_mouse_is_hidden;
}

/////////////////////////////////////////////////////////////////FRM FUNCTIONS////////////////////////////////////////////////////////////////////////

static DWORD BuildFrmId(DWORD lstRef, DWORD lstNum) {
	DWORD frmID;
	__asm {
		push 0;
		xor  ecx, ecx;
		xor  ebx, ebx;
		mov  edx, lstNum;
		mov  eax, lstRef;
		call art_id_;
		mov  frmID, eax;
	}
	return frmID;
}

void UnloadFrm(DWORD FrmObj) {
	__asm {
		mov  eax, FrmObj;
		call art_ptr_unlock_;
	}
}

BYTE* GetFrmSurface(DWORD FrmID, DWORD FrameNum, DWORD Ori, DWORD *FrmObj_out) {
	BYTE *Surface;
	__asm {
		mov  ecx, FrmObj_out; // 0x518F4C
		mov  ebx, Ori;
		mov  edx, FrameNum;
		mov  eax, FrmID;
		call art_ptr_lock_data_;
		mov  Surface, eax;
	}
	return Surface;
}

/*
BYTE* GetFrmSurface2(DWORD FrmID, DWORD *FrmObj_out, DWORD *frmWidth_out, DWORD *frmHeight_out) {
	BYTE *Surface;
	__asm {
		mov  ecx, frmHeight_out;
		mov  ebx, frmWidth_out;
		mov  edx, FrmObj_out; // 0x518F4C
		mov  eax, FrmID;
		call art_lock_;
		mov  Surface, eax;
	}
	return Surface;
}
*/

FrmHeaderData* GetFrm(DWORD FrmID, DWORD *FrmObj_out) {
	FrmHeaderData* Frm;
	__asm {
		mov  edx, FrmObj_out;
		mov  eax, FrmID;
		call art_ptr_lock_;
		mov  Frm, eax;
	}
	return Frm;
}

DWORD GetFrmFrameWidth(FrmHeaderData* Frm, DWORD FrameNum, DWORD Ori) {
	DWORD Width;
	__asm {
		mov  ebx, Ori; // 0-5
		mov  edx, FrameNum;
		mov  eax, Frm;
		call art_frame_width_;
		mov  Width, eax;
	}
	return Width;
}

DWORD GetFrmFrameHeight(FrmHeaderData* Frm, DWORD FrameNum, DWORD Ori) {
	DWORD Height;
	__asm {
		mov  ebx, Ori; // 0-5
		mov  edx, FrameNum;
		mov  eax, Frm;
		call art_frame_length_;
		mov  Height, eax;
	}
	return Height;
}

BYTE* GetFrmFrameSurface(FrmHeaderData* Frm,  DWORD FrameNum, DWORD Ori) {
	BYTE *Surface;
	__asm {
		mov  ebx, Ori; // 0-5
		mov  edx, FrameNum;
		mov  eax, Frm;
		call art_frame_data_;
		mov  Surface, eax;
	}
	return Surface;
}

/////////////////////////////////////////////////////////////////UNLISTED FRM FUNCTIONS////////////////////////////////////////////////////////////////////////

static bool LoadFrmHeader(UNLSTDfrm *frmHeader, void* frmStream) {
	if (FReadDword(frmStream, &frmHeader->version) == -1)
		return false;
	else if (FReadWord(frmStream, &frmHeader->FPS) == -1)
		return false;
	else if (FReadWord(frmStream, &frmHeader->actionFrame) == -1)
		return false;
	else if (FReadWord(frmStream, &frmHeader->numFrames) == -1)
		return false;
	else if (FReadWordArray(frmStream, frmHeader->xCentreShift, 6) == -1)
		return false;
	else if (FReadWordArray(frmStream, frmHeader->yCentreShift, 6) == -1)
		return false;
	else if (FReadDwordArray(frmStream, frmHeader->oriOffset, 6) == -1)
		return false;
	else if (FReadDword(frmStream, &frmHeader->frameAreaSize) == -1)
		return false;

	return true;
}

static bool LoadFrmFrame(UNLSTDframe *frame, void* frmStream) {
	//FRMframe *frameHeader = (FRMframe*)frameMEM;
	//BYTE* frameBuff = frame + sizeof(FRMframe);

	if (FReadWord(frmStream, &frame->width) == -1)
		return false;
	else if (FReadWord(frmStream, &frame->height) == -1)
		return false;
	else if (FReadDword(frmStream, &frame->size) == -1)
		return false;
	else if (FReadWord(frmStream, &frame->x) == -1)
		return false;
	else if (FReadWord(frmStream, &frame->y) == -1)
		return false;

	frame->indexBuff = new BYTE[frame->size];
	if (FReadString(frmStream, (char*)frame->indexBuff, frame->size, 1) != 1)
		return false;

	return true;
}

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef) {
	if (folderRef > 10) return nullptr;

	char *artfolder = (char*)(0x51073C + folderRef * 32); // address of art type name
	char FrmPath[MAX_PATH];

	sprintf_s(FrmPath, MAX_PATH, "art\\%s\\%s", artfolder, frmName);

	UNLSTDfrm *frm = new UNLSTDfrm;

	void *frmStream = FOpenFile(FrmPath, "rb");

	if (frmStream) {
		if (!LoadFrmHeader(frm, frmStream)) {
			FCloseFile(frmStream);
			delete frm;
			return nullptr;
		}

		DWORD oriOffset_1st = frm->oriOffset[0];
		DWORD oriOffset_new = 0;
		frm->frames = new UNLSTDframe[6 * frm->numFrames];
		for (int ori = 0; ori < 6; ori++) {
			if (ori == 0 || frm->oriOffset[ori] != oriOffset_1st) {
				frm->oriOffset[ori] = oriOffset_new;
				for (int fNum = 0; fNum < frm->numFrames; fNum++) {
					if (!LoadFrmFrame(&frm->frames[oriOffset_new + fNum], frmStream)) {
						FCloseFile(frmStream);
						delete frm;
						return nullptr;
					}
				}
				oriOffset_new += frm->numFrames;
			} else {
				frm->oriOffset[ori] = 0;
			}
		}

		FCloseFile(frmStream);
	} else {
		delete frm;
		return nullptr;
	}
	return frm;
}

/////////////////////////////////////////////////////////////////WINDOW FUNCTIONS////////////////////////////////////////////////////////////////////////

int AddWin(DWORD x, DWORD y, DWORD width, DWORD height, DWORD BGColourIndex, DWORD flags) {
	int WinRef;
	__asm {
		push flags;
		push BGColourIndex;
		mov  ecx, height;
		mov  ebx, width;
		mov  edx, y;
		mov  eax, x;
		call win_add_;
		mov  WinRef, eax;
	}
	return WinRef;
}

void DestroyWin(int WinRef) {
	__asm {
		mov  eax, WinRef;
		call win_delete_;
	}
}

WINinfo *GetWinStruct(int WinRef) {
	WINinfo *winStruct;
	__asm {
		push edx;
		mov  eax, WinRef;
		call GNW_find_;
		mov  winStruct, eax;
		pop  edx;
	}
	return winStruct;
}

BYTE* GetWinSurface(int WinRef) {
	BYTE *surface;
	__asm {
		mov  eax, WinRef;
		call win_get_buf_;
		mov  surface, eax;
	}
	return surface;
}

void ShowWin(int WinRef) {
	__asm {
		mov  eax, WinRef;
		call win_show_;
	}
}

/*
void HideWin(int WinRef) {
	__asm {
		mov  eax, WinRef;
		call win_hide_;
	}
}
*/

/////////////////////////////////////////////////////////////////BUTTON FUNCTIONS////////////////////////////////////////////////////////////////////////

int check_buttons() {
	int key_code;
	__asm {
		call get_input_;
		mov  key_code, eax;
	}
	return key_code;
}

/////////////////////////////////////////////////////////////////TEXT FUNCTIONS////////////////////////////////////////////////////////////////////////

void SetFont(long ref) {
	__asm {
		mov  eax, ref;
		call text_font_;
	}
}

long GetFont() {
	return *(DWORD*)_curr_font_num;
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

// print text to surface
void PrintText(char *DisplayText, BYTE ColourIndex, DWORD Xpos, DWORD Ypos, DWORD TxtWidth, DWORD ToWidth, BYTE *ToSurface) {
	DWORD posOffset = Ypos * ToWidth + Xpos;
	__asm {
		xor  eax, eax;
		mov  al, ColourIndex;
		push eax;
		mov  edx, DisplayText;
		mov  ebx, TxtWidth;
		mov  ecx, ToWidth;
		mov  eax, ToSurface;
		add  eax, posOffset;
		call dword ptr ds:[_text_to_buf];
	}
}

// gets the height of the currently selected font
DWORD GetTextHeight() {
	DWORD TxtHeight;
	__asm {
		call dword ptr ds:[_text_height]; // get text height
		mov  TxtHeight, eax;
	}
	return TxtHeight;
}

// gets the length of a string using the currently selected font
DWORD GetTextWidth(char *TextMsg) {
	DWORD TxtWidth;
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[_text_width]; // get text width
		mov  TxtWidth, eax;
	}
	return TxtWidth;
}

// get width of Char for current font
DWORD GetCharWidth(char CharVal) {
	DWORD charWidth;
	__asm {
		mov  al, CharVal;
		call dword ptr ds:[_text_char_width];
		mov  charWidth, eax;
	}
	return charWidth;
}

// get maximum string length for current font - if all characters were maximum width
DWORD GetMaxTextWidth(char *TextMsg) {
	DWORD msgWidth;
	__asm {
		mov  eax, TextMsg;
		call dword ptr ds:[_text_mono_width];
		mov  msgWidth, eax;
	}
	return msgWidth;
}

// get number of pixels between characters for current font
DWORD GetCharGapWidth() {
	DWORD gapWidth;
	__asm {
		call dword ptr ds:[_text_spacing];
		mov  gapWidth, eax;
	}
	return gapWidth;
}

// get maximum character width for current font
DWORD GetMaxCharWidth() {
	DWORD charWidth = 0;
	__asm {
		call dword ptr ds:[_text_max];
		mov  charWidth, eax;
	}
	return charWidth;
}

static bool CreateWordWrapList(char *TextMsg, DWORD WrapWidth, DWORD *lineNum, LineNode *StartLine) {
	*lineNum = 1;

	if (GetMaxCharWidth() >= WrapWidth) return false;
	if (GetTextWidth(TextMsg) < WrapWidth) return true;

	DWORD GapWidth = GetCharGapWidth();

	StartLine->next = new LineNode;
	LineNode *NextLine = StartLine->next;

	DWORD lineWidth = 0, wordWidth = 0, i = 0;
	char CurrentChar;

	while (TextMsg[i] != '\0') {
		CurrentChar = TextMsg[i++];

		int cWidth = GetCharWidth(CurrentChar) + GapWidth;
		lineWidth += cWidth;
		wordWidth += cWidth;

		if (lineWidth <= WrapWidth) {
			if (isspace(CurrentChar) || CurrentChar == '-') {
				NextLine->offset = i;
				wordWidth = 0;
			}
		} else {
			if (isspace(CurrentChar)) {
				NextLine->offset = i;
				wordWidth = 0;
			}
			lineWidth = wordWidth;
			wordWidth = 0;
			CurrentChar = '\0';
			*lineNum += 1;
			NextLine->next = new LineNode;
			NextLine = NextLine->next;
		}
		if (TextMsg[i] == '\0') NextLine->offset = 0;
	}
	return true;
}

static void DeleteWordWrapList(LineNode *CurrentLine) {
	LineNode *NextLine = nullptr;

	while (CurrentLine != nullptr) {
		NextLine = CurrentLine->next;
		delete CurrentLine;
		CurrentLine = NextLine;
	}
}

/////////////////////////////////////////////////////////////////DAT FUNCTIONS////////////////////////////////////////////////////////////////////////

static void* LoadDat(char*fileName) {
	void *dat = nullptr;
	__asm {
		mov  eax, fileName;
		call dbase_open_;
		mov  dat, eax;
	}
	return dat;
}

static void UnloadDat(void *dat) {
	__asm {
		mov  eax, dat;
		call dbase_close_;
	}
}

/////////////////////////////////////////////////////////////////OTHER FUNCTIONS////////////////////////////////////////////////////////////////////////

/*
void __fastcall DrawWinLine(int winRef, DWORD startXPos, DWORD endXPos, DWORD startYPos, DWORD endYPos, BYTE colour) {
	__asm {
		xor  eax, eax;
		mov  al, colour;
		push eax;
		push endYPos;
		mov  eax, ecx; // winRef
		mov  ecx, endXPos;
		mov  ebx, startYPos;
		//mov  edx, xStartPos;
		call win_line_;
	}
}
*/

static void PlayAcm(char *acmName) {
	__asm {
		mov  eax, acmName;
		call gsound_play_sfx_file_;
	}
}

void _stdcall RefreshArtCache() {
	__asm {
		call art_flush_;
	}
}

void _stdcall RefreshHeroBaseArt() {
	__asm {
		call proto_dude_update_gender_;
	}
}

/*
// Check fallout file and get file size (result 0 - file exists)
long __stdcall db_dir_entry(const char *fileName, DWORD *sizeOut) {
	__asm {
		mov  edx, sizeOut;
		mov  eax, fileName;
		call db_dir_entry_;
	}
}
*/

/////////////////////////////////////////////////////////////////APP MOD FUNCTIONS////////////////////////////////////////////////////////////////////////

static char _stdcall GetSex() {
	char sex;
	__asm {
		mov  edx, STAT_gender;              // sex stat ref
		mov  eax, dword ptr ds:[_obj_dude]; // hero state structure
		call stat_level_;                   // get Player stat val
		test eax, eax;                      // male=0, female=1
		jne  female;
		mov  sex, 'M';
		jmp  endFunc;
female:
		mov  sex, 'F';
endFunc:
	}
	return sex;
}

static __declspec(noinline) int _stdcall LoadHeroDat(unsigned int race, unsigned int style, bool flush = false) {
	if (flush) RefreshArtCache();

	if (heroPathPtr->pDat) { // unload previous Dats
		UnloadDat(heroPathPtr->pDat);
		heroPathPtr->pDat = nullptr;
		heroPathPtr->isDat = 0;
	}
	if (racePathPtr->pDat) {
		UnloadDat(racePathPtr->pDat);
		racePathPtr->pDat = nullptr;
		racePathPtr->isDat = 0;
	}

	const char sex = GetSex();

	sprintf_s(heroPathPtr->path, 64, appearancePathFmt, sex, race, style, ".dat");
	int result = GetFileAttributes(heroPathPtr->path);
	if (result != INVALID_FILE_ATTRIBUTES && !(result & FILE_ATTRIBUTE_DIRECTORY)) { // check if Dat exists for selected appearance
		heroPathPtr->pDat = LoadDat(heroPathPtr->path);
		heroPathPtr->isDat = 1;
	} else {
		sprintf_s(heroPathPtr->path, 64, appearancePathFmt, sex, race, style, "");
		if (GetFileAttributes(heroPathPtr->path) == INVALID_FILE_ATTRIBUTES) // check if folder exists for selected appearance
			return -1;
	}

	tempPathPtr = &heroPathPtr; // set path for selected appearance
	heroPathPtr->next = *(sPath**)_paths;

	if (style != 0) {
		sprintf_s(racePathPtr->path, 64, appearancePathFmt, sex, race, 0, ".dat");
		int result = GetFileAttributes(racePathPtr->path);
		if (result != INVALID_FILE_ATTRIBUTES && !(result & FILE_ATTRIBUTE_DIRECTORY)) { // check if Dat exists for selected race base appearance
			racePathPtr->pDat = LoadDat(racePathPtr->path);
			racePathPtr->isDat = 1;
		} else {
			sprintf_s(racePathPtr->path, 64, appearancePathFmt, sex, race, 0, "");
		}

		if (GetFileAttributes(racePathPtr->path) != INVALID_FILE_ATTRIBUTES) { // check if folder/Dat exists for selected race base appearance
			heroPathPtr->next = racePathPtr; // set path for selected race base appearance
			racePathPtr->next = *(sPath**)_paths;
		}
	}
	return 0;
}

// insert hero art path in front of main path structure when loading art
static void __declspec(naked) LoadNewHeroArt() {
	__asm {
		cmp byte ptr ds:[esi], 'r';
		je  isReading;
		mov ecx, _paths;
		jmp setPath;
isReading:
		mov ecx, tempPathPtr;
setPath:
		mov ecx, dword ptr ds:[ecx];
		retn;
	}
}

static void __declspec(naked) CheckHeroExist() {
	__asm {
		cmp  esi, critterArraySize;       // check if loading hero art
		jle  endFunc;
		mov  eax, _art_name;              // critter art file name address (file name)
		call db_access_;                  // check art file exists
		test eax, eax;
		jnz  endFunc;

		// if file not found load regular critter art instead
		sub  esi, critterArraySize;
		add  esp, 4;                      // drop func ret address
		mov  eax, 0x4194E2;
		jmp  eax;
endFunc:
		mov  eax, _art_name;
		retn;
	}
}

// adjust base hero art if num below hero art range
static void __declspec(naked) AdjustHeroBaseArt() {
	__asm {
		add eax, critterListSize;
		mov dword ptr ds:[_art_vault_guy_num], eax;
		retn;
	}
}

// adjust armor art if num below hero art range
static void __declspec(naked) AdjustHeroArmorArt() {
	__asm {
		pop  ebx;                     // get ret addr
		mov  dword ptr ss:[esp], ebx; // reinsert ret addr in old (push 0)
		xor  ebx, ebx;
		push 0;
		call art_id_;                 // call load frm func
		mov  dx, ax;
		and  dh, 0x0F;                // mask out current weapon flag
		cmp  edx, critterListSize;    // check if critter art in PC range
		jg   endFunc;
		add  eax, critterListSize;    // shift critter art index up into hero range
endFunc:
		//mov dword ptr ds:[_i_fid], eax;
		retn;
	}
}

static void _stdcall SetHeroArt(bool newArtFlag) {
	TGameObj* hero = *ptr_obj_dude;           // hero state struct
	long heroFID = hero->artFid;              // get hero FrmID
	DWORD fidBase = heroFID & 0xFFF;          // mask out current weapon flag

	if (fidBase > critterListSize) {          // check if critter LST index is in Hero range
		if (!newArtFlag) {
			heroFID -= critterListSize;       // shift index down into normal critter range
			hero->artFid = heroFID;
		}
	} else if (newArtFlag) {
		heroFID += critterListSize;           // shift index up into hero range
		hero->artFid = heroFID;               // set new FrmID to hero state struct
	}
}

// return hero art val to normal before saving
static void __declspec(naked) SavCritNumFix() {
	__asm {
		push ecx;
		push edx;
		push eax;
		push 0;              // set hero FrmID LST index to normal range before saving
		call SetHeroArt;
		pop  eax;
		call obj_save_dude_; // save current hero state structure fuction
		push eax;
		push 1;              // return hero FrmID LST index back to hero art range after saving hero state structure
		call SetHeroArt;
		pop  eax;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static void __declspec(naked) DoubleArt() {
	__asm {
		cmp dword ptr ss:[esp + 0xCC], 0x510774; // check if loading critter lst. 0x510774 = addr of critter list size val
		jne endFunc;
		shl edi, 1;                              // double critter list size to add room for hero art
endFunc:
		jmp db_fseek_;
	}
}

// create a duplicate list of critter names at the end with an additional '_' character at the beginning of its name
static void FixCritList() {
	//int size = (*(DWORD*)0x510774) / 2; // critter list size after resize by DoubleArt func
	critterListSize = (*(DWORD*)0x510774) / 2;
	critterArraySize = critterListSize * 13;

	char *CritList = (*(char**)0x51076C);           // critter list offset
	char *HeroList = CritList + critterArraySize;   // set start of hero critter list after regular critter list

	memset(HeroList, 0, critterArraySize);

	for (DWORD i = 0; i < critterListSize; i++) {   // copy critter name list to hero name list
		*HeroList = '_';                            // insert a '_' char at the front of new hero critt names. fallout wont load the same name twice
		memcpy(HeroList + 1, CritList, 11);
		HeroList += 13;
		CritList += 13;
	}
}

static void __declspec(naked) AddHeroCritNames() { // art_init_
	__asm {
		call FixCritList; // insert names for hero critters
		mov  eax, dword ptr ds:[_art + 0x3C];
		retn;
	}
}

///////////////////////////////////////////////////////////////GRAPHICS HERO FUNCTIONS//////////////////////////////////////////////////////////////////////

static void DrawPC() {
	RECT critRect;
	__asm {
		lea  edx, critRect;                 // out critter RECT*
		mov  eax, dword ptr ds:[_obj_dude]; // dude critter struct
		call obj_bound_;                    // get critter rect func

		mov edx, dword ptr ds:[_obj_dude];  // dude critter struct
		lea eax, critRect;                  // RECT*
		mov edx, dword ptr ds:[edx + 0x28]; // map level the dude is on
		call tile_refresh_rect_;            // draw rect area func
	}
}

// scan inventory items for armor and weapons currently being worn or wielded and setup matching FrmID for PC
void _stdcall RefreshPCArt() {
	__asm {
		call proto_dude_update_gender_;         // refresh PC base model art

		mov  eax, dword ptr ds:[_obj_dude];     // PC state struct
		mov  dword ptr ds:[_inven_dude], eax;   // inventory temp pointer to PC state struct
		mov  eax, dword ptr ds:[_inven_dude];
		lea  edx, dword ptr ds:[eax + 0x2C];
		mov  dword ptr ds:[_pud], edx;          // PC inventory

		xor  eax, eax;
		xor  edx, edx; // itemListOffset
		xor  ebx, ebx; // itemNum

		mov  dword ptr ds:[_i_rhand], eax; // item2
		mov  dword ptr ds:[_i_worn], eax;  // armor
		mov  dword ptr ds:[_i_lhand], eax; // item1
		jmp  LoopStart;

CheckNextItem:
		mov  eax, dword ptr ds:[eax + 0x8]; // PC inventory item list
		mov  eax, dword ptr ds:[edx + eax]; // PC inventory item list + itemListOffset

		test byte ptr ds:[eax + 0x27], 1; // if item in item1 slot
		jnz  IsItem1;
		test byte ptr ds:[eax + 0x27], 2; // if item in item2 slot
		jnz  IsItem2;
		test byte ptr ds:[eax + 0x27], 4; // if item in armor slot
		jnz  IsArmor;
		jmp  SetNextItem;

IsItem1:
		mov  dword ptr ds:[_i_lhand], eax; // set item1
		test byte ptr ds:[eax + 0x27], 2;  // check if same item type also in item2 slot
		jz   SetNextItem;

IsItem2:
		mov  dword ptr ds:[_i_rhand], eax; // set item2
		jmp  SetNextItem;

IsArmor:
		mov  dword ptr ds:[_i_worn], eax; // set armor

SetNextItem:
		inc  ebx;    // itemNum++
		add  edx, 8; // itemListOffset + itemsize
LoopStart:
		mov  eax, dword ptr ds:[_pud]; // PC inventory
		cmp  ebx, dword ptr ds:[eax];  // size of item list
		jl   CheckNextItem;

		// inventory function - setup pc FrmID and store at address _i_fid
		call adjust_fid_;

		// copy new FrmID to hero state struct
		mov  edx, dword ptr ds:[_i_fid];
		mov  eax, dword ptr ds:[_inven_dude];
		mov  dword ptr ds:[eax + 0x20], edx;
		//call obj_change_fid_

		xor  eax, eax;
		mov  dword ptr ds:[_i_rhand], eax; // item2
		mov  dword ptr ds:[_i_worn], eax;  // armor
		mov  dword ptr ds:[_i_lhand], eax; // item1
	}

	if (!appModEnabled) return;

	if (LoadHeroDat(currentRaceVal, currentStyleVal) != 0) {     // if load fails
		currentStyleVal = 0;                                     // set style to default
		if (LoadHeroDat(currentRaceVal, currentStyleVal) != 0) { // if race fails with style at default
			currentRaceVal = 0;                                  // set race to default
			LoadHeroDat(currentRaceVal, currentStyleVal);
		}
	}
	RefreshArtCache();
	DrawPC();
}

void _stdcall LoadHeroAppearance() {
	if (!appModEnabled) return;

	GetAppearanceGlobals(&currentRaceVal, &currentStyleVal);
	LoadHeroDat(currentRaceVal, currentStyleVal, true);
	SetHeroArt(true);
	DrawPC();
}

void _stdcall SetNewCharAppearanceGlobals() {
	if (!appModEnabled) return;

	if (currentRaceVal > 0 || currentStyleVal > 0) {
		SetAppearanceGlobals(currentRaceVal, currentStyleVal);
	}
}

// op_set_hero_style
void _stdcall SetHeroStyle(int newStyleVal) {
	if (!appModEnabled || newStyleVal == currentStyleVal) return;

	if (LoadHeroDat(currentRaceVal, newStyleVal, true) != 0) {  // if new style cannot be set
		if (currentRaceVal == 0 && newStyleVal == 0) {
			currentStyleVal = 0;                                // ignore error if appearance = default
		} else {
			LoadHeroDat(currentRaceVal, currentStyleVal);       // reload original style
		}
	} else {
		currentStyleVal = newStyleVal;
	}
	SetAppearanceGlobals(currentRaceVal, currentStyleVal);
	DrawPC();
}

// op_set_hero_race
void _stdcall SetHeroRace(int newRaceVal) {
	if (!appModEnabled || newRaceVal == currentRaceVal) return;

	if (LoadHeroDat(newRaceVal, 0, true) != 0) {          // if new race fails with style at 0
		if (newRaceVal == 0) {
			currentRaceVal = 0;
			currentStyleVal = 0;                          // ignore if appearance = default
		} else {
			LoadHeroDat(currentRaceVal, currentStyleVal); // reload original race & style
		}
	} else {
		currentRaceVal = newRaceVal;
		currentStyleVal = 0;
	}
	SetAppearanceGlobals(currentRaceVal, currentStyleVal); // store new globals
	DrawPC();
}

// Reset Appearance when selecting "Create Character" from the New Char screen
static void __declspec(naked) CreateCharReset() {
	__asm {
		cmp  currentStyleVal, 0;
		jnz  reset;
		cmp  currentRaceVal, 0;
		jz   endFunc;
reset:  // set race and style to defaults
		push edx;
		push ecx;
		xor  eax, eax;
		mov  currentRaceVal, eax;
		mov  currentStyleVal, eax;
		push eax; // flush
		push eax;
		push eax;
		call LoadHeroDat;
		pop  ecx;
		pop  edx;
		call proto_dude_update_gender_;
endFunc:
		mov  eax, 1;
		retn;
	}
}

/////////////////////////////////////////////////////////////////INTERFACE FUNCTIONS///////////////////////////////////////////////////////////////////////

static void sub_draw(long subWidth, long subHeight, long fromWidth, long fromHeight, long fromX, long fromY, BYTE *fromBuff,
					 long toWidth, long toHeight, long toX, long toY, BYTE *toBuff, int maskRef) {

	fromBuff += fromY * fromWidth + fromX;
	toBuff += toY * toWidth + toX;

	for (long h = 0; h < subHeight; h++) {
		for (long w = 0; w < subWidth; w++) {
			if (fromBuff[w] != maskRef)
				toBuff[w] = fromBuff[w];
		}
		fromBuff += fromWidth;
		toBuff += toWidth;
	}
}

static void DrawBody(DWORD critNum, BYTE* surface) {
	DWORD critFrmLock;

	FrmHeaderData *critFrm = GetFrm(BuildFrmId(1, critNum), &critFrmLock);
	DWORD critWidth = GetFrmFrameWidth(critFrm, 0, charRotOri);
	DWORD critHeight = GetFrmFrameHeight(critFrm, 0, charRotOri);
	BYTE *critSurface = GetFrmFrameSurface(critFrm, 0, charRotOri);
	sub_draw(critWidth, critHeight, critWidth, critHeight, 0, 0, critSurface, 70, 102, 35 - critWidth / 2, 51 - critHeight / 2, surface, 0);

	UnloadFrm(critFrmLock);
	critSurface = nullptr;
}

static void DrawPCConsole() {
	DWORD NewTick = *(DWORD*)0x5709C4;  // char scrn gettickcount ret
	DWORD RotSpeed = *(DWORD*)0x47066B; // get rotation speed - inventory rotation speed

	if (charRotTick > NewTick) charRotTick = NewTick;

	if (NewTick - charRotTick > RotSpeed) {
		charRotTick = NewTick;
		if (charRotOri < 5) {
			charRotOri++;
		} else {
			charRotOri = 0;
		}

		int WinRef = *(DWORD*)_edit_win; // char screen window ref
		//BYTE *WinSurface = GetWinSurface(WinRef);

		WINinfo *WinInfo = GetWinStruct(WinRef);

		BYTE *ConSurface = new BYTE [70 * 102];
		sub_draw(70, 102, 640, 480, 338, 78, charScrnBackSurface, 70, 102, 0, 0, ConSurface, 0);

		//DWORD critNum = *(DWORD*)_art_vault_guy_num; // pointer to current base hero critter FrmId
		DWORD critNum = *(DWORD*)(*(DWORD*)_obj_dude + 0x20); // pointer to current armored hero critter FrmId
		DrawBody(critNum, ConSurface);

		sub_draw(70, 102, 70, 102, 0, 0, ConSurface, WinInfo->width, WinInfo->height, 338, 78, WinInfo->surface, 0);

		delete[] ConSurface;
		WinInfo = nullptr;

		RedrawWin(WinRef);
	}
}

/*
void DrawCharNote(DWORD LstNum, char *TitleTxt, char *AltTitleTxt, char *Message) {
	__asm {
		mov  ecx, message      //dword ptr ds:[_folder_card_desc]
		mov  ebx, alttitletxt  //dword ptr ds:[_folder_card_title2]
		mov  edx, titletxt     //dword ptr ds:[_folder_card_title]
		mov  eax, lstnum       //dword ptr ds:[_folder_card_fid]
		call drawcard_
	}
}
*/

static void DrawCharNote(bool style, int winRef, DWORD xPosWin, DWORD yPosWin, BYTE *BGSurface, DWORD xPosBG, DWORD yPosBG, DWORD widthBG, DWORD heightBG) {
	MSGList MsgList;
	char *TitleMsg = nullptr;
	char *InfoMsg = nullptr;

	char *MsgFileName = (style) ? "game\\AppStyle.msg" : "game\\AppRace.msg";

	if (LoadMsgList(&MsgList, MsgFileName) == 1) {
		TitleMsg = GetMsg(&MsgList, 100, 2);
		InfoMsg = GetMsg(&MsgList, 101, 2);
	}

	WINinfo *winInfo = GetWinStruct(winRef);

	BYTE *PadSurface = new BYTE [280 * 168];
	sub_draw(280, 168, widthBG, heightBG, xPosBG, yPosBG, BGSurface, 280, 168, 0, 0, PadSurface, 0);

	UNLSTDfrm *frm = LoadUnlistedFrm((style) ? "AppStyle.frm" : "AppRace.frm", 10);
	if (frm) {
		sub_draw(frm->frames[0].width, frm->frames[0].height, frm->frames[0].width, frm->frames[0].height, 0, 0, frm->frames[0].indexBuff, 280, 168, 136, 37, PadSurface, 0); // cover buttons pics bottom
		delete frm;
	}

	int oldFont = GetFont(); // store current font
	SetFont(0x66);           // set font for title

	DWORD textHeight;
	BYTE colour = *(BYTE*)_colorTable; // black color

	if (TitleMsg != nullptr) {
		textHeight = GetTextHeight();
		PrintText(TitleMsg, colour, 0, 0, 265, 280, PadSurface);
		// draw line
		memset(PadSurface + 280 * textHeight, colour, 265);
		memset(PadSurface + 280 * (textHeight + 1), colour, 265);
	}

	DWORD lineNum = 0;
	LineNode *StartLine = new LineNode;
	LineNode *CurrentLine, *NextLine;

	if (InfoMsg != nullptr) {
		SetFont(0x65); // set font for info
		textHeight = GetTextHeight();

		if (CreateWordWrapList(InfoMsg, 160, &lineNum, StartLine)) {
			int lineHeight = 43;

			if (lineNum == 1) {
				PrintText(InfoMsg, colour, 0, lineHeight, 280, 280, PadSurface);
			} else {
				if (lineNum > 11) lineNum = 11;
				CurrentLine = StartLine;

				for (DWORD line = 0; line < lineNum; line++) {
					NextLine = CurrentLine->next;
					char TempChar = InfoMsg[NextLine->offset]; //[line+1]];
					InfoMsg[NextLine->offset] = '\0';
					PrintText(InfoMsg + CurrentLine->offset, colour, 0, lineHeight, 280, 280, PadSurface);
					InfoMsg[NextLine->offset] = TempChar;
					lineHeight += textHeight + 1;
					CurrentLine = NextLine;
				}
			}
		}
	}
	sub_draw(280, 168, 280, 168, 0, 0, PadSurface, winInfo->width, winInfo->height, xPosWin, yPosWin, winInfo->surface, 0);

	SetFont(oldFont); // restore previous font
	DestroyMsgList(&MsgList);

	*(long*)_card_old_fid1 = -1; // reset fid

	DeleteWordWrapList(StartLine);
	delete[]PadSurface;
	CurrentLine = nullptr;
	NextLine = nullptr;
	winInfo = nullptr;
}

static void _stdcall DrawCharNoteNewChar(bool type) {
	DrawCharNote(type, *(DWORD*)_edit_win, 348, 272, charScrnBackSurface, 348, 272, 640, 480);
}

// op_hero_select_win
void _stdcall HeroSelectWindow(int raceStyleFlag) {
	if (!appModEnabled) return;

	UNLSTDfrm *frm = LoadUnlistedFrm("AppHeroWin.frm", 6);
	if (frm == nullptr) {
		DebugPrintf("\nApperanceMod: art\\intrface\\AppHeroWin.frm file not found.");
		return;
	}

	bool isStyle = (raceStyleFlag != 0);
	DWORD resWidth = *(DWORD*)0x4CAD6B;
	DWORD resHeight = *(DWORD*)0x4CAD66;

	int winRef = AddWin(resWidth / 2 - 242, (resHeight - 100) / 2 - 65, 484, 230, 100, 0x4);
	if (winRef == -1) {
		delete frm;
		return;
	}

	int mouseWasHidden = IsMouseHidden();
	if (mouseWasHidden) ShowMouse();
	int oldMouse = GetMousePic();
	SetMousePic(1);

	BYTE *winSurface = GetWinSurface(winRef);
	BYTE *mainSurface = new BYTE [484 * 230];

	sub_draw(484, 230, 484, 230, 0, 0, frm->frames[0].indexBuff, 484, 230, 0, 0, mainSurface, 0);
	delete frm;

	DWORD MenuUObj, MenuDObj;
	BYTE *MenuUSurface = GetFrmSurface(BuildFrmId(6, 299), 0, 0, &MenuUObj); // MENUUP Frm
	BYTE *MenuDSurface = GetFrmSurface(BuildFrmId(6, 300), 0, 0, &MenuDObj); // MENUDOWN Frm
	WinRegisterButton(winRef, 115, 181, 26, 26, -1, -1, -1, 0x0D, MenuUSurface, MenuDSurface, 0, 0x20);

	DWORD DidownUObj, DidownDObj;
	BYTE *DidownUSurface = GetFrmSurface(BuildFrmId(6, 93), 0, 0, &DidownUObj); // MENUUP Frm
	BYTE *DidownDSurface = GetFrmSurface(BuildFrmId(6, 94), 0, 0, &DidownDObj); // MENUDOWN Frm
	WinRegisterButton(winRef, 25, 84, 24, 25, -1, -1, -1, 0x150, DidownUSurface, DidownDSurface, 0, 0x20);

	DWORD DiupUObj, DiupDObj;
	BYTE *DiupUSurface = GetFrmSurface(BuildFrmId(6, 100), 0, 0, &DiupUObj); // MENUUP Frm
	BYTE *DiupDSurface = GetFrmSurface(BuildFrmId(6, 101), 0, 0, &DiupDObj); // MENUDOWN Frm
	WinRegisterButton(winRef, 25, 59, 23, 24, -1, -1, -1, 0x148, DiupUSurface, DiupDSurface, 0, 0x20);

	int oldFont = GetFont();
	SetFont(0x67);

	char titleText[16];
	// Get alternate text from ini if available
	if (isStyle) {
		GetPrivateProfileString("AppearanceMod", "StyleText", "Style", titleText, 16, translationIni);
	} else {
		GetPrivateProfileString("AppearanceMod", "RaceText", "Race", titleText, 16, translationIni);
	}

	BYTE textColour = *(BYTE*)_PeanutButter; // PeanutButter colour - palette offset stored in mem
	DWORD titleTextWidth = GetTextWidth(titleText);
	PrintText(titleText, textColour, 92 - titleTextWidth / 2, 10, titleTextWidth, 484, mainSurface);

	GetPrivateProfileString("AppearanceMod", "DoneBtn", "Done", titleText, 16, translationIni);
	titleTextWidth = GetTextWidth(titleText);
	PrintText(titleText, textColour, 80 - titleTextWidth / 2, 185, titleTextWidth, 484, mainSurface);

	sub_draw(484, 230, 484, 230, 0, 0, mainSurface, 484, 230, 0, 0, winSurface, 0);
	ShowWin(winRef);

	SetFont(0x65);

	BYTE *ConDraw = new BYTE [70 * 102];

	int button = 0;
	bool drawFlag = true; // redraw flag for char note pad

	DWORD RotSpeed = *(DWORD*)0x47066B; // get rotation speed - inventory rotation speed
	DWORD RedrawTick = 0, NewTick = 0, OldTick = 0;

	DWORD critNum = *(DWORD*)_art_vault_guy_num; // pointer to current base hero critter FrmID
	//DWORD critNum = *(DWORD*)(*(DWORD*)_obj_dude + 0x20); // pointer to current armored hero critter FrmID

	int raceVal = currentRaceVal, styleVal = currentStyleVal; // show default style when setting race
	if (!isStyle) styleVal = 0;
	LoadHeroDat(raceVal, styleVal, true);

	while (true) {                // main loop
		NewTick = GetTickCount(); // timer for redraw
		if (OldTick > NewTick) OldTick = NewTick;

		if (NewTick - OldTick > RotSpeed) { // time to rotate critter
			OldTick = NewTick;
			if (charRotOri < 5)
				charRotOri++;
			else
				charRotOri = 0;
		}
		if (RedrawTick > NewTick) RedrawTick = NewTick;

		if (NewTick - RedrawTick > 60) { // time to redraw
			RedrawTick = NewTick;

			sub_draw(70, 102, 484, 230, 66, 53, mainSurface, 70, 102, 0, 0, ConDraw, 0);
			DrawBody(critNum, ConDraw);
			sub_draw(70, 102, 70, 102, 0, 0, ConDraw, 484, 230, 66, 53, winSurface, 0);

			if (drawFlag) {
				DrawCharNote(isStyle, winRef, 190, 29, mainSurface, 190, 29, 484, 230);
				drawFlag = false;
			}
			RedrawWin(winRef);
		}

		button = check_buttons();
		if (button == 0x148) { // previous style/race -up arrow button pushed
			PlayAcm("ib1p1xx1");

			if (isStyle) {
				if (styleVal == 0) continue;
				styleVal--;
				if (LoadHeroDat(raceVal, styleVal, true) != 0) {
					styleVal = 0;
					LoadHeroDat(raceVal, styleVal);
				}
				drawFlag = true;
			} else { // Race
				if (raceVal == 0) continue;
				raceVal--;
				styleVal = 0;
				if (LoadHeroDat(raceVal, styleVal, true) != 0) {
					raceVal = 0;
					LoadHeroDat(raceVal, styleVal);
				}
				drawFlag = true;
			}
		} else if (button == 0x150) { // Next style/race - down arrow button pushed
			PlayAcm("ib1p1xx1");

			if (isStyle) {
				styleVal++;
				if (LoadHeroDat(raceVal, styleVal, true) != 0) {
					styleVal--;
					LoadHeroDat(raceVal, styleVal);
				} else {
					drawFlag = true;
				}
			} else { // Race
				raceVal++;
				if (LoadHeroDat(raceVal, 0, true) != 0) {
					raceVal--;
					LoadHeroDat(raceVal, styleVal);
				} else {
					styleVal = 0;
					drawFlag = true;
				}
			}
		} else if (button == 0x0D) { // save and exit - Enter button pushed
			PlayAcm("ib1p1xx1");
			if (!isStyle && currentRaceVal == raceVal) { // return style to previous value if no race change
				styleVal = currentStyleVal;
			}
			currentRaceVal = raceVal;
			currentStyleVal = styleVal;
			break;
		} else if (button == 0x1B) { // exit - ESC button pushed
			break;
		}
	}

	LoadHeroDat(currentRaceVal, currentStyleVal, true);
	SetAppearanceGlobals(currentRaceVal, currentStyleVal);

	DestroyWin(winRef);
	delete[]mainSurface;
	delete[]ConDraw;

	UnloadFrm(MenuUObj);
	UnloadFrm(MenuDObj);
	MenuUSurface = nullptr;
	MenuDSurface = nullptr;

	UnloadFrm(DidownUObj);
	UnloadFrm(DidownDObj);
	DidownUSurface = nullptr;
	DidownDSurface = nullptr;

	UnloadFrm(DiupUObj);
	UnloadFrm(DiupDObj);
	DiupUSurface = nullptr;
	DiupDSurface = nullptr;

	SetFont(oldFont);
	SetMousePic(oldMouse);

	if (mouseWasHidden) HideMouse();
}

static void FixTextHighLight() {
	__asm {
		// redraw special text
		mov  eax, 7;
		xor  ebx, ebx;
		xor  edx, edx;
		call PrintBasicStat_;
		// redraw trait options text
		call ListTraits_;
		// redraw skills text
		xor  eax, eax;
		call ListSkills_;
		// redraw level text
		call PrintLevelWin_;
		// redraw perks, karma, kill text
		call DrawFolder_;
		// redraw hit points to crit chance text
		call ListDrvdStats_;
		// redraw note pad area text
		//call DrawInfoWin_;
	}
}

static int _stdcall CheckCharButtons() {
	int button = check_buttons();

	int drawFlag = -1;

	int infoLine = *(DWORD*)_info_line;
	if (infoLine == 0x503 || infoLine == 0x504) {
		*(DWORD*)_info_line -= 2;
		*(DWORD*)_frstc_draw1 = 1;
		DrawCharNoteNewChar(infoLine != 0x503);
	} else if (infoLine == 0x501 || infoLine == 0x502) {
		switch (button) {
		case 0x14B: // button left
		case 0x14D: // button right
			if (*(DWORD*)_glblmode == 1) { //if in char creation scrn
				if (infoLine == 0x501) {
					button = button + 0x3C6;
				} else if (infoLine == 0x502) {
					button = button + 0x3C6 + 1;
				}
			}
			break;
		case 0x148: // button up
		case 0x150: // button down
			if (infoLine == 0x501) {
				button = 0x502;
			} else if (infoLine == 0x502) {
				button = 0x501;
			}
			break;
		case 0x0D:  // button return
		case 0x1B:  // button esc
		case 0x1F4: // button done
		case 'd':   // button done
		case 'D':   // button done
		case 0x1F6: // button cancel
		case 'c':   // button cancel
		case 'C':   // button cancel
			*(DWORD*)_info_line += 2; // 0x503/0x504 for redrawing note when reentering char screen
			break;
		}
	}

	switch (button) {
	case 0x9: // tab button pushed
		if (infoLine < 0x3D || infoLine >= 0x4F) { // if menu ref in last menu go to race
			break;
		}
		button = 0x501;
	case 0x501: // race title button pushed
		if (infoLine != 0x501) {
			*(DWORD*)_info_line = 0x501;
			drawFlag = 3;
		}
		break;
	case 0x502: // style title button pushed
		if (infoLine != 0x502) {
			*(DWORD*)_info_line = 0x502;
			drawFlag = 2;
		}
		break;
	case 0x511: // race left button pushed
		if (currentRaceVal == 0) {
			drawFlag = 4;
			break;
		}
		currentStyleVal = 0; // reset style
		currentRaceVal--;
		if (LoadHeroDat(currentRaceVal, currentStyleVal, true) != 0) {
			currentRaceVal = 0;
			LoadHeroDat(currentRaceVal, currentStyleVal);
		}
		drawFlag = 1;
		break;
	case 0x513: // race right button pushed
		currentRaceVal++;

		if (LoadHeroDat(currentRaceVal, 0, true) != 0) {
			currentRaceVal--;
			LoadHeroDat(currentRaceVal, currentStyleVal);
			drawFlag = 4;
		} else {
			currentStyleVal = 0; // reset style
			drawFlag = 1;
		}
		break;
	case 0x512: // style left button pushed
		if (currentStyleVal == 0) {
			drawFlag = 4;
			break;
		}
		currentStyleVal--;
		if (LoadHeroDat(currentRaceVal, currentStyleVal, true)) {
			currentStyleVal = 0;
			LoadHeroDat(currentRaceVal, currentStyleVal, true);
		}
		drawFlag = 0;
		break;
	case 0x514: // style right button pushed
		currentStyleVal++;

		if (LoadHeroDat(currentRaceVal, currentStyleVal, true) != 0) {
			currentStyleVal--;
			LoadHeroDat(currentRaceVal, currentStyleVal);
			drawFlag = 4;
		} else {
			drawFlag = 0;
		}
		break;
	}

	if (drawFlag != -1) {
		bool style = false; // Race;
		switch (drawFlag) {
		case 0:
			*(DWORD*)_info_line = 0x502;
			style = true;
			goto play;
		case 1:
			*(DWORD*)_info_line = 0x501;
		play:
			PlayAcm("ib3p1xx1");
			break;
		case 2:
			style = true;
		case 3:
			PlayAcm("ISDXXXX1");
			break;
		default:
			PlayAcm("IB3LU1X1");
			return button;
		}
		FixTextHighLight();
		DrawCharNoteNewChar(style);
	}
	DrawPCConsole();

	return button;
}

static void __declspec(naked) CheckCharScrnButtons() {
	__asm {
		call CheckCharButtons;
		cmp  eax, 0x500;
		jl   endFunc;
		cmp  eax, 0x515;
		jg   endFunc;
		add  esp, 4;   // ditch old ret addr
		push 0x431E8A; // recheck buttons if app mod button
endFunc:
		retn;
	}
}

static void __fastcall HeroGenderChange(long gender) {
	// get PC stat current gender
	long newGender = StatLevel(*ptr_obj_dude, STAT_gender);
	if (newGender == gender) return;      // check if gender has been changed

	long baseModel = (newGender)          // check if male 0
		? *(DWORD*)0x5108AC               // base female model
		: *(DWORD*)_art_vault_person_nums; // base male model

	// adjust base hero art
	baseModel += critterListSize;
	*(DWORD*)_art_vault_guy_num = baseModel;

	// reset race and style to defaults
	currentRaceVal = 0;
	currentStyleVal = 0;
	LoadHeroDat(0, 0);

	RefreshHeroBaseArt();

	// Check If Race or Style selected to redraw info note
	int infoLine = *(DWORD*)_info_line;
	if (infoLine == 0x501 || infoLine == 0x502) {
		DrawCharNoteNewChar(infoLine != 0x501);
	}
}

static void __declspec(naked) SexScrnEnd() {
	__asm {
		push edx;
		mov  edx, STAT_gender;
		mov  eax, dword ptr ds:[_obj_dude];
		call stat_level_;               // get PC stat current gender
		mov  ecx, eax;                  // gender
		call SexWindow_;                // call gender selection window
/*
		xor  ebx, ebx;
		cmp byte ptr ds:[_gmovie_played_list + 0x3], 1 // check if wearing vault suit
		jne NoVaultSuit
		mov ebx, 0x8
NoVaultSuit:
		mov  eax, dword ptr ds:[ebx + _art_vault_person_nums]; // base male model
*/
		call HeroGenderChange;
		pop  edx;
		retn;
	}
}

// Create race and style selection buttons when creating a character
static void __declspec(naked) AddCharScrnButtons() {
	__asm {
		pushad; // prolog
		mov  ebp, esp;
		sub  esp, __LOCAL_SIZE;
	}

	int WinRef;
	WinRef = *(DWORD*)_edit_win; // char screen window ref

	// race and style title buttons
	WinRegisterButton(WinRef, 332, 0, 82, 32, -1, -1, 0x501, -1, 0, 0, 0, 0);
	WinRegisterButton(WinRef, 332, 226, 82, 32, -1, -1, 0x502, -1, 0, 0, 0, 0);

	if (*(DWORD*)_glblmode == 1) { // equals 1 if new char screen - equals 0 if ingame char screen
		if (newButtonSurface == nullptr) {
			newButtonSurface = new BYTE [20 * 18 * 4];

			DWORD frmLock; // frm objects for char screen Appearance button
			BYTE *frmSurface;

			frmSurface = GetFrmSurface(BuildFrmId(6, 122), 0, 0, &frmLock); //SLUFrm
			sub_draw(20, 18, 20, 18, 0, 0, frmSurface, 20, 18 * 4, 0, 0, newButtonSurface, 0);
			UnloadFrm(frmLock);

			frmSurface = GetFrmSurface(BuildFrmId(6, 123), 0, 0, &frmLock); //SLDFrm
			sub_draw(20, 18, 20, 18, 0, 0, frmSurface, 20, 18 * 4, 0, 18, newButtonSurface, 0);
			UnloadFrm(frmLock);

			frmSurface = GetFrmSurface(BuildFrmId(6, 124), 0, 0, &frmLock); //SRUFrm
			sub_draw(20, 18, 20, 18, 0, 0, frmSurface, 20, 18 * 4, 0, 18 * 2, newButtonSurface, 0);
			UnloadFrm(frmLock);

			frmSurface = GetFrmSurface(BuildFrmId(6, 125), 0, 0, &frmLock); //SRDFrm
			sub_draw(20, 18, 20, 18, 0, 0, frmSurface, 20, 18 * 4, 0, 18 * 3, newButtonSurface, 0);
			UnloadFrm(frmLock);

			frmSurface = nullptr;
		}

		// check if Data exists for other races male or female, and if so enable race selection buttons
		if (GetFileAttributes("Appearance\\hmR01S00") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR01S00") != INVALID_FILE_ATTRIBUTES ||
			GetFileAttributes("Appearance\\hmR01S00.dat") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR01S00.dat") != INVALID_FILE_ATTRIBUTES) {
			// race selection buttons
			WinRegisterButton(WinRef, 348, 37, 20, 18, -1, -1, -1, 0x511, newButtonSurface, newButtonSurface + (20 * 18), 0, 0x20);
			WinRegisterButton(WinRef, 373, 37, 20, 18, -1, -1, -1, 0x513, newButtonSurface + (20 * 18 * 2), newButtonSurface + (20 * 18 * 3), 0, 0x20);
		}
		// check if Data exists for other styles male or female, and if so enable style selection buttons
		if (GetFileAttributes("Appearance\\hmR00S01") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR00S01") != INVALID_FILE_ATTRIBUTES ||
			GetFileAttributes("Appearance\\hmR00S01.dat") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR00S01.dat") != INVALID_FILE_ATTRIBUTES) {
			// style selection buttons
			WinRegisterButton(WinRef, 348, 199, 20, 18, -1, -1, -1, 0x512, newButtonSurface, newButtonSurface + (20 * 18), 0, 0x20);
			WinRegisterButton(WinRef, 373, 199, 20, 18, -1, -1, -1, 0x514, newButtonSurface + (20 * 18 * 2), newButtonSurface + (20 * 18 * 3), 0, 0x20);
		}
	}

	__asm {
		mov  esp, ebp; // epilog
		popad;
		// move tag skills button to fit Appearance interface
		mov  edx, 396 + 30; // tag/skills button xpos offset
		retn;
	}
}

// Loading or creating a background image for the character creation/editing interface
static void __declspec(naked) FixCharScrnBack() {
	__asm {
		mov  dword ptr ds:[_bckgnd], eax; // surface ptr for char scrn back
		test eax, eax;                    // check if frm loaded ok
		je   endFunc;
		// prolog
		pushad;
		mov  ebp, esp;
		sub  esp, __LOCAL_SIZE;
	}

	if (charScrnBackSurface == nullptr) {
		charScrnBackSurface = new BYTE [640 * 480];

		UNLSTDfrm *frm = LoadUnlistedFrm((*(long*)_glblmode) ? "AppChCrt.frm" : "AppChEdt.frm", 6);

		if (frm != nullptr) {
			sub_draw(640, 480, 640, 480, 0, 0, frm->frames[0].indexBuff, 640, 480, 0, 0, charScrnBackSurface, 0);
			delete frm;
		} else {
			BYTE *OldCharScrnBackSurface = *(BYTE**)_bckgnd; // char screen background frm surface

			// copy old charscrn surface to new
			sub_draw(640, 480, 640, 480, 0, 0, OldCharScrnBackSurface, 640, 480, 0, 0, charScrnBackSurface, 0);

			// copy Tag Skill Counter background to the right
			sub_draw(38, 26, 640, 480, 519, 228, OldCharScrnBackSurface, 640, 480, 519 + 36, 228, charScrnBackSurface, 0);

			// copy a blank part of the Tag Skill Bar hiding the old counter
			sub_draw(38, 26, 640, 480, 460, 228, OldCharScrnBackSurface, 640, 480, 519, 228, charScrnBackSurface, 0);

			sub_draw(36, 258, 640, 480, 332, 0, OldCharScrnBackSurface, 640, 480, 408, 0, charScrnBackSurface, 0); // shift behind button rail
			sub_draw(6, 32, 640, 480, 331, 233, OldCharScrnBackSurface, 640, 480, 330, 6, charScrnBackSurface, 0); // shadow for style/race button

			DWORD FrmObj, FrmMaskObj; // frm objects for char screen Appearance button
			BYTE *FrmSurface, *FrmMaskSurface;

			FrmSurface = GetFrmSurface(BuildFrmId(6, 113), 0, 0, &FrmObj);
			sub_draw(81, 132, 292, 376, 163, 20, FrmSurface, 640, 480, 331, 63, charScrnBackSurface, 0);  // char view win
			sub_draw(79, 31, 292, 376, 154, 228, FrmSurface, 640, 480, 331, 32, charScrnBackSurface, 0);  // upper  char view win
			sub_draw(79, 30, 292, 376, 158, 236, FrmSurface, 640, 480, 331, 195, charScrnBackSurface, 0); // lower  char view win
			UnloadFrm(FrmObj);

			// Sexoff Frm
			FrmSurface = GetFrmSurface(BuildFrmId(6, 188), 0, 0, &FrmObj);
			// Sex button mask frm
			FrmMaskSurface = GetFrmSurface(BuildFrmId(6, 187), 0, 0, &FrmMaskObj);

			sub_draw(80, 28, 80, 32, 0, 0, FrmMaskSurface, 80, 32, 0, 0, FrmSurface, 0x39); // mask for style and race buttons
			UnloadFrm(FrmMaskObj);
			FrmMaskSurface = nullptr;

			FrmSurface[80 * 32 - 1] = 0;
			FrmSurface[80 * 31 - 1] = 0;
			FrmSurface[80 * 30 - 1] = 0;

			FrmSurface[80 * 32 - 2] = 0;
			FrmSurface[80 * 31 - 2] = 0;
			FrmSurface[80 * 30 - 2] = 0;

			FrmSurface[80 * 32 - 3] = 0;
			FrmSurface[80 * 31 - 3] = 0;
			FrmSurface[80 * 30 - 3] = 0;

			FrmSurface[80 * 32 - 4] = 0;
			FrmSurface[80 * 31 - 4] = 0;
			FrmSurface[80 * 30 - 4] = 0;

			sub_draw(80, 32, 80, 32, 0, 0, FrmSurface, 640, 480, 332, 0, charScrnBackSurface, 0);   // style and race buttons
			sub_draw(80, 32, 80, 32, 0, 0, FrmSurface, 640, 480, 332, 225, charScrnBackSurface, 0); // style and race buttons
			UnloadFrm(FrmObj);

			// frm background for char screen Appearance button
			FrmSurface = GetFrmSurface(BuildFrmId(6, 174), 0, 0, &FrmObj);                                   // Pickchar frm
			sub_draw(69, 20, 640, 480, 282, 320, FrmSurface, 640, 480, 337, 37, charScrnBackSurface, 0);  // button backround top
			sub_draw(69, 20, 640, 480, 282, 320, FrmSurface, 640, 480, 337, 199, charScrnBackSurface, 0); // button backround bottom
			sub_draw(47, 16, 640, 480, 94, 394, FrmSurface, 640, 480, 347, 39, charScrnBackSurface, 0);   // cover buttons pics top
			sub_draw(47, 16, 640, 480, 94, 394, FrmSurface, 640, 480, 347, 201, charScrnBackSurface, 0);  // cover buttons pics bottom
			UnloadFrm(FrmObj);
			FrmSurface = nullptr;
		}

		int oldFont = GetFont();
		SetFont(0x67);

		char RaceText[8], StyleText[8];
		// Get alternate text from ini if available
		GetPrivateProfileString("AppearanceMod", "RaceText", "Race", RaceText, 8, translationIni);
		GetPrivateProfileString("AppearanceMod", "StyleText", "Style", StyleText, 8, translationIni);

		DWORD raceTextWidth = GetTextWidth(RaceText);
		DWORD styleTextWidth = GetTextWidth(StyleText);

		BYTE PeanutButter = *(BYTE*)_PeanutButter; // palette offset stored in mem

		PrintText(RaceText, PeanutButter, 372 - raceTextWidth / 2, 6, raceTextWidth, 640, charScrnBackSurface);
		PrintText(StyleText, PeanutButter, 372 - styleTextWidth / 2, 231, styleTextWidth, 640, charScrnBackSurface);
		SetFont(oldFont);
	}

	*(BYTE**)_bckgnd = charScrnBackSurface; // surface ptr for char scrn back

	__asm {
		mov esp, ebp; // epilog
		popad;
endFunc:
		retn;
	}
}

static void DeleteCharSurfaces() {
	delete[] newButtonSurface;
	newButtonSurface = nullptr;

	delete[] charScrnBackSurface;
	charScrnBackSurface = nullptr;
}

static void __declspec(naked) CharScrnEnd() {
	__asm {
		push eax;
		call DeleteCharSurfaces;
		pop  eax;
		mov  ebp, dword ptr ds:[_info_line];
		retn;
	}
}

//////////////////////////////////////////////////////////////////////FIX FUNCTIONS////////////////////////////////////////////////////////////////////////

// Adjust PC SFX acm name. Skip Underscore char at the start of PC App Name
static void __declspec(naked) FixPcSFX() {
	__asm {
		cmp byte ptr ds:[ebx], 0x5F; // check if Name begins with an '_' character
		jne endFunc;
		inc ebx;                     // shift address to next char
endFunc:
		// restore original code
		mov eax, ebx;
		cmp dword ptr ds:[_gsound_initialized], 0;
		retn;
	}
}

/*
// Set path to normal before printing or saving character details
static void __declspec(naked) FixCharScrnSaveNPrint() {
	__asm {
		push TempPathPtr //store current path
		mov  eax, _paths
		mov  TempPathPtr, eax //set path to normal
		push esi
		call OptionWindow_ //call char-scrn menu function
		pop  esi
		pop  TempPathPtr //restore stored path

		call RefreshPCArt
		ret
	}
}
*/

static void __declspec(naked) FixPcCriticalHitMsg() {
	__asm {
		cmp eax, critterListSize; // check if critter art in PC range
		jle endFunc;
		sub eax, critterListSize; // shift critter art index down out of hero range
endFunc:
		jmp art_alias_num_;
	}
}

static const DWORD op_obj_art_fid_Ret = 0x45C5D9;
static void __declspec(naked) op_obj_art_fid_hack() {
	__asm {
		mov  esi, [edi + 0x20]; // artFid
		push ecx;
		call RealDudeObject;
		pop  ecx;
		cmp  eax, edi; // object is dude?
		jnz  skip;
		sub  esi, critterListSize; // fix hero FrmID
skip:
		jmp  op_obj_art_fid_Ret;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Load Appearance data from GCD file
static void __fastcall LoadGCDAppearance(void *fileStream) {
	currentRaceVal = 0;
	currentStyleVal = 0;
	DWORD temp;
	if (FReadDword(fileStream, &temp) != -1 && temp < 100) {
		currentRaceVal = (int)temp;
		if (FReadDword(fileStream, &temp) != -1 && temp < 100) {
			currentStyleVal = (int)temp;
		}
	}
	FCloseFile(fileStream);

	// load hero appearance
	if (LoadHeroDat(currentRaceVal, currentStyleVal, true) != 0) { // if load fails
		currentStyleVal = 0;                                       // set style to default
		if (LoadHeroDat(currentRaceVal, currentStyleVal) != 0) {   // if race fails with style at default
			currentRaceVal = 0;                                    // set race to default
			LoadHeroDat(currentRaceVal, currentStyleVal);
		}
	}
	RefreshHeroBaseArt();
}

// Save Appearance data to GCD file
static void __fastcall SaveGCDAppearance(void *FileStream) {
	if (FWriteDword(FileStream, (DWORD)currentRaceVal) != -1) {
		FWriteDword(FileStream, (DWORD)currentStyleVal);
	}
	FCloseFile(FileStream);
}

static void EnableHeroAppearanceMod() {
	appModEnabled = true;

	// setup paths
	heroPathPtr = new sPath;
	racePathPtr = new sPath;
	heroPathPtr->path = new char[64];
	racePathPtr->path = new char[64];

	heroPathPtr->isDat = 0;
	racePathPtr->isDat = 0;
	heroPathPtr->pDat = nullptr;
	racePathPtr->pDat = nullptr;

	// Check if new Appearance char scrn button pushed (editor_design_)
	HookCall(0x431E9D, CheckCharScrnButtons);

	// Destroy new Appearance button mem after use (editor_design_)
	MakeCall(0x4329D8, CharScrnEnd, 1);

	// Check if sex has changed and reset char appearance (editor_design_)
	HookCall(0x4322E8, SexScrnEnd);

	// Load New Hero Art (xfopen_)
	MakeCall(0x4DEEE5, LoadNewHeroArt, 1);

	// Divert critter frm file name function exit for file checking (art_get_name_)
	SafeWrite8(0x419520, 0xEB); // divert func exit
	SafeWrite32(0x419521, 0x9090903E);

	// Check if new hero art exists otherwise use regular art (art_get_name_)
	MakeCall(0x419560, CheckHeroExist);

	// Double size of critter art index creating a new area for hero art (art_read_lst_)
	HookCall(0x4196B0, DoubleArt);

	// Add new hero critter names at end of critter list (art_init_)
	MakeCall(0x418B39, AddHeroCritNames);

	// Shift base hero critter art offset up into hero section (proto_dude_update_gender_)
	MakeCall(0x49F9DA, AdjustHeroBaseArt);

	// Adjust hero art index offset when changing armor (adjust_fid_)
	HookCall(0x4717D1, AdjustHeroArmorArt);

	// Hijack Save Hero State Structure fuction address 9CD54800
	// Return hero art index offset back to normal before saving
	SafeWrite32(0x519400, (DWORD)&SavCritNumFix);

	// Add new Appearance mod buttons (RegInfoAreas_)
	MakeCall(0x43A788, AddCharScrnButtons);

	// Mod char scrn background and add new app mod graphics. also adjust tag/skill button x pos (CharEditStart_)
	MakeCall(0x432B92, FixCharScrnBack);

	// Tag Skills text x pos
	SafeWrite32(0x433372, 0x24826 + 36);  // Tag Skills text x pos1
	SafeWrite32(0x4362BE, 0x24826 + 36);  // Tag Skills text x pos2
	SafeWrite32(0x4362F2, 522 + 36);      // Tag Skills num counter2 x pos1
	SafeWrite32(0x43631E, 522 + 36);      // Tag Skills num counter2 x pos2

	// skill points
	SafeWrite32(0x436262, 0x24810 + 36);  // Skill Points text x pos
	SafeWrite32(0x43628A, 522 + 36);      // Skill Points num counter x pos1
	SafeWrite32(0x43B5B2, 522 + 36);      // Skill Points num counter x pos2

	// make room for char view window
	SafeWrite32(0x433678, 347 + 76);      // shift skill buttons right 80
	SafeWrite32(0x4363CE, 380 + 68);      // shift skill name text right 80
	SafeWrite32(0x43641C, 573 + 10);      // shift skill % num text right 80
	SafeWrite32(0x43A74C, 223 - 76 + 10); // skill list mouse area button width
	SafeWrite32(0x43A75B, 370 + 76);      // skill list mouse area button xpos
	SafeWrite32(0x436220, 3580 + 68);     // skill text xpos
	SafeWrite32(0x43A71E, 223 - 68);      // skill button width
	SafeWrite32(0x43A72A, 376 + 68);      // skill button xpos

	// redraw area for skill list
	SafeWrite32(0x4361C4, 370 + 76); // xpos
	SafeWrite32(0x4361D9, 270 - 76); // width
	SafeWrite32(0x4361DE, 370 + 76); // xpos

	// skill slider thingy
	SafeWrite32(0x43647C, 592 + 3);  // xpos
	SafeWrite32(0x4364FA, 614 + 3);  // plus button xpos
	SafeWrite32(0x436567, 614 + 3);  // minus button xpos

	// fix for Char Screen note position was x484 y309 now x383 y308
	//SafeWrite32(0x43AB55, 308 * 640 + 483); // minus button xpos

	// Adjust PC SFX Name (gsound_load_sound_)
	MakeCall(0x4510EB, FixPcSFX, 2);

	// Set path to normal before printing or saving character details
	//HookCall(0x432359, FixCharScrnSaveNPrint);

	// Load Appearance data from GCD file (pc_load_data_)
	SafeWrite8(0x42DF5E, 0xF1); // mov ecx, esi "*FileStream"
	HookCall(0x42DF5F, LoadGCDAppearance);

	// Save Appearance data to GCD file (pc_save_data_)
	SafeWrite8(0x42E162, 0xF1); // mov ecx, esi "*FileStream"
	HookCall(0x42E163, SaveGCDAppearance);

	// Reset Appearance when selecting "Create Character" from the New Char screen (select_character_)
	MakeCall(0x4A7405, CreateCharReset);

	// Fixes missing console critical hit messages when PC is attacked. (combat_get_loc_name_)
	HookCall(0x42613A, FixPcCriticalHitMsg);

	// Force Criticals For Testing
	//SafeWrite32(0x423A8F, 0x90909090);
	//SafeWrite32(0x423A93, 0x90909090);
}

void HeroAppearanceModExit() {
	if (!appModEnabled) return;

	if (heroPathPtr) {
		delete[] heroPathPtr->path;
		delete heroPathPtr;
	}
	if (racePathPtr) {
		delete[] racePathPtr->path;
		delete racePathPtr;
	}
}

void HeroAppearanceModInit() {
	int heroAppearanceMod = GetPrivateProfileIntA("Misc", "EnableHeroAppearanceMod", 0, ini);
	if (heroAppearanceMod > 0) {
		dlog("Setting up Appearance Char Screen buttons.", DL_INIT);
		EnableHeroAppearanceMod();
		// Hero FrmID fix for obj_art_fid script function
		if (heroAppearanceMod != 2) MakeJump(0x45C5C3, op_obj_art_fid_hack);
		dlogr(" Done", DL_INIT);
	}
}
