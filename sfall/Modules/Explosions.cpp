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

#include "..\main.h"

#include "..\FalloutEngine\Fallout2.h"
#include "..\Logging.h"
#include "..\SimplePatch.h"
#include "ScriptExtender.h"

#include "Explosions.h"

static bool lightingEnabled = false;

static const DWORD ranged_attack_lighting_fix_back = 0x4118F8;

// enable lighting for flying projectile, based on projectile PID data (light intensity & radius)
static void __declspec(naked) ranged_attack_lighting_fix() {
	__asm {
		mov     eax, [esp+40]; // projectile ptr - 1st arg
		mov     ecx, [esp+1Ch]; // protoPtr of projectile
		mov     edx, [ecx+0x0C]; // light radius - 2nd arg
		mov     ebx, [ecx+0x10]; // light intensity - 4th arg
		//mov     ebx, 0x10000; // light intensity - 4th arg
		xor     ecx, ecx; // unknown(0) - 3rd argument
		call    FuncOffs::obj_set_light_;
		jmp     ranged_attack_lighting_fix_back; // jump back
	}
}

// misc functions from the engine..
static const DWORD register_object_play_sfx = 0x41541C;

static const DWORD explosion_effect_hook_back = 0x411AB9;
static DWORD explosion_effect_starting_dir = 0;
static void __declspec(naked) explosion_effect_hook() {
	__asm {
		mov     bl, lightingEnabled
		test    bl, bl
		jz      usevanilla
		xor     ebx, ebx
		call    FuncOffs::register_object_animate_
		jmp     next
usevanilla:
		xor     ebx, ebx
		call    FuncOffs::register_object_animate_and_hide_
next:
		mov     al, lightingEnabled
		test    al, al
		jz      skiplight
		mov     eax, [esp+40]; // projectile ptr - 1st arg
		mov     edx, 0xFFFF0008; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, 0
		call    FuncOffs::register_object_light_;
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
		call    FuncOffs::register_object_funset_

		mov     eax, [esp+24]; // explosion obj ptr
		mov     edx, 0xFFFF0008; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, 0
		call    FuncOffs::register_object_light_;

		mov     eax, [esp+24]
		xor     edx, edx
		mov     ebx, 0
		call    FuncOffs::register_object_animate_

		jmp     explosion_lighting_fix2_back; // jump back
	}
}

DWORD _stdcall LogThis(DWORD value1, DWORD value2, DWORD value3) {
	dlog_f("anim_set_check__light_fix: object 0x%X, something 0x%X, radius 0x%X", DL_MAIN, value1, value2, value3);
	return value1;
}

static const DWORD anim_set_check__light_back = 0x415A4C;
static void __declspec(naked) anim_set_check__light_fix() {
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
		jmp     anim_set_check__light_back; // jump back right to the "obj_set_light" call
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
		call    FuncOffs::register_object_light_;
		mov     eax, esi;
		pop     ebx;
		pop     edx;
		call    FuncOffs::register_object_animate_; // overwritten call
		mov     eax, esi; // projectile ptr - 1st arg
		mov     edx, 0x00010000; // maximum radius + intensity (see anim_set_check__light_fix)
		mov     ebx, -1
		call    FuncOffs::register_object_light_;

		jmp     fire_dance_lighting_back; // jump back
	}
}


static const DWORD explosion_dmg_check_adr[] = {0x411709, 0x4119FC, 0x411C08, 0x4517C1, 0x423BC8};
static const DWORD explosion_art_adr[] = {0x411A19, 0x411A29, 0x411A35, 0x411A3C};
static const DWORD explosion_art_defaults[] = {10, 2, 31, 29};
static const DWORD explosion_radius_grenade = 0x479183;
static const DWORD explosion_radius_rocket  = 0x47918B;

static const size_t numArtChecks = sizeof(explosion_art_adr) / sizeof(explosion_art_adr[0]);
static const size_t numDmgChecks = sizeof(explosion_dmg_check_adr) / sizeof(explosion_dmg_check_adr[0]);

enum MetaruleExplosionsMode {
	EXPL_FORCE_EXPLOSION_PATTERN = 1,
	EXPL_FORCE_EXPLOSION_ART = 2,
	EXPL_FORCE_EXPLOSION_RADIUS = 3,
	EXPL_FORCE_EXPLOSION_DMGTYPE = 4
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
		case EXPL_FORCE_EXPLOSION_ART: {
			for (int i = 0; i < numArtChecks; i++) {
				SafeWrite32(explosion_art_adr[i], (BYTE)arg1);
			}
			break;
		}
		case EXPL_FORCE_EXPLOSION_RADIUS:
			SafeWrite32(explosion_radius_grenade, arg1);
			SafeWrite32(explosion_radius_rocket, arg1);
			break;
		case EXPL_FORCE_EXPLOSION_DMGTYPE: {
			for (int i = 0; i < numDmgChecks; i++) {
				SafeWrite8(explosion_dmg_check_adr[i], (BYTE)arg1);
			}
			break;
		}
	}
	return 0;
}

void ResetExplosionSettings() {
	int i;
	// explosion pattern
	explosion_effect_starting_dir = 0;
	SafeWrite8(0x411B54, 6); // last direction
	// explosion art
	for (i = 0; i < 4; i++) {
		SafeWrite32(explosion_art_adr[i], explosion_art_defaults[i]);
	}
	// explosion radiuses
	SafeWrite32(explosion_radius_grenade, 2);
	SafeWrite32(explosion_radius_rocket, 3);
	// explosion dmgtype
	for (i = 0; i < 4; i++) {
		SafeWrite8(explosion_dmg_check_adr[i], 6);
	}
}

void Explosions::init() {
	MakeCall(0x411AB4, &explosion_effect_hook, true); // required for explosions_metarule

	if (GetPrivateProfileIntA("Misc", "ExplosionsEmitLight", 0, ini)) {
		dlog("Initing Explosion changes.", DL_INIT);
		lightingEnabled = true;
		MakeCall(0x4118E1, &ranged_attack_lighting_fix, true);
		MakeCall(0x410A4A, &fire_dance_lighting_fix1, true);
		MakeCall(0x415A3F, anim_set_check__light_fix, true); // this allows to change light intensity

		dlogr(" Done", DL_INIT);
	}
	
	DWORD tmp;
	tmp = SimplePatch<DWORD>(0x4A2873, "Misc", "Dynamite_DmgMax", 50, 0, 9999);
	SimplePatch<DWORD>(0x4A2878, "Misc", "Dynamite_DmgMin", 30, 0, tmp);
	tmp = SimplePatch<DWORD>(0x4A287F, "Misc", "PlasticExplosive_DmgMax", 80, 0, 9999);
	SimplePatch<DWORD>(0x4A2884, "Misc", "PlasticExplosive_DmgMin", 40, 0, tmp);
}
