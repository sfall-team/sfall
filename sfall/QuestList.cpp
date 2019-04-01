/*
 *    sfall
 *    Copyright (C) 2009, 2010  The sfall team
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

#include "main.h"

#include "FalloutEngine.h"
#include "QuestList.h"

#define EXITCODE_BREAK      (-1)
#define EXITCODE_NORMAL     (0)
#define EXITCODE_ERROR      (1)
#define EXITCODE_NEXTQUEST  (2)

static int questsButtonsType;
static int questsScrollButtonsX;
static int questsScrollButtonsY;

static bool buttonsPressed = false;
static bool outRangeFlag = false;
static bool pageFlag = false;
static bool closeFlag = false;
static bool InQuestsList = false;

static bool calledflag = false;
static DWORD called_quest_number = 0;
static DWORD total_quests_pages = 0;
static DWORD curent_quest_page = 0;
static DWORD look_quests = 0;         // check current quests

static DWORD first_quest_page = 0;
static DWORD last_quest_page = MAXINT;

static std::vector<int> pageQuest;

// Fix crash when the quest list is too long
static void __declspec(naked) PipStatus_hook_printfix() {
	__asm {
		test outRangeFlag, 0xFF;
		jnz  force;
		call _word_wrap_;
		push eax;
		movzx eax, word ptr [esp + 0x49C + 8];
		dec  eax;
		shl  eax, 1;
		add  eax, dword ptr ds:[_cursor_line];
		cmp  eax, dword ptr ds:[_bottom_line]; // check max
		jb   skip;
		mov  eax, dword ptr ds:[_quest_count];
		sub  eax, 2;
		mov  dword ptr [esp + 0x4BC - 0x24 + 8], eax; // set last counter
		mov  outRangeFlag, 1;
skip:
		pop  eax;
		retn;
force:
		or   eax, -1; // force log error "out of range"
		mov  outRangeFlag, 0;
		retn;
	}
}
// ------------------------------------

// Add an unused bit for an additional text offset 60 pixels to the right
// it might be needed outside of the function to display quest list pages
static void __declspec(naked) pip_print_hack() {
	__asm {
		test bh, 1;
		jz   skip;
		add  edx, 60;  // pixel offset
		jmp  end;
skip:
		test bh, 2;
		jz   end;
		retn;
end:
		add  dword ptr [esp], 0x14;
		retn;
	}
}

static void ResetPageValues() {
	if (total_quests_pages > 0) pageQuest.resize(1);

	pageFlag = false;
	curent_quest_page = 0;
	total_quests_pages = 0;
	first_quest_page = 0;
	last_quest_page = MAXINT;
}

// Event of entering the quest list and up/down keystrokes
static void __declspec(naked) pipboy_hack_press0() {
	__asm {
		// fixed vanilla bug, preventing displaying the quest list of other locations when clicking on the pipboy screen
		test InQuestsList, 0xFF;
		jz   skip;
		test buttonsPressed, 0xFF;
		jnz  skip;
		add  dword ptr [esp], 0x0E;
		retn;
		// --------
skip:
		mov  calledflag, 1;
		mov  called_quest_number, ebx
		// engine
		mov  edx, dword ptr ds:[_crnt_func];
		retn;
	}
}

// Pipboy keystroke event
static void __declspec(naked) pipboy_hack_press1() {
	__asm {
		mov  InQuestsList, 0;
		mov  calledflag, 0;
		test pageFlag, 0xFF;
		jz   skip;
		push eax;
		push edx;
		push ecx;
		call ResetPageValues;
		pop  ecx;
		pop  edx;
		pop  eax;
skip:
		// engine
		sub ebx, 0x1F4;
		retn;
	}
}

// Press back button
static void __declspec(naked) pipboy_hack_back() {
	__asm {
		mov  InQuestsList, 0;
		mov  calledflag, 0;
		test pageFlag, 0xFF;
		jz   skip;
		push eax;
		push edx;
		push ecx;
		call ResetPageValues;
		pop  ecx;
		pop  edx;
		pop  eax;
skip:
		// engine
		mov eax, 0x401;
		retn;
	}
}

static void AddPage(int lines) {
	total_quests_pages++;
	pageQuest.push_back(look_quests); // add the quest number to array at top of page
	*ptr_cursor_line = 3 + lines; // reset current line to beginning, and add text lines
	outRangeFlag = true;
}

// Print quests page text
static long __fastcall QuestsPrint(const char* text, int width, DWORD* buf, BYTE* count) {
	look_quests++; // quests counter

	if (outRangeFlag) {
		if (pageFlag) return EXITCODE_BREAK; // pages are already counted
		// count the number of pages
		WordWrap(text, width, buf, count);

		// check whether the text of the quest leaves the current page
		int lines = 2 * ((int)*count - 1); // number of lines of text
		if (*ptr_cursor_line + lines >= *ptr_bottom_line) {
			AddPage(lines);                // quest does not fit in the current page, we add a page
		} else {
			*ptr_cursor_line += lines;
		}
		return EXITCODE_NEXTQUEST;
	}

	// check if current quest is in range
	if (look_quests < first_quest_page) { // check lower range
		return EXITCODE_NEXTQUEST;
	}
	if (look_quests > last_quest_page) {  // check upper range
		return EXITCODE_BREAK;           // exit from quests loop
	}

	if (WordWrap(text, width, buf, count) == -1) return EXITCODE_ERROR; // error wrap

	if (!pageFlag || last_quest_page == MAXINT) {        // pages have not been calculated yet
		// check whether the text of the quest leaves the current page
		int lines = 2 * ((int)*count - 1);
		if (*ptr_cursor_line + lines >= *ptr_bottom_line) {
			AddPage(lines); // quest does not fit in the current page, we add a page
			return EXITCODE_NEXTQUEST;
		}
	}
	return EXITCODE_NORMAL;
}

static const DWORD PipStatus_NormalRet = 0x49818B;
static const DWORD PipStatus_NextRet   = 0x498237;
static const DWORD PipStatus_BreakRet  = 0x4982A4;
static void __declspec(naked) PipStatus_hack_print() {
	__asm {
		push ecx;
		push ebx;
		push edx;
		push ecx;         // count
		push ebx;         // buf
		mov  ecx, eax;    // text
		call QuestsPrint; // edx - width
		pop  edx;
		pop  ebx;
		pop  ecx;
		cmp  eax, 0;
		jl   jbreak;
		cmp  eax, 1;
		jg   jnext;
		jmp  PipStatus_NormalRet;
jnext:
		jmp  PipStatus_NextRet;
jbreak:
		jmp  PipStatus_BreakRet;
	}
}

static char bufPage[16];
static const char* format = "%s %d %s %d";
static void __declspec(naked) PrintPages() {
	__asm {
		// total pages
		mov  eax, total_quests_pages;
		inc  eax;
		push eax;
		// text 'of'
		mov  ebx, 212;                      // message
		mov  edx, _pipmesg;
		mov  eax, _pipboy_message_file;
		call getmsg_;
		push eax;
		// current page
		mov  eax, curent_quest_page;
		inc  eax;
		push eax;
		// text 'Page'
		mov  ebx, 213;                      // message
		mov  edx, _pipmesg;
		mov  eax, _pipboy_message_file;
		call getmsg_;
		push eax;

		push format;
		lea  eax, [bufPage];
		push eax;                           // text buf
		call sprintf_;
		add  esp, 0x18;

		mov  ebx, 1;
		mov  dword ptr ds:[_cursor_line], ebx;
		mov  bl, byte ptr ds:[_GreenColor];
		mov  edx, 0x21;
		lea  eax, [bufPage];
		jmp  pip_print_;
	}
}

static void __declspec(naked) PipStatus_hook_end() {
	__asm {
		call pip_back_;
		test total_quests_pages, 0xFF;
		jz   skip;
		push edx;
		call PrintPages;
		pop  edx;
skip:
		mov  look_quests, 0;
		mov  outRangeFlag, 0;
		mov  pageFlag, 1;     // pages are calculated, there is no need to do it again
		mov  closeFlag, 1;    // set flag, need to reset on close pipboy
		mov  InQuestsList, 1; // flag for fix bug
		retn;
	}
}

static DWORD __fastcall ActionButtons(register DWORD key) {
	buttonsPressed = false;
	if (key == 0x300) { // up
		if (curent_quest_page == 0) return -1;
		last_quest_page = pageQuest[curent_quest_page] - 1;
		curent_quest_page--;
		first_quest_page = pageQuest[curent_quest_page];
		buttonsPressed = true;
		return called_quest_number;
	}
	if (key == 0x301) { // down
		if (curent_quest_page < total_quests_pages) {
			curent_quest_page++;
			first_quest_page = pageQuest[curent_quest_page];
			last_quest_page  = ((pageQuest.size() - 1) > curent_quest_page)
							? pageQuest[curent_quest_page + 1] - 1
							: MAXINT;
			buttonsPressed = true;
			return called_quest_number;
		}
	}
	return -1;
}

static void __declspec(naked) pipboy_hack_action() {
	__asm {
		test calledflag, 0xFF;
		jz   skip;
		push eax;
		push edx;
		mov  ecx, ebx;
		call ActionButtons;
		cmp  eax, -1;
		cmovne ebx, eax;  // called_quest_number
		pop  edx;
		pop  eax;
		xor  ecx, ecx;
skip:
		// engine
		cmp  ebx, 0x1F4;
		retn;
	}
}

static void RegisterButtonSoundFunc0() {
	__asm {
		mov  ebx, gsound_red_butt_release_;
		mov  edx, gsound_red_butt_press_;
		call win_register_button_sound_func_;
	}
}

static void __stdcall ArtButtonFunc(DWORD buttonKey, DWORD buttonMem, DWORD indexArt) {
	__asm {
		xor  ecx, ecx;
		xor  ebx, ebx;
		mov  edx, indexArt;          // index from intrface.lst
		mov  eax, 6;                 // OBJ_TYPE_INTRFACE
		push ecx;
		call art_id_;
		//
		mov  ecx, buttonKey;
		xor  ebx, ebx;
		xor  edx, edx;
		call art_ptr_lock_data_;
		mov  ecx, buttonMem;
		mov  dword ptr [ecx], eax;   // first texture memory address
	}
}

// Create buttons
static void __declspec(naked) StartPipboy_hack() {
	__asm {
		push edi;
		mov  ebp, esp; // prolog
		sub  esp, __LOCAL_SIZE;
	}

	DWORD xPos, yPos, height, width, winRef;
	BYTE *picDown, *picUp;

	int indexUpArt0;   // number from intrface.lst for dn button-up
	int indexDownArt0; // for dn button-down
	int indexUpArt1;   // for up button-up
	int indexDownArt1; // for up button-down

	if (questsButtonsType > 1) {
		height = 23;
		width = 22;

		indexUpArt0   = 54;
		indexDownArt0 = 52;
		indexUpArt1   = 53;
		indexDownArt1 = 50;
	} else {
		height = 14;
		width = 11;

		indexUpArt0   = 181;
		indexDownArt0 = 182;
		indexUpArt1   = 199;
		indexDownArt1 = 200;
	}

	// Load new texture for first (up) button. I used memory address for texture from buttons at chracter screen.
	// Everything fine, because this buttons can't use in one time, and they everytime recreating.
	// Down
	ArtButtonFunc(_optionsButtonUpKey,   _optionsButtonUp,   indexUpArt0);
	ArtButtonFunc(_optionsButtonDownKey, _optionsButtonDown, indexDownArt0);
	// Up
	ArtButtonFunc(_optionsButtonUpKey,   _optionsButtonUp1,   indexUpArt1);
	ArtButtonFunc(_optionsButtonDownKey, _optionsButtonDown1, indexDownArt1);

	xPos = questsScrollButtonsX;
	yPos = questsScrollButtonsY;
	winRef = *ptr_pip_win;

	// creating new 2 buttons
	picDown = (BYTE*)*ptr_optionsButtonDown1;
	picUp   = (BYTE*)*ptr_optionsButtonUp1;
	if (WinRegisterButton(winRef, xPos, yPos, width, height, -1, -1, -1, 0x300, picUp,  picDown, 0, 32) != -1) {
		RegisterButtonSoundFunc0();
	}

	picDown = (BYTE*)*ptr_optionsButtonDown;
	picUp   = (BYTE*)*ptr_optionsButtonUp;
	if (WinRegisterButton(winRef, xPos, yPos + height, width, height, -1, -1, -1, 0x301, picUp,  picDown, 0, 32) != -1) {
		RegisterButtonSoundFunc0();
	}

	__asm {
		mov  esp, ebp;    // epilog
		pop  edi;
		mov  ebp, 0x1F4;  // overwrite engine code
		retn;
	}
}

// Reset states when closing pipboy
static void __declspec(naked) pipboy_hook() {
	__asm {
		call EndPipboy_;
		test closeFlag, 0xFF;
		jz   skip;
		call ResetPageValues;
		mov  closeFlag, 0;
skip:
		mov  calledflag, 0;
		mov  InQuestsList, 0;
		retn;
	}
}

void QuestListPatch() {
	MakeCall(0x4974E4, StartPipboy_hack);

	MakeCall(0x497173, pipboy_hack_action, 1);

	MakeCall(0x4971B2, pipboy_hack_press0, 1);
	MakeCall(0x497183, pipboy_hack_press1, 1);

	MakeCall(0x4971D9, pipboy_hack_back);
	HookCall(0x497219, pipboy_hook);

	MakeJump(0x498186, PipStatus_hack_print);
	HookCall(0x4982B0, PipStatus_hook_end);

	MakeCall(0x497A7D, pip_print_hack);
}

void QuestListInit() {
	questsButtonsType = GetPrivateProfileIntA("Misc", "UseScrollingQuestsList", 0, ini);
	if (questsButtonsType > 0) {
		dlog("Applying quests list patch.", DL_INIT);
		QuestListPatch();

		questsScrollButtonsX = GetPrivateProfileIntA("Misc", "QuestsScrollButtonsX", 140, ini);
		if (questsScrollButtonsX < 0 || questsScrollButtonsX > 618) questsScrollButtonsX = 140;
		questsScrollButtonsY = GetPrivateProfileIntA("Misc", "QuestsScrollButtonsY", 334, ini);
		if (questsScrollButtonsY < 0 || questsScrollButtonsY > 434) questsScrollButtonsY = 334;

		pageQuest.reserve(4); // init
		pageQuest.push_back(1);

		dlogr(" Done", DL_INIT);
	} else {
		HookCall(0x498186, PipStatus_hook_printfix); // fix "out of bounds" bug when printing a list of quests
	}
}
