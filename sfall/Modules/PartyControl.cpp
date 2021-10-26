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

#include "..\main.h"
#include "..\FalloutEngine\Fallout2.h"
#include "..\Translate.h"

#include "PartyControl.h"

namespace sfall
{

bool npcAutoLevelEnabled;
bool npcEngineLevelUp = true;

static DWORD Mode;
static bool isControllingNPC = false;
static char skipCounterAnim;
static std::vector<WORD> Chars;
static int delayedExperience;

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
	//long addictGvar[8];
	long tag_skill[4];
	//DWORD bbox_sneak;

	DudeState() : obj_dude(nullptr) {}
} realDude;

static bool __stdcall IsInPidList(fo::GameObject* obj) {
	int pid = obj->protoId & 0xFFFFFF;
	for (std::vector<WORD>::iterator it = Chars.begin(); it != Chars.end(); ++it) {
		if (*it == pid) {
			return true;
		}
	}
	return false;
}

static void __stdcall SetInventoryCheck(bool skip) {
	if (skip) {
		SafeWrite16(0x46E7CD, 0x9090); // Inventory check
		SafeWrite32(0x46E7CF, 0x90909090);
	} else {
		SafeWrite16(0x46E7CD, 0x850F); // Inventory check
		SafeWrite32(0x46E7CF, 0x4B1);
	}
}

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

	if (skipCounterAnim == 1) {
		skipCounterAnim++;
		SafeWrite8(0x422BDE, 0); // no animate
	}
	if (isDebug) fo::func::debug_printf("\n[SFALL] Save dude state.");
}

// take control of the NPC
static void TakeControlOfNPC(fo::GameObject* npc) {
	if (isDebug) fo::func::debug_printf("\n[SFALL] Take control of critter.");

	// remove skill tags
	long tagSkill[4];
	std::fill(std::begin(tagSkill), std::end(tagSkill), -1);
	fo::util::SkillSetTags(tagSkill, 4);

	// reset traits
	fo::ptr::pc_trait[0] = fo::ptr::pc_trait[1] = -1;

	// reset perks (except Awareness)
	std::memset(&(*fo::ptr::perkLevelDataList)[0].perkData[1], 0, sizeof(DWORD) * (fo::PERK_count - 1));

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

	// switch main dude_obj pointers - this should be done last!
	*fo::ptr::combat_turn_obj = npc;
	*fo::ptr::obj_dude = npc;
	*fo::ptr::inven_dude = npc;
	*fo::ptr::inven_pid = npc->protoId;
	*fo::ptr::art_vault_guy_num = npc->artFid & 0xFFF;

	isControllingNPC = true;
	delayedExperience = 0;
	SetInventoryCheck(true);

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

	if (redraw) {
		if (skipCounterAnim == 2) {
			skipCounterAnim--;
			SafeWrite8(0x422BDE, 1); // restore
		}
		fo::func::intface_redraw();
	}

	SetInventoryCheck(false);
	isControllingNPC = false;

	if (isDebug) fo::func::debug_printf("\n[SFALL] Restore control to dude.\n");
}

static long __stdcall CombatTurn(fo::GameObject* obj) {
	__asm {
		mov  eax, obj;
		call fo::funcoffs::combat_turn_;
	}
}

// return values: 0 - use vanilla handler, 1 - skip vanilla handler, return 0 (normal status), -1 - skip vanilla, return -1 (game ended)
static long __stdcall CombatWrapperInner(fo::GameObject* obj) {
	if ((obj != *fo::ptr::obj_dude) && (Chars.size() == 0 || IsInPidList(obj)) && (Mode == 1 || fo::func::isPartyMember(obj))) {
		// save "real" dude state
		SaveRealDudeState();
		TakeControlOfNPC(obj);

		// Do combat turn
		long turnResult = CombatTurn(obj);

		// restore state
		if (isControllingNPC) { // if game was loaded during turn, PartyControlReset() was called and already restored state
			RestoreRealDudeState();
		}
		// -1 means that combat ended during turn
		return (turnResult == -1) ? -1 : 1;
	}
	return 0;
}


// this hook fixes NPCs art switched to main dude art after inventory screen closes
static void __declspec(naked) FidChangeHook() {
	__asm {
		cmp  isControllingNPC, 0;
		je   skip;
		push eax;
		mov  eax, [eax + 0x20]; // current fid
		and  eax, 0xFFFF0FFF;
		and  edx, 0x0000F000;
		or   edx, eax; // only change one octet with weapon type
		pop  eax;
skip:
		call fo::funcoffs::obj_change_fid_;
		retn;
	}
}

static void __stdcall DisplayCantDoThat() {
	fo::func::display_print(fo::util::GetMessageStr(fo::ptr::proto_main_msg_file, 675)); // I Can't do that
}

// 1 skip handler, -1 don't skip
int __fastcall PartyControl_SwitchHandHook(fo::GameObject* item) {
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

static void __declspec(naked) CombatWrapper_v2() {
	__asm {
		sub  esp, 4;
		pushad;
		push eax;
		call CombatWrapperInner;
		mov  [esp + 32], eax;
		popad;
		add  esp, 4;
		cmp  [esp - 4], 0;
		je   gonormal;
		cmp  [esp - 4], -1;
		je   combatend;
		xor  eax, eax;
		retn;
combatend:
		mov  eax, -1; // don't continue combat, as the game was loaded
		retn;
gonormal:
		jmp  fo::funcoffs::combat_turn_;
	}
}

// hack to exit from combat_add_noncoms function without crashing when you load game during PM/NPC turn
static void __declspec(naked) combat_add_noncoms_hook() {
	__asm {
		call CombatWrapper_v2;
		inc  eax;
		jnz  end; // jump if return value != -1
		mov  ds:[FO_VAR_list_com], eax; // eax = 0
		mov  ecx, [esp + 4]; // list
end:
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
}

bool PartyControl_IsNpcControlled() {
	return isControllingNPC;
}

fo::GameObject* PartyControl_RealDudeObject() {
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

static void NpcAutoLevelPatch() {
	npcAutoLevelEnabled = GetConfigInt("Misc", "NPCAutoLevel", 0) != 0;
	if (npcAutoLevelEnabled) {
		dlog("Applying NPC autolevel patch.", DL_INIT);
		SafeWrite8(0x495CFB, CodeType::JumpShort); // jmps 0x495D28 (skip random check)
		dlogr(" Done", DL_INIT);
	}
}

void PartyControl_OnGameLoad() {
	PartyControlReset();
	if (!npcEngineLevelUp) {
		npcEngineLevelUp = true;
		SafeWrite16(0x4AFC1C, 0x840F);
	}
}

void PartyControl_Init() {
	Mode = GetConfigInt("Misc", "ControlCombat", 0);
	if (Mode > 2) Mode = 0;
	if (Mode > 0) {
		dlogr("Initializing party control...", DL_INIT);
		char pidbuf[512];
		pidbuf[511] = 0;
		if (GetConfigString("Misc", "ControlCombatPIDList", "", pidbuf, 511)) {
			char* ptr = pidbuf;
			char* comma;
			while (true) {
				comma = strchr(ptr, ',');
				if (!comma) break;
				*comma = 0;
				if (strlen(ptr) > 0)
					Chars.push_back((WORD)strtoul(ptr, 0, 0));
				ptr = comma + 1;
			}
			if (strlen(ptr) > 0)
				Chars.push_back((WORD)strtoul(ptr, 0, 0));
		}
		dlog_f("  Mode %d, Chars read: %d.\n", DL_INIT, Mode, Chars.size());

		HookCall(0x46EBEE, FidChangeHook);

		HookCall(0x422354, combat_add_noncoms_hook);
		const DWORD combatWrapperAddr[] = {0x422D87, 0x422E20};
		HookCalls(CombatWrapper_v2, combatWrapperAddr);

		HookCall(0x454218, stat_pc_add_experience_hook); // call inside op_give_exp_points_hook
		const DWORD pcFlagToggleAddr[] = {0x4124F1, 0x41279A};
		HookCalls(pc_flag_toggle_hook, pcFlagToggleAddr);
		HookCall(0x49EB09, proto_name_hook);

		// Gets dude perks and traits from script while controlling another NPC
		// WARNING: Handling dude perks/traits in the engine code while controlling another NPC remains impossible, this requires serious hacking of the engine code
		HookCall(0x458242, GetRealDudePerk);  // op_has_trait_
		HookCall(0x458326, GetRealDudeTrait); // op_has_trait_
	}

	MakeCall(0x45F47C, intface_toggle_items_hack);

	NpcAutoLevelPatch();

	skipCounterAnim = (GetConfigInt("Misc", "SpeedInterfaceCounterAnims", 0) == 3) ? 1 : 0;

	// Display party member's current level & AC & addict flag
	if (GetConfigInt("Misc", "PartyMemberExtraInfo", 0)) {
		dlog("Applying display NPC extra info patch.", DL_INIT);
		HookCall(0x44926F, gdControlUpdateInfo_hook);
		Translate_Get("sfall", "PartyLvlMsg", "Lvl:", levelMsg, 12);
		Translate_Get("sfall", "PartyACMsg", "AC:", armorClassMsg, 12);
		Translate_Get("sfall", "PartyAddictMsg", "Addict", addictMsg, 16);
		dlogr(" Done", DL_INIT);
	}
}

}
