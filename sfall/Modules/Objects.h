#pragma once

namespace sfall
{

enum UniqueID : long {
	Start = 0x0FFFFFFF, // start at 0x10000000
	End   = 0x7FFFFFFF
};

class Objects {
public:
	static const char* name() { return "Objects"; }
	static void init();

	static void OnGameLoad();

	static long uniqueID;

	static bool IsUniqueID(long id);

	static long __fastcall SetObjectUniqueID(fo::GameObject* obj);
	static long __fastcall SetSpecialID(fo::GameObject* obj);
	static void SetNewEngineID(fo::GameObject* obj);

	static void __stdcall SetAutoUnjamLockTime(DWORD time);
	static void __stdcall LoadProtoAutoMaxLimit();
};

}
