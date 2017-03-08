#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Sound.h"

namespace sfall
{

static char attackerSnd[8];
static char targetSnd[8];

static void __declspec(naked) MsgCopy() {
	__asm {
		mov edi, [esp+0xc];
		pushad;
		cmp eax, FO_VAR_target_str;
		jne attacker;
		lea eax, targetSnd;
		jmp end;
attacker:
		lea eax, attackerSnd;
end:
		mov edx, edi;
		mov ebx, 8;
		call fo::funcoffs::strncpy_;
		popad;
		jmp fo::funcoffs::strncpy_;
	}
}

static void __declspec(naked) DisplayMsg() {
	__asm {
		pushad;
		cmp edx, FO_VAR_target_str;
		jne attacker;
		lea eax, targetSnd;
		jmp end;
attacker:
		lea eax, attackerSnd;
end:
		mov ebx, [eax];
		test bl, bl;
		jz skip;
		call fo::funcoffs::gsound_play_sfx_file_;
skip:
		popad;
		jmp fo::funcoffs::text_object_create_;
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
}

}
