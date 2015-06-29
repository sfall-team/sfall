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

#include "main.h"

#include "AnimationsAtOnceLimit.h"


static bool AniLimitFixActive=false;

//pointers to new animation struct arrays
static BYTE *aniMem;
static BYTE *aniMem2;

void AnimationsAtOnceInit(signed char aniMax) {

	if(aniMax<=32) return;

	AniLimitFixActive=true;

	//allocate memory to store larger animation struct arrays
	aniMem = new BYTE[2656*(aniMax+1)];
	aniMem2 = new BYTE[3240*(aniMax+1)];


	//set general animation limit check (old 20) aniMax-12 -- +12 reserved for PC movement(4) + other critical animations(8)?
	SafeWrite8(0x413C07, aniMax-12);

	//PC movement animation limit checks (old 24) aniMax-8 -- +8 reserved for other critical animations?.
	SafeWrite8(0x416E11, aniMax-8);
	SafeWrite8(0x416F64, aniMax-8);
	SafeWrite8(0x417143, aniMax-8);
	SafeWrite8(0x41725C, aniMax-8);
	SafeWrite8(0x4179CC, aniMax-8);

	//Max animation limit checks (old 32) aniMax
	SafeWrite8(0x413A70, aniMax);
	SafeWrite8(0x413ADD, aniMax);
	SafeWrite8(0x413BDD, aniMax);
	SafeWrite8(0x413EB8, aniMax);
	SafeWrite8(0x413F4E, aniMax);
	SafeWrite8(0x4186F1, aniMax);


	//Max animations checks - animation struct size * max num of animations (old 2656*32=84992)
	SafeWrite32(0x413AA9, 2656*aniMax);
	SafeWrite32(0x413CB7, 2656*aniMax);
	SafeWrite32(0x413DC2, 2656*aniMax);
	SafeWrite32(0x417F3A, 2656*aniMax);


	//divert old animation structure list pointers to newly alocated memory

	//struct array 1///////////////////

	//old addr 0x54C1B4
	SafeWrite32(0x413A9E, (DWORD)aniMem);

	//old addr 0x54C1C0
	SafeWrite32(0x413AA4, 12+(DWORD)aniMem);
	SafeWrite32(0x413DBC, 12+(DWORD)aniMem);

	//old addr 0x54CC14
	SafeWrite32(0x413B96, 2656+(DWORD)aniMem);
	SafeWrite32(0x413C5A, 2656+(DWORD)aniMem);
	SafeWrite32(0x413CF0, 2656+(DWORD)aniMem);
	SafeWrite32(0x413DE1, 2656+(DWORD)aniMem);
	SafeWrite32(0x413E66, 2656+(DWORD)aniMem);
	SafeWrite32(0x413EF3, 2656+(DWORD)aniMem);
	SafeWrite32(0x413FA2, 2656+(DWORD)aniMem);
	SafeWrite32(0x414161, 2656+(DWORD)aniMem);
	SafeWrite32(0x4142D3, 2656+(DWORD)aniMem);
	SafeWrite32(0x41449A, 2656+(DWORD)aniMem);
	SafeWrite32(0x41460B, 2656+(DWORD)aniMem);
	SafeWrite32(0x4146FF, 2656+(DWORD)aniMem);
	SafeWrite32(0x414826, 2656+(DWORD)aniMem);
	SafeWrite32(0x41491A, 2656+(DWORD)aniMem);
	SafeWrite32(0x4149F8, 2656+(DWORD)aniMem);
	SafeWrite32(0x414AD0, 2656+(DWORD)aniMem);
	SafeWrite32(0x414BA4, 2656+(DWORD)aniMem);
	SafeWrite32(0x414C8C, 2656+(DWORD)aniMem);
	SafeWrite32(0x414CF0, 2656+(DWORD)aniMem);
	SafeWrite32(0x414D60, 2656+(DWORD)aniMem);
	SafeWrite32(0x414DD0, 2656+(DWORD)aniMem);
	SafeWrite32(0x414E48, 2656+(DWORD)aniMem);
	SafeWrite32(0x414EDA, 2656+(DWORD)aniMem);
	SafeWrite32(0x414F5E, 2656+(DWORD)aniMem);
	SafeWrite32(0x414FEE, 2656+(DWORD)aniMem);
	SafeWrite32(0x41505C, 2656+(DWORD)aniMem);
	SafeWrite32(0x4150D0, 2656+(DWORD)aniMem);
	SafeWrite32(0x415158, 2656+(DWORD)aniMem);
	SafeWrite32(0x4151B8, 2656+(DWORD)aniMem);
	SafeWrite32(0x415286, 2656+(DWORD)aniMem);
	SafeWrite32(0x41535C, 2656+(DWORD)aniMem);
	SafeWrite32(0x4153D0, 2656+(DWORD)aniMem);
	SafeWrite32(0x41544A, 2656+(DWORD)aniMem);
	SafeWrite32(0x4154EC, 2656+(DWORD)aniMem);
	SafeWrite32(0x4155EA, 2656+(DWORD)aniMem);
	SafeWrite32(0x4156C0, 2656+(DWORD)aniMem);
	SafeWrite32(0x4156D5, 2656+(DWORD)aniMem);
	SafeWrite32(0x4156F2, 2656+(DWORD)aniMem);
	SafeWrite32(0x41572F, 2656+(DWORD)aniMem);
	SafeWrite32(0x41573E, 2656+(DWORD)aniMem);
	SafeWrite32(0x415B1B, 2656+(DWORD)aniMem);
	SafeWrite32(0x415B56, 2656+(DWORD)aniMem);
	SafeWrite32(0x415BB6, 2656+(DWORD)aniMem);
	SafeWrite32(0x415C7C, 2656+(DWORD)aniMem);
	SafeWrite32(0x415CA3, 2656+(DWORD)aniMem);
	SafeWrite32(0x415DE4, 2656+(DWORD)aniMem);

	//old addr 0x54CC18
	SafeWrite32(0x413D07, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415700, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415B6B, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415B78, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415C2D, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415D38, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415D56, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415D63, 2656+4+(DWORD)aniMem);
	SafeWrite32(0x415DCF, 2656+4+(DWORD)aniMem);

	//old addr 0x54CC1C
	SafeWrite32(0x413C6A, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413CA3, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413CF6, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413E76, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413EA4, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413F03, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413F20, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x413F3A, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x4156EC, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x415B72, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x415C18, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x415C58, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x415C6D, 2656+8+(DWORD)aniMem);
	SafeWrite32(0x415DBE, 2656+8+(DWORD)aniMem);

	//old addr 0x54CC20
	SafeWrite32(0x413B2A, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413B33, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413B43, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413B54, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413B66, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413BA2, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413BAB, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413BC0, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413BCD, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413C3C, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413C87, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413D01, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413D10, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413D36, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413D53, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413DAD, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x413E93, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x4155DF, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x415AE2, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x415D9A, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x415DDE, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x415E06, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x415E12, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x417F25, 2656+12+(DWORD)aniMem);
	SafeWrite32(0x417F30, 2656+12+(DWORD)aniMem);

	//old addr 0x54CC24
	SafeWrite32(0x413C7E, 2656+16+(DWORD)aniMem);
	SafeWrite32(0x413E8A, 2656+16+(DWORD)aniMem);
	SafeWrite32(0x413F17, 2656+16+(DWORD)aniMem);
	SafeWrite32(0x415C24, 2656+16+(DWORD)aniMem);
	SafeWrite32(0x415D16, 2656+16+(DWORD)aniMem);
	SafeWrite32(0x415D44, 2656+16+(DWORD)aniMem);

	//old addr 0x54CC28
	SafeWrite32(0x413C76, 2656+20+(DWORD)aniMem);
	SafeWrite32(0x413E82, 2656+20+(DWORD)aniMem);
	SafeWrite32(0x413F0F, 2656+20+(DWORD)aniMem);
	SafeWrite32(0x415C3E, 2656+20+(DWORD)aniMem);
	SafeWrite32(0x415D0E, 2656+20+(DWORD)aniMem);
	SafeWrite32(0x415D4D, 2656+20+(DWORD)aniMem);

	//old addr 0x54CC38
	SafeWrite32(0x413F29, 2656+36+(DWORD)aniMem);

	//old addr 0x54CC3C
	SafeWrite32(0x413D1C, 2656+40+(DWORD)aniMem);
	SafeWrite32(0x41570D, 2656+40+(DWORD)aniMem);
	SafeWrite32(0x415720, 2656+40+(DWORD)aniMem);

	//old addr 0x54CC48
	SafeWrite32(0x415C35, 2656+52+(DWORD)aniMem);


	//struct array 2///////////////////

	//old addr 0x530014
	SafeWrite32(0x416E4A, (DWORD)aniMem2);
	SafeWrite32(0x416E56, (DWORD)aniMem2);
	SafeWrite32(0x416EBD, (DWORD)aniMem2);
	SafeWrite32(0x416F98, (DWORD)aniMem2);
	SafeWrite32(0x416FAC, (DWORD)aniMem2);
	SafeWrite32(0x4170C5, (DWORD)aniMem2);
	SafeWrite32(0x417167, (DWORD)aniMem2);
	SafeWrite32(0x4171F6, (DWORD)aniMem2);
	SafeWrite32(0x4172A5, (DWORD)aniMem2);
	SafeWrite32(0x417583, (DWORD)aniMem2);
	SafeWrite32(0x417856, (DWORD)aniMem2);
	SafeWrite32(0x4178AE, (DWORD)aniMem2);
	SafeWrite32(0x417937, (DWORD)aniMem2);
	SafeWrite32(0x4179FA, (DWORD)aniMem2);
	SafeWrite32(0x417A86, (DWORD)aniMem2);
	SafeWrite32(0x417BB7, (DWORD)aniMem2);
	SafeWrite32(0x417CD1, (DWORD)aniMem2);
	SafeWrite32(0x417D54, (DWORD)aniMem2);
	SafeWrite32(0x417E14, (DWORD)aniMem2);
	SafeWrite32(0x417E3C, (DWORD)aniMem2);
	SafeWrite32(0x417FB1, (DWORD)aniMem2);
	SafeWrite32(0x417FB7, (DWORD)aniMem2);
	SafeWrite32(0x417FCC, (DWORD)aniMem2);

	//old addr 0x530018
	SafeWrite32(0x415D7D, 4+(DWORD)aniMem2);
	SafeWrite32(0x416E40, 4+(DWORD)aniMem2);
	SafeWrite32(0x416F8F, 4+(DWORD)aniMem2);
	SafeWrite32(0x41738B, 4+(DWORD)aniMem2);
	SafeWrite32(0x417786, 4+(DWORD)aniMem2);
	SafeWrite32(0x4177E7, 4+(DWORD)aniMem2);
	SafeWrite32(0x417983, 4+(DWORD)aniMem2);
	SafeWrite32(0x417AC1, 4+(DWORD)aniMem2);
	SafeWrite32(0x417B70, 4+(DWORD)aniMem2);
	SafeWrite32(0x417C0D, 4+(DWORD)aniMem2);

	//old addr 0x53001C
	SafeWrite32(0x416EB8, 8+(DWORD)aniMem2);
	SafeWrite32(0x416ECB, 8+(DWORD)aniMem2);
	SafeWrite32(0x416FA6, 8+(DWORD)aniMem2);
	SafeWrite32(0x416FFF, 8+(DWORD)aniMem2);
	SafeWrite32(0x41702F, 8+(DWORD)aniMem2);
	SafeWrite32(0x4177F9, 8+(DWORD)aniMem2);
	SafeWrite32(0x417AC7, 8+(DWORD)aniMem2);
	SafeWrite32(0x417ADB, 8+(DWORD)aniMem2);
	SafeWrite32(0x417C63, 8+(DWORD)aniMem2);
	SafeWrite32(0x417CA3, 8+(DWORD)aniMem2);

	//old addr 0x530020
	SafeWrite32(0x416EF8, 12+(DWORD)aniMem2);
	SafeWrite32(0x4173EE, 12+(DWORD)aniMem2);

	//old addr 0x530024
	SafeWrite32(0x416EC3, 16+(DWORD)aniMem2);
	SafeWrite32(0x417035, 16+(DWORD)aniMem2);
	SafeWrite32(0x417AD5, 16+(DWORD)aniMem2);
	SafeWrite32(0x417B7B, 16+(DWORD)aniMem2);
	SafeWrite32(0x417B9A, 16+(DWORD)aniMem2);

	//old addr 0x530028
	SafeWrite32(0x416ED8, 20+(DWORD)aniMem2);
	SafeWrite32(0x417066, 20+(DWORD)aniMem2);
	SafeWrite32(0x417B08, 20+(DWORD)aniMem2);
	SafeWrite32(0x417B88, 20+(DWORD)aniMem2);

	//old addr 0x53002C
	SafeWrite32(0x415BF7, 24+(DWORD)aniMem2);
	SafeWrite32(0x416EEC, 24+(DWORD)aniMem2);
	SafeWrite32(0x41706C, 24+(DWORD)aniMem2);
	SafeWrite32(0x4177AB, 24+(DWORD)aniMem2);
	SafeWrite32(0x4179A4, 24+(DWORD)aniMem2);
	SafeWrite32(0x417ACF, 24+(DWORD)aniMem2);
	SafeWrite32(0x417B94, 24+(DWORD)aniMem2);
	SafeWrite32(0x417C30, 24+(DWORD)aniMem2);
	SafeWrite32(0x417D73, 24+(DWORD)aniMem2);
	SafeWrite32(0x417E78, 24+(DWORD)aniMem2);

	//old addr 0x530030
	SafeWrite32(0x416869, 28+(DWORD)aniMem2);
	SafeWrite32(0x416871, 28+(DWORD)aniMem2);
	SafeWrite32(0x4168B0, 28+(DWORD)aniMem2);
	SafeWrite32(0x4168FC, 28+(DWORD)aniMem2);
	SafeWrite32(0x416942, 28+(DWORD)aniMem2);
	SafeWrite32(0x41694A, 28+(DWORD)aniMem2);
	SafeWrite32(0x416D6D, 28+(DWORD)aniMem2);
	SafeWrite32(0x416D74, 28+(DWORD)aniMem2);
	SafeWrite32(0x416DB2, 28+(DWORD)aniMem2);
	SafeWrite32(0x416DE4, 28+(DWORD)aniMem2);
	SafeWrite32(0x416DEC, 28+(DWORD)aniMem2);
	SafeWrite32(0x416F08, 28+(DWORD)aniMem2);
	SafeWrite32(0x416F36, 28+(DWORD)aniMem2);
	SafeWrite32(0x4170FC, 28+(DWORD)aniMem2);
	SafeWrite32(0x41759D, 28+(DWORD)aniMem2);
	SafeWrite32(0x4176EE, 28+(DWORD)aniMem2);
	SafeWrite32(0x4178A7, 28+(DWORD)aniMem2);
	SafeWrite32(0x41792F, 28+(DWORD)aniMem2);
	SafeWrite32(0x417B1A, 28+(DWORD)aniMem2);
	SafeWrite32(0x417BAE, 28+(DWORD)aniMem2);

	//old addr 0x530034
	SafeWrite32(0x415BFF, 32+(DWORD)aniMem2);
	SafeWrite32(0x415D85, 32+(DWORD)aniMem2);
	SafeWrite32(0x41687B, 32+(DWORD)aniMem2);
	SafeWrite32(0x416D7E, 32+(DWORD)aniMem2);
	SafeWrite32(0x416E7A, 32+(DWORD)aniMem2);
	SafeWrite32(0x416F12, 32+(DWORD)aniMem2);
	SafeWrite32(0x417023, 32+(DWORD)aniMem2);
	SafeWrite32(0x417106, 32+(DWORD)aniMem2);
	SafeWrite32(0x417385, 32+(DWORD)aniMem2);
	SafeWrite32(0x417434, 32+(DWORD)aniMem2);
	SafeWrite32(0x4174BA, 32+(DWORD)aniMem2);
	SafeWrite32(0x4175A7, 32+(DWORD)aniMem2);
	SafeWrite32(0x41760D, 32+(DWORD)aniMem2);
	SafeWrite32(0x4176E7, 32+(DWORD)aniMem2);
	SafeWrite32(0x4176F4, 32+(DWORD)aniMem2);
	SafeWrite32(0x41771E, 32+(DWORD)aniMem2);
	SafeWrite32(0x41779A, 32+(DWORD)aniMem2);
	SafeWrite32(0x4177E1, 32+(DWORD)aniMem2);
	SafeWrite32(0x417806, 32+(DWORD)aniMem2);
	SafeWrite32(0x4178A1, 32+(DWORD)aniMem2);
	SafeWrite32(0x4178B4, 32+(DWORD)aniMem2);
	SafeWrite32(0x41790B, 32+(DWORD)aniMem2);
	SafeWrite32(0x417929, 32+(DWORD)aniMem2);
	SafeWrite32(0x417961, 32+(DWORD)aniMem2);
	SafeWrite32(0x417993, 32+(DWORD)aniMem2);
	SafeWrite32(0x417B0E, 32+(DWORD)aniMem2);
	SafeWrite32(0x417B60, 32+(DWORD)aniMem2);
	SafeWrite32(0x417BF8, 32+(DWORD)aniMem2);
	SafeWrite32(0x417C15, 32+(DWORD)aniMem2);
	SafeWrite32(0x417C21, 32+(DWORD)aniMem2);
	SafeWrite32(0x417C4B, 32+(DWORD)aniMem2);
	SafeWrite32(0x417D79, 32+(DWORD)aniMem2);
	SafeWrite32(0x417E31, 32+(DWORD)aniMem2);
	SafeWrite32(0x417F58, 32+(DWORD)aniMem2);
	SafeWrite32(0x417F81, 32+(DWORD)aniMem2);
	SafeWrite32(0x417FC6, 32+(DWORD)aniMem2);

	//old addr 0x530038
	SafeWrite32(0x4168D7, 36+(DWORD)aniMem2);
	SafeWrite32(0x416914, 36+(DWORD)aniMem2);
	SafeWrite32(0x41691F, 36+(DWORD)aniMem2);
	SafeWrite32(0x416DD9, 36+(DWORD)aniMem2);
	SafeWrite32(0x416EE1, 36+(DWORD)aniMem2);
	SafeWrite32(0x41758F, 36+(DWORD)aniMem2);

	//old addr 0x53003A
	SafeWrite32(0x416903, 38+(DWORD)aniMem2);

	//old addr 0x53003B
	SafeWrite32(0x4168C5, 39+(DWORD)aniMem2);
	SafeWrite32(0x416DBD, 39+(DWORD)aniMem2);

	//old addr 0x53003C
	SafeWrite32(0x4173CE, 40+(DWORD)aniMem2);
	SafeWrite32(0x4174C1, 40+(DWORD)aniMem2);
	SafeWrite32(0x4175F1, 40+(DWORD)aniMem2);
	SafeWrite32(0x417730, 40+(DWORD)aniMem2);

}



void AnimationsAtOnceExit() {
	if(!AniLimitFixActive) return;
	delete[] aniMem;
	delete[] aniMem2;
}

