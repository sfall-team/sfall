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

#include <stdio.h>
#include "FalloutEngine.h"
#include "HeroAppearance.h"
#include "SuperSave.h"

//extern
DWORD LSPageOffset=0;


int LSButtDN=0;


//--------------------------------------
void SavePageOffsets() {

  char SavePath[MAX_PATH];

  char buffer[6];

  strcpy_s(SavePath, MAX_PATH, *(char**)_patches);
  strcat_s(SavePath, MAX_PATH, "savegame\\SLOTDAT.ini");

  _itoa_s(*(DWORD*)_slot_cursor, buffer, 10);
  WritePrivateProfileString("POSITION", "ListNum", buffer, SavePath);
  _itoa_s(LSPageOffset, buffer, 10);
  WritePrivateProfileString("POSITION", "PageOffset", buffer, SavePath);

}


//------------------------------------------
static void __declspec(naked) save_page_offsets(void) {

  __asm {
      //save last slot position values to file
      call SavePageOffsets
      //restore original code
      mov eax, dword ptr ds:[_lsgwin]
      ret
  }

}


//--------------------------------------
void LoadPageOffsets() {

  char LoadPath[MAX_PATH];

  strcpy_s(LoadPath, MAX_PATH, *(char**)_patches);
  strcat_s(LoadPath, MAX_PATH, "savegame\\SLOTDAT.ini");

  *(DWORD*)_slot_cursor=GetPrivateProfileInt("POSITION", "ListNum", 0, LoadPath);
  if(*(DWORD*)_slot_cursor>9)*(DWORD*)_slot_cursor=9;

  LSPageOffset=GetPrivateProfileInt("POSITION", "PageOffset", 0, LoadPath);
  if(LSPageOffset>9990)LSPageOffset=9990;

}


//------------------------------------------
static void __declspec(naked) load_page_offsets(void) {


  __asm {
      //load last slot position values from file
      call LoadPageOffsets
      //restore original code
      mov edx, 0x50A480 // ASCII "SAV"
      ret
  }

}


//------------------------------------------
static void __declspec(naked) create_page_buttons(void) {

  __asm {
     //left button -10
     push 32//ButType
     push 0//? always 0
     push 0//PicDown
     push 0//PicUp
     push 0x14B//ButtUp left button
     push 0x54B//ButtDown
     push 0x500//HovOff
     push -1//HovOn
     push 20//Height
     mov ecx, 24//Width
     mov edx, 100//Xpos
     mov ebx, 56//Ypos
     mov eax, dword ptr ds:[_lsgwin]//WinRef
     call win_register_button_
     //left button -100
     push 32//ButType
     push 0//? always 0
     push 0//PicDown
     push 0//PicUp
     push 0x149//ButtUp PGUP button
     push 0x549//ButtDown
     push 0x500//HovOff
     push -1//HovOn
     push 20//Height
     mov ecx, 24//Width
     mov edx, 68//Xpos
     mov ebx, 56//Ypos
     mov eax, dword ptr ds:[_lsgwin]//WinRef
     call win_register_button_//create button function
     //right button +10
     push 32//ButType
     push 0//? always 0
     push 0//PicDown
     push 0//PicUp
     push 0x14D//ButtUp right button
     push 0x54D//ButtDown
     push 0x500//HovOff
     push -1//HovOn
     push 20//Height
     mov ecx, 24//Width
     mov edx, 216//Xpos
     mov ebx, 56//Ypos
     mov eax, dword ptr ds:[_lsgwin]//WinRef
     call win_register_button_//create button function
     //right button +100
     push 32//ButType
     push 0//? always 0
     push 0//PicDown
     push 0//PicUp
     push 0x151//ButtUp PGDN button
     push 0x551//ButtDown
     push 0x500//HovOff
     push -1//HovOn
     push 20//Height
     mov ecx, 24//Width
     mov edx, 248//Xpos
     mov ebx, 56//Ypos
     mov eax, dword ptr ds:[_lsgwin]//WinRef
     call win_register_button_//create button function
     //Set Number button
     push 32//ButType
     push 0//? always 0
     push 0//PicDown
     push 0//PicUp
     push -1//ButtUp
     push 'p'//ButtDown
     push -1//HovOff
     push -1//HovOn
     push 20//Height
     mov ecx, 60//Width
     mov edx, 140//Xpos
     mov ebx, 56//Ypos
     mov eax, dword ptr ds:[_lsgwin]//WinRef
     call win_register_button_//create button function

     //restore original code
     mov eax, 0x65
     ret
  }
}


//------------------------------------------------------
void SetPageNum() {

  int WinRef=*(DWORD*)_lsgwin;//load/save winref
  if(WinRef==NULL)return;
  WINinfo *SaveLoadWin = GetWinStruct(WinRef);
  if(SaveLoadWin->surface==NULL)return;

  BYTE ConsoleGold = *(BYTE*)_YellowColor;//palette offset stored in mem - text colour

  char TempText[32];
  unsigned int TxtMaxWidth=GetMaxCharWidth()*8;//GetTextWidth(TempText);
  unsigned int TxtWidth=0;

  DWORD NewTick=0, OldTick=0;
  int button=0;
  int exitFlag=0;
  char blip='_';
  char Number[5];
  int numpos=0;

  DWORD tempPageOffset=-1;


  while(!exitFlag) {
     NewTick = GetTickCount();//timer for redraw
     if(OldTick>NewTick)
        OldTick=NewTick;
     if(NewTick-OldTick>166) {//time to draw
        OldTick=NewTick;
     
	 if(blip=='_')blip=' ';
	 else blip='_';
     

	 sprintf_s(TempText, 32, "#%d%c", tempPageOffset/10+1, '_');
     if(tempPageOffset==-1)
		 sprintf_s(TempText, 32, "#%c", '_');
     TxtWidth=GetTextWidth(TempText);

	 sprintf_s(TempText, 32, "#%d%c", tempPageOffset/10+1, blip);
     if(tempPageOffset==-1)
		 sprintf_s(TempText, 32, "#%c", blip);

     //fill over text area with consol black colour
     for(unsigned int y=SaveLoadWin->width*52; y<SaveLoadWin->width*82; y=y+SaveLoadWin->width)
         memset(SaveLoadWin->surface+y+170-TxtMaxWidth/2, 0xCF, TxtMaxWidth);


     PrintText(TempText, ConsoleGold, 170-TxtWidth/2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);
     RedrawWin(WinRef);
	 }

     button=check_buttons();
	 if(button>='0' && button<='9') {
		if(numpos<4) {
		  Number[numpos]=button;
		  Number[numpos+1]='\0';
		  numpos++;
		  if(Number[0]=='0') {
			 numpos=0;
             tempPageOffset=-1;
		  }
		  else 
			 tempPageOffset=(atoi(Number)-1)*10;
		}
		//else exitFlag=-1;
	 }
	 else if(button==0x08 && numpos) {
		 numpos--;
		 Number[numpos]='\0';
		 if(!numpos) tempPageOffset=-1;
		 else tempPageOffset=(atoi(Number)-1)*10;
	 }
	 else if(button==0x0D || button==0x20 || button=='p' || button=='P') exitFlag=-1;//Enter, Space or P Keys
	 else if(button==0x1B)tempPageOffset=-1, exitFlag=-1;//Esc key
  }

  if(tempPageOffset!=-1 && tempPageOffset<=9990)
	  LSPageOffset=tempPageOffset;

  SaveLoadWin=NULL;
}


//------------------------------------------
static void __declspec(naked) check_page_buttons(void) {
/*
0047BD49  |> 3D 48010000            |CMP EAX,148
0047BD4E  |. 75 2E                  |JNZ SHORT fallout2.0047BD7E
0047BD50  |. 8B15 B8935100          |MOV EDX,DWORD PTR DS:[5193B8]
0047BD56  |. 4A                     |DEC EDX
0047BD57  |. 8915 B8935100          |MOV DWORD PTR DS:[5193B8],EDX
0047BD5D  |. 85D2                   |TEST EDX,EDX
0047BD5F  |. 7D 07                  |JGE SHORT fallout2.0047BD68
0047BD61  |. 31C0                   |XOR EAX,EAX
0047BD63  |. A3 B8935100            |MOV DWORD PTR DS:[5193B8],EAX
0047BD68  |> B9 FFFFFFFF            |MOV ECX,-1                          //button pressed exit check
0047BD6D  |. BA 01000000            |MOV EDX,1
0047BD72  |. 898C24 28020000        |MOV DWORD PTR SS:[ESP+228],ECX
0047BD79  |. E9 7B010000            |JMP fallout2.0047BEF9
*/

  __asm {
     cmp eax, 0x14B//left button
  jnz CheckFastLeft
     cmp LSPageOffset, 10
  jl SetRet
     sub LSPageOffset, 10
  jmp SetRet
  CheckFastLeft:
     cmp eax, 0x149//fast left PGUP button
  jnz CheckRight
     cmp LSPageOffset, 100
  jl FirstPage
     sub LSPageOffset, 100
  jmp SetRet
  FirstPage:
     mov LSPageOffset, 0
  jmp SetRet
  CheckRight:
     cmp eax, 0x14D//right button
  jnz CheckFastRight
     cmp LSPageOffset, 9980
  jg SetRet
     add LSPageOffset, 10
  jmp SetRet
  CheckFastRight:
     cmp eax, 0x151//fast right PGDN button
  jnz CheckSetNumber
     cmp LSPageOffset, 9890
  jg LastPage
     add LSPageOffset, 100
  jmp SetRet
LastPage:
     mov LSPageOffset, 9990
  jmp SetRet
  CheckSetNumber:
     cmp eax, 'p'//p button pressed - start SetPageNum func
  jnz CheckSetNumber2
     pushad
     call SetPageNum
	 popad
  jmp SetRet
  CheckSetNumber2:
     cmp eax, 'P'//P button pressed - start SetPageNum func
  jnz CheckButtonDown
     pushad
     call SetPageNum
	 popad
  jmp SetRet
  CheckButtonDown:
     cmp eax, 0x500//button in down state
  jl CheckUp
  SetRet:
     mov LSButtDN, eax
     push esi
     mov esi, 0x47E5D0 //reset page save list func
     call esi
     pop esi
     add dword ptr ds:[esp], 26// set return to button pressed code
  jmp EndFunc
  CheckUp:
     //restore original code
	 cmp eax, 0x148//up button
  EndFunc:
     ret
  }
}


//------------------------------------------
void DrawPageText() {

  int WinRef=*(DWORD*)_lsgwin;//load/save winref
  if(WinRef==NULL)return;
  WINinfo *SaveLoadWin = GetWinStruct(WinRef);
  if(SaveLoadWin->surface==NULL)return;

  //fill over text area with consol black colour
  for(unsigned int y=SaveLoadWin->width*52; y<SaveLoadWin->width*82; y=y+SaveLoadWin->width)
      memset(SaveLoadWin->surface+50+y, 0xCF, 240);


  BYTE ConsoleGreen = *(BYTE*)_GreenColor;//palette offset stored in mem - text colour
  BYTE ConsoleGold = *(BYTE*)_YellowColor;//palette offset stored in mem - text colour
  BYTE Colour = ConsoleGreen;

  char TempText[32];
  sprintf_s(TempText, 32, "[ %d ]", LSPageOffset/10+1);

  unsigned int TxtWidth=GetTextWidth(TempText);
  PrintText(TempText, Colour, 170-TxtWidth/2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

  if(LSButtDN==0x549) Colour=ConsoleGold;
  else Colour = ConsoleGreen;
  strcpy_s(TempText, 12, "<<");
  TxtWidth=GetTextWidth(TempText);
  PrintText(TempText, Colour, 80-TxtWidth/2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

  if(LSButtDN==0x54B) Colour=ConsoleGold;
  else Colour = ConsoleGreen;
  strcpy_s(TempText, 12, "<");
  TxtWidth=GetTextWidth(TempText);
  PrintText(TempText, Colour, 112-TxtWidth/2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);



  if(LSButtDN==0x551) Colour=ConsoleGold;
  else Colour = ConsoleGreen;
  strcpy_s(TempText, 12, ">>");
  TxtWidth=GetTextWidth(TempText);
  PrintText(TempText, Colour, 260-TxtWidth/2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

  if(LSButtDN==0x54D) Colour=ConsoleGold;
  else Colour = ConsoleGreen;
  strcpy_s(TempText, 12, ">");
  TxtWidth=GetTextWidth(TempText);
  PrintText(TempText, Colour, 228-TxtWidth/2, 60, TxtWidth, SaveLoadWin->width, SaveLoadWin->surface);

  SaveLoadWin=NULL;
}


//------------------------------------------
static void __declspec(naked) draw_page_text(void) {

  __asm {
     pushad
	 call DrawPageText
     popad
     //restore original code
     mov ebp, 0x57
     ret
  }
}


//------------------------------------------
//add page num offset when reading and writing various save data files
static void __declspec(naked) AddPageOffset01(void) {

  __asm {
	 mov eax, dword ptr ds:[_slot_cursor]//list position 0-9
	 add eax, LSPageOffset//add page num offset
	 ret
	}
}


//------------------------------------------
//getting info for the 10 currently displayed save slots from save.dats
static void __declspec(naked) AddPageOffset02(void) {

  __asm {
	 pop edx//pop ret addr
	 mov eax, 0x50A514 //ASCII "SAVE.DAT"
	 push eax
	 lea eax, dword ptr ds:[ebx+1]
	 add eax, LSPageOffset//add page num offset
	 push edx//push ret addr
	 ret
	}
}


//------------------------------------------
//printing current 10 slot numbers
static void __declspec(naked) AddPageOffset03(void) {

  __asm {
     inc eax
	 add eax, LSPageOffset//add page num offset
	 mov bl, byte ptr ss:[esp+0x10]//add 4 bytes - func ret addr
	 ret
	}
}



//--------------------------------------------------------------------------
void EnableSuperSaving() {

  //save/load button setup func
  SafeWrite8(0x47D80D, 0xE8);
  SafeWrite32(0x47D80E, (DWORD)&create_page_buttons - 0x47D812);

  //Draw button text
  SafeWrite8(0x47E6E8, 0xE8);
  SafeWrite32(0x47E6E9, (DWORD)&draw_page_text - 0x47E6ED);

  //check save buttons
  SafeWrite8(0x47BD49, 0xE8);
  SafeWrite32(0x47BD4A, (DWORD)&check_page_buttons - 0x47BD4E);

  //check load buttons
  SafeWrite8(0x47CB1C, 0xE8);
  SafeWrite32(0x47CB1D, (DWORD)&check_page_buttons - 0x47CB21);


  //save current page and list positions to file on load/save scrn exit
  SafeWrite8(0x47D828, 0xE8);
  SafeWrite32(0x47D829, (DWORD)&save_page_offsets - 0x47D82D);

  //load saved page and list positions from file
  SafeWrite8(0x47B82B, 0xE8);
  SafeWrite32(0x47B82C, (DWORD)&load_page_offsets - 0x47B830);


  //Add Load/Save page offset to Load/Save folder number/////////////////
  SafeWrite8(0x47B929, 0xE8);
  SafeWrite32(0x47B92A, (DWORD)&AddPageOffset01 - 0x47B92E);

  SafeWrite8(0x47D8DB, 0xE8);
  SafeWrite32(0x47D8DC, (DWORD)&AddPageOffset01 - 0x47D8E0);

  SafeWrite8(0x47D9B0, 0xE8);
  SafeWrite32(0x47D9B1, (DWORD)&AddPageOffset01 - 0x47D9B5);

  SafeWrite8(0x47DA34, 0xE8);
  SafeWrite32(0x47DA35, (DWORD)&AddPageOffset01 - 0x47DA39);

  SafeWrite8(0x47DABF, 0xE8);
  SafeWrite32(0x47DAC0, (DWORD)&AddPageOffset01 - 0x47DAC4);

  SafeWrite8(0x47DB58, 0xE8);
  SafeWrite32(0x47DB59, (DWORD)&AddPageOffset01 - 0x47DB5D);

  SafeWrite8(0x47DBE9, 0xE8);
  SafeWrite32(0x47DBEA, (DWORD)&AddPageOffset01 - 0x47DBEE);

  SafeWrite8(0x47DC9C, 0xE8);
  SafeWrite32(0x47DC9D, (DWORD)&AddPageOffset01 - 0x47DCA1);

  SafeWrite8(0x47EC77, 0xE8);
  SafeWrite32(0x47EC78, (DWORD)&AddPageOffset01 - 0x47EC7C);

  SafeWrite8(0x47F5AB, 0xE8);
  SafeWrite32(0x47F5AC, (DWORD)&AddPageOffset01 - 0x47F5B0);

  SafeWrite8(0x47F694, 0xE8);
  SafeWrite32(0x47F695, (DWORD)&AddPageOffset01 - 0x47F699);

  SafeWrite8(0x47F6EB, 0xE8);
  SafeWrite32(0x47F6EC, (DWORD)&AddPageOffset01 - 0x47F6F0);

  SafeWrite8(0x47F7FB, 0xE8);
  SafeWrite32(0x47F7FC, (DWORD)&AddPageOffset01 - 0x47F800);

  SafeWrite8(0x47F892, 0xE8);
  SafeWrite32(0x47F893, (DWORD)&AddPageOffset01 - 0x47F897);

  SafeWrite8(0x47FB86, 0xE8);
  SafeWrite32(0x47FB87, (DWORD)&AddPageOffset01 - 0x47FB8B);

  SafeWrite8(0x47FC3A, 0xE8);
  SafeWrite32(0x47FC3B, (DWORD)&AddPageOffset01 - 0x47FC3F);

  SafeWrite8(0x47FCF2, 0xE8);
  SafeWrite32(0x47FCF3, (DWORD)&AddPageOffset01 - 0x47FCF7);

  SafeWrite8(0x480117, 0xE8);
  SafeWrite32(0x480118, (DWORD)&AddPageOffset01 - 0x48011C);

  SafeWrite8(0x4801CF, 0xE8);
  SafeWrite32(0x4801D0, (DWORD)&AddPageOffset01 - 0x4801D4);

  SafeWrite8(0x480234, 0xE8);
  SafeWrite32(0x480235, (DWORD)&AddPageOffset01 - 0x480239);

  SafeWrite8(0x480310, 0xE8);
  SafeWrite32(0x480311, (DWORD)&AddPageOffset01 - 0x480315);

  SafeWrite8(0x4803F3, 0xE8);
  SafeWrite32(0x4803F4, (DWORD)&AddPageOffset01 - 0x4803F8);

  SafeWrite8(0x48049F, 0xE8);
  SafeWrite32(0x4804A0, (DWORD)&AddPageOffset01 - 0x4804A4);

  SafeWrite8(0x480512, 0xE8);
  SafeWrite32(0x480513, (DWORD)&AddPageOffset01 - 0x480517);

  SafeWrite8(0x4805F2, 0xE8);
  SafeWrite32(0x4805F3, (DWORD)&AddPageOffset01 - 0x4805F7);

  SafeWrite8(0x480767, 0xE8);
  SafeWrite32(0x480768, (DWORD)&AddPageOffset01 - 0x48076C);

  SafeWrite8(0x4807E6, 0xE8);
  SafeWrite32(0x4807E7, (DWORD)&AddPageOffset01 - 0x4807EB);

  SafeWrite8(0x480839, 0xE8);
  SafeWrite32(0x48083A, (DWORD)&AddPageOffset01 - 0x48083E);

  SafeWrite8(0x4808D3, 0xE8);
  SafeWrite32(0x4808D4, (DWORD)&AddPageOffset01 - 0x4808D8);


  SafeWrite8(0x47E5E1, 0xE8);
  SafeWrite32(0x47E5E2, (DWORD)&AddPageOffset02 - 0x47E5E6);
  SafeWrite16(0x47E5E6, 0x9090);
  SafeWrite8(0x47E5E8, 0x90);


  SafeWrite8(0x47E756, 0xE8);
  SafeWrite32(0x47E757, (DWORD)&AddPageOffset03 - 0x47E75B);

}
