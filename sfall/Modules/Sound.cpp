#include "..\main.h"

//#include <vector>
#include "..\FalloutEngine\Fallout2.h"

#include "Sound.h"

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

void Sound::init() {
	int tmp;
	if (tmp = GetConfigInt("Sound", "NumSoundBuffers", 0)) {
		SafeWrite8(0x451129, (BYTE)tmp);
	}

	if (GetConfigInt("Sound", "AllowSoundForFloats", 0)) {
		HookCall(0x42B7C7, MsgCopy);
		HookCall(0x42B849, DisplayMsg);
	}

	//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
	if (GetConfigInt("Sound", "Test_ForceFloats", 0)) {
		SafeWrite8(0x42B772, 0xeb);
	}

	/*if(tmp=GetPrivateProfileIntA("Sound", "ForceSoundAcceleration", 0, ini)) {
		if(tmp>=1&&tmp<=4) SetupSoundAcceleration(tmp);
	}*/
}
