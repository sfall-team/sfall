#include "main.h"

//#include <vector>

static char attackerSnd[8];
static char targetSnd[8];
static const DWORD strncpy_ptr=0x4F014F;
static const DWORD text_object_create=0x4B036C;
static const DWORD gsound_play_sfx_file=0x4519A8;

static void __declspec(naked) MsgCopy() {
	__asm {
		mov edi, [esp+0xc];
		pushad;
		cmp eax, 0x56D518;
		jne attacker;
		lea eax, targetSnd;
		jmp end;
attacker:
		lea eax, attackerSnd;
end:
		mov edx, edi;
		mov ebx, 8;
		call strncpy_ptr;
		popad;
		jmp strncpy_ptr;
	}
}
static void __declspec(naked) DisplayMsg() {
	__asm {
		pushad;
		cmp edx, 0x56D518;
		jne attacker;
		lea eax, targetSnd;
		jmp end;
attacker:
		lea eax, attackerSnd;
end:
		mov ebx, [eax];
		test bl, bl;
		jz skip;
		call gsound_play_sfx_file;
skip:
		popad;
		jmp text_object_create;
	}
}

void PlaySfx(const char* name) {
	__asm {
		mov eax, name;
		call gsound_play_sfx_file;
	}
}

/*struct ChangedReg {

};
std::vector<ChangedReg>* regChanges=0;

void SoundExit() {
	if(regChanges) {

	}
}

static void SetupSoundAcceleration(int level) {
	//RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\ControlSet001\\Hardware Profiles\\Current\\System\\CurrentControlSet\\", 0, 0, 0);
}
*/
void SoundInit() {
	int tmp;
	if(tmp=GetPrivateProfileIntA("Sound", "NumSoundBuffers", 0, ini)) {
		SafeWrite8(0x451129, (BYTE)tmp);
	}

	if(GetPrivateProfileIntA("Sound", "AllowSoundForFloats", 0, ini)) {
		HookCall(0x42B7C7, MsgCopy);
		HookCall(0x42B849, DisplayMsg);
	}

	//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
	if(GetPrivateProfileIntA("Sound", "Test_ForceFloats", 0, ini)) {
		SafeWrite8(0x42B772, 0xeb);
	}

	/*if(tmp=GetPrivateProfileIntA("Sound", "ForceSoundAcceleration", 0, ini)) {
		if(tmp>=1&&tmp<=4) SetupSoundAcceleration(tmp);
	}*/
}