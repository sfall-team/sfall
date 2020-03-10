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
#include "FalloutEngine.h"
#include "LoadGameHook.h"
#include "Message.h"
#include "PartyControl.h"
#include "ScriptExtender.h"

#include "HeroAppearance.h"

bool appModEnabled = false; // check if Appearance mod enabled for script fuctions

const char* appearancePathFmt = "Appearance\\h%cR%02dS%02d%s";

// char scrn surfaces
BYTE *newButtonSurface = nullptr;
BYTE *charScrnBackSurface = nullptr;

// char scrn critter rotation vars
DWORD charRotTick = 0;
DWORD charRotOri = 0;

bool raceButtons = false, styleButtons = false;
int currentRaceVal = 0, currentStyleVal = 0;     // holds Appearance values to restore after global reset in NewGame2 function in LoadGameHooks.cpp
DWORD critterListSize = 0, critterArraySize = 0; // Critter art list size

PathNode **heroAppPaths = ptr_paths;
// index: 0 - only folder (w/o extension .dat), 1 - file or folder .dat
PathNode *heroPathPtr[2] = {nullptr, nullptr};
PathNode *racePathPtr[2] = {nullptr, nullptr};

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
#pragma pack(push, 1)
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
#pragma pack(pop)

/////////////////////////////////////////////////////////////////UNLISTED FRM FUNCTIONS//////////////////////////////////////////////////////////////

static bool LoadFrmHeader(UNLSTDfrm *frmHeader, DbFile* frmStream) {
	if (DbFReadInt(frmStream, &frmHeader->version) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frmHeader->FPS) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frmHeader->actionFrame) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frmHeader->numFrames) == -1)
		return false;
	else if (DbFReadShortCount(frmStream, frmHeader->xCentreShift, 6) == -1)
		return false;
	else if (DbFReadShortCount(frmStream, frmHeader->yCentreShift, 6) == -1)
		return false;
	else if (DbFReadIntCount(frmStream, frmHeader->oriOffset, 6) == -1)
		return false;
	else if (DbFReadInt(frmStream, &frmHeader->frameAreaSize) == -1)
		return false;

	return true;
}

static bool LoadFrmFrame(UNLSTDframe *frame, DbFile* frmStream) {
	//FRMframe *frameHeader = (FRMframe*)frameMEM;
	//BYTE* frameBuff = frame + sizeof(FRMframe);

	if (DbFReadShort(frmStream, &frame->width) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frame->height) == -1)
		return false;
	else if (DbFReadInt(frmStream, &frame->size) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frame->x) == -1)
		return false;
	else if (DbFReadShort(frmStream, &frame->y) == -1)
		return false;

	frame->indexBuff = new BYTE[frame->size];
	if (DbFRead(frame->indexBuff, frame->size, 1, frmStream) != 1)
		return false;

	return true;
}

UNLSTDfrm *LoadUnlistedFrm(char *frmName, unsigned int folderRef) {
	if (folderRef > OBJ_TYPE_SKILLDEX) return nullptr;

	char *artfolder = ptr_art[folderRef].path; // address of art type name
	char FrmPath[MAX_PATH];

	sprintf_s(FrmPath, MAX_PATH, "art\\%s\\%s", artfolder, frmName);

	UNLSTDfrm *frm = new UNLSTDfrm;

	DbFile* frmStream = XFOpen(FrmPath, "rb");

	if (frmStream != nullptr) {
		if (!LoadFrmHeader(frm, frmStream)) {
			DbFClose(frmStream);
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
						DbFClose(frmStream);
						delete frm;
						return nullptr;
					}
				}
				oriOffset_new += frm->numFrames;
			} else {
				frm->oriOffset[ori] = 0;
			}
		}

		DbFClose(frmStream);
	} else {
		delete frm;
		return nullptr;
	}
	return frm;
}

/////////////////////////////////////////////////////////////////TEXT FUNCTIONS//////////////////////////////////////////////////////////////////////

static void SetFont(long ref) {
	TextFont(ref);
}

static long GetFont() {
	return *ptr_curr_font_num;
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

/////////////////////////////////////////////////////////////////DAT FUNCTIONS///////////////////////////////////////////////////////////////////////

static void* LoadDat(const char* fileName) {
	return DbaseOpen(fileName);
}

static void UnloadDat(void* dat) {
	DbaseClose(dat);
}

/////////////////////////////////////////////////////////////////OTHER FUNCTIONS/////////////////////////////////////////////////////////////////////

static DWORD BuildFrmId(DWORD lstRef, DWORD lstNum) {
	return (lstRef << 24) | lstNum;
}

/////////////////////////////////////////////////////////////////APP MOD FUNCTIONS///////////////////////////////////////////////////////////////////

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

// functions to load and save appearance globals
static void SetAppearanceGlobals(int race, int style) {
	SetGlobalVar("HAp_Race", race);
	SetGlobalVar("HApStyle", style);
}

static void GetAppearanceGlobals(int *race, int *style) {
	*race = GetGlobalVar("HAp_Race");
	*style = GetGlobalVar("HApStyle");
}

static __declspec(noinline) int _stdcall LoadHeroDat(unsigned int race, unsigned int style, bool flush = false) {
	if (flush) ArtFlush();

	if (heroPathPtr[1]->pDat) { // unload previous Dats
		UnloadDat(heroPathPtr[1]->pDat);
		heroPathPtr[1]->pDat = nullptr;
		heroPathPtr[1]->isDat = 0;
	}
	if (racePathPtr[1]->pDat) {
		UnloadDat(racePathPtr[1]->pDat);
		racePathPtr[1]->pDat = nullptr;
		racePathPtr[1]->isDat = 0;
	}

	const char sex = GetSex();
	bool folderIsExist = false, heroDatIsExist = false;
	// check if folder exists for selected appearance
	sprintf_s(heroPathPtr[0]->path, 64, appearancePathFmt, sex, race, style, "");
	if (GetFileAttributes(heroPathPtr[0]->path) != INVALID_FILE_ATTRIBUTES) {
		folderIsExist = true;
	}
	// check if Dat exists for selected appearance
	sprintf_s(heroPathPtr[1]->path, 64, appearancePathFmt, sex, race, style, ".dat");
	int result = GetFileAttributes(heroPathPtr[1]->path);
	if (result != INVALID_FILE_ATTRIBUTES) {
		if (!(result & FILE_ATTRIBUTE_DIRECTORY)) {
			heroPathPtr[1]->pDat = LoadDat(heroPathPtr[1]->path);
			heroPathPtr[1]->isDat = 1;
		}
		if (folderIsExist) heroPathPtr[0]->next = heroPathPtr[1];
		heroDatIsExist = true;
	} else if (!folderIsExist) {
		return -1; // no .dat files and folder
	}

	heroAppPaths = &heroPathPtr[1 - folderIsExist]; // set path for selected appearance
	heroPathPtr[0 + heroDatIsExist]->next = ptr_paths[0]; // heroPathPtr[] >> foPaths

	if (style != 0) {
		bool raceDatIsExist = false, folderIsExist = false;
		// check if folder exists for selected race base appearance
		sprintf_s(racePathPtr[0]->path, 64, appearancePathFmt, sex, race, 0, "");
		if (GetFileAttributes(racePathPtr[0]->path) != INVALID_FILE_ATTRIBUTES) {
			folderIsExist = true;
		}
		// check if Dat (or folder) exists for selected race base appearance
		sprintf_s(racePathPtr[1]->path, 64, appearancePathFmt, sex, race, 0, ".dat");
		int result = GetFileAttributes(racePathPtr[1]->path);
		if (result != INVALID_FILE_ATTRIBUTES) {
			if (!(result & FILE_ATTRIBUTE_DIRECTORY)) {
				racePathPtr[1]->pDat = LoadDat(racePathPtr[1]->path);
				racePathPtr[1]->isDat = 1;
			}
			if (folderIsExist) racePathPtr[0]->next = racePathPtr[1];
			raceDatIsExist = true;
		} else if (!folderIsExist) {
			return 0;
		}

		heroPathPtr[0 + heroDatIsExist]->next = racePathPtr[1 - folderIsExist]; // set path for selected race base appearance
		racePathPtr[0 + raceDatIsExist]->next = ptr_paths[0]; // insert racePathPtr in chain path: heroPathPtr[] >> racePathPtr[] >> foPaths
	}
	return 0;
}

// insert hero art path in front of main path structure when loading art
static void __declspec(naked) LoadNewHeroArt() {
	__asm {
		cmp byte ptr ds:[esi], 'r';
		jne isNotReading;
		mov ecx, heroAppPaths;
		mov ecx, dword ptr ds:[ecx]; // set app path
		retn;
isNotReading:
		mov ecx, _paths;
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
	if (critterListSize > 2048) {
		MessageBoxA(0, "This mod cannot be used because the maximum limit of the FID count in the critters.lst is exceeded.\n"
					   "Please disable the mod and restart the game.", "Hero Appearance mod", MB_TASKMODAL | MB_ICONERROR);
		ExitProcess(-1);
	}
	critterArraySize = critterListSize * 13;

	char *CritList = (*(char**)0x51076C);         // critter list offset
	char *HeroList = CritList + critterArraySize; // set start of hero critter list after regular critter list

	memset(HeroList, 0, critterArraySize);

	for (DWORD i = 0; i < critterListSize; i++) { // copy critter name list to hero name list
		*HeroList = '_';                          // insert a '_' char at the front of new hero critt names. fallout wont load the same name twice
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

///////////////////////////////////////////////////////////////GRAPHICS HERO FUNCTIONS///////////////////////////////////////////////////////////////

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
	ArtFlush();
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

	if (LoadHeroDat(currentRaceVal, newStyleVal, true) != 0) { // if new style cannot be set
		if (currentRaceVal == 0 && newStyleVal == 0) {
			currentStyleVal = 0; // ignore error if appearance = default
		} else {
			LoadHeroDat(currentRaceVal, currentStyleVal); // reload original style
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

	if (LoadHeroDat(newRaceVal, 0, true) != 0) { // if new race fails with style at 0
		if (newRaceVal == 0) {
			currentRaceVal = 0;
			currentStyleVal = 0; // ignore if appearance = default
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

/////////////////////////////////////////////////////////////////INTERFACE FUNCTIONS/////////////////////////////////////////////////////////////////

static void surface_draw(long width, long height, long fromWidth, long fromX, long fromY, BYTE *fromBuff,
                         long toWidth, long toX, long toY, BYTE *toBuff, int maskRef)
{
	fromBuff += fromY * fromWidth + fromX;
	toBuff += toY * toWidth + toX;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) {
			if (fromBuff[w] != maskRef) toBuff[w] = fromBuff[w];
		}
		fromBuff += fromWidth;
		toBuff += toWidth;
	}
}

static void surface_draw(long width, long height, long fromWidth, long fromX, long fromY, BYTE *fromBuff,
                         long toWidth, long toX, long toY, BYTE *toBuff)
{
	fromBuff += fromY * fromWidth + fromX;
	toBuff += toY * toWidth + toX;

	for (long h = 0; h < height; h++) {
		for (long w = 0; w < width; w++) toBuff[w] = fromBuff[w];
		fromBuff += fromWidth;
		toBuff += toWidth;
	}
}

static void DrawBody(long critNum, BYTE* surface, long x, long y, long toWidth) {
	DWORD critFrmLock;

	FrmHeaderData *critFrm = ArtPtrLock(BuildFrmId(1, critNum), &critFrmLock);
	DWORD critWidth = ArtFrameWidth(critFrm, 0, charRotOri);
	DWORD critHeight = ArtFrameLength(critFrm, 0, charRotOri);
	BYTE* critSurface = ArtFrameData(critFrm, 0, charRotOri);

	long xOffset = x + (35 - (critWidth / 2));
	long yOffset = y + (51 - (critHeight / 2));
	surface_draw(critWidth, critHeight, critWidth, 0, 0, critSurface, toWidth, xOffset, yOffset, surface, 0);

	ArtPtrUnlock(critFrmLock);
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

		int WinRef = *ptr_edit_win; // char screen window ref
		WINinfo *WinInfo = GNWFind(WinRef);

		//DWORD critNum = *ptr_art_vault_guy_num; // pointer to current base hero critter FrmId
		DWORD critNum = *(DWORD*)(*(DWORD*)_obj_dude + 0x20); // pointer to current armored hero critter FrmId

		surface_draw(70, 102, 640, 338, 78, charScrnBackSurface, WinInfo->width, 338, 78, WinInfo->surface); // restore background image
		DrawBody(critNum, WinInfo->surface, 338, 78, WinInfo->width);

		WinDraw(WinRef);
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

	if (MessageLoad(&MsgList, MsgFileName) == 1) {
		TitleMsg = GetMsg(&MsgList, 100, 2);
		InfoMsg = GetMsg(&MsgList, 101, 2);
	}

	WINinfo *winInfo = GNWFind(winRef);

	BYTE *PadSurface = new BYTE [280 * 168];
	surface_draw(280, 168, widthBG, xPosBG, yPosBG, BGSurface, 280, 0, 0, PadSurface);

	UNLSTDfrm *frm = LoadUnlistedFrm((style) ? "AppStyle.frm" : "AppRace.frm", OBJ_TYPE_SKILLDEX);
	if (frm) {
		DrawToSurface(frm->frames[0].width, frm->frames[0].height, 0, 0, frm->frames[0].width, frm->frames[0].indexBuff, 136, 37, 280, 168, PadSurface, 0); // cover buttons pics bottom
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
	surface_draw(280, 168, 280, 0, 0, PadSurface, winInfo->width, xPosWin, yPosWin, winInfo->surface);

	SetFont(oldFont); // restore previous font
	MessageExit(&MsgList);

	*(long*)_card_old_fid1 = -1; // reset fid

	DeleteWordWrapList(StartLine);
	delete[] PadSurface;
}

static void _stdcall DrawCharNoteNewChar(bool type) {
	DrawCharNote(type, *ptr_edit_win, 348, 272, charScrnBackSurface, 348, 272, 640, 480);
}

// op_hero_select_win
void _stdcall HeroSelectWindow(int raceStyleFlag) {
	if (!appModEnabled) return;

	UNLSTDfrm *frm = LoadUnlistedFrm("AppHeroWin.frm", OBJ_TYPE_INTRFACE);
	if (frm == nullptr) {
		DebugPrintf("\nApperanceMod: art\\intrface\\AppHeroWin.frm file not found.");
		return;
	}

	bool isStyle = (raceStyleFlag != 0);
	DWORD resWidth = *(DWORD*)0x4CAD6B;
	DWORD resHeight = *(DWORD*)0x4CAD66;

	int winRef = WinAdd(resWidth / 2 - 242, (resHeight - 100) / 2 - 65, 484, 230, 100, 0x4);
	if (winRef == -1) {
		delete frm;
		return;
	}

	int mouseWasHidden = *ptr_mouse_is_hidden;
	if (mouseWasHidden) MouseShow();
	int oldMouse = *ptr_gmouse_current_cursor;
	GmouseSetCursor(1);

	BYTE *winSurface = WinGetBuf(winRef);
	BYTE *mainSurface = new BYTE [484 * 230];

	surface_draw(484, 230, 484, 0, 0, frm->frames[0].indexBuff, 484, 0, 0, mainSurface);
	delete frm;

	DWORD MenuUObj, MenuDObj;
	BYTE *MenuUSurface = ArtPtrLockData(BuildFrmId(6, 299), 0, 0, &MenuUObj); // MENUUP Frm
	BYTE *MenuDSurface = ArtPtrLockData(BuildFrmId(6, 300), 0, 0, &MenuDObj); // MENUDOWN Frm
	WinRegisterButton(winRef, 115, 181, 26, 26, -1, -1, -1, 0x0D, MenuUSurface, MenuDSurface, 0, 0x20);

	DWORD DidownUObj, DidownDObj;
	BYTE *DidownUSurface = ArtPtrLockData(BuildFrmId(6, 93), 0, 0, &DidownUObj); // MENUUP Frm
	BYTE *DidownDSurface = ArtPtrLockData(BuildFrmId(6, 94), 0, 0, &DidownDObj); // MENUDOWN Frm
	WinRegisterButton(winRef, 25, 84, 24, 25, -1, -1, -1, 0x150, DidownUSurface, DidownDSurface, 0, 0x20);

	DWORD DiupUObj, DiupDObj;
	BYTE *DiupUSurface = ArtPtrLockData(BuildFrmId(6, 100), 0, 0, &DiupUObj); // MENUUP Frm
	BYTE *DiupDSurface = ArtPtrLockData(BuildFrmId(6, 101), 0, 0, &DiupDObj); // MENUDOWN Frm
	WinRegisterButton(winRef, 25, 59, 23, 24, -1, -1, -1, 0x148, DiupUSurface, DiupDSurface, 0, 0x20);

	int oldFont = GetFont();
	SetFont(0x67);

	char titleText[16];
	// Get alternate text from ini if available
	if (isStyle) {
		Translate("AppearanceMod", "StyleText", "Style", titleText, 16);
	} else {
		Translate("AppearanceMod", "RaceText", "Race", titleText, 16);
	}

	BYTE textColour = *ptr_PeanutButter; // PeanutButter colour - palette offset stored in mem
	DWORD titleTextWidth = GetTextWidth(titleText);
	PrintText(titleText, textColour, 92 - titleTextWidth / 2, 10, titleTextWidth, 484, mainSurface);

	Translate("AppearanceMod", "DoneBtn", "Done", titleText, 16);
	titleTextWidth = GetTextWidth(titleText);
	PrintText(titleText, textColour, 80 - titleTextWidth / 2, 185, titleTextWidth, 484, mainSurface);

	surface_draw(484, 230, 484, 0, 0, mainSurface, 484, 0, 0, winSurface);
	WinShow(winRef);

	SetFont(0x65);

	int button = 0;
	bool drawFlag = true;               // redraw flag for char note pad

	DWORD RotSpeed = *(DWORD*)0x47066B; // get rotation speed - inventory rotation speed
	DWORD RedrawTick = 0, NewTick = 0, OldTick = 0;

	DWORD critNum = *ptr_art_vault_guy_num; // pointer to current base hero critter FrmID
	//DWORD critNum = *(DWORD*)(*(DWORD*)_obj_dude + 0x20); // pointer to current armored hero critter FrmID

	int raceVal = currentRaceVal, styleVal = currentStyleVal; // show default style when setting race
	if (!isStyle) styleVal = 0;
	LoadHeroDat(raceVal, styleVal, true);

	SetLoopFlag(HEROWIN);

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

			surface_draw(70, 102, 484, 66, 53, mainSurface, 484, 66, 53, winSurface); // restore background image
			DrawBody(critNum, winSurface, 66, 53, 484);

			if (drawFlag) {
				DrawCharNote(isStyle, winRef, 190, 29, mainSurface, 190, 29, 484, 230);
				drawFlag = false;
			}
			WinDraw(winRef);
		}

		button = GetInputBtn();
		if (button == 0x148) { // previous style/race - up arrow button pushed
			GsoundPlaySfxFile("ib1p1xx1");

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
			GsoundPlaySfxFile("ib1p1xx1");

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
			GsoundPlaySfxFile("ib1p1xx1");
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

	ClearLoopFlag(HEROWIN);

	LoadHeroDat(currentRaceVal, currentStyleVal, true);
	SetAppearanceGlobals(currentRaceVal, currentStyleVal);

	WinDelete(winRef);
	delete[] mainSurface;

	ArtPtrUnlock(MenuUObj);
	ArtPtrUnlock(MenuDObj);

	ArtPtrUnlock(DidownUObj);
	ArtPtrUnlock(DidownDObj);

	ArtPtrUnlock(DiupUObj);
	ArtPtrUnlock(DiupDObj);

	SetFont(oldFont);
	GmouseSetCursor(oldMouse);

	if (mouseWasHidden) MouseHide();
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
	int button = GetInputBtn();

	int drawFlag = -1;

	int infoLine = *ptr_info_line;
	if (infoLine == 0x503 || infoLine == 0x504) {
		*ptr_info_line -= 2;
		*(DWORD*)_frstc_draw1 = 1;
		DrawCharNoteNewChar(infoLine != 0x503);
	} else if (infoLine == 0x501 || infoLine == 0x502) {
		switch (button) {
		case 0x14B: // button left
		case 0x14D: // button right
			if (*ptr_glblmode == 1) { //if in char creation scrn
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
			*ptr_info_line += 2; // 0x503/0x504 for redrawing note when reentering char screen
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
			*ptr_info_line = 0x501;
			drawFlag = 3;
		}
		break;
	case 0x502: // style title button pushed
		if (infoLine != 0x502) {
			*ptr_info_line = 0x502;
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
			*ptr_info_line = 0x502;
			style = true;
			goto play;
		case 1:
			*ptr_info_line = 0x501;
		play:
			GsoundPlaySfxFile("ib3p1xx1");
			break;
		case 2:
			style = true;
		case 3:
			GsoundPlaySfxFile("ISDXXXX1");
			break;
		default:
			GsoundPlaySfxFile("IB3LU1X1");
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
		: *ptr_art_vault_person_nums;     // base male model

	// adjust base hero art
	baseModel += critterListSize;
	*ptr_art_vault_guy_num = baseModel;

	// reset race and style to defaults
	currentRaceVal = 0;
	currentStyleVal = 0;
	LoadHeroDat(0, 0);

	ProtoDudeUpdateGender();

	// Check If Race or Style selected to redraw info note
	int infoLine = *ptr_info_line;
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

// Create race and style selection buttons when creating a character (hero)
static void __declspec(naked) AddCharScrnButtons() {
	__asm {
		pushad; // prolog
		mov  ebp, esp;
		sub  esp, __LOCAL_SIZE;
	}

	int WinRef;
	WinRef = *ptr_edit_win; // char screen window ref

	// race and style title buttons
	WinRegisterButton(WinRef, 332,   0, 82, 32, -1, -1, 0x501, -1, 0, 0, 0, 0);
	WinRegisterButton(WinRef, 332, 226, 82, 32, -1, -1, 0x502, -1, 0, 0, 0, 0);

	if (*ptr_glblmode == 1 && (styleButtons || raceButtons)) { // equals 1 if new char screen - equals 0 if ingame char screen
		if (newButtonSurface == nullptr) {
			newButtonSurface = new BYTE [20 * 18 * 4];

			DWORD frmLock; // frm objects for char screen Appearance button
			BYTE* frmSurface;

			frmSurface = ArtPtrLockData(BuildFrmId(6, 122), 0, 0, &frmLock); //SLUFrm
			surface_draw(20, 18, 20, 0, 0, frmSurface, 20, 0, 0, newButtonSurface);
			ArtPtrUnlock(frmLock);

			frmSurface = ArtPtrLockData(BuildFrmId(6, 123), 0, 0, &frmLock); //SLDFrm
			surface_draw(20, 18, 20, 0, 0, frmSurface, 20, 0, 18, newButtonSurface);
			ArtPtrUnlock(frmLock);

			frmSurface = ArtPtrLockData(BuildFrmId(6, 124), 0, 0, &frmLock); //SRUFrm
			surface_draw(20, 18, 20, 0, 0, frmSurface, 20, 0, 18 * 2, newButtonSurface);
			ArtPtrUnlock(frmLock);

			frmSurface = ArtPtrLockData(BuildFrmId(6, 125), 0, 0, &frmLock); //SRDFrm
			surface_draw(20, 18, 20, 0, 0, frmSurface, 20, 0, 18 * 3, newButtonSurface);
			ArtPtrUnlock(frmLock);
		}
		if (raceButtons) { // race selection buttons
			WinRegisterButton(WinRef, 348, 37, 20, 18, -1, -1, -1, 0x511, newButtonSurface, newButtonSurface + (20 * 18), 0, 0x20);
			WinRegisterButton(WinRef, 374, 37, 20, 18, -1, -1, -1, 0x513, newButtonSurface + (20 * 18 * 2), newButtonSurface + (20 * 18 * 3), 0, 0x20);
		}
		if (styleButtons) { // style selection buttons
			WinRegisterButton(WinRef, 348, 199, 20, 18, -1, -1, -1, 0x512, newButtonSurface, newButtonSurface + (20 * 18), 0, 0x20);
			WinRegisterButton(WinRef, 374, 199, 20, 18, -1, -1, -1, 0x514, newButtonSurface + (20 * 18 * 2), newButtonSurface + (20 * 18 * 3), 0, 0x20);
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

		UNLSTDfrm *frm = LoadUnlistedFrm((*ptr_glblmode) ? "AppChCrt.frm" : "AppChEdt.frm", OBJ_TYPE_INTRFACE);

		if (frm != nullptr) {
			surface_draw(640, 480, 640, 0, 0, frm->frames[0].indexBuff, 640, 0, 0, charScrnBackSurface);
			delete frm;
		} else {
			BYTE* oldCharScrnBackSurface = *ptr_bckgnd; // char screen background frm surface

			// copy old charscrn surface to new
			surface_draw(640, 480, 640, 0, 0, oldCharScrnBackSurface, 640, 0, 0, charScrnBackSurface);

			// copy Tag Skill Counter background to the right
			surface_draw(38, 26, 640, 519, 228, oldCharScrnBackSurface, 640, 519 + 36, 228, charScrnBackSurface);

			// copy a blank part of the Tag Skill Bar hiding the old counter
			surface_draw(38, 26, 640, 460, 228, oldCharScrnBackSurface, 640, 519, 228, charScrnBackSurface);

			surface_draw(36, 258, 640, 332, 0, oldCharScrnBackSurface, 640, 408, 0, charScrnBackSurface); // shift behind button rail
			surface_draw(6, 32, 640, 331, 233, oldCharScrnBackSurface, 640, 330, 6, charScrnBackSurface); // shadow for style/race button

			DWORD FrmObj, FrmMaskObj; // frm objects for char screen Appearance button
			BYTE *FrmSurface, *FrmMaskSurface;

			FrmSurface = ArtPtrLockData(BuildFrmId(OBJ_TYPE_INTRFACE, 113), 0, 0, &FrmObj); // "Use Item On" window

			surface_draw(81, 132, 292, 163, 20, FrmSurface, 640, 331, 63, charScrnBackSurface);  // char view win
			surface_draw(79, 31, 292, 154, 228, FrmSurface, 640, 331, 32, charScrnBackSurface);  // upper  char view win
			surface_draw(79, 30, 292, 158, 236, FrmSurface, 640, 331, 195, charScrnBackSurface); // lower  char view win

			ArtPtrUnlock(FrmObj);

			// Sexoff Frm
			FrmSurface = ArtPtrLockData(BuildFrmId(OBJ_TYPE_INTRFACE, 188), 0, 0, &FrmObj);
			BYTE* newFrmSurface = new BYTE [80 * 32];
			surface_draw(80, 32, 80, 0, 0, FrmSurface, 80, 0, 0, newFrmSurface);
			ArtPtrUnlock(FrmObj);

			// Sex button mask frm
			FrmMaskSurface = ArtPtrLockData(BuildFrmId(OBJ_TYPE_INTRFACE, 187), 0, 0, &FrmMaskObj);
			// crop the Sexoff image by mask
			surface_draw(80, 28, 80, 0, 0, FrmMaskSurface, 80, 0, 0, newFrmSurface, 0x39); // mask for style and race buttons
			ArtPtrUnlock(FrmMaskObj);

			newFrmSurface[80 * 32 - 1] = 0;
			newFrmSurface[80 * 31 - 1] = 0;
			newFrmSurface[80 * 30 - 1] = 0;

			newFrmSurface[80 * 32 - 2] = 0;
			newFrmSurface[80 * 31 - 2] = 0;
			newFrmSurface[80 * 30 - 2] = 0;

			newFrmSurface[80 * 32 - 3] = 0;
			newFrmSurface[80 * 31 - 3] = 0;
			newFrmSurface[80 * 30 - 3] = 0;

			newFrmSurface[80 * 32 - 4] = 0;
			newFrmSurface[80 * 31 - 4] = 0;
			newFrmSurface[80 * 30 - 4] = 0;

			surface_draw(80, 32, 80, 0, 0, newFrmSurface, 640, 332,   0, charScrnBackSurface, 0); // race buttons
			surface_draw(80, 32, 80, 0, 0, newFrmSurface, 640, 332, 225, charScrnBackSurface, 0); // style buttons
			delete[] newFrmSurface;

			// frm background for char screen Appearance button
			if (*ptr_glblmode && (styleButtons || raceButtons)) {
				FrmSurface = ArtPtrLockData(BuildFrmId(OBJ_TYPE_INTRFACE, 174), 0, 0, &FrmObj); // Pickchar frm
				if (raceButtons)  surface_draw(69, 20, 640, 281, 319, FrmSurface, 640, 337,  36, charScrnBackSurface); // button backround top
				if (styleButtons) surface_draw(69, 20, 640, 281, 319, FrmSurface, 640, 337, 198, charScrnBackSurface); // button backround bottom
				ArtPtrUnlock(FrmObj);
			}
		}

		int oldFont = GetFont();
		SetFont(0x67);

		char RaceText[8], StyleText[8];
		// Get alternate text from ini if available
		Translate("AppearanceMod", "RaceText", "Race", RaceText, 8);
		Translate("AppearanceMod", "StyleText", "Style", StyleText, 8);

		DWORD raceTextWidth = GetTextWidth(RaceText);
		DWORD styleTextWidth = GetTextWidth(StyleText);

		BYTE PeanutButter = *ptr_PeanutButter; // palette offset stored in mem

		PrintText(RaceText, PeanutButter, 372 - raceTextWidth / 2, 6, raceTextWidth, 640, charScrnBackSurface);
		PrintText(StyleText, PeanutButter, 372 - styleTextWidth / 2, 231, styleTextWidth, 640, charScrnBackSurface);
		SetFont(oldFont);
	}

	*ptr_bckgnd = charScrnBackSurface; // surface ptr for char scrn back

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

//////////////////////////////////////////////////////////////////////FIX FUNCTIONS//////////////////////////////////////////////////////////////////

// Adjust PC SFX acm name. Skip Underscore char at the start of PC frm file name
static void __declspec(naked) FixPcSFX() {
	__asm {
		cmp byte ptr ds:[ebx], '_';  // check if Name begins with an 0x5F character
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
		mov  eax, [edi + 0x64]; // protoId
		cmp  eax, PID_Player;
		jne  skip;
		mov  eax, esi;
		and  eax, 0xFFF; // index in .LST
		cmp  eax, critterListSize;
		jle  skip;
		sub  esi, critterListSize; // fix hero FrmID
skip:
		jmp  op_obj_art_fid_Ret;
	}
}

static void __declspec(naked) op_metarule3_hook() {
	__asm {
		mov  edi, [esp + 0x4C - 0x44 + 8]; // source
		cmp  edi, ds:[_obj_dude];
		jne  skip;
		mov  edi, [edi + 0x64]; // protoId
		cmp  edi, PID_Player;
		jne  skip;
		mov  edi, edx; // edx = set fid number
		and  edi, 0xFFF;
		cmp  edi, critterListSize;
		jg   skip;
		add  edx, critterListSize;
skip:
		jmp  art_id_;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Load Appearance data from GCD file
static void __fastcall LoadGCDAppearance(DbFile* fileStream) {
	currentRaceVal = 0;
	currentStyleVal = 0;
	DWORD temp;
	if (DbFReadInt(fileStream, &temp) != -1 && temp < 100) {
		currentRaceVal = (int)temp;
		if (DbFReadInt(fileStream, &temp) != -1 && temp < 100) {
			currentStyleVal = (int)temp;
		}
	}
	DbFClose(fileStream);

	// load hero appearance
	if (LoadHeroDat(currentRaceVal, currentStyleVal, true) != 0) { // if load fails
		currentStyleVal = 0;                                       // set style to default
		if (LoadHeroDat(currentRaceVal, currentStyleVal) != 0) {   // if race fails with style at default
			currentRaceVal = 0;                                    // set race to default
			LoadHeroDat(currentRaceVal, currentStyleVal);
		}
	}
	ProtoDudeUpdateGender();
}

// Save Appearance data to GCD file
static void __fastcall SaveGCDAppearance(DbFile *FileStream) {
	if (DbFWriteInt(FileStream, (DWORD)currentRaceVal) != -1) {
		DbFWriteInt(FileStream, (DWORD)currentStyleVal);
	}
	DbFClose(FileStream);
}

static void EnableHeroAppearanceMod() {
	appModEnabled = true;

	// setup paths
	heroPathPtr[0] = new PathNode();
	racePathPtr[0] = new PathNode();
	heroPathPtr[0]->path = new char[64];
	racePathPtr[0]->path = new char[64];

	heroPathPtr[1] = new PathNode();
	racePathPtr[1] = new PathNode();
	heroPathPtr[1]->path = new char[64];
	racePathPtr[1]->path = new char[64];

	// check if Data exists for other races male or female, and if so enable race selection buttons
	if (GetFileAttributes("Appearance\\hmR01S00") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR01S00") != INVALID_FILE_ATTRIBUTES ||
		GetFileAttributes("Appearance\\hmR01S00.dat") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR01S00.dat") != INVALID_FILE_ATTRIBUTES) {
		raceButtons = true;
	}
	// check if Data exists for other styles male or female, and if so enable style selection buttons
	if (GetFileAttributes("Appearance\\hmR00S01") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR00S01") != INVALID_FILE_ATTRIBUTES ||
		GetFileAttributes("Appearance\\hmR00S01.dat") != INVALID_FILE_ATTRIBUTES || GetFileAttributes("Appearance\\hfR00S01.dat") != INVALID_FILE_ATTRIBUTES) {
		styleButtons = true;
	}

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

	delete[] heroPathPtr[0]->path;
	delete[] heroPathPtr[1]->path;
	delete heroPathPtr[0];
	delete heroPathPtr[1];

	delete[] racePathPtr[0]->path;
	delete[] racePathPtr[1]->path;
	delete racePathPtr[0];
	delete racePathPtr[1];
}

void HeroAppearanceModInit() {
	int heroAppearanceMod = GetConfigInt("Misc", "EnableHeroAppearanceMod", 0);
	if (heroAppearanceMod > 0) {
		dlog("Setting up Appearance Char Screen buttons.", DL_INIT);
		EnableHeroAppearanceMod();

		// Hero FrmID fix for obj_art_fid/art_change_fid_num script functions
		if (heroAppearanceMod != 2) {
			MakeJump(0x45C5C3, op_obj_art_fid_hack);
			HookCall(0x4572BE, op_metarule3_hook);
		}
		dlogr(" Done", DL_INIT);
	}
}