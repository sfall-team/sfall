/*
 *    sfall
 *    Copyright (C) 2008, 2009, 2010  The sfall team
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

#include "..\main.h"

#include "Books.h"

namespace sfall 
{

static int BooksCount = 0;
static const int BooksMax = 30;
static char iniBooks[MAX_PATH];

struct sBook {
	DWORD bookPid;
	DWORD msgID;
	DWORD skill;
};

static sBook books[BooksMax];

static sBook* _stdcall FindBook(DWORD pid) {
	for (int i = 0; i < BooksCount; i++) {
		if (books[i].bookPid == pid) {
			return &books[i];
		}
	}
	return 0;
}

static const DWORD obj_use_book_hook_back = 0x49BA5A;
static void __declspec(naked) obj_use_book_hook() {
	__asm {
		push eax;
		call FindBook;
		test eax, eax;
		jnz found;
		mov edi, -1;
		jmp end;
found:
		mov edi, [eax + 4];
		mov ecx, [eax + 8];
end:
		jmp obj_use_book_hook_back;
	}
}

void LoadVanillaBooks() {
	int i = BooksCount;
	// book of science
	books[i + 0].bookPid = 73;
	books[i + 0].msgID = 802;
	books[i + 0].skill = 12;
	// Dean's electronics
	books[i + 1].bookPid = 76;
	books[i + 1].msgID = 803;
	books[i + 1].skill = 13;
	// First Aid
	books[i + 2].bookPid = 80;
	books[i + 2].msgID = 804;
	books[i + 2].skill = 6;
	// Guns & Bullets
	books[i + 3].bookPid = 102;
	books[i + 3].msgID = 805;
	books[i + 3].skill = 0;
	// Scouts Handbook
	books[i + 4].bookPid = 86;
	books[i + 4].msgID = 806;
	books[i + 4].skill = 17;
	BooksCount += 5;
}

void Books::init() {
	auto booksFile = GetConfigString("Misc", "BooksFile", "", MAX_PATH);
	if (booksFile.size() > 0) {
		sprintf(iniBooks, ".\\%s", booksFile.c_str());
		dlog("Applying books patch...", DL_INIT);
		memset(books, 0, sizeof(sBook)*BooksCount);

		int i, n = 0, count;
		if (GetPrivateProfileIntA("main", "overrideVanilla", 0, iniBooks) == 0) {
			LoadVanillaBooks();
		}
		count = GetPrivateProfileIntA("main", "count", 0, iniBooks);

		char section[4];
		for (i = 1; i <= count; i++) {
			_itoa_s(i, section, 10);
			if (BooksCount >= BooksMax) break;
			if (books[BooksCount].bookPid = GetPrivateProfileIntA(section, "PID", 0, iniBooks)) {
				books[BooksCount].msgID = GetPrivateProfileIntA(section, "TextID", 0, iniBooks);
				books[BooksCount].skill = GetPrivateProfileIntA(section, "Skill", 0, iniBooks);
				n++;
				BooksCount++;
			}
		}

		MakeCall(0x49B9FB, &obj_use_book_hook, true);
		dlog_f(" (%d/%d books) Done\n", DL_INIT, n, count);
	}
}

}
