#include "..\main.h"

//#include <vector>
#include "..\FalloutEngine\Fallout2.h"

static char attackerSnd[8];
static char targetSnd[8];

static void __declspec(naked) MsgCopy() {
	__asm {
		mov edi, [esp+0xc];
		pushad;
		cmp eax, VARPTR_target_str;
		jne attacker;
		lea eax, targetSnd;
		jmp end;
attacker:
		lea eax, attackerSnd;
end:
		mov edx, edi;
		mov ebx, 8;
		call FuncOffs::strncpy_;
		popad;
		jmp FuncOffs::strncpy_;
	}
}

static void __declspec(naked) DisplayMsg() {
	__asm {
		pushad;
		cmp edx, VARPTR_target_str;
		jne attacker;
		lea eax, targetSnd;
		jmp end;
attacker:
		lea eax, attackerSnd;
end:
		mov ebx, [eax];
		test bl, bl;
		jz skip;
		call FuncOffs::gsound_play_sfx_file_;
skip:
		popad;
		jmp FuncOffs::text_object_create_;
	}
}

void PlaySfx(const char* name) {
	__asm {
		mov eax, name;
		call FuncOffs::gsound_play_sfx_file_;
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