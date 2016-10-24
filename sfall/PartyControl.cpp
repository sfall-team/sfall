/*
 *    sfall
 *    Copyright (C) 2013  The sfall team
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


/*
	KNOWN ISSUES with party control
	- doesn't work with NPC's wearing armor mod, armor won't change when you change it from critter's inventory
	- shows perks and some other info for the main dude in character screen
*/

#include <vector>

#include "main.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "PartyControl.h"

static DWORD Mode;
static int IsControllingNPC = 0;
static std::vector<WORD> Chars;

static TGameObj* real_dude = NULL;
static DWORD real_traits[2];
static char real_pc_name[32];
static DWORD real_last_level;
static DWORD real_Level;
static DWORD real_Experience;
static DWORD real_free_perk;
static DWORD real_unspent_skill_points;
static DWORD real_map_elevation;
static DWORD real_sneak_working;
static DWORD real_sneak_queue_time;
static DWORD real_hand;
static DWORD real_itemButtonItems[6*2];
static DWORD real_perkLevelDataList[PERK_count];
static DWORD real_drug_gvar[6];
static DWORD real_jet_gvar;
static DWORD real_tag_skill[4];
static DWORD real_bbox_sneak;

static const DWORD* list_com = (DWORD*)_list_com;

static bool _stdcall IsInPidList(TGameObj* obj) {
	int pid = obj->pid & 0xFFFFFF;
	for (std::vector<WORD>::iterator it = Chars.begin(); it != Chars.end(); it++) {
		if (*it == pid) {
			return true;
		}
	}
	return false;
}

static void _stdcall SetInventoryCheck(bool skip) {
	if (skip) {
		SafeWrite16(0x46E7CD, 0x9090); //Inventory check
		SafeWrite32(0x46E7Cf, 0x90909090);
	} else {
		SafeWrite16(0x46E7CD, 0x850F); //Inventory check
		SafeWrite32(0x46E7Cf, 0x4B1);
	}
}

// return values: 0 - use vanilla handler, 1 - skip vanilla handler, return 0 (normal status), -1 - skip vanilla, return -1 (game ended)
static int _stdcall CombatWrapperInner(TGameObj* obj) {
	if ((obj != *ptr_obj_dude) && (Chars.size() == 0 || IsInPidList(obj)) && (Mode == 1 || IsPartyMember(obj))) {
		// transfer control to NPC
		IsControllingNPC = 1;
		SetInventoryCheck(true);
		char dudeWeaponSlot = (char)*ptr_itemCurrentItem;
		// save "real" dude state
		real_dude = *ptr_obj_dude;
		*ptr_obj_dude = obj;
		*ptr_inven_dude = obj;
		memcpy(real_traits, ptr_pc_traits, sizeof(DWORD)*2);

		// deduce active hand by weapon anim code
		char critterAnim = (obj->artFID & 0xF000) >> 12; // current weapon as seen in hands
		if (AnimCodeByWeapon(GetInvenWeaponLeft(obj)) == critterAnim) { // definitely left hand..
			*ptr_itemCurrentItem = 0;
		} else {
			*ptr_itemCurrentItem = 1;
		}
		int turnResult;
		__asm {
			call intface_redraw_;
			mov eax, obj;
			call combat_turn_;
			mov turnResult, eax;
		}
		// restore state
		if (IsControllingNPC) { // if game was loaded during turn, PartyControlReset() was called and already restored state
			*ptr_itemCurrentItem = dudeWeaponSlot;
			memcpy(ptr_pc_traits, real_traits, sizeof(DWORD)*2);
			*ptr_obj_dude = real_dude;
			*ptr_inven_dude = real_dude;
			__asm {
				call intface_redraw_;
			}
			SetInventoryCheck(false);
			IsControllingNPC = 0;
		}
		// -1 means that combat ended during turn
		return (turnResult == -1) ? -1 : 1;
	}
	return 0;
}


// this hook fixes NPCs art switched to main dude art after inventory screen closes
static void _declspec(naked) FidChangeHook() {
	_asm {
		cmp IsControllingNPC, 0;
		je skip;
		push eax;
		mov eax, [eax+0x20]; // current fid
		and eax, 0xffff0fff;
		and edx, 0x0000f000;
		or edx, eax; // only change one octet with weapon type
		pop eax;
skip:
		call obj_change_fid_;
		retn;
	}
}

/*
static void _declspec(naked) ItemDropHook() {
	_asm {
		call item_add_force_;
		retn;
	}
}
*/

// 1 skip handler, -1 don't skip
int __stdcall PartyControl_SwitchHandHook(TGameObj* item) {
	if (ItemGetType(item) == 3 && IsControllingNPC > 0) {
		int canUse;
		/* check below uses AI packets and skills to check if weapon is usable
		__asm {
			mov edx, item;
			mov eax, obj_dude_ptr;
			mov eax, [eax];
			mov ebx, 2;
			call ai_can_use_weapon_;
			mov canUse, eax;
		}*/
		int fId = (*ptr_obj_dude)->artFID;
		char weaponCode = AnimCodeByWeapon(item);
		fId = (fId & 0xffff0fff) | (weaponCode << 12);
		// check if art with this weapon exists
		__asm {
			mov eax, fId;
			call art_exists_;
			mov canUse, eax;
		}
		if (!canUse) {
			DisplayConsoleMessage(GetMessageStr(MSG_FILE_PROTO, 675)); // I can't do that
			return 1;
		}
	}
	return -1;
}


static void _declspec(naked) CombatWrapper_v2() {
	__asm {
		sub esp, 4;
		pushad;
		push eax;
		call CombatWrapperInner;
		mov [esp+32], eax;
		popad;
		add esp, 4;
		cmp [esp-4], 0;
		je gonormal;
		cmp [esp-4], -1;
		je combatend;
		xor eax, eax;
		retn;
combatend:
		mov eax, -1; // don't continue combat, as the game was loaded
		retn;
gonormal:
		jmp combat_turn_;
	}
}

// hack to exit from this function safely when you load game during NPC turn
static const DWORD CombatHack_add_noncoms_back = 0x422359;
static void _declspec(naked) CombatHack_add_noncoms_() {
	__asm {
		call CombatWrapper_v2;
		cmp eax, -1;
		jne gonormal;
		mov eax, list_com;
		mov [eax], 0;
		mov ecx, [esp];
gonormal:
		jmp CombatHack_add_noncoms_back;
	}
}

void PartyControlInit() {
	Mode = GetPrivateProfileIntA("Misc", "ControlCombat", 0, ini);
	if (Mode > 2) 
		Mode = 0;
	if (Mode > 0) {
		char pidbuf[512];
		pidbuf[511] = 0;
		if (GetPrivateProfileStringA("Misc", "ControlCombatPIDList", "", pidbuf, 511, ini)) {
			char* ptr = pidbuf;
			char* comma;
			while (true) {
				comma = strchr(ptr, ',');
				if (!comma) 
					break;
				*comma = 0;
				if (strlen(ptr) > 0)
					Chars.push_back((WORD)strtoul(ptr, 0, 0));
				ptr = comma + 1;
			}
			if (strlen(ptr) > 0)
				Chars.push_back((WORD)strtoul(ptr, 0, 0));
		}
		dlog_f(" Mode %d, Chars read: %d.", DL_INIT, Mode, Chars.size());

		HookCall(0x46EBEE, &FidChangeHook);
		//TODO: Change trait check.

		MakeCall(0x422354, &CombatHack_add_noncoms_, true);
		HookCall(0x422D87, &CombatWrapper_v2);
		HookCall(0x422E20, &CombatWrapper_v2);
	} else
		dlog(" Disabled.", DL_INIT);
}

void __stdcall PartyControlReset() {
	if (real_dude != NULL && IsControllingNPC > 0) {
		memcpy(ptr_pc_traits, real_traits, sizeof(DWORD)*2);
		*ptr_obj_dude = real_dude;
		*ptr_inven_dude = real_dude;
		IsControllingNPC = 0;
	}
	real_dude = NULL;
	SetInventoryCheck(false);
}
