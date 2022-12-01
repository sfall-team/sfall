#pragma once

namespace sfall
{

void InitObjectHookScripts();

void Inject_UseObjOnHook();
void Inject_UseObjHook();
void Inject_UseAnimateObjHook();
void Inject_DescriptionObjHook();
void Inject_SetLightingHook();
void Inject_ScriptProcedureHook();
void Inject_ScriptProcedureHook2();

long UseObjOnHook_Invoke(fo::GameObject* source, fo::GameObject* item, fo::GameObject* target);

}
