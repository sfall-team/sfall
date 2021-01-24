/*
 *    sfall
 *    Copyright (C) 2009, 2010  Mash (Matt Wells, mashw at bigpond dot net dot au)
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

#include "main.h"

#include "Animations.h"
#include "FalloutEngine.h"
#include "LoadGameHook.h"

static const int animRecordSize = sizeof(AnimationSet);
static const int sadSize = 3240;

static int animationLimit = 32;

//pointers to new animation struct arrays
static std::vector<AnimationSet> new_anim_set;
static std::vector<BYTE> new_sad;

static DWORD animSetAddr = FO_VAR_anim_set;
static DWORD sadAddr = FO_VAR_sad;

static const DWORD animPCMove[] = {
	0x416E11, 0x416F64, 0x417143, 0x41725C, 0x4179CC,
};

static const DWORD animMaxCheck[] = {
	0x413A70, 0x413ADD, 0x413BDD, 0x413EB8, 0x413F4E, 0x4186F1,
};

static const DWORD animMaxSizeCheck[] = {
	0x413AA9, 0x413CB7, 0x413DC2, 0x417F3A,
};

static const DWORD fake_anim_set_C[] = {
	0x413AA4, 0x413DBC,
};

static const DWORD anim_set_0[] = {
	0x413B96, 0x413C5A, 0x413CF0, 0x413DE1, 0x413E66, 0x413EF3, 0x413FA2,
	0x414161, 0x4142D3, 0x41449A, 0x41460B, 0x4146FF, 0x414826, 0x41491A,
	0x4149F8, 0x414AD0, 0x414BA4, 0x414C8C, 0x414CF0, 0x414D60, 0x414DD0,
	0x414E48, 0x414EDA, 0x414F5E, 0x414FEE, 0x41505C, 0x4150D0, 0x415158,
	0x4151B8, 0x415286, 0x41535C, 0x4153D0, 0x41544A, 0x4154EC, 0x4155EA,
	0x4156C0, 0x4156D5, 0x4156F2, 0x41572F, 0x41573E, 0x415B1B, 0x415B56,
	0x415BB6, 0x415C7C, 0x415CA3, /*0x415DE4, - conflct with 0x415DE2*/
};

static const DWORD anim_set_4[] = {
	0x413D07, 0x415700, 0x415B6B, 0x415B78, 0x415C2D, 0x415D38, 0x415D56,
	0x415D63, 0x415DCF,
};

static const DWORD anim_set_8[] = {
	0x413C6A, 0x413CA3, 0x413CF6, 0x413E76, 0x413EA4, 0x413F03, 0x413F20,
	0x413F3A, 0x4156EC, 0x415B72, 0x415C18, 0x415C58, 0x415C6D, 0x415DBE,
};

static const DWORD anim_set_C[] = {
	0x413B2A, 0x413B33, 0x413B43, 0x413B54, 0x413B66, 0x413BA2, 0x413BAB,
	0x413BC0, 0x413BCD, 0x413C3C, 0x413C87, 0x413D01, 0x413D10, 0x413D36,
	0x413D53, 0x413DAD, 0x413E93, 0x4155DF, 0x415AE2, 0x415D9A, 0x415DDE,
	0x415E06, 0x415E12, 0x417F25, 0x417F30,
};

static const DWORD anim_set_10[] = {
	0x413C7E, 0x413E8A, 0x413F17, 0x415C24, 0x415D16, 0x415D44,
};

static const DWORD anim_set_14[] = {
	0x413C76, 0x413E82, 0x413F0F, 0x415C3E, 0x415D0E, 0x415D4D,
};

static const DWORD anim_set_28[] = {
 0x413D1C, 0x41570D, 0x415720,
};

static const DWORD sad_0[] = {
	0x416E4A, 0x416E56, 0x416EBD, 0x416F98, 0x416FAC, 0x4170C5, 0x417167,
	0x4171F6, 0x4172A5, 0x417583, 0x417856, 0x4178AE, 0x417937, 0x4179FA,
	0x417A86, 0x417BB7, 0x417CD1, 0x417D54, 0x417E14, 0x417E3C, 0x417FB1,
	0x417FB7, 0x417FCC,
};

static const DWORD sad_4[] = {
	0x415D7D, 0x416E40, 0x416F8F, 0x41738B, 0x417786, 0x4177E7, 0x417983,
	0x417AC1, 0x417B70, 0x417C0D,
};

static const DWORD sad_8[] = {
	0x416EB8, 0x416ECB, 0x416FA6, 0x416FFF, 0x41702F, 0x4177F9, 0x417AC7,
	0x417ADB, 0x417C63, 0x417CA3,
};

static const DWORD sad_C[] = {
	0x416EF8, 0x4173EE,
};

static const DWORD sad_10[] = {
	0x416EC3, 0x417035, 0x417AD5, 0x417B7B, 0x417B9A,
};

static const DWORD sad_14[] = {
	0x416ED8, 0x417066, 0x417B08, 0x417B88,
};

static const DWORD sad_18[] = {
	0x415BF7, 0x416EEC, 0x41706C, 0x4177AB, 0x4179A4, 0x417ACF, 0x417B94,
	0x417C30, 0x417D73, 0x417E78,
};

static const DWORD sad_1C[] = {
	0x416869, 0x416871, 0x4168B0, 0x4168FC, 0x416942, 0x41694A, 0x416D6D,
	0x416D74, 0x416DB2, 0x416DE4, 0x416DEC, 0x416F08, 0x416F36, 0x4170FC,
	0x41759D, 0x4176EE, 0x4178A7, 0x41792F, 0x417B1A, 0x417BAE,
};

static const DWORD sad_20[] = {
	0x415BFF, 0x415D85, 0x41687B, 0x416D7E, 0x416E7A, 0x416F12, 0x417023,
	0x417106, 0x417385, 0x417434, 0x4174BA, 0x4175A7, 0x41760D, 0x4176E7,
	0x4176F4, 0x41771E, 0x41779A, 0x4177E1, 0x417806, 0x4178A1, 0x4178B4,
	0x41790B, 0x417929, 0x417961, 0x417993, 0x417B0E, 0x417B60, 0x417BF8,
	0x417C15, 0x417C21, 0x417C4B, 0x417D79, 0x417E31, 0x417F58, 0x417F81,
	0x417FC6,
};

static const DWORD sad_24[] = {
	0x4168D7, 0x416914, 0x41691F, 0x416DD9, 0x416EE1, 0x41758F,
};

static const DWORD sad_27[] = {
	0x4168C5, 0x416DBD,
};

static const DWORD sad_28[] = {
	0x4173CE, 0x4174C1, 0x4175F1, 0x417730,
};

static DWORD __fastcall AnimCombatFix(TGameObj* src, BYTE combatFlag) {
	DWORD animAddr = animSetAddr;

	if (animationLimit > 32) {
		animAddr += animRecordSize; // include a dummy
	}

	if (combatFlag & 2) { // combat flag is set
		__asm call combat_anim_finished_;
	}
	return animAddr;
}

static void __declspec(naked) anim_set_end_hack() {
	__asm {
		push ecx;
		call AnimCombatFix;
		mov  [eax][esi], ebx;
		pop  ecx;
		xor  dl, dl; // for goto 0x415DF2;
		retn;
	}
}

static DWORD __fastcall CheckSetSad(BYTE openFlag, DWORD valueMul) {
	bool result = false;
	int offset = (sadSize * valueMul) + 32;

	if (*(DWORD*)(sadAddr + offset) == -1000) {
		result = true;
	} else if (!InCombat() && !(openFlag & 1)) {
		*(DWORD*)(sadAddr + offset) = -1000;
		result = true;
	}

	return result;
}

static void __declspec(naked) object_move_hack() {
	static const DWORD object_move_back0 = 0x417611;
	static const DWORD object_move_back1 = 0x417616;
	__asm {
		mov  ecx, ds:[ecx + 0x3C];         // openFlag
		mov  edx, [esp + 0x4C - 0x20];     // valueMul
		call CheckSetSad;
		test eax, eax;
		jz   end;
		jmp  object_move_back0;            // fixed jump
end:
		jmp  object_move_back1;            // default
	}
}

static void __declspec(naked) action_climb_ladder_hook() {
	__asm {
		cmp  word ptr [edi + 0x40], 0xFFFF; // DestTile
		je   skip;
		cmp  dword ptr [edi + 0x3C], 0;     // DestMap
		je   reset;
		push edx;
		mov  edx, ds:[FO_VAR_map_number];
		cmp  dword ptr [edi + 0x3C], edx;
		pop  edx;
		jne  skip;
reset:
		and  al, ~0x4; // reset RB_DONTSTAND flag
skip:
		jmp  register_begin_;
	}
}

static void __declspec(naked) art_alias_fid_hack() {
	static const DWORD art_alias_fid_Ret = 0x419A6D;
	__asm {
		cmp  eax, ANIM_called_shot_pic;
		je   skip;
		cmp  eax, ANIM_charred_body;
		je   skip;
		cmp  eax, ANIM_charred_body_sf;
		je   skip;
		add  esp, 4;
		jmp  art_alias_fid_Ret;
skip:
		retn;
	}
}

void ApplyAnimationsAtOncePatches(signed char aniMax) {
	if (aniMax <= 32) return;

	//allocate memory to store larger animation struct arrays
	new_anim_set.resize(aniMax + 1);
	new_sad.resize(sadSize * (aniMax + 1));

	animSetAddr = reinterpret_cast<DWORD>(new_anim_set.data());
	sadAddr = reinterpret_cast<DWORD>(new_sad.data());

	//set general animation limit check (old 20) aniMax-12 -- +12 reserved for PC movement(4) + other critical animations(8)?
	SafeWrite8(0x413C07, aniMax - 12);

	//PC movement animation limit checks (old 24) aniMax-8 -- +8 reserved for other critical animations?.
	SafeWriteBatch<BYTE>(aniMax - 8, animPCMove);

	//Max animation limit checks (old 32) aniMax
	SafeWriteBatch<BYTE>(aniMax, animMaxCheck);

	//Max animations checks - animation struct size * max num of animations (old 2656*32=84992)
	SafeWriteBatch<DWORD>(animRecordSize * aniMax, animMaxSizeCheck);

	//divert old animation structure list pointers to newly allocated memory

	//struct array 1///////////////////

	//old addr 0x54C1B4
	SafeWrite32(0x413A9E, animSetAddr);

	//old addr 0x54C1C0
	SafeWriteBatch<DWORD>(12 + animSetAddr, fake_anim_set_C);

	//old addr 0x54CC14
	SafeWriteBatch<DWORD>(animRecordSize + animSetAddr, anim_set_0);

	//old addr 0x54CC18
	SafeWriteBatch<DWORD>(animRecordSize + 4 + animSetAddr, anim_set_4);

	//old addr 0x54CC1C
	SafeWriteBatch<DWORD>(animRecordSize + 8 + animSetAddr, anim_set_8);

	//old addr 0x54CC20
	SafeWriteBatch<DWORD>(animRecordSize + 12 + animSetAddr, anim_set_C);

	//old addr 0x54CC24
	SafeWriteBatch<DWORD>(animRecordSize + 16 + animSetAddr, anim_set_10);

	//old addr 0x54CC28
	SafeWriteBatch<DWORD>(animRecordSize + 20 + animSetAddr, anim_set_14);

	//old addr 0x54CC38
	SafeWrite32(0x413F29, animRecordSize + 36 + animSetAddr);

	//old addr 0x54CC3C
	SafeWriteBatch<DWORD>(animRecordSize + 40 + animSetAddr, anim_set_28);

	//old addr 0x54CC48
	SafeWrite32(0x415C35, animRecordSize + 52 + animSetAddr);

	//struct array 2///////////////////

	//old addr 0x530014
	SafeWriteBatch<DWORD>(sadAddr, sad_0);

	//old addr 0x530018
	SafeWriteBatch<DWORD>(4 + sadAddr, sad_4);

	//old addr 0x53001C
	SafeWriteBatch<DWORD>(8 + sadAddr, sad_8);

	//old addr 0x530020
	SafeWriteBatch<DWORD>(12 + sadAddr, sad_C);

	//old addr 0x530024
	SafeWriteBatch<DWORD>(16 + sadAddr, sad_10);

	//old addr 0x530028
	SafeWriteBatch<DWORD>(20 + sadAddr, sad_14);

	//old addr 0x53002C
	SafeWriteBatch<DWORD>(24 + sadAddr, sad_18);

	//old addr 0x530030
	SafeWriteBatch<DWORD>(28 + sadAddr, sad_1C);

	//old addr 0x530034
	SafeWriteBatch<DWORD>(32 + sadAddr, sad_20);

	//old addr 0x530038
	SafeWriteBatch<DWORD>(36 + sadAddr, sad_24);

	//old addr 0x53003A
	SafeWrite32(0x416903, 38 + sadAddr);

	//old addr 0x53003B
	SafeWriteBatch<DWORD>(39 + sadAddr, sad_27);

	//old addr 0x53003C
	SafeWriteBatch<DWORD>(40 + sadAddr, sad_28);
}

void Animations_Init() {
	animationLimit = GetConfigInt("Misc", "AnimationsAtOnceLimit", 32);
	if (animationLimit > 32) {
		if (animationLimit > 127) {
			animationLimit = 127;
		}
		dlog("Applying AnimationsAtOnceLimit patch.", DL_INIT);
		ApplyAnimationsAtOncePatches(animationLimit);
		dlogr(" Done", DL_INIT);
	}
	// Fix for calling anim() functions in combat
	MakeCall(0x415DE2, anim_set_end_hack, 1);

	// Fix crash when the critter goes through a door with animation trigger
	MakeJump(0x41755E, object_move_hack);

	// Fix for the player stuck at "climbing" frame after ladder climbing animation
	HookCall(0x411E1F, action_climb_ladder_hook);

	// Add ANIM_charred_body/ANIM_charred_body_sf animations to art aliases
	MakeCall(0x419A17, art_alias_fid_hack);
}

void Animations_Exit() {
}
