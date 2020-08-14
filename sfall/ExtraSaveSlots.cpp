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

#include "ExtraSaveSlots.h"

DWORD LSPageOffset = 0;

int LSButtDN = 0;
BYTE* SaveLoadSurface = nullptr;

static const char* filename = "%s\\savegame\\slotdat.ini";

void SavePageOffsets() {
	char SavePath[MAX_PATH], buffer[6];

	sprintf_s(SavePath, MAX_PATH, filename, *ptr_patches);

	_itoa_s(*ptr_slot_cursor, buffer, 10);
	WritePrivateProfileStringA("POSITION", "ListNum", buffer, SavePath);
	_itoa_s(LSPageOffset, buffer, 10);
	WritePrivateProfileStringA("POSITION", "PageOffset", buffer, SavePath);
}

static void __declspec(naked) save_page_offsets(void) {
	__asm {
		// save last slot position values to file
		call SavePageOffsets;
		// restore original code
		mov  eax, dword ptr ds:[_lsgwin];
		retn;
	}
}

void LoadPageOffsets() {
	char LoadPath[MAX_PATH];

	sprintf_s(LoadPath, MAX_PATH, filename, *ptr_patches);

	*ptr_slot_cursor = iniGetInt("POSITION", "ListNum", 0, LoadPath);
	if (*ptr_slot_cursor > 9) {
		*ptr_slot_cursor = 0;
	}
	LSPageOffset = iniGetInt("POSITION", "PageOffset", 0, LoadPath);
	if (LSPageOffset > 9990) {
		LSPageOffset = 0;
	}
}

static void __declspec(naked) load_page_offsets(void) {
	__asm {
		// load last slot position values from file
		call LoadPageOffsets;
		// restore original code
		mov  edx, 0x50A480; // ASCII "SAV"
		retn;
	}
}

static void CreateButtons() {
	DWORD winRef = *ptr_lsgwin;

	// left button -10       | X | Y | W | H |HOn |HOff |BDown |BUp |PicUp |PicDown |? |ButType
	WinRegisterButton(winRef, 100, 60, 24, 20, -1, 0x500, 0x54B, 0x14B, 0, 0, 0, 32);
	// left button -100
	WinRegisterButton(winRef,  68, 60, 24, 20, -1, 0x500, 0x549, 0x149, 0, 0, 0, 32);
	// right button +10
	WinRegisterButton(winRef, 216, 60, 24, 20, -1, 0x500, 0x54D, 0x14D, 0, 0, 0, 32);
	// right button +100
	WinRegisterButton(winRef, 248, 60, 24, 20, -1, 0x500, 0x551, 0x151, 0, 0, 0, 32);
	// Set Number button
	WinRegisterButton(winRef, 140, 60, 60, 20, -1, -1, 'p', -1, 0, 0, 0, 32);
}

static void __declspec(naked) create_page_buttons(void) {
	__asm {
		call CreateButtons;
		mov  eax, 0x65; // restore original code
		retn;
	}
}

void SetPageNum() {
	DWORD winRef = *ptr_lsgwin; // load/save winref
	if (winRef == 0) {
		return;
	}
	WINinfo *SaveLoadWin = GNWFind(winRef);
	if (SaveLoadWin->surface == nullptr) {
		return;
	}

	BYTE ConsoleGold = *ptr_YellowColor; // palette offset stored in mem - text colour

	char TempText[32];
	unsigned int TxtMaxWidth = GetMaxCharWidth() * 8; // GetTextWidth(TempText);
	unsigned int HalfMaxWidth = TxtMaxWidth / 2;
	unsigned int TxtWidth = 0;

	DWORD NewTick = 0, OldTick = 0;
	int button = 0, exitFlag = 0, numpos = 0;
	char Number[5], blip = '_';

	DWORD tempPageOffset = -1;

	char* EndBracket = "]";
	int width = GetTextWidth(EndBracket);

	while (!exitFlag) {
		NewTick = GetTickCount(); // timer for redraw
		if (OldTick > NewTick) {
			OldTick = NewTick;
		}
		if (NewTick - OldTick > 166) { // time to draw
			OldTick = NewTick;

			blip = (blip == '_') ? ' ' : '_';

			if (tempPageOffset == -1) {
				sprintf_s(TempText, 32, "[ %c ]", '_');
			} else {
				sprintf_s(TempText, 32, "[ %d%c ]", tempPageOffset / 10 + 1, '_');
			}
			TxtWidth = GetTextWidth(TempText);

			if (tempPageOffset == -1) {
				sprintf_s(TempText, 32, "[ %c", blip);
			} else {
				sprintf_s(TempText, 32, "[ %d%c", tempPageOffset / 10 + 1, blip);
			}

			int z = 0;
			// paste image part from buffer into text area
			for (int y = SaveLoadWin->width * 60; y < SaveLoadWin->width * 75; y += SaveLoadWin->width) {
				memcpy(SaveLoadWin->surface + y + (170 - HalfMaxWidth), SaveLoadSurface + (100 - HalfMaxWidth) + (200 * z++), TxtMaxWidth);
			}

			int HalfTxtWidth = TxtWidth / 2;

			PrintText(TempText, ConsoleGold, 170 - HalfTxtWidth, 64, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);
			PrintText(EndBracket, ConsoleGold, (170 - HalfTxtWidth) + TxtWidth - width, 64, width, SaveLoadWin->width, SaveLoadWin->surface);
			WinDraw(winRef);
		}

		button = GetInputBtn();
		if (button >= '0' && button <= '9') {
			if (numpos < 4) {
				Number[numpos] = button;
				Number[numpos + 1] = '\0';
				numpos++;
				if (Number[0] == '0') {
					numpos = 0;
					tempPageOffset = -1;
				} else {
					tempPageOffset = (atoi(Number) - 1) * 10;
				}
			}
			//else exitFlag=-1;
		} else if (button == 0x08 && numpos) {
			numpos--;
			Number[numpos] = '\0';
			if (!numpos) {
				tempPageOffset = -1;
			} else {
				tempPageOffset = (atoi(Number) - 1) * 10;
			}
		} else if (button == 0x0D || button == 0x20 || button == 'p' || button == 'P') {
			exitFlag = -1; // Enter, Space or P Keys
		} else if (button == 0x1B) {
			tempPageOffset = -1, exitFlag = -1; // Esc key
		}
	}

	if (tempPageOffset != -1 && tempPageOffset <= 9990) {
		LSPageOffset = tempPageOffset;
	}

	SaveLoadWin = nullptr;
}

static long __fastcall CheckPage(long button) {
	switch (button) {
		case 0x14B:                        // left button
			if (LSPageOffset >= 10) LSPageOffset -= 10;
			__asm call gsound_red_butt_press_;
			break;
		case 0x149:                        // fast left PGUP button
			if (LSPageOffset < 100) {
				LSPageOffset = 0;          // First Page
			} else {
				LSPageOffset -= 100;
			}
			__asm call gsound_red_butt_press_;
			break;
		case 0x14D:                        // right button
			if (LSPageOffset <= 9980) LSPageOffset += 10;
			__asm call gsound_red_butt_press_;
			break;
		case 0x151:                        // fast right PGDN button
			if (LSPageOffset > 9890) {
				LSPageOffset = 9990;       // Last Page
			} else {
				LSPageOffset += 100;
			}
			__asm call gsound_red_butt_press_;
			break;
		case 'p':                          // p/P button pressed - start SetPageNum func
		case 'P':
			SetPageNum();
			break;
		default:
			if (button < 0x500) return 1;  // button in down state
	}

	LSButtDN = button;
	return 0;
}

static void __declspec(naked) check_page_buttons(void) {
	__asm {
		pushad;
		mov  ecx, eax;
		call CheckPage;
		test eax, eax;
		popad;
		jnz  CheckUp;
		add  dword ptr ds:[esp], 26;        // set return to button pressed code
		jmp  GetSlotList_;                  // reset page save list func
CheckUp:
		// restore original code
		cmp  eax, 0x148;                    // up button
		retn;
	}
}

void DrawPageText() {
	if (*ptr_lsgwin == 0) {
		return;
	}
	WINinfo *SaveLoadWin = GNWFind(*ptr_lsgwin);
	if (SaveLoadWin->surface == nullptr) {
		return;
	}

	int z = 0;
	if (SaveLoadSurface == nullptr) {
		SaveLoadSurface = new BYTE[3000];
		// save part of original image to buffer
		for (int y = SaveLoadWin->width * 60; y < SaveLoadWin->width * 75; y += SaveLoadWin->width) {
			memcpy(SaveLoadSurface + (200 * z++), SaveLoadWin->surface + 70 + y, 200);
		}
	} else {
		// paste image from buffer into text area
		for (int y = SaveLoadWin->width * 60; y < SaveLoadWin->width * 75; y += SaveLoadWin->width) {
			memcpy(SaveLoadWin->surface + 70 + y, SaveLoadSurface + (200 * z++), 200);
		}
	}

	BYTE ConsoleGreen = *ptr_GreenColor; // palette offset stored in mem - text colour
	BYTE ConsoleGold = *ptr_YellowColor; // palette offset stored in mem - text colour
	BYTE Colour = ConsoleGreen;

	char TempText[32];
	sprintf_s(TempText, 32, "[ %d ]", LSPageOffset / 10 + 1);

	unsigned int TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 170 - TxtWidth / 2, 64, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x549) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, "<<");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 80 - TxtWidth / 2, 64, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x54B) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, "<");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 112 - TxtWidth / 2, 64, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x551) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, ">>");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 260 - TxtWidth / 2, 64, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	if (LSButtDN == 0x54D) {
		Colour = ConsoleGold;
	} else {
		Colour = ConsoleGreen;
	}
	strcpy_s(TempText, 12, ">");
	TxtWidth = GetTextWidth(TempText);
	PrintText(TempText, Colour, 228 - TxtWidth / 2, 64, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

	SaveLoadWin = nullptr;
}

static void __declspec(naked) draw_page_text(void) {
	__asm {
		pushad;
		call DrawPageText;
		popad;
		mov  ebp, 0x57; // restore original code
		retn;
	}
}

// add page num offset when reading and writing various save data files
static void __declspec(naked) AddPageOffset01(void) {
	__asm {
		mov  eax, dword ptr ds:[_slot_cursor]; // list position 0-9
		add  eax, LSPageOffset; // add page num offset
		retn;
	}
}

// getting info for the 10 currently displayed save slots from save.dats
static void __declspec(naked) AddPageOffset02(void) {
	__asm {
		push 0x50A514;          // ASCII "SAVE.DAT"
		lea  eax, [ebx + 1];
		add  eax, LSPageOffset; // add page num offset
		mov  edx, 0x47E5E9;     // ret addr
		jmp  edx;
	}
}

// printing current 10 slot numbers
static void __declspec(naked) AddPageOffset03(void) {
	__asm {
		inc  eax;
		add  eax, LSPageOffset; // add page num offset
		mov  bl, byte ptr ss:[esp + 0x10]; // add 4 bytes - func ret addr
		retn;
	}
}

void EnableSuperSaving() {
	// save/load button setup func
	MakeCall(0x47D80D, create_page_buttons);

	// Draw button text
	MakeCall(0x47E6E8, draw_page_text);

	// check save/load buttons
	const DWORD checkPageBtnsAddr[] = {0x47BD49, 0x47CB1C};
	MakeCalls(check_page_buttons, checkPageBtnsAddr);

	// save current page and list positions to file on load/save scrn exit
	MakeCall(0x47D828, save_page_offsets);

	// load saved page and list positions from file
	MakeCall(0x47B82B, load_page_offsets);

	// Add Load/Save page offset to Load/Save folder number
	const DWORD AddPageOffset01Addr[] = {
		0x47B929, 0x47D8DB, 0x47D9B0, 0x47DA34, 0x47DABF, 0x47DB58, 0x47DBE9,
		0x47DC9C, 0x47EC77, 0x47F5AB, 0x47F694, 0x47F6EB, 0x47F7FB, 0x47F892,
		0x47FB86, 0x47FC3A, 0x47FCF2, 0x480117, 0x4801CF, 0x480234, 0x480310,
		0x4803F3, 0x48049F, 0x480512, 0x4805F2, 0x480767, 0x4807E6, 0x480839,
		0x4808D3
	};
	MakeCalls(AddPageOffset01, AddPageOffset01Addr);

	MakeJump(0x47E5E1, AddPageOffset02);

	MakeCall(0x47E756, AddPageOffset03);
}

static void GetSaveFileTime(char* filename, FILETIME* ftSlot) {
	char fname[65];
	sprintf_s(fname, "%s\\%s", *ptr_patches, filename);

	HANDLE hFile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		GetFileTime(hFile, NULL, NULL, ftSlot);
		CloseHandle(hFile);
	} else {
		ftSlot->dwHighDateTime = 0;
		ftSlot->dwLowDateTime = 0;
	};
}

static const char* commentFmt = "%02d/%02d/%d - %02d:%02d:%02d";

static void CreateSaveComment(char* bufstr) {
	SYSTEMTIME stUTC, stLocal;
	GetSystemTime(&stUTC);
	SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

	char buf[30];
	sprintf_s(buf, commentFmt, stLocal.wDay, stLocal.wMonth, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
	strcpy(bufstr, buf);
}

static long autoQuickSave = 0;
static long quickSavePage = 0;

static FILETIME ftPrevSlot;

static DWORD __stdcall QuickSaveGame(DbFile* file, char* filename) {
	long currSlot = *ptr_slot_cursor;

	if (file) { // This slot is not empty
		DbFClose(file);

		FILETIME ftCurrSlot;
		GetSaveFileTime(filename, &ftCurrSlot);

		if (currSlot == 0 || ftCurrSlot.dwHighDateTime > ftPrevSlot.dwHighDateTime
			|| (ftCurrSlot.dwHighDateTime == ftPrevSlot.dwHighDateTime && ftCurrSlot.dwLowDateTime > ftPrevSlot.dwLowDateTime))
		{
			ftPrevSlot.dwHighDateTime = ftCurrSlot.dwHighDateTime;
			ftPrevSlot.dwLowDateTime  = ftCurrSlot.dwLowDateTime;

			if (++currSlot > autoQuickSave) {
				currSlot = 0;
			} else {
				*ptr_slot_cursor = currSlot;
				return 0x47B929; // check next slot
			}
		}
	}

	// Save to slot
	*ptr_slot_cursor = currSlot;
	LSData* saveData = (LSData*)_LSData;
	CreateSaveComment(saveData[currSlot].comment);
	*ptr_quick_done = 1;

	return 0x47B9A4; // normal return
}

static void __declspec(naked) SaveGame_hack0() {
	__asm {
		mov  ds:[_flptr], eax;
		push ecx;
		push edi;
		push eax;
		call QuickSaveGame;
		pop  ecx;
		jmp  eax;
	}
}

static void __declspec(naked) SaveGame_hack1() {
	__asm {
		mov ds:[_slot_cursor], 0;
		mov eax, quickSavePage;
		mov LSPageOffset, eax;
		retn;
	}
}

///////////////////////////////////////////////////////////////////////////////

static void __fastcall SetSaveComment(char* comment) {
	int i = 0;
	const char* mapName = MapGetShortName(*ptr_map_number);
	do {
		comment[i] = mapName[i];
	} while (++i < 20 && mapName[i]);
	comment[i++] = ':';
	comment[i] = '\0';
}

static void __declspec(naked) GetComment_hack() {
	__asm {
		cmp [ecx], 0;
		jne notEmpty;
		pushadc;
		call SetSaveComment;
		popadc;
notEmpty:
		push 0x47F031;
		jmp  get_input_str2_;
	}
}

void ExtraSaveSlotsInit() {
	bool extraSaveSlots = (GetConfigInt("Misc", "ExtraSaveSlots", 0) != 0);
	if (extraSaveSlots) {
		dlog("Applying extra save slots patch.", DL_INIT);
		EnableSuperSaving();
		dlogr(" Done", DL_INIT);
	}

	autoQuickSave = GetConfigInt("Misc", "AutoQuickSave", 0);
	if (autoQuickSave > 0) {
		dlog("Applying auto quick save patch.", DL_INIT);
		if (autoQuickSave > 10) autoQuickSave = 10;
		autoQuickSave--; // reserved slot count

		quickSavePage = GetConfigInt("Misc", "AutoQuickSavePage", 0);
		if (quickSavePage > 1000) quickSavePage = 1000;

		if (extraSaveSlots && quickSavePage > 0) {
			quickSavePage = (quickSavePage - 1) * 10;
			MakeCall(0x47B923, SaveGame_hack1, 1);
		} else {
			SafeWrite8(0x47B923, 0x89);
			SafeWrite32(0x47B924, 0x5193B83D); // mov [slot_cursor], edi = 0
		}
		MakeJump(0x47B984, SaveGame_hack0);
		dlogr(" Done", DL_INIT);
	}

	// Adds the city name in the description for empty save slots
	MakeJump(0x47F02C, GetComment_hack);
}

void ExtraSaveSlotsExit() {
	if (SaveLoadSurface) delete[] SaveLoadSurface;
}
