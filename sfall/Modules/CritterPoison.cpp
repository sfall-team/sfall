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
#include "PartyControl.h"

#include "CritterPoison.h"

namespace sfall
{

//static long adjustPoison = -2;

long CritterPoison::adjustPoisonHP_Default = -1;
long CritterPoison::adjustPoisonHP; // default or returned value from HOOK_ADJUSTPOISON

void CritterPoison::SetDefaultAdjustPoisonHP(long value) {
	adjustPoisonHP_Default = adjustPoisonHP = value;
}

void __fastcall sf_critter_adjust_poison(fo::GameObject* critter, long amount) {
	if (amount == 0) return;
	if (amount > 0) {
		amount -= fo::func::stat_level(critter, fo::STAT_poison_resist) * amount / 100;
	} else if (critter->critter.poison == 0) {
		return;
	}
	critter->critter.poison += amount;
	if (critter->critter.poison < 0) {
		critter->critter.poison = 0; // level can't be negative
	} else {
		// set uID for saving queue
		//Objects::SetObjectUniqueID(critter);
		//fo::func::queue_add(10 * (505 - 5 * critter->critter.poison), critter, nullptr, fo::QueueType::poison_event);
	}
}

static __declspec(naked) void critter_adjust_poison_hack() {
	__asm {
		mov  edx, esi;
		mov  ecx, edi;
		jmp  sf_critter_adjust_poison;
	}
}

__declspec(naked) void critter_check_poison_hack() {
	__asm {
		mov  eax, CritterPoison::adjustPoisonHP_Default;
		mov  edx, CritterPoison::adjustPoisonHP;
		mov  CritterPoison::adjustPoisonHP, eax;
		retn;
	}
}

void __fastcall critter_check_poison_fix() {
	if (PartyControl::IsNpcControlled()) {
		// since another critter is being controlled, we can't apply the poison effect to it
		// instead, we add the "poison" event to dude again, which will be triggered when dude returns to the player's control
		fo::func::queue_clear_type(fo::QueueType::poison_event, nullptr);
		fo::GameObject* dude = PartyControl::RealDudeObject();
		fo::func::queue_add(10, dude, nullptr, fo::QueueType::poison_event);
	}
}

static __declspec(naked) void critter_check_poison_hack_fix() {
	using namespace fo;
	using namespace Fields;
	__asm {
		mov  ecx, [eax + protoId]; // critter.pid
		cmp  ecx, PID_Player;
		jnz  notDude;
		retn;
notDude:
		call critter_check_poison_fix;
		or   al, 1; // unset ZF (exit from func)
		retn;
	}
}

__declspec(naked) void critter_adjust_poison_hack_fix() { // can also be called from HOOK_ADJUSTPOISON
	using namespace fo;
	using namespace Fields;
	__asm {
		mov  edx, ds:[FO_VAR_obj_dude];
		mov  ebx, [eax + protoId]; // critter.pid
		mov  ecx, PID_Player;
		retn;
	}
}

static __declspec(naked) void critter_check_rads_hack() {
	using namespace fo;
	using namespace Fields;
	__asm {
		mov  edx, ds:[FO_VAR_obj_dude];
		mov  eax, [eax + protoId]; // critter.pid
		mov  ecx, PID_Player;
		retn;
	}
}

void CritterPoison::init() {
	// Allow changing the poison level for NPCs
	MakeCall(0x42D226, critter_adjust_poison_hack);
	SafeWrite8(0x42D22C, 0xDA); // jmp 0x42D30A

	// Adjust poison damage
	SetDefaultAdjustPoisonHP(*(DWORD*)0x42D332);
	MakeCall(0x42D331, critter_check_poison_hack);

	// Fix/tweak for party control
	MakeCall(0x42D31F, critter_check_poison_hack_fix, 1);
	MakeCall(0x42D21C, critter_adjust_poison_hack_fix, 1);
	SafeWrite8(0x42D223, 0xCB); // cmp eax, edx > cmp ebx, ecx
	// also rads
	MakeCall(0x42D4FE, critter_check_rads_hack, 1);
	SafeWrite8(0x42D505, 0xC8); // cmp eax, edx > cmp eax, ecx
}

}
