/*
 *    sfall
 *    Copyright (C) 2008-2024  The sfall team
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
#include "LoadGameHook.h"

#include "Animations.h"

namespace sfall
{

enum AnimFlags {
	e_Priority  = 0x001,
	e_InCombat  = 0x002,
	e_Reserved  = 0x004,
	e_InUse     = 0x008,
	e_Suspend   = 0x010, // animation not start running in register_end
	e_Clear     = 0x020,
	e_EndAnim   = 0x040,
	e_DontStand = 0x080,
	e_Append    = 0x100, // sfall flag
};

static int animationLimit = 32;
static int lockLimit;

//pointers to new animation struct arrays
static std::vector<fo::AnimationSet> sf_anim_set;
static std::vector<fo::AnimationSad> sf_sad;

static std::vector<int8_t> lockAnimSet;

static fo::AnimationSet* animSet = (fo::AnimationSet*)FO_VAR_anim_set;
static fo::AnimationSad* animSad = (fo::AnimationSad*)FO_VAR_sad;

static const DWORD animPCMove[] = {
	0x416E11, 0x416F64, 0x417143, 0x41725C, 0x4179CC,
};

static const DWORD animMaxCheck[] = {
	0x413A70, 0x413ADD, 0x413BDD, 0x413EB8, 0x413F4E, 0x4186F1,
};

static const DWORD animMaxSizeCheck[] = {
	0x413AA9, 0x413CB7, 0x413DC2, 0x417F3A,
};

static struct AnimSetHack {
	const DWORD anim_set_0[45] = { // curr_anim
		0x413B96, 0x413C5A, 0x413CF0, 0x413DE1, 0x413E66, 0x413EF3, 0x413FA2,
		0x414161, 0x4142D3, 0x41449A, 0x41460B, 0x4146FF, 0x414826, 0x41491A,
		0x4149F8, 0x414AD0, 0x414BA4, 0x414C8C, 0x414CF0, 0x414D60, 0x414DD0,
		0x414E48, 0x414EDA, 0x414F5E, 0x414FEE, 0x41505C, 0x4150D0, 0x415158,
		0x4151B8, 0x415286, 0x41535C, 0x4153D0, 0x41544A, 0x4154EC, 0x4155EA,
		0x4156C0, 0x4156D5, 0x4156F2, 0x41572F, 0x41573E, 0x415B1B, 0x415B56,
		0x415BB6, 0x415C7C, 0x415CA3, /*0x415DE4, - conflict with 0x415DE2*/
	};

	const DWORD anim_set_4[9] = { // counter
		0x413D07, 0x415700, 0x415B6B, 0x415B78, 0x415C2D, 0x415D38, 0x415D56,
		0x415D63, 0x415DCF,
	};

	const DWORD anim_set_8[14] = { // anim_counter
		0x413C6A, 0x413CA3, 0x413CF6, 0x413E76, 0x413EA4, 0x413F03, 0x413F20,
		0x413F3A, 0x4156EC, 0x415B72, 0x415C18, 0x415C58, 0x415C6D, 0x415DBE,
	};

	const DWORD anim_set_C[25] = { // flags
		0x413B2A, 0x413B33, 0x413B43, 0x413B54, 0x413B66, 0x413BA2, 0x413BAB,
		0x413BC0, 0x413BCD, 0x413C3C, 0x413C87, 0x413D01, 0x413D10, 0x413D36,
		0x413D53, 0x413DAD, 0x413E93, 0x4155DF, 0x415AE2, 0x415D9A, 0x415DDE,
		0x415E06, 0x415E12, 0x417F25, 0x417F30,
	};

	const DWORD anim_set_C_shift[2] = { // flags
		0x413AA4, 0x413DBC,
	};

	const DWORD anim_set_10[6] = { // anim_0.animType
		0x413C7E, 0x413F17, 0x415C24, 0x415D16, 0x415D44,
		0x413E8A, /*- conflict with 0x413E88*/
	};

	const DWORD anim_set_14[6] = { // anim_0.source
		0x413C76, 0x413E82, 0x413F0F, 0x415C3E, 0x415D0E, 0x415D4D,
	};

	const DWORD anim_set_28[3] = { // anim_0.delay
		0x413D1C, 0x41570D, 0x415720,
	};
} animSetAddrs;

static struct SadHack {
	const DWORD sad_0[23] = {
		0x416E4A, 0x416E56, 0x416EBD, 0x416F98, 0x416FAC, 0x4170C5, 0x417167,
		0x4171F6, 0x4172A5, 0x417583, 0x417856, 0x4178AE, 0x417937, 0x4179FA,
		0x417A86, 0x417BB7, 0x417CD1, 0x417D54, 0x417E14, 0x417E3C, 0x417FB1,
		0x417FB7, 0x417FCC,
	};

	const DWORD sad_4[10] = {
		0x415D7D, 0x416E40, 0x416F8F, 0x41738B, 0x417786, 0x4177E7, 0x417983,
		0x417AC1, 0x417B70, 0x417C0D,
	};

	const DWORD sad_8[10] = {
		0x416EB8, 0x416ECB, 0x416FA6, 0x416FFF, 0x41702F, 0x4177F9, 0x417AC7,
		0x417ADB, 0x417C63, 0x417CA3,
	};

	const DWORD sad_C[2] = {
		0x416EF8, 0x4173EE,
	};

	const DWORD sad_10[5] = {
		0x416EC3, 0x417035, 0x417AD5, 0x417B7B, 0x417B9A,
	};

	const DWORD sad_14[4] = {
		0x416ED8, 0x417066, 0x417B08, 0x417B88,
	};

	const DWORD sad_18[10] = {
		0x415BF7, 0x416EEC, 0x41706C, 0x4177AB, 0x4179A4, 0x417ACF, 0x417B94,
		0x417C30, 0x417D73, 0x417E78,
	};

	const DWORD sad_1C[20] = {
		0x416869, 0x416871, 0x4168B0, 0x4168FC, 0x416942, 0x41694A, 0x416D6D,
		0x416D74, 0x416DB2, 0x416DE4, 0x416DEC, 0x416F08, 0x416F36, 0x4170FC,
		0x41759D, 0x4176EE, 0x4178A7, 0x41792F, 0x417B1A, 0x417BAE,
	};

	const DWORD sad_20[36] = {
		0x415BFF, 0x415D85, 0x41687B, 0x416D7E, 0x416E7A, 0x416F12, 0x417023,
		0x417106, 0x417385, 0x417434, 0x4174BA, 0x4175A7, 0x41760D, 0x4176E7,
		0x4176F4, 0x41771E, 0x41779A, 0x4177E1, 0x417806, 0x4178A1, 0x4178B4,
		0x41790B, 0x417929, 0x417961, 0x417993, 0x417B0E, 0x417B60, 0x417BF8,
		0x417C15, 0x417C21, 0x417C4B, 0x417D79, 0x417E31, 0x417F58, 0x417F81,
		0x417FC6,
	};

	const DWORD sad_24[6] = {
		0x4168D7, 0x416914, 0x41691F, 0x416DD9, 0x416EE1, 0x41758F,
	};

	const DWORD sad_27[2] = {
		0x4168C5, 0x416DBD,
	};

	const DWORD sad_28[4] = {
		0x4173CE, 0x4174C1, 0x4175F1, 0x417730,
	};
} sadAddrs;

////////////////////////////////////////////////////////////////////////////////

static long appendSlot = -2;

/*
	When the check_registry_ function finds an object for which a new animation
	is being registered in the list of a sequence of already registered
	animations, the engine stops registering animations for this object.

	This implementation will copy all currently registered animations from the
	current slot to another slot with already registered animations, appending
	them to the end of the sequence.

	This allows the engine to continue to animate the object sequentially.
*/
static long __fastcall CopyRegistry(long checkSlot) {
	long currRegSlot = fo::var::curr_anim_set;
	if (!(animSet[currRegSlot].flags & AnimFlags::e_Reserved)) return 0;

	if (currRegSlot == checkSlot) { // this will be bug!
		#ifndef NDEBUG
		BREAKPOINT;
		#endif
		return 0;
	}
	long currRegAnims = fo::var::curr_anim_counter;
	long lastAnim = animSet[checkSlot].totalAnimCount;

	devlog_f("\n\nCopyRegistry: animSet[%d] anims:%d >> animSet[%d] anims:%d", 0, currRegSlot, currRegAnims, checkSlot, lastAnim);
	devlog_f("\n checkSlot: currAnim:%d, counter:%d", 0, animSet[checkSlot].currentAnim, animSet[checkSlot].counter);

	for (long i = animSet[checkSlot].counter; i < lastAnim; i++) {
		long animType = animSet[checkSlot].animations[i].animType;
		devlog_f("\n anim[%d]: animType:%d, animCode:%d", 0, i, animType, animSet[checkSlot].animations[i].animCode);
		if (animType < 4) return 0; // don't add to "move to" types of animations
	}

	long totalRegAnims = (animSet[checkSlot].flags & AnimFlags::e_Append) ? currRegAnims : lastAnim + currRegAnims;
	if (totalRegAnims >= 55) return 0; // not enough slots for animations

	// copy the animations of the registered slot to the slot with the existing animations
	for (long i = 0; i < currRegAnims; i++) {
		devlog_f("\n copy slot: animSet[%d].anim[%d] >> animSet[%d].anim[%d]", 0, currRegSlot, i, checkSlot, lastAnim);
		animSet[checkSlot].animations[lastAnim++] = animSet[currRegSlot].animations[i];
	}
	animSet[checkSlot].flags |= animSet[currRegSlot].flags | AnimFlags::e_Append | AnimFlags::e_Suspend; // set flags
	animSet[currRegSlot].flags = 0;

	fo::var::curr_anim_counter = totalRegAnims; // set the current number of registered animations in the slot
	fo::var::curr_anim_set = checkSlot;         // replace the currently registered slot with a slot with existing registered animations

	appendSlot = checkSlot;
	return lastAnim;
}

static __declspec(naked) void check_registry_hack() {
	__asm {
		mov  eax, animSet;
		cmp  [eax][edx][0x10], 0x0B; // animSet[].anim[].animType == Must_Call
		je   skip;
		//cmp  [eax][edx][0x10], 0x0C; // animSet[].anim[].animType == Call3
		//je   skip;
		test [eax][esi][0x0C], e_EndAnim; // anim_set[].flags
		jnz  skip;
		push ecx;
		push edx;
		call CopyRegistry;
		test eax, eax;
		cmovnz ebx, eax; // set ebx to the current count of animations to check the next anim_set slot
		pop  edx;
		pop  ecx;
		// (re)set ZF
		setz al;
		test al, al;
skip:
		retn;
	}
}

// sets the counter to the index of the anim_# slot with the added animation
static long __fastcall anim_cleanup_sub(long currAnims) {
	//long currAnims = fo::var::curr_anim_counter;
	fo::AnimationSet* set = &animSet[fo::var::curr_anim_set];
	if (set->flags & AnimFlags::e_Append) {
		set->flags ^= AnimFlags::e_Append;
		if (set->currentAnim == -1000 || set->totalAnimCount >= currAnims) return -1;
		return set->totalAnimCount;
	}
	// default
	return (currAnims <= 0) ? -1 : 0;
}

static __declspec(naked) void anim_cleanup_hack() {
	__asm {
		call anim_cleanup_sub;
		mov  ecx, eax; // ecx - anims slot index
		mov  ebx, eax; // ebx - anims counter
		test eax, eax;
		retn;
	}
}

static void __fastcall CheckAppendReg(long, long totalAnims) {
	long slot = appendSlot;
	appendSlot = -2;
	if (slot != fo::var::curr_anim_set) return;

	//long totalAnims = fo::var::curr_anim_counter;
	fo::AnimationSet* set = &animSet[slot];

	#ifndef NDEBUG
	devlog_f("\nregister_end: anim_set: %d, total_anim: %d", 0, slot, totalAnims);
	for (long i = 0; i < totalAnims; i++) {
		devlog_f("\n anim[%d]: animType:%d, animCode:%d, source:%x (%s), callFunc:%x, callFunc3:%x", 0, i,
		         set->animations[i].animType, set->animations[i].animCode,
		         set->animations[i].source, fo::func::critter_name(set->animations[i].source),
		         set->animations[i].callFunc, set->animations[i].callFunc3
		);
	}
	#endif

	for (long i = set->totalAnimCount; i < totalAnims; i++) {
		if (set->animations[i].animType < 4) {
			for (long i = set->totalAnimCount; i < totalAnims; i++) {
				// cleanup
				long frm = set->animations[i].frmPtr;
				if (frm) fo::func::art_ptr_unlock(frm);
				// anim Must_Call
				if (set->animations[i].animType == 11 && (DWORD)set->animations[i].callFunc == (DWORD)fo::funcoffs::gsnd_anim_sound_) {
					long sfx = (long)set->animations[i].target;
					__asm {
						mov  eax, sfx;
						call fo::funcoffs::gsound_delete_sfx_;
					}
				}
			}
			return; // exit, do not set the total number of animations in the slot (the value remains the same)
		}
	}
	set->totalAnimCount = totalAnims;
}

static __declspec(naked) void register_end_hack_begin() {
	__asm {
		mov  edx, ds:[FO_VAR_curr_anim_counter];
		mov  esi, animSet;
		and  [esi][eax][0xC], ~e_InUse;          // animSet[].flags (unset inUse flag)
		test word ptr [esi][eax][0xC], e_Append; // animSet[].flags
		jnz  isAppend;                           // slot with added animation
		retn;
isAppend:
		mov  esi, eax; // keep offset to anim_set slot
		call CheckAppendReg;
		xor  ecx, ecx;
		add  esp, 4;
		mov  edx, 0x413D14;
		mov  eax, esi;
		jmp  edx;
	}
}

////////////////////////////////////////////////////////////////////////////////

static BYTE __fastcall CheckLockAnimSlot(long, long slot) {
	return lockAnimSet[slot];
}

static long __fastcall LockAnimSlot(long slot) {
	if (slot < animationLimit && animSet[slot].counter != animSet[slot].totalAnimCount) {
		lockAnimSet[slot] = 8;
	}
	return slot;
}

static long __fastcall UnlockAnimSlot() {
	long lockCount = 0;
	for (auto it = lockAnimSet.begin(); it != lockAnimSet.end(); ++it) {
		if (*it > 0) {
			(*it)--;
			lockCount++;
		}
	}
	if (lockCount >= lockLimit) fo::func::debug_printf("\n[SFALL] Warning: The number of animated slots in the lock is too large, locked %d of %d", lockCount, animationLimit);
	return 0;
}

static __declspec(naked) void register_end_hack_end() {
	__asm {
//		mov  eax, animSet;
//		test word ptr [eax][esi][0xC], e_Append; // slot with added animation?
//		jz   skip;
//		and  word ptr [eax][esi][0xC], ~(e_Append | e_Suspend); // animSet[].flags (unset flags)
//skip:
		call UnlockAnimSlot;
		pop  esi;
		pop  edx;
		pop  ecx;
		retn;
	}
}

static __declspec(naked) void register_clear_hook() {
	__asm {
		mov  ecx, eax;
		call LockAnimSlot;
		jmp  fo::funcoffs::anim_set_end_;
	}
}

static __declspec(naked) void anim_free_slot_hack() {
	static const DWORD anim_free_slot_next = 0x413BD5;
	using namespace fo;
	__asm {
		pushadc;
		call CheckLockAnimSlot;
		test al, al;
		popadc;
		jnz  isLock;
		test word ptr [esp + 0x10 - 0x10 + 4], RB_UNKNOWN; // command & RB_100_ (register_ping_)
		retn;
isLock:
		add  esp, 4;
		jmp  anim_free_slot_next;
	}
}

static void ClearAllLock() {
	std::fill(lockAnimSet.begin(), lockAnimSet.end(), 0);
}

static __declspec(naked) void anim_stop_hack() {
	__asm {
		call ClearAllLock;
		mov  edx, 1;
		retn;
	}
}

////////////////////////////////////////////////////////////////////////////////
#ifndef NDEBUG
static void __fastcall ClearDataAnimations(long slot) {
	std::memset(animSet[slot].animations, 0, sizeof(fo::AnimationSet) - 16);
}
#endif

static __declspec(naked) void anim_set_end_hack() {
	__asm {
		test dl, e_InCombat; // is combat flag set?
		jz   skip;
		call fo::funcoffs::combat_anim_finished_;
skip:
		mov  eax, animSet;
		mov  [eax][esi], ebx; // anim_set[].curr_anim = -1000
	#ifndef NDEBUG
		mov  ecx, [esp + 0x2C - 0x1C + 4]; // current slot
		call ClearDataAnimations;
	#endif
		retn;
	}
}

static bool __fastcall CheckSetSad(BYTE openFlag, DWORD slot) {
	if (animSad[slot].animStep == -1000) {
		return true;
	} else if (!InCombat() && !(openFlag & 1)) {
		animSad[slot].animStep = -1000;
		return true;
	}
	return false;
}

static __declspec(naked) void object_move_hack() {
	static const DWORD object_move_back0 = 0x417611;
	static const DWORD object_move_back1 = 0x417616;
	__asm {
		mov  ecx, ds:[ecx + 0x3C];         // openFlag
		mov  edx, [esp + 0x4C - 0x20];     // slot (current)
		call CheckSetSad;
		test al, al;
		jz   end;
		jmp  object_move_back0;            // fixed jump
end:
		jmp  object_move_back1;            // default
	}
}

static __declspec(naked) void action_climb_ladder_hook() {
	using namespace fo;
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
		and  al, ~RB_DONTSTAND; // unset flag
skip:
		jmp  fo::funcoffs::register_begin_;
	}
}

static __declspec(naked) void art_alias_fid_hack() {
	static const DWORD art_alias_fid_Ret = 0x419A6D;
	using namespace fo;
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

static __declspec(naked) void obj_use_container_hook() {
	static const DWORD obj_use_container_Ret = 0x49D012;
	static const DWORD obj_use_container_ExitRet = 0x49D069;
	using namespace fo::Fields;
	__asm {
		cmp  dword ptr [ecx + currFrame], 1; // grave type containers in the open state?
		je   skip;                           // skip animation
		jmp  fo::funcoffs::register_begin_;  // vanilla behavior
skip:
		add  esp, 4;
		cmp  edi, ds:[FO_VAR_obj_dude];
		jne  notDude;
		xor  eax, eax;
		jmp  obj_use_container_Ret; // print message
notDude:
		jmp  obj_use_container_ExitRet;
	}
}

// replace vanilla anim_stop_ function in combat_begin_
static void __stdcall combat_begin_anim_stop_hook() {
	fo::var::anim_in_anim_stop = true;
	fo::var::curr_anim_set = -1;

	for (int i = 0; i < animationLimit; i++) {
		const fo::AnimationSet& set = animSet[i];
		if (set.currentAnim >= 1 && set.animations[set.currentAnim - 1].animType == fo::ANIM_TYPE_ANIMATE_FOREVER) continue;
		fo::func::anim_set_end(i);
	}

	fo::var::anim_in_anim_stop = false;
	fo::func::object_anim_compact();
}

static void ApplyAnimationsAtOncePatches(signed char aniMax) {
	//allocate memory to store larger animation struct arrays
	sf_anim_set.resize(aniMax + 1); // include a dummy
	sf_sad.resize(aniMax + 1); // -8?

	//replace addresses for arrays
	animSet = &sf_anim_set[1]; // the zero slot for the game remains unused
	animSad = sf_sad.data();

	//set general animation limit check (old 20) aniMax-12 (4 reserved for PC movement + 8 other critical animations?)
	SafeWriteBatch<BYTE>(aniMax - 12, { 0x413C07 });

	//PC movement animation limit checks (old 24) aniMax-8 (8 reserved for other critical animations?)
	SafeWriteBatch<BYTE>(aniMax - 8, animPCMove);

	//Max animation limit checks (old 32) aniMax
	SafeWriteBatch<BYTE>(aniMax, animMaxCheck);

	//Max animations checks - animation struct size * max num of animations (old 2656*32=84992)
	SafeWriteBatch<DWORD>(sizeof(fo::AnimationSet) * aniMax, animMaxSizeCheck);

	//divert old animation structure list pointers to newly allocated memory

	fo::AnimationSet* animSetAddr = &sf_anim_set[0];

	//old addr 0x54C1B4
	SafeWriteBatch<DWORD>((DWORD)&animSetAddr->currentAnim, { 0x413A9E }); // anim_reset_
	//old addr 0x54C1C0
	SafeWriteBatch<DWORD>((DWORD)&animSetAddr->flags, animSetAddrs.anim_set_C_shift);

	SafeWriteBatch<DWORD>((DWORD)&animSet->currentAnim, animSetAddrs.anim_set_0);
	SafeWriteBatch<DWORD>((DWORD)&animSet->counter, animSetAddrs.anim_set_4);
	SafeWriteBatch<DWORD>((DWORD)&animSet->totalAnimCount, animSetAddrs.anim_set_8);
	SafeWriteBatch<DWORD>((DWORD)&animSet->flags, animSetAddrs.anim_set_C);
	SafeWriteBatch<DWORD>((DWORD)&animSet->animations[0].animType, animSetAddrs.anim_set_10);
	SafeWriteBatch<DWORD>((DWORD)&animSet->animations[0].source, animSetAddrs.anim_set_14);
	SafeWriteBatch<DWORD>((DWORD)&animSet->animations[0].animCode, { 0x413F29 });
	SafeWriteBatch<DWORD>((DWORD)&animSet->animations[0].delay, animSetAddrs.anim_set_28);
	SafeWriteBatch<DWORD>((DWORD)&animSet->animations[0].flags, { 0x415C35 });

	SafeWriteBatch<DWORD>((DWORD)&animSad->flags, sadAddrs.sad_0);
	SafeWriteBatch<DWORD>((DWORD)&animSad->source, sadAddrs.sad_4);
	SafeWriteBatch<DWORD>((DWORD)&animSad->fid, sadAddrs.sad_8);
	SafeWriteBatch<DWORD>((DWORD)&animSad->animCode, sadAddrs.sad_C);
	SafeWriteBatch<DWORD>((DWORD)&animSad->ticks, sadAddrs.sad_10);
	SafeWriteBatch<DWORD>((DWORD)&animSad->tpf, sadAddrs.sad_14);
	SafeWriteBatch<DWORD>((DWORD)&animSad->animSetSlot, sadAddrs.sad_18);
	SafeWriteBatch<DWORD>((DWORD)&animSad->pathCount, sadAddrs.sad_1C);
	SafeWriteBatch<DWORD>((DWORD)&animSad->animStep, sadAddrs.sad_20);
	SafeWriteBatch<DWORD>((DWORD)&animSad->dstTile, sadAddrs.sad_24);
	SafeWriteBatch<DWORD>((DWORD)&animSad->rotation1, { 0x416903 });
	SafeWriteBatch<DWORD>((DWORD)&animSad->rotation2, sadAddrs.sad_27);
	SafeWriteBatch<DWORD>((DWORD)&animSad->pathData[0].tile, sadAddrs.sad_28);
}

void Animations::init() {
	animationLimit = IniReader::GetConfigInt("Misc", "AnimationsAtOnceLimit", sfall::animationLimit);
	if (animationLimit < 32) animationLimit = 32;
	if (animationLimit > 32) {
		if (animationLimit > 127) animationLimit = 127;
		dlogr("Applying AnimationsAtOnceLimit patch.", DL_INIT);
		ApplyAnimationsAtOncePatches(animationLimit);
	}

	// Improved implementation of animation registration
	#ifndef NDEBUG
	MakeCall(0x413E88, check_registry_hack, 2);
	MakeCall(0x413DCE, anim_cleanup_hack, 1);
	SafeWrite16(0x413DD4, 0x4478); // js 0x413E1A
	MakeCall(0x413CE8, register_end_hack_begin, 1);
	SafeWrite16(0x413D0B, 0xC689); // and dl, not 8 > mov esi, eax (keep offset to anim_set slot)
	SafeWrite8(0x413D0D, CodeType::Nop);
	#endif

	// Implement a temporary lock on an animation slot after it is cleared by the register_clear_ function
	// to prevent it from being used as a free slot when registering a nested animation
	MakeJump(0x413D64, register_end_hack_end);
	HookCall(0x413C97, register_clear_hook);
	MakeCall(0x413BB7, anim_free_slot_hack);
	MakeCall(0x4186CF, anim_stop_hack);

	lockAnimSet.resize(animationLimit);
	lockLimit = animationLimit / 2;
	animationLimit--; // this is necessary so as not to block the last slot

	LoadGameHook::OnGameReset() += ClearAllLock;

	// Fix for calling anim() functions in combat
	MakeCall(0x415DE2, anim_set_end_hack, 1);
	SafeWrite8(0x415DEB, CodeType::JumpShort); // jz > jmp

	// Fix crash when the critter goes through a door with animation trigger
	MakeJump(0x41755E, object_move_hack);

	// Allow playing the "magic hands" animation when using an item on an object
	SafeWrite16(0x4120B8, 0x9090); // action_use_an_item_on_object_

	// Fix for the player stuck at "climbing" frame after ladder climbing animation
	HookCall(0x411E1F, action_climb_ladder_hook);

	// Add ANIM_charred_body/ANIM_charred_body_sf animations to art aliases
	MakeCall(0x419A17, art_alias_fid_hack);

	// Fix for grave type containers in the open state not executing the use_p_proc procedure
	if (IniReader::GetConfigInt("Misc", "GraveContainersFix", 0)) {
		dlogr("Applying grave type containers fix.", DL_INIT);
		HookCall(0x49CFAC, obj_use_container_hook);
		SafeWrite16(0x4122D9, 0x9090); // action_get_an_object_
	}

	// Prevent the "forever" type of animation on objects from stopping when entering combat
	HookCall(0x421A48, combat_begin_anim_stop_hook);
}

//void Animations::exit() {
//}

}
