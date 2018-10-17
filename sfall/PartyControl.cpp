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
*/

#include <algorithm>
#include <vector>

#include "main.h"
#include "Define.h"
#include "FalloutEngine.h"
#include "HeroAppearance.h"
#include "PartyControl.h"
#if (_MSC_VER < 1600)
#include "Cpp11_emu.h"
#endif

static DWORD Mode;
static int IsControllingNPC = 0;
static std::vector<WORD> Chars;
static int DelayedExperience;

static TGameObj* real_dude = nullptr;
static DWORD real_traits[2];
static char real_pc_name[32];
static DWORD real_last_level;
static DWORD real_Level;
static DWORD real_Experience;
static char real_free_perk;
static DWORD real_unspent_skill_points;
//static DWORD real_map_elevation;
static DWORD real_sneak_working;
//static DWORD real_sneak_queue_time;
static DWORD real_hand;
static DWORD real_itemButtonItems[6*2];
static DWORD real_perkLevelDataList[PERK_count];
//static DWORD real_drug_gvar[6];
//static DWORD real_jet_gvar;
static int real_tag_skill[4];
//static DWORD real_bbox_sneak;

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

static void __stdcall StatPcAddExperience(int amount) {
	__asm {
		mov eax, amount
		call stat_pc_add_experience_
	}
}

// saves the state of PC before moving control to NPC
static void SaveRealDudeState() {
	real_dude = *ptr_obj_dude;
	real_hand = *ptr_itemCurrentItem;
	memcpy(real_itemButtonItems, ptr_itemButtonItems, sizeof(DWORD) * 6 * 2);
	memcpy(real_traits, ptr_pc_traits, sizeof(DWORD) * 2);
	memcpy(real_perkLevelDataList, *ptr_perkLevelDataList, sizeof(DWORD) * PERK_count);
	strcpy_s(real_pc_name, 32, ptr_pc_name);
	real_Level = *ptr_Level_;
	real_last_level = *ptr_last_level;
	real_Experience = *ptr_Experience_;
	real_free_perk = *ptr_free_perk;
	real_unspent_skill_points = ptr_curr_pc_stat[0];
	//real_map_elevation = *ptr_map_elevation;
	real_sneak_working = *ptr_sneak_working;
	SkillGetTags(real_tag_skill, 4);
}

// take control of the NPC
static void TakeControlOfNPC(TGameObj* npc) {
	// remove skill tags
	int tagSkill[4];
#if (_MSC_VER < 1600)
	std::fill(std_begin(tagSkill), std_end(tagSkill), -1);
#else
	std::fill(std::begin(tagSkill), std::end(tagSkill), -1);
#endif
	SkillSetTags(tagSkill, 4);

	// reset traits
	ptr_pc_traits[0] = ptr_pc_traits[1] = -1;

	// reset perks
	for (int i = 0; i < PERK_count; i++) {
		(*ptr_perkLevelDataList)[i] = 0;
	}

	// change character name
	CritterPcSetName(CritterName(npc));

	// change level
	int level = IsPartyMember(npc) 
		? PartyMemberGetCurrentLevel(npc) 
		: 0;

	*ptr_Level_ = level;
	*ptr_last_level = level;

	// reset other stats
	*ptr_Experience_ = 0;
	*ptr_free_perk = 0;
	ptr_curr_pc_stat[0] = 0;
	*ptr_sneak_working = 0;

	// deduce active hand by weapon anim code
	char critterAnim = (npc->artFID & 0xF000) >> 12; // current weapon as seen in hands
	if (AnimCodeByWeapon(InvenLeftHand(npc)) == critterAnim) { // definitely left hand..
		*ptr_itemCurrentItem = 0;
	} else {
		*ptr_itemCurrentItem = 1;
	}

	*ptr_inven_pid = npc->pid;

	// switch main dude_obj pointers - this should be done last!
	*ptr_obj_dude = npc;
	*ptr_inven_dude = npc;

	IsControllingNPC = 1;
	DelayedExperience = 0;
	SetInventoryCheck(true);

	InterfaceRedraw();
}

// restores the real dude state
static void RestoreRealDudeState() {
	*ptr_obj_dude = real_dude;
	*ptr_inven_dude = real_dude;

	*ptr_itemCurrentItem = real_hand;
	memcpy(ptr_itemButtonItems, real_itemButtonItems, sizeof(DWORD) * 6 * 2);
	memcpy(ptr_pc_traits, real_traits, sizeof(DWORD) * 2);
	memcpy(*ptr_perkLevelDataList, real_perkLevelDataList, sizeof(DWORD) * PERK_count);
	strcpy_s(ptr_pc_name, 32, real_pc_name);
	*ptr_Level_ = real_Level;
	*ptr_last_level = real_last_level;
	*ptr_Experience_ = real_Experience;
	*ptr_free_perk = real_free_perk;
	ptr_curr_pc_stat[0] = real_unspent_skill_points;
	//real_map_elevation = *ptr_map_elevation; -- why save elevation?
	*ptr_sneak_working = real_sneak_working;
	SkillSetTags(real_tag_skill, 4);

	*ptr_inven_pid = real_dude->pid;

	if (DelayedExperience > 0) {
		StatPcAddExperience(DelayedExperience);
	}

	InterfaceRedraw();

	SetInventoryCheck(false);
	IsControllingNPC = 0;
	real_dude = nullptr;
}

static int __stdcall CombatTurn(TGameObj* obj) {
	__asm {
		mov eax, obj;
		call combat_turn_;
	}
}

// return values: 0 - use vanilla handler, 1 - skip vanilla handler, return 0 (normal status), -1 - skip vanilla, return -1 (game ended)
static int _stdcall CombatWrapperInner(TGameObj* obj) {
	if ((obj != *ptr_obj_dude) && (Chars.size() == 0 || IsInPidList(obj)) && (Mode == 1 || IsPartyMember(obj))) {
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

static void __stdcall DisplayCantDoThat() {
	DisplayConsoleMessage(GetMessageStr(MSG_FILE_PROTO, 675)); // I Can't do that
}

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

static void __declspec(naked) stat_pc_add_experience_hook() {
	__asm {
		xor  eax, eax
		cmp  IsControllingNPC, eax
		je   skip
		add  DelayedExperience, esi
		retn
skip:
		xchg esi, eax
		jmp  stat_pc_add_experience_
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
		call  pc_flag_toggle_
		retn
	}
}

static char levelMsg[12], armorClassMsg[12], addictMsg[16];
static void __fastcall PartyMemberPrintStat(BYTE* surface, DWORD toWidth) {
	const char* fmt = "%s %d";
	char lvlMsg[16], acMsg[16];

	TGameObj* partyMember = (TGameObj*)*ptr_dialog_target;
	int xPos = 350;

	int level = PartyMemberGetCurrentLevel(partyMember);
	sprintf_s(lvlMsg, fmt, levelMsg, level);

	BYTE color = *(BYTE*)_GreenColor;
	int widthText = GetTextWidth(lvlMsg);
	PrintText(lvlMsg, color, xPos - widthText, 96, widthText, toWidth, surface);

	int ac = StatLevel(partyMember, STAT_ac);
	sprintf_s(acMsg, fmt, armorClassMsg, ac);

	xPos -= GetTextWidth(armorClassMsg) + 20;
	PrintText(acMsg, color, xPos, 167, GetTextWidth(acMsg), toWidth, surface);

	color = (QueueFindFirst(partyMember, 2)) ? *(BYTE*)_RedColor : *(BYTE*)_DarkGreenColor;
	widthText = GetTextWidth(addictMsg);
	PrintText(addictMsg, color, 350 - widthText, 148, widthText, toWidth, surface);
}

static void __declspec(naked) gdControlUpdateInfo_hook() {
	__asm {
		mov  edi, eax; // keep fontnum
		mov  ecx, ebp;
		mov  edx, esi;
		call PartyMemberPrintStat;
		mov  eax, edi;
		jmp  text_font_;
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
		dlog_f("  Mode %d, Chars read: %d.\n", DL_INIT, Mode, Chars.size());

		HookCall(0x46EBEE, &FidChangeHook);

		MakeJump(0x422354, CombatHack_add_noncoms_);
		HookCall(0x422D87, &CombatWrapper_v2);
		HookCall(0x422E20, &CombatWrapper_v2);

		HookCall(0x454218, &stat_pc_add_experience_hook); // call inside op_give_exp_points_hook
		HookCall(0x4124F1, &pc_flag_toggle_hook);
		HookCall(0x41279A, &pc_flag_toggle_hook);
	} else
		dlogr("  Disabled.", DL_INIT);

	// display party member's current level & AC & addict flag
	if (GetPrivateProfileIntA("Misc", "DisplayNPCExtraInfo", 0, ini)) {
		dlog("Applying display NPC extra info patch.", DL_INIT);
		HookCall(0x44926F, gdControlUpdateInfo_hook);
		GetPrivateProfileString("sfall", "PartyLvlMsg", "Lvl:", levelMsg, 12, translationIni);
		GetPrivateProfileString("sfall", "PartyACMsg", "AC:", armorClassMsg, 12, translationIni);
		GetPrivateProfileString("sfall", "PartyAddictMsg", "Addict", addictMsg, 16, translationIni);
		dlogr(" Done", DL_INIT);
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
