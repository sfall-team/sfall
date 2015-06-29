/*
 *    sfall
 *    Copyright (C) 2008-2015  The sfall team
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

#include "Logging.h"
#include "FalloutEngine.h"

// global variables
TGameObj** obj_dude_ptr = (TGameObj**)(0x6610B8);
TGameObj** inven_dude_ptr = (TGameObj**)(0x519058);
DWORD* activeUIHand_ptr = (DWORD*)(0x518F78); // 0 - left, 1 - right
DWORD* dude_traits = (DWORD*)(0x66BE40); // 2 of them
DWORD* itemCurrentItem = (DWORD*)(0x518F78); 
DWORD* itemButtonItems = (DWORD*)(0x5970F8);

/**
	ENGINE FUNCTIONS OFFSETS
	const names should end with underscore
*/

// INVENTORY FUNCTIONS
const DWORD inven_left_hand_ = 0x471BBC; // eax - object
const DWORD inven_right_hand_ = 0x471B70; // eax - object

// PROTO FUNCTIONS
const DWORD proto_ptr_ = 0x4A2108; // eax - PID, edx - int** - pointer to a pointer to a proto struct
const DWORD item_get_type_ = 0x477AFC; // eax - TGameObj* item
const DWORD item_w_max_ammo_ = 0x478674; // eax - object
const DWORD item_w_cur_ammo_ = 0x4786A0; // eax - object

// AI FUNCTIONS
const DWORD ai_can_use_weapon_ = 0x4298EC;  // (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0

// UI FUNCTIONS
const DWORD isPartyMember_ = 0x494FC4; // eax - object
const DWORD intface_redraw_ = 0x45EB98;
const DWORD interface_disable_ = 0x45EAFC;
const DWORD interface_enable_ = 0x45EA64;
const DWORD intface_toggle_items_ = 0x45F404;
const unsigned int display_print_ = 0x43186C;
const DWORD intface_item_reload_ = 0x460B20;
const DWORD intface_toggle_item_state_ = 0x45F4E0;
const DWORD intface_use_item_ = 0x45F5EC;

// OBJECTS manipulation
const DWORD obj_set_light_ = 0x48AC90; // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
const DWORD obj_new_ = 0x489A84;  // int aObj*<eax>, int aPid<ebx>
const DWORD obj_turn_off_ = 0x48AE68;  // int aObj<eax>, int ???<edx>
const DWORD obj_move_to_tile_ = 0x48A568;  // int aObj<eax>, int aTile<edx>, int aElev<ebx>

const DWORD obj_find_first_at_tile_ = 0x48B5A8; //  <eax>(int elevation<eax>, int tile<edx>)
const DWORD obj_find_next_at_tile_ = 0x48B608; // no args
const DWORD critter_is_dead_ = 0x42DD18; // eax - critter

// ANIMATION
const DWORD tile_refresh_rect_ = 0x4B12C0; // (int elevation<edx>, unkown<ecx>)
const DWORD register_object_animate_ = 0x4149D0;  // int aObj<eax>, int aAnim<edx>, int delay<ebx>
const DWORD register_object_animate_and_hide_ = 0x414B7C;  // int aObj<eax>, int aAnim<edx>, int delay<ebx>
const DWORD register_object_must_erase_ = 0x414E20;  // int aObj<eax>
const DWORD register_object_change_fid_ = 0x41518C;  // int aObj<eax>, int aFid<edx>, int aDelay<ebx>
const DWORD register_object_light_ = 0x415334; // <eax>(int aObj<eax>, int aRadius<edx>, int aDelay<ebx>)
const DWORD register_object_funset_ = 0x4150A8; // int aObj<eax>, int ???<edx>, int aDelay<ebx> - not really sure what this does
const DWORD register_object_take_out_ = 0x415238; // int aObj<eax>, int aHoldFrame<edx> - hold frame ID (1 - spear, 2 - club, etc.)
const DWORD register_object_turn_towards_ = 0x414C50; // int aObj<eax>, int aTile<edx>

// ART
const DWORD art_exists_ = 0x4198C8; // eax - frameID, used for critter FIDs

// misc 
static const DWORD getmsg_ = 0x48504C; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory

// WRAPPERS
// please, use CamelCase for those


int __stdcall ItemGetType(TGameObj* item) {
	__asm {
		mov eax, item;
		call item_get_type_;
	}
}

int _stdcall IsPartyMember(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call isPartyMember_;
	}
}

TGameObj* GetInvenWeaponLeft(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call inven_left_hand_;
	}
}

TGameObj* GetInvenWeaponRight(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call inven_right_hand_;
	}
}


char* GetProtoPtr(DWORD pid) {
	char* proto;
	__asm {
		mov eax, pid;
		lea edx, proto;
		call proto_ptr_;
	}
	return proto;
}

char AnimCodeByWeapon(TGameObj* weapon) {
	if (weapon != NULL) {
		char* proto = GetProtoPtr(weapon->pid);
		if (proto && *(int*)(proto + 32) == 3) {
			return (char)(*(int*)(proto + 36)); 
		}
	}
	return 0;
}


void DisplayConsoleMessage(const char* msg) {
	__asm {
		mov eax, msg;
		call display_print_;
	}
}

static DWORD mesg_buf[4] = {0, 0, 0, 0};
const char* _stdcall GetMessageStr(DWORD fileAddr, DWORD messageId)
{
	DWORD buf = (DWORD)mesg_buf;
	const char* result;
	__asm {
		mov eax, fileAddr
		mov ebx, messageId
		mov edx, buf
		call getmsg_
		mov result, eax
	}
	return result;
}
