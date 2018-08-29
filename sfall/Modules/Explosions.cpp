/*
 *    sfall
 *    Copyright (C) 2008-2014  The sfall team
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "..\Logging.h"
#include "..\SimplePatch.h"
#include "Scripting\Arrays.h"
#include "LoadGameHook.h"
#include "MainLoopHook.h"
#include "ScriptExtender.h"

#include "Explosions.h"

namespace sfall
{

using namespace fo::Fields;

struct explosiveInfo {
	DWORD pid;
	DWORD pidActive;
	DWORD minDamage;
	DWORD maxDamage;
};

std::vector<explosiveInfo> explosives;

static bool onlyOnce = false;
static bool lightingEnabled = false;
static bool explosionsMetaruleReset = false;
static bool explosionsDamageReset = true;

static const DWORD ranged_attack_lighting_fix_back = 0x4118F8;

// enable lighting for flying projectile, based on projectile PID data (light intensity & radius)
static void __declspec(naked) ranged_attack_lighting_fix() {
	__asm {
		mov eax, [esp+40];   // projectile ptr - 1st arg
		mov ecx, [eax+0x70]; // check existing light intensity
		cmp ecx, 0;			 // if non-zero, skip obj_set_light call
		jnz skip;
		mov ecx, [esp+0x1C]; // protoPtr of projectile
		mov edx, [ecx+0x0C]; // light radius - 2nd arg
		mov ebx, [ecx+0x10]; // light intensity - 4th arg
		xor ecx, ecx; // unknown(0) - 3rd argument
		call fo::funcoffs::obj_set_light_;
skip:
		jmp ranged_attack_lighting_fix_back; // jump back
	}
}

// misc functions from the engine..
//static const DWORD register_object_play_sfx = 0x41541C;

static const DWORD explosion_effect_hook_back = 0x411AB9;
static DWORD explosion_effect_starting_dir = 0;
static void __declspec(naked) explosion_effect_hook() {
	__asm {
		mov     bl, lightingEnabled
		test    bl, bl
		jz      usevanilla
		xor     ebx, ebx
		call    fo::funcoffs::register_object_animate_
		jmp     next
usevanilla:
		xor     ebx, ebx
		call    fo::funcoffs::register_object_animate_and_hide_
next:
		mov     al, lightingEnabled
		test    al, al
		jz      skiplight
		mov     eax, [esp+40]; // projectile ptr - 1st arg
		mov     edx, 0xFFFF0008; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, 0
		call    fo::funcoffs::register_object_light_;
skiplight:
		mov     edi, explosion_effect_starting_dir; // starting direction
		jmp     explosion_effect_hook_back; // jump back
	}
}

static const DWORD explosion_lighting_fix2_back = 0x412FC7;
// enable lighting for central explosion object (action_explode)
static void __declspec(naked) explosion_lighting_fix2() {
	__asm {
		mov     eax, [esp+24]
		mov     edx, 1 // turn on
		mov     ebx, 0
		call    fo::funcoffs::register_object_funset_

		mov     eax, [esp+24]; // explosion obj ptr
		mov     edx, 0xFFFF0008; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, 0
		call    fo::funcoffs::register_object_light_;

		mov     eax, [esp+24]
		xor     edx, edx
		mov     ebx, 0
		call    fo::funcoffs::register_object_animate_

		jmp     explosion_lighting_fix2_back; // jump back
	}
}

DWORD _stdcall LogThis(DWORD value1, DWORD value2, DWORD value3) {
	dlog_f("anim_set_check__light_fix: object 0x%X, something 0x%X, radius 0x%X", DL_MAIN, value1, value2, value3);
	return value1;
}

static const DWORD anim_set_check_light_back = 0x415A4C;
static void __declspec(naked) anim_set_check_light_fix() {
	__asm {
		mov     eax, [esi+4] // object
		lea     ecx, [esp+16]  // unknown.. something related to next "tile_refresh_rect" call?
		mov     edx, [esi+12] // radius set in "reg_anim_light"
		mov     ebx, edx
		shr     ebx, 16 // take highest 2 bytes
		test    ebx, ebx
		jz      nothingspecial
		// special case
		and     edx, 0xFF  // use lowest byte as radius, highest 2 bytes as intensity
		inc     ebx
		jmp     end
nothingspecial:
		mov     ebx, [eax+112] // object current light intensity (original behavior)
end:
		jmp     anim_set_check_light_back; // jump back right to the "obj_set_light" call
	}
}

static const DWORD fire_dance_lighting_back = 0x410A4F;
// enable lighting for burning poor guy
static void __declspec(naked) fire_dance_lighting_fix1() {
	__asm {
		push edx;
		push ebx;
		mov     eax, esi; // projectile ptr - 1st arg
		mov     edx, 0xFFFF0002; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, 0
		call    fo::funcoffs::register_object_light_;
		mov     eax, esi;
		pop     ebx;
		pop     edx;
		call    fo::funcoffs::register_object_animate_; // overwritten call
		mov     eax, esi; // projectile ptr - 1st arg
		mov     edx, 0x00010000; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, -1
		call    fo::funcoffs::register_object_light_;

		jmp     fire_dance_lighting_back; // jump back
	}
}

//-----------------------------------------------------------------

static DWORD __fastcall CheckExplosives(DWORD register pid) {
	for (const auto &item: explosives) {
		if (item.pid == pid) return item.pidActive;
	}
	return 0;
}

static DWORD __fastcall CheckActiveExplosives(DWORD register pid) {
	for (const auto &item: explosives) {
		if (item.pidActive == pid) return 0;
	}
	return 1;
}

static DWORD __fastcall GetDamage(DWORD register pid, DWORD &min, DWORD &max) {
	DWORD result = 0;
	for (const auto &item: explosives) {
		if (item.pidActive == pid) {
			min = item.minDamage;
			max = item.maxDamage;
			result = 1;
			break;
		}
	}
	return result;
}

static DWORD __fastcall SetQueueExplosionDamage(DWORD register pid) {
	DWORD min, max;
	DWORD result = GetDamage(pid, min, max);

	if (result) {
		_asm mov edx, min;
		_asm mov ecx, max;
	}

	return result;
}

static void __declspec(naked) obj_use_explosive_hack() {
	using namespace fo;
	__asm {
		cmp  edx, PID_PLASTIC_EXPLOSIVES;
		jz   end;
		// end engine code
		pushad;
		mov  ecx, edx;
		call CheckExplosives;
		test eax, eax;
		popad;
		jnz  end;
		retn;
end:
		mov dword ptr [esp], 0x49BCE0;
		retn;
	}
}

static void __declspec(naked) obj_use_explosive_active_hack() {
	using namespace fo;
	__asm {
		cmp  eax, PID_PLASTIC_EXPLOSIVES;
		jz   end;
		// end engine code
		push ebx;
		push ecx;
		push edx;
		mov  ecx, eax;
		call CheckExplosives;
		pop  edx;
		pop  ecx;
		pop  ebx;
		test eax, eax;
		jz   skipSet;
		mov  dword ptr [esi + protoId], eax; // change item pid to active;
skipSet:
		mov dword ptr [esp], 0x49BD62;
end:
		retn;
	}
}

static void __declspec(naked) queue_do_explosion_hack() {
	using namespace fo;
	__asm {
	    cmp  edx, PID_ACTIVE_DYNAMITE;
		jz   dynamite;
		cmp  edx, PID_ACTIVE_PLASTIC_EXPLOSIVE;
		jz   end;
		push edx;
		mov  ecx, edx;
		call SetQueueExplosionDamage;
		test eax, eax;
		mov  ebx, edx;
		pop  edx;
		jz   end;
		mov  dword ptr [esp], 0x4A2888;
end:
		retn;
dynamite:
		mov  dword ptr [esp], 0x4A2872;
		retn;
	}
}

static void __declspec(naked) inven_action_cursor_drop_hack() {
	using namespace fo;
	__asm {
		cmp  ebx, PID_ACTIVE_PLASTIC_EXPLOSIVE;
		jz   end;
		// end engine code
		push eax;
		push ecx;
		mov  ecx, ebx;
		call CheckActiveExplosives;
		test eax, eax; // check in engine
		pop  ecx;
		pop  eax;
end:
		retn;
	}
}

static void __declspec(naked) protinstTestDroppedExplosive_hack() {
	__asm {
		jz   end;
		mov  ecx, edx;
		call CheckActiveExplosives;
		test eax, eax;
		jz   end;
		mov  dword ptr [esp], 0x49C112; // exit, no active item explosive
end:
		retn;
	}
}

static void apply_hack() {
	onlyOnce = true;
	MakeCall(0x49BCC7, obj_use_explosive_hack);        // check explosives
	MakeCall(0x49BD56, obj_use_explosive_active_hack); // set active explosive
	MakeCall(0x4A2865, queue_do_explosion_hack);       // set damage explosive
	MakeCall(0x4737F2, inven_action_cursor_drop_hack); // check drop explosives
	SafeWrite8(0x4737F7, 0x90);
	MakeCall(0x49C005, protinstTestDroppedExplosive_hack); // check drop explosives
	SafeWrite8(0x49C00A, 0x90);
}

void Explosions::AddToExplosives(DWORD pid, DWORD activePid, DWORD minDmg, DWORD maxDmg) {
	for (unsigned int i = 0; i < explosives.size(); i++) {
		if (explosives[i].pid == pid) {
			explosives.erase(explosives.begin() + i);
			break;
		}
	}
	explosives.push_back({ pid, activePid, minDmg, maxDmg });

	if (!onlyOnce) apply_hack();
}

//----------------------------------------------------------------------------

static const DWORD explosion_dmg_check_adr[] = {0x411709, 0x4119FC, 0x411C08, 0x4517C1, 0x423BC8, 0x42381A};
static const DWORD explosion_art_adr[] = {0x411A19, 0x411A29, 0x411A35, 0x411A3C};
static const DWORD explosion_art_defaults[] = {10, 2, 31, 29};
static const DWORD explosion_radius_grenade = 0x479183;
static const DWORD explosion_radius_rocket  = 0x47918B;

static const DWORD dynamite_min_dmg = 0x4A2878;
static const DWORD dynamite_max_dmg = 0x4A2873;
static const DWORD plastic_min_dmg  = 0x4A2884;
static const DWORD plastic_max_dmg  = 0x4A287F;

static DWORD set_expl_radius_grenade = 2;
static DWORD set_expl_radius_rocket  = 3;

static const size_t numArtChecks = sizeof(explosion_art_adr) / sizeof(explosion_art_adr[0]);
static const size_t numDmgChecks = sizeof(explosion_dmg_check_adr) / sizeof(explosion_dmg_check_adr[0]);

static void SetExplosionRadius(int arg1, int arg2) {
	SafeWrite32(explosion_radius_grenade, arg1);
	SafeWrite32(explosion_radius_rocket, arg2);
}

static void SetExplosionDamage(int pid, int min, int max) {
	explosionsDamageReset = true;
	switch (pid) {
		case fo::ProtoId::PID_DYNAMITE:
			SafeWrite32(dynamite_min_dmg, min);
			SafeWrite32(dynamite_max_dmg, max);
			break;
		case fo::ProtoId::PID_PLASTIC_EXPLOSIVES:
			SafeWrite32(plastic_min_dmg, min);
			SafeWrite32(plastic_max_dmg, max);
			break;
	}
}

static int GetExplosionDamage(int pid) {
	DWORD min = 0, max = 0;
	switch (pid) {
		case fo::ProtoId::PID_DYNAMITE:
			min = *(DWORD*)dynamite_min_dmg;
			max = *(DWORD*)dynamite_max_dmg;
			break;
		case fo::ProtoId::PID_PLASTIC_EXPLOSIVES:
			min = *(DWORD*)plastic_min_dmg;
			max = *(DWORD*)plastic_max_dmg;
			break;
		default:
			GetDamage(pid, min, max);
	}

	DWORD arrayId = script::TempArray(2, 0);
	script::arrays[arrayId].val[0] = min;
	script::arrays[arrayId].val[1] = max;

	return arrayId;
}

enum MetaruleExplosionsMode {
	EXPL_FORCE_EXPLOSION_PATTERN = 1,
	EXPL_FORCE_EXPLOSION_ART = 2,
	EXPL_FORCE_EXPLOSION_RADIUS = 3,
	EXPL_FORCE_EXPLOSION_DMGTYPE = 4,
	EXPL_STATIC_EXPLOSION_RADIUS = 5,
	EXPL_GET_EXPLOSION_DAMAGE = 6,
	EXPL_SET_DYNAMITE_EXPLOSION_DAMAGE = 7,
	EXPL_SET_PLASTIC_EXPLOSION_DAMAGE = 8,
};

int _stdcall ExplosionsMetaruleFunc(int mode, int arg1, int arg2) {
	switch (mode) {
		case EXPL_FORCE_EXPLOSION_PATTERN:
			if (arg1) {
				explosion_effect_starting_dir = 2; // bottom-right
				SafeWrite8(0x411B54, 4); // bottom-left + 1
			} else {
				explosion_effect_starting_dir = 0;
				SafeWrite8(0x411B54, 6); // last direction
			}
			break;
		case EXPL_FORCE_EXPLOSION_ART:
			for (int i = 0; i < numArtChecks; i++) {
				SafeWrite32(explosion_art_adr[i], (BYTE)arg1);
			}
			break;
		case EXPL_FORCE_EXPLOSION_RADIUS:
			SetExplosionRadius(arg1, arg1);
			break;
		case EXPL_FORCE_EXPLOSION_DMGTYPE:
			for (int i = 0; i < numDmgChecks; i++) {
				SafeWrite8(explosion_dmg_check_adr[i], (BYTE)arg1);
			}
			break;
		case EXPL_STATIC_EXPLOSION_RADIUS:
			if (arg1 > 0) set_expl_radius_grenade = arg1;
			if (arg2 > 0) set_expl_radius_rocket = arg2;
			SetExplosionRadius(set_expl_radius_grenade, set_expl_radius_rocket);
			break;
		case EXPL_GET_EXPLOSION_DAMAGE:
			return GetExplosionDamage(arg1);
		case EXPL_SET_DYNAMITE_EXPLOSION_DAMAGE:
			SetExplosionDamage(fo::ProtoId::PID_DYNAMITE, arg1, arg2);
			return 0;
		case EXPL_SET_PLASTIC_EXPLOSION_DAMAGE:
			SetExplosionDamage(fo::ProtoId::PID_PLASTIC_EXPLOSIVES, arg1, arg2);
			return 0;
		default:
			return -1;
	}
	if (mode != EXPL_STATIC_EXPLOSION_RADIUS) explosionsMetaruleReset = true;
	return 0;
}

void ResetExplosionSettings() {
	if (!explosionsMetaruleReset) return;
	// explosion pattern
	explosion_effect_starting_dir = 0;
	SafeWrite8(0x411B54, 6); // last direction
	// explosion art
	for (int i = 0; i < numArtChecks; i++) {
		SafeWrite32(explosion_art_adr[i], explosion_art_defaults[i]);
	}
	// explosion radiuses
	SetExplosionRadius(set_expl_radius_grenade, set_expl_radius_rocket);
	// explosion dmgtype
	for (int i = 0; i < numDmgChecks; i++) {
		SafeWrite8(explosion_dmg_check_adr[i], fo::DamageType::DMG_explosion);
	}
	explosionsMetaruleReset = false;
}

void ResetExplosionRadius() {
	if (set_expl_radius_grenade != 2 || set_expl_radius_rocket != 3)
		SetExplosionRadius(2, 3);
}

static void SetDefaultExplosionDamage() {
	if (!explosives.empty()) explosives.clear();

	if (!explosionsDamageReset) return;
	DWORD tmp;
	tmp = SimplePatch<DWORD>(dynamite_max_dmg, "Misc", "Dynamite_DmgMax", 50, 0, 9999);
	SimplePatch<DWORD>(dynamite_min_dmg, "Misc", "Dynamite_DmgMin", 30, 0, tmp);
	tmp = SimplePatch<DWORD>(plastic_max_dmg, "Misc", "PlasticExplosive_DmgMax", 80, 0, 9999);
	SimplePatch<DWORD>(plastic_min_dmg, "Misc", "PlasticExplosive_DmgMin", 40, 0, tmp);
	explosionsDamageReset = false;
}

void Explosions::init() {
	MakeJump(0x411AB4, explosion_effect_hook); // required for explosions_metarule

	lightingEnabled = GetConfigInt("Misc", "ExplosionsEmitLight", 0) != 0;
	if (lightingEnabled) {
		dlog("Applying Explosion changes.", DL_INIT);
		MakeJump(0x4118E1, ranged_attack_lighting_fix);
		MakeJump(0x410A4A, fire_dance_lighting_fix1);
		MakeJump(0x415A3F, anim_set_check_light_fix); // this allows to change light intensity

		dlogr(" Done", DL_INIT);
	}

	// set damage from ddraw config
	SetDefaultExplosionDamage();

	// after each combat attack, reset metarule_explosions settings
	MainLoopHook::OnAfterCombatAttack() += ResetExplosionSettings;

	LoadGameHook::OnGameReset() += ResetExplosionRadius;
	LoadGameHook::OnGameReset() += SetDefaultExplosionDamage;
}

}
