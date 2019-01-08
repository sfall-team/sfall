#include "main.h"

//#include <vector>
#include "FalloutEngine.h"

static char attackerSnd[9] = {0};
static char targetSnd[9] = {0}; 

static void __declspec(naked) combatai_msg_hook() {
	__asm {
		mov  edi, [esp + 0xC]; // lip file from msg
		push eax;
		cmp  eax, _target_str;
		jne  attacker;
		lea  eax, targetSnd;
		jmp  skip;
attacker:
		lea  eax, attackerSnd;
skip:
		push edx;
		push ebx;
		mov  edx, edi;
		mov  ebx, 8;
		call strncpy_;
		pop  ebx;
		pop  edx;
		pop  eax;
		jmp  strncpy_;
	}
}

static void __declspec(naked) ai_print_msg_hook() {
	__asm {
		push eax;
		cmp  edx, _target_str;
		jne  attacker;
		lea  eax, targetSnd;
		jmp  skip;
attacker:
		lea  eax, attackerSnd;
skip:
		push ecx;
		mov  ecx, [eax];
		test cl, cl;
		jz   end;
		call gsound_play_sfx_file_;
end:
		pop  ecx;
		pop  eax;
		jmp  text_object_create_;
	}
}

void PlaySfx(const char* name) {
	__asm {
		mov  eax, name;
		call gsound_play_sfx_file_;
	}
}

void SoundInit() {
	if (int sBuff = GetPrivateProfileIntA("Sound", "NumSoundBuffers", 0, ini)) {
		SafeWrite8(0x451129, (BYTE)sBuff);
	}

	if (GetPrivateProfileIntA("Sound", "AllowSoundForFloats", 0, ini)) {
		HookCall(0x42B7C7, combatai_msg_hook); // copy msg
		HookCall(0x42B849, ai_print_msg_hook);

		//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
		if (IsDebug && GetPrivateProfileIntA("Sound", "Test_ForceFloats", 0, ini)) {
			SafeWrite8(0x42B6F5, 0xEB); // bypass chance
		}
	}
}
