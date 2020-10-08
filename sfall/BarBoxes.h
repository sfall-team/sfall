#pragma once

void BarBoxesInit();
void BarBoxesExit();
void BarBoxes_OnGameLoad();
long BarBoxes_SetMaxSlots();

int  __stdcall BarBoxes_MaxBox();
void __stdcall BarBoxes_SetText(int box, const char* text, DWORD color);

bool __stdcall BarBoxes_GetBox(int i);
void __stdcall BarBoxes_AddBox(int i);
void __stdcall BarBoxes_RemoveBox(int i);
long __stdcall BarBoxes_AddExtraBox();
