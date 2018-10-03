#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "Objects.h"

namespace sfall
{

static int unjamTimeState;
static int maxCountProto = 512;

void Objects::SetAutoUnjamLockTime(DWORD time) {
	if (!unjamTimeState) BlockCall(0x4A364A); // disable auto unjam at midnight

	if (time > 0) {
		SafeWrite8(0x4831D9, (BYTE)time);
		if (unjamTimeState == 2) {
			SafeWrite8(0x4831DA, 0x7C);
		}
		unjamTimeState = 1;
	} else {
		SafeWrite8(0x4831DA, 0xEB); // disable auto unjam
		unjamTimeState = 2;
	}
}

void RestoreObjUnjamAllLocks() {
	if (unjamTimeState) {
		SafeWrite8(0x4A364A, 0xE8);
		SafeWrite32(0x4A364B, 0xFFFF9E69);
		SafeWrite8(0x4831DA, 0x7C);
		SafeWrite8(0x4831D9, 24);
		unjamTimeState = 0;
	}
}

static void __declspec(naked) proto_ptr_hack() {
	__asm {
		mov  ecx, maxCountProto;
		cmp  ecx, 4096;
		jae  skip;
		cmp  eax, ecx;
		jb   end;
		add  ecx, 256;
		mov  maxCountProto, ecx;
skip:
		cmp  eax, ecx;
end:
		retn;
	}
}

void Objects::LoadProtoAutoMaxLimit() {
	if (maxCountProto != -1) {
		MakeCall(0x4A21B2, proto_ptr_hack);
	}
}

void Objects::init() {
	LoadGameHook::OnGameReset() += []() {
		RestoreObjUnjamAllLocks();
	};

	int maxlimit = GetConfigInt("Misc", "LoadProtoMaxLimit", -1);
	if (maxlimit != -1) {
		maxCountProto = -1;
		if (maxlimit >= 512) {
			if (maxlimit > 4096) maxlimit = 4096;
			SafeWrite32(0x4A21B3, maxlimit);
		}
	}
}

}
