#pragma once

void BarBoxesInit();
void BarBoxesExit();
void ResetBoxes();
void ResetBoxCount();
long SetBoxMaxSlots();

int  __stdcall BarBoxes_MaxBox();
void __stdcall BarBoxes_SetText(int box, const char* text, DWORD color);

bool __stdcall GetBox(int i);
void __stdcall AddBox(int i);
void __stdcall RemoveBox(int i);
long __stdcall AddExtraBox();
