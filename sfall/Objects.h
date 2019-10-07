#pragma once

enum UniqueID : long {
	UID_START = 0x0FFFFFFF, // start at 0x10000000
	UID_END   = 0x7FFFFFFF
};

extern long objUniqueID;

void ObjectsInit();

long __fastcall SetObjectUniqueID(TGameObj* obj);
long __fastcall SetSpecialID(TGameObj* obj);
void SetNewEngineID(TGameObj* obj);

void SetAutoUnjamLockTime(DWORD time);
void RestoreObjUnjamAllLocks();
void LoadProtoAutoMaxLimit();
