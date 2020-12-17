#pragma once

void Interface_Init();
void Interface_Exit();
void Interface_OnGameLoad();
void Interface_OnBeforeGameInit();

long Interface_ActiveInterfaceWID();
WINinfo* Interface_GetWindow(long winType);
