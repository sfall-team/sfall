/*
 *    sfall
 *    Copyright (C) 2008-2023  The sfall team
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
#include "..\Translate.h"

#include "..\Game\objects.h"
#include "..\Game\tilemap.h"

#include "PartyControl.h"

namespace sfall
{

bool npcAutoLevelEnabled;
bool npcEngineLevelUp = true;

static bool isControllingNPC = false;
static char skipCounterAnim;

static int delayedExperience;

struct WeaponStateSlot {
	long npcID;
	bool leftIsCopy;
	bool rightIsCopy;
	fo::ItemButtonItem leftSlot;
	fo::ItemButtonItem rightSlot;

	WeaponStateSlot() : leftIsCopy(false), rightIsCopy(false) {}
};
std::vector<WeaponStateSlot> weaponState;

static struct DudeState {
	fo::GameObject* obj_dude;
	DWORD art_vault_guy_num;
	long traits[2];
	char pc_name[32];
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
	bool isSaved;

	DudeState() : obj_dude(nullptr), isSaved(false) {}
} realDude;

// saves the state of PC before moving control to NPC
static void SaveRealDudeState() {
	realDude.obj_dude = *fo::ptr::obj_dude;
	realDude.art_vault_guy_num = *fo::ptr::art_vault_guy_num;
	realDude.itemCurrentItem = *fo::ptr::itemCurrentItem;
	std::memcpy(realDude.itemButtonItems, fo::ptr::itemButtonItems, sizeof(fo::ItemButtonItem) * 2);
	std::memcpy(realDude.traits, fo::ptr::pc_trait, sizeof(long) * 2);
	std::memcpy(realDude.perkLevelDataList, *fo::ptr::perkLevelDataList, sizeof(DWORD) * fo::PERK_count);
	strcpy_s(realDude.pc_name, 32, fo::ptr::pc_name);
	realDude.Level = *fo::ptr::Level_pc;
	realDude.last_level = *fo::ptr::last_level;
	realDude.Experience = *fo::ptr::Experience_pc;
	realDude.free_perk = *fo::ptr::free_perk;
	realDude.unspent_skill_points = fo::ptr::curr_pc_stat[fo::PCSTAT_unspent_skill_points];
	realDude.sneak_working = *fo::ptr::sneak_working;
	fo::util::SkillGetTags(realDude.tag_skill, 4);

	for (int i = 0; i < 6; i++) realDude.addictGvar[i] = (*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[i].addictGvar];
	realDude.addictGvar[6] = (*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[7].addictGvar];
	realDude.addictGvar[7] = (*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[8].addictGvar];

	realDude.isSaved = true;

	if (skipCounterAnim == 1) {
		skipCounterAnim++;
		SafeWrite8(0x422BDE, 0); // no animate
	}
	if (isDebug) fo::func::debug_printf("\n[SFALL] Save dude state.");
}

// take control of the NPC
static void SetCurrentDude(fo::GameObject* npc) {
	if (isDebug) fo::func::debug_printf("\n[SFALL] Take control of critter.");

	// remove skill tags
	long tagSkill[4];
	std::fill(std::begin(tagSkill), std::end(tagSkill), -1);
	fo::util::SkillSetTags(tagSkill, 4);

	// reset traits
	fo::ptr::pc_trait[0] = fo::ptr::pc_trait[1] = -1;

	// reset perks
	std::memset(*fo::ptr::perkLevelDataList, 0, sizeof(DWORD) * fo::PERK_count);

	// change level
	int level = fo::func::isPartyMember(npc)
	          ? fo::func::partyMemberGetCurLevel(npc)
	          : 0;

	*fo::ptr::Level_pc = level;
	*fo::ptr::last_level = level;

	// change character name
	fo::func::critter_pc_set_name(fo::func::critter_name(npc));

	// reset other stats
	*fo::ptr::Experience_pc = 0;
	*fo::ptr::free_perk = 0;
	fo::ptr::curr_pc_stat[fo::PCSTAT_unspent_skill_points] = 0;
	*fo::ptr::sneak_working = 0;

	// deduce active hand by weapon anim code
	char critterAnim = (npc->artFid & 0xF000) >> 12; // current weapon as seen in hands
	if (fo::util::AnimCodeByWeapon(fo::func::inven_left_hand(npc)) == critterAnim) { // definitely left hand
		*fo::ptr::itemCurrentItem = fo::HandSlot::Left;
	} else {
		*fo::ptr::itemCurrentItem = fo::HandSlot::Right;
	}
	// restore selected weapon mode
	size_t count = weaponState.size();
	for (size_t i = 0; i < count; i++) {
		if (weaponState[i].npcID == npc->id) {
			bool isMatch = false;
			if (weaponState[i].leftIsCopy) {
				fo::GameObject* item = fo::func::inven_left_hand(npc);
				if (item && item->protoId == weaponState[i].leftSlot.item->protoId) {
					std::memcpy(&fo::ptr::itemButtonItems[0], &weaponState[i].leftSlot, 0x14);
					isMatch = true;
				}
				weaponState[i].leftIsCopy = false;
			}
			if (weaponState[i].rightIsCopy) {
				fo::GameObject* item = fo::func::inven_right_hand(npc);
				if (item && item->protoId == weaponState[i].rightSlot.item->protoId) {
					std::memcpy(&fo::ptr::itemButtonItems[1], &weaponState[i].rightSlot, 0x14);
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
	for (int i = 0; i < 9; i++) (*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[i].addictGvar] = 0;
	for (int i = 0; i < 9; i++) {
		if (!fo::util::CheckAddictByPid(npc, fo::ptr::drugInfoList[i].itemPid)) continue;
		(*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[i].addictGvar] = 1;
		isAddict = true;
	}
	fo::util::ToggleNpcFlag(npc, 4, isAddict); // for show/hide addiction box (fix bug)

	// switch main dude_obj pointers - this should be done last!
	*fo::ptr::combat_turn_obj = npc;
	*fo::ptr::obj_dude = npc;
	*fo::ptr::inven_dude = npc;
	*fo::ptr::inven_pid = npc->protoId;
	*fo::ptr::art_vault_guy_num = npc->artFid & 0xFFF;

	isControllingNPC = true;
	delayedExperience = 0;

	//if (!isPartyMember) Objects::SetObjectUniqueID(npc);

	fo::func::intface_redraw();
}

// restores the real dude state
static void RestoreRealDudeState(bool redraw = true) {
	assert(realDude.obj_dude != nullptr);

	*fo::ptr::map_elevation = realDude.obj_dude->elevation;

	*fo::ptr::obj_dude = realDude.obj_dude;
	*fo::ptr::inven_dude = realDude.obj_dude;
	*fo::ptr::inven_pid = realDude.obj_dude->protoId;
	*fo::ptr::art_vault_guy_num = realDude.art_vault_guy_num;

	*fo::ptr::itemCurrentItem = realDude.itemCurrentItem;
	std::memcpy(fo::ptr::itemButtonItems, realDude.itemButtonItems, sizeof(fo::ItemButtonItem) * 2);
	std::memcpy(fo::ptr::pc_trait, realDude.traits, sizeof(long) * 2);
	std::memcpy(*fo::ptr::perkLevelDataList, realDude.perkLevelDataList, sizeof(DWORD) * fo::PERK_count);
	strcpy_s(fo::ptr::pc_name, 32, realDude.pc_name);
	*fo::ptr::Level_pc = realDude.Level;
	*fo::ptr::last_level = realDude.last_level;
	*fo::ptr::Experience_pc = realDude.Experience;
	*fo::ptr::free_perk = realDude.free_perk;
	fo::ptr::curr_pc_stat[fo::PCSTAT_unspent_skill_points] = realDude.unspent_skill_points;
	*fo::ptr::sneak_working = realDude.sneak_working;
	fo::util::SkillSetTags(realDude.tag_skill, 4);

	if (delayedExperience > 0) {
		fo::func::stat_pc_add_experience(delayedExperience);
	}

	for (int i = 0; i < 6; i++) (*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[i].addictGvar] = realDude.addictGvar[i];
	(*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[7].addictGvar] = realDude.addictGvar[6];
	(*fo::ptr::game_global_vars)[fo::ptr::drugInfoList[8].addictGvar] = realDude.addictGvar[7];

	if (redraw) {
		if (skipCounterAnim == 2) {
			skipCounterAnim--;
			SafeWrite8(0x422BDE, 1); // restore
		}
		fo::func::intface_redraw();
	}

	realDude.isSaved = false;
	isControllingNPC = false;

	if (isDebug) fo::func::debug_printf("\n[SFALL] Restore control to dude.\n");
}

static void __stdcall DisplayCantDoThat() {
	fo::func::display_print(fo::util::GetMessageStr(fo::ptr::proto_main_msg_file, 675)); // I Can't do that
}

// 1 skip handler, -1 don't skip
int __fastcall PartyControl::SwitchHandHook(fo::GameObject* item) {
	// don't allow to use the weapon, if no art exist for it
	if (/*isControllingNPC &&*/ fo::func::item_get_type(item) == fo::ItemType::item_type_weapon) {
		int fId = *fo::ptr::i_fid; //(*fo::ptr::obj_dude)->artFid;
		long weaponCode = fo::util::AnimCodeByWeapon(item);
		fId = (fId & 0xFFFF0FFF) | (weaponCode << 12);
		if (!fo::func::art_exists(fId)) {
			DisplayCantDoThat();
			return 1;
		}
	}
	return -1;
}

static long __fastcall GetRealDudePerk(fo::GameObject* source, long perk) {
	if (isControllingNPC && source == realDude.obj_dude) {
		return realDude.perkLevelDataList[perk];
	}
	return fo::func::perk_level(source, perk);
}

static long __fastcall GetRealDudeTrait(fo::GameObject* source, long trait) {
	if (isControllingNPC && source == realDude.obj_dude) {
		return (trait == realDude.traits[0] || trait == realDude.traits[1]) ? 1 : 0;
	}
	return fo::func::trait_level(trait);
}

static void __declspec(naked) inven_pickup_hook() {
	__asm {
		pushadc;
		mov  ecx, eax; // item
		call PartyControl::SwitchHandHook;
		test eax, eax;
		popadc;
		jns  skip; // eax > -1
		jmp  fo::funcoffs::switch_hand_;
skip:
		retn;
	}
}

static void __declspec(naked) stat_pc_add_experience_hook() {
	__asm {
		cmp  isControllingNPC, 0;
		jne  skip;
		xchg esi, eax;
		jmp  fo::funcoffs::stat_pc_add_experience_;
skip:
		add  delayedExperience, esi;
		retn;
	}
}

// prevents using sneak when controlling NPCs
static void __declspec(naked) pc_flag_toggle_hook() {
	__asm {
		cmp  isControllingNPC, 0;
		jne  near DisplayCantDoThat;
		jmp  fo::funcoffs::pc_flag_toggle_;
	}
}

// prevents equipping a weapon when the current appearance has no animation for it
static void __declspec(naked) intface_toggle_items_hack() {
	__asm {
//		cmp  isControllingNPC, 0;
//		jne  checkArt;
//		and  eax, 0x0F000;
//		retn;
//checkArt:
		mov  ebx, eax; // keep current dude fid
		push edx;      // weapon animation code
		and  ebx, 0x0F000;
		shl  edx, 12;
		and  eax, 0xFFFF0FFF;
		or   eax, edx;
		pop  edx;
		call fo::funcoffs::art_exists_;
		test eax, eax;
		jz   noArt;
		mov  eax, ebx;
		retn;
noArt:
		mov  eax, 1;
		sub  eax, ds:[FO_VAR_itemCurrentItem];
		mov  ds:[FO_VAR_itemCurrentItem], eax; // revert
		call DisplayCantDoThat;
		add  esp, 4; // destroy return addr
		pop  edx;
		pop  ecx;
		pop  ebx;
		retn;
	}
}

static void __declspec(naked) proto_name_hook() {
	__asm {
		cmp  isControllingNPC, 0;
		jne  pcName;
		jmp  fo::funcoffs::critter_name_;
pcName:
		lea  eax, realDude.pc_name;
		retn;
	}
}

static void PartyControlReset() {
	if (realDude.obj_dude != nullptr && isControllingNPC) {
		RestoreRealDudeState(false);
		if (skipCounterAnim == 2) {
			skipCounterAnim = 1; // skipCounterAnim--;
			SafeWrite8(0x422BDE, 1); // restore
		}
	}
	realDude.obj_dude = nullptr;
	realDude.isSaved = false;
	weaponState.clear();
}

bool PartyControl::IsNpcControlled() {
	return isControllingNPC;
}

static bool CopyItemSlots(WeaponStateSlot &element, bool isSwap) {
	bool isCopy = false;
	if (fo::ptr::itemButtonItems[0 + isSwap].itsWeapon && fo::ptr::itemButtonItems[0 + isSwap].item) {
		std::memcpy(&element.leftSlot, &fo::ptr::itemButtonItems[0 + isSwap], 0x14);
		element.leftIsCopy = isCopy = true;
	}
	if (fo::ptr::itemButtonItems[1 - isSwap].itsWeapon && fo::ptr::itemButtonItems[1 - isSwap].item) {
		std::memcpy(&element.rightSlot, &fo::ptr::itemButtonItems[1 - isSwap], 0x14);
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

static void SaveWeaponMode(bool isSwap) {
	for (size_t i = 0; i < weaponState.size(); i++) {
		if (weaponState[i].npcID == (*fo::ptr::obj_dude)->id) {
			CopyItemSlots(weaponState[i], isSwap);
			return;
		}
	}
	WeaponStateSlot wState;
	if (CopyItemSlots(wState, isSwap)) {
		wState.npcID = (*fo::ptr::obj_dude)->id;
		weaponState.push_back(wState);
	}
}

// Moves the weapon from the active left slot to the right slot and saves the selected weapon mode for NPC
static void NPCWeaponTweak() {
	bool isSwap = false;
	if (*fo::ptr::itemCurrentItem == fo::HandSlot::Left) {
		// set active left item to right slot
		fo::GameObject* lItem = fo::func::inven_left_hand(*fo::ptr::obj_dude);
		if (lItem) {
			isSwap = true;
			fo::GameObject* rItem = fo::func::inven_right_hand(*fo::ptr::obj_dude);
			lItem->flags &= ~fo::ObjectFlag::Left_Hand;
			lItem->flags |= fo::ObjectFlag::Right_Hand;
			if (rItem) {
				rItem->flags &= ~fo::ObjectFlag::Right_Hand;
				rItem->flags |= fo::ObjectFlag::Left_Hand;
			}
		}
	}
	SaveWeaponMode(isSwap);
}

void PartyControl::SwitchToCritter(fo::GameObject* critter) {
	static bool onlyOnce = false;
	if (skipCounterAnim == 2 && critter && critter == realDude.obj_dude) {
		skipCounterAnim--;
		SafeWrite8(0x422BDE, 1); // restore
	}

	if (isControllingNPC) {
		NPCWeaponTweak();
		if (critter == nullptr || critter == realDude.obj_dude) RestoreRealDudeState(critter != nullptr); // return control to dude
	}
	if (critter && critter != PartyControl::RealDudeObject()) {
		if (!isControllingNPC && !realDude.isSaved) {
			SaveRealDudeState();
		}
		SetCurrentDude(critter);

		if (!onlyOnce) {
			onlyOnce = true;

			HookCall(0x49EB09, proto_name_hook);

			// Gets dude perks and traits from script while controlling another NPC
			// WARNING: Handling dude perks/traits in the engine code while controlling another NPC remains impossible, this requires serious hacking of the engine code
			HookCall(0x458242, GetRealDudePerk);  // op_has_trait_
			HookCall(0x458326, GetRealDudeTrait); // op_has_trait_
		}
	}
}

fo::GameObject* PartyControl::RealDudeObject() {
	return realDude.obj_dude != nullptr
	       ? realDude.obj_dude
	       : *fo::ptr::obj_dude;
}

static char levelMsg[12], armorClassMsg[12], addictMsg[16];

static void __fastcall PartyMemberPrintStat(BYTE* surface, DWORD toWidth) {
	const char* fmt = "%s %d";
	char lvlMsg[16], acMsg[16];

	fo::GameObject* partyMember = *fo::ptr::dialog_target;
	int xPos = 350;

	int level = fo::func::partyMemberGetCurLevel(partyMember);
	sprintf_s(lvlMsg, fmt, levelMsg, level);

	BYTE color = *fo::ptr::GreenColor;
	int widthText = fo::util::GetTextWidth(lvlMsg);
	fo::util::PrintText(lvlMsg, color, xPos - widthText, 96, widthText, toWidth, surface);

	int ac = fo::func::stat_level(partyMember, fo::STAT_ac);
	sprintf_s(acMsg, fmt, armorClassMsg, ac);

	xPos -= fo::util::GetTextWidth(armorClassMsg) + 20;
	fo::util::PrintText(acMsg, color, xPos, 167, fo::util::GetTextWidth(acMsg), toWidth, surface);

	if (fo::func::queue_find_first(partyMember, 2)) {
		color = *fo::ptr::RedColor;
		widthText = fo::util::GetTextWidth(addictMsg);
		fo::util::PrintText(addictMsg, color, 350 - widthText, 148, widthText, toWidth, surface);
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

////////////////////////////////////////////////////////////////////////////////

static bool partyOrderPickTargetLoop;
static std::vector<std::string> partyOrderAttackMsg;

struct PartyTarget {
	fo::GameObject* member;
	fo::GameObject* target;
};

static std::vector<PartyTarget> partyOrderAttackTarget;

static fo::GameObject* GetMemberTarget(fo::GameObject* member) {
	for (std::vector<PartyTarget>::iterator it = partyOrderAttackTarget.begin(); it != partyOrderAttackTarget.end(); ++it) {
		if (it->member->id == member->id) {
			fo::GameObject* target = it->target;
			if (target && target->critter.IsDead()) it->target = nullptr;
			return it->target;
		}
	}
	return nullptr;
}

static void SetMemberTarget(fo::GameObject* member, fo::GameObject* target) {
	for (std::vector<PartyTarget>::iterator it = partyOrderAttackTarget.begin(); it != partyOrderAttackTarget.end(); ++it) {
		if (it->member->id == member->id) {
			it->target = target;
			return;
		}
	}
	PartyTarget pt = { member, target };
	partyOrderAttackTarget.push_back(pt);
}

// disables the display of the hit chance value when picking a target
static void __declspec(naked) gmouse_bk_process_hack() {
	__asm {
		mov  edx, -1; // mode
		mov  eax, ds:[FO_VAR_gmouse_3d_current_mode];
		test partyOrderPickTargetLoop, 0xFF;
		cmovnz eax, edx;
		retn;
	}
}

static void __fastcall action_attack_to(long unused, fo::GameObject* partyMember) {
	fo::func::gmouse_set_cursor(20);
	fo::func::gmouse_3d_set_mode(2);

	fo::GameObject* targetObject = nullptr;
	fo::GameObject* validTarget = nullptr;

	long outlineColor = 0; // backup color
	fo::BoundRect rect;
	partyOrderPickTargetLoop = true;

	do {
		fo::GameObject* underObject = fo::func::object_under_mouse(1, 0, *fo::ptr::map_elevation);

		if (targetObject && targetObject != underObject) {
			targetObject->outline = outlineColor;
			fo::func::obj_bound(targetObject, &rect);
			if (!outlineColor) {
				rect.x--;
				rect.y--;
				rect.offx += 2;
				rect.offy += 2;
			}
			fo::func::tile_refresh_rect(&rect, *fo::ptr::map_elevation);
			targetObject = validTarget = nullptr;
		}
		if (underObject && underObject != targetObject && underObject->IsCritter() && underObject->critter.teamNum != partyMember->critter.teamNum) {
			if (underObject->critter.IsNotDead()) {
				outlineColor = underObject->outline;

				if ((*fo::ptr::combatNumTurns || underObject->critter.combatState) &&
				    game::Objects::is_within_perception(partyMember, underObject, 0) && // HOOK_WITHINPERCEPTION
				    fo::func::make_path_func(partyMember, partyMember->tile, underObject->tile, 0, 0, game::Tilemap::obj_path_blocking_at_) > 0)
				{
					underObject->outline = 254 << 8; // flashing red
					validTarget = underObject;
				} else {
					underObject->outline = 10 << 8; // grey
				}
				fo::func::obj_bound(underObject, &rect);
				fo::func::tile_refresh_rect(&rect, *fo::ptr::map_elevation);
				targetObject = underObject;
			}
		}
		if (validTarget && *fo::ptr::mouse_buttons == 1) break; // left mouse button

	} while (*fo::ptr::mouse_buttons != 2 && fo::func::get_input() != 27); // 27 - escape code

	if (validTarget && *fo::ptr::mouse_buttons == 1) {
		SetMemberTarget(partyMember, validTarget);
		validTarget->outline = outlineColor;

		fo::AIcap* cap = fo::func::ai_cap(partyMember);
		if (cap->disposition == fo::AIpref::Disposition::DISP_custom) {
			cap->attack_who = fo::AIpref::AttackWho::ATKWHO_whomever;
		}

		// floating text messages
		const char* message;
		switch (fo::func::critter_body_type(partyMember)) {
		case fo::BodyType::Quadruped: // creatures
			message = partyOrderAttackMsg[0].c_str();
			break;
		case fo::BodyType::Robotic:
			message = partyOrderAttackMsg[1].c_str();
			break;
		default: // biped
			long max = partyOrderAttackMsg.size() - 1;
			long rnd = (max > 2) ? fo::func::roll_random(2, max) : 2;
			message = partyOrderAttackMsg[rnd].c_str();
		}
		fo::util::PrintFloatText(partyMember, message, cap->color, cap->outline_color, cap->font);
	}

	partyOrderPickTargetLoop = false;
	*fo::ptr::mouse_buttons = 0;

	fo::func::gmouse_set_cursor(0);
	fo::func::gmouse_3d_set_mode(1);
}

static void __declspec(naked) gmouse_handle_event_hook() {
	__asm {
		test ds:[FO_VAR_combat_state], 1;
		jnz  action_attack_to;
		jmp  fo::funcoffs::action_talk_to_;
	}
}

static void __declspec(naked) gmouse_handle_event_hack() {
	__asm {
		test ds:[FO_VAR_combat_state], 1;
		jnz  pick;
		retn;
pick:
		mov  eax, edi; // critter
		call fo::funcoffs::isPartyMember_;
		test eax, eax;
		jz   end;
		mov  bl, 1;
		mov  eax, 5;
		mov  [esp + 4], eax; // actionMenuList
		mov  word ptr ds:[FO_VAR_gmouse_3d_action_nums][5*2], 81; // index in INTRFACE.LST (ACTIONI.FRM)
end:
		or  eax, 1;
		retn;
	}
}

static void __declspec(naked) gmouse_handle_event_hook_restore() {
	__asm {
		mov word ptr ds:[FO_VAR_gmouse_3d_action_nums][5*2], 263; // index in INTRFACE.LST (TALKN.FRM)
		jmp fo::funcoffs::map_enable_bk_processes_;
	}
}

static void __fastcall SetOrderTarget(fo::GameObject* attacker) {
	if (attacker->critter.teamNum || partyOrderAttackTarget.empty()) return;
	fo::GameObject* target = GetMemberTarget(attacker);
	if (target) attacker->critter.whoHitMe = target;
}

static void __declspec(naked) combat_ai_hook_target() {
	__asm {
		push ecx;
		mov  ecx, eax;
		call SetOrderTarget;
		pop  ecx;
		mov  eax, esi;
		jmp  fo::funcoffs::ai_danger_source_;
	}
}

void PartyControl::OrderAttackPatch() {
	static bool orderAttackPatch = false;
	if (orderAttackPatch) return;

	MakeCall(0x44C4A7, gmouse_handle_event_hack, 2);
	HookCall(0x44C75F, gmouse_handle_event_hook);
	HookCall(0x44C69A, gmouse_handle_event_hook_restore);
	MakeCall(0x44B830, gmouse_bk_process_hack);

	HookCall(0x42B235, combat_ai_hook_target);
	orderAttackPatch = true;
}

static void NpcAutoLevelPatch() {
	npcAutoLevelEnabled = IniReader::GetConfigInt("Misc", "NPCAutoLevel", 0) != 0;
	if (npcAutoLevelEnabled) {
		dlogr("Applying NPC autolevel patch.", DL_INIT);
		SafeWrite8(0x495CFB, CodeType::JumpShort); // jmps 0x495D28 (skip random check)
	}
}

void PartyControl::OnCombatEnd() {
	partyOrderAttackTarget.clear();
}

void PartyControl::OnGameReset() {
	PartyControlReset();
	if (!npcEngineLevelUp) {
		npcEngineLevelUp = true;
		SafeWrite16(0x4AFC1C, 0x840F);
	}
}

void PartyControl::init() {
	if (IniReader::GetConfigInt("Misc", "PartyOrderToAttack", 0)) {
		dlogr("Applying order to attack patch for party members.", DL_INIT);
		PartyControl::OrderAttackPatch();
	}

	HookCall(0x454218, stat_pc_add_experience_hook); // call inside op_give_exp_points_hook
	const DWORD pcFlagToggleAddr[] = {0x4124F1, 0x41279A};
	HookCalls(pc_flag_toggle_hook, pcFlagToggleAddr);
	MakeCall(0x45F47C, intface_toggle_items_hack);
	const DWORD switchHandAddr[] = {0x4712E3, 0x47136D}; // left slot, right slot
	HookCalls(inven_pickup_hook, switchHandAddr); // will be overwritten if HOOK_INVENTORYMOVE is injected

	NpcAutoLevelPatch();

	skipCounterAnim = (IniReader::GetConfigInt("Misc", "SpeedInterfaceCounterAnims", 0) == 3) ? 1 : 0;

	// Display party member's current level & AC & addict flag
	if (IniReader::GetConfigInt("Misc", "PartyMemberExtraInfo", 0)) {
		dlogr("Applying display extra info patch for party members.", DL_INIT);
		HookCall(0x44926F, gdControlUpdateInfo_hook);
		Translate::Get("sfall", "PartyLvlMsg", "Lvl:", levelMsg, 12);
		Translate::Get("sfall", "PartyACMsg", "AC:", armorClassMsg, 12);
		Translate::Get("sfall", "PartyAddictMsg", "Addict", addictMsg, 16);
	}

	partyOrderAttackMsg.push_back(Translate::Get("sfall", "PartyOrderAttackCreature", "::Growl::"));
	partyOrderAttackMsg.push_back(Translate::Get("sfall", "PartyOrderAttackRobot", "::Beep::"));
	std::vector<std::string> msgs = Translate::GetList("sfall", "PartyOrderAttackHuman", "I'll take care of it.|Okay, I got it.", '|');
	partyOrderAttackMsg.insert(partyOrderAttackMsg.cend(), msgs.cbegin(), msgs.cend());
}

}
