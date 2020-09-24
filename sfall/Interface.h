#pragma once

void InterfaceInit();
void InterfaceExit();
void Interface_OnGameLoad();
void InterfaceGmouseHandleHook();

long Interface_ActiveInterfaceWID();
WINinfo* Interface_GetWindow(long winType);
