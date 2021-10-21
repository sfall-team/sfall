#pragma once

namespace sfall
{

enum UniqueID : long {
	UID_START = 0x0FFFFFFF, // start at 0x10000000
	UID_END   = 0x7FFFFFFF
};

extern long Objects_uniqueID;

void Objects_Init();
void Objects_OnGameLoad();

bool Objects_IsUniqueID(long id);

long __fastcall Objects_SetObjectUniqueID(fo::GameObject* obj);
long __fastcall Objects_SetSpecialID(fo::GameObject* obj);
void Objects_SetNewEngineID(fo::GameObject* obj);

void __stdcall Objects_SetAutoUnjamLockTime(DWORD time);
void __stdcall Objects_LoadProtoAutoMaxLimit();

}
