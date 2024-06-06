/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "QuestList.h"

namespace sfall
{

enum ExitCode : int {
	Break = -1,
	Normal,
	Error,
	NextQuest
};

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
static DWORD look_quests = 0; // check current quests

static DWORD first_quest_page = 0;
static DWORD last_quest_page = INT_MAX;

static std::vector<int> pageQuest;

// Fix crash when the quest list is too long
static __declspec(naked) void PipStatus_hook_printfix() {
	__asm {
		test outRangeFlag, 0xFF;
		jnz  force;
		call fo::funcoffs::_word_wrap_;
		push eax;
		movzx eax, word ptr [esp + 0x49C + 8];
		dec  eax;
		shl  eax, 1;
		add  eax, dword ptr ds:[FO_VAR_cursor_line];
		cmp  eax, dword ptr ds:[FO_VAR_bottom_line];  // check max
		jb   skip;
		mov  eax, dword ptr ds:[FO_VAR_quest_count];
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
static __declspec(naked) void pip_print_hack() {
	__asm {
		test bh, 1;
		jz   skip;
		add  edx, 60; // pixel offset
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
	last_quest_page = INT_MAX;
}

// Event of entering the quest list and up/down keystrokes
static __declspec(naked) void pipboy_hack_press0() {
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
		mov  edx, dword ptr ds:[FO_VAR_crnt_func];
		retn;
	}
}

// Pipboy keystroke event
static __declspec(naked) void pipboy_hack_press1() {
	__asm {
		mov  InQuestsList, 0;
		mov  calledflag, 0;
		test pageFlag, 0xFF;
		jz   skip;
		pushadc;
		call ResetPageValues;
		popadc;
skip:
		// engine
		sub ebx, 0x1F4;
		retn;
	}
}

// Press back button
static __declspec(naked) void pipboy_hack_back() {
	__asm {
		mov  InQuestsList, 0;
		mov  calledflag, 0;
		test pageFlag, 0xFF;
		jz   skip;
		pushadc;
		call ResetPageValues;
		popadc;
skip:
		// engine
		mov eax, 0x401;
		retn;
	}
}

static void AddPage(int lines) {
	total_quests_pages++;
	pageQuest.push_back(look_quests); // add the quest number to array at top of page
	fo::var::cursor_line = 3 + lines; // reset current line to beginning, and add text lines
	outRangeFlag = true;
}

// Print quests page text
static long __fastcall QuestsPrint(BYTE* count, int width, DWORD* buf, const char* text) {
	look_quests++; // quests counter

	if (outRangeFlag) {
		if (pageFlag) return ExitCode::Break; // pages are already counted
		// count the number of pages
		fo::func::_word_wrap(text, width, buf, count);

		// check whether the text of the quest leaves the current page
		int lines = 2 * ((int)*count - 1); // number of lines of text
		if (fo::var::cursor_line + lines >= fo::var::bottom_line) {
			AddPage(lines);                // quest does not fit in the current page, we add a page
		} else {
			fo::var::cursor_line += lines;
		}
		return ExitCode::NextQuest;
	}

	// check if current quest is in range
	if (look_quests < first_quest_page) { // check lower range
		return ExitCode::NextQuest;
	}
	if (look_quests > last_quest_page) {  // check upper range
		return ExitCode::Break;           // exit from quests loop
	}

	if (fo::func::_word_wrap(text, width, buf, count) == -1) return ExitCode::Error; // error wrap

	if (!pageFlag || last_quest_page == INT_MAX) {        // pages have not been calculated yet
		// check whether the text of the quest leaves the current page
		int lines = 2 * ((int)*count - 1);
		if (fo::var::cursor_line + lines >= fo::var::bottom_line) {
			AddPage(lines); // quest does not fit in the current page, we add a page
			return ExitCode::NextQuest;
		}
	}
	return ExitCode::Normal;
}

static __declspec(naked) void PipStatus_hack_print() {
	static const DWORD PipStatus_NormalRet = 0x49818B;
	static const DWORD PipStatus_NextRet   = 0x498237;
	static const DWORD PipStatus_BreakRet  = 0x4982A4;
	__asm {
		push ecx;
		push edx;
		push eax;         // text
		push ebx;         // buf
		call QuestsPrint; // ecx - count, edx - width
		pop  edx;
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

static __declspec(naked) void PrintPages() {
	__asm {
		// total pages
		mov  eax, total_quests_pages;
		inc  eax;
		push eax;
		// text 'of'
		mov  ebx, 212;                      // message
		mov  edx, FO_VAR_pipmesg;
		mov  eax, FO_VAR_pipboy_message_file;
		call fo::funcoffs::getmsg_;
		push eax;
		// current page
		mov  eax, curent_quest_page;
		inc  eax;
		push eax;
		// text 'Page'
		mov  ebx, 213;                      // message
		mov  edx, FO_VAR_pipmesg;
		mov  eax, FO_VAR_pipboy_message_file;
		call fo::funcoffs::getmsg_;
		push eax;

		push format;
		lea  eax, bufPage;
		push eax;                           // text buf
		call fo::funcoffs::sprintf_;
		add  esp, 0x18;

		mov  ebx, 1;
		mov  dword ptr ds:[FO_VAR_cursor_line], ebx;
		mov  bl, byte ptr ds:[FO_VAR_GreenColor];
		mov  edx, 0x21;
		lea  eax, bufPage;
		jmp  fo::funcoffs::pip_print_;
	}
}

static __declspec(naked) void PipStatus_hook_end() {
	__asm {
		call fo::funcoffs::pip_back_;
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

static DWORD __fastcall ActionButtons(DWORD key) {
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
			                 : INT_MAX;
			buttonsPressed = true;
			return called_quest_number;
		}
	}
	return -1;
}

static __declspec(naked) void pipboy_hack_action() {
	__asm {
		test calledflag, 0xFF;
		jz   skip;
		push eax;
		push edx;
		mov  ecx, ebx;
		call ActionButtons;
		cmp  eax, -1;
		cmovne ebx, eax; // called_quest_number
		pop  edx;
		pop  eax;
		xor  ecx, ecx;
skip:
		// engine
		cmp  ebx, 0x1F4;
		retn;
	}
}

static void RegisterButtonSound() {
	__asm {
		mov  ebx, fo::funcoffs::gsound_red_butt_release_;
		mov  edx, fo::funcoffs::gsound_red_butt_press_;
		call fo::funcoffs::win_register_button_sound_func_; // eax - register button
	}
}

static void LoadArtButton(DWORD buttonKey, DWORD buttonMem, DWORD indexArt) { // indexArt - index from intrface.lst
	long artId = fo::func::art_id(fo::ArtType::OBJ_TYPE_INTRFACE, indexArt, 0, 0, 0);
	*(BYTE**)buttonMem = fo::func::art_ptr_lock_data(artId, 0, 0, (DWORD*)buttonKey); // first texture memory address
}

// Create buttons
static __declspec(naked) void StartPipboy_hack() {
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
	LoadArtButton(FO_VAR_optionsButtonUpKey,   FO_VAR_optionsButtonUp,   indexUpArt0);
	LoadArtButton(FO_VAR_optionsButtonDownKey, FO_VAR_optionsButtonDown, indexDownArt0);
	// Up
	LoadArtButton(FO_VAR_optionsButtonUpKey,   FO_VAR_optionsButtonUp1,   indexUpArt1);
	LoadArtButton(FO_VAR_optionsButtonDownKey, FO_VAR_optionsButtonDown1, indexDownArt1);

	xPos = questsScrollButtonsX;
	yPos = questsScrollButtonsY;
	winRef = fo::var::pip_win;

	// creating new 2 buttons
	picDown = (BYTE*)fo::var::optionsButtonDown1;
	picUp   = (BYTE*)fo::var::optionsButtonUp1;
	if (fo::func::win_register_button(winRef, xPos, yPos, width, height, -1, -1, -1, 0x300, picUp,  picDown, 0, 32) != -1) {
		RegisterButtonSound();
	}

	picDown = (BYTE*)fo::var::optionsButtonDown;
	picUp   = (BYTE*)fo::var::optionsButtonUp;
	if (fo::func::win_register_button(winRef, xPos, yPos + height, width, height, -1, -1, -1, 0x301, picUp,  picDown, 0, 32) != -1) {
		RegisterButtonSound();
	}

	__asm {
		mov  esp, ebp;   // epilog
		pop  edi;
		mov  ebp, 0x1F4; // overwrite engine code
		retn;
	}
}

// Reset states when closing pipboy
static __declspec(naked) void pipboy_hook() {
	__asm {
		call fo::funcoffs::EndPipboy_;
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

static void __fastcall QuestListSort(fo::QuestData* questList, size_t numElements) {
	if (numElements <= 1) return;

	fo::QuestData* tmpList = new fo::QuestData[numElements];
	std::memcpy(tmpList, questList, numElements * sizeof(fo::QuestData));

	long locStart = 1500; // message number from which the locations starting
	size_t leftStart = 0;
	long locLast = 0;

	for (size_t i = 0; i < numElements; i++) {
		long loc = tmpList[i].location;
		if (locStart > loc) locStart = loc;
		if (loc > locLast) locLast = loc;
	}

	for (size_t i = 0; i < numElements;) {
		for (size_t j = leftStart; j < numElements; j++) {
			if (!tmpList[j].location) {
				if ((j - leftStart) == 1) leftStart = j;
				continue;
			}
			if (tmpList[j].location == locStart) {
				// unsorted?
				if (j != i) questList[i] = tmpList[j];
				if (++i >= numElements) break;
				tmpList[j].location = 0;
			}
		}
		if (++locStart > locLast) break;
	}
	delete[] tmpList;
}

static __declspec(naked) void quest_init_hook() {
	__asm {
		mov  ecx, eax;      // questList (base)
		jmp  QuestListSort; // edx - numElements
	}
}

static void QuestListPatch() {
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

////////////////////////////////////////////////////////////////////////////////

struct QuestFailure {
	long gvarNum;
	long failureVal;
};

std::vector<QuestFailure> questFailures;

static long FindGVarQuestFailure(long globalVarNum) {
	for (size_t i = 0; i < questFailures.size(); i++) {
		if (questFailures[i].gvarNum == globalVarNum) return i;
	}
	return -1;
}

void QuestList::AddQuestFailureValue(long globalVarNum, long failureThreshold) {
	long index = FindGVarQuestFailure(globalVarNum);
	if (index == -1) {
		questFailures.push_back({ globalVarNum, failureThreshold });
	} else {
		questFailures[index].failureVal = failureThreshold;
	}
}

static BYTE __fastcall CheckQuestFailureState(fo::QuestData* quest, BYTE completeColor) {
	if (questFailures.empty()) return completeColor;

	const BYTE failureColor = 137; // dark red

	long index = FindGVarQuestFailure(quest->gvarIndex);
	return (index != -1 && fo::var::game_global_vars[quest->gvarIndex] >= questFailures[index].failureVal) ? failureColor : completeColor;
}

static __declspec(naked) void PipStatus_hack() {
	__asm {
		push eax;
		push ecx;
		mov  dl, ds:[0x6A5B34]; // completeColor (dark green)
		mov  ecx, ds:[FO_VAR_quests];
		add  ecx, [esp + 0x4BC - 0x28 + 12];
		call CheckQuestFailureState;
		mov  bl, al;
		pop  ecx;
		pop  eax;
		retn;
	}
}

static void ResetQuests() {
	questFailures.clear();
}

void QuestList::init() {
	LoadGameHook::OnGameReset() += ResetQuests;

	MakeCall(0x498222, PipStatus_hack, 1);

	questsButtonsType = IniReader::GetConfigInt("Misc", "UseScrollingQuestsList", 0);
	if (questsButtonsType > 0) {
		dlogr("Applying quests list patch.", DL_INIT);
		QuestListPatch();

		questsScrollButtonsX = IniReader::GetConfigInt("Misc", "QuestsScrollButtonsX", 140);
		if (questsScrollButtonsX < 0 || questsScrollButtonsX > 618) questsScrollButtonsX = 140;
		questsScrollButtonsY = IniReader::GetConfigInt("Misc", "QuestsScrollButtonsY", 334);
		if (questsScrollButtonsY < 0 || questsScrollButtonsY > 434) questsScrollButtonsY = 334;

		pageQuest.reserve(4); // init
		pageQuest.push_back(1);
	} else {
		HookCall(0x498186, PipStatus_hook_printfix); // fix "out of bounds" bug when printing a list of quests
	}

	// Replace the qsort_ unstable sorting function with a simple stable sorting algorithm
	HookCall(0x49A7C2, quest_init_hook);
}

}
