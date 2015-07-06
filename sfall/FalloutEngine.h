/*
* sfall
* Copyright (C) 2008-2015 The sfall team
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

/* 
* FALLOUT2.EXE structs, function offsets and wrappers should be placed here  
* 
* only place functions and variables here which are likely to be used in more than one module
*
*/

#include "FalloutStructs.h"

extern TGameObj** obj_dude_ptr;
extern TGameObj** inven_dude_ptr;
extern DWORD* activeUIHand_ptr; // 0 - left, 1 - right
extern DWORD* dude_traits; // 2 of them
extern DWORD* itemCurrentItem;
extern DWORD* itemButtonItems;

/*
* HOW TO USE ENGINE FUNCTIONS:
*
* in ASM code, call offsets directly, don't call wrappers as they might not be _stdcall
* in C++ code, use wrappers (add new ones if the don't exist yet)
*
* Note: USE C++! 
* 1) Place thin __declspec(naked) hooks, only use minimum ASM to pass values to/from C++
* 2) Call _stdcall functions from (1), write those entirely in C++ (with little ASM blocks only to call engine functions, when you are too lazy to add wrapper)
*/

extern const DWORD item_get_type_;
extern const DWORD inven_left_hand_; // eax - object
extern const DWORD inven_right_hand_; // eax - object
extern const DWORD proto_ptr_; // eax - PID, edx - int** - pointer to a pointer to a proto struct
extern const DWORD ai_can_use_weapon_; //  (TGameObj *aCritter<eax>, int aWeapon<edx>, int a2Or3<ebx>) returns 1 or 0
extern const DWORD item_w_max_ammo_; // eax - object
extern const DWORD item_w_cur_ammo_; // eax - object


// Interface
extern const DWORD intface_redraw_; // no args
extern const DWORD interface_disable_;
extern const DWORD interface_enable_;
extern const DWORD intface_toggle_items_;
extern const unsigned int display_print_; // eax - char* to display
extern const DWORD intface_item_reload_; // no args
extern const DWORD intface_toggle_item_state_; // no args
extern const DWORD intface_use_item_; // no args

// objects
extern const DWORD isPartyMember_; // (<eax> - object) - bool result
extern const DWORD obj_set_light_; // <eax>(int aObj<eax>, signed int aDist<edx>, int a3<ecx>, int aIntensity<ebx>)
extern const DWORD obj_new_;  // int aObj*<eax>, int aPid<ebx>
extern const DWORD obj_turn_off_;  // int aObj<eax>, int ???<edx>
extern const DWORD obj_move_to_tile_;  // int aObj<eax>, int aTile<edx>, int aElev<ebx>
extern const DWORD obj_find_first_at_tile_; //  <eax>(int elevation<eax>, int tile<edx>)
extern const DWORD obj_find_next_at_tile_; // no args
extern const DWORD critter_is_dead_; // eax - critter
extern const DWORD tile_coord_; // eax - tilenum, edx (int*) - x, ebx (int*) - y

// Animation
extern const DWORD tile_refresh_rect_; // (int elevation<edx>, unkown<ecx>)
extern const DWORD register_object_animate_;  // int aObj<eax>, int aAnim<edx>, int delay<ebx>
extern const DWORD register_object_animate_and_hide_;  // int aObj<eax>, int aAnim<edx>, int delay<ebx>
extern const DWORD register_object_must_erase_;  // int aObj<eax>
extern const DWORD register_object_change_fid_;  // int aObj<eax>, int aFid<edx>, int aDelay<ebx>
extern const DWORD register_object_light_; // <eax>(int aObj<eax>, int aRadius<edx>, int aDelay<ebx>)
extern const DWORD register_object_funset_; // int aObj<eax>, int ???<edx>, int aDelay<ebx>
extern const DWORD register_object_take_out_; // int aObj<eax>, int aHoldFrame<edx> - hold frame ID (1 - spear, 2 - club, etc.)
extern const DWORD register_object_turn_towards_; // int aObj<eax>, int aTile<edx>

extern const DWORD art_exists_; // eax - frameID, used for critter FIDs

extern const DWORD getmsg_; // eax - msg file addr, ebx - message ID, edx - int[4]  - loads string from MSG file preloaded in memory

#define MSG_FILE_COMBAT		(0x56D368)
#define MSG_FILE_AI			(0x56D510)
#define MSG_FILE_SCRNAME	(0x56D754)
#define MSG_FILE_MISC		(0x58E940)
#define MSG_FILE_CUSTOM		(0x58EA98)
#define MSG_FILE_INVENTRY	(0x59E814)
#define MSG_FILE_ITEM		(0x59E980)
#define MSG_FILE_LSGAME		(0x613D28)
#define MSG_FILE_MAP		(0x631D48)
#define MSG_FILE_OPTIONS	(0x6637E8)
#define MSG_FILE_PERK		(0x6642D4)
#define MSG_FILE_PIPBOY		(0x664348)
#define MSG_FILE_QUESTS		(0x664410)
#define MSG_FILE_PROTO		(0x6647FC)
#define MSG_FILE_SCRIPT		(0x667724)
#define MSG_FILE_SKILL		(0x668080)
#define MSG_FILE_SKILLDEX	(0x6680F8)
#define MSG_FILE_STAT		(0x66817C)
#define MSG_FILE_TRAIT		(0x66BE38)
#define MSG_FILE_WORLDMAP	(0x672FB0)

// WRAPPERS:
int _stdcall IsPartyMember(TGameObj* obj);
TGameObj* GetInvenWeaponLeft(TGameObj* obj);
TGameObj* GetInvenWeaponRight(TGameObj* obj);
char* GetProtoPtr(DWORD pid);
char AnimCodeByWeapon(TGameObj* weapon);
void DisplayConsoleMessage(const char* msg);
const char* _stdcall GetMessageStr(DWORD fileAddr, DWORD messageId);
int __stdcall ItemGetType(TGameObj* item);