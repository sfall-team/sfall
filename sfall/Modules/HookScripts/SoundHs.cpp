#include "..\..\FalloutEngine\Fallout2.h"
#include "..\..\SafeWrite.h"
#include "..\HookScripts.h"
#include "Common.h"

#include "SoundHs.h"

using namespace sfall::script;

namespace sfall
{

static DWORD __fastcall BuildSfxWeaponHook_Script(long effectType, fo::GameObject* weapon, long hitMode, fo::GameObject* target) {
	BeginHook();
	allowNonIntReturn = true;
	argCount = 4;

	args[0] = effectType;
	args[1] = (DWORD)weapon;
	args[2] = (DWORD)hitMode;
	args[3] = (DWORD)target;

	RunHookScript(HOOK_BUILDSFXWEAPON);

	DWORD textPtr = cRet > 0 && retTypes[0] == DataType::STR
	              ? rets[0]
	              : 0;
	EndHook();

	return textPtr; // -1 - default handler
}

static __declspec(naked) void gsnd_build_weapon_sfx_name_hook() {
	__asm {
		pushadc;  // save state
		push ebx;
		push ecx; // target
		push ebx; // hitMode
		mov  ecx, eax; // effectType
		call BuildSfxWeaponHook_Script; // edx - weapon
		test eax, eax; // pointer to text
		pop  ebx; // restore state
		pop  ecx;
		pop  edx;
		jnz  skip;
		pop  eax;
		jmp  fo::funcoffs::gsnd_build_weapon_sfx_name_;
skip:
		add  esp, 4;
		retn;
	}
}


void Inject_BuildSfxWeaponHook() {
	HookCalls(gsnd_build_weapon_sfx_name_hook, {
		0x410DB3, // show_damage_to_object_
		0x411397, 0x411538, // action_melee_
		0x411787, 0x41196C, 0x411A96, 0x411B82, // action_ranged_
		0x4268F5, // combat_attack_this_
		0x42A9B4, 0x42AA92, 0x42AAF0, // ai_try_attack_
		0x42AF4C, // cai_attempt_w_reload_
		0x45BD31, // op_sfx_build_weapon_name_
		0x460B87, // intface_item_reload_
		0x476629, // drop_ammo_into_weapon_
	});
}

void InitSoundHookScripts() {
	HookScripts::LoadHookScript("hs_buildsfxweapon", HOOK_BUILDSFXWEAPON);
}

}
