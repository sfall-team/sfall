#pragma once

namespace sfall
{

void InitObjectHookScripts();

void Inject_UseObjOnHook();
void Inject_UseObjHook();

long UseObjOnHook_Invoke(fo::GameObject* source, fo::GameObject* item, fo::GameObject* target);

}
