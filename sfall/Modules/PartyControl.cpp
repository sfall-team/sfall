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

//#include <algorithm>

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "HookScripts\InventoryHs.h"
#include "Drugs.h"
#include "HookScripts.h"
#include "LoadGameHook.h"

#include "PartyControl.h"

namespace sfall
{

bool npcAutoLevelEnabled;
bool npcEngineLevelUp = true;

bool isControllingNPC = false;
bool skipCounterAnim  = false;

static int delayedExperience;
static bool switchHandHookInjected = false;

struct WeaponStateSlot {
	long npcID;
	bool leftIsCopy  = false;
	bool rightIsCopy = false;
	fo::ItemButtonItem leftSlot;
	fo::ItemButtonItem rightSlot;
};
std::vector<WeaponStateSlot> weaponState;

static struct DudeState {
	fo::GameObject* obj_dude = nullptr;
	DWORD art_vault_guy_num;
	long traits[2];
	char pc_name[sizeof(fo::var::pc_name)];
	DWORD last_level;
	DWORD Level;
	DWORD Experience;
	char free_perk;
	DWORD unspent_skill_points;
	//DWORD map_elevation;
	DWORD sneak_working;
	//DWORD sneak_queue_time;
	DWORD itemCurrentItem;
	fo::ItemButtonItem itemButtonItems[2];
	long perkLevelDataList[fo::PERK_count];
	long addictGvar[8];
	long tag_skill[4];
	//DWORD bbox_sneak;
	long* extendAddictGvar = nullptr;
} realDude;

static void SaveAddictGvarState() {
	int n = 0;
	for (int i = 0; i < Drugs::GetDrugCount(); i++) {
		long gvarID = Drugs::GetDrugGvar(i);
		if (gvarID > 0) realDude.extendAddictGvar[n++] = fo::var::game_global_vars[gvarID];
	}
}

static void RestoreAddictGvarState() {
	int n = 0;
	for (int i = 0; i < Drugs::GetDrugCount(); i++) {
		long gvarID = Drugs::GetDrugGvar(i);
		if (gvarID > 0) fo::var::game_global_vars[gvarID] = realDude.extendAddictGvar[n++];
	}
}

static bool SetAddictGvar(fo::GameObject* npc) {
	bool isAddict = false;
	int count = Drugs::GetDrugCount();
	for (int i = 0; i < count; i++) {
		long gvarID = Drugs::GetDrugGvar(i);
		if (gvarID > 0) fo::var::game_global_vars[gvarID] = 0;
	}
	for (int i = 0; i < count; i++) {
		long pid = Drugs::GetDrugPid(i);
		if (pid > 0) {
			long gvarID = Drugs::GetDrugGvar(i);
			if (gvarID <= 0 || !fo::CheckAddictByPid(npc, pid)) continue;
			fo::var::game_global_vars[gvarID] = 1;
			isAddict = true;
		}
	}
	return isAddict;
}

// saves the state of PC before moving control to NPC
static void SaveRealDudeState() {
	realDude.obj_dude = fo::var::obj_dude;
	realDude.art_vault_guy_num = fo::var::art_vault_guy_num;
	realDude.itemCurrentItem = fo::var::itemCurrentItem;
	memcpy(realDude.itemButtonItems, fo::var::itemButtonItems, sizeof(fo::ItemButtonItem) * 2);
	memcpy(realDude.traits, fo::var::pc_trait, sizeof(long) * 2);
	memcpy(realDude.perkLevelDataList, fo::var::perkLevelDataList, sizeof(DWORD) * fo::PERK_count);
	strcpy_s(realDude.pc_name, sizeof(fo::var::pc_name), fo::var::pc_name);
	realDude.Level = fo::var::Level_;
	realDude.last_level = fo::var::last_level;
	realDude.Experience = fo::var::Experience_;
	realDude.free_perk = fo::var::free_perk;
	realDude.unspent_skill_points = fo::var::curr_pc_stat[0];
	realDude.sneak_working = fo::var::sneak_working;
	fo::SkillGetTags(realDude.tag_skill, 4);

	for (int i = 0; i < 6; i++) realDude.addictGvar[i] = fo::var::game_global_vars[fo::var::drugInfoList[i].addictGvar];
	realDude.addictGvar[6] = fo::var::game_global_vars[fo::var::drugInfoList[7].addictGvar];
	realDude.addictGvar[7] = fo::var::game_global_vars[fo::var::drugInfoList[8].addictGvar];
	if (realDude.extendAddictGvar) SaveAddictGvarState();

	if (skipCounterAnim) SafeWriteBatch<BYTE>(0, {0x422BDE, 0x4229EC}); // no animate
}

// take control of the NPC
static void SetCurrentDude(fo::GameObject* npc) {
	// remove skill tags
	long tagSkill[4];
	std::fill(std::begin(tagSkill), std::end(tagSkill), -1);
	fo::SkillSetTags(tagSkill, 4);

	// reset traits
	fo::var::pc_trait[0] = fo::var::pc_trait[1] = -1;

	// copy existing party member perks or reset list for non-party member NPC
	long isPartyMember = fo::IsPartyMemberByPid(npc->protoId);
	if (isPartyMember) {
		memcpy(fo::var::perkLevelDataList, fo::var::perkLevelDataList + (fo::PERK_count * (isPartyMember - 1)), sizeof(DWORD) * fo::PERK_count);
	} else {
		for (int i = 0; i < fo::PERK_count; i++) {
			fo::var::perkLevelDataList[i] = 0;
		}
	}

	// change level
	int level = (isPartyMember) // fo::func::isPartyMember(npc)
				? fo::func::partyMemberGetCurLevel(npc)
				: 0;

	fo::var::Level_ = level;
	fo::var::last_level = level;

	// change character name
	fo::func::critter_pc_set_name(fo::func::critter_name(npc));

	// reset other stats
	fo::var::Experience_ = 0;
	fo::var::free_perk = 0;
	fo::var::curr_pc_stat[0] = 0;
	fo::var::sneak_working = 0;

	// deduce active hand by weapon anim code
	char critterAnim = (npc->artFid & 0xF000) >> 12; // current weapon as seen in hands
	if (fo::AnimCodeByWeapon(fo::func::inven_left_hand(npc)) == critterAnim) { // definitely left hand
		fo::var::itemCurrentItem = fo::ActiveSlot::Left;
	} else {
		fo::var::itemCurrentItem = fo::ActiveSlot::Right;
	}
	// restore selected weapon mode
	size_t count = weaponState.size();
	for (size_t i = 0; i < count; i++) {
		if (weaponState[i].npcID == npc->id) {
			bool isMatch = false;
			if (weaponState[i].leftIsCopy) {
				auto item = fo::func::inven_left_hand(npc);
				if (item && item->protoId == weaponState[i].leftSlot.item->protoId) {
					memcpy(&fo::var::itemButtonItems[0], &weaponState[i].leftSlot, 0x14);
					isMatch = true;
				}
				weaponState[i].leftIsCopy = false;
			}
			if (weaponState[i].rightIsCopy) {
				auto item = fo::func::inven_right_hand(npc);
				if (item && item->protoId == weaponState[i].rightSlot.item->protoId) {
					memcpy(&fo::var::itemButtonItems[1], &weaponState[i].rightSlot, 0x14);
					isMatch = true;
				}
				weaponState[i].rightIsCopy = false;
			}
			if (!isMatch) {
				if (i < count - 1) weaponState[i] = weaponState.back();
				weaponState.pop_back();
			}
			break;
		}
	}

	bool isAddict = false;
	for (int i = 0; i < 9; i++) fo::var::game_global_vars[fo::var::drugInfoList[i].addictGvar] = 0;
	for (int i = 0; i < 9; i++) {
		if (!fo::CheckAddictByPid(npc, fo::var::drugInfoList[i].itemPid)) continue;
		fo::var::game_global_vars[fo::var::drugInfoList[i].addictGvar] = 1;
		isAddict = true;
	}
	if (realDude.extendAddictGvar) isAddict |= SetAddictGvar(npc); // check new added addictions
	fo::ToggleNpcFlag(npc, 4, isAddict); // for show/hide addiction box (fix bug)

	// switch main dude_obj pointers - this should be done last!
	fo::var::obj_dude = npc;
	fo::var::inven_dude = npc;
	fo::var::inven_pid = npc->protoId;
	fo::var::art_vault_guy_num = npc->artFid & 0xFFF;

	isControllingNPC = true;
	delayedExperience = 0;

	fo::func::intface_redraw();
}

// restores the real dude state
static void RestoreRealDudeState() {
	assert(realDude.obj_dude != nullptr);

	fo::var::map_elevation = realDude.obj_dude->elevation;

	fo::var::obj_dude = realDude.obj_dude;
	fo::var::inven_dude = realDude.obj_dude;
	fo::var::inven_pid = realDude.obj_dude->protoId;
	fo::var::art_vault_guy_num = realDude.art_vault_guy_num;

	fo::var::itemCurrentItem = realDude.itemCurrentItem;
	memcpy(fo::var::itemButtonItems, realDude.itemButtonItems, sizeof(DWORD) * 6 * 2);
	memcpy(fo::var::pc_trait, realDude.traits, sizeof(long) * 2);
	memcpy(fo::var::perkLevelDataList, realDude.perkLevelDataList, sizeof(DWORD) * fo::PERK_count);
	strcpy_s(fo::var::pc_name, sizeof(fo::var::pc_name), realDude.pc_name);
	fo::var::Level_ = realDude.Level;
	fo::var::last_level = realDude.last_level;
	fo::var::Experience_ = realDude.Experience;
	fo::var::free_perk = realDude.free_perk;
	fo::var::curr_pc_stat[0] = realDude.unspent_skill_points;
	fo::var::sneak_working = realDude.sneak_working;
	fo::SkillSetTags(realDude.tag_skill, 4);

	if (delayedExperience > 0) {
		fo::func::stat_pc_add_experience(delayedExperience);
	}

	for (int i = 0; i < 6; i++) fo::var::game_global_vars[fo::var::drugInfoList[i].addictGvar] = realDude.addictGvar[i];
	fo::var::game_global_vars[fo::var::drugInfoList[7].addictGvar] = realDude.addictGvar[6];
	fo::var::game_global_vars[fo::var::drugInfoList[8].addictGvar] = realDude.addictGvar[7];
	if (realDude.extendAddictGvar) RestoreAddictGvarState();

	if (skipCounterAnim) SafeWriteBatch<BYTE>(1, {0x422BDE, 0x4229EC}); // restore
	fo::func::intface_redraw();

	isControllingNPC = false;
}

static void __stdcall DisplayCantDoThat() {
	fo::func::display_print(fo::GetMessageStr(&fo::var::proto_main_msg_file, 675)); // I Can't do that
}

// 1 skip handler, -1 don't skip
int __stdcall PartyControl::SwitchHandHook(fo::GameObject* item) {
	// don't allow to use the weapon, if no art exist for it
	if (isControllingNPC && fo::func::item_get_type(item) == fo::ItemType::item_type_weapon) {
		int fId = fo::var::obj_dude->artFid;
		long weaponCode = fo::AnimCodeByWeapon(item);
		fId = (fId & 0xFFFF0FFF) | (weaponCode << 12);
		if (!fo::func::art_exists(fId)) {
			DisplayCantDoThat();
			return 1;
		}
	}
	return -1;
}

long __fastcall GetRealDudePerk(fo::GameObject* source, long perk) {
	if (isControllingNPC && source == realDude.obj_dude) {
		return realDude.perkLevelDataList[perk];
	}
	return fo::func::perk_level(source, perk);
}

long __fastcall GetRealDudeTrait(fo::GameObject* source, long trait) {
	if (isControllingNPC && source == realDude.obj_dude) {
		return (trait == realDude.traits[0] || trait == realDude.traits[1]) ? 1 : 0;
	}
	return fo::func::trait_level(trait);
}

static void __declspec(naked) stat_pc_add_experience_hook() {
	__asm {
		cmp  isControllingNPC, 0;
		je   skip;
		add  delayedExperience, esi;
		retn;
skip:
		xchg esi, eax;
		jmp  fo::funcoffs::stat_pc_add_experience_;
	}
}

// prevents using sneak when controlling NPCs
static void __declspec(naked) pc_flag_toggle_hook() {
	__asm {
		cmp  isControllingNPC, 0;
		je   end;
		call DisplayCantDoThat;
		retn;
end:
		jmp  fo::funcoffs::pc_flag_toggle_;
	}
}

void __stdcall PartyControlReset() {
	if (realDude.obj_dude != nullptr && isControllingNPC) {
		RestoreRealDudeState();
	}
	weaponState.clear();
}

bool PartyControl::IsNpcControlled() {
	return isControllingNPC;
}

bool CopyItemSlots(WeaponStateSlot &element, bool isSwap) {
	bool isCopy = false;
	if (fo::var::itemButtonItems[0 + isSwap].itsWeapon && fo::var::itemButtonItems[0 + isSwap].item) {
		memcpy(&element.leftSlot, &fo::var::itemButtonItems[0 + isSwap], 0x14);
		element.leftIsCopy = isCopy = true;
	}
	if (fo::var::itemButtonItems[1 - isSwap].itsWeapon && fo::var::itemButtonItems[1 - isSwap].item) {
		memcpy(&element.rightSlot, &fo::var::itemButtonItems[1 - isSwap], 0x14);
		element.rightIsCopy = isCopy = true;
	}
	if (isSwap && isCopy) {
		if (element.leftIsCopy) {
			element.leftSlot.primaryAttack = fo::AttackType::ATKTYPE_LWEAPON_PRIMARY;
			element.leftSlot.secondaryAttack = fo::AttackType::ATKTYPE_LWEAPON_SECONDARY;
		}
		if (element.rightIsCopy) {
			element.rightSlot.primaryAttack = fo::AttackType::ATKTYPE_RWEAPON_PRIMARY;
			element.rightSlot.secondaryAttack = fo::AttackType::ATKTYPE_RWEAPON_SECONDARY;
		}
	}
	return isCopy;
}

void SaveWeaponMode(bool isSwap) {
	for (size_t i = 0; i < weaponState.size(); i++) {
		if (weaponState[i].npcID == fo::var::obj_dude->id) {
			CopyItemSlots(weaponState[i], isSwap);
			return;
		}
	}
	WeaponStateSlot wState;
	if (CopyItemSlots(wState, isSwap)) {
		wState.npcID = fo::var::obj_dude->id;
		weaponState.push_back(wState);
	}
}

void PartyControl::SwitchToCritter(fo::GameObject* critter) {
	if (isControllingNPC) {
		bool isSwap = false;
		if (fo::var::itemCurrentItem == fo::ActiveSlot::Left) {
			// set active left item to right slot
			fo::GameObject* lItem = fo::func::inven_left_hand(fo::var::obj_dude);
			if (lItem) {
				isSwap = true;
				fo::GameObject* rItem = fo::func::inven_right_hand(fo::var::obj_dude);
				lItem->flags &= ~fo::ObjectFlag::Left_Hand;
				lItem->flags |= fo::ObjectFlag::Right_Hand;
				if (rItem) {
					rItem->flags &= ~fo::ObjectFlag::Right_Hand;
					rItem->flags |= fo::ObjectFlag::Left_Hand;
				}
			}
		}
		SaveWeaponMode(isSwap);
		if (critter == nullptr || critter == realDude.obj_dude) RestoreRealDudeState();
	} else {
		SaveRealDudeState();
	}
	if (critter != nullptr && critter != realDude.obj_dude) {
		SetCurrentDude(critter);
		if (switchHandHookInjected) return;
		switchHandHookInjected = true;
		if (!HookScripts::IsInjectHook(HOOK_INVENTORYMOVE)) Inject_SwitchHandHook();
		// Gets dude perks and traits from script while controlling another NPC
		// WARNING: Handling dude perks/traits in the engine code while controlling another NPC remains impossible, this requires serious hacking of the engine code
		HookCall(0x458242, GetRealDudePerk);  // op_has_trait_
		HookCall(0x458326, GetRealDudeTrait); // op_has_trait_
	}
}

fo::GameObject* PartyControl::RealDudeObject() {
	return realDude.obj_dude != nullptr
		? realDude.obj_dude
		: fo::var::obj_dude;
}

static char levelMsg[12], armorClassMsg[12], addictMsg[16];
static void __fastcall PartyMemberPrintStat(BYTE* surface, DWORD toWidth) {
	const char* fmt = "%s %d";
	char lvlMsg[16], acMsg[16];

	fo::GameObject* partyMember = (fo::GameObject*)fo::var::dialog_target;
	int xPos = 350;

	int level = fo::func::partyMemberGetCurLevel(partyMember);
	sprintf_s(lvlMsg, fmt, levelMsg, level);

	BYTE color = fo::var::GreenColor;
	int widthText = fo::GetTextWidth(lvlMsg);
	fo::PrintText(lvlMsg, color, xPos - widthText, 96, widthText, toWidth, surface);

	int ac = fo::func::stat_level(partyMember, fo::STAT_ac);
	sprintf_s(acMsg, fmt, armorClassMsg, ac);

	xPos -= fo::GetTextWidth(armorClassMsg) + 20;
	fo::PrintText(acMsg, color, xPos, 167, fo::GetTextWidth(acMsg), toWidth, surface);

	if (fo::func::queue_find_first(partyMember, 2)) {
		color = fo::var::RedColor;
		widthText = fo::GetTextWidth(addictMsg);
		fo::PrintText(addictMsg, color, 350 - widthText, 148, widthText, toWidth, surface);
	}
}

static void __declspec(naked) gdControlUpdateInfo_hook() {
	__asm {
		mov  edi, eax; // keep fontnum
		mov  ecx, ebp;
		mov  edx, esi;
		call PartyMemberPrintStat;
		mov  eax, edi;
		jmp  fo::funcoffs::text_font_;
	}
}

static void NpcAutoLevelPatch() {
	npcAutoLevelEnabled = GetConfigInt("Misc", "NPCAutoLevel", 0) != 0;
	if (npcAutoLevelEnabled) {
		dlog("Applying NPC autolevel patch.", DL_INIT);
		SafeWrite8(0x495CFB, 0xEB); // jmps 0x495D28 (skip random check)
		dlogr(" Done", DL_INIT);
	}
}

void PartyControl::init() {
	LoadGameHook::OnGameReset() += []() {
		PartyControlReset();
		if (!npcEngineLevelUp) {
			npcEngineLevelUp = true;
			SafeWrite16(0x4AFC1C, 0x840F);
		}
	};

	if (Drugs::addictionGvarCount) realDude.extendAddictGvar = new long[Drugs::addictionGvarCount];

	HookCall(0x454218, stat_pc_add_experience_hook); // call inside op_give_exp_points_hook
	HookCalls(pc_flag_toggle_hook, { 0x4124F1, 0x41279A });

	NpcAutoLevelPatch();

	skipCounterAnim = (GetConfigInt("Misc", "SpeedInterfaceCounterAnims", 0) == 3);

	// display party member's current level & AC & addict flag
	if (GetConfigInt("Misc", "PartyMemberExtraInfo", 0)) {
		dlog("Applying display NPC extra info patch.", DL_INIT);
		HookCall(0x44926F, gdControlUpdateInfo_hook);
		Translate("sfall", "PartyLvlMsg", "Lvl:", levelMsg, 12);
		Translate("sfall", "PartyACMsg", "AC:", armorClassMsg, 12);
		Translate("sfall", "PartyAddictMsg", "Addict", addictMsg, 16);
		dlogr(" Done", DL_INIT);
	}
}

void PartyControl::exit() {
	if (realDude.extendAddictGvar) delete[] realDude.extendAddictGvar;
}

}
