#pragma once

namespace sfall
{

void Interface_Init();
void Interface_Exit();
void Interface_OnGameLoad();
void Interface_OnBeforeGameInit();

long Interface_ActiveInterfaceWID();
fo::Window* Interface_GetWindow(long winType);

}
