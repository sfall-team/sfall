#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"

#include "Sound.h"

namespace sfall
{

static char attackerSnd[9] = {0};
static char targetSnd[9] = {0}; 

static void __declspec(naked) combatai_msg_hook() {
	__asm {
		mov  edi, [esp + 0xC]; // lip file from msg
		push eax;
		cmp  eax, FO_VAR_target_str;
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
		call fo::funcoffs::strncpy_;
		pop  ebx;
		pop  edx;
		pop  eax;
		jmp  fo::funcoffs::strncpy_;
	}
}

static void __declspec(naked) ai_print_msg_hook() {
	__asm {
		push eax;
		cmp  edx, FO_VAR_target_str;
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
		call fo::funcoffs::gsound_play_sfx_file_;
end:
		pop  ecx;
		pop  eax;
		jmp  fo::funcoffs::text_object_create_;
	}
}

void Sound::init() {
	if (int sBuff = GetConfigInt("Sound", "NumSoundBuffers", 0)) {
		SafeWrite8(0x451129, (BYTE)sBuff);
	}

	if (GetConfigInt("Sound", "AllowSoundForFloats", 0)) {
		HookCall(0x42B7C7, combatai_msg_hook); // copy msg
		HookCall(0x42B849, ai_print_msg_hook);

		//Yes, I did leave this in on purpose. Will be of use to anyone trying to add in the sound effects
		if (isDebug && iniGetInt("Debugging", "Test_ForceFloats", 0, ::sfall::ddrawIni)) {
			SafeWrite8(0x42B6F5, 0xEB); // bypass chance
		}
	}
}

}
