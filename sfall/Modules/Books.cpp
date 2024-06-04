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

#include <stdio.h>

#include "..\main.h"

#include "Books.h"

namespace sfall
{

static int BooksCount = 0;
static const int BooksMax = 50;

#pragma pack(push, 1)
struct sBook {
	DWORD bookPid;
	DWORD msgID;
	DWORD skill;
};
#pragma pack(pop)

static sBook* books = nullptr;

static sBook* __fastcall FindBook(DWORD pid) {
	for (int i = BooksCount; i >= 0; i--) {
		if (books[i].bookPid == pid) {
			return &books[i];
		}
	}
	return 0;
}

static __declspec(naked) void obj_use_book_hook() {
	static const DWORD obj_use_book_hook_back = 0x49BA5A;
	__asm {
		mov  edi, -1;
		mov  ecx, eax;
		call FindBook;
		test eax, eax;
		jz   skip;
		mov  edi, [eax + 4]; // msgID
		mov  ecx, [eax + 8]; // skill
skip:
		jmp  obj_use_book_hook_back;
	}
}

static void LoadVanillaBooks() {
	// book of science
	books[0].bookPid = 73;
	books[0].msgID = 802;
	books[0].skill = 12;
	// Dean's electronics
	books[1].bookPid = 76;
	books[1].msgID = 803;
	books[1].skill = 13;
	// First Aid
	books[2].bookPid = 80;
	books[2].msgID = 804;
	books[2].skill = 6;
	// Guns & Bullets
	books[3].bookPid = 102;
	books[3].msgID = 805;
	books[3].skill = 0;
	// Scouts Handbook
	books[4].bookPid = 86;
	books[4].msgID = 806;
	books[4].skill = 17;
}

void Books::init() {
	auto booksFile = IniReader::GetConfigString("Misc", "BooksFile", "");
	if (!booksFile.empty()) {
		const char* iniBooks = booksFile.insert(0, ".\\").c_str();
		if (GetFileAttributesA(iniBooks) == INVALID_FILE_ATTRIBUTES) return;

		dlog("Applying books patch...", DL_INIT);
		bool includeVanilla = (IniReader::GetInt("main", "overrideVanilla", 0, iniBooks) == 0);
		if (includeVanilla) BooksCount = 5;

		int count = IniReader::GetInt("main", "count", 0, iniBooks);

		int n = 0;
		if (count > 0) {
			if (count > BooksMax) count = BooksMax;
			books = new sBook[BooksCount + count];

			if (includeVanilla) LoadVanillaBooks();

			char section[4];
			for (int i = 1; i <= count; i++) {
				_itoa_s(i, section, 10);
				if (books[BooksCount].bookPid = IniReader::GetInt(section, "PID", 0, iniBooks)) {
					books[BooksCount].msgID = IniReader::GetInt(section, "TextID", 0, iniBooks);
					books[BooksCount].skill = IniReader::GetInt(section, "Skill", 0, iniBooks);
					BooksCount++;
					n++;
				}
			}
			BooksCount--; // set to last index

			MakeJump(0x49B9FB, obj_use_book_hook);
		}
		dlog_f(" (%d/%d books)\n", DL_INIT, n, count);
	}
}

void Books::exit() {
	if (books) delete[] books;
}

}
