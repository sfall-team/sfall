/*
*    sfall
*    Copyright (C) 2008-2016  The sfall team
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
*/

#include <algorithm>
#include <vector>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "LoadGameHook.h"

#include "PartyControl.h"

namespace sfall
{

static DWORD Mode;
static int IsControllingNPC = 0;
static std::vector<WORD> Chars;
static int DelayedExperience;

static fo::GameObject* real_dude = nullptr;
static long real_traits[2];
static char real_pc_name[sizeof(fo::var::pc_name)];
static DWORD real_last_level;
static DWORD real_Level;
static DWORD real_Experience;
static char real_free_perk;
static DWORD real_unspent_skill_points;
//static DWORD real_map_elevation;
static DWORD real_sneak_working;
//static DWORD real_sneak_queue_time;
static DWORD real_hand;
static fo::ItemButtonItem real_itemButtonItems[2];
static int real_perkLevelDataList[fo::PERK_count];
//static DWORD real_drug_gvar[6];
//static DWORD real_jet_gvar;
static int real_tag_skill[4];
//static DWORD real_bbox_sneak;

static bool _stdcall IsInPidList(fo::GameObject* obj) {
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

static void __stdcall StatPcAddExperience(int amount) {
	__asm {
		mov eax, amount
		call fo::funcoffs::stat_pc_add_experience_
	}
}

// saves the state of PC before moving control to NPC
static void SaveRealDudeState() {
	real_dude = fo::var::obj_dude;
	real_hand = fo::var::itemCurrentItem;
	memcpy(real_itemButtonItems, fo::var::itemButtonItems, sizeof(fo::ItemButtonItem) * 2);
	memcpy(real_traits, fo::var::pc_trait, sizeof(long) * 2);
	memcpy(real_perkLevelDataList, fo::var::perkLevelDataList, sizeof(DWORD) * fo::PERK_count);
	strcpy_s(real_pc_name, sizeof(fo::var::pc_name), fo::var::pc_name);
	real_Level = fo::var::Level_;
	real_last_level = fo::var::last_level;
	real_Experience = fo::var::Experience_;
	real_free_perk = fo::var::free_perk;
	real_unspent_skill_points = fo::var::curr_pc_stat[0];
	//real_map_elevation = fo::var::map_elevation;
	real_sneak_working = fo::var::sneak_working;
	fo::SkillGetTags(real_tag_skill, 4);
}

// take control of the NPC
static void TakeControlOfNPC(fo::GameObject* npc) {
	// remove skill tags
	int tagSkill[4];
#if (_MSC_VER < 1600)
	std::fill(std_begin(tagSkill), std_end(tagSkill), -1);
#else
	std::fill(std::begin(tagSkill), std::end(tagSkill), -1);
#endif
	fo::SkillSetTags(tagSkill, 4);

	// reset traits
	fo::var::pc_trait[0] = fo::var::pc_trait[1] = -1;

	// reset perks
	for (int i = 0; i < fo::PERK_count; i++) {
		fo::var::perkLevelDataList[i] = 0;
	}

	// change character name
	fo::func::critter_pc_set_name(fo::func::critter_name(npc));

	// change level
	int level = fo::func::isPartyMember(npc) 
		? fo::func::partyMemberGetCurLevel(npc) 
		: 0;

	fo::var::Level_ = level;
	fo::var::last_level = level;

	// reset other stats
	fo::var::Experience_ = 0;
	fo::var::free_perk = 0;
	fo::var::curr_pc_stat[0] = 0;
	fo::var::sneak_working = 0;

	// deduce active hand by weapon anim code
	char critterAnim = (npc->artFid & 0xF000) >> 12; // current weapon as seen in hands
	if (fo::AnimCodeByWeapon(fo::func::inven_left_hand(npc)) == critterAnim) { // definitely left hand..
		fo::var::itemCurrentItem = 0;
	} else {
		fo::var::itemCurrentItem = 1;
	}

	fo::var::inven_pid = npc->pid;

	// switch main dude_obj pointers - this should be done last!
	fo::var::obj_dude = npc;
	fo::var::inven_dude = npc;

	IsControllingNPC = 1;
	DelayedExperience = 0;
	SetInventoryCheck(true);

	fo::func::intface_redraw();
}

// restores the real dude state
static void RestoreRealDudeState() {
	fo::var::obj_dude = real_dude;
	fo::var::inven_dude = real_dude;

	fo::var::itemCurrentItem = real_hand;
	memcpy(fo::var::itemButtonItems, real_itemButtonItems, sizeof(DWORD) * 6 * 2);
	memcpy(fo::var::pc_trait, real_traits, sizeof(long) * 2);
	memcpy(fo::var::perkLevelDataList, real_perkLevelDataList, sizeof(DWORD) * fo::PERK_count);
	strcpy_s(fo::var::pc_name, sizeof(fo::var::pc_name), real_pc_name);
	fo::var::Level_ = real_Level;
	fo::var::last_level = real_last_level;
	fo::var::Experience_ = real_Experience;
	fo::var::free_perk = real_free_perk;
	fo::var::curr_pc_stat[0] = real_unspent_skill_points;
	//real_map_elevation = fo::var::map_elevation; -- why save elevation?
	fo::var::sneak_working = real_sneak_working;
	fo::SkillSetTags(real_tag_skill, 4);

	fo::var::inven_pid = real_dude->pid;

	if (DelayedExperience > 0) {
		StatPcAddExperience(DelayedExperience);
	}

	fo::func::intface_redraw();

	SetInventoryCheck(false);
	IsControllingNPC = 0;
	real_dude = nullptr;
}

static int __stdcall CombatTurn(fo::GameObject* obj) {
	__asm {
		mov eax, obj;
		call fo::funcoffs::combat_turn_;
	}
}

// return values: 0 - use vanilla handler, 1 - skip vanilla handler, return 0 (normal status), -1 - skip vanilla, return -1 (game ended)
static int _stdcall CombatWrapperInner(fo::GameObject* obj) {
	if ((obj != fo::var::obj_dude) && (Chars.size() == 0 || IsInPidList(obj)) && (Mode == 1 || fo::func::isPartyMember(obj))) {
		// save "real" dude state
		SaveRealDudeState();
		TakeControlOfNPC(obj);
		
		// Do combat turn
		int turnResult = CombatTurn(obj);

		// restore state
		if (IsControllingNPC) { // if game was loaded during turn, PartyControlReset() was called and already restored state
			RestoreRealDudeState();
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
		call fo::funcoffs::obj_change_fid_;
		retn;
	}
}

/*
static void _declspec(naked) ItemDropHook() {
	_asm {
		call fo::funcoffs::item_add_force_;
		retn;
	}
}
*/

static void __stdcall DisplayCantDoThat() {
	fo::func::display_print(fo::GetMessageStr(&fo::var::proto_main_msg_file, 675)); // I Can't do that
}

// 1 skip handler, -1 don't skip
int __stdcall PartyControl_SwitchHandHook(fo::GameObject* item) {
	if (fo::func::item_get_type(item) == 3 && IsControllingNPC > 0) {
		int canUse;
		/* check below uses AI packets and skills to check if weapon is usable
		__asm {
			mov edx, item;
			mov eax, obj_dude_ptr;
			mov eax, [eax];
			mov ebx, 2;
			call fo::funcoffs::ai_can_use_weapon_;
			mov canUse, eax;
		}*/
		int fId = (fo::var::obj_dude)->artFid;
		long weaponCode = fo::AnimCodeByWeapon(item);
		fId = (fId & 0xffff0fff) | (weaponCode << 12);
		// check if art with this weapon exists
		__asm {
			mov eax, fId;
			call fo::funcoffs::art_exists_;
			mov canUse, eax;
		}
		if (!canUse) {
			DisplayCantDoThat();
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
		jmp fo::funcoffs::combat_turn_;
	}
}

// hack to exit from this function safely when you load game during NPC turn
static const DWORD CombatHack_add_noncoms_back = 0x422359;
static void _declspec(naked) CombatHack_add_noncoms_() {
	__asm {
		call CombatWrapper_v2;
		cmp eax, -1;
		jne gonormal;
		mov eax, FO_VAR_list_com;
		mov [eax], 0;
		mov ecx, [esp];
gonormal:
		jmp CombatHack_add_noncoms_back;
	}
}

static void __declspec(naked) stat_pc_add_experience_hook() {
	__asm {
		xor  eax, eax
		cmp  IsControllingNPC, eax
		je   skip
		add  DelayedExperience, esi
		retn
skip:
		xchg esi, eax
		jmp  fo::funcoffs::stat_pc_add_experience_
	}
}

// prevents using sneak when controlling NPCs
static void __declspec(naked) pc_flag_toggle_hook() {
	__asm {
		cmp  IsControllingNPC, 0
		je   end
		call DisplayCantDoThat
		retn
end:
		call  fo::funcoffs::pc_flag_toggle_
		retn
	}
}

void __stdcall PartyControlReset() {
	if (real_dude != nullptr && IsControllingNPC > 0) {
		RestoreRealDudeState();
	}
}

bool IsNpcControlled() {
	return IsControllingNPC != 0;
}

void PartyControl::init() {
	Mode = GetConfigInt("Misc", "ControlCombat", 0);
	if (Mode > 2) {
		Mode = 0;
	}
	if (Mode > 0) {
		auto pidList = GetConfigList("Misc", "ControlCombatPIDList", "", 512);
		if (pidList.size() > 0) {
			for (auto &pid : pidList) {
				Chars.push_back(static_cast<WORD>(strtoul(pid.c_str(), 0, 0)));
			}
		}
		dlog_f(" Mode %d, Chars read: %d.", DL_INIT, Mode, Chars.size());

		HookCall(0x46EBEE, &FidChangeHook);

		MakeCall(0x422354, &CombatHack_add_noncoms_, true);
		HookCall(0x422D87, &CombatWrapper_v2);
		HookCall(0x422E20, &CombatWrapper_v2);

		HookCall(0x454218, &stat_pc_add_experience_hook); // call inside op_give_exp_points_hook
		HookCall(0x4124F1, &pc_flag_toggle_hook);
		HookCall(0x41279A, &pc_flag_toggle_hook);

		LoadGameHook::onGameReset += PartyControlReset;
	} else {
		dlog(" Disabled.", DL_INIT);
	}
}

}
